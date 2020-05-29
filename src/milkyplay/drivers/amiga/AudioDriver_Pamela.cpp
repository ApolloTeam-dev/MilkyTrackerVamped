#include "AudioDriver_Pamela.h"
#include "MasterMixer.h"

#include <exec/exec.h>
#include <proto/exec.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <hardware/cia.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CUSTOM_DMACON2          (CUSTOM_DMACON  + 0x200)
#define CUSTOM_INTENA2          (CUSTOM_INTENA  + 0x200)

#define AUDIO_REGBASE(CH)       (CUSTOM_REGBASE + 0x400 + ((CH & 0xf) << 4))
#define AUDIO_REG(CH, IDX)      (AUDIO_REGBASE(CH) + (IDX))

#define AUDIO_LOCHI(CH)         AUDIO_REG(CH, 0x00)
#define AUDIO_LOCLO(CH)         AUDIO_REG(CH, 0x02)
#define AUDIO_LENHI(CH)         AUDIO_REG(CH, 0x04)
#define AUDIO_LENLO(CH)         AUDIO_REG(CH, 0x06)
#define AUDIO_VOLUME(CH)        AUDIO_REG(CH, 0x08)
#define AUDIO_MODE(CH)          AUDIO_REG(CH, 0x0a)
#define AUDIO_PERIOD(CH)        AUDIO_REG(CH, 0x0c)

#define AUDIO_MODEF_16          (1<<0)
#define AUDIO_MODEF_ONESHOT     (1<<1)

#define DMA2F_AUDIO             0xfff

#define MAX_CHANNELS            16

#define SAMPLE_SIZE             MP_NUMBYTES


AudioDriver_Pamela::AudioDriver_Pamela()
: AudioDriver_Amiga()
{
}

AudioDriver_Pamela::~AudioDriver_Pamela()
{
}

mp_uint32
AudioDriver_Pamela::getChannels() const
{
    return MAX_CHANNELS;
}

mp_uint32
AudioDriver_Pamela::getSampleSize() const
{
    return SAMPLE_SIZE;
}

mp_sint32
AudioDriver_Pamela::initDevice(mp_sint32 bufferSizeInWords, mp_uint32 mixFrequency, MasterMixer* mixer)
{
#if DEBUG_DRIVER
    printf("%s\n", __PRETTY_FUNCTION__);
    printf("Wanted mix frequency: %ld\n", mixFrequency);
    printf("Driver running in Pamela mode\n");
#endif

    mixFrequency = mixFrequency > 44100 ? 44100 : mixFrequency;

    return AudioDriver_Amiga::initDevice(bufferSizeInWords, mixFrequency, mixer);
}

void
AudioDriver_Pamela::setGlobalVolume(mp_ubyte volume)
{
    int i;

    for(i = 0; i < MAX_CHANNELS; i++) {
        *((volatile mp_uword *) AUDIO_VOLUME(i)) = volume;
    }
}

void
AudioDriver_Pamela::disableDMA()
{
    switch(outputMode) {
    case Mix:
        *((volatile mp_uword *) CUSTOM_DMACON) = DMAF_AUD0 | DMAF_AUD1;

        break;
    default:
    case DirectOut:
        *((volatile mp_uword *) CUSTOM_DMACON) = DMAF_AUDIO;
        *((volatile mp_uword *) CUSTOM_DMACON2) = DMA2F_AUDIO;

        break;
    }
}

void
AudioDriver_Pamela::enableDMA()
{
    switch(outputMode) {
    case Mix:
        *((volatile mp_uword *) CUSTOM_DMACON) = DMAF_SETCLR | DMAF_AUD0 | DMAF_AUD1;

        break;
    default:
    case DirectOut:
        *((volatile mp_uword *) CUSTOM_DMACON) = DMAF_SETCLR | DMAF_AUDIO;
        *((volatile mp_uword *) CUSTOM_DMACON2) = DMAF_SETCLR | DMA2F_AUDIO;

        break;
    }
}

void
AudioDriver_Pamela::initHardware()
{
    int i;

    // Initialize audio hardware with default values
    mp_uword period = PAULA_CLK_PAL / this->mixFrequency;

    switch(outputMode) {
    case Mix:
        *((volatile mp_uint32 *) AUDIO_LOCHI(0)) = (mp_uint32) samplesLeft;
        *((volatile mp_uword *) AUDIO_LENLO(0)) = chunkSize >> 1;
        *((volatile mp_uword *) AUDIO_PERIOD(0)) = period;

        *((volatile mp_uint32 *) AUDIO_LOCHI(1)) = (mp_uint32) samplesRight;
        *((volatile mp_uword *) AUDIO_LENLO(1)) = chunkSize >> 1;
        *((volatile mp_uword *) AUDIO_PERIOD(1)) = period;

        break;
    default:
    case DirectOut:
        for(i = 0; i < MAX_CHANNELS; i++) {
            *((volatile mp_uint32 *) AUDIO_LOCHI(i)) = (mp_uint32) chanRing[i];
            *((volatile mp_uword *) AUDIO_LENLO(i)) = chunkSize >> 1;
            *((volatile mp_uword *) AUDIO_PERIOD(i)) = period;
            *((volatile mp_uword *) AUDIO_MODE(i)) = AUDIO_MODEF_16;
        }

        break;
    }
}

void
AudioDriver_Pamela::playAudioImpl()
{
    int i;

    switch(outputMode) {
    case Mix:
        *((volatile mp_uint32 *) AUDIO_LOCHI(0)) = (mp_uint32) (samplesLeft + idxRead);
        *((volatile mp_uword *) AUDIO_LENLO(0)) = chunkSize >> 1;

        *((volatile mp_uint32 *) AUDIO_LOCHI(1)) = (mp_uint32) (samplesRight + idxRead);
        *((volatile mp_uword *) AUDIO_LENLO(1)) = chunkSize >> 1;

        break;
    default:
    case DirectOut:
        for(i = 0; i < MAX_CHANNELS; i++) {
            *((volatile mp_uint32 *) AUDIO_LOCHI(i)) = (mp_uint32) (chanRing[i] + idxRead);
            *((volatile mp_uword *) AUDIO_LENLO(i)) = chunkSize >> 1;
        }

        break;
    }
}

void
AudioDriver_Pamela::bufferAudioImpl()
{
    int i;

    switch(outputMode) {
    case Mix:
        {
            mp_sword * f = samplesFetched;
            mp_sword
                * l = samplesLeft + idxWrite,
                * r = samplesRight + idxWrite;

            if (isMixerActive())
                mixer->mixerHandler(f);
            else
                memset(f, 0, fetchSize << 2);

            for(i = 0; i < fetchSize; i++) {
                *(l++) = *(f++);
                *(r++) = *(f++);
            }
        }
        break;
    default:
    case DirectOut:
        {
            if (isMixerActive()) {
                for(i = 0; i < MAX_CHANNELS; i++) {
                    chanRingPtrs[i] = chanRing[i] + idxWrite;
                }
                mixer->mixerHandler(NULL, MAX_CHANNELS, chanRingPtrs);
            } else {
                for(i = 0; i < MAX_CHANNELS; i++) {
                    memset(chanRing[i] + idxWrite, 0, fetchSize * SAMPLE_SIZE);
                }
            }
        }
        break;
    }
}


const char*
AudioDriver_Pamela::getDriverID()
{
    return "Apollo SAGA Pamela Audio Driver";
}

mp_sint32
AudioDriver_Pamela::getPreferredBufferSize() const
{
    return 2048;
}
