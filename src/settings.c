#include "settings.h"

#include "utils.h"
#include "state.h"


typedef enum
{
    SettingsVolume,
    SettingsFOV,
    SettingsSensitivity,
    SettingsFullscreen,
    SettingsBack,

    SettingsElementCount
} SettingsElementID;

#define NO_ELEMENT SettingsElementCount

#define FONT_SIZE 32
#define LABEL_WIDTH   ((FONT_SIZE * 13) / 2)
#define SLIDER_WIDTH   (FONT_SIZE * 8)
#define SLIDER_HEIGHT ((FONT_SIZE * 6)  / 8)
#define BUTTON_WIDTH (SLIDER_WIDTH / 4)

#define SETTINGS_FILE "settings"


static SettingsElementID settingsSelectedElement = NO_ELEMENT;
static bool buttonDown = false;

static Rect settingsElementRects[SettingsElementCount];


static bool volumeCallback(int x, int y, bool click)
{
    (void)y; (void)click;

    if      (x < 0)            x = 0;
    else if (x > SLIDER_WIDTH) x = SLIDER_WIDTH;

    appState.volume = (float)x / SLIDER_WIDTH;

    return false;
}

static bool fovCallback(int x, int y, bool click)
{
    (void)y; (void)click;

    if      (x < 0)            x = 0;
    else if (x > SLIDER_WIDTH) x = SLIDER_WIDTH;

    camState.fov = MINIMUM_FOV + ((float)x / SLIDER_WIDTH) * (MAXIMUM_FOV - MINIMUM_FOV);

    return false;
}

static bool sensitivityCallback(int x, int y, bool click)
{
    (void)y; (void)click;

    if      (x < 0)            x = 0;
    else if (x > SLIDER_WIDTH) x = SLIDER_WIDTH;

    gameState.sensitivity = MIN_SENSITIVITY
                          + ((float)x / SLIDER_WIDTH) * (MAX_SENSITIVITY - MIN_SENSITIVITY);

    return false;
}

static bool fullscreenCallback(int x, int y, bool click)
{
    (void)x; (void)y;

    if (click) {
        appState.fullscreen = !appState.fullscreen;
        bagE_setFullscreen(appState.fullscreen);
    }

    return false;
}

static bool backCallback(int x, int y, bool click)
{
    (void)x; (void)y;
    return click;
}


static GUIElementCallback settingsElementCallbacks[] = {
    [SettingsVolume]      = volumeCallback,
    [SettingsFOV]         = fovCallback,
    [SettingsSensitivity] = sensitivityCallback,
    [SettingsFullscreen]  = fullscreenCallback,
    [SettingsBack]        = backCallback,
};

static_assert(length(settingsElementCallbacks) == SettingsElementCount,
              "unfilled element callback");


static int xOff = 0;
static int yOff = 0;


void settingsProcessMouse(bagE_Mouse *m)
{
    int x = m->x;
    int y = m->y;

    if (buttonDown && settingsSelectedElement != NO_ELEMENT) {
        Rect rect = settingsElementRects[settingsSelectedElement];
        settingsElementCallbacks[settingsSelectedElement](x - rect.x, y - rect.y, false);
        return;
    }

    for (SettingsElementID id = 0; id < SettingsElementCount; ++id) {
        Rect rect = settingsElementRects[id];

        if (x >= rect.x && x <= rect.x + rect.w
         && y >= rect.y && y <= rect.y + rect.h) {
            if (id == settingsSelectedElement)
                return;

            settingsSelectedElement = id;
            return;
        }
    }

    settingsSelectedElement = NO_ELEMENT;
}


bool settingsClick(bagE_MouseButton *mb, bool down)
{
    buttonDown = down;

    if (settingsSelectedElement != NO_ELEMENT) {
        int x = mb->x;
        int y = mb->y;
        Rect rect = settingsElementRects[settingsSelectedElement];
        if (settingsElementCallbacks[settingsSelectedElement](x - rect.x, y - rect.y, down)) {
            settingsSave();
            return true;
        }
    }

    return false;
}


void settingsUpdate(float dt)
{
    (void)dt;

    settingsElementRects[SettingsVolume] = (Rect)
            { xOff + LABEL_WIDTH, yOff + FONT_SIZE * 0, SLIDER_WIDTH, FONT_SIZE };
    settingsElementRects[SettingsFOV] = (Rect)
            { xOff + LABEL_WIDTH, yOff + FONT_SIZE * 1, SLIDER_WIDTH, FONT_SIZE };
    settingsElementRects[SettingsSensitivity] = (Rect)
            { xOff + LABEL_WIDTH, yOff + FONT_SIZE * 2, SLIDER_WIDTH, FONT_SIZE };

    int buttonOffset = (SLIDER_WIDTH - BUTTON_WIDTH) / 2;

    settingsElementRects[SettingsFullscreen] = (Rect)
            { xOff + LABEL_WIDTH + buttonOffset, yOff + FONT_SIZE * 3, BUTTON_WIDTH, FONT_SIZE };

    settingsElementRects[SettingsBack] = (Rect)
            { xOff, yOff + FONT_SIZE * 5, FONT_SIZE * 4, FONT_SIZE * 2 };
}


