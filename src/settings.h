#ifndef SETTINGS_H
#define SETTINGS_H

#include "bag_engine.h"
#include "gui.h"

#include <stdbool.h>

void settingsProcessMouse(bagE_Mouse *m);
bool settingsClick(bagE_MouseButton *mb, bool down);
void settingsUpdate(float dt);
void settingsRender(void);
bool settingsProcessEsc(void);

void settingsSave(void);
void settingsLoad(void);

#define SETTINGS_WIDTH 530
#define SETTINGS_HEIGHT 230


#endif
