#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* readFile(const char* filename, size_t* out_size) {
    char cwd[FILENAME_MAX];
    getcwd(cwd, sizeof(cwd));
    
    //TODO: Make it look from where the executable is located instead form current working dir
    char fullPath[FILENAME_MAX];
    snprintf(fullPath, sizeof(fullPath), "%s/build/%s", cwd, filename);
    printf("%s\n", fullPath);
    
    FILE* file = fopen(fullPath, "rb");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(size * sizeof(char));
    if (!buffer) {
        fclose(file);
        perror("Failed to allocate memory");
        return NULL;
    }

    fread(buffer, 1, size, file);
    fclose(file);

    if (out_size != NULL) {
        *out_size = size;
    }
    printf("Shader size: %d\n", size);
    return buffer;
}

