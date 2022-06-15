#include "splash.h"

#include "levels.h"
#include "gui.h"
#include "audio.h"

#include <assert.h>


#define TEXT_MUL 3
#define NO_BUTTON -1


typedef enum
{
    SplashNewGame,
    SplashContinue,
    SplashSettings,
    SplashQuit,

    SplashButtonCount
} SplashButtonID;


static const char *buttonNames[] = {
    [SplashNewGame]  = " NEW GAME ",
    [SplashContinue] = " CONTINUE ",
    [SplashSettings] = " SETTINGS ",
    [SplashQuit]     = "   QUIT   ",
};

static_assert(length(buttonNames) == SplashButtonCount,
              "unfilled button name");


static void onNewGame(void)
{
    printf("New Game\n");
    gameState.inSplash = false;
    playerState.gaming = !gameState.isEditor;
    inputState.playerInput = true;
    bagE_setHiddenCursor(true);

    // FIXME:
    levelLoad(LevelBruh);
}

static void onContinue(void)
{
    printf("Continue\n");
}

static void onSettings(void)
{
    printf("Settings\n");
}

static void onQuit(void)
{
    appState.running = false;
}

static GUIButtonCallback buttonCallbacks[] = {
    [SplashNewGame]  = onNewGame,
    [SplashContinue] = onContinue,
    [SplashSettings] = onSettings,
    [SplashQuit]     = onQuit,
};

static_assert(length(buttonCallbacks) == SplashButtonCount,
              "unfilled button callback");


static Rect buttonRects[SplashButtonCount];

static ModelObject    brugModel;
static unsigned       brugTexture;
static ModelTransform brugTransform;

static int selectedButton = NO_BUTTON;

static unsigned envMap;

static float timePassed = 0.0f;


void initSplash(void)
{
    brugTransform = (ModelTransform) {
        .x = -1.5f, .y = -1.0f, .z = -2.0f,
        .scale = 1.0f,
        .rx = 0.0f, .ry = (float)(-M_PI / 2.0 + 0.5), .rz = 0.0f
    };

    brugTexture = createTexture("res/brug.png");
    brugModel   = loadModelObject("res/brug_pose.model");

    envMap = createCubeTexture(
            "res/Maskonaive2/posx.png",
            "res/Maskonaive2/negx.png",

            "res/Maskonaive2/posy.png",
            "res/Maskonaive2/negy.png",

            "res/Maskonaive2/posz.png",
            "res/Maskonaive2/negz.png"
    );
}


void exitSplash(void)
{
    // TODO: free things
}


void updateSplash(float dt)
{
    timePassed += dt;

    int x = (appState.windowWidth / 5) * 3;
    int height = 16 * TEXT_MUL;
    int padding = 4 * TEXT_MUL;
    int y = (appState.windowHeight - (height + padding) * SplashButtonCount) / 2;

    for (int i = 0; i < SplashButtonCount; ++i) {
        buttonRects[i] = (Rect) { x, y, height / 2, height };
        y += height + padding;
    }
}


void renderSplash(void)
{
    /* skybox */
    glDisable(GL_DEPTH_TEST);

    glUseProgram(level.skyboxProgram);
    glBindVertexArray(level.boxModel.vao);
    glBindTextureUnit(0, level.skyboxCubemap);

    glDrawElements(GL_TRIANGLES, BOX_INDEX_COUNT, GL_UNSIGNED_INT, 0);

    glEnable(GL_DEPTH_TEST);


    /* brug */
    glUseProgram(level.textureProgram);

    glBindVertexArray(brugModel.vao);
    glBindTextureUnit(0, brugTexture);

    Matrix modelMat = modelTransformToMatrix(brugTransform);

    glProgramUniformMatrix4fv(
            level.textureProgram,
            0,
            1,
            GL_FALSE,
            modelMat.data
    );

    glDrawElements(GL_TRIANGLES, brugModel.indexCount, GL_UNSIGNED_INT, 0);

    /* glock base */
    Matrix modelGlock = matrixScale(0.15f, 0.15f, 0.15f);

    Matrix mul = matrixRotationX((float)(M_PI / 16));
    modelGlock = matrixMultiply(&mul, &modelGlock);

    mul = matrixRotationY((float)-(M_PI / 2));
    modelGlock = matrixMultiply(&mul, &modelGlock);

    mul = matrixTranslation(-1.1f, 0.85f, -2.6f);
    modelGlock = matrixMultiply(&mul, &modelGlock);

    glBindVertexArray(level.glockBase.vao);
    glBindTextureUnit(0, level.gunTexture);

    glProgramUniformMatrix4fv(
            level.textureProgram,
            0,
            1,
            GL_FALSE,
            modelGlock.data
    );

    glDrawElements(GL_TRIANGLES, level.glockBase.indexCount, GL_UNSIGNED_INT, 0);

    {
        /* gatling base */
        Matrix modelGatling = matrixScale(0.5f, 0.5f, 0.5f);

        mul = matrixRotationX((float)(M_PI / 16));
        modelGatling = matrixMultiply(&mul, &modelGatling);

        mul = matrixRotationY((float)(M_PI / 3.5));
        modelGatling = matrixMultiply(&mul, &modelGatling);

        mul = matrixTranslation(-0.9f, 0.0f, -1.0f);
        modelGatling = matrixMultiply(&mul, &modelGatling);

        glBindVertexArray(level.gatlingBase.vao);

        glProgramUniformMatrix4fv(
                level.textureProgram,
                0,
                1,
                GL_FALSE,
                modelGatling.data
        );

        glDrawElements(GL_TRIANGLES, level.gatlingBase.indexCount, GL_UNSIGNED_INT, 0);
    }

    /* gatling */
    Matrix modelGatling = matrixScale(0.5f, 0.5f, 0.5f);

    mul = matrixRotationZ(timePassed * 2.0f);
    modelGatling = matrixMultiply(&mul, &modelGatling);

    mul = matrixRotationX((float)(M_PI / 16));
    modelGatling = matrixMultiply(&mul, &modelGatling);

    mul = matrixRotationY((float)(M_PI / 3.5));
    modelGatling = matrixMultiply(&mul, &modelGatling);

    mul = matrixTranslation(-0.9f, 0.0f, -1.0f);
    modelGatling = matrixMultiply(&mul, &modelGatling);


    glUseProgram(level.metalProgram);
    glBindVertexArray(level.gatling.vao);

    glProgramUniformMatrix4fv(level.metalProgram, 0, 1, GL_FALSE, modelGatling.data);

    glDrawElements(GL_TRIANGLES, level.gatling.indexCount, GL_UNSIGNED_INT, 0);


    glBindVertexArray(level.glock.vao);

    glProgramUniformMatrix4fv(level.metalProgram, 0, 1, GL_FALSE, modelGlock.data);

    glDrawElements(GL_TRIANGLES, level.glock.indexCount, GL_UNSIGNED_INT, 0);
}


