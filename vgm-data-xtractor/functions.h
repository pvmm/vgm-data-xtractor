#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

char* get_file_name(char* path);

void unload_dropped_files(void);

//
// GUI functions
//

int show_button(Rectangle bounds, const char *text);

int show_about_box(void);

int show_message(char* title, char* message);

int show_load_dialog(const char* title, const char* extension, FilePathList* files);

int show_drop_down(Rectangle bounds, char* options, int* index, bool status);

//
// priority handling
//

enum priority
{
        P_DEFAULT     = 0,
	P_DROP_DOWN   = 1,
        P_MSG_DIALOG  = 2,
        P_FILE_DIALOG = 4,
        P_ERR_DIALOG  = 8,
	P_ALL         = 15,
};

void enable_gui(void);

void disable_gui_if(bool cond);

void set_gui_lock(enum priority p);

void reset_gui_lock(enum priority p);

bool gui_status(enum priority p);

bool gui_status_not(enum priority p);

//
// error handling
//

int show_error(char* message);

bool delayed(void);

void process_errors(void);

bool has_error(void);

void append_error_message(char* fmt, ...);

void drop_error_message(void);

#endif // _FUNCTIONS_H_
