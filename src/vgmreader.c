#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>

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
struct VGMDataBlock {
    uint32_t type;
    uint32_t size;
    uint8_t *data;
};

static const char* type_descriptions[] = {
    "uncompressed recorded streams",
    "compressed recorded streams",
    "decompression table",
    "ROM/RAM image dumps",
    "RAM writes (<= 64 KB)",
    "RAM writes (> 64 KB)",
};

static struct VGMDataBlock blocks[MAX_BLOCK_COUNT];

// Dropdown options
static bool changed = false;
static int data_blocks = 0;
static char block_options[1000] = "#113#no blocks found";

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
	snprintf(block_options, 1000, "#113#no block");
        for (int i = 0; i < data_blocks; ++i)
        {
            const char* desc;
            strcat(block_options, ";#06#");
	    if (blocks[i].type <= 0x3f)
                desc = type_descriptions[0];
	    else if (blocks[i].type <= 0x7e)
                desc = type_descriptions[1];
	    else if (blocks[i].type == 0x7f)
                desc = type_descriptions[2];
	    else if (blocks[i].type <= 0xbf)
                desc = type_descriptions[3];
	    else if (blocks[i].type <= 0xdf)
                desc = type_descriptions[4];
	    else {
                desc = type_descriptions[5];
	    }
            snprintf(filename, 50, "block_%i.raw: %s", i, desc);
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
    //printf("File saved to %s\n", filename);

    return true;
}

size_t extract_data_blocks(uint8_t* file_data, size_t data_size)
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

bool check_header(const char* header)
{
    // Check for VGM magic number ('Vgm ')
    if (header[0] != 'V' || header[1] != 'g' || header[2] != 'm' || header[3] != ' ') {
        append_error_message("Invalid VGM file: VGM magic string not found\n");
        return false;
    }
    return true;
}

uint32_t get_data_offset(const char* header)
{
    // Get the offset to the data section
    uint32_t data_offset = *(uint32_t *)(header + VGM_DATA_OFFSET);
    if (data_offset == 0) {
        data_offset = 0x40; // Default to the end of the header
    } else {
        data_offset += 0x34;
    }
    return data_offset;
}

uint32_t get_eof_offset(const char* header)
{
    // Get the total file size from the EOF offset field
    uint32_t eof_offset = *(uint32_t *)(header + VGM_EOF_OFFSET);
    if (eof_offset == 0) {
        append_error_message("Invalid EOF offset in header\n");
        return 0;
    }
    return eof_offset;
}

bool _load_gzfile(const char* filename)
{
    gzFile file = gzopen(filename, "rb");
    if (!file) {
        append_error_message("Failed to open .gz file");
        return false;
    }

    uint8_t header[VGM_HEADER_SIZE];
    if (gzread(file, header, VGM_HEADER_SIZE) != VGM_HEADER_SIZE) {
        append_error_message("Error reading VGM header: file too short\n");
        gzclose(file);
        return false;
    }

    if (!check_header(header)) return false;

    uint32_t data_offset = get_data_offset(header);

    uint32_t eof_offset = get_eof_offset(header);
    if (!eof_offset) {
        gzclose(file);
        return false;
    }

    // Calculate the size of the data
    size_t file_size = eof_offset + 4;
    size_t data_size = file_size - data_offset;
    //printf("File data extracted (%zu bytes)\n", data_size);

    // Allocate memory for all file commands
    uint8_t *file_data = (uint8_t *)malloc(data_size);
    if (!file_data) {
        append_error_message("Memory allocation failed\n");
        gzclose(file);
        return false;
    }

    if (gzseek(file, data_offset, SEEK_SET) == -1)
    {
        append_error_message("Error seeking commands\n");
        free(file_data);
        gzclose(file);
        return false;
    }

    if (gzread(file, file_data, data_size) != data_size)
    {
        append_error_message("Error reading command data\n");
        free(file_data);
        gzclose(file);
        return false;
    }

    gzclose(file);

    data_blocks = extract_data_blocks(file_data, data_size);
    if (data_blocks) changed = true;
    return data_blocks >= 0;
}

bool _load_file(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (!file) {
        append_error_message("Error opening file \"%s\"\n", filename);
        return false;
    }

    uint8_t header[VGM_HEADER_SIZE];
    if (fread(header, 1, VGM_HEADER_SIZE, file) != VGM_HEADER_SIZE) {
        append_error_message("Error reading VGM header: file too short\n");
        fclose(file);
        return false;
    }

    if (!check_header(header)) return false;
    uint32_t data_offset = get_data_offset(header);

    uint32_t eof_offset = get_eof_offset(header);
    if (!eof_offset) {
        fclose(file);
        return false;
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
        return false;
    }

    if (fseek(file, data_offset, SEEK_SET) == -1)
    {
        append_error_message("Error seeking commands\n");
        free(file_data);
        fclose(file);
        return false;
    }

    if (fread(file_data, 1, data_size, file) != data_size)
    {
        append_error_message("Error reading command data\n");
        free(file_data);
        fclose(file);
        return false;
    }

    fclose(file);

    data_blocks = extract_data_blocks(file_data, data_size);
    if (data_blocks) changed = true;
    return data_blocks >= 0;
}

bool load_file(const char* filename)
{
    return IsFileExtension(filename, ".vgz") ? _load_gzfile(filename) : _load_file(filename);
}