void renderSplashOverlay(void)
{
    Color rectColor = {{ 0.2f, 0.3f, 0.4f, 0.75f }};
    Color textColor = {{ 1.0f, 1.0f, 1.0f, 1.0f }};

    Color rectColor2 = {{ 1.0f, 1.0f, 1.0f, 1.0f }};
    Color textColor2 = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

    int maxTextLen = (int)strlen(buttonNames[0]);

    for (int i = 1; i < SplashButtonCount; ++i) {
        int textLen = (int)strlen(buttonNames[i]);
        if (textLen > maxTextLen)
            maxTextLen = textLen;
    }

    int margin = TEXT_MUL * 2;

    guiBeginRect();

    for (int i = 0; i < SplashButtonCount; ++i) {
        Rect rect = buttonRects[i];
        if (i == selectedButton) {
            guiDrawRect(rect.x - margin, rect.y,
                        rect.w * maxTextLen + margin * 2, rect.h,
                        rectColor2);
        } else {
            guiDrawRect(rect.x - margin, rect.y,
                        rect.w * maxTextLen + margin * 2, rect.h,
                        rectColor);
        }
    }


    guiBeginText();

    for (int i = 0; i < SplashButtonCount; ++i) {
        Rect rect = buttonRects[i];
        if (i == selectedButton) {
            guiDrawText(buttonNames[i], rect.x, rect.y, rect.w, rect.h, 0, textColor2);
        } else {
            guiDrawText(buttonNames[i], rect.x, rect.y, rect.w, rect.h, 0, textColor);
        }
    }
}


void splashProcessButton(bagE_MouseButton *mb)
{
    if (mb->button != bagE_ButtonLeft)
        return;

    int x = mb->x;
    int y = mb->y;

    for (int i = 0; i < SplashButtonCount; ++i) {
        Rect rect = buttonRects[i];
        int length = (int)strlen(buttonNames[i]);

        if (x >= rect.x && x <= (rect.x + rect.w * length)
         && y >= rect.y && y <= (rect.y + rect.h)) {
            playSound((Sound) {
                .data = level.vineThud,
                .end   = level.vineThudLength,
                .volL  = 0.25f,
                .volR  = 0.25f,
                .times = 1,
            });
            buttonCallbacks[i]();
            return;
        }
    }
}


void splashProcessMouse(bagE_Mouse *m)
{
    int x = m->x;
    int y = m->y;

    for (int i = 0; i < SplashButtonCount; ++i) {
        Rect rect = buttonRects[i];
        int length = (int)strlen(buttonNames[i]);

        if (x >= rect.x && x <= (rect.x + rect.w * length)
         && y >= rect.y && y <= (rect.y + rect.h)) {
            if (i == selectedButton)
                return;

            playSound((Sound) {
                .data = level.vineThud,
                .end   = level.vineThudLength / 16,
                .volL  = 0.15f,
                .volR  = 0.15f,
                .times = 1,
            });

            selectedButton = i;
            return;
        }
    }

    selectedButton = NO_BUTTON;
}
