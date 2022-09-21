// DirectDraw.h : 
//
/// APIS: IDirect3DRMDevice3,
///       IDirect3D3,
///       IDirectDraw[14], IDirectDrawClipper, IDirectDrawPalette, IDirectDrawSurface4
/// DEPENDENCIES: Bmp, Files, Main, Maths, Utils, (Dxbug, Errors, Memory)
/// DEPENDENTS: Animation, Containers, Draw, Flic, Image, Main, Movie, Init,
///             Lego, Loader, NERPs, RadarMap

#pragma once

#include "../../platform/windows.h"

#include "../../common.h"
#include "../colour.h"
#include "../geometry.h"


/**********************************************************************************
 ******** Forward Global Namespace Declarations
 **********************************************************************************/

#pragma region Forward Declarations

struct IDirectDraw;
struct IDirectDraw4;
struct IDirectDrawSurface4;
struct IDirectDrawClipper;
struct _D3DDeviceDesc;
typedef struct _D3DDeviceDesc			D3DDEVICEDESC, * LPD3DDEVICEDESC;
struct _DDSURFACEDESC2;
typedef struct _DDSURFACEDESC2			DDSURFACEDESC2, FAR* LPDDSURFACEDESC2;
struct _DDPIXELFORMAT;
typedef struct _DDPIXELFORMAT			DDPIXELFORMAT, FAR* LPDDPIXELFORMAT;

#pragma endregion


namespace Gods98
{; // !<---

/**********************************************************************************
 ******** Forward Declarations
 **********************************************************************************/

#pragma region Forward Declarations

struct BMP_Image;        // from `engine/drawing/Bmp.h`
struct BMP_PaletteEntry; // from `engine/drawing/Bmp.h`

enum_scoped_forward(FileFlags) : uint32;
enum_scoped_forward_end(FileFlags);

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define GRAPHICS_MAXDRIVERS				20
#define GRAPHICS_MAXDEVICES				20
#define GRAPHICS_MAXMODES				200
#define GRAPHICS_DRIVERSTRINGLEN		256
#define GRAPHICS_DEVICESTRINGLEN		256
#define GRAPHICS_MODESTRINGLEN			256

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum Graphics_DriverFlags : uint32
{
	GRAPHICS_DRIVER_FLAG_NONE			= 0,

	GRAPHICS_DRIVER_FLAG_VALID			= 0x1,
	GRAPHICS_DRIVER_FLAG_PRIMARY		= 0x10,
	GRAPHICS_DRIVER_FLAG_WINDOWOK		= 0x20,
};
flags_end(Graphics_DriverFlags, 0x4);


enum Graphics_DeviceFlags : uint32
{
	GRAPHICS_DEVICE_FLAG_NONE			= 0,

	GRAPHICS_DEVICE_FLAG_VALID			= 0x1,
	GRAPHICS_DEVICE_FLAG_DEPTH8			= 0x10,
	GRAPHICS_DEVICE_FLAG_DEPTH16		= 0x20,
	GRAPHICS_DEVICE_FLAG_DEPTH24		= 0x40,
	GRAPHICS_DEVICE_FLAG_DEPTH32		= 0x80,
	GRAPHICS_DEVICE_FLAG_ZBUFF8			= 0x100,
	GRAPHICS_DEVICE_FLAG_ZBUFF16		= 0x200,
	GRAPHICS_DEVICE_FLAG_ZBUFF24		= 0x400,
	GRAPHICS_DEVICE_FLAG_ZBUFF32		= 0x800,
	GRAPHICS_DEVICE_FLAG_COLOUR			= 0x1000,
	GRAPHICS_DEVICE_FLAG_HARDWARE		= 0x2000,
	GRAPHICS_DEVICE_FLAG_VIDEOTEXTURE	= 0x4000,
	GRAPHICS_DEVICE_FLAG_SYSTEMTEXTURE	= 0x8000,
};
flags_end(Graphics_DeviceFlags, 0x4);


enum Graphics_ModeFlags : uint32
{
	GRAPHICS_MODE_FLAG_NONE				= 0,

