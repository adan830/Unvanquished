/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// snd_local.h -- private sound definations

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_public.h"

#define PAINTBUFFER_SIZE     4096                               // this is in samples

#define SND_CHUNK_SIZE       1024                               // samples
#define SND_CHUNK_SIZE_FLOAT ( SND_CHUNK_SIZE / 2 )             // floats
#define SND_CHUNK_SIZE_BYTE  ( SND_CHUNK_SIZE * 2 )             // floats

typedef struct
{
	int left;                       // the final values will be clamped to +/- 0x00ffff00 and shifted down
	int right;
} portable_samplepair_t;

typedef struct adpcm_state
{
	short sample;           /* Previous output value */
	char  index;            /* Index into stepsize table */
} adpcm_state_t;

typedef struct sndBuffer_s
{
	short              sndChunk[ SND_CHUNK_SIZE ];
	struct sndBuffer_s *next;

	int                size;
	adpcm_state_t      adpcm;
} sndBuffer;

typedef struct sfx_s
{
	sndBuffer    *soundData;
	qboolean     defaultSound;                              // couldn't be loaded, so use buzz
	qboolean     inMemory;                                  // not in Memory
	qboolean     soundCompressed;                           // not in Memory
	int          soundCompressionMethod;
	int          soundLength;
	char         soundName[ MAX_QPATH ];
	int          lastTimeUsed;
	int          duration;
	struct sfx_s *next;
} sfx_t;

typedef struct
{
	int  channels;
	int  samples;                                           // mono samples in buffer
	int  submission_chunk;                                  // don't mix less than this #
	int  samplebits;
	int  speed;
	byte *buffer;
} dma_t;

#define START_SAMPLE_IMMEDIATE 0x7fffffff

typedef struct loopSound_s
{
	vec3_t   origin;
	vec3_t   velocity;
	sfx_t    *sfx;
	int      mergeFrame;
	qboolean active;
	qboolean kill;
	qboolean doppler;
	float    dopplerScale;
	float    oldDopplerScale;
	int      framenum;
} loopSound_t;

typedef struct
{
	int      allocTime;
	int      startSample;                   // START_SAMPLE_IMMEDIATE = set immediately on next mix
	int      entnum;                        // to allow overriding a specific sound
	int      entchannel;                    // to allow overriding a specific sound
	int      leftvol;                       // 0-255 volume after spatialization
	int      rightvol;                      // 0-255 volume after spatialization
	int      master_vol;                    // 0-255 volume before spatialization
	float    dopplerScale;
	float    oldDopplerScale;
	vec3_t   origin;                // only use if fixed_origin is set
	qboolean fixed_origin;          // use origin instead of fetching entnum's origin
	sfx_t    *thesfx;               // sfx structure
	qboolean doppler;
} channel_t;

#define WAV_FORMAT_PCM 1

typedef struct
{
	int format;
	int rate;
	int width;
	int channels;
	int samples;
	int dataofs;                                    // chunk starts this many bytes from file start
} wavinfo_t;

/*
====================================================================

  SYSTEM SPECIFIC FUNCTIONS

====================================================================
*/

// initializes cycling through a DMA buffer and returns information on it
qboolean SNDDMA_Init( void );

// gets the current DMA position
int      SNDDMA_GetDMAPos( void );

// shutdown the DMA xfer.
void     SNDDMA_Shutdown( void );

void     SNDDMA_BeginPainting ( void );

void     SNDDMA_Submit( void );

//====================================================================

#define MAX_CHANNELS 96

extern  channel_t             s_channels[ MAX_CHANNELS ];
extern  channel_t             loop_channels[ MAX_CHANNELS ];
extern  int                   numLoopChannels;

extern  int                   s_paintedtime;
extern  int                   s_rawend;
extern  vec3_t                listener_forward;
extern  vec3_t                listener_right;
extern  vec3_t                listener_up;
extern  dma_t                 dma;

extern unsigned char          s_entityTalkAmplitude[ MAX_CLIENTS ];

#define MAX_RAW_SAMPLES 16384
extern  portable_samplepair_t s_rawsamples[ MAX_RAW_SAMPLES ];

extern cvar_t                 *s_volume;
extern cvar_t                 *s_nosound;
extern cvar_t                 *s_khz;
extern cvar_t                 *s_show;
extern cvar_t                 *s_mixahead;

extern cvar_t                 *s_testsound;
extern cvar_t                 *s_separation;

qboolean                      S_LoadSound( sfx_t *sfx );

void                          SND_free( sndBuffer *v );
sndBuffer                      *SND_malloc();
void                          SND_setup();

void                          S_PaintChannels( int endtime );

void                          S_memoryLoad( sfx_t *sfx );
portable_samplepair_t         *S_GetRawSamplePointer();

// spatializes a channel
void                          S_Spatialize( channel_t *ch );

int                           S_GetVoiceAmplitude( int entityNum );

// adpcm functions
int                           S_AdpcmMemoryNeeded( const wavinfo_t *info );
void                          S_AdpcmEncodeSound( sfx_t *sfx, short *samples );
void                          S_AdpcmGetSamples( sndBuffer *chunk, short *to );

// wavelet function

#define SENTINEL_MULAW_ZERO_RUN     127
#define SENTINEL_MULAW_FOUR_BIT_RUN 126

void S_FreeOldestSound();

#define NXStream byte

void         encodeWavelet( sfx_t *sfx, short *packets );
void         decodeWavelet( sndBuffer *stream, short *packets );

void         encodeMuLaw( sfx_t *sfx, short *packets );

extern short mulawToShort[ 256 ];

extern short *sfxScratchBuffer;
extern sfx_t *sfxScratchPointer;
extern int   sfxScratchIndex;

void         SOrig_Init( void );
void         SOrig_Shutdown( void );
void         SOrig_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx );
void         SOrig_StartLocalSound( sfxHandle_t sfx, int channelNum );
void         SOrig_StartBackgroundTrack( const char *intro, const char *loop );
void         SOrig_StopBackgroundTrack( void );
void         SOrig_RawSamples( int stream, int samples, int rate, int width, int s_channels, const byte *data, float volume, int entityNum );
void         SOrig_StopAllSounds( void );
void         SOrig_ClearLoopingSounds( qboolean killall );
void         SOrig_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void         SOrig_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void         SOrig_StopLoopingSound( int entityNum );

void         SOrig_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[ 3 ], int inwater );
void         SOrig_UpdateEntityPosition( int entityNum, const vec3_t origin );
void         SOrig_Update( void );
void         SOrig_DisableSounds( void );
void         SOrig_BeginRegistration( void );
sfxHandle_t  SOrig_RegisterSound( const char *sample, qboolean compressed );
void         SOrig_ClearSoundBuffer( void );
int          SOrig_SoundDuration( sfxHandle_t handle );
int          SOrig_GetVoiceAmplitude( int entnum );
int          SOrig_GetSoundLength( sfxHandle_t sfxHandle );

#if defined( USE_VOIP )
void         SOrig_StartCapture( void );
int          SOrig_AvailableCaptureSamples( void );
void         SOrig_Capture( int samples, byte *data );
void         SOrig_StopCapture( void );
void         SOrig_MasterGain( float gain );

#endif

int SOrig_GetCurrentSoundTime ( void );
