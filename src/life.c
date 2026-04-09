#include <stdio.h>
#include <stdlib.h>
#include "life.h"

struct life_state Life;

int countNeighbors(int x0, int y0){
    int count = 0 - Life.current[y0 * Life.width + x0];
    for(int dx = -1; dx <= 1; ++dx){
        for(int dy = -1; dy <= 1; ++dy){
            int x = (x0 + Life.width + dx) % Life.width;
            int y = (y0 + Life.height + dy) % Life.height;
            count += Life.current[y * Life.width + x];
        }
    }
    return count;
}

void updateField(){
    for(int y = 0; y < Life.height; ++y){
        for(int x = 0; x < Life.width; ++x){
            int count = countNeighbors(x, y);
            int live = Life.current[y * Life.width + x];
            Life.next[y * Life.width + x] = ((count > (2 - live)) && (count < 4));
        }
    }
    char * tmp;
    tmp = Life.current;
    Life.current = Life.next;
    Life.next = tmp;
}

void printField(){
    for(int y = 0; y < Life.height; ++y){
        for(int x = 0; x < Life.width; ++x){
            if (Life.current[y * Life.width + x] == 0) printf(".");
            else printf("@");
        }
        printf("\n");
    }
    printf("\n");
}

void loadSample(){
    Life.current[5 * Life.width + 3] = 1;
    Life.current[6 * Life.width + 4] = 1;
    Life.current[6 * Life.width + 5] = 1;
    Life.current[7 * Life.width + 3] = 1;
    Life.current[7 * Life.width + 4] = 1;
}

void initField(int width, int height){
    Life.width = width;
    Life.height = height;
    Life.current = (char*)calloc(Life.width * Life.height, sizeof(char));
    Life.next = (char*)calloc(Life.width * Life.height, sizeof(char));
}

void freeField(){
    free(Life.current);
    free(Life.next);
    Life.current = NULL;
    Life.next = NULL;
    Life.width = 0;
    Life.height = 0;
}