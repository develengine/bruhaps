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
    unsigned imageProgram;

    unsigned textFont;

    unsigned dummyVao;
} GUI;

extern GUI gui;


typedef void (*GUIButtonCallback)(void);


void initGUI(void);
void exitGUI(void);

void guiUpdateResolution(int windowWidth, int windowHeight);

void guiBeginRect(void);
void guiBeginText(void);
void guiBeginImage(void);

void guiUseImage(unsigned texture);

void guiDrawRect(int x, int y, int w, int h, Color color);
void guiDrawText(const char *text, int x, int y, int w, int h, int s, Vector color);
void guiDrawImageColored(int x, int y, int w, int h,
                         float tx, float ty, float tw, float th,
                         Color color);
void guiDrawImage(int x, int y, int w, int h,
                  float tx, float ty, float tw, float th);


#endif
