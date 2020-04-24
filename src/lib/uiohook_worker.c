#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <uiohook.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "uiohook_worker.h"

// Thread and mutex variables.
#ifdef _WIN32
static HANDLE hook_thread;

static CRITICAL_SECTION hook_running_mutex;
static CRITICAL_SECTION hook_control_mutex;
static CONDITION_VARIABLE hook_control_cond;
#else
static pthread_t hook_thread;

static pthread_mutex_t hook_running_mutex;
static pthread_mutex_t hook_control_mutex;
static pthread_cond_t hook_control_cond;
#endif

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
	// Lock the running mutex so we know if the hook is enabled.
	#ifdef _WIN32
	EnterCriticalSection(&hook_running_mutex);
	#else
	pthread_mutex_lock(&hook_running_mutex);
	#endif

	// Unlock the control mutex so hook_enable() can continue.
	#ifdef _WIN32
	EnterCriticalSection(&hook_control_mutex);
	WakeConditionVariable(&hook_control_cond);
	LeaveCriticalSection(&hook_control_mutex);
	#else
	pthread_mutex_lock(&hook_control_mutex);
	pthread_cond_signal(&hook_control_cond);
	pthread_mutex_unlock(&hook_control_mutex);
	#endif
	break;

  case EVENT_HOOK_DISABLED:
	// Lock the control mutex until we exit.
	#ifdef _WIN32
	EnterCriticalSection(&hook_control_mutex);
	#else
	pthread_mutex_lock(&hook_control_mutex);
	#endif

	// Unlock the running mutex so we know if the hook is disabled.
	#ifdef _WIN32
	LeaveCriticalSection(&hook_running_mutex);
	#else
	pthread_mutex_unlock(&hook_running_mutex);
	#endif
	break;

  case EVENT_KEY_PRESSED:
  case EVENT_KEY_RELEASED:
  // case EVENT_KEY_TYPED:
  case EVENT_MOUSE_CLICKED:
  case EVENT_MOUSE_PRESSED:
  case EVENT_MOUSE_RELEASED:
  case EVENT_MOUSE_MOVED:
  // case EVENT_MOUSE_DRAGGED:
  case EVENT_MOUSE_WHEEL: {
    user_dispatcher(event);
    break;
  }

  default:
	break;
  }
}

