#ifndef BONSAI_H
#define BONSAI_H

#include <Arduino.h>

// Terminal grid dimensions for the right-hand panel
#define BONSAI_WIDTH 40
#define BONSAI_HEIGHT 25

// ASCII characters representation
#define CHAR_TRUNK '|'
#define CHAR_BRANCH_LEFT '\\'
#define CHAR_BRANCH_RIGHT '/'
#define CHAR_LEAF '&'
#define CHAR_LEAF_ALT '*'
#define CHAR_EMPTY ' '

enum CharType {
    TYPE_EMPTY = 0,
    TYPE_WOOD = 1,
    TYPE_LEAF = 2
};

struct BonsaiCell {
    char ch;
    CharType type; // Useful for rendering leaves in red
};

// Generates a new bonsai tree into the internal buffer
void generateBonsai();

// Gets the buffer directly for rendering
BonsaiCell getBonsaiCell(int x, int y);

#endif
