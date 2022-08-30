// Sound.cpp : 
//

#include "../../platform/windows.h"
#include "../../platform/d3drm.h"
#include "../../platform/dsound.h"
#include "../../platform/timeapi.h"
#include <windowsx.h>
#include <mmsystem.h>

#include "../core/Errors.h"
#include "../Main.h"
#include "3DSound.h"

#include "Sound.h"


/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define SOUND_MUSICUPDATEDELAY	4000	// Wait 4 seconds before performing update checks.

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

//extern SND sndTbl[MAX_SAMPLES];

// <LegoRR.exe @00545868>
Gods98::Sound_Globs & Gods98::soundGlobs = *(Gods98::Sound_Globs*)0x00545868; // = { 0 };

// <LegoRR.exe @00577500>
char (& Gods98::mciReturn)[MCI_RETURN_SIZE] = *(char(*)[MCI_RETURN_SIZE])0x00577500;

static bool _cdOpen     = false;  // The cdaudio device is currently assumed to be open.
static bool _cdError    = false;  // A cdaudio error was reported (set, but never read).
static bool _cdPlaying  = false;  // The cdaudio device is currently assumed to be playing (and open).
//static bool _cdMusicFix = false;  // The dll housing mciSendStringA is assigned to be the Musix Fix winmm.dll (which has some bugs).
static uint32 _cdTrackCount  = 0; // The number of tracks detected by the cdaudio device.
static uint32 _cdClosingTime = 0; // If non-zero, then the time at which the cdaudio device was stopped.
                                  // Wait a bit longer from this point then close the device.

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

uint32 Gods98::Sound_CDTrackCount()
{
	return _cdTrackCount;
}

bool Gods98::Sound_IsCDPlaying()
{
	return _cdPlaying;
}



// <LegoRR.exe @00488e10>
bool32 __cdecl Gods98::Sound_Initialise(bool32 nosound)
{
	log_firstcall();

	soundGlobs.initialised = !nosound;
	soundGlobs.loopCDTrack = false;

	if (nosound)
		return true;

	// Get the number of detected music tracks.
	/// NOTE: Music Fix dll doesn't need the device to be open to get this number.
	///       But is this the same with the regular winmm.dll??
	//if (Open_CD()) {
		_cdTrackCount = std::max(0, Count_CDTracks());
		Error_DebugF("CD: found %i tracks\n", _cdTrackCount);
	//	Close_CD();
	//}

	if (Sound3D_Initialise(Main_hWnd()))
		return true;

	return false;
}

// <LegoRR.exe @00488e50>
bool32 __cdecl Gods98::Sound_IsInitialised(void)
{
	log_firstcall();

	return (soundGlobs.initialised && Sound3D_Initialised());
}

// <LegoRR.exe @00488e70>
bool32 __cdecl Gods98::Sound_PlayCDTrack(uint32 track, SoundMode mode, SoundCDStopCallback StopCallback)
{
	log_firstcall();

	if (soundGlobs.initialised) {
		if (_cdPlaying) {
			// Stopping the cdaudio device first is essential because of how Music Fix dll only shuffles on play.
			Stop_CD();
		}

		soundGlobs.loopCDTrack = (mode == SoundMode::Loop);
		soundGlobs.currTrack = track;
		soundGlobs.CDStopCallback = StopCallback;
		_cdPlaying = Play_CDTrack(track);
		_cdClosingTime = 0; // Don't close CD.

		/// NOTE: We currently don't use this for anything, so no need to perform the check.
		// Music Fix dll does not report a lot of information like current track, use this to determine if we need special handling.
		//_cdMusicFix = _cdPlaying && (Current_CDTrack() == -1);
		//Error_DebugF("CD: This %s the Music Fix dll\n", (_cdMusicFix ? "is" : "is not"));

		return _cdPlaying;
	}

	return false;
}

