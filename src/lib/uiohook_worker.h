#ifndef ADDON_SRC_UIOHOOK_WORKER_H_
#define ADDON_SRC_UIOHOOK_WORKER_H_

#include <uiohook.h>

#define UIOHOOK_ERROR_THREAD_CREATE				0x10

int uiohook_worker_start(dispatcher_t dispatch_proc);

int uiohook_worker_stop();

#endif // !ADDON_SRC_UIOHOOK_WORKER_H_
