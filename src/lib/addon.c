#include <stdlib.h>
#include <string.h>
#include <node_api.h>
#include <uiohook.h>
#include "napi_helpers.h"
#include "uiohook_worker.h"

static napi_threadsafe_function threadsafe_fn = NULL;
static bool is_worker_running = false;

void dispatch_proc(uiohook_event* const event) {
  if (threadsafe_fn == NULL) return;

  uiohook_event* copied_event = malloc(sizeof(uiohook_event));
  memcpy(copied_event, event, sizeof(uiohook_event));
  if (copied_event->type == EVENT_MOUSE_DRAGGED) {
    copied_event->type = EVENT_MOUSE_MOVED;
  }

  napi_status status = napi_call_threadsafe_function(threadsafe_fn, copied_event, napi_tsfn_nonblocking);
  if (status == napi_closing) {
    threadsafe_fn = NULL;
    free(copied_event);
    return;
  }
  NAPI_FATAL_IF_FAILED(status, "dispatch_proc", "napi_call_threadsafe_function");
}

napi_value uiohook_to_js_event(napi_env env, uiohook_event* event) {
  napi_status status;

  napi_value event_obj;
  status = napi_create_object(env, &event_obj);
  NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_object");

  napi_value e_type;
  status = napi_create_uint32(env, event->type, &e_type);
  NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_uint32");

  napi_value e_altKey;
  status = napi_get_boolean(env, (event->mask & (MASK_ALT)), &e_altKey);
  NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_get_boolean");

  napi_value e_ctrlKey;
  status = napi_get_boolean(env, (event->mask & (MASK_CTRL)), &e_ctrlKey);
  NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_get_boolean");

  napi_value e_metaKey;
  status = napi_get_boolean(env, (event->mask & (MASK_META)), &e_metaKey);
  NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_get_boolean");

  napi_value e_shiftKey;
  status = napi_get_boolean(env, (event->mask & (MASK_SHIFT)), &e_shiftKey);
  NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_get_boolean");

  napi_value e_time;
  status = napi_create_double(env, (double)event->time, &e_time);
  NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_double");

  if (event->type == EVENT_KEY_PRESSED || event->type == EVENT_KEY_RELEASED) {
    napi_value e_keycode;
    status = napi_create_uint32(env, event->data.keyboard.keycode, &e_keycode);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_uint32");

    napi_property_descriptor descriptors[] = {
      { "type",     NULL, NULL, NULL, NULL, e_type,     napi_enumerable, NULL },
      { "time",     NULL, NULL, NULL, NULL, e_time,     napi_enumerable, NULL },
      { "altKey",   NULL, NULL, NULL, NULL, e_altKey,   napi_enumerable, NULL },
      { "ctrlKey",  NULL, NULL, NULL, NULL, e_ctrlKey,  napi_enumerable, NULL },
      { "metaKey",  NULL, NULL, NULL, NULL, e_metaKey,  napi_enumerable, NULL },
      { "shiftKey", NULL, NULL, NULL, NULL, e_shiftKey, napi_enumerable, NULL },
      { "keycode",  NULL, NULL, NULL, NULL, e_keycode,  napi_enumerable, NULL },
    };
    status = napi_define_properties(env, event_obj, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_define_properties");
    return event_obj;
  }
  else if (event->type == EVENT_MOUSE_MOVED || event->type == EVENT_MOUSE_PRESSED || event->type == EVENT_MOUSE_RELEASED || event->type == EVENT_MOUSE_CLICKED) {
    napi_value e_x;
    status = napi_create_int32(env, event->data.mouse.x, &e_x);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_int32");

    napi_value e_y;
    status = napi_create_int32(env, event->data.mouse.y, &e_y);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_int32");

    napi_value e_button;
    status = napi_create_uint32(env, event->data.mouse.button, &e_button);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_uint32");

    napi_value e_clicks;
    status = napi_create_uint32(env, event->data.mouse.clicks, &e_clicks);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_uint32");

    napi_property_descriptor descriptors[] = {
      { "type",     NULL, NULL, NULL, NULL, e_type,     napi_enumerable, NULL },
      { "time",     NULL, NULL, NULL, NULL, e_time,     napi_enumerable, NULL },
      { "altKey",   NULL, NULL, NULL, NULL, e_altKey,   napi_enumerable, NULL },
      { "ctrlKey",  NULL, NULL, NULL, NULL, e_ctrlKey,  napi_enumerable, NULL },
      { "metaKey",  NULL, NULL, NULL, NULL, e_metaKey,  napi_enumerable, NULL },
      { "shiftKey", NULL, NULL, NULL, NULL, e_shiftKey, napi_enumerable, NULL },
      { "x",        NULL, NULL, NULL, NULL, e_x,        napi_enumerable, NULL },
      { "y",        NULL, NULL, NULL, NULL, e_y,        napi_enumerable, NULL },
      { "button",   NULL, NULL, NULL, NULL, e_button,   napi_enumerable, NULL },
      { "clicks",   NULL, NULL, NULL, NULL, e_clicks,   napi_enumerable, NULL },
    };
    status = napi_define_properties(env, event_obj, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_define_properties");
    return event_obj;
  }
  else if (event->type == EVENT_MOUSE_WHEEL) {
    napi_value e_x;
    status = napi_create_int32(env, event->data.wheel.x, &e_x);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_int32");

    napi_value e_y;
    status = napi_create_int32(env, event->data.wheel.y, &e_y);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_int32");

    napi_value e_clicks;
    status = napi_create_uint32(env, event->data.wheel.clicks, &e_clicks);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_uint32");

    napi_value e_amount;
    status = napi_create_uint32(env, event->data.wheel.amount, &e_amount);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_uint32");

    napi_value e_direction;
    status = napi_create_uint32(env, event->data.wheel.direction, &e_direction);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_uint32");

    napi_value e_rotation;
    status = napi_create_int32(env, event->data.wheel.rotation, &e_rotation);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_create_int32");

    napi_property_descriptor descriptors[] = {
      { "type",      NULL, NULL, NULL, NULL, e_type,      napi_enumerable, NULL },
      { "time",      NULL, NULL, NULL, NULL, e_time,      napi_enumerable, NULL },
      { "altKey",    NULL, NULL, NULL, NULL, e_altKey,    napi_enumerable, NULL },
      { "ctrlKey",   NULL, NULL, NULL, NULL, e_ctrlKey,   napi_enumerable, NULL },
      { "metaKey",   NULL, NULL, NULL, NULL, e_metaKey,   napi_enumerable, NULL },
      { "shiftKey",  NULL, NULL, NULL, NULL, e_shiftKey,  napi_enumerable, NULL },
      { "x",         NULL, NULL, NULL, NULL, e_x,         napi_enumerable, NULL },
      { "y",         NULL, NULL, NULL, NULL, e_y,         napi_enumerable, NULL },
      { "clicks",    NULL, NULL, NULL, NULL, e_clicks,    napi_enumerable, NULL },
      { "amount",    NULL, NULL, NULL, NULL, e_amount,    napi_enumerable, NULL },
      { "direction", NULL, NULL, NULL, NULL, e_direction, napi_enumerable, NULL },
      { "rotation",  NULL, NULL, NULL, NULL, e_rotation,  napi_enumerable, NULL },
    };
    status = napi_define_properties(env, event_obj, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    NAPI_FATAL_IF_FAILED(status, "uiohook_to_js_event", "napi_define_properties");
    return event_obj;
  }

  return NULL; // never
}

void tsfn_to_js_proxy(napi_env env, napi_value js_callback, void* context, void* _event) {
  uiohook_event* event = (uiohook_event*)_event;

  if (env == NULL || js_callback == NULL || is_worker_running == false) {
    free(event);
    return;
  }

  napi_status status;

  napi_value event_obj = uiohook_to_js_event(env, event);

  napi_value global;
  status = napi_get_global(env, &global);
  NAPI_FATAL_IF_FAILED(status, "tsfn_to_js_proxy", "napi_get_global");

  status = napi_call_function(env, global, js_callback, 1, &event_obj, NULL);
  NAPI_FATAL_IF_FAILED(status, "tsfn_to_js_proxy", "napi_call_function");

  free(event);
}

napi_value AddonStart(napi_env env, napi_callback_info info) {
  if (is_worker_running == true)
    return NULL;

  napi_status status;

  size_t info_argc = 1;
  napi_value info_argv[1];
  status = napi_get_cb_info(env, info, &info_argc, info_argv, NULL, NULL);
  NAPI_THROW_IF_FAILED(env, status, NULL);

  napi_value cb = info_argv[0];

  napi_value async_resource_name;
  status = napi_create_string_utf8(env, "UIOHOOK_NAPI", NAPI_AUTO_LENGTH, &async_resource_name);
  NAPI_THROW_IF_FAILED(env, status, NULL);

  status = napi_create_threadsafe_function(env, cb, NULL, async_resource_name, 0, 1, NULL, NULL, NULL, tsfn_to_js_proxy, &threadsafe_fn);
  NAPI_THROW_IF_FAILED(env, status, NULL);

  int worker_status = uiohook_worker_start(dispatch_proc);

  if (worker_status != UIOHOOK_SUCCESS) {
    napi_release_threadsafe_function(threadsafe_fn, napi_tsfn_release);
    threadsafe_fn = NULL;
  }

  switch (worker_status) {
  case UIOHOOK_SUCCESS: {
    is_worker_running = true;
    return NULL;
  }
  case UIOHOOK_ERROR_THREAD_CREATE:
    NAPI_THROW(env, "UIOHOOK_ERROR_THREAD_CREATE", "Failed to create worker thread.", NULL);
  case UIOHOOK_ERROR_OUT_OF_MEMORY:
    NAPI_THROW(env, "UIOHOOK_ERROR_OUT_OF_MEMORY", "Failed to allocate memory.", NULL);
  case UIOHOOK_ERROR_X_OPEN_DISPLAY:
    NAPI_THROW(env, "UIOHOOK_ERROR_X_OPEN_DISPLAY", "Failed to open X11 display.", NULL);
  case UIOHOOK_ERROR_X_RECORD_NOT_FOUND:
    NAPI_THROW(env, "UIOHOOK_ERROR_X_RECORD_NOT_FOUND", "Unable to locate XRecord extension.", NULL);
  case UIOHOOK_ERROR_X_RECORD_ALLOC_RANGE:
    NAPI_THROW(env, "UIOHOOK_ERROR_X_RECORD_ALLOC_RANGE", "Unable to allocate XRecord range.", NULL);
  case UIOHOOK_ERROR_X_RECORD_CREATE_CONTEXT:
    NAPI_THROW(env, "UIOHOOK_ERROR_X_RECORD_CREATE_CONTEXT", "Unable to allocate XRecord context.", NULL);
  case UIOHOOK_ERROR_X_RECORD_ENABLE_CONTEXT:
    NAPI_THROW(env, "UIOHOOK_ERROR_X_RECORD_ENABLE_CONTEXT", "Failed to enable XRecord context.", NULL);
  case UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX:
    NAPI_THROW(env, "UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX", "Failed to register low level windows hook.", NULL);
  case UIOHOOK_ERROR_AXAPI_DISABLED:
    NAPI_THROW(env, "UIOHOOK_ERROR_AXAPI_DISABLED", "Failed to enable access for assistive devices.", NULL);
  case UIOHOOK_ERROR_CREATE_EVENT_PORT:
    NAPI_THROW(env, "UIOHOOK_ERROR_CREATE_EVENT_PORT", "Failed to create apple event port.", NULL);
  case UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE:
    NAPI_THROW(env, "UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE", "Failed to create apple run loop source.", NULL);
  case UIOHOOK_ERROR_GET_RUNLOOP:
    NAPI_THROW(env, "UIOHOOK_ERROR_GET_RUNLOOP", "Failed to acquire apple run loop.", NULL);
  case UIOHOOK_ERROR_CREATE_OBSERVER:
    NAPI_THROW(env, "UIOHOOK_ERROR_CREATE_OBSERVER", "Failed to create apple run loop observer.", NULL);
  case UIOHOOK_FAILURE:
  default:
    NAPI_THROW(env, "UIOHOOK_FAILURE", "An unknown hook error occurred.", NULL);
  }
}

napi_value AddonStop(napi_env env, napi_callback_info info) {
  if (is_worker_running == false)
    return NULL;

  int status = uiohook_worker_stop();

  switch (status) {
  case UIOHOOK_SUCCESS: {
    is_worker_running = false;
    napi_release_threadsafe_function(threadsafe_fn, napi_tsfn_release);
    threadsafe_fn = NULL;
    return NULL;
  }
  case UIOHOOK_ERROR_OUT_OF_MEMORY:
    NAPI_THROW(env, "UIOHOOK_ERROR_OUT_OF_MEMORY", "Failed to allocate memory.", NULL);
  case UIOHOOK_ERROR_X_RECORD_GET_CONTEXT:
    NAPI_THROW(env, "UIOHOOK_ERROR_X_RECORD_GET_CONTEXT", "Failed to get XRecord context.", NULL);
  case UIOHOOK_FAILURE:
  default:
    NAPI_THROW(env, "UIOHOOK_FAILURE", "An unknown hook error occurred.", NULL);
  }
}

void AddonCleanUp (void* arg) {
  if (is_worker_running) {
    uiohook_worker_stop();
  }
}

typedef enum {
  key_tap,
  key_down,
  key_up,
  force_uint = 0xFFFFFFFF
} key_tap_type;

napi_value AddonKeyTap (napi_env env, napi_callback_info info) {
  napi_status status;

  size_t info_argc = 2;
  napi_value info_argv[2];
  status = napi_get_cb_info(env, info, &info_argc, info_argv, NULL, NULL);
  NAPI_THROW_IF_FAILED(env, status, NULL);

  // [0] KeyCode
  uint32_t keycode;
  status = napi_get_value_uint32(env, info_argv[0], &keycode);
  NAPI_THROW_IF_FAILED(env, status, NULL);

  // [1] KeyTapType
  key_tap_type tap_type;
  status = napi_get_value_uint32(env, info_argv[1], (int32_t*)&tap_type);
  NAPI_THROW_IF_FAILED(env, status, NULL);

  uiohook_event event;
  memset(&event, 0, sizeof(event));
  event.data.keyboard.keycode = keycode;

  if (tap_type != key_up) {
    event.type = EVENT_KEY_PRESSED;
    hook_post_event(&event);
  }
  if (tap_type != key_down) {
    event.type = EVENT_KEY_RELEASED;
    hook_post_event(&event);
  }

  return NULL;
}

NAPI_MODULE_INIT() {
  napi_status status;
  napi_value export_fn;

  status = napi_create_function(env, NULL, 0, AddonStart, NULL, &export_fn);
  NAPI_FATAL_IF_FAILED(status, "NAPI_MODULE_INIT", "napi_create_function");
  status = napi_set_named_property(env, exports, "start", export_fn);
  NAPI_FATAL_IF_FAILED(status, "NAPI_MODULE_INIT", "napi_set_named_property");

  status = napi_create_function(env, NULL, 0, AddonStop, NULL, &export_fn);
  NAPI_FATAL_IF_FAILED(status, "NAPI_MODULE_INIT", "napi_create_function");
  status = napi_set_named_property(env, exports, "stop", export_fn);
  NAPI_FATAL_IF_FAILED(status, "NAPI_MODULE_INIT", "napi_set_named_property");

  status = napi_create_function(env, NULL, 0, AddonKeyTap, NULL, &export_fn);
  NAPI_FATAL_IF_FAILED(status, "NAPI_MODULE_INIT", "napi_create_function");
  status = napi_set_named_property(env, exports, "keyTap", export_fn);
  NAPI_FATAL_IF_FAILED(status, "NAPI_MODULE_INIT", "napi_set_named_property");

  status = napi_add_env_cleanup_hook(env, AddonCleanUp, NULL);
  NAPI_FATAL_IF_FAILED(status, "NAPI_MODULE_INIT", "napi_add_env_cleanup_hook");

  return exports;
}
