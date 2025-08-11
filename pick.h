// pick.h — Cross-platform native dialog library (file pickers, message boxes)
// Single-header library — define PICK_IMPLEMENTATION in ONE source file before including.
//
// Example:
//   #define PICK_IMPLEMENTATION
//   #include "pick.h"
//
//   void file_picked(const char* path, void* user) {
//       if (path) printf("Selected: %s\n", path);
//       else      printf("Cancelled\n");
//   }
//
//   PickFileOptions opts = {0};
//   opts.title = "Select a file";
//   pick_file(&opts, file_picked, NULL);
//
// ─────────────────────────────────────────────────────────────────────────────
// API OVERVIEW
//   void pick_file   (const PickFileOptions*,  PickFileCallback,       void*);
//   void pick_files  (const PickFileOptions*,  PickMultiFileCallback,  void*);
//   void pick_folder (const PickFileOptions*,  PickFileCallback,       void*);
//   void pick_folders(const PickFileOptions*,  PickMultiFileCallback,  void*);
//   void pick_save   (const PickFileOptions*,  PickFileCallback,       void*);
//   void pick_message(const PickMessageOptions*, PickMessageCallback,  void*);
//   void pick_alert  (const char* title, const char* msg, void* parent);
//   void pick_confirm(const char* title, const char* msg, void* parent,
//                     PickMessageCallback, void*);
//
//   - All APIs are asynchronous (non-blocking) and invoke callbacks on the main thread.
//   - Strings passed to callbacks are valid only during the callback; copy if needed.
//
// ─────────────────────────────────────────────────────────────────────────────
// PLATFORM NOTES
//
// [macOS]
//   - Implemented via NSOpenPanel / NSSavePanel / NSAlert.
//   - Implementation source file can be .h/.c/.m/.mm but MUST be compiled
//     as Objective-C or Objective-C++ (e.g. `-x objective-c` when not .m/.mm).
//   - Link with: -framework AppKit -framework Foundation -lobjc
//
//   Example CMake (library form):
//     add_library(pick STATIC pick_impl.c)                 # TU can be .c
//     target_include_directories(pick PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
//     target_compile_options(pick PRIVATE -x objective-c)  # force ObjC mode
//     target_link_libraries(pick PUBLIC "-framework AppKit" "-framework Foundation" objc)
//
//   Parent window (sheets):
//     Pass an NSWindow* (or object responding to -window) in options->parent_handle
//     to attach the dialog as a sheet.
//
// [Windows]
//   - Not implemented. Including the implementation will #error.
//   - Planned: IFileDialog for pickers, Task Dialog for messages.
//
// [Linux]
//   - Not implemented. Including the implementation will #error.
//   - Planned: xdg-desktop-portal / GTK backend.
//
// [Web / Emscripten]
//   - Planned
//
// ─────────────────────────────────────────────────────────────────────────────
// LICENSE
//   MIT
#ifndef PICK_H
#define PICK_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define PICK_PLATFORM_MACOS
#elif defined(_WIN32) || defined(_WIN64)
#define PICK_PLATFORM_WINDOWS
#elif defined(__linux__)
#define PICK_PLATFORM_LINUX
#else
#error "Unsupported platform"
#endif

/// @brief File type filter for file dialogs
typedef struct PickFilter {
  const char *name;        ///< Display name (e.g., "Images")
  const char **extensions; ///< Array of extensions without dots (e.g., ["png", "jpg"])
  int extension_count;     ///< Number of extensions
} PickFilter;

/// @brief Button configuration for message boxes
typedef enum PickButtonType {
  PICK_BUTTON_OK,
  PICK_BUTTON_OK_CANCEL,
  PICK_BUTTON_YES_NO,
  PICK_BUTTON_YES_NO_CANCEL
} PickButtonType;

/// @brief Visual style for message boxes
typedef enum PickMessageStyle {
  PICK_STYLE_INFO,
  PICK_STYLE_WARNING,
  PICK_STYLE_ERROR,
  PICK_STYLE_QUESTION
} PickMessageStyle;

/// @brief Result from message box interaction
typedef enum PickButtonResult {
  PICK_RESULT_OK,
  PICK_RESULT_CANCEL,
  PICK_RESULT_YES,
  PICK_RESULT_NO,
  PICK_RESULT_CLOSED ///< Window closed without button click
} PickButtonResult;

