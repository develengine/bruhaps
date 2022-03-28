#ifndef ANIMATION_H
#define ANIMATION_H

#include "linalg.h"
#include "res.h"
#include "utils.h"


typedef struct
{
    float start, end;
    float time;
    int iteration;
} Animation;


bool updateAnimation(Animation *animation, float dt);

void computePoseTransforms(const Armature *armature, JointTransform *transforms, float time);

void computeArmatureMatrices(
        Matrix base,
        Matrix *output,
        const JointTransform *transforms,
        const Armature *armature,
        unsigned index
);

#endif
