#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define PICK_IMPLEMENTATION
#include "../pick.h"
#include "raylib.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif


typedef struct {
    char status[512];
    char last_result[1024];
    Color status_color;
    float status_timer;
    bool waiting_for_dialog;
} UIState;

static UIState ui = {
    .status = "Ready - Click any button to test a dialog",
    .last_result = "Results will appear here...",
    .status_color = DARKGRAY,
    .status_timer = 0,
    .waiting_for_dialog = false
};


typedef struct {
    Rectangle bounds;
    const char* text;
    const char* description;
    Color color;
    void (*action)(void);
} Button;


static void test_file_picker(void);
static void test_multi_file_picker(void);
static void test_folder_picker(void);
static void test_save_dialog(void);
static void test_alert(void);
static void test_confirm(void);
static void test_yes_no(void);
static void test_warning(void);
static void test_error(void);
static void test_custom_message(void);


static void set_status(const char* msg, Color color) {
    strncpy(ui.status, msg, sizeof(ui.status) - 1);
    ui.status[sizeof(ui.status) - 1] = '\0';
    ui.status_color = color;
    ui.status_timer = 5.0f;
}

static void set_result(const char* result) {
    strncpy(ui.last_result, result, sizeof(ui.last_result) - 1);
    ui.last_result[sizeof(ui.last_result) - 1] = '\0';
}


static void on_file_selected(const char* path, void* user) {
    (void)user;
    ui.waiting_for_dialog = false;
    
    if (path) {
        char result[1024];
        snprintf(result, sizeof(result), "✓ File selected:\n%s", path);
        set_result(result);
        set_status("File selected successfully!", DARKGREEN);
    } else {
        set_result("✗ File selection cancelled");
        set_status("File selection cancelled", ORANGE);
    }
}

static void on_files_selected(const char** paths, int count, void* user) {
    (void)user;
    ui.waiting_for_dialog = false;
    
    if (paths && count > 0) {
        char result[1024];
        int offset = snprintf(result, sizeof(result), "✓ Selected %d files:\n", count);
        
        for (int i = 0; i < count && offset < (int)sizeof(result) - 50; i++) {
            const char* filename = strrchr(paths[i], '/');
            if (!filename) filename = strrchr(paths[i], '\\');
            if (!filename) filename = paths[i];
            else filename++;
            
            offset += snprintf(result + offset, sizeof(result) - offset, 
                             "%d. %s\n", i + 1, filename);
        }
        
        set_result(result);
        char msg[256];
        snprintf(msg, sizeof(msg), "Selected %d files!", count);
        set_status(msg, DARKGREEN);
    } else {
        set_result("✗ Multi-file selection cancelled");
        set_status("Multi-file selection cancelled", ORANGE);
    }
}

static void on_folder_selected(const char* path, void* user) {
    (void)user;
    ui.waiting_for_dialog = false;
    
    if (path) {
        char result[1024];
        snprintf(result, sizeof(result), "✓ Folder selected:\n%s", path);
        set_result(result);
        set_status("Folder selected successfully!", DARKGREEN);
    } else {
        set_result("✗ Folder selection cancelled");
        set_status("Folder selection cancelled", ORANGE);
    }
}

static void on_save_selected(const char* path, void* user) {
    (void)user;
    ui.waiting_for_dialog = false;
    
    if (path) {
        char result[1024];
        snprintf(result, sizeof(result), "✓ Save path selected:\n%s", path);
        set_result(result);
        set_status("Save path selected!", DARKGREEN);
        
#ifdef __EMSCRIPTEN__
        
        FILE* f = fopen(path, "w");
        if (f) {
            fprintf(f, "Hello from pick.h raylib demo!\n");
            fprintf(f, "This file was created in the browser's memory filesystem.\n");
            fclose(f);
        }
#endif
    } else {
        set_result("✗ Save cancelled");
        set_status("Save cancelled", ORANGE);
    }
}

