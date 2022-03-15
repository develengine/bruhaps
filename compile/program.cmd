
cl /std:c11 /wd5105 /nologo /EHsc /Feprogram win32/bag_win32.c src/main.c src/utils.c glad/src/gl.c /Isrc /Iglad/include User32.lib Gdi32.lib Opengl32.lib

@echo off
