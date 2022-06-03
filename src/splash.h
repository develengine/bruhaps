#ifndef SPLASH_H
#define SPLASH_H

#include "bag_engine.h"


void initSplash(void);
void exitSplash(void);

void updateSplash(float dt);
void renderSplash(void);
void renderSplashOverlay(void);

void splashProcessButton(bagE_MouseButton *mb);
void splashProcessMouse(bagE_Mouse *m);

#endif