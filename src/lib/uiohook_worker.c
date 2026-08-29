#include <stdarg.h>
#include <stdio.h>
#include <uiohook.h>
#include <uv.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

#include "uiohook_worker.h"

// Startup handshake state, guarded by hook_control_mutex.
//
// This replaces the previous use of hook_running_mutex as an implicit
// "is the hook running" flag. A mutex cannot carry that information race-free:
// trylock() answers "is it held right now", which is not the same question as
// "has the hook finished starting", and the two diverge on exactly the
// interleaving that used to deadlock (see hook_enable below).
typedef enum {
  HOOK_START_PENDING = 0,   // hook_run() has not reported anything yet
  HOOK_START_RUNNING,       // EVENT_HOOK_ENABLED was dispatched
  HOOK_START_FINISHED       // hook_run() returned; the thread is joinable
} hook_start_state;

// Thread and mutex variables.
static uv_thread_t hook_thread;
static int hook_thread_status;
static uv_mutex_t hook_control_mutex;
static uv_cond_t hook_control_cond;
static hook_start_state hook_state;

static dispatcher_t user_dispatcher = NULL;

bool logger_proc(unsigned int level, const char* format, ...) {
  bool status = false;

  va_list args;
  switch (level) {
  case LOG_LEVEL_WARN:
  case LOG_LEVEL_ERROR:
    va_start(args, format);
    status = vfprintf(stderr, format, args) >= 0;
    va_end(args);
    break;
  }

  return status;
}

// NOTE: The following callback executes on the same thread that hook_run() is called 
// from.  This is important because hook_run() attaches to the operating systems
// event dispatcher and may delay event delivery to the target application.
// Furthermore, some operating systems may choose to disable your hook if it 
// takes to long to process.  If you need to do any extended processing, please 
// do so by copying the event to your own queued dispatch thread.
void worker_dispatch_proc(uiohook_event* const event) {
  switch (event->type) {
  case EVENT_HOOK_ENABLED:
    // Publish the state change and wake hook_enable(). The mutex is held only
    // for the handoff itself, and is released before returning to hook_run().
    uv_mutex_lock(&hook_control_mutex);
    hook_state = HOOK_START_RUNNING;
    uv_cond_signal(&hook_control_cond);
    uv_mutex_unlock(&hook_control_mutex);
    break;

  case EVENT_KEY_PRESSED:
  case EVENT_KEY_RELEASED:
  // case EVENT_KEY_TYPED:
  case EVENT_MOUSE_CLICKED:
  case EVENT_MOUSE_PRESSED:
  case EVENT_MOUSE_RELEASED:
  case EVENT_MOUSE_MOVED:
  case EVENT_MOUSE_DRAGGED:
  case EVENT_MOUSE_WHEEL: {
    user_dispatcher(event);
    break;
  }

  default:
    break;
  }
}

void hook_thread_proc(void* arg) {
  #ifdef _WIN32
  // Attempt to set the thread priority to time critical.
  HANDLE this_thread = GetCurrentThread();
  if (SetThreadPriority(this_thread, THREAD_PRIORITY_TIME_CRITICAL) == FALSE) {
    logger_proc(LOG_LEVEL_WARN, "%s [%u]: Could not set thread priority %li for thread %#p! (%#lX)\n",
      __FUNCTION__, __LINE__, (long)THREAD_PRIORITY_TIME_CRITICAL,
      this_thread, (unsigned long)GetLastError());
  }
  #else
  // Raise the thread priority
  pthread_t this_thread = pthread_self();
  struct sched_param params = {
    .sched_priority = (sched_get_priority_max(SCHED_RR) / 2)
  };
  if (pthread_setschedparam(this_thread, SCHED_RR, &params) != 0) {
    logger_proc(LOG_LEVEL_WARN, "%s [%u]: Could not set thread priority %i for thread 0x%lX!\n",
      __FUNCTION__, __LINE__, params.sched_priority, (unsigned long)this_thread);
  }
  #endif

  // Set the hook status.
  hook_thread_status = hook_run();

  // hook_run() has returned, so the hook is no longer running whatever it
  // reported on the way in. This is set unconditionally, overwriting
  // HOOK_START_RUNNING if EVENT_HOOK_ENABLED was already dispatched: if the
  // hook enabled and then stopped before hook_enable() observed the first
  // signal, "the thread has exited" is the more useful answer, and it keeps
  // hook_enable() from returning success for a thread nobody will ever join.
  //
  // The mutex is acquired here rather than inherited from EVENT_HOOK_DISABLED:
  // unlocking a mutex this thread never locked is undefined behaviour, and
  // signalling without holding it allowed the wakeup to be lost entirely.
  uv_mutex_lock(&hook_control_mutex);
  hook_state = HOOK_START_FINISHED;
  uv_cond_signal(&hook_control_cond);
  uv_mutex_unlock(&hook_control_mutex);
}

