#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#if defined(PLATFORM_WEB)
	#define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
	#include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
#endif

#undef RAYGUI_IMPLEMENTATION                // Avoid including raygui implementation again

#include "functions.h"

#define ALLOWED_FILE_EXT    ".txt"

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const char* tool_name = TOOL_NAME;
static const char* tool_version = TOOL_VERSION;
static const char* tool_description = TOOL_DESCRIPTION;

// NOTE: Max length depends on OS, in Windows MAX_PATH = 256
static char in_filename[512] = { 0 };
static char out_filename[512] = { 0 };
bool save_changes_required = false;

int main()
{
	// hide warnings since this is an incomplete example
	(void)tool_description;
	(void)out_filename;
	(void)tool_name;
	(void)tool_version;
	(void)in_filename;

	InitWindow(800, 600, "Raygui Sample App");
	SetTargetFPS(60);

	int result;
	FilePathList files;
	bool request_load_dialog = false;
	bool request_about_box = false;

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

		process_errors();

		if (show_button((Rectangle){ 24, 24, 120, 30 }, "#11#Open file..."))
		{
			request_load_dialog = true;
		}

		if (request_load_dialog && (result = show_load_dialog("Load file", ALLOWED_FILE_EXT, &files)) >= 0)
		{
			if (result > 0)
			{
				for (int i = 0; i < files.count; ++i)
				{
					load_file(files.paths[0]);
				}
				//SetWindowTitle(TextFormat("%s v%s | File: %s", tool_name, tool_version, GetFileName(in_filename)));
#ifdef CUSTOM_MODAL_DIALOGS
				unload_dropped_files();
#endif
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

		EndDrawing();
	}

	CloseWindow();
	return 0;
}
