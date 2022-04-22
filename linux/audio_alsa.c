#include "audio.h"

#define alloca(x)  __builtin_alloca(x)

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>


static pthread_t thread;
static int running = 1;


static int xrunRecover(snd_pcm_t *handle, int err)
{
    if (err == -EPIPE) {
        err = snd_pcm_prepare(handle);
        if (err < 0)
            fprintf(stderr, "Can't recover from underrun, prepare failed: %s\n",
                    snd_strerror(err));
        return 0;
    } else if (err == -ESTRPIPE) {
        while ((err = snd_pcm_resume(handle)) == -EAGAIN)
            sleep(1);

        if (err < 0) {
            err = snd_pcm_prepare(handle);
            if (err < 0)
                fprintf(stderr, "Can't recover from suspend, prepare failed: %s\n",
                        snd_strerror(err));
        }
        return 0;
    }
    return err;
}


void *startAlsa(void *param)
{
    AudioInfo *info = (AudioInfo*)param;
    AudioWriteCallback writeCallback = info->writeCallback;
    // Open device

    const char *deviceName = "default";
    snd_pcm_t *device;

    int ret = snd_pcm_open(&device, deviceName, SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0)
        fprintf(stderr, "failed to open pcm device! error: %s\n", snd_strerror(ret));


    // Hardware parameters

    snd_pcm_hw_params_t *hwParams;
    snd_pcm_hw_params_alloca(&hwParams);

    ret = snd_pcm_hw_params_any(device, hwParams);
    if (ret < 0)
        fprintf(stderr, "failed to fill hw_params! error: %s\n", snd_strerror(ret));

    ret = snd_pcm_hw_params_test_access(device, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) // NOTE(engine): Maybe handle later
        printf("requested acces not available! error: %s\n", snd_strerror(ret));
    ret = snd_pcm_hw_params_set_access(device, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0)
        fprintf(stderr, "failed to set access! error: %s\n", snd_strerror(ret));
    // TODO: Add support for interupt processing

    ret = snd_pcm_hw_params_test_format(device, hwParams, SND_PCM_FORMAT_S16_LE);
    if (ret < 0) // NOTE(engine): Maybe handle later
        printf("requested format not available! error: %s\n", snd_strerror(ret));
    ret = snd_pcm_hw_params_set_format(device, hwParams, SND_PCM_FORMAT_S16_LE);
    if (ret < 0)
        fprintf(stderr, "failed to set format! error: %s\n", snd_strerror(ret));

    const unsigned int CHANNEL_COUNT = 2;
    unsigned int chMin, chMax;
    snd_pcm_hw_params_get_channels_min(hwParams, &chMin);
    snd_pcm_hw_params_get_channels_max(hwParams, &chMax);
    printf("min channel n: %d, max channel n: %d\n", chMin, chMax);

    ret = snd_pcm_hw_params_test_channels(device, hwParams, CHANNEL_COUNT);
    if (ret < 0) // NOTE(engine): Maybe handle later
        printf("requested channel count not available! error: %s\n", snd_strerror(ret));
    ret = snd_pcm_hw_params_set_channels(device, hwParams, CHANNEL_COUNT);
    if (ret < 0)
        fprintf(stderr, "failed to set channel count! error: %s\n", snd_strerror(ret));

    const unsigned int RESAMPLE = 1;
    ret = snd_pcm_hw_params_set_rate_resample(device, hwParams, RESAMPLE);
    if (ret < 0)
        fprintf(stderr, "failed to set resample! error: %s\n", snd_strerror(ret));

    unsigned int rate = AUDIO_SAMPLES_PER_SECOND;
    int dir = 0;
    ret = snd_pcm_hw_params_set_rate_near(device, hwParams, &rate, &dir);
    if (ret < 0)
        fprintf(stderr, "failed to set rate near! error: %s\n", snd_strerror(ret));
    if (dir != 0) // NOTE(engine): Maybe handle later
        printf("wrong rate direction!");
    if (rate != AUDIO_SAMPLES_PER_SECOND) // NOTE(engine): Maybe handle later
        printf("wrong sample rate!");
    printf("returned sample rate: %d\n", rate);

    const unsigned int BUFFER_SIZE = AUDIO_SAMPLES_PER_SECOND / 10;
    snd_pcm_uframes_t bufferSize = BUFFER_SIZE;
    ret = snd_pcm_hw_params_set_buffer_size_near(device, hwParams, &bufferSize);
    if (ret < 0)
        fprintf(stderr, "failed to set buffer size near! error: %s\n", snd_strerror(ret));
    if (bufferSize != BUFFER_SIZE) // NOTE(engine): Maybe handle later
        printf("wring buffer size");
    printf("returned buffer size: %ld\n", bufferSize);

    const unsigned int PERIOD_SIZE = AUDIO_SAMPLES_PER_SECOND / 40;
    snd_pcm_uframes_t periodSize = PERIOD_SIZE;
    dir = 0;
    ret = snd_pcm_hw_params_set_period_size_near(device, hwParams, &periodSize, &dir);
    if (ret < 0)
        fprintf(stderr, "failed to set period size near! error: %s\n", snd_strerror(ret));
    if (dir != 0) // NOTE(engine): Maybe handle later
        printf("wrong period direction!");
    if (periodSize != PERIOD_SIZE) // NOTE(engine): Maybe handle later
        printf("wrong period size!");
    printf("returned period size: %ld\n", periodSize);

    ret = snd_pcm_hw_params(device, hwParams);
    if (ret < 0) // NOTE(engine): Maybe handle later
        fprintf(stderr, "failed to set hardware parameters! error: %s\n", snd_strerror(ret));


    // Software parameters

    snd_pcm_sw_params_t *swParams;
    snd_pcm_sw_params_alloca(&swParams);

    ret = snd_pcm_sw_params_current(device, swParams);
    if (ret < 0)
        fprintf(stderr, "failed to set current software params! error: %s\n", snd_strerror(ret));

    ret = snd_pcm_sw_params_set_start_threshold(device, swParams, (bufferSize / periodSize) * periodSize);
    if (ret < 0)
        fprintf(stderr, "failed to set start threshold! error: %s\n", snd_strerror(ret));

    ret = snd_pcm_sw_params_set_avail_min(device, swParams, periodSize);
    if (ret < 0)
        fprintf(stderr, "failed to set avail min! error: %s\n", snd_strerror(ret));
    // TODO(engine): Add support for interupt processing

    ret = snd_pcm_sw_params(device, swParams);
    if (ret < 0)
        fprintf(stderr, "failed to set software parameters! error: %s\n", snd_strerror(ret));


    // Write loop

    int16_t *writeBuffer = (int16_t*)calloc(periodSize * CHANNEL_COUNT, sizeof(int16_t));
    if (!writeBuffer) {
        fprintf(stderr, "failed to allocate memory for write buffer!");
        exit(EXIT_FAILURE);
    }

    while (running) {
        writeCallback(writeBuffer, periodSize);

        int16_t *framePointer = writeBuffer;
        uint32_t framesLeft = periodSize;

        while (framesLeft > 0) {
            ret = snd_pcm_writei(device, framePointer, framesLeft);

            if (ret == -EAGAIN)
                continue;

            if (ret < 0) {
                if (xrunRecover(device, ret) < 0) {
                    fprintf(stderr, "failed write! error: %s\n", snd_strerror(ret));
                    exit(EXIT_FAILURE);
                }
                break;
            }

            framePointer += ret * CHANNEL_COUNT;
            framesLeft -= ret;
        }
    }

    // snd_pcm_drain(device);
    snd_pcm_close(device);
    free(writeBuffer);

    pthread_exit(0);
}


void initAudio(AudioInfo *info)
{
    int ret = pthread_create(&thread, NULL, startAlsa, info);

    if (ret) {
        fprintf(stderr, "failed to create a separate audio thread!\n");
        exit(222);
    }
}


void exitAudio(void)
{
    running = 0;
    pthread_join(thread, NULL);
}

