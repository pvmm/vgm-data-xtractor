#include "raylib.h"
#include "raygui.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
    #include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
#endif

#include "functions.h"

#define GUI_FILE_DIALOGS_IMPLEMENTATION
#include "gui_file_dialogs.h"               // GUI: File Dialogs

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_ERRORS 5

// control status
static enum priority priority = P_DEFAULT;
static char *error_messages[MAX_ERRORS] = { NULL };
static int error_index = -1;

void process_errors(void)
{
	if (gui_status_not(P_FILE_DIALOG) && IsFileDropped())
	{
		append_error_message("Unexpected file dragging. Click on the \"Open File...\" button first.");
		unload_dropped_files();
	}
	if (has_error())
	{
		if (show_error(error_messages[error_index]) != -1)
		{
			drop_error_message();
		}
	}
}

bool has_error(void)
{
	return error_index >= 0;
}

void append_error_message(char* fmt, ...)
{
	if (error_index >= MAX_ERRORS)
	{
		printf("Max error count reached.");
		return;
	}

	va_list ap;
	va_start(ap, fmt);
	error_messages[++error_index] = malloc(256);
	vsnprintf(error_messages[error_index], 256, fmt, ap);
	va_end(ap);
}

void drop_error_message(void)
{
	if (error_index > -1)
	{
		free(error_messages[error_index--]);
	}
}

int show_about_box()
{
	if (has_error() || gui_status(P_ERR_DIALOG)) return -1;
	int result = -1;

	if (IsFileDropped())
	{
		set_gui_lock(P_ERR_DIALOG);
		append_error_message("Unexpected file dragging. Click on the \"Open File...\" button first.");
		UnloadDroppedFiles(LoadDroppedFiles());
	} else {
		disable_gui_if(false);
		set_gui_lock(P_MSG_DIALOG);
		result = GuiMessageBox(
			(Rectangle){ GetScreenWidth() / 2 - 200, GetScreenHeight() / 2 - 180, 400, 180 },
			"#191#About", "Hi! This is a message", "OK");
		if (result >= 0) reset_gui_lock(P_MSG_DIALOG);
	}
	return result;
}

int show_button(Rectangle bounds, const char *text)
{
	disable_gui_if(has_error() || gui_status_not(P_DEFAULT));
	return GuiButton(bounds, text);
}

int show_error(char* message)
{
	set_gui_lock(P_ERR_DIALOG);
	disable_gui_if(false); // error dialog has maximum priority
	int result = GuiMessageBox(
		(Rectangle){ GetScreenWidth() / 2 - 200, GetScreenHeight() / 2 - 180, 400, 180 },
		"Error", message, "OK");
	if (result >= 0) reset_gui_lock(P_ERR_DIALOG);
	return result;
}

int show_message(char* title, char* message)
{
	if (has_error() || gui_status(P_ERR_DIALOG)) return -1;

	int result = -1;
	if (IsFileDropped())
	{
		set_gui_lock(P_ERR_DIALOG);
		append_error_message("Unexpected file dragging. Click on the \"Open File...\" button instead.");
		unload_dropped_files();
	} else {
		set_gui_lock(P_MSG_DIALOG);
		result = GuiMessageBox(
			(Rectangle){ GetScreenWidth() / 2 - 200, GetScreenHeight() / 2 - 180, 400, 180 },
			title, message, "OK");
		if (result >= 0) reset_gui_lock(P_MSG_DIALOG);
	}
	return result;
}

int show_load_dialog(const char* title, const char* extension, FilePathList* files)
{
	static bool load_error = false;

	// disable on error
	if (has_error())
		return -1;
	// reset status after load error
	if (load_error) {
		load_error = false;
		return -1;
	}

	disable_gui_if(gui_status_not(P_FILE_DIALOG) && gui_status_not(P_DEFAULT));
	set_gui_lock(P_FILE_DIALOG);

	char filename[512] = { 0 };
#if defined(CUSTOM_MODAL_DIALOGS) 
	int result = GuiFileDialog(DIALOG_MESSAGE, title, filename, "OK", "Just drag and drop your file!");
	// process wrong file input
	if (IsFileDropped())
	{
		*files = LoadDroppedFiles();
		for (int i = 0; i < files->count; ++i)
		{
			if (!IsFileExtension(files->paths[i], extension))
			{
				append_error_message("Wrong file type: %s", get_file_name(files->paths[i]));
			}
		}
		if (has_error())
		{
			// reset dragged files on error
			unload_dropped_files();
			result = -1;
			load_error = true;
		} else {
			result = files->count;
		}
	} else if (result == 1) {
		result = 0;
	}
#else
	char filters[10];
	snprintf(filters, 10, "*%s", extension);
	int result = GuiFileDialog(DIALOG_OPEN_FILE, title, filename, filters, message);
#endif
	// reset status after modal
	if (result >= 0) reset_gui_lock(P_FILE_DIALOG);
	return result;
}

char* get_file_name(char* path)
{
	char *s;

	if (*(s = strrchr(path, '/')) != '\0') {
		return s + 1;
	}

	return path;
}

bool gui_status(enum priority p)
{
	if (p == P_DEFAULT) return priority == p;
	return (priority & p) > 0;
}

bool gui_status_not(enum priority p)
{
	if (p == P_DEFAULT) return priority != p;
	return (priority & p) == 0;
}

void set_gui_lock(enum priority p)
{
	priority |= p;
}

void reset_gui_lock(enum priority p)
{
	priority &= ~p;
}

void mayble_block_gui(void)
{
	if (priority > P_DEFAULT && !GuiIsLocked()) {
		GuiLock();
		GuiSetAlpha(0.5f);
	} else if (priority == P_DEFAULT) {
		GuiUnlock();
		GuiSetAlpha(1.0f);
	}
}



void load_file(char* filename)
{
	printf("function load_file(%s) called\n", get_file_name(filename));
}

void disable_gui_if(bool cond)
{
	// lock only once because setting alpha again will make things invisible.
	if (!GuiIsLocked() && cond) {
		GuiLock();
		GuiSetAlpha(0.5f);
	} else if (!cond) {
		GuiUnlock();
		GuiSetAlpha(1.0f);
	}
}

void unload_dropped_files(void)
{
	UnloadDroppedFiles(LoadDroppedFiles());
}
