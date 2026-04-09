#ifndef LIFE_H
#define LIFE_H

struct field{
    char* current;
    char* next;
    int width, height;
};

extern struct field Field;

int countNeighbors(int x0, int y0);

void updateField();

void printField();

void loadSample();

void initField(int width, int height);

void freeField();

#endif