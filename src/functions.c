#include "raylib.h"
#include "raygui.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
    #include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
#endif

#include "functions.h"

#include <string.h>
#define GUI_FILE_DIALOGS_IMPLEMENTATION
#include "gui_file_dialogs.h"               // GUI: File Dialogs

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_ERRORS 5

// control status
static int timeout = 0;
static enum priority priority = P_DEFAULT;
static char *error_messages[MAX_ERRORS] = { NULL };
static int error_index = -1;

static char _filename[512] = { 0 };
#if defined(PLATFORM_DESKTOP)
static FilePathList _files = { 0 };
static char* _paths[1]     = { NULL };
#endif

bool delayed(void)
{
	if (timeout > 0) return timeout-- ? true : false;
	//else reset_gui_lock(P_ALL);
	return false;
}

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
		enable_gui();
		set_gui_lock(P_MSG_DIALOG);
		result = GuiMessageBox(
			(Rectangle){ GetScreenWidth() / 2 - 200, GetScreenHeight() / 2 - 180, 400, 180 },
			"#191#About VGM-data-xtractor", "\
     GNU GENERAL PUBLIC LICENSE\n\
       Version 3, 29 June 2007\n\
\
Copyright (C) 2007 Free Software Foundation, Inc. <https://fsf.org/>\n\
Everyone is permitted to copy and distribute verbatim copies\n\
of this license document, but changing it is not allowed.", "OK");
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
	enable_gui(); // error dialog has maximum priority
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

int show_load_dialog(const char* title, FilePathList* files)
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

#if defined(CUSTOM_MODAL_DIALOGS) 
	int result = GuiFileDialog(DIALOG_MESSAGE, title, _filename, "OK", "Just drag and drop your file.");
	// process wrong file input
	if (IsFileDropped())
	{
		*files = LoadDroppedFiles();
		for (int i = 0; i < files->count; ++i)
		{
			if (!IsFileExtension(files->paths[i], ".vgm;.vgz"))
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
	int result = GuiFileDialog(DIALOG_OPEN_FILE, title, _filename, "*.vgm;*.vgz", "VGM files (*.vgm, *.vgz)");
	if (result > 0) {
		_files.paths = _paths;
		_files.paths[0] = _filename;
		_files.count = 1;
		*files = _files;
	}
#endif
	// reset status after modal
	if (result >= 0) reset_gui_lock(P_FILE_DIALOG);
	return result;
}

int show_drop_down(Rectangle bounds, char* options, int* index, bool status)
{
	disable_gui_if(gui_status_not(P_DEFAULT) && gui_status_not(P_DROP_DOWN));
	int result = GuiDropdownBox(bounds, options, index, status);
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

void maybe_block_gui(void)
{
	if (priority > P_DEFAULT && !GuiIsLocked()) {
		GuiLock();
		GuiSetAlpha(0.5f);
	} else if (priority == P_DEFAULT) {
		GuiUnlock();
		GuiSetAlpha(1.0f);
	}
}

void enable_gui(void)
{
	GuiUnlock();
	GuiSetAlpha(1.0f);
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
#if defined(PLATFORM_WEB)
	UnloadDroppedFiles(LoadDroppedFiles());
#endif
}