static void on_message_response(PickButtonResult result, void* user) {
    const char* dialog_type = (const char*)user;
    ui.waiting_for_dialog = false;
    
    char msg[256];
    char result_text[256];
    
    switch(result) {
        case PICK_RESULT_OK:
            snprintf(result_text, sizeof(result_text), "✓ %s: OK clicked", dialog_type);
            snprintf(msg, sizeof(msg), "%s - OK", dialog_type);
            set_status(msg, DARKGREEN);
            break;
        case PICK_RESULT_YES:
            snprintf(result_text, sizeof(result_text), "✓ %s: YES clicked", dialog_type);
            snprintf(msg, sizeof(msg), "%s - YES", dialog_type);
            set_status(msg, DARKGREEN);
            break;
        case PICK_RESULT_NO:
            snprintf(result_text, sizeof(result_text), "✗ %s: NO clicked", dialog_type);
            snprintf(msg, sizeof(msg), "%s - NO", dialog_type);
            set_status(msg, ORANGE);
            break;
        case PICK_RESULT_CANCEL:
            snprintf(result_text, sizeof(result_text), "✗ %s: CANCEL clicked", dialog_type);
            snprintf(msg, sizeof(msg), "%s - CANCEL", dialog_type);
            set_status(msg, GRAY);
            break;
        default:
            snprintf(result_text, sizeof(result_text), "? %s: Dialog closed", dialog_type);
            snprintf(msg, sizeof(msg), "%s - Closed", dialog_type);
            set_status(msg, GRAY);
            break;
    }
    
    set_result(result_text);
}


static void test_file_picker(void) {
    if (ui.waiting_for_dialog) return;
    
    const char* img_ext[] = {"png", "jpg", "jpeg", "gif", "bmp"};
    const char* doc_ext[] = {"txt", "md", "pdf", "doc", "docx"};
    const char* code_ext[] = {"c", "h", "cpp", "hpp", "py", "js"};
    
    PickFilter filters[] = {
        {"All Files", NULL, 0},
        {"Images", img_ext, 5},
        {"Documents", doc_ext, 5},
        {"Source Code", code_ext, 6}
    };
    
    PickFileOptions opts = {0};
    opts.title = "Select a File";
    opts.filters = filters;
    opts.filter_count = 4;
    
    pick_file(&opts, on_file_selected, NULL);
    set_status("Opening file picker...", BLUE);
    ui.waiting_for_dialog = true;
}

static void test_multi_file_picker(void) {
    if (ui.waiting_for_dialog) return;
    
    PickFileOptions opts = {0};
    opts.title = "Select Multiple Files";
    opts.allow_multiple = true;
    
    pick_files(&opts, on_files_selected, NULL);
    set_status("Opening multi-file picker...", BLUE);
    ui.waiting_for_dialog = true;
}

static void test_folder_picker(void) {
    if (ui.waiting_for_dialog) return;
    
    PickFileOptions opts = {0};
    opts.title = "Select a Folder";
    
    pick_folder(&opts, on_folder_selected, NULL);
    set_status("Opening folder picker...", BLUE);
    ui.waiting_for_dialog = true;
}

static void test_save_dialog(void) {
    if (ui.waiting_for_dialog) return;
    
    const char* txt_ext[] = {"txt", "md"};
    const char* data_ext[] = {"json", "xml", "csv"};
    
    PickFilter filters[] = {
        {"Text Files", txt_ext, 2},
        {"Data Files", data_ext, 3}
    };
    
    PickFileOptions opts = {0};
    opts.title = "Save Your File";
    opts.default_name = "untitled.txt";
    opts.filters = filters;
    opts.filter_count = 2;
    opts.can_create_dirs = true;
    
    pick_save(&opts, on_save_selected, NULL);
    set_status("Opening save dialog...", BLUE);
    ui.waiting_for_dialog = true;
}

static void test_alert(void) {
    if (ui.waiting_for_dialog) return;
    
    pick_alert("Information", 
               "This is a simple alert dialog.\n\n"
               "It displays information to the user and only has an OK button.", 
               NULL);
    set_status("Showing alert...", BLUE);
    set_result("Alert shown (no callback for simple alerts)");
}

static void test_confirm(void) {
    if (ui.waiting_for_dialog) return;
    
    pick_confirm("Confirm Action", 
                 "Are you sure you want to proceed with this action?\n\n"
                 "This dialog has OK and Cancel buttons.", 
                 NULL, 
                 on_message_response, 
                 "Confirm");
    set_status("Showing confirmation...", BLUE);
    ui.waiting_for_dialog = true;
}