int hook_enable() {
  // Lock the thread control mutex. This is held across the whole handshake so
  // the state cannot change between the wait and the decision.
  uv_mutex_lock(&hook_control_mutex);

  hook_state = HOOK_START_PENDING;

  if (uv_thread_create(&hook_thread, hook_thread_proc, NULL) != 0) {
    uv_mutex_unlock(&hook_control_mutex);
    return UIOHOOK_ERROR_THREAD_CREATE;
  }

  // Wait for the hook thread to either report itself enabled or exit.
  //
  // The predicate loop is required, not defensive: uv_cond_wait() is permitted
  // to wake spuriously. The previous code treated any wakeup as a decision
  // point and asked uv_mutex_trylock(&hook_running_mutex) what had happened. On
  // a premature wakeup that trylock SUCCEEDS -- the hook thread has not reached
  // EVENT_HOOK_ENABLED yet, so nothing holds the mutex -- which reads as
  // "startup problem" and calls uv_thread_join() while holding both mutexes the
  // hook thread needs to complete EVENT_HOOK_ENABLED. Neither side can proceed,
  // and for an Electron/GUI consumer the thread stuck in that join is the main
  // thread: the application hangs at startup and stops responding to signals.
  while (hook_state == HOOK_START_PENDING) {
    uv_cond_wait(&hook_control_cond, &hook_control_mutex);
  }

  hook_start_state state = hook_state;
  uv_mutex_unlock(&hook_control_mutex);

  if (state == HOOK_START_RUNNING) {
    return UIOHOOK_SUCCESS;
  }

  // The thread has returned from hook_run() -- either it failed to start, or it
  // started and stopped again before we got here. Both are reported by its own
  // status. The join cannot block on the handshake, and it deliberately runs
  // without the control mutex held, so it cannot block on anything the hook
  // thread might still want.
  uv_thread_join(&hook_thread);
  return hook_thread_status;
}


int uiohook_worker_start(dispatcher_t dispatch_proc) {
  // Lock the thread control mutex.  This will be unlocked when the
  // thread has finished starting, or when it has fully stopped.

  // Create event handles for the thread hook.
  uv_mutex_init(&hook_control_mutex);
  uv_cond_init(&hook_control_cond);

  // Set the logger callback for library output.
  hook_set_logger_proc(logger_proc);

  // Set the event callback for uiohook events.
  hook_set_dispatch_proc(worker_dispatch_proc);

  user_dispatcher = dispatch_proc;

  // Start the hook and block.
  // NOTE If EVENT_HOOK_ENABLED was delivered, the status will always succeed.
  int status = hook_enable();
  if (status != UIOHOOK_SUCCESS) {
    // Close event handles for the thread hook.
    uv_mutex_destroy(&hook_control_mutex);
    uv_cond_destroy(&hook_control_cond);
  }

  return status;
}

int uiohook_worker_stop() {
  int status = hook_stop();

  if (status == UIOHOOK_SUCCESS) {
    uv_thread_join(&hook_thread);

    // Close event handles for the thread hook.
    uv_mutex_destroy(&hook_control_mutex);
    uv_cond_destroy(&hook_control_cond);
  }

  return status;
}
