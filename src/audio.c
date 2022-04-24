#include "audio.h"

#include "utils.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define NO_NEXT -1

static int availableHead;
static int availableTail;
static int playingHead;
static int playingTail;

static volatile Sound sounds[MAX_SOUND_COUNT];


bool playSound(SoundInfo info, volatile SoundInfo **handle)
{
    
}


void audioCallback(int16_t *buffer, unsigned size)
{
    static float x = 0.0f;

    for (unsigned i = 0; i < size * 2; i += 2) {
        int16_t sample = sinf(x) * INT16_MAX * 0.25f;
        buffer[i + 0] = sample;
        buffer[i + 1] = sample;
        x += 2000.0f / AUDIO_SAMPLES_PER_SECOND;
    }

    if (x > AUDIO_SAMPLES_PER_SECOND)
        x -= (float)AUDIO_SAMPLES_PER_SECOND;
}


void emptySounds(void)
{
    availableHead = 0;
    availableTail = MAX_SOUND_COUNT - 1;
    playingHead = NO_NEXT;
    playingTail = NO_NEXT;

    for (int i = 0; i < MAX_SOUND_COUNT; ++i)
        sounds[i].next = i + 1;

    sounds[MAX_SOUND_COUNT - 1].next = NO_NEXT;
}


void initAudio(void)
{
    AudioInfo info = {
        .writeCallback = audioCallback,
    };

    initAudioEngine(info);
}


void exitAudio(void)
{
    exitAudioEngine();
}


int16_t *loadWAV(const char *path, unsigned *length)
{
    char buffer[4];
    uint16_t typeOfFormat, channelCount, frameSize, bitsPerSample;
    uint32_t sampleRate, dataSize;
    FILE *file;

    if (!(file = fopen(path, "rb"))) {
        fprintf(stderr, "Can't find file \"%s\"\n", path);
        exit(666);
    }

    // FIXME: add a file length check

    fread(buffer, 1, 4, file);
    if (strcmp(buffer, "RIFF")) {
        fprintf(stderr, "Can't parse file \"%s\". Wrong format.\n", path);
        exit(666);
    }

    fseek(file, 4, SEEK_CUR);
    fread(buffer, 1, 4, file);
    if (strcmp(buffer, "WAVE")) {
        fprintf(stderr, "Can't parse file \"%s\". Wrong format.\n", path);
        exit(666);
    }

    fseek(file, 4, SEEK_CUR);

    int32_t lof;
    fread(&lof, 1, 4, file);
    printf("lof: %d\n", lof);

    fread(&typeOfFormat, 1, 2, file);
    fread(&channelCount, 1, 2, file);
    fread(&sampleRate, 1, 4, file);
    fseek(file, 4, SEEK_CUR);
    fread(&frameSize, 1, 2, file);
    fread(&bitsPerSample, 1, 2, file);

    // NOTE: not sure why it can have some extra garbage in it
    // FIXME: just horrible, but add a file size check
    char state = ' ';
    while (state != 'f') {
        char c;
        safe_read(&c, 1, 1, file);
        switch (c) {
            case 'd':
                state = 'd';
                continue;
            case 'a':
                if (state == 't') {
                    state = 'f';
                    continue;
                }
                if (state == 'd') {
                    state = 'a';
                    continue;
                }
                break;
            case 't':
                if (state == 'a') {
                    state = 't';
                    continue;
                }
                break;
            default:
                break;
        }

        state = ' ';
    }

    fread(&dataSize, 1, 4, file);
    
#if 1
    printf("WAV file: %s\n"
           "    PCM: %d\n"
           "    Channels: %d\n"
           "    Sample Rate: %d\n"
           "    Frame Size: %d\n"
           "    Bits per Sample: %d\n"
           "    Data Size: %d\n",
           path,
           typeOfFormat,
           channelCount,
           sampleRate,
           frameSize,
           bitsPerSample,
           dataSize);
#endif

    int16_t *data = malloc(dataSize);
    if (!data) {
        fprintf(stderr, "failed to allocate memory for audio data \"%s\"\n", path);
        exit(666);
    }

    fread(data, 1, dataSize, file);
    *length = dataSize / (bitsPerSample / 8);

    fclose(file);

    return data;
}