/// @brief Icon types for message boxes
typedef enum PickIconType {
  PICK_ICON_DEFAULT,
  PICK_ICON_CUSTOM,
  PICK_ICON_APP,
  PICK_ICON_TRASH,
  PICK_ICON_FOLDER,
  PICK_ICON_DOCUMENT,
  PICK_ICON_LOCKED,
  PICK_ICON_UNLOCKED,
  PICK_ICON_NETWORK,
  PICK_ICON_USER,
  PICK_ICON_CAUTION,
  PICK_ICON_ERROR,
  PICK_ICON_STOP,
  PICK_ICON_INVALID
} PickIconType;

/// @brief Configuration for file picker dialogs
typedef struct PickFileOptions {
  const char *title;        ///< Dialog title/message
  const char *default_path; ///< Starting directory or default file path
  const char *default_name; ///< Default filename (save dialogs)
  PickFilter *filters;      ///< Array of file filters
  int filter_count;         ///< Number of filters
  bool can_create_dirs;     ///< Allow creating directories (save dialogs)
  bool allow_multiple;      ///< Allow selecting multiple items
  const void *parent_handle;      ///< Platform-specific parent window handle (optional)
} PickFileOptions;

/// @brief Configuration for message boxes and sheets
typedef struct PickMessageOptions {
  const char *title;       ///< Title bar text (windows) or bold heading (sheets)
  const char *message;     ///< Main message text
  const char *detail;      ///< Additional detail text (optional)
  PickButtonType buttons;  ///< Which buttons to show
  PickMessageStyle style;  ///< Icon/style to use
  PickIconType icon_type;  ///< Icon type to use
  const char *icon_path;   ///< Path to custom icon (if icon_type == PICK_ICON_CUSTOM)
  const void *parent_handle;     ///< If set, shows as sheet/modal dialog on parent
} PickMessageOptions;

/// @brief Callback for single file selection
/// @param path Selected file path or NULL if cancelled
/// @param user_data User-provided context
typedef void (*PickFileCallback)(const char *path, void *user_data);

/// @brief Callback for multiple file selection
/// @param paths Array of selected file paths or NULL if cancelled
/// @param count Number of selected files
/// @param user_data User-provided context
typedef void (*PickMultiFileCallback)(const char **paths, int count,
                                      void *user_data);

/// @brief Callback for message box response
/// @param result Button that was clicked
/// @param user_data User-provided context
typedef void (*PickMessageCallback)(PickButtonResult result, void *user_data);

/// @brief Shows file open dialog for single file selection (async)
/// @param options Dialog configuration (can be NULL for defaults)
/// @param callback Function called with selected file
/// @param user_data Context passed to callback
void pick_file(const PickFileOptions *options, PickFileCallback callback,
               void *user_data);

/// @brief Shows file open dialog for multiple file selection (async)
/// @param options Dialog configuration (can be NULL for defaults)
/// @param callback Function called with selected files
/// @param user_data Context passed to callback
void pick_files(const PickFileOptions *options, PickMultiFileCallback callback,
                void *user_data);

/// @brief Shows folder selection dialog (async)
/// @param options Dialog configuration (can be NULL for defaults)
/// @param callback Function called with selected folder
/// @param user_data Context passed to callback
void pick_folder(const PickFileOptions *options, PickFileCallback callback,
                 void *user_data);

/// @brief Shows folder selection dialog for multiple folders (async)
/// @param options Dialog configuration (can be NULL for defaults)
/// @param callback Function called with selected folders
/// @param user_data Context passed to callback
void pick_folders(const PickFileOptions *options,
                  PickMultiFileCallback callback, void *user_data);

/// @brief Shows file save dialog (async)
/// @param options Dialog configuration (can be NULL for defaults)
/// @param callback Function called with save path
/// @param user_data Context passed to callback
void pick_save(const PickFileOptions *options, PickFileCallback callback,
               void *user_data);

/// @brief Shows message box or sheet (async)
/// @param options Message box configuration
/// @param callback Function called with user response (can be NULL)
/// @param user_data Context passed to callback
void pick_message(const PickMessageOptions *options,
                  PickMessageCallback callback, void *user_data);

/// @brief Shows simple alert dialog with OK button
/// @param title Alert title
/// @param message Alert message
/// @param parent_handle Parent window (NULL for standalone)
void pick_alert(const char *title, const char *message, void *parent_handle);

