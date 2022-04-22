#ifndef AUDIO_H
#define AUDIO_H

// #include <stdint.h>
typedef short int16_t;

#define AUDIO_SAMPLES_PER_SECOND 48000


typedef void (*AudioWriteCallback)(int16_t *buffer, unsigned size);

typedef struct
{
    AudioWriteCallback writeCallback;
} AudioInfo;

void initAudio(AudioInfo *info);
void exitAudio(void);

#endif
