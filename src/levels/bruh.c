#include "levels.h"


static const AtlasView atlasViews[] = {
    { 0.25f, 0.75f, 0.25f, 0.25f, 1, 1 },  // cobble
    { 0.0f,  0.75f, 0.25f, 0.25f, 1, 1 },  // grass
};


void levelBruhLoad(void)
{
    level.terrainAtlas = createTexture("res/terrain_atlas.png");

    level.skyboxCubemap = createCubeTexture(
            "Maskonaive2/posx.png",
            "Maskonaive2/negx.png",

            "Maskonaive2/posy.png",
            "Maskonaive2/negy.png",

            "Maskonaive2/posz.png",
            "Maskonaive2/negz.png"
    );
}


void levelBruhUnload(void)
{
    glDeleteTextures(1, &level.terrainAtlas);
    glDeleteTextures(1, &level.skyboxCubemap);
}


void levelBruhInit(void)
{
}


void levelBruhExit(void)
{
}