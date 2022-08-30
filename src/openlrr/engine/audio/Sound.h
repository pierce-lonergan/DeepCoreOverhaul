// Sound.h : 
//
/// APIS: mci, mmio
/// DEPENDENCIES: 3DSound, Main, (Errors)
/// DEPENDENTS: 3DSound, Main, ...

#pragma once

#include "../../platform/windows.h"

// no getting around this include without some very ugly work-arounds :(
#include <mmsystem.h>

#include "../../common.h"


/**********************************************************************************
 ******** Forward Global Namespace Declarations
 **********************************************************************************/

#pragma region Forward Declarations

struct IDirectSoundBuffer;
//struct MMCKINFO;
//struct tWAVEFORMATEX;
//typedef struct tWAVEFORMATEX WAVEFORMATEX, * PWAVEFORMATEX, FAR* LPWAVEFORMATEX;
//typedef long MCIERROR;

#pragma endregion


namespace Gods98
{; // !<---

/**********************************************************************************
 ******** Function Typedefs
 **********************************************************************************/

#pragma region Function Typedefs

typedef void (__cdecl* SoundCDStopCallback)(void);

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define WAVEVERSION			1

#ifndef ER_MEM
#define ER_MEM 				0xe000
#endif

#ifndef ER_CANNOTOPEN
#define ER_CANNOTOPEN 		0xe100
#endif

#ifndef ER_NOTWAVEFILE
#define ER_NOTWAVEFILE 		0xe101
#endif

#ifndef ER_CANNOTREAD
#define ER_CANNOTREAD 		0xe102
#endif

#ifndef ER_CORRUPTWAVEFILE
#define ER_CORRUPTWAVEFILE	0xe103
#endif

#ifndef ER_CANNOTWRITE
#define ER_CANNOTWRITE		0xe104
#endif


#define MAX_SAMPLES			100

#define MULTI_SOUND			5

#define MCI_RETURN_SIZE		200

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum class SoundMode : uint32
{
	Once  = 0,
	Loop  = 1,
	Multi = 2,
};
assert_sizeof(SoundMode, 0x4);


// (used internally)
enum class Sound_CDMode
{
	None,  // All other values
	Error, // Failed to get mode (or Music Fix dll returned "beef")
	NotReady,
	Paused,
	Playing,
	Stopped,
};

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

// (unused)
struct SND
{
	char sName[128];
	sint32 sSize;
	sint32 cFreq;
	sint32 cVolume;
	sint32 sOffset;
	sint32 sActive;
	char* sData;
	WAVEFORMATEX* pwf;
	IDirectSoundBuffer* lpDsb;
	IDirectSoundBuffer* lpDsb3D;
#ifdef MULTI_SOUND
	IDirectSoundBuffer* lpDsbM[MULTI_SOUND];
	IDirectSoundBuffer* lpDsbM3D[MULTI_SOUND];
	sint32 sVoice;
	sint32 sVoice3D;
#endif
};


struct Sound
{
	/*0,4*/ uint32 handle;
	/*4*/
};
assert_sizeof(Sound, 0x4);


struct Sound_Globs
{
	// [globs: start]
	/*000,4*/ uint32 useSound;		// Number of sounds in soundList
	/*004,4*/ bool32 initialised;
	/*008,190*/ Sound soundList[MAX_SAMPLES];
	/*198,4*/ sint32 currTrack;
	/*19c,4*/ bool32 loopCDTrack;
	/*1a0,4*/ SoundCDStopCallback CDStopCallback;
	/*1a4,4*/ bool32 updateCDTrack;
	// [globs: end]
	/*1a8,4*/ uint32 s_Update_lastUpdate;
	/*1ac,8*/ uint32 reserved1[2];
	/*1b4,4*/ MCIERROR mciErr;
	/*1b8*/
};
assert_sizeof(Sound_Globs, 0x1b8);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

//extern SND sndTbl[MAX_SAMPLES];

// <LegoRR.exe @00545868>
extern Sound_Globs & soundGlobs;

// <LegoRR.exe @00577500>
extern char (& mciReturn)[MCI_RETURN_SIZE];

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

/// CUSTOM: Returns the number of detected cdaudio tracks.
uint32 Sound_CDTrackCount();

/// CUSTOM: Returns true if the cdaudio is currently playing.
bool Sound_IsCDPlaying();


// <LegoRR.exe @00488e10>
bool32 __cdecl Sound_Initialise(bool32 nosound);

// <LegoRR.exe @00488e50>
bool32 __cdecl Sound_IsInitialised(void);

// <LegoRR.exe @00488e70>
bool32 __cdecl Sound_PlayCDTrack(uint32 track, SoundMode mode, SoundCDStopCallback StopCallback);

// <LegoRR.exe @00488eb0>
bool32 __cdecl Sound_StopCD(void);

// <LegoRR.exe @00488ec0>
void __cdecl Sound_Update(bool32 cdtrack);

// <LegoRR.exe @00488f30>
sint32 __cdecl WaveOpenFile(void* fileData, uint32 fileSize,
				OUT HMMIO* phmmioIn, OUT WAVEFORMATEX** ppwfxInfo, OUT MMCKINFO* pckInRIFF);

// <LegoRR.exe @00489130>
uint32 __cdecl GetWaveAvgBytesPerSec(const char* pszFileName);

// <LegoRR.exe @004891d0>
sint32 __cdecl WaveOpenFile2(IN const char* pszFileName,
				OUT HMMIO* phmmioIn, OUT WAVEFORMATEX** ppwfxInfo, OUT MMCKINFO* pckInRIFF);

// <LegoRR.exe @00489380>
sint32 __cdecl WaveStartDataRead(HMMIO* phmmioIn, MMCKINFO* pckIn, MMCKINFO* pckInRIFF);

// <LegoRR.exe @004893c0>
sint32 __cdecl WaveReadFile(IN HMMIO hmmioIn, IN uint32 cbRead, OUT uint8* pbDest,
				IN MMCKINFO* pckIn, OUT uint32* cbActualRead);

// <LegoRR.exe @00489490>
sint32 __cdecl WaveCloseReadFile(IN HMMIO* phmmio, IN WAVEFORMATEX** ppwfxSrc);

// Starts playing the specified track in the cdaudio device (or a random track for Music Fix dll).
// <LegoRR.exe @004894d0>
bool32 __cdecl Restart_CDTrack(sint32 track);

// <LegoRR.exe @00489520>
void __cdecl ReportCDError(void);

// Returns true if the cdaudio device is playing and its current track is less than or equal to the specified.
// <LegoRR.exe @00489540>
bool32 __cdecl Status_CDTrack(sint32 track);

// Opens and starts specified track in the cdaudio device (or a random track for Music Fix dll).
// <LegoRR.exe @004895f0>
bool32 __cdecl Play_CDTrack(sint32 track);

// Stops and closes the cdaudio device.
// <LegoRR.exe @00489660>
bool32 __cdecl Stop_CDTrack(void);


/// CUSTOM: Opens the cdaudio device if not already open, and returns its opened state (true if open).
bool Open_CD();

/// CUSTOM: Closes the cdaudio device if not already closed, and returns its closed state (true if closed)
bool Close_CD();

/// CUSTOM: Stops the cdaudio device without closing it.
bool Stop_CD();

/// CUSTOM: Returns the cdaudio playback mode (or Error on failure/Music Fix dll).
Sound_CDMode Mode_CD();

/// CUSTOM: Returns the number of cdaudio tracks (or -1 on failure/Music Fix dll).
sint32 Count_CDTracks();

/// CUSTOM: Returns the current cdaudio track (1-indexed?) (or -1 on failure/Music Fix dll).
sint32 Current_CDTrack();

#pragma endregion

}