#ifdef _WIN32
DWORD WINAPI hook_thread_proc(LPVOID arg) {
#else
void* hook_thread_proc(void* arg) {
#endif
  // Set the hook status.
  int status = hook_run();
  if (status != UIOHOOK_SUCCESS) {
    #ifdef _WIN32
	* (DWORD*)arg = status;
    #else
	* (int*)arg = status;
    #endif
  }

  // Make sure we signal that we have passed any exception throwing code for
  // the waiting hook_enable().
  #ifdef _WIN32
  WakeConditionVariable(&hook_control_cond);
  LeaveCriticalSection(&hook_control_mutex);

  return status;
  #else
  // Make sure we signal that we have passed any exception throwing code for
  // the waiting hook_enable().
  pthread_cond_signal(&hook_control_cond);
  pthread_mutex_unlock(&hook_control_mutex);

  return arg;
  #endif
}

int hook_enable() {
  // Lock the thread control mutex.  This will be unlocked when the
  // thread has finished starting, or when it has fully stopped.
  #ifdef _WIN32
  EnterCriticalSection(&hook_control_mutex);
  #else
  pthread_mutex_lock(&hook_control_mutex);
  #endif

  // Set the initial status.
  int status = UIOHOOK_FAILURE;

  #ifndef _WIN32
  // Create the thread attribute.
  pthread_attr_t hook_thread_attr;
  pthread_attr_init(&hook_thread_attr);

  // Get the policy and priority for the thread attr.
  int policy;
  pthread_attr_getschedpolicy(&hook_thread_attr, &policy);
  int priority = sched_get_priority_max(policy);
  #endif

  #if defined(_WIN32)
  DWORD hook_thread_id;
  DWORD* hook_thread_status = malloc(sizeof(DWORD));
  hook_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)hook_thread_proc, hook_thread_status, 0, &hook_thread_id);
  if (hook_thread != INVALID_HANDLE_VALUE) {
  #else
  int* hook_thread_status = malloc(sizeof(int));
  if (pthread_create(&hook_thread, &hook_thread_attr, hook_thread_proc, hook_thread_status) == 0) {
  #endif
    #if defined(_WIN32)
	// Attempt to set the thread priority to time critical.
	if (SetThreadPriority(hook_thread, THREAD_PRIORITY_TIME_CRITICAL) == 0) {
	  logger_proc(LOG_LEVEL_WARN, "%s [%u]: Could not set thread priority %li for thread %#p! (%#lX)\n",
		__FUNCTION__, __LINE__, (long)THREAD_PRIORITY_TIME_CRITICAL,
		hook_thread, (unsigned long)GetLastError());
	}
    #elif (defined(__APPLE__) && defined(__MACH__)) || _POSIX_C_SOURCE >= 200112L
	// Some POSIX revisions do not support pthread_setschedprio so we will 
	// use pthread_setschedparam instead.
	struct sched_param param = { .sched_priority = priority };
	if (pthread_setschedparam(hook_thread, SCHED_OTHER, &param) != 0) {
	  logger_proc(LOG_LEVEL_WARN, "%s [%u]: Could not set thread priority %i for thread 0x%lX!\n",
		__FUNCTION__, __LINE__, priority, (unsigned long)hook_thread);
	}
    #else
	// Raise the thread priority using glibc pthread_setschedprio.
	if (pthread_setschedprio(hook_thread, priority) != 0) {
	  logger_proc(LOG_LEVEL_WARN, "%s [%u]: Could not set thread priority %i for thread 0x%lX!\n",
		__FUNCTION__, __LINE__, priority, (unsigned long)hook_thread);
	}
    #endif


	// Wait for the thread to indicate that it has passed the 
	// initialization portion by blocking until either a EVENT_HOOK_ENABLED 
	// event is received or the thread terminates.
	// NOTE This unlocks the hook_control_mutex while we wait.
    #ifdef _WIN32
	SleepConditionVariableCS(&hook_control_cond, &hook_control_mutex, INFINITE);
    #else
	pthread_cond_wait(&hook_control_cond, &hook_control_mutex);
    #endif

    #ifdef _WIN32
	if (TryEnterCriticalSection(&hook_running_mutex) != FALSE) {
    #else
	if (pthread_mutex_trylock(&hook_running_mutex) == 0) {
    #endif
	  // Lock Successful; The hook is not running but the hook_control_cond 
	  // was signaled!  This indicates that there was a startup problem!

	  // Get the status back from the thread.
      #ifdef _WIN32
	  WaitForSingleObject(hook_thread, INFINITE);
	  GetExitCodeThread(hook_thread, hook_thread_status);
      #else
	  pthread_join(hook_thread, (void**)&hook_thread_status);
      #endif
      status = *hook_thread_status;
	}
	else {
	  // Lock Failure; The hook is currently running and wait was signaled
	  // indicating that we have passed all possible start checks.  We can 
	  // always assume a successful startup at this point.
	  status = UIOHOOK_SUCCESS;
	}

	free(hook_thread_status);

	logger_proc(LOG_LEVEL_DEBUG, "%s [%u]: Thread Result: (%#X).\n",
	  __FUNCTION__, __LINE__, status);
	}
  else {
	status = UIOHOOK_ERROR_THREAD_CREATE;
  }

  // Make sure the control mutex is unlocked.
  #ifdef _WIN32
  LeaveCriticalSection(&hook_control_mutex);
  #else
  pthread_mutex_unlock(&hook_control_mutex);
  #endif

  return status;
}


int uiohook_worker_start(dispatcher_t dispatch_proc) {
  // Lock the thread control mutex.  This will be unlocked when the
  // thread has finished starting, or when it has fully stopped.
  #ifdef _WIN32
  // Create event handles for the thread hook.
  InitializeCriticalSection(&hook_running_mutex);
  InitializeCriticalSection(&hook_control_mutex);
  InitializeConditionVariable(&hook_control_cond);
  #else
  pthread_mutex_init(&hook_running_mutex, NULL);
  pthread_mutex_init(&hook_control_mutex, NULL);
  pthread_cond_init(&hook_control_cond, NULL);
  #endif

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
    #ifdef _WIN32
    CloseHandle(hook_thread);
    DeleteCriticalSection(&hook_running_mutex);
    DeleteCriticalSection(&hook_control_mutex);
    #else
    pthread_mutex_destroy(&hook_running_mutex);
    pthread_mutex_destroy(&hook_control_mutex);
    pthread_cond_destroy(&hook_control_cond);
    #endif
  }

  return status;
}

int uiohook_worker_stop() {
  int status = hook_stop();

  if (status == UIOHOOK_SUCCESS) {
    #ifdef _WIN32
    WaitForSingleObject(hook_thread, INFINITE);
    #else
    pthread_join(hook_thread, NULL);
    #endif

    // Close event handles for the thread hook.
    #ifdef _WIN32
    CloseHandle(hook_thread);
    DeleteCriticalSection(&hook_running_mutex);
    DeleteCriticalSection(&hook_control_mutex);
    #else
    pthread_mutex_destroy(&hook_running_mutex);
    pthread_mutex_destroy(&hook_control_mutex);
    pthread_cond_destroy(&hook_control_cond);
    #endif
  }

  return status;
}
