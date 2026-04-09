#include <stdio.h>
#include <stdlib.h>
#include "life.h"

struct field Field;

int countNeighbors(int x0, int y0){
    int count = 0 - Field.current[y0 * Field.width + x0];
    for(int dx = -1; dx <= 1; ++dx){
        for(int dy = -1; dy <= 1; ++dy){
            int x = (x0 + Field.width + dx) % Field.width;
            int y = (y0 + Field.height + dy) % Field.height;
            count += Field.current[y * Field.width + x];
        }
    }
    return count;
}

void updateField(){
    for(int y = 0; y < Field.height; ++y){
        for(int x = 0; x < Field.width; ++x){
            int count = countNeighbors(x, y);
            int live = Field.current[y * Field.width + x];
            Field.next[y * Field.width + x] = ((count > (2 - live)) && (count < 4));
        }
    }
    char * tmp;
    tmp = Field.current;
    Field.current = Field.next;
    Field.next = tmp;
}

void printField(){
    for(int y = 0; y < Field.height; ++y){
        for(int x = 0; x < Field.width; ++x){
            if (Field.current[y * Field.width + x] == 0) printf(".");
            else printf("@");
        }
        printf("\n");
    }
    printf("\n");
}

void loadSample(){
    Field.current[5 * Field.width + 3] = 1;
    Field.current[6 * Field.width + 4] = 1;
    Field.current[6 * Field.width + 5] = 1;
    Field.current[7 * Field.width + 3] = 1;
    Field.current[7 * Field.width + 4] = 1;
}

void initField(int width, int height){
    Field.width = width;
    Field.height = height;
    Field.current = (char*)calloc(Field.width * Field.height, sizeof(char));
    Field.next = (char*)calloc(Field.width * Field.height, sizeof(char));
}

void freeField(){
    free(Field.current);
    free(Field.next);
    Field.current = NULL;
    Field.next = NULL;
    Field.width = 0;
    Field.height = 0;
}