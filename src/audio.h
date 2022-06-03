#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#define AUDIO_SAMPLES_PER_SECOND 48000

typedef void (*AudioWriteCallback)(int16_t *buffer, unsigned size);

typedef struct
{
    AudioWriteCallback writeCallback;
} AudioInfo;

#define MAX_SOUND_COUNT 32
#define TIMES_INF       0xffffffff

typedef struct
{
    int16_t *data;
    unsigned times;
    size_t start, end, pos;
    float volL, volR;

    /* null initialized */
    union {
        int next;
        void *volatile *handle;
    };
    unsigned stopped;   /* true/false */
} Sound;


void initAudioEngine(AudioInfo info);
void exitAudioEngine(void);

void initAudio(void);
void exitAudio(void);

bool playSound(Sound info);

int16_t *loadWAV(const char *path, uint64_t *length);

#endif