/// @brief Shows confirmation dialog with OK/Cancel buttons
/// @param title Dialog title
/// @param message Dialog message
/// @param parent_handle Parent window (NULL for standalone)
/// @param callback Function called with user response
/// @param user_data Context passed to callback
void pick_confirm(const char *title, const char *message, void *parent_handle,
                  PickMessageCallback callback, void *user_data);

/// @brief Frees memory for a single path returned by the library
/// @param path Path to free
void pick_free(char *path);

/// @brief Frees memory for multiple paths returned by the library
/// @param paths Array of paths to free
/// @param count Number of paths
void pick_free_multiple(char **paths, int count);

#ifdef __cplusplus
}
#endif

#ifdef PICK_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

#ifdef PICK_PLATFORM_MACOS

#include <CoreFoundation/CoreFoundation.h>
#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/objc.h>
#include <objc/runtime.h>

#ifndef nil
#define nil ((id)0)
#endif
#ifndef YES
#define YES ((BOOL)1)
#endif
#ifndef NO
#define NO ((BOOL)0)
#endif

typedef unsigned long NSUInteger;
typedef long NSInteger;
typedef double CGFloat;

enum {
  NSAlertStyleWarning = 0,
  NSAlertStyleInformational = 1,
  NSAlertStyleCritical = 2
};

enum {
  NSAlertFirstButtonReturn = 1000,
  NSAlertSecondButtonReturn = 1001,
  NSAlertThirdButtonReturn = 1002
};

enum { NSModalResponseCancel = 0, NSModalResponseOK = 1 };

enum { NSApplicationActivationPolicyRegular = 0 };

static id pick_objc_string(const char *str) {
  if (!str)
    return nil;
  return ((id (*)(id, SEL, const char *))objc_msgSend)(
      (id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"),
      str);
}

static id pick_objc_url_from_path(const char *path, bool is_directory) {
  if (!path)
    return nil;
  id str = pick_objc_string(path);
  if (!str)
    return nil;
  return ((id (*)(id, SEL, id, BOOL))objc_msgSend)(
      (id)objc_getClass("NSURL"),
      sel_registerName("fileURLWithPath:isDirectory:"), str,
      is_directory ? YES : NO);
}

static char *pick_objc_path_from_url(id url) {
  if (!url)
    return NULL;
  id path = ((id (*)(id, SEL))objc_msgSend)(url, sel_registerName("path"));
  if (!path)
    return NULL;
  const char *utf8 = ((const char *(*)(id, SEL))objc_msgSend)(
      path, sel_registerName("UTF8String"));
  if (!utf8)
    return NULL;

  size_t len = strlen(utf8);
  char *result = (char *)malloc(len + 1);
  if (result) {
    memcpy(result, utf8, len + 1);
  }
  return result;
}

static id pick_objc_app_instance(void) {
  return ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSApplication"),
                                         sel_registerName("sharedApplication"));
}

static void pick_objc_ensure_app_initialized(void) {
  static bool initialized = false;
  if (initialized)
    return;
  initialized = true;

  id app = pick_objc_app_instance();

  ((void (*)(id, SEL, NSInteger))objc_msgSend)(
      app, sel_registerName("setActivationPolicy:"),
      NSApplicationActivationPolicyRegular);

  ((void (*)(id, SEL))objc_msgSend)(app, sel_registerName("finishLaunching"));

  ((void (*)(id, SEL, BOOL))objc_msgSend)(
      app, sel_registerName("activateIgnoringOtherApps:"), YES);
}

static void pick_objc_run_on_main(void (^block)(void)) {
  BOOL is_main = ((BOOL (*)(id, SEL))objc_msgSend)(
      (id)objc_getClass("NSThread"), 
      sel_registerName("isMainThread"));
  
  if (is_main) {
    block();
  } else {
    dispatch_async(dispatch_get_main_queue(), block);
  }
}

static id pick_objc_window_from_handle(const void *handle) {
  if (!handle)
    return nil;

  if ([(id)handle isKindOfClass:[objc_getClass("NSWindow") class]]) {
    return (id)handle;
  }

  if ([(id)handle respondsToSelector:sel_registerName("window")]) {
    return ((id (*)(id, SEL))objc_msgSend)((id)handle,
                                           sel_registerName("window"));
  }

  return nil;
}

static id pick_objc_create_file_extensions_array(const PickFilter *filters,
                                                 int filter_count) {
  if (!filters || filter_count <= 0)
    return nil;

  id array = ((id (*)(id, SEL))objc_msgSend)(
      (id)objc_getClass("NSMutableArray"), sel_registerName("array"));

  for (int i = 0; i < filter_count; i++) {
    for (int j = 0; j < filters[i].extension_count; j++) {
      id ext = pick_objc_string(filters[i].extensions[j]);
      if (ext) {
        ((void (*)(id, SEL, id))objc_msgSend)(
            array, sel_registerName("addObject:"), ext);
      }
    }
  }

  return array;
}

static id pick_objc_create_open_panel(const PickFileOptions *options,
                                      bool allow_dirs, bool allow_files) {
  id panel = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSOpenPanel"),
                                             sel_registerName("openPanel"));

  ((void (*)(id, SEL, BOOL))objc_msgSend)(
      panel, sel_registerName("setCanChooseDirectories:"),
      allow_dirs ? YES : NO);
  ((void (*)(id, SEL, BOOL))objc_msgSend)(
      panel, sel_registerName("setCanChooseFiles:"), allow_files ? YES : NO);
  ((void (*)(id, SEL, BOOL))objc_msgSend)(
      panel, sel_registerName("setAllowsMultipleSelection:"),
      (options && options->allow_multiple) ? YES : NO);

  if (options) {
    if (options->title) {
      ((void (*)(id, SEL, id))objc_msgSend)(panel,
                                            sel_registerName("setMessage:"),
                                            pick_objc_string(options->title));
    }

    if (options->default_path) {
      id url = pick_objc_url_from_path(options->default_path, YES);
      if (url) {
        ((void (*)(id, SEL, id))objc_msgSend)(
            panel, sel_registerName("setDirectoryURL:"), url);
      }
    }

    if (allow_files && options->filters && options->filter_count > 0) {
      id extensions = pick_objc_create_file_extensions_array(
          options->filters, options->filter_count);
      if (extensions) {
        ((void (*)(id, SEL, id))objc_msgSend)(
            panel, sel_registerName("setAllowedFileTypes:"), extensions);
      }
    }
  }

  return panel;
}

static id pick_objc_create_save_panel(const PickFileOptions *options) {
  id panel = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSSavePanel"),
                                             sel_registerName("savePanel"));

  if (options) {
    if (options->title) {
      ((void (*)(id, SEL, id))objc_msgSend)(panel,
                                            sel_registerName("setMessage:"),
                                            pick_objc_string(options->title));
    }

    if (options->default_path) {
      id url = pick_objc_url_from_path(options->default_path, YES);
      if (url) {
        ((void (*)(id, SEL, id))objc_msgSend)(
            panel, sel_registerName("setDirectoryURL:"), url);
      }
    }

    if (options->default_name) {
      ((void (*)(id, SEL, id))objc_msgSend)(
          panel, sel_registerName("setNameFieldStringValue:"),
          pick_objc_string(options->default_name));
    }

    ((void (*)(id, SEL, BOOL))objc_msgSend)(
        panel, sel_registerName("setCanCreateDirectories:"),
        options->can_create_dirs ? YES : NO);

    if (options->filters && options->filter_count > 0) {
      id extensions = pick_objc_create_file_extensions_array(
          options->filters, options->filter_count);
      if (extensions) {
        ((void (*)(id, SEL, id))objc_msgSend)(
            panel, sel_registerName("setAllowedFileTypes:"), extensions);
      }
    }
  }

  return panel;
}

typedef struct {
  PickMessageCallback callback;
  void *user_data;
} pick_message_context;

static NSInteger pick_objc_alert_style(PickMessageStyle style) {
  switch (style) {
  case PICK_STYLE_ERROR:
    return NSAlertStyleCritical;
  case PICK_STYLE_WARNING:
    return NSAlertStyleWarning;
  default:
    return NSAlertStyleInformational;
  }
}

static PickButtonResult pick_objc_button_result(NSInteger response,
                                                PickButtonType buttons) {
  if (response == NSAlertFirstButtonReturn) {
    switch (buttons) {
    case PICK_BUTTON_OK:
    case PICK_BUTTON_OK_CANCEL:
      return PICK_RESULT_OK;
    case PICK_BUTTON_YES_NO:
    case PICK_BUTTON_YES_NO_CANCEL:
      return PICK_RESULT_YES;
    }
  } else if (response == NSAlertSecondButtonReturn) {
    switch (buttons) {
    case PICK_BUTTON_OK:
      return PICK_RESULT_OK;
    case PICK_BUTTON_OK_CANCEL:
      return PICK_RESULT_CANCEL;
    case PICK_BUTTON_YES_NO:
      return PICK_RESULT_NO;
    case PICK_BUTTON_YES_NO_CANCEL:
      return PICK_RESULT_NO;
    }
  } else if (response == NSAlertThirdButtonReturn) {
    return PICK_RESULT_CANCEL;
  }
  return PICK_RESULT_CLOSED;
}