	GRAPHICS_MODE_FLAG_VALID			= 0x1,
};
flags_end(Graphics_ModeFlags, 0x4);

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct Graphics_Driver
{
	/*000,10*/ GUID guid;
	/*010,100*/ char desc[GRAPHICS_DRIVERSTRINGLEN];
	/*110,4*/ Graphics_DriverFlags flags;
	/*114*/
};
assert_sizeof(Graphics_Driver, 0x114);


struct Graphics_Device
{
	/*000,10*/ GUID guid;
	/*010,100*/ char desc[GRAPHICS_DEVICESTRINGLEN];
	/*110,4*/ Graphics_DeviceFlags flags;
	/*114*/
};
assert_sizeof(Graphics_Device, 0x114);


struct Graphics_Mode
{
	/*000,4*/ uint32 width;
	/*004,4*/ uint32 height;
	/*008,4*/ uint32 bitDepth;
	/*00c,100*/ char desc[GRAPHICS_MODESTRINGLEN];
	/*10c,4*/ Graphics_ModeFlags flags;
	/*110*/
};
assert_sizeof(Graphics_Mode, 0x110);


struct DirectDraw_Globs
{
	// [globs: start]
	/*00,4*/ HWND hWnd;
	/*04,4*/ IDirectDraw4* lpDirectDraw;
	/*08,4*/ IDirectDrawSurface4* fSurf;
	/*0c,4*/ IDirectDrawSurface4* bSurf;
	/*10,4*/ IDirectDrawSurface4* zSurf;			// (unused)
	/*14,4*/ IDirectDrawClipper* lpFrontClipper;
	/*18,4*/ IDirectDrawClipper* lpBackClipper;
	/*1c,4*/ Graphics_Driver* driverList;
	/*20,4*/ Graphics_Driver* selectedDriver;		// (unused)
	/*24,4*/ Graphics_Device* deviceList;
	/*28,4*/ Graphics_Device* selectedDevice;		// (unused)
	/*2c,4*/ Graphics_Mode* modeList;
	/*30,4*/ uint32 driverCount;
	/*34,4*/ uint32 deviceCount;
	/*38,4*/ uint32 modeCount;
	/*3c,4*/ bool32 fullScreen;
	/*40,4*/ uint32 width;
	/*44,4*/ uint32 height;
	// [globs: end]
	/*48*/
};
assert_sizeof(DirectDraw_Globs, 0x48);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @0076bc80>
extern DirectDraw_Globs & directDrawGlobs;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @00406500>
IDirectDraw4* __cdecl noinline(DirectDraw)(void);
__inline IDirectDraw4* DirectDraw(void) { return directDrawGlobs.lpDirectDraw; }

// <LegoRR.exe @00406510>
IDirectDrawSurface4* __cdecl noinline(DirectDraw_bSurf)(void);
__inline IDirectDrawSurface4* DirectDraw_bSurf(void) { return directDrawGlobs.bSurf; }


// <unused>
__inline IDirectDrawSurface4* DirectDraw_fSurf(void) { return directDrawGlobs.fSurf; }

// <unused>
__inline bool32 DirectDraw_FullScreen(void) { return directDrawGlobs.fullScreen; }


// <LegoRR.exe @0047c430>
void __cdecl DirectDraw_Initialise(HWND hWnd);

// <LegoRR.exe @0047c480>
bool32 __cdecl DirectDraw_EnumDrivers(OUT Graphics_Driver* list, OUT uint32* count);

// <LegoRR.exe @0047c4b0>
BOOL __stdcall DirectDraw_EnumDriverCallback(GUID* lpGUID, char* lpDriverDescription, char* lpDriverName, void* lpContext);

// <LegoRR.exe @0047c5a0>
bool32 __cdecl DirectDraw_EnumDevices(const Graphics_Driver* driver, OUT Graphics_Device* list, OUT uint32* count);

// <LegoRR.exe @0047c640>
HRESULT __stdcall DirectDraw_EnumDeviceCallback(GUID* lpGuid, char* lpDeviceDescription,
												char* lpDeviceName, D3DDEVICEDESC* lpHWDesc,
												D3DDEVICEDESC* lpHELDesc, void* lpContext);

// <LegoRR.exe @0047c770>
bool32 __cdecl DirectDraw_EnumModes(const Graphics_Driver* driver, bool32 fullScreen, OUT Graphics_Mode* list, OUT uint32* count);

// <LegoRR.exe @0047c810>
HRESULT __stdcall DirectDraw_EnumModeCallback(DDSURFACEDESC2* lpDDSurfaceDesc, void* lpContext);

// <LegoRR.exe @0047c8d0>
bool32 __cdecl DirectDraw_Setup(bool32 fullscreen, const Graphics_Driver* driver, const Graphics_Device* device,
								const Graphics_Mode* mode, uint32 xPos, uint32 yPos, uint32 width, uint32 height);

// <LegoRR.exe @0047cb90>
void __cdecl DirectDraw_Flip(void);

// <LegoRR.exe @0047cbb0>
bool32 __cdecl DirectDraw_SaveBMP(IDirectDrawSurface4* surface, const char* fname);

/// CUSTOM: Support for FileFlags.
bool DirectDraw_SaveBMP2(IDirectDrawSurface4* surface, const char* fname, FileFlags fileFlags);

// <LegoRR.exe @0047cee0>
void __cdecl DirectDraw_ReturnFrontBuffer(void);

// <LegoRR.exe @0047cf10>
void __cdecl DirectDraw_BlitBuffers(void);

// <LegoRR.exe @0047cfb0>
void __cdecl DirectDraw_Shutdown(void);

// Adjust the texture usage for cards that don't like 8 bit textures...
// <LegoRR.exe @0047d010>
void __cdecl DirectDraw_AdjustTextureUsage(IN OUT uint32* textureUsage);

// <LegoRR.exe @0047d090>
bool32 __cdecl DirectDraw_GetAvailTextureMem(OUT uint32* total, OUT uint32* avail);

/// LEGACY: Use DirectDraw_ClearRGB or DirectDraw_ClearRGBF.
// <LegoRR.exe @0047d0e0>
void __cdecl DirectDraw_Clear(OPTIONAL const Area2F* window, ColourBGRAPacked bgrColour);

/// CUSTOM:
void DirectDraw_ClearRGBF(OPTIONAL const Area2F* window, real32 r, real32 g, real32 b, real32 a = 1.0f);

/// CUSTOM:
void DirectDraw_ClearRGB(OPTIONAL const Area2F* window, uint8 r, uint8 g, uint8 b, uint8 a = 255);

/// CUSTOM:
void DirectDraw_ClearSurfaceRGBF(IDirectDrawSurface4* surf, OPTIONAL const Area2F* window, real32 r, real32 g, real32 b, real32 a = 1.0f);

/// CUSTOM:
void DirectDraw_ClearSurfaceRGB(IDirectDrawSurface4* surf, OPTIONAL const Area2F* window, uint8 r, uint8 g, uint8 b, uint8 a = 255);

/// CUSTOM:
void _DirectDraw_ClearSurface(IDirectDrawSurface4* surf, OPTIONAL const Area2F* window, uint32 surfColour);

// <LegoRR.exe @0047d1a0>
bool32 __cdecl DirectDraw_CreateClipper(bool32 fullscreen, uint32 width, uint32 height);

/// LEGACY: Use DirectDraw_CopySurface.
// Original Name: DirectDraw_Blt8To16
// <LegoRR.exe @0047d2c0>
void __cdecl DirectDraw_Blt8ToSurface(IDirectDrawSurface4* target, IDirectDrawSurface4* source, const BMP_PaletteEntry* palette);

/// CUSTOM:
void DirectDraw_FlipSurface(void* buffer, uint32 height, uint32 pitch);

/// CUSTOM:
void DirectDraw_FlipSurface(const BMP_Image* image);

/// CUSTOM:
void DirectDraw_FlipSurface(const DDSURFACEDESC2* desc);

/// CUSTOM:
bool DirectDraw_CopySurface(const BMP_Image* dstImage, const BMP_Image* srcImage, bool flip);

/// CUSTOM:
bool DirectDraw_CopySurface(const BMP_Image* dstImage, const DDSURFACEDESC2* srcDesc, bool flip, OPTIONAL const BMP_PaletteEntry* srcPalette = nullptr);

/// CUSTOM:
bool DirectDraw_CopySurface(const DDSURFACEDESC2* dstDesc, const BMP_Image* srcImage, bool flip);

/// CUSTOM:
bool DirectDraw_CopySurface(const DDSURFACEDESC2* dstDesc, const DDSURFACEDESC2* srcDesc, bool flip, OPTIONAL const BMP_PaletteEntry* srcPalette = nullptr);

/// CUSTOM:
bool DirectDraw_CopySurface(IDirectDrawSurface4* dstSurf, IDirectDrawSurface4* srcSurf, bool flip, OPTIONAL const BMP_PaletteEntry* srcPalette = nullptr);

/// CUSTOM:
void DirectDraw_GetSurfaceInfo(const DDSURFACEDESC2* desc, OUT BMP_Image* image, OPTIONAL const BMP_PaletteEntry* palette = nullptr);

/// LEGACY: Use DirectDraw_ToColourFromRGB.
// <LegoRR.exe @0047d590>
uint32 __cdecl DirectDraw_GetColour(IDirectDrawSurface4* surf, ColourBGRAPacked bgrColour);

/// CUSTOM: Converts real RGB values to a surface colour value.
uint32 DirectDraw_ToColourFromRGBF(IDirectDrawSurface4* surf, real32 r, real32 g, real32 b, real32 a = 1.0f);

/// CUSTOM: Converts byte RGB values to a surface colour value.
uint32 DirectDraw_ToColourFromRGB(IDirectDrawSurface4* surf, uint8 r, uint8 g, uint8 b, uint8 a = 255);

/// CUSTOM: Converts a surface colour value to real RGB values.
bool DirectDraw_FromColourToRGBF(IDirectDrawSurface4* surf, uint32 surfColour, OPTIONAL OUT real32* r, OPTIONAL OUT real32* g, OPTIONAL OUT real32* b, OPTIONAL OUT real32* a = nullptr);

/// CUSTOM: Converts a surface colour value to byte RGB values.
bool DirectDraw_FromColourToRGB(IDirectDrawSurface4* surf, uint32 surfColour, OPTIONAL OUT uint8* r, OPTIONAL OUT uint8* g, OPTIONAL OUT uint8* b, OPTIONAL OUT uint8* a = nullptr);

/// CUSTOM:
bool DirectDraw_GetPaletteEntries(IDirectDrawSurface4* surf, OUT BMP_PaletteEntry* palette, uint32 index = 0, uint32 count = 256);


// <LegoRR.exe @0047d6b0>
uint32 __cdecl DirectDraw_CountMaskBits(uint32 mask);

/// CUSTOM: Replacement for Image_CountMaskBitShift.
uint32 __cdecl DirectDraw_CountMaskBitShift(uint32 mask);

/// CUSTOM: Gets the bit depth selected during DirectDraw_Setup.
uint32 DirectDraw_BitDepth();

/// NOTE: This expects low,high to be in COLORREF format (from lowest to highest bit order: RGBA).
/// CUSTOM: Expand a colour key to a 16-bit range, to include colours that normally don't match when in 24-bit or 32-bit mode.
void DirectDraw_ColourKeyTo16BitRange(IN OUT uint8* lowr,  IN OUT uint8* lowg,  IN OUT uint8* lowb,
									  IN OUT uint8* highr, IN OUT uint8* highg, IN OUT uint8* highb);



/// CUSTOM: Shorthand to get number of bytes in a bit depth.
constexpr uint32 DirectDraw_BitToByteDepth(uint32 bitDepth)
{
	return (bitDepth + 7) / 8;
}

/// CUSTOM: Gets the mask for a bit count with optional shift.
constexpr uint32 DirectDraw_BitCountToMask(uint32 bitCount, uint32 bitShift = 0)
{
	// Note: Left shifting as many bits as the type size is undefined behaviour.
	if (bitShift >= (sizeof(uint32) * 8)) return 0;
	return (((bitCount >= (sizeof(uint32) * 8)) ? 0u : (1u << bitCount)) - 1) << bitShift;
}

/// CUSTOM: Converts a singel RGBA channel value to part of a pixel colour value.
constexpr uint32 DirectDraw_ShiftChannelByte(uint8 value, uint32 bitCount, uint32 bitShift)
{
	// Note: Left shifting as many bits as the type size is undefined behaviour.
	if (bitShift >= (sizeof(uint32) * 8)) return 0;
	return ((value >> (8 - bitCount)) << bitShift);
}

/// CUSTOM: Extracts a single RGBA channel value from a pixel colour value.
constexpr uint8 DirectDraw_UnshiftChannelByte(uint32 value, uint32 bitCount, uint32 bitShift)
{
	return (((value >> bitShift) & DirectDraw_BitCountToMask(bitCount)) << (8 - bitCount));
}

/// CUSTOM: Returns the address offset to a pixel from the start of the buffer.
constexpr void* DirectDraw_PixelAddress(void* buffer, uint32 pitch, uint32 bitDepth, uint32 x, uint32 y)
{
	return (static_cast<uint8*>(buffer) + (y * pitch) + (x * DirectDraw_BitToByteDepth(bitDepth)));
}

/// CUSTOM: Returns the address offset to a pixel from the start of the buffer.
constexpr const void* DirectDraw_PixelAddress(const void* buffer, uint32 pitch, uint32 bitDepth, uint32 x, uint32 y)
{
	return DirectDraw_PixelAddress(const_cast<void*>(buffer), pitch, bitDepth, x, y);
}


// <inlined>
inline bool32 DirectDraw_SetupFullScreen(const Graphics_Driver* driver, const Graphics_Device* device, const Graphics_Mode* mode)
{
	return DirectDraw_Setup(true, driver, device, mode, 0, 0, 320, 200);
}

// <inlined>
inline bool32 DirectDraw_SetupWindowed(const Graphics_Device* device, uint32 xPos, uint32 yPos, uint32 width, uint32 height)
{
	return DirectDraw_Setup(false, nullptr, device, nullptr, xPos, yPos, width, height);
}

#pragma endregion

}