static void test_yes_no(void) {
    if (ui.waiting_for_dialog) return;
    
    PickMessageOptions opts = {0};
    opts.title = "Question";
    opts.message = "Do you like this demo?";
    opts.detail = "Your feedback helps improve the library!";
    opts.buttons = PICK_BUTTON_YES_NO;
    opts.style = PICK_STYLE_QUESTION;
    opts.icon_type = PICK_ICON_STOP;
    
    pick_message(&opts, on_message_response, "Yes/No");
    set_status("Showing Yes/No dialog...", BLUE);
    ui.waiting_for_dialog = true;
}

static void test_warning(void) {
    if (ui.waiting_for_dialog) return;
    
    PickMessageOptions opts = {0};
    opts.title = "Unsaved Changes";
    opts.message = "Do you want to save your changes before closing?";
    opts.detail = "Your changes will be lost if you don't save them.\n"
                  "This action cannot be undone.";
    opts.buttons = PICK_BUTTON_YES_NO_CANCEL;
    opts.style = PICK_STYLE_WARNING;
    opts.icon_type = PICK_ICON_CAUTION;
    
    pick_message(&opts, on_message_response, "Warning");
    set_status("Showing warning...", BLUE);
    ui.waiting_for_dialog = true;
}

static void test_error(void) {
    if (ui.waiting_for_dialog) return;
    
    PickMessageOptions opts = {0};
    opts.title = "Error";
    opts.message = "Failed to load the file.";
    opts.detail = "The file may be corrupted or in an unsupported format.\n\n"
                  "Error code: 0x0000DEAD\n"
                  "Please contact support if this problem persists.";
    opts.buttons = PICK_BUTTON_OK;
    opts.style = PICK_STYLE_ERROR;
    opts.icon_type = PICK_ICON_ERROR;
    
    pick_message(&opts, on_message_response, "Error");
    set_status("Showing error...", BLUE);
    ui.waiting_for_dialog = true;
}

static void test_custom_message(void) {
    if (ui.waiting_for_dialog) return;
    
    PickMessageOptions opts = {0};
    opts.title = "Custom Dialog";
    opts.message = "This is a custom message dialog with multiple options.";
    opts.detail = "You can customize:\n"
                  "• Button configuration\n"
                  "• Icon type\n"
                  "• Dialog style\n"
                  "• And more!";
    opts.buttons = PICK_BUTTON_OK_CANCEL;
    opts.style = PICK_STYLE_INFO;
    opts.icon_type = PICK_ICON_CAUTION;
    
    pick_message(&opts, on_message_response, "Custom");
    set_status("Showing custom message...", BLUE);
    ui.waiting_for_dialog = true;
}


static Button buttons[] = {
    
    {{20, 100, 200, 50}, "File Picker", "Select a single file", DARKBLUE, test_file_picker},
    {{20, 160, 200, 50}, "Multi-File Picker", "Select multiple files", DARKBLUE, test_multi_file_picker},
    {{20, 220, 200, 50}, "Folder Picker", "Select a folder", DARKBLUE, test_folder_picker},
    {{20, 280, 200, 50}, "Save Dialog", "Choose save location", DARKGREEN, test_save_dialog},
    
    
    {{240, 100, 200, 50}, "Alert", "Simple information", DARKGRAY, test_alert},
    {{240, 160, 200, 50}, "Confirm", "OK/Cancel dialog", ORANGE, test_confirm},
    {{240, 220, 200, 50}, "Yes/No", "Question dialog", PURPLE, test_yes_no},
    {{240, 280, 200, 50}, "Custom Message", "Customizable dialog", DARKPURPLE, test_custom_message},
    
    
    {{460, 100, 200, 50}, "Warning", "Three-button warning", GOLD, test_warning},
    {{460, 160, 200, 50}, "Error", "Error message", MAROON, test_error},
};

static const int button_count = sizeof(buttons) / sizeof(buttons[0]);


static bool draw_button(const Button* btn) {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, btn->bounds);
    bool clicked = hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    
    
    if (ui.waiting_for_dialog) {
        hover = false;
        clicked = false;
    }
    
    Color color = btn->color;
    if (ui.waiting_for_dialog) {
        color = GRAY;
    } else if (hover) {
        color.r = (unsigned char)(color.r * 1.2f);
        color.g = (unsigned char)(color.g * 1.2f);
        color.b = (unsigned char)(color.b * 1.2f);
    }
    
    DrawRectangleRounded(btn->bounds, 0.1f, 8, color);
    
    if (hover && !ui.waiting_for_dialog) {
        DrawRectangleLinesEx(btn->bounds, 2, WHITE);
    }
    
    
    int title_width = MeasureText(btn->text, 18);
    int title_x = btn->bounds.x + (btn->bounds.width - title_width) / 2;
    int title_y = btn->bounds.y + 8;
    DrawText(btn->text, title_x, title_y, 18, WHITE);
    
    
    int desc_width = MeasureText(btn->description, 12);
    int desc_x = btn->bounds.x + (btn->bounds.width - desc_width) / 2;
    int desc_y = btn->bounds.y + 30;
    DrawText(btn->description, desc_x, desc_y, 12, RAYWHITE);
    
    return clicked;
}