static void pick_objc_set_alert_icon(id alert, PickIconType icon_type,
                                     const char *icon_path) {
  id icon = nil;

  switch (icon_type) {
  case PICK_ICON_DEFAULT:
    return;

  case PICK_ICON_CUSTOM:
    if (icon_path) {
      id path = pick_objc_string(icon_path);
      icon = ((id (*)(id, SEL, id))objc_msgSend)(
          ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSImage"),
                                          sel_registerName("alloc")),
          sel_registerName("initWithContentsOfFile:"), path);
    }
    break;

  case PICK_ICON_APP:
    icon = ((id (*)(id, SEL, id))objc_msgSend)(
        (id)objc_getClass("NSImage"), sel_registerName("imageNamed:"),
        pick_objc_string("NSApplicationIcon"));
    break;

  case PICK_ICON_TRASH:
    {
      id workspace =
          ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSWorkspace"),
                                          sel_registerName("sharedWorkspace"));
      icon = ((id (*)(id, SEL, id))objc_msgSend)(
          workspace, sel_registerName("iconForFile:"),
          pick_objc_string("~/.Trash"));
    }
    break;

  case PICK_ICON_FOLDER:
    icon = ((id (*)(id, SEL, id))objc_msgSend)(
        (id)objc_getClass("NSImage"), sel_registerName("imageNamed:"),
        pick_objc_string("NSFolder"));
    break;

  case PICK_ICON_DOCUMENT:
    icon = ((id (*)(id, SEL, id))objc_msgSend)(
        (id)objc_getClass("NSImage"), sel_registerName("imageNamed:"),
        pick_objc_string("NSMultipleDocuments"));
    break;

  case PICK_ICON_LOCKED:
    icon = ((id (*)(id, SEL, id))objc_msgSend)(
        (id)objc_getClass("NSImage"), sel_registerName("imageNamed:"),
        pick_objc_string("NSLockLockedTemplate"));
    break;

  case PICK_ICON_UNLOCKED:
    icon = ((id (*)(id, SEL, id))objc_msgSend)(
        (id)objc_getClass("NSImage"), sel_registerName("imageNamed:"),
        pick_objc_string("NSLockUnlockedTemplate"));
    break;

  case PICK_ICON_NETWORK:
    icon = ((id (*)(id, SEL, id))objc_msgSend)(
        (id)objc_getClass("NSImage"), sel_registerName("imageNamed:"),
        pick_objc_string("NSNetwork"));
    break;

  case PICK_ICON_USER:
    icon = ((id (*)(id, SEL, id))objc_msgSend)(
        (id)objc_getClass("NSImage"), sel_registerName("imageNamed:"),
        pick_objc_string("NSUser"));
    break;

  case PICK_ICON_CAUTION:
    icon = ((id (*)(id, SEL, id))objc_msgSend)(
        (id)objc_getClass("NSImage"), sel_registerName("imageNamed:"),
        pick_objc_string("NSCaution"));
    break;
    
  case PICK_ICON_ERROR:
    icon = ((id (*)(id, SEL, id))objc_msgSend)(
        (id)objc_getClass("NSImage"), sel_registerName("imageNamed:"),
        pick_objc_string("NSCriticalAlertIcon"));
    break;

  case PICK_ICON_STOP:
    icon = ((id (*)(id, SEL, id))objc_msgSend)(
        (id)objc_getClass("NSImage"), sel_registerName("imageNamed:"),
        pick_objc_string("NSStopProgressTemplate"));
    break;

  case PICK_ICON_INVALID:
    icon = ((id (*)(id, SEL, id))objc_msgSend)(
        (id)objc_getClass("NSImage"), sel_registerName("imageNamed:"),
        pick_objc_string("NSInvalidDataFreestandingTemplate"));
    break;
  }

  if (icon) {
    ((void (*)(id, SEL, id))objc_msgSend)(alert, sel_registerName("setIcon:"),
                                          icon);
  }
}

