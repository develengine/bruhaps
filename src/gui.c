#include "gui.h"

#include "bag_engine.h"
#include "res.h"
#include "core.h"

GUI gui;


static char textScratch[MAX_GUI_TEXT_LENGTH];


void initGUI(void)
{
    glCreateVertexArrays(1, &gui.dummyVao);

    gui.rectProgram = createProgram(
            "shaders/rect_vertex.glsl",
            "shaders/rect_fragment.glsl"
    );

    gui.textFont = createTexture("res/font_base.png");
    gui.textProgram = createProgram(
            "shaders/text_vertex.glsl",
            "shaders/text_fragment.glsl"
    );

}


void exitGUI(void)
{
    glDeleteProgram(gui.rectProgram);
    glDeleteProgram(gui.textProgram);
    glDeleteTextures(1, &gui.textFont);
}


void guiBeginRect(void)
{
    glUseProgram(gui.rectProgram);
}


void guiBeginText(void)
{
    glUseProgram(gui.textProgram);
    glBindTextureUnit(0, gui.textFont);
}


void guiDrawRect(int x, int y, int w, int h, Vector color)
{
    glProgramUniform2i(gui.rectProgram, 1, x, y);
    glProgramUniform2i(gui.rectProgram, 2, w, h);
    glProgramUniform4fv(gui.rectProgram, 3, 1, color.data);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void guiDrawText(const char *text, int x, int y, int w, int h, int s, Vector color)
{
    int textLen = 0;
    for (; text[textLen] != '\0'; ++textLen)
        textScratch[textLen] = text[textLen];

    int wordCount = textLen / 16 + (textLen % 16 != 0);

    glProgramUniform2i(gui.textProgram, 1, x, y);
    glProgramUniform2i(gui.textProgram, 2, w, h);
    glProgramUniform2i(gui.textProgram, 3, s, s);
    glProgramUniform4fv(gui.textProgram, 4, 1, color.data);
    glProgramUniform4uiv(gui.textProgram, 5, wordCount, (unsigned *)textScratch);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, textLen);
}