static void update_frame(void) {
    
    int windowWidth = GetScreenWidth();
    int windowHeight = GetScreenHeight();
    
    
    if (ui.status_timer > 0) {
        ui.status_timer -= GetFrameTime();
        if (ui.status_timer <= 0) {
            ui.status_color = DARKGRAY;
            strncpy(ui.status, "Ready - Click any button to test a dialog", sizeof(ui.status));
        }
    }
    
    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    
    DrawRectangle(0, 0, windowWidth, 70, (Color){50, 50, 50, 255});
    DrawText("Pick.h Interactive Demo", 20, 15, 32, WHITE);
    DrawText("Test native file and message dialogs across platforms", 20, 48, 16, LIGHTGRAY);
    
    
    const char* platform = "Platform: Native";
#ifdef __EMSCRIPTEN__
    platform = "Platform: Web (Emscripten)";
#elif defined(_WIN32)
    platform = "Platform: Windows";
#elif defined(__APPLE__)
    platform = "Platform: macOS";
#elif defined(__linux__)
    platform = "Platform: Linux";
#endif
    int platform_width = MeasureText(platform, 14);
    DrawText(platform, windowWidth - platform_width - 20, 48, 14, LIGHTGRAY);
    
    
    for (int i = 0; i < button_count; i++) {
        if (draw_button(&buttons[i]) && buttons[i].action) {
            buttons[i].action();
        }
    }
    
    
    DrawRectangle(0, 350, windowWidth, 50, (Color){240, 240, 240, 255});
    DrawText("Status:", 20, 360, 14, DARKGRAY);
    DrawText(ui.status, 80, 360, 14, ui.status_color);
    
    if (ui.waiting_for_dialog) {
        DrawText("Waiting for dialog response...", 20, 378, 12, GRAY);
    }
    
    
    int resultsHeight = windowHeight - 400 - 20;  
    DrawRectangle(0, 400, windowWidth, resultsHeight, (Color){250, 250, 250, 255});
    DrawLine(0, 400, windowWidth, 400, LIGHTGRAY);
    DrawText("Last Result:", 20, 410, 16, DARKGRAY);
    
    
    const char* result_line = ui.last_result;
    int y_offset = 435;
    char line_buffer[256];
    int line_start = 0;
    int maxY = windowHeight - 25;  
    
    for (int i = 0; ui.last_result[i] != '\0'; i++) {
        if (ui.last_result[i] == '\n' || ui.last_result[i + 1] == '\0') {
            int line_length = i - line_start;
            if (ui.last_result[i + 1] == '\0' && ui.last_result[i] != '\n') {
                line_length++;
            }
            
            if (line_length > 0 && line_length < 256) {
                strncpy(line_buffer, &ui.last_result[line_start], line_length);
                line_buffer[line_length] = '\0';
                DrawText(line_buffer, 20, y_offset, 14, DARKBLUE);
                y_offset += 18;
            }
            
            line_start = i + 1;
            
            
            if (y_offset > maxY - 20) {
                DrawText("...", 20, y_offset, 14, GRAY);
                break;
            }
        }
    }
    
    
    DrawRectangle(0, windowHeight - 20, windowWidth, 20, (Color){50, 50, 50, 255});
    DrawText("Click any button to test • Dialogs are native to your OS • Results appear below", 
             10, windowHeight - 16, 12, LIGHTGRAY);
    
    EndDrawing();
}

int main(void) {
    const int screenWidth = 680;
    const int screenHeight = 600;
    
    
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    
    InitWindow(screenWidth, screenHeight, "Pick.h Dialog Demo - Interactive Playground");
    SetWindowMinSize(680, 600);  
    SetTargetFPS(60);
    
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(update_frame, 0, 1);
#else
    while (!WindowShouldClose()) {
        update_frame();
    }
#endif
    
    CloseWindow();
    return 0;
}