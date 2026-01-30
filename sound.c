/*
 *  This file is part of 'Johnny Reborn'
 *
 *  An open-source engine for the classic
 *  'Johnny Castaway' screensaver by Sierra.
 *
 *  Copyright (C) 2019 Jeremie GUILLAUME
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "platform.h"
#include <string.h>

#include "mytypes.h"
#include "utils.h"
#include "sound.h"



#define NUM_OF_SOUNDS  25
#define NUM_SCRANTIC_SOUNDS 24

static const int scrantic_offsets[NUM_SCRANTIC_SOUNDS] = {
    0x1DC00, 0x20800, 0x20E00,
    0x22C00, 0x24000, 0x24C00,
    0x28A00, 0x2C600, 0x2D000,
    0x2DE00, 0x34400, 0x32E00,
    0x39C00, 0x43400, 0x37200,
    0x37E00, 0x45A00, 0x3AE00,
    0x3E600, 0x3F400, 0x41200,
    0x42600, 0x42C00, 0x43400
};




struct TSound {
    uint32  length;
    uint8   *data;
};


int soundDisabled = 0;


static struct TSound sounds[NUM_OF_SOUNDS];
static struct TSound *currentSound;

static uint8  *currentPtr;
static uint32 currentRemaining;


static void soundCallback(void *userdata, uint8 *stream, int rqdLen)
{ 
    if ((int)currentRemaining > rqdLen) {
        memcpy(stream, currentPtr, rqdLen);
        currentPtr += rqdLen;
        currentRemaining -= (uint32)rqdLen;
    }
    else {
        memcpy(stream, currentPtr, currentRemaining);
        memset(stream + currentRemaining, 127, rqdLen - currentRemaining);  // 127 == silence
        currentRemaining = 0;
        //SDL_PauseAudio(1);
    }
}


void soundInit(void)
{

    printf("soundInit: called");
    if (soundDisabled) {
        printf("soundInit: soundDisabled is set, skipping init");
        return;
    }


    printf("soundInit: calling platformInitAudio");
    if (platformInitAudio() < 0) {
        printf("Platform init audio error: %s", platformGetError());
        soundDisabled = 1;
        return;
    }

    PlatformAudioSpec audioSpec;


    // Load all sounds from SCRANTIC.SCR
    char scrantic_path[256];
    snprintf(scrantic_path, sizeof(scrantic_path), "data/SCRANTIC.SCR");

    printf("soundInit: opening SCRANTIC.SCR at %s", scrantic_path);
    FILE *f = fopen(scrantic_path, "rb");
    if (!f) {
        printf("Could not open SCRANTIC.SCR for reading");
        soundDisabled = 1;
        return;
    }

    for (int i = 0; i < NUM_SCRANTIC_SOUNDS; i++) {
        debugMsg("soundInit: loading sound %d from offset 0x%X", i+1, scrantic_offsets[i]);
        if (scrantic_offsets[i] == -1) {
            sounds[i+1].data = NULL;
            sounds[i+1].length = 0;
            printf("soundInit: sound %d offset is -1, skipping", i+1);
            continue;
        }
        fseek(f, scrantic_offsets[i], SEEK_SET);
        uint16 size = readUint16(f);
        int wav_size = size + 8;
            printf("soundInit: sound %d size = %d (wav_size = %d)", i+1, size, wav_size);
        fseek(f, scrantic_offsets[i], SEEK_SET);
        uint8 *buffer = (uint8*)malloc(wav_size);
        if (fread(buffer, 1, wav_size, f) != wav_size) {
            free(buffer);
            sounds[i+1].data = NULL;
            sounds[i+1].length = 0;
            printf("Failed to read sound %d from SCRANTIC.SCR", i+1);
            continue;
        }
        sounds[i+1].data = buffer;
        sounds[i+1].length = wav_size;
        printf("soundInit: loaded sound %d successfully", i+1);
    }
    fclose(f);

    audioSpec.callback = soundCallback;
    audioSpec.userdata = NULL;
    audioSpec.samples  = 1024;


    printf("soundInit: calling platformOpenAudio");
    if (platformOpenAudio(&audioSpec) < 0) {
        printf("platformOpenAudio() error: %s", platformGetError());
        soundDisabled = 1;
        return;
    }

    currentRemaining = 0;
    platformPauseAudio(0);
}


void soundEnd()
{
    if (soundDisabled)
        return;

    platformCloseAudio();

    for (int i=0; i < NUM_OF_SOUNDS; i++)
        if (sounds[i].data != NULL)
            platformFreeWAV(sounds[i].data);
}


void soundPlay(int nb)
{
    printf("soundPlay: called with nb=%d", nb);
    if (soundDisabled) {
        printf("soundPlay: soundDisabled is set, skipping playback");
        return;
    }

    if (nb < 0 || NUM_OF_SOUNDS <= nb) {
        printf("soundPlay: wrong sound sample index #%d", nb);
        return;
    }

    if (sounds[nb].length) {
        printf("soundPlay: playing sound #%d, length=%u", nb, sounds[nb].length);
        platformLockAudio();
        currentSound     = &sounds[nb];
        currentPtr       = currentSound->data;
        currentRemaining = currentSound->length;
        platformUnlockAudio();
    }
    else {
        printf("soundPlay: Non-existent sound sample #%d", nb);
    }
}