static id pick_objc_create_alert(const PickMessageOptions *options) {
  id alert = ((id (*)(id, SEL))objc_msgSend)(
      ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSAlert"),
                                      sel_registerName("alloc")),
      sel_registerName("init"));

  if (options->title) {
    ((void (*)(id, SEL, id))objc_msgSend)(alert,
                                          sel_registerName("setMessageText:"),
                                          pick_objc_string(options->title));
  }

  if (options->message) {
    ((void (*)(id, SEL, id))objc_msgSend)(
        alert, sel_registerName("setInformativeText:"),
        pick_objc_string(options->message));
  }

  ((void (*)(id, SEL, NSInteger))objc_msgSend)(
      alert, sel_registerName("setAlertStyle:"),
      pick_objc_alert_style(options->style));

  if (options->icon_type != PICK_ICON_DEFAULT) {
    pick_objc_set_alert_icon(alert, options->icon_type, options->icon_path);
  }

  switch (options->buttons) {
  case PICK_BUTTON_OK:
    ((id (*)(id, SEL, id))objc_msgSend)(
        alert, sel_registerName("addButtonWithTitle:"), pick_objc_string("OK"));
    break;
  case PICK_BUTTON_OK_CANCEL:
    ((id (*)(id, SEL, id))objc_msgSend)(
        alert, sel_registerName("addButtonWithTitle:"), pick_objc_string("OK"));
    ((id (*)(id, SEL, id))objc_msgSend)(alert,
                                        sel_registerName("addButtonWithTitle:"),
                                        pick_objc_string("Cancel"));
    break;
  case PICK_BUTTON_YES_NO:
    ((id (*)(id, SEL, id))objc_msgSend)(alert,
                                        sel_registerName("addButtonWithTitle:"),
                                        pick_objc_string("Yes"));
    ((id (*)(id, SEL, id))objc_msgSend)(
        alert, sel_registerName("addButtonWithTitle:"), pick_objc_string("No"));
    break;
  case PICK_BUTTON_YES_NO_CANCEL:
    ((id (*)(id, SEL, id))objc_msgSend)(alert,
                                        sel_registerName("addButtonWithTitle:"),
                                        pick_objc_string("Yes"));
    ((id (*)(id, SEL, id))objc_msgSend)(
        alert, sel_registerName("addButtonWithTitle:"), pick_objc_string("No"));
    ((id (*)(id, SEL, id))objc_msgSend)(alert,
                                        sel_registerName("addButtonWithTitle:"),
                                        pick_objc_string("Cancel"));
    break;
  }

  return alert;
}

typedef struct {
  PickFileCallback single_callback;
  PickMultiFileCallback multi_callback;
  void *user_data;
} pick_file_context;

void pick_file(const PickFileOptions *options, PickFileCallback callback,
               void *user_data) {
  pick_objc_run_on_main(^{
    pick_objc_ensure_app_initialized();

    id panel = pick_objc_create_open_panel(options, false, true);
    id parent_window =
        pick_objc_window_from_handle(options ? options->parent_handle : NULL);

    pick_file_context *ctx =
        (pick_file_context *)malloc(sizeof(pick_file_context));
    ctx->single_callback = callback;
    ctx->multi_callback = NULL;
    ctx->user_data = user_data;

    void (^completion_handler)(NSInteger) = ^(NSInteger response) {
      char *path = NULL;
      if (response == NSModalResponseOK) {
        id url =
            ((id (*)(id, SEL))objc_msgSend)(panel, sel_registerName("URL"));
        path = pick_objc_path_from_url(url);
      }
      if (ctx->single_callback) {
        ctx->single_callback(path, ctx->user_data);
      }
      free(path);
      free(ctx);
    };

    if (parent_window) {
      ((void (*)(id, SEL, id, id))objc_msgSend)(
          panel,
          sel_registerName("beginSheetModalForWindow:completionHandler:"),
          parent_window, (id)completion_handler);
    } else {
      ((void (*)(id, SEL, id))objc_msgSend)(
          panel, sel_registerName("beginWithCompletionHandler:"),
          (id)completion_handler);
    }
  });
}

