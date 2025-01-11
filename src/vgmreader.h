#ifndef _VGMDATA_H_
#define _VGMDATA_H_

#include "raylib.h"

void download_block(int i);

bool load_gzfile(char* filename);

bool load_file(char* filename);

bool load_files(FilePathList* files);

char* get_data_blocks(void);

#endif // _VGMDATA_H_
