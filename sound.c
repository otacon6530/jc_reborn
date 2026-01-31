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

    debugMsg("soundInit: called");
    if (soundDisabled) {
        debugMsg("soundInit: soundDisabled is set, skipping init");
        return;
    }


    debugMsg("soundInit: calling platformInitAudio");
    if (platformInitAudio() < 0) {
        debugMsg("Platform init audio error: %s", platformGetError());
        soundDisabled = 1;
        return;
    }

    PlatformAudioSpec audioSpec;

    // Dynamically find and load all RIFF/WAVE sounds from SCRANTIC.SCR
    char scrantic_path[256];
    snprintf(scrantic_path, sizeof(scrantic_path), "data/SCRANTIC.SCR");

    debugMsg("soundInit: opening SCRANTIC.SCR at %s", scrantic_path);
    FILE *f = fopen(scrantic_path, "rb");
    if (!f) {
        debugMsg("Could not open SCRANTIC.SCR for reading");
        soundDisabled = 1;
        return;
    }

    // Read entire file into memory
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *filedata = (uint8_t*)malloc(filesize);
    if (!filedata) {
        fclose(f);
        debugMsg("Failed to allocate memory for SCRANTIC.SCR");
        soundDisabled = 1;
        return;
    }
    fread(filedata, 1, filesize, f);
    fclose(f);

    // Find all RIFF/WAVE headers
    int found = 1; // Start at 1 so sounds[1] is first
    for (long i = 0; i < filesize - 12 && found < NUM_OF_SOUNDS; i++) {
        if (memcmp(filedata + i, "RIFF", 4) == 0 && memcmp(filedata + i + 8, "WAVE", 4) == 0) {
            // Get chunk size (little endian)
            uint32_t chunk_size = filedata[i+4] | (filedata[i+5]<<8) | (filedata[i+6]<<16) | (filedata[i+7]<<24);
            uint32_t wav_size = chunk_size + 8; // 'RIFF' + size(4) + chunk_size
            if (i + wav_size > filesize) wav_size = filesize - i;
            uint8_t *buffer = (uint8_t*)malloc(wav_size);
            if (buffer) {
                memcpy(buffer, filedata + i, wav_size);
                sounds[found].data = buffer;
                sounds[found].length = wav_size;
                debugMsg("soundInit: loaded sound %d at offset 0x%lX, size %u", found, i, wav_size);
                found++;
            }
            i += wav_size - 1; // Skip to end of this chunk
        }
    }
    free(filedata);

    // Zero out any unused slots (including sounds[0])
    for (int i = 0; i < NUM_OF_SOUNDS; i++) {
        if (sounds[i].data == NULL) {
            sounds[i].data = NULL;
            sounds[i].length = 0;
        }
    }


    // Set all required fields for PlatformAudioSpec
    audioSpec.freq = 22050;      // Standard sample rate for classic WAVs
    audioSpec.format = 1;        // 1 = AUDIO_U8 (unsigned 8-bit PCM), adjust if needed
    audioSpec.channels = 1;      // Mono, adjust if your WAVs are stereo
    audioSpec.samples = 1024;    // Buffer size
    audioSpec.callback = soundCallback;
    audioSpec.userdata = NULL;



    debugMsg("soundInit: audioSpec.freq = %d", audioSpec.freq);
    debugMsg("soundInit: audioSpec.format = %u", (unsigned)audioSpec.format);
    debugMsg("soundInit: audioSpec.channels = %u", (unsigned)audioSpec.channels);
    debugMsg("soundInit: audioSpec.samples = %u", (unsigned)audioSpec.samples);
    debugMsg("soundInit: calling platformOpenAudio");
    int open_result = platformOpenAudio(&audioSpec);
    if (open_result < 0) {
        debugMsg("platformOpenAudio() error: %s", platformGetError());
        debugMsg("soundInit: platformOpenAudio returned %d", open_result);
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
    debugMsg("soundPlay: called with nb=%d", nb);
    if (soundDisabled) {
        debugMsg("soundPlay: soundDisabled is set, skipping playback");
        return;
    }

    if (nb < 0 || NUM_OF_SOUNDS <= nb) {
        debugMsg("soundPlay: wrong sound sample index #%d", nb);
        return;
    }

    if (sounds[nb].length) {
        debugMsg("soundPlay: playing sound #%d, length=%u", nb, sounds[nb].length);
        platformLockAudio();
        currentSound     = &sounds[nb];
        currentPtr       = currentSound->data;
        currentRemaining = currentSound->length;
        platformUnlockAudio();
    }
    else {
        debugMsg("soundPlay: Non-existent sound sample #%d", nb);
    }
}