// <LegoRR.exe @00488eb0>
bool32 __cdecl Gods98::Sound_StopCD(void)
{
	log_firstcall();

	if (soundGlobs.initialised) {
		if (_cdPlaying) {
			/// CHANGE: Music Fix dll isn't thread-safe when closing cdaudio, so only stop the cdaudio from playing (which is safe).
			//          Instead we'll close the CD after a time delay to avoid race conditions.
			_cdPlaying = !Stop_CD();
		}
		// Still close the device even if we weren't playing to start.
		if (!_cdPlaying) {
			_cdClosingTime = Main_GetTime();
		}
		return !_cdPlaying;
	}

	return false;
}

// <LegoRR.exe @00488ec0>
void __cdecl Gods98::Sound_Update(bool32 cdtrack)
{
	log_firstcall();

	if (soundGlobs.initialised) {
		//static uint32 lastUpdate = 0;
		const uint32 time = Main_GetTime();

		/// CHANGE: Close the CD after a time delay to avoid race conditions in Music Fix dll.
		if (_cdClosingTime != 0 && time > (_cdClosingTime + SOUND_MUSICUPDATEDELAY)) {
			if (!_cdPlaying) {
				//Stop_CDTrack();
				Close_CD();
				Error_Debug("CD: closed\n");
			}
			_cdClosingTime = 0; // No longer closing.
		}

		if (time > (soundGlobs.s_Update_lastUpdate + SOUND_MUSICUPDATEDELAY)) {

			if (cdtrack) {
				if (!Status_CDTrack(soundGlobs.currTrack)) {
					if (soundGlobs.loopCDTrack) Restart_CDTrack(soundGlobs.currTrack);
					if (soundGlobs.CDStopCallback) soundGlobs.CDStopCallback();
				}
			}
			soundGlobs.s_Update_lastUpdate = time;
		}
	}
}

