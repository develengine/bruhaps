#ifndef GUI_H
#define GUI_H

#include "linalg.h"


typedef struct
{
    int x, y, w, h;
} Rect;


#define MAX_GUI_TEXT_LENGTH (256 * 4 * 4)

typedef struct
{
    unsigned rectProgram;
    
    unsigned textProgram;
    unsigned textFont;

    unsigned dummyVao;
} GUI;

extern GUI gui;


typedef void (*GUIButtonCallback)(void);


void initGUI(void);
void exitGUI(void);

void guiBeginRect(void);
void guiBeginText(void);

void guiDrawRect(int x, int y, int w, int h, Vector color);
void guiDrawText(const char *text, int x, int y, int w, int h, int s, Vector color);


#endif