void pick_files(const PickFileOptions *options, PickMultiFileCallback callback,
                void *user_data) {
  pick_objc_run_on_main(^{
    pick_objc_ensure_app_initialized();

    PickFileOptions opts = options ? *options : (PickFileOptions){0};
    opts.allow_multiple = true;

    id panel = pick_objc_create_open_panel(&opts, false, true);
    id parent_window =
        pick_objc_window_from_handle(options ? options->parent_handle : NULL);

    pick_file_context *ctx =
        (pick_file_context *)malloc(sizeof(pick_file_context));
    ctx->single_callback = NULL;
    ctx->multi_callback = callback;
    ctx->user_data = user_data;

    void (^completion_handler)(NSInteger) = ^(NSInteger response) {
      char **paths = NULL;
      int count = 0;

      if (response == NSModalResponseOK) {
        id urls =
            ((id (*)(id, SEL))objc_msgSend)(panel, sel_registerName("URLs"));
        NSUInteger url_count = ((NSUInteger (*)(id, SEL))objc_msgSend)(
            urls, sel_registerName("count"));

        if (url_count > 0) {
          paths = (char **)calloc(url_count, sizeof(char *));
          if (paths) {
            for (NSUInteger i = 0; i < url_count; i++) {
              id url = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(
                  urls, sel_registerName("objectAtIndex:"), i);
              char *path = pick_objc_path_from_url(url);
              if (path) {
                paths[count++] = path;
              }
            }
          }
        }
      }

      if (ctx->multi_callback) {
        ctx->multi_callback((const char **)paths, count, ctx->user_data);
      }

      pick_free_multiple(paths, count);
      free(ctx);
    };

    if (parent_window) {
      ((void (*)(id, SEL, id, id))objc_msgSend)(
          panel,
          sel_registerName("beginSheetModalForWindow:completionHandler:"),
          parent_window, (id)completion_handler);
    } else {
      ((void (*)(id, SEL, id))objc_msgSend)(
          panel, sel_registerName("beginWithCompletionHandler:"),
          (id)completion_handler);
    }
  });
}

void pick_folder(const PickFileOptions *options, PickFileCallback callback,
                 void *user_data) {
  pick_objc_run_on_main(^{
    pick_objc_ensure_app_initialized();

    id panel = pick_objc_create_open_panel(options, true, false);
    id parent_window =
        pick_objc_window_from_handle(options ? options->parent_handle : NULL);

    pick_file_context *ctx =
        (pick_file_context *)malloc(sizeof(pick_file_context));
    ctx->single_callback = callback;
    ctx->multi_callback = NULL;
    ctx->user_data = user_data;

    void (^completion_handler)(NSInteger) = ^(NSInteger response) {
      char *path = NULL;
      if (response == NSModalResponseOK) {
        id url =
            ((id (*)(id, SEL))objc_msgSend)(panel, sel_registerName("URL"));
        path = pick_objc_path_from_url(url);
      }
      if (ctx->single_callback) {
        ctx->single_callback(path, ctx->user_data);
      }
      free(path);
      free(ctx);
    };

    if (parent_window) {
      ((void (*)(id, SEL, id, id))objc_msgSend)(
          panel,
          sel_registerName("beginSheetModalForWindow:completionHandler:"),
          parent_window, (id)completion_handler);
    } else {
      ((void (*)(id, SEL, id))objc_msgSend)(
          panel, sel_registerName("beginWithCompletionHandler:"),
          (id)completion_handler);
    }
  });
}

void pick_folders(const PickFileOptions *options,
                  PickMultiFileCallback callback, void *user_data) {
  pick_objc_run_on_main(^{
    pick_objc_ensure_app_initialized();

    PickFileOptions opts = options ? *options : (PickFileOptions){0};
    opts.allow_multiple = true;

    id panel = pick_objc_create_open_panel(&opts, true, false);
    id parent_window =
        pick_objc_window_from_handle(options ? options->parent_handle : NULL);

    pick_file_context *ctx =
        (pick_file_context *)malloc(sizeof(pick_file_context));
    ctx->single_callback = NULL;
    ctx->multi_callback = callback;
    ctx->user_data = user_data;

    void (^completion_handler)(NSInteger) = ^(NSInteger response) {
      char **paths = NULL;
      int count = 0;

      if (response == NSModalResponseOK) {
        id urls =
            ((id (*)(id, SEL))objc_msgSend)(panel, sel_registerName("URLs"));
        NSUInteger url_count = ((NSUInteger (*)(id, SEL))objc_msgSend)(
            urls, sel_registerName("count"));

        if (url_count > 0) {
          paths = (char **)calloc(url_count, sizeof(char *));
          if (paths) {
            for (NSUInteger i = 0; i < url_count; i++) {
              id url = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(
                  urls, sel_registerName("objectAtIndex:"), i);
              char *path = pick_objc_path_from_url(url);
              if (path) {
                paths[count++] = path;
              }
            }
          }
        }
      }

      if (ctx->multi_callback) {
        ctx->multi_callback((const char **)paths, count, ctx->user_data);
      }

      pick_free_multiple(paths, count);
      free(ctx);
    };

    if (parent_window) {
      ((void (*)(id, SEL, id, id))objc_msgSend)(
          panel,
          sel_registerName("beginSheetModalForWindow:completionHandler:"),
          parent_window, (id)completion_handler);
    } else {
      ((void (*)(id, SEL, id))objc_msgSend)(
          panel, sel_registerName("beginWithCompletionHandler:"),
          (id)completion_handler);
    }
  });
}

