#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "raylib.h"
#include "raygui.h"
#include "functions.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
    #include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
#endif

#define VGM_HEADER_SIZE 0x40
#define VGM_EOF_OFFSET  0x04
#define VGM_DATA_OFFSET 0x34
#define MAX_BLOCK_COUNT 1000

// Structure to hold data block information
typedef struct {
    uint32_t type;
    uint32_t size;
    uint8_t *data;
} VGMDataBlock;

// Dropdown options
static bool changed = false;
static int data_blocks = 0;
static char block_options[1000] = "#06#no blocks found";

void download_block(int i)
{
#if defined(PLATFORM_WEB)
    char filename[50];
    snprintf(filename, 50, "block_%i.raw", i);
    // Download file from MEMFS (emscripten memory filesystem)
    emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", filename, GetFileName(filename)));
#endif
}

char* get_data_blocks(void)
{
    if (changed)
    {
        char filename[50];
	snprintf(block_options, 1000, "#06#no block");
        for (int i = 0; i < data_blocks; ++i)
        {
            strcat(block_options, ";#06#");
            snprintf(filename, 50, "block_%i.raw", i);
            strcat(block_options, filename);
        }
	changed = false;
    }

    return block_options;
}

bool save_block(int count, uint8_t* file_data, size_t size)
{
    char filename[100];
    snprintf(filename, 100, "block_%i.raw", count);

    FILE* file = fopen(filename, "wb");
    if (!file) {
        append_error_message("Error opening file \"%s\"\n", filename);
        return false;
    }

    if (fwrite(file_data, 1, size, file) != size) {
        append_error_message("Error writing raw sample: out of space?\n");
        fclose(file);
        return false;
    }

    fclose(file);
    append_error_message("File saved to %s\n", filename);

    return true;
}

size_t extract_data_blocks(uint8_t* file_data, size_t data_size, VGMDataBlock* blocks)
{
    size_t block_count = 0;
    uint8_t *ptr = file_data;
    uint8_t *end = file_data + data_size;
    free(file_data);

    // Search for 0x67 command byte and extract data blocks
    while (ptr < end)
    {
        if (*ptr == 0x67 && (ptr + 7 < end))
        {
            ptr++; // skip command
            uint8_t type = *ptr++;  // data type
            // compatibility mode detected, reading again.
            if (type == 0x66) type = *ptr++;
            uint32_t size = *(uint32_t *)ptr; // size
            // ignore most significant bit
            size &= 0x7fffffff;
            ptr += 4;

            if (ptr + size <= end)
            {
                blocks[block_count].type = type;
                //printf("%zu block type: %x\n", block_count, blocks[block_count].type);
                blocks[block_count].size = size - 8;
                //printf("%zu block size: %u\n", block_count, blocks[block_count].size);
                if (blocks[block_count].size > 0)
                {
                    blocks[block_count].data = (uint8_t *)malloc(size);
                    if (!blocks[block_count].data)
                    {
                        perror("Memory allocation error");
                        free(file_data);
                        return 0;
                    }
                    memcpy(blocks[block_count].data, ptr, size);
                    if (!save_block(block_count, blocks[block_count].data, blocks[block_count].size))
                    {
                        return 0;
                    }

                    block_count++;
                }

                if (block_count >= MAX_BLOCK_COUNT)
                {
                    append_error_message("Reserved memory exhausted.\n");
                    break;
                }
            }
            ptr += size - 8;
        }
        else
        {
            ptr++;
        }
    }

    return block_count;
}

// Function to extract sample data from a VGM file
bool load_file(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file) {
        append_error_message("Error opening file \"%s\"\n", filename);
        return -1;
    }

    // Read VGM header
    uint8_t header[VGM_HEADER_SIZE];
    if (fread(header, 1, VGM_HEADER_SIZE, file) != VGM_HEADER_SIZE) {
        append_error_message("Error reading VGM header: file too short\n");
        fclose(file);
        return -1;
    }

    // Check for VGM magic number ('Vgm ')
    if (header[0] != 'V' || header[1] != 'g' || header[2] != 'm' || header[3] != ' ') {
        append_error_message("Invalid VGM file: VGM magic string not found\n");
        fclose(file);
        return -1;
    }

    // Get the offset to the data section
    uint32_t data_offset = *(uint32_t *)(header + VGM_DATA_OFFSET);
    if (data_offset == 0) {
        data_offset = 0x40; // Default to the end of the header
    } else {
        data_offset += 0x34;
    }

    // Get the total file size from the EOF offset field
    uint32_t eof_offset = *(uint32_t *)(header + VGM_EOF_OFFSET);
    if (eof_offset == 0) {
        append_error_message("Invalid EOF offset in header\n");
        fclose(file);
        return -1;
    }

    // Calculate the size of the data
    size_t file_size = eof_offset + 4;
    size_t data_size = file_size - data_offset;
    //printf("File data extracted (%zu bytes)\n", data_size);

    // Allocate memory for all file commands
    uint8_t *file_data = (uint8_t *)malloc(data_size);
    if (!file_data) {
        append_error_message("Memory allocation failed\n");
        fclose(file);
        return -1;
    }

    // Seek to the data offset and read the sample data
    fseek(file, data_offset, SEEK_SET);
    if (fread(file_data, 1, data_size, file) != data_size) {
        append_error_message("Error reading sample data\n");
        free(file_data);
        fclose(file);
        return -1;
    }
    fclose(file);

    VGMDataBlock blocks[MAX_BLOCK_COUNT];
    data_blocks = extract_data_blocks(file_data, data_size, blocks);
    if (data_blocks) changed = true;
    return data_blocks >= 0;
}
