#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

char* loadFile(const char* path){
    FILE* f = fopen(path, "r");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    char * data = malloc(size + 1);
    size_t read = fread(data, sizeof(char), size, f);
    data[read] = '\0';

    fclose(f);
    return data;
}