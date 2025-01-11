#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#if defined(PLATFORM_WEB)
	#define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
	#include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
#endif

#undef RAYGUI_IMPLEMENTATION                // Avoid including raygui implementation again

#include "functions.h"
#include "vgmreader.h"

#define ALLOWED_FILE_EXT    ".vgm;.vgz"

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const char* tool_name = TOOL_NAME;
static const char* tool_version = TOOL_VERSION;
static const char* tool_description = TOOL_DESCRIPTION;

// NOTE: Max length depends on OS, in Windows MAX_PATH = 256
bool save_changes_required = false;

int main()
{
	char title[100];
	snprintf("%s: %s, %s", 100, tool_name, tool_description, tool_version);
	InitWindow(800, 600, title);
	SetTargetFPS(60);

	int result;
	FilePathList files;
	bool request_load_dialog = false;
	bool request_about_box = false;
	int cb_index = 0;
	bool cb_edit_mode = false;

	while (!WindowShouldClose())
	{
		//if (delayed()) continue;

		BeginDrawing();
		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

		process_errors();

		if (show_button((Rectangle){ 24, 24, 120, 30 }, "#11#Open file..."))
		{
			request_load_dialog = true;
		}

		if (request_load_dialog && (result = show_load_dialog("Load VGM file", ALLOWED_FILE_EXT, &files)) >= 0)
		{
			if (result > 0)
			{
				for (int i = 0; i < files.count; ++i)
				{
					load_file(files.paths[0]);
				}
				SetWindowTitle(TextFormat("%s v%s | File: %s", tool_name, tool_version, GetFileName(files.paths[0])));
				unload_dropped_files();
				save_changes_required = false;
			}
			request_load_dialog = false;
		}

		if (show_button((Rectangle){ 24, 70, 120, 30 }, "#191#About..."))
			request_about_box = true;

		if (request_about_box && show_about_box() >= 0)
		{
			request_about_box = false;
		}

		// Dropdown at last
		if (show_drop_down((Rectangle){ 200, 24, 576, 30 }, get_data_blocks(), &cb_index, cb_edit_mode))
		{
			if (cb_edit_mode && cb_index > 0)
			{
				printf("selected!\n");
				download_block(cb_index - 1);
			}
			cb_edit_mode = !cb_edit_mode;
		}

		EndDrawing();
	}

	CloseWindow();
	return 0;
}