void pick_save(const PickFileOptions *options, PickFileCallback callback,
               void *user_data) {
  pick_objc_run_on_main(^{
    pick_objc_ensure_app_initialized();

    id panel = pick_objc_create_save_panel(options);
    id parent_window =
        pick_objc_window_from_handle(options ? options->parent_handle : NULL);

    pick_file_context *ctx =
        (pick_file_context *)malloc(sizeof(pick_file_context));
    ctx->single_callback = callback;
    ctx->multi_callback = NULL;
    ctx->user_data = user_data;

    void (^completion_handler)(NSInteger) = ^(NSInteger response) {
      char *path = NULL;
      if (response == NSModalResponseOK) {
        id url =
            ((id (*)(id, SEL))objc_msgSend)(panel, sel_registerName("URL"));
        path = pick_objc_path_from_url(url);
      }
      if (ctx->single_callback) {
        ctx->single_callback(path, ctx->user_data);
      }
      free(path);
      free(ctx);
    };

    if (parent_window) {
      ((void (*)(id, SEL, id, id))objc_msgSend)(
          panel,
          sel_registerName("beginSheetModalForWindow:completionHandler:"),
          parent_window, (id)completion_handler);
    } else {
      ((void (*)(id, SEL, id))objc_msgSend)(
          panel, sel_registerName("beginWithCompletionHandler:"),
          (id)completion_handler);
    }
  });
}

void pick_message(const PickMessageOptions *options,
                  PickMessageCallback callback, void *user_data) {
  pick_objc_run_on_main(^{
    pick_objc_ensure_app_initialized();

    id alert = pick_objc_create_alert(options);
    id parent_window =
        pick_objc_window_from_handle(options ? options->parent_handle : NULL);

    pick_message_context *ctx =
        (pick_message_context *)malloc(sizeof(pick_message_context));
    ctx->callback = callback;
    ctx->user_data = user_data;

    void (^completion_handler)(NSInteger) = ^(NSInteger response) {
      PickButtonResult result =
          pick_objc_button_result(response, options->buttons);
      if (ctx->callback) {
        ctx->callback(result, ctx->user_data);
      }
      free(ctx);
    };

    if (parent_window) {
      ((void (*)(id, SEL, id, id))objc_msgSend)(
          alert,
          sel_registerName("beginSheetModalForWindow:completionHandler:"),
          parent_window, (id)completion_handler);
    } else {
      dispatch_async(dispatch_get_main_queue(), ^{
        NSInteger response = ((NSInteger (*)(id, SEL))objc_msgSend)(
            alert, sel_registerName("runModal"));
        completion_handler(response);
      });
    }
  });
}

void pick_alert(const char *title, const char *message, void *parent_handle) {
  PickMessageOptions opts = {0};
  opts.title = title;
  opts.message = message;
  opts.buttons = PICK_BUTTON_OK;
  opts.style = PICK_STYLE_INFO;
  opts.parent_handle = parent_handle;

  pick_message(&opts, NULL, NULL);
}

void pick_confirm(const char *title, const char *message, void *parent_handle,
                  PickMessageCallback callback, void *user_data) {
  PickMessageOptions opts = {0};
  opts.title = title;
  opts.message = message;
  opts.buttons = PICK_BUTTON_OK_CANCEL;
  opts.style = PICK_STYLE_QUESTION;
  opts.parent_handle = parent_handle;

  pick_message(&opts, callback, user_data);
}

#endif

#ifdef PICK_PLATFORM_WINDOWS
#error "Windows implementation not yet available"
#endif

#ifdef PICK_PLATFORM_LINUX
#error "Linux implementation not yet available"
#endif

void pick_free(char *path) { free(path); }

void pick_free_multiple(char **paths, int count) {
  if (paths) {
    for (int i = 0; i < count; i++) {
      free(paths[i]);
    }
    free(paths);
  }
}

#endif

#endif