bool settingsProcessEsc(void)
{
    settingsSave();
    return true;
}


void settingsRender(void)
{
    Color rectColor = {{ 0.1f, 0.2f, 0.3f, 1.0f }};
    Color knobColor = {{ 1.0f, 1.0f, 1.0f, 1.0f }};
    int yo = (FONT_SIZE - SLIDER_HEIGHT) / 2;

    guiBeginRect();
                                        /* NOTE: kinda weird */
    for (SettingsElementID id = 0; id < SettingsBack; ++id) {
        Rect rect = settingsElementRects[id];
        guiDrawRect(rect.x, rect.y + yo, rect.w, SLIDER_HEIGHT, rectColor);
    }

    float sliderOffsets[] = {
        [SettingsVolume]      = appState.volume,
        [SettingsFOV]         = (camState.fov - MINIMUM_FOV) / (MAXIMUM_FOV - MINIMUM_FOV),
        [SettingsSensitivity] = (gameState.sensitivity - MIN_SENSITIVITY)
                              / (MAX_SENSITIVITY - MIN_SENSITIVITY),
    };
                                        /* NOTE: kinda weird */
    for (SettingsElementID id = 0; id < SettingsFullscreen; ++id) {
        Rect rect = settingsElementRects[id];
        int x = rect.x + SLIDER_WIDTH * sliderOffsets[id] - FONT_SIZE / 4;
        guiDrawRect(x, rect.y, FONT_SIZE / 2, FONT_SIZE, knobColor);
    }

    {
        Rect rect = settingsElementRects[SettingsFullscreen];
        int offset = appState.fullscreen ? BUTTON_WIDTH - FONT_SIZE / 2 : 0;
        guiDrawRect(rect.x + offset, rect.y, FONT_SIZE / 2, FONT_SIZE, knobColor);
    }


    Color textColor = {{ 1.0f, 1.0f, 1.0f, 1.0f }};
    int fontWidth = FONT_SIZE / 2;

    guiBeginText();
    guiDrawText("     volume:", xOff, yOff + FONT_SIZE * 0, fontWidth, FONT_SIZE, 0, textColor);
    guiDrawText("        fov:", xOff, yOff + FONT_SIZE * 1, fontWidth, FONT_SIZE, 0, textColor);
    guiDrawText("sensitivity:", xOff, yOff + FONT_SIZE * 2, fontWidth, FONT_SIZE, 0, textColor);
    guiDrawText(" fullscreen:", xOff, yOff + FONT_SIZE * 3, fontWidth, FONT_SIZE, 0, textColor);

    if (settingsSelectedElement == SettingsBack) {
        guiDrawText("back", xOff, yOff + FONT_SIZE * 5, FONT_SIZE, FONT_SIZE * 2, 0, textColor);
    } else {
        Color buttonColor = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
        guiDrawText("back", xOff, yOff + FONT_SIZE * 5, FONT_SIZE, FONT_SIZE * 2, 0, buttonColor);
    }

    char infoBuffer[64];
    int infoOffset = xOff + LABEL_WIDTH + SLIDER_WIDTH + fontWidth;

    snprintf(infoBuffer, 64, "%d", (int)(appState.volume * 100));
    guiDrawText(infoBuffer, infoOffset, yOff + FONT_SIZE * 0, fontWidth, FONT_SIZE, 0, textColor);
    snprintf(infoBuffer, 64, "%d", (int)camState.fov);
    guiDrawText(infoBuffer, infoOffset, yOff + FONT_SIZE * 1, fontWidth, FONT_SIZE, 0, textColor);
    snprintf(infoBuffer, 64, "%d", (int)(sliderOffsets[SettingsSensitivity] * 100));
    guiDrawText(infoBuffer, infoOffset, yOff + FONT_SIZE * 2, fontWidth, FONT_SIZE, 0, textColor);
    snprintf(infoBuffer, 64, "%s", appState.fullscreen ? "on" : "off");
    guiDrawText(infoBuffer, infoOffset, yOff + FONT_SIZE * 3, fontWidth, FONT_SIZE, 0, textColor);
}


void settingsSave(void)
{
    FILE *file = fopen(SETTINGS_FILE, "w");
    file_check(file, SETTINGS_FILE);

    safe_write(&appState.volume,       sizeof(float), 1, file);
    safe_write(&camState.fov,          sizeof(float), 1, file);
    safe_write(&gameState.sensitivity, sizeof(float), 1, file);
    safe_write(&appState.fullscreen,   sizeof(bool),  1, file);

    fclose(file);
}


void settingsLoad(void)
{
    FILE *file = fopen(SETTINGS_FILE, "r");
    file_check(file, SETTINGS_FILE);

    safe_read(&appState.volume,       sizeof(float), 1, file);
    safe_read(&camState.fov,          sizeof(float), 1, file);
    safe_read(&gameState.sensitivity, sizeof(float), 1, file);
    safe_read(&appState.fullscreen,   sizeof(bool),  1, file);

    eof_check(file);

    fclose(file);

    bagE_setFullscreen(appState.fullscreen);
}

