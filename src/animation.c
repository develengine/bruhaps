#include "animation.h"


bool updateAnimation(Animation *animation, float dt)
{
    bool looped = false;

    animation->time += dt;

    // FIXME: use fmod
    float loopTime = animation->end - animation->start;
    while (animation->time > loopTime) {
        animation->time -= loopTime;
        looped = true;
    }

    return looped;
}


void computeArmatureMatrices(
        Matrix base,
        Matrix *output,
        const JointTransform *transforms,
        const Armature *armature,
        unsigned index
) {
    JointTransform transform = transforms[index];

    Matrix local = matrixTranslation(
            transform.position[0],
            transform.position[1],
            transform.position[2]
    );
    Matrix mat = quaternionToMatrix(transform.rotation);

    local = matrixMultiply(&local, &mat);

    base = matrixMultiply(&base, &local);

    const unsigned *children = armature->hierarchy + armature->childOffsets[index];
    for (unsigned i = 0; i < armature->childCounts[index]; ++i)
        computeArmatureMatrices(base, output, transforms, armature, children[i]);

    base = matrixMultiply(&base, armature->ibms + index);

    output[index] = base;
}


static float getBoneTransforms(
        const Armature *armature,
        JointTransform *first,
        JointTransform *second,
        int index,
        float time
) {
    unsigned frameCount  = armature->frameCounts[index];
    if (frameCount == 0)
        assert(NOT_IMPLEMENTED);

    const float *timeStamps = armature->timeStamps;

    unsigned offset = armature->frameOffsets[index];

    unsigned firstOff  = offset;
    unsigned secondOff = offset;

    for (unsigned i = 1; i < frameCount; ++i) {
        firstOff = secondOff;
        ++secondOff;

        if (timeStamps[offset + i] > time)
            break;
    }

    *first  = armature->transforms[firstOff];
    *second = armature->transforms[secondOff];

    return (time - timeStamps[firstOff]) / (timeStamps[secondOff] - timeStamps[firstOff]);
}


// TODO: cache per bone positions
void computePoseTransforms(const Armature *armature, JointTransform *transforms, float time)
{
    for (int i = 0; i < armature->boneCount; ++i) {
        JointTransform first, second;
        float blend = getBoneTransforms(armature, &first, &second, i, time);

        transforms[i].rotation = quaternionNLerp(first.rotation, second.rotation, blend);
        positionLerp(transforms[i].position, first.position, second.position, blend);
    }
}
