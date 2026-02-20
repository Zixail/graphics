#ifndef LIFE_H
#define LIFE_H

struct life_field{
    char* current;
    char* next;
    int width, height;
};

extern struct life_field Field;

int countNeighbors(int x0, int y0);

void updateField();

void printField();

void loadSample();

void fieldInit(int width, int height);

#endif