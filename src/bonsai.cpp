#include "bonsai.h"
#include <math.h>

BonsaiCell grid[BONSAI_WIDTH][BONSAI_HEIGHT];

void putCell(int x, int y, char c, CharType t) {
    if (x >= 0 && x < BONSAI_WIDTH && y >= 0 && y < BONSAI_HEIGHT) {
        // For leaves, we randomly assign as some character so we can render them as mixed red/black later.
        // We will just store the intended leaf character.
        grid[x][y] = {c, t};
    }
}

void clearBonsai() {
    for (int x = 0; x < BONSAI_WIDTH; x++) {
        for (int y = 0; y < BONSAI_HEIGHT; y++) {
            grid[x][y] = {' ', TYPE_EMPTY};
        }
    }
}

// Recursive function to grow branch
void branch(float x, float y, float angle, float length) {
    if (length < 2) {
        // Draw leaves around the tip randomly
        putCell((int)x, (int)y, '&', TYPE_LEAF);
        putCell((int)x-1, (int)y, '*', TYPE_LEAF);
        putCell((int)x+1, (int)y, '&', TYPE_LEAF);
        putCell((int)x, (int)y-1, '&', TYPE_LEAF);
        putCell((int)x, (int)y+1, '*', TYPE_LEAF);
        return;
    }
    
    float newX = x + cos(angle) * length;
    // Y expands downwards in terminal coordinate system, so subtract sine
    float newY = y - sin(angle) * length; 

    // Line drawing using Bresenham's line algorithm mapped to text cell sizes
    int x1 = (int)round(x);
    int y1 = (int)round(y);
    int x2 = (int)round(newX);
    int y2 = (int)round(newY);
    
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    
    // Pick character roughly matching slope
    char lineChar = '|';
    if (abs(x2 - x1) > abs(y2 - y1)) {
        if ((x2 - x1 > 0 && y2 - y1 > 0) || (x2 - x1 < 0 && y2 - y1 < 0)) lineChar = '\\';
        else lineChar = '/';
    }

    while (true) {
        putCell(x1, y1, lineChar, TYPE_WOOD);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
    
    // Branch creation probability
    float branchAngle1 = angle - (float)random(10, 45) * M_PI / 180.0;
    float branchAngle2 = angle + (float)random(10, 45) * M_PI / 180.0;
    
    // Recursively grow
    branch(newX, newY, branchAngle1, length * 0.75); // Slightly longer branches
    if (random(100) > 10) { // 90% chance to bifurcate for a fuller tree
        branch(newX, newY, branchAngle2, length * 0.65);
    }
}

void generateBonsai() {
    clearBonsai();
    randomSeed(esp_random()); // Seed using ESP32 hardware random number generator
    
    int cx = BONSAI_WIDTH / 2;
    int btm = BONSAI_HEIGHT - 1;
    
    // Create classic base pot representation
    putCell(cx - 3, btm, '\\', TYPE_WOOD);
    putCell(cx + 3, btm, '/', TYPE_WOOD);
    for (int i = cx - 2; i <= cx + 2; i++) putCell(i, btm, '_', TYPE_WOOD);

    putCell(cx - 4, btm - 1, '\\', TYPE_WOOD);
    putCell(cx + 4, btm - 1, '/', TYPE_WOOD);
    for (int i = cx - 3; i <= cx + 3; i++) putCell(i, btm - 1, '_', TYPE_WOOD);
    
    // Start trunk recursion
    // Start trunk right at the top of the pot, with a longer initial length to grow taller
    branch(cx, btm - 2, M_PI/2, 7.5);
}

BonsaiCell getBonsaiCell(int x, int y) {
    if (x >= 0 && x < BONSAI_WIDTH && y >= 0 && y < BONSAI_HEIGHT) {
        return grid[x][y];
    }
    return {' ', TYPE_EMPTY};
}
