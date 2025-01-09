#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define VGM_HEADER_SIZE 0x40
#define VGM_EOF_OFFSET 0x04
#define VGM_DATA_OFFSET 0x34

// Structure to hold data block information
typedef struct {
    uint32_t type;
    uint32_t size;
    uint8_t *data;
} VGMDataBlock;

// Function to extract sample data from a VGM file
int load_file(const char *filename) {
    printf("load_file() called\n");
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error opening file \"%s\"\n", filename);
        return -1;
    }

    // Read VGM header
    uint8_t header[VGM_HEADER_SIZE];
    if (fread(header, 1, VGM_HEADER_SIZE, file) != VGM_HEADER_SIZE) {
        printf("Error reading VGM header: file too short\n");
        fclose(file);
        return -1;
    }

    // Check for VGM magic number ('Vgm ')
    if (header[0] != 'V' || header[1] != 'g' || header[2] != 'm' || header[3] != ' ') {
        printf("Invalid VGM file: VGM magic string not found\n");
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
        printf("Invalid EOF offset in header\n");
        fclose(file);
        return -1;
    }

    // Calculate the size of the data
    uint32_t file_size = eof_offset + 4;
    uint32_t file_data_size = file_size - data_offset;
    printf("File data extracted (%u bytes)\n", file_data_size);

    // Allocate memory for all file commands
    uint8_t *file_data = (uint8_t *)malloc(file_data_size);
    if (!file_data) {
        perror("Memory allocation failed\n");
        fclose(file);
        return -1;
    }

    // Seek to the data offset and read the sample data
    fseek(file, data_offset, SEEK_SET);
    if (fread(file_data, 1, file_data_size, file) != file_data_size) {
        printf("Error reading sample data\n");
        free(file_data);
        fclose(file);
        return -1;
    }

    size_t block_count = 0;
    uint8_t *ptr = file_data;
    uint8_t *end = file_data + file_size;
    free(file_data);

    return block_count;
}