// <LegoRR.exe @00488f30>
sint32 __cdecl Gods98::WaveOpenFile(void* fileData, uint32 fileSize,
				OUT HMMIO* phmmioIn, OUT WAVEFORMATEX** ppwfxInfo, OUT MMCKINFO* pckInRIFF)
{
	log_firstcall();

	MMCKINFO        ckIn;           // chunk info. for general use.
	PCMWAVEFORMAT   pcmWaveFormat;  // Temp PCM structure to load in.       
	uint16            cbExtraAlloc;   // Extra bytes for waveformatex
	MMIOINFO		mmioInfo;		// For loading from memory...


	// Initialization...
	*ppwfxInfo = nullptr;
	sint32 nError = 0; // Return value.
	HMMIO hmmioIn = nullptr;

	std::memset(&mmioInfo, 0, sizeof(MMIOINFO));
	mmioInfo.fccIOProc = FOURCC_MEM;
	mmioInfo.pchBuffer = (HPSTR)fileData;
	mmioInfo.cchBuffer = fileSize;
	
	//	if ((hmmioIn = ::mmioOpenA(pszFileName, nullptr, MMIO_ALLOCBUF | MMIO_READ)) == nullptr)
	if ((hmmioIn = ::mmioOpenA(nullptr, &mmioInfo, MMIO_ALLOCBUF | MMIO_READ)) == nullptr)
	{
		nError = ER_CANNOTOPEN;
		goto ERROR_READING_WAVE;
	}
	
	if ((nError = (sint32)::mmioDescend(hmmioIn, pckInRIFF, nullptr, 0)) != 0)
	{
		goto ERROR_READING_WAVE;
	}
	
	
	if ((pckInRIFF->ckid != FOURCC_RIFF) || (pckInRIFF->fccType != mmioFOURCC('W', 'A', 'V', 'E')))
	{
		nError = ER_NOTWAVEFILE;
		goto ERROR_READING_WAVE;
	}
	
	/* Search the input file for for the 'fmt ' chunk.     */
	ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
	if ((nError = (sint32)::mmioDescend(hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0)
	{
		goto ERROR_READING_WAVE;                
	}
	
	/* Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
	* if there are extra parameters at the end, we'll ignore them */
	
	if (ckIn.cksize < (sint32) sizeof(PCMWAVEFORMAT))
	{
		nError = ER_NOTWAVEFILE;
		goto ERROR_READING_WAVE;
	}
	
	/* Read the 'fmt ' chunk into <pcmWaveFormat>.*/     
	if (::mmioRead(hmmioIn, (HPSTR) &pcmWaveFormat, (sint32) sizeof(pcmWaveFormat)) != (sint32) sizeof(pcmWaveFormat))
	{
		nError = ER_CANNOTREAD;
		goto ERROR_READING_WAVE;
	}
	
	
	// Ok, allocate the waveformatex, but if its not pcm
	// format, read the next word, and thats how many extra
	// bytes to allocate.
	if (pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM)
	{
		cbExtraAlloc = 0;
	}
	else
	{
		// Read in length of extra bytes.
		if (::mmioRead(hmmioIn, (LPTSTR) &cbExtraAlloc,
			(sint32) sizeof(cbExtraAlloc)) != (sint32) sizeof(cbExtraAlloc))
		{
			nError = ER_CANNOTREAD;
			goto ERROR_READING_WAVE;
		}
		
	}
	
	// Ok, now allocate that waveformatex structure.
	if ((*ppwfxInfo = (WAVEFORMATEX*)::GlobalAlloc(GMEM_FIXED, sizeof(WAVEFORMATEX)+cbExtraAlloc)) == nullptr)
	{
		nError = ER_MEM;
		goto ERROR_READING_WAVE;
	}
	
	// Copy the bytes from the pcm structure to the waveformatex structure
	std::memcpy(*ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat));
	(*ppwfxInfo)->cbSize = cbExtraAlloc;
	
	// Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
	if (cbExtraAlloc != 0)
	{
		if (::mmioRead(hmmioIn, (LPTSTR) (((uint8*)&((*ppwfxInfo)->cbSize))+sizeof(cbExtraAlloc)),
			(sint32) (cbExtraAlloc)) != (sint32) (cbExtraAlloc))
		{
			nError = ER_NOTWAVEFILE;
			goto ERROR_READING_WAVE;
		}
	}
	
	/* Ascend the input file out of the 'fmt ' chunk. */                                                            
	if ((nError = ::mmioAscend(hmmioIn, &ckIn, 0)) != 0)
	{
		goto ERROR_READING_WAVE;
		
	}
	
	goto TEMPCLEANUP;               

ERROR_READING_WAVE:
	if (*ppwfxInfo != nullptr)
	{
		::GlobalFree(*ppwfxInfo);
		*ppwfxInfo = nullptr;
	}               

	if (hmmioIn != nullptr)
	{
		::mmioClose(hmmioIn, 0);
		hmmioIn = nullptr;
	}
	
TEMPCLEANUP:
	*phmmioIn = hmmioIn;

	return(nError);
}

// <LegoRR.exe @00489130>
uint32 __cdecl Gods98::GetWaveAvgBytesPerSec(const char* pszFileName)
{
	log_firstcall();

	MMCKINFO        ckIn;           // chunk info. for general use.
	PCMWAVEFORMAT   pcmWaveFormat;  // Temp PCM structure to load in.
	uint32 avgBytesPerSec = 0;
	MMCKINFO		pckInRIFF;


	// Initialization...
	sint32 nError = 0;
	HMMIO hmmioIn = nullptr;
	
	if ((hmmioIn = ::mmioOpenA(const_cast<char*>(pszFileName), nullptr, MMIO_READ)) == nullptr)
	{
		nError = ER_CANNOTOPEN;
		goto ERROR_READING_WAVE;
	}

	if ((nError = (sint32)::mmioDescend(hmmioIn, &pckInRIFF, nullptr, 0)) != 0)
	{
		goto ERROR_READING_WAVE;
	}

	if ((pckInRIFF.ckid != FOURCC_RIFF) || (pckInRIFF.fccType != mmioFOURCC('W', 'A', 'V', 'E')))
	{
		nError = ER_NOTWAVEFILE;
		goto ERROR_READING_WAVE;
	}
			
	/* Search the input file for for the 'fmt ' chunk.     */
    ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if ((nError = (sint32)::mmioDescend(hmmioIn, &ckIn, &pckInRIFF, MMIO_FINDCHUNK)) != 0)
	{
		goto ERROR_READING_WAVE;
	}
					
	/* Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
    * if there are extra parameters at the end, we'll ignore them */
    
    if (ckIn.cksize < (sint32) sizeof(PCMWAVEFORMAT))
	{
		nError = ER_NOTWAVEFILE;
		goto ERROR_READING_WAVE;
	}
															
	/* Read the 'fmt ' chunk into <pcmWaveFormat>.*/     
    if (::mmioRead(hmmioIn, (HPSTR) &pcmWaveFormat, (sint32) sizeof(pcmWaveFormat)) != (sint32) sizeof(pcmWaveFormat))
	{
		nError = ER_CANNOTREAD;
		goto ERROR_READING_WAVE;
	}
	
	avgBytesPerSec = pcmWaveFormat.wf.nAvgBytesPerSec;

ERROR_READING_WAVE:
	if (hmmioIn != nullptr)
	{
		::mmioClose(hmmioIn, 0);
		hmmioIn = nullptr;
	}

	return avgBytesPerSec;
}

// <LegoRR.exe @004891d0>
sint32 __cdecl Gods98::WaveOpenFile2(IN const char* pszFileName,
				OUT HMMIO* phmmioIn, OUT WAVEFORMATEX** ppwfxInfo, OUT MMCKINFO* pckInRIFF)
{
	log_firstcall();

	MMCKINFO        ckIn;           // chunk info. for general use.
	PCMWAVEFORMAT   pcmWaveFormat;  // Temp PCM structure to load in.       
	WORD            cbExtraAlloc;   // Extra bytes for waveformatex


	// Initialization...
	*ppwfxInfo = nullptr;
	sint32 nError = 0; // Return value.
	HMMIO hmmioIn = nullptr;
	
	if ((hmmioIn = ::mmioOpenA(const_cast<char*>(pszFileName), nullptr, MMIO_ALLOCBUF | MMIO_READ)) == nullptr)
	{
		nError = ER_CANNOTOPEN;
		goto ERROR_READING_WAVE;
	}

	if ((nError = (sint32)::mmioDescend(hmmioIn, pckInRIFF, nullptr, 0)) != 0)
	{
		goto ERROR_READING_WAVE;
	}


	if ((pckInRIFF->ckid != FOURCC_RIFF) || (pckInRIFF->fccType != mmioFOURCC('W', 'A', 'V', 'E')))
	{
		nError = ER_NOTWAVEFILE;
		goto ERROR_READING_WAVE;
	}
			
	/* Search the input file for for the 'fmt ' chunk.     */
    ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if ((nError = (sint32)::mmioDescend(hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0)
	{
		goto ERROR_READING_WAVE;                
	}
					
	/* Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
    * if there are extra parameters at the end, we'll ignore them */
    
    if (ckIn.cksize < (sint32) sizeof(PCMWAVEFORMAT))
	{
		nError = ER_NOTWAVEFILE;
		goto ERROR_READING_WAVE;
	}
															
	/* Read the 'fmt ' chunk into <pcmWaveFormat>.*/     
    if (::mmioRead(hmmioIn, (HPSTR) &pcmWaveFormat, (sint32) sizeof(pcmWaveFormat)) != (sint32) sizeof(pcmWaveFormat))
	{
		nError = ER_CANNOTREAD;
		goto ERROR_READING_WAVE;
	}
							

	// Ok, allocate the waveformatex, but if its not pcm
	// format, read the next word, and thats how many extra
	// bytes to allocate.
	if (pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM)
	{
		cbExtraAlloc = 0;
	}
	else
	{
		// Read in length of extra bytes.
		if (::mmioRead(hmmioIn, (LPSTR) &cbExtraAlloc,
			(sint32) sizeof(cbExtraAlloc)) != (sint32) sizeof(cbExtraAlloc))
		{
			nError = ER_CANNOTREAD;
			goto ERROR_READING_WAVE;
		}

	}
							
	// Ok, now allocate that waveformatex structure.
	if ((*ppwfxInfo = (WAVEFORMATEX*)::GlobalAlloc(GMEM_FIXED, sizeof(WAVEFORMATEX)+cbExtraAlloc)) == nullptr)
	{
		nError = ER_MEM;
		goto ERROR_READING_WAVE;
	}

	// Copy the bytes from the pcm structure to the waveformatex structure
	std::memcpy(*ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat));
	(*ppwfxInfo)->cbSize = cbExtraAlloc;

	// Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
	if (cbExtraAlloc != 0)
	{
		if (::mmioRead(hmmioIn, (LPSTR) (((uint8*)&((*ppwfxInfo)->cbSize))+sizeof(cbExtraAlloc)),
			(sint32) (cbExtraAlloc)) != (sint32) (cbExtraAlloc))
		{
			nError = ER_NOTWAVEFILE;
			goto ERROR_READING_WAVE;
		}
	}

	/* Ascend the input file out of the 'fmt ' chunk. */                                                            
	if ((nError = ::mmioAscend(hmmioIn, &ckIn, 0)) != 0)
	{
		goto ERROR_READING_WAVE;

	}
	

	goto TEMPCLEANUP;               

ERROR_READING_WAVE:
	if (*ppwfxInfo != nullptr)
	{
		::GlobalFree(*ppwfxInfo);
		*ppwfxInfo = nullptr;
	}               

	if (hmmioIn != nullptr)
	{
		::mmioClose(hmmioIn, 0);
		hmmioIn = nullptr;
	}
	
TEMPCLEANUP:
	*phmmioIn = hmmioIn;

	return(nError);
}

// <LegoRR.exe @00489380>
sint32 __cdecl Gods98::WaveStartDataRead(HMMIO* phmmioIn, MMCKINFO* pckIn, MMCKINFO* pckInRIFF)
{
	log_firstcall();

	sint32 nError = 0;
	
	// Do a nice little seek...
	if ((nError = ::mmioSeek(*phmmioIn, pckInRIFF->dwDataOffset + sizeof(FOURCC), SEEK_SET)) == -1)
	{
		nError = 1;
	}

	nError = 0;
	//      Search the input file for for the 'data' chunk.
	pckIn->ckid = mmioFOURCC('d', 'a', 't', 'a');
	if ((nError = ::mmioDescend(*phmmioIn, pckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0)
	{
		//goto ERROR_READING_WAVE;
	}

	//goto CLEANUP;

//ERROR_READING_WAVE:

//CLEANUP:        
	return(nError);
}

// <LegoRR.exe @004893c0>
sint32 __cdecl Gods98::WaveReadFile(IN HMMIO hmmioIn, IN uint32 cbRead, OUT uint8* pbDest,
				IN MMCKINFO* pckIn, OUT uint32* cbActualRead)
{
	log_firstcall();

	MMIOINFO  mmioinfoIn; // current status of <hmmioIn>
	uint32 cT, cbDataIn;

	sint32 nError = 0;

	if (nError = ::mmioGetInfo(hmmioIn, &mmioinfoIn, 0) != 0)
	{
		//goto ERROR_CANNOT_READ;
		*cbActualRead = 0;
		return(nError);
	}
				
	cbDataIn = cbRead;
	if (cbDataIn > pckIn->cksize) 
		cbDataIn = pckIn->cksize;       

	pckIn->cksize -= cbDataIn;
	
	for (cT = 0; cT < cbDataIn; cT++)
	{
		/* Copy the bytes from the io to the buffer. */
		if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
		{
			if ((nError = ::mmioAdvance(hmmioIn, &mmioinfoIn, MMIO_READ)) != 0)
			{
				//goto ERROR_CANNOT_READ;
				*cbActualRead = 0;
				return(nError);
			} 
			if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
			{
				nError = ER_CORRUPTWAVEFILE;
				//goto ERROR_CANNOT_READ;
				*cbActualRead = 0;
				return(nError);
			}
		}
			
		// Actual copy.
		static_assert(sizeof(*mmioinfoIn.pchNext) == 0x1, "mmioinfoIn.pchNext must be a byte ptr");
		// original from Gods98: can't compile
		//*((uint8*)pbDest+cT) = *((uint8*)mmioinfoIn.pchNext)++;
		*((uint8*)pbDest+cT) = *(uint8*)(mmioinfoIn.pchNext++);
	}

	if ((nError = mmioSetInfo(hmmioIn, &mmioinfoIn, 0)) != 0)
	{
		//goto ERROR_CANNOT_READ;
		*cbActualRead = 0;
		return(nError);
	}

	*cbActualRead = cbDataIn;
//	goto FINISHED_READING;

//ERROR_CANNOT_READ:
//	*cbActualRead = 0;

//FINISHED_READING:
	return(nError);
}

// <LegoRR.exe @00489490>
sint32 __cdecl Gods98::WaveCloseReadFile(IN HMMIO* phmmio, IN WAVEFORMATEX** ppwfxSrc)
{
	log_firstcall();

	if (*ppwfxSrc != nullptr)
	{
		::GlobalFree(*ppwfxSrc);
		*ppwfxSrc = nullptr;
	}

	if (*phmmio != nullptr)
	{
		::mmioClose(*phmmio, 0);
		*phmmio = nullptr;
	}

	return(0);
}

// <LegoRR.exe @004894d0>
bool32 __cdecl Gods98::Restart_CDTrack(sint32 track)
{
	log_firstcall();

	//if (!_cdError)
	{
		char buff[100];
		std::sprintf(buff, "play cdaudio from %i", (track + 1));
		soundGlobs.mciErr = ::mciSendStringA(buff, mciReturn, sizeof(mciReturn), nullptr);
		if (!soundGlobs.mciErr) {
			return true;
		}

		ReportCDError();
	}
	return false;
}

// <LegoRR.exe @00489520>
void __cdecl Gods98::ReportCDError(void)
{
	log_firstcall();

	_cdError = true;

	::mciGetErrorStringA(soundGlobs.mciErr, mciReturn, sizeof(mciReturn));

	Error_Warn(true, mciReturn);
}

// <LegoRR.exe @00489540>
bool32 __cdecl Gods98::Status_CDTrack(sint32 track)
{
	log_firstcall();

	//if (!_cdError)
	{
		if (Mode_CD() != Sound_CDMode::Stopped) {
			return (Current_CDTrack() <= (track + 1));
		}
	}
	return false;
}

// <LegoRR.exe @004895f0>
bool32 __cdecl Gods98::Play_CDTrack(sint32 track)
{
	log_firstcall();

	//if (!_cdError)
	{
		if (Open_CD()) {
			return Restart_CDTrack(track);
		}
	}
	return false;
}

// <LegoRR.exe @00489660>
bool32 __cdecl Gods98::Stop_CDTrack(void)
{
	log_firstcall();

	//if (!_cdError)
	{
		if (Stop_CD()) {
			return Close_CD();
		}
	}
	return false;
}



bool Gods98::Open_CD()
{
	//if (!_cdError)
	{
		if (_cdOpen) {
			return true; // Already open.
		}
		else {
			soundGlobs.mciErr = ::mciSendStringA("open cdaudio", mciReturn, sizeof(mciReturn), nullptr);
			if (!soundGlobs.mciErr) {

				soundGlobs.mciErr = ::mciSendStringA("set cdaudio time format tmsf", mciReturn, sizeof(mciReturn), nullptr);
				if (!soundGlobs.mciErr) {
					_cdOpen = true;
					return true;
				}
				else {
					/// CHANGE: Failed to set format, so just close the CD.
					///         Since this format is necessary to start playing a track from the beginning.
					ReportCDError();
					Close_CD();
					return false;
				}
			}

			ReportCDError();
		}
	}
	return false;
}

bool Gods98::Close_CD()
{
	//if (!_cdError)
	{
		if (!_cdOpen) {
			return true; // Already closed.
		}
		else {
			soundGlobs.mciErr = ::mciSendStringA("close cdaudio", mciReturn, sizeof(mciReturn), nullptr);
			if (!soundGlobs.mciErr) {
				_cdOpen = false;
				return true;
			}

			ReportCDError();
		}
	}
	return false;
}

bool Gods98::Stop_CD()
{
	//if (!_cdError)
	{
		if (!_cdOpen) {
			return true; // Already stopped.
		}
		else {
			soundGlobs.mciErr = ::mciSendStringA("stop cdaudio", mciReturn, sizeof(mciReturn), nullptr);
			if (!soundGlobs.mciErr) {
				return true;
			}

			ReportCDError();
		}
	}
	return false;
}

Gods98::Sound_CDMode Gods98::Mode_CD()
{
	//if (!_cdError)
	{
		std::memset(mciReturn, 0, sizeof(mciReturn));
		soundGlobs.mciErr = ::mciSendStringA("status cdaudio mode", mciReturn, sizeof(mciReturn), nullptr);
		if (!soundGlobs.mciErr) {
			if (mciReturn[0] != '\0' && ::_stricmp(mciReturn, "beef") != 0) {

				if (::_stricmp(mciReturn, "not ready") == 0) return Sound_CDMode::NotReady;
				if (::_stricmp(mciReturn, "paused")    == 0) return Sound_CDMode::Paused;
				if (::_stricmp(mciReturn, "playing")   == 0) return Sound_CDMode::Playing;
				if (::_stricmp(mciReturn, "stopped")   == 0) return Sound_CDMode::Stopped;

				return Sound_CDMode::None; // Default unhandled value
			}
			else {
				return Sound_CDMode::Error; // Music Fix dll
			}
		}

		ReportCDError();
	}
	return Sound_CDMode::Error;
}

sint32 Gods98::Count_CDTracks()
{
	//if (!_cdError)
	{
		std::memset(mciReturn, 0, sizeof(mciReturn));
		soundGlobs.mciErr = ::mciSendStringA("status cdaudio number of tracks", mciReturn, sizeof(mciReturn), nullptr);
		if (!soundGlobs.mciErr) {
			if (mciReturn[0] != '\0') {
				return std::atoi(mciReturn);
			}
			else {
				return -1; // Music Fix dll
			}
		}

		ReportCDError();
	}
	return -1;
}

sint32 Gods98::Current_CDTrack()
{
	//if (!_cdError)
	{
		std::memset(mciReturn, 0, sizeof(mciReturn));
		soundGlobs.mciErr = ::mciSendStringA("status cdaudio current track", mciReturn, sizeof(mciReturn), nullptr);
		if (!soundGlobs.mciErr) {
			if (mciReturn[0] != '\0') {
				return std::atoi(mciReturn); // Return 1-indexed number.
			}
			else {
				return -1; // Music Fix dll
			}
		}

		ReportCDError();
	}
	return -1;
}

#pragma endregion
