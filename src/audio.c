#include "audio.h"

#include "utils.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define NO_NEXT -1

static int availableHead;

static volatile Sound sounds[MAX_SOUND_COUNT];

static int availableTail;


bool playSound(Sound sound)
{
    int next = sounds[availableHead].next;
    if (next == NO_NEXT)
        return false;

    sounds[availableHead] = sound;

    availableHead = next;

    return true;
}


void audioCallback(int16_t *buffer, unsigned size)
{
    size *= 2;

    for (size_t i = 0; i < size; ++i)
        buffer[i] = 0;

    int stoppedCount = 0;

    for (int soundID = 0; soundID < MAX_SOUND_COUNT; ++soundID) {
        Sound sound = sounds[soundID];

        if (sound.stopped) {
            ++stoppedCount;
            continue;
        }

        size_t written = 0;

        while (written < size) {
            size_t left    = sound.end - sound.pos;
            size_t toWrite = left < size ? left : size;

            for (size_t i = 0; i < toWrite; i += 2) {
                buffer[i + 0] += (int16_t)(sound.data[sound.pos + i + 0] * sound.volL);
                buffer[i + 1] += (int16_t)(sound.data[sound.pos + i + 1] * sound.volR);
            }

            sound.pos += toWrite;
            written   += toWrite;

            if (left < size) {
                sound.pos = sound.start;

                if (sound.times == TIMES_INF)
                    continue;

                --sound.times;

                if (sound.times == 0)
                    written = size;
            }
        }

        if (sound.times == 0) {
            if (sound.handle)
                *(sound.handle) = NULL;

            sounds[soundID].stopped = true;
            sounds[soundID].next = NO_NEXT;
            sounds[availableTail].next = soundID;
            availableTail = soundID;
        } else {
            sounds[soundID].pos   = sound.pos;
            sounds[soundID].times = sound.times;
        }
    }
}


void emptySounds(void)
{
    availableHead = 0;
    availableTail = MAX_SOUND_COUNT - 1;

    for (int i = 0; i < MAX_SOUND_COUNT; ++i) {
        sounds[i].next = i + 1;
        sounds[i].stopped = true;
    }

    sounds[MAX_SOUND_COUNT - 1].next = NO_NEXT;
}


void initAudio(void)
{
    AudioInfo info = {
        .writeCallback = audioCallback,
    };

    emptySounds();

    initAudioEngine(info);
}


void exitAudio(void)
{
    exitAudioEngine();
}


int16_t *loadWAV(const char *path, uint64_t *length)
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
    if (strncmp(buffer, "RIFF", 4)) {
        fprintf(stderr, "Can't parse file \"%s\". Wrong format.\n", path);
        fprintf(stderr, "here\n");
        exit(666);
    }

    fseek(file, 4, SEEK_CUR);
    fread(buffer, 1, 4, file);
    if (strncmp(buffer, "WAVE", 4)) {
        fprintf(stderr, "Can't parse file \"%s\". Wrong format.\n", path);
        fprintf(stderr, "there\n");
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
