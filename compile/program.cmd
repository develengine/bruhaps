
cl /std:c11 /wd5105 /nologo /EHsc /Feprogram win32/bag_win32.c src/main.c src/utils.c src/res.c src/animation.c src/terrain.c glad/src/gl.c /Isrc /Iglad/include /DDEBUG User32.lib Gdi32.lib Opengl32.lib

@echo off
