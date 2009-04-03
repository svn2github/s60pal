//
// Copyright (c) 2009, Wei Mingzhi <whistler@openoffice.org>.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "palcommon.h"
#include "sound.h"
#include "rixplay.h"
#include "global.h"
static BOOL gSndOpened = FALSE;



typedef struct tagSNDPLAYER
{
   FILE                     *mkf;
   SDL_AudioSpec             spec;
   LPBYTE                    buf, pos;
   INT                       audio_len;
} SNDPLAYER;

static SNDPLAYER gSndPlayer;

VOID ChangeVolume(INT iDirectory)
{
//
// Check for NULL pointer.
//
if (iDirectory > 0)
	{
	if (gVolume <= SDL_MIX_MAXVOLUME)
		{
		gVolume += SDL_MIX_MAXVOLUME * 0.1;
		}
	else
		{
		gVolume = SDL_MIX_MAXVOLUME;
		}
	}
else
	{
	if (gVolume > 0)
		{
		gVolume -= SDL_MIX_MAXVOLUME * 0.1;
		}
	else
		{
		gVolume = 0;
		}

	}

}
static SDL_AudioSpec *
SOUND_LoadVOCFromBuffer(
   LPCBYTE                lpVOC,
   SDL_AudioSpec         *lpSpec,
   LPBYTE                *lppBuffer
)
/*++
  Purpose:

    Load a VOC file in a buffer. Currently supports type 01 block only.

  Parameters:

    [IN]  lpVOC - pointer to the buffer of the VOC file.

    [OUT] lpSpec - pointer to the SDL_AudioSpec structure, which contains
                   some basic information about the VOC file.

    [OUT] lppBuffer - the output buffer.

  Return value:

    Pointer to the SDL_AudioSpec structure, NULL if failed.

--*/
{
   INT freq, len, x, i, l;

   //
   // Skip header
   //
   lpVOC += 0x1B;

   //
   // Length is 3 bytes long
   //
   len = SWAP16(*(LPWORD)lpVOC);
   lpVOC += 2;
   x = *(lpVOC++);
   x <<= 16;
   len += x - 2;

   //
   // One byte for frequency
   //
   freq = 1000000 / (256 - *lpVOC);

#if 1

   lpVOC += 2;

   //
   // Convert the sample to 22050Hz manually, as SDL doesn't like "strange" sample rates.
   //
   x = (INT)(len * (22050.0 / freq));

   *lppBuffer = (LPBYTE)calloc(1, x);
   if (*lppBuffer == NULL)
   {
      return NULL;
   }
   for (i = 0; i < x; i++)
   {
      l = (INT)(i * (freq / 22050.0));
      if (l < 0)
      {
         l = 0;
      }
      else if (l >= len)
      {
         l = len - 1;
      }
      (*lppBuffer)[i] = lpVOC[l];
   }

   lpSpec->channels = 1;
   lpSpec->format = AUDIO_U8;
   lpSpec->freq = 22050;
   lpSpec->size = x;

#else

   *lppBuffer = (unsigned char *)malloc(len);
   if (*lppBuffer == NULL)
   {
      return NULL;
   }

   lpSpec->channels = 1;
   lpSpec->format = AUDIO_U8;
   lpSpec->freq = freq;
   lpSpec->size = len;

   lpVOC += 2;
   memcpy(*lppBuffer, lpVOC, len);

#endif

   return lpSpec;
}

static VOID SDLCALL
SOUND_FillAudio(
   LPVOID          udata,
   LPBYTE          stream,
   INT             len
)
/*++
  Purpose:

    SDL sound callback function.

  Parameters:

    [IN]  udata - pointer to user-defined parameters (Not used).

    [OUT] stream - pointer to the stream buffer.

    [IN]  len - Length of the buffer.

  Return value:

    None.

--*/
{
   //
   // Play music
   //
   if (!g_fNoMusic)
   {
      RIX_FillBuffer(stream, len);
   }

   //
   // No current playing sound
   //
   if (g_fNoSound || gSndPlayer.buf == NULL)
   {
      return;
   }

   //
   // Only play if we have data left
   //
   if (gSndPlayer.audio_len == 0)
   {
      //
      // Delete the audio buffer from memory
      //
      free(gSndPlayer.buf);
      gSndPlayer.buf = NULL;
      return;
   }

   //
   // Mix as much data as possible
   //
   len = (len > gSndPlayer.audio_len) ? gSndPlayer.audio_len : len;
   SDL_MixAudio(stream, gSndPlayer.pos, len, gVolume);
   gSndPlayer.pos += len;
   gSndPlayer.audio_len -= len;
}

