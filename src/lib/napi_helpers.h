#ifndef ADDON_SRC_HELPERS_H_
#define ADDON_SRC_HELPERS_H_

#include <node_api.h>
#include <stdio.h>

// Return NULL on pending exception, or fatally error on
// all other errors.
//
// This allows us to return control to JS code ASAP, as
// recommended by:
// https://nodejs.org/api/n-api.html#:~:text=when%20an%20exception%20is%20pending
#define NAPI_RETURN_NULL_OR_FATAL_IF_FAILED(env, status, \
  location, message)                                     \
  do {                                                   \
    if ((status) == napi_pending_exception) {            \
      return NULL;                                       \
    }                                                    \
    NAPI_FATAL_IF_FAILED(env, status, location,          \
      message);                                          \
  } while (0)

#define NAPI_FATAL_IF_FAILED(env, status, location,      \
  message)                                               \
  do {                                                   \
    if ((status) != napi_ok) {                           \
      const napi_extended_error_info* info;              \
      char full_message[100];                            \
      snprintf(full_message, 100, "%s", (message));      \
      if ((env) != NULL) {                               \
        napi_status get_error_status =                   \
          napi_get_last_error_info((env), &info);        \
        if (get_error_status == napi_ok &&               \
          info->error_message != NULL) {                 \
          snprintf(full_message, 100, "%s: %s",          \
            (message), info->error_message);             \
        }                                                \
      }                                                  \
      napi_fatal_error(location, NAPI_AUTO_LENGTH,       \
                       full_message, NAPI_AUTO_LENGTH);  \
    }                                                    \
  } while (0)

#define NAPI_THROW_IF_FAILED_VOID(env, status)           \
  if ((status) != napi_ok) {                             \
    NAPI_FATAL_IF_FAILED(                                \
      napi_throw(env, error_create(env)),                \
      env,                                               \
      "NAPI_THROW_IF_FAILED_VOID",                       \
      "napi_throw");                                     \
    return;                                              \
  }

#define NAPI_THROW_IF_FAILED(env, status, ...)           \
  if ((status) != napi_ok) {                             \
    NAPI_FATAL_IF_FAILED(                                \
      env,                                               \
      napi_throw(env, error_create(env)),                \
      "NAPI_THROW_IF_FAILED_VOID",                       \
      "napi_throw");                                     \
    return __VA_ARGS__;                                  \
  }

#define NAPI_THROW_VOID(env, code, msg)                  \
  do {                                                   \
    NAPI_FATAL_IF_FAILED(                                \
      env,                                               \
      napi_throw_error(env, (code), (msg)),              \
      "NAPI_THROW_VOID",                                 \
      "napi_throw_error");                               \
    return;                                              \
  } while (0)

#define NAPI_THROW(env, code, msg, ...)                  \
  do {                                                   \
    NAPI_FATAL_IF_FAILED(                                \
      env,                                               \
      napi_throw_error(env, (code), (msg)),              \
      "NAPI_THROW",                                      \
      "napi_throw_error");                               \
    return __VA_ARGS__;                                  \
  } while (0)


napi_value error_create(napi_env env);

#endif // !ADDON_SRC_HELPERS_H_
