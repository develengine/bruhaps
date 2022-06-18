#ifndef SPLASH_H
#define SPLASH_H

#include "bag_engine.h"

#include <stdbool.h>


void initSplash(void);
void exitSplash(void);

void updateSplash(float dt);
void renderSplash(void);
void renderSplashOverlay(void);

void splashProcessButton(bagE_MouseButton *mb, bool down);
void splashProcessMouse(bagE_Mouse *m);

#endif