INT
SOUND_OpenAudio(
   VOID
)
/*++
  Purpose:

    Initialize the audio subsystem.

  Parameters:

    None.

  Return value:

    0 if succeed, others if failed.

--*/
{
   SDL_AudioSpec spec;

   if (gSndOpened)
   {
      //
      // Already opened
      //
      return -1;
   }

   gSndOpened = FALSE;

   //
   // Load the MKF file.
   //
   //gSndPlayer.mkf = fopen("voc.mkf", "rb");
   gSndPlayer.mkf = fopen("e:\\data\\pal\\voc.mkf", "rb");

   if (gSndPlayer.mkf == NULL)
   {
      return -2;
   }

   //
   // Open the sound subsystem.
   //
   gSndPlayer.spec.freq = 22050;
   gSndPlayer.spec.format = AUDIO_S16;
   gSndPlayer.spec.channels = 1;
   gSndPlayer.spec.samples = 1024;
   gSndPlayer.spec.callback = SOUND_FillAudio;

   if (SDL_OpenAudio(&gSndPlayer.spec, &spec) < 0)
   {
      //
      // Failed
      //
      return -3;
   }

   memcpy(&gSndPlayer.spec, &spec, sizeof(SDL_AudioSpec));

   gSndPlayer.buf = NULL;
   gSndPlayer.pos = NULL;
   gSndPlayer.audio_len = 0;
   gSndOpened = TRUE;

   //
   // Initialize the music subsystem.
   //
   RIX_Init("e:\\data\\pal\\mus.mkf");
  
   //RIX_Init("mus.mkf");

   //
   // Let the callback function run so that musics will be played.
   //
   SDL_PauseAudio(0);

   return 0;
}

VOID
SOUND_CloseAudio(
   VOID
)
/*++
  Purpose:

    Close the audio subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   SDL_CloseAudio();

   if (gSndPlayer.buf != NULL)
   {
      free(gSndPlayer.buf);
      gSndPlayer.buf = NULL;
   }

   if (gSndPlayer.mkf != NULL)
   {
      fclose(gSndPlayer.mkf);
      gSndPlayer.mkf = NULL;
   }

   RIX_Shutdown();
}

VOID
SOUND_Play(
   INT    iSoundNum
)
/*++
  Purpose:

    Play a sound in voc.mkf file.

  Parameters:

    [IN]  iSoundNum - number of the sound.

  Return value:

    None.

--*/
{
   SDL_AudioCVT    wavecvt;
   SDL_AudioSpec   wavespec;
   LPBYTE          buf, bufdec;
   UINT            samplesize;
   int             len;

   if (!gSndOpened || g_fNoSound)
   {
      return;
   }

   //
   // Stop playing current sound.
   //
   if (gSndPlayer.buf != NULL)
   {
      LPBYTE p = gSndPlayer.buf;
      gSndPlayer.buf = NULL;
      free(p);
   }

   if (iSoundNum < 0)
   {
      return;
   }

   //
   // Get the length of the sound file.
   //
   len = PAL_MKFGetChunkSize(iSoundNum, gSndPlayer.mkf);
   if (len <= 0)
   {
      return;
   }

   buf = (LPBYTE)calloc(len, 1);
   if (buf == NULL)
   {
      return;
   }

   //
   // Read the sound file from the MKF archive.
   //
   PAL_MKFReadChunk(buf, len, iSoundNum, gSndPlayer.mkf);

   SOUND_LoadVOCFromBuffer(buf, &wavespec, &bufdec);
   free(buf);

   //
   // Build the audio converter and create conversion buffers
   //
   if (SDL_BuildAudioCVT(&wavecvt, wavespec.format, wavespec.channels, wavespec.freq,
      gSndPlayer.spec.format, gSndPlayer.spec.channels, gSndPlayer.spec.freq) < 0)
   {
      free(bufdec);
      return;
   }

   samplesize = ((wavespec.format & 0xFF) / 8) * wavespec.channels;
   wavecvt.len = wavespec.size & ~(samplesize - 1);
   wavecvt.buf = (LPBYTE)malloc(wavecvt.len * wavecvt.len_mult);
   if (wavecvt.buf == NULL)
   {
      free(bufdec);
      return;
   }
   memcpy(wavecvt.buf, bufdec, wavespec.size);
   free(bufdec);

   //
   // Run the audio converter
   //
   if (SDL_ConvertAudio(&wavecvt) < 0)
   {
      return;
   }

   gSndPlayer.buf = wavecvt.buf;
   gSndPlayer.audio_len = wavecvt.len * wavecvt.len_mult;
   gSndPlayer.pos = wavecvt.buf;
}
