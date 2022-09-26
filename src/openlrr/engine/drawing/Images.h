// Images.h : 
//
/// APIS: IDirectDraw4, IDirectDrawSurface4
/// DEPENDENCIES: Bmp, DirectDraw, Files, (Dxbug, Errors, Memory)
/// DEPENDENTS: Fonts, Bubble, FrontEnd, HelpWindow, Info, Interface, Lego, Loader,
///             NERPs, Objective, ObjInfo, Panel, Pointer, Priorities, Rewards,
///             ScrollInfo, Text, ToolTip

#pragma once

#include "../../common.h"
#include "../colour.h"
#include "../geometry.h"
#include "../core/ListSet.hpp"


/**********************************************************************************
 ******** Forward Global Namespace Declarations
 **********************************************************************************/

#pragma region Forward Declarations

struct _D3DRMIMAGE;
typedef struct _D3DRMIMAGE D3DRMIMAGE;
struct IDirectDraw4;
struct IDirectDrawSurface4;
struct _DDSURFACEDESC2;
typedef struct _DDSURFACEDESC2 DDSURFACEDESC2;
struct tagPALETTEENTRY;
typedef struct tagPALETTEENTRY PALETTEENTRY;
typedef unsigned long COLORREF;
//struct PALETTEENTRY;
//enum D3DRENDERSTATETYPE : uint32;

#pragma endregion


namespace Gods98
{; // !<---

/**********************************************************************************
 ******** Forward Declarations
 **********************************************************************************/

#pragma region Forward Declarations

struct BMP_PaletteEntry; // from `engine/drawing/Bmp.h`
struct BMP_Image;        // from `engine/drawing/Bmp.h`

enum_scoped_forward(FileFlags) : uint32;
enum_scoped_forward_end(FileFlags);

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define IMAGE_MAXLISTS			32			// 2^32 - 1 possible images...

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

// used by `Image_DisplayScaled2` for
//  `IDirect3DDevice3->DrawPrimitive(D3DPT_TRIANGLESTRIP, IMAGE_VERTEXFLAGS, vertices, 4, D3DDP_DONOTLIGHT|D3DDP_WAIT)`
#define IMAGE_VERTEXFLAGS		(D3DFVF_DIFFUSE|D3DFVF_XYZRHW|D3DFVF_TEX1)


flags_scoped(Image_GlobFlags) : uint32
{
	IMAGE_GLOB_FLAG_NONE        = 0,
	IMAGE_GLOB_FLAG_INITIALISED = 0x1,
};
flags_scoped_end(Image_GlobFlags, 0x4);


flags_scoped(ImageFlags) : uint32
{
	IMAGE_FLAG_NONE    = 0,

	IMAGE_FLAG_TRANS   = 0x2,
	IMAGE_FLAG_TEXTURE = 0x4,

	/// CUSTOM:
	IMAGE_FLAG_FROMPALETTE = 0x80000000,
};
flags_scoped_end(ImageFlags, 0x4);


enum class Image_TextureMode : sint32
{
	Normal            = 0,
	Subtractive       = 1,
	Additive          = 2,
	Transparent       = 3,

	Fixed_Subtractive = 4,	// Not affected by the global intensity value
	Fixed_Additive    = 5,
	Fixed_Transparent = 6,

	Count = 7,
};
assert_sizeof(Image_TextureMode, 0x4);

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct ImageVertex
{
	/*00,10*/ Vector4F position;
	/*10,4*/ uint32 colour;
	/*14,4*/ real32 tu;
	/*18,4*/ real32 tv;
	/*1c*/
};
assert_sizeof(ImageVertex, 0x1c);


struct Image
{
	/*00,4*/ IDirectDrawSurface4* surface;
	//IDirect3DTexture2* texture;
	//D3DTEXTUREHANDLE textureHandle;
	/*04,4*/ uint32 width;
	/*08,4*/ uint32 height;
	/*0c,4*/ uint32 penZero; // Surface colour.
	/*10,4*/ uint32 pen255;  // Surface colour.
	/*14,4*/ ColourRGBAPacked penZeroRGB;
	/*18,4*/ ImageFlags flags;
	/*1c,4*/ Image *nextFree;
	/*20*/
};
assert_sizeof(Image, 0x20);


struct Image_Globs
{
	// [globs: start]
	/*00,80*/ Image* listSet[IMAGE_MAXLISTS];		// Images list
	/*80,4*/ Image* freeList;
	/*84,4*/ uint32 listCount;						// number of lists.
	/*88,4*/ Image_GlobFlags flags;
	// [globs: end]
	/*8c*/
};
assert_sizeof(Image_Globs, 0x8c);


using Image_ListSet = ListSet::WrapperCollection<Gods98::Image_Globs>;

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00534908>
extern Image_Globs & imageGlobs;

extern Image_ListSet imageListSet;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

/// CUSTOM: Gets if the Images module rendering is enabled.
bool Image_IsRenderEnabled();

/// CUSTOM: Sets if the Images module rendering is enabled. For testing performance.
void Image_SetRenderEnabled(bool enabled);


// <LegoRR.exe @0047d6d0>
void __cdecl Image_Initialise(void);

// <LegoRR.exe @0047d6f0>
void __cdecl Image_Shutdown(void);

// <LegoRR.exe @0047d730>
void __cdecl Image_Remove(Image* dead);

// <LegoRR.exe @0047d750>
bool32 __cdecl Image_CopyToDataToSurface(IDirectDrawSurface4* surface, const BMP_Image* image);

/// LEGACY: Use DirectDraw_CopySurface.
// <LegoRR.exe @0047d7e0>
bool32 __cdecl Image_8BitSourceCopy(const DDSURFACEDESC2* desc, const BMP_Image* image);

/// LEGACY: Use DirectDraw_CountMaskBits.
// <LegoRR.exe @0047d9c0>
uint32 __cdecl Image_CountMaskBits(uint32 mask);

/// LEGACY: Use DirectDraw_CountMaskBitShift.
// <LegoRR.exe @0047d9e0>
uint32 __cdecl Image_CountMaskBitShift(uint32 mask);

/// LEGACY: Use DirectDraw_FlipSurface.
// <LegoRR.exe @0047da00>
void __cdecl Image_FlipSurface(const DDSURFACEDESC2* desc);

/// LEGACY: Use DirectDraw_CopySurface.
// <LegoRR.exe @0047dac0>
bool32 __cdecl Image_24BitSourceCopy(const DDSURFACEDESC2* desc, const BMP_Image* image);

// <LegoRR.exe @0047dc90>
Image* __cdecl Image_LoadBMPScaled(const char* fileName, uint32 width, uint32 height);

/// CUSTOM: Support for FileFlags.
Image* Image_LoadBMPScaled2(const char* fileName, uint32 width, uint32 height, FileFlags fileFlags);

/// CUSTOM: Create an empty image without needing to load a file.
Image* Image_CreateNew(uint32 width, uint32 height);

/// CUSTOM:
void Image_ClearRGBF(Image* image, OPTIONAL const Area2F* window, real32 r, real32 g, real32 b, real32 a = 1.0f);

/// CUSTOM:
void Image_ClearRGB(Image* image, OPTIONAL const Area2F* window, uint8 r, uint8 g, uint8 b, uint8 a = 255);


// <LegoRR.exe @0047de50>
ColourRGBAPacked __cdecl Image_RGB2CR(uint8 r, uint8 g, uint8 b);

/// CUSTOM:
ColourRGBAPacked __cdecl Image_RGBA2CR(uint8 r, uint8 g, uint8 b, uint8 a);

/// CUSTOM: Core functionality that Image_SetPenZeroTrans and Image_SetupTrans wraps around.
void _Image_SetupTrans(Image* image, uint32 low, uint32 high, bool truncateTo16Bit);

// <LegoRR.exe @0047de80>
void __cdecl Image_SetPenZeroTrans(Image* image);

// <LegoRR.exe @0047deb0>
void __cdecl Image_SetupTrans(Image* image, real32 lowr, real32 lowg, real32 lowb, real32 highr, real32 highg, real32 highb);

/// CUSTOM:
void Image_SetupTrans2(Image* image, real32 lowr, real32 lowg, real32 lowb, real32 highr, real32 highg, real32 highb, bool truncateTo16Bit);

// <LegoRR.exe @0047df70>
void __cdecl Image_DisplayScaled(Image* image, OPTIONAL const Area2F* src, OPTIONAL const Point2F* destPos, OPTIONAL const Point2F* destSize);

/// CUSTOM:
void Image_DisplayScaled2(Image* image, OPTIONAL const Area2F* src, OPTIONAL const Point2F* destPos,
						  OPTIONAL const Point2F* destSize, sint32 drawScale, bool forceRender = false);

// <LegoRR.exe @0047e120>
void* __cdecl Image_LockSurface(Image* image, OUT uint32* pitch, OUT uint32* bpp);

// <LegoRR.exe @0047e190>
void __cdecl Image_UnlockSurface(Image* image);

/// LEGACY: Previously named Image_GetPen255. Converts Palette Entry 255 to big endian. Don't use.
// <LegoRR.exe @0047e1b0>
uint32 __cdecl Image_GetPen255BigEndian(Image* image);

/// LEGACY: Previously named Image_GetPixelMask. Converts pixel mask to big endian. Don't use.
// <LegoRR.exe @0047e210>
uint32 __cdecl Image_GetPixelMaskBigEndian(Image* image);

/// CUSTOM: Replacement for Image_GetPenZero. Returns colour as it is stored in the surface.
uint32 Image_GetPaletteEntry0(Image* image);

/// CUSTOM: Replacement for Image_GetPen255BigEndian. Returns colour as it is stored in the surface.
uint32 Image_GetPaletteEntry255(Image* image);

/// CUSTOM: Replacement for Image_GetPixelMaskBigEndian. Returns pixel mask as it is stored in the surface.
uint32 Image_GetPixelMask(Image* image);

// Returns pixel as it is stored in the surface.
// <LegoRR.exe @0047e260>
bool32 __cdecl Image_GetPixel(Image* image, uint32 x, uint32 y, OUT uint32* colour);

// REPLACEMENT FOR: Image_GetPixel, because the functions that use GetPixel are checking specifically for black (0).
// <LegoRR.exe @0047e260>
bool32 __cdecl Image_GetPixelTruncate(Image* image, uint32 x, uint32 y, OUT uint32* colour);

// <LegoRR.exe @0047e310>
Image* __cdecl Image_Create(IDirectDrawSurface4* surface, uint32 width, uint32 height, ColourRGBAPacked penZero, ColourRGBAPacked pen255);

/// LEGACY:
// <LegoRR.exe @0047e380>
void __cdecl Image_AddList(void);

// <LegoRR.exe @0047e3f0>
void __cdecl Image_RemoveAll(void);

// <LegoRR.exe @0047e450>
uint32 __cdecl Image_DDColorMatch(IDirectDrawSurface4* surface, ColourRGBAPacked cr);

// <LegoRR.exe @0047e590>
void __cdecl Image_CR2RGB(ColourRGBAPacked cr, OPTIONAL OUT uint8* r, OPTIONAL OUT uint8* g, OPTIONAL OUT uint8* b);

/// CUSTOM:
void __cdecl Image_CR2RGBA(ColourRGBAPacked cr, OPTIONAL OUT uint8* r, OPTIONAL OUT uint8* g, OPTIONAL OUT uint8* b, OPTIONAL OUT uint8* a);

// <LegoRR.exe @0047e5c0>
void __cdecl Image_GetScreenshot(OUT Image* image, uint32 xsize, uint32 ysize);


// <LegoRR.exe @0047e6a0>
void __cdecl Image_InitFromSurface(Image* newImage, IDirectDrawSurface4* surface, uint32 width, uint32 height,
								   ColourRGBAPacked penZero, ColourRGBAPacked pen255);

// <LegoRR.exe @0047e700>
bool32 __cdecl Image_SaveBMP(Image* image, const char* fname);

/// CUSTOM: Support for FileFlags.
bool Image_SaveBMP2(Image* image, const char* fname, FileFlags fileFlags);


// <missing>
//void __cdecl Image_GetPenZero(const Image* image, OPTIONAL OUT real32* r, OPTIONAL OUT real32* g, OPTIONAL OUT real32* b);


/// CUSTOM: Gets the surface of the image.
IDirectDrawSurface4* Image_GetSurface(Image* image);

/// CUSTOM: Find a valid transparency colour for the image that isn't in the list.
bool Image_FindTransColour(Image* image, const ColourRGBF* colourList, uint32 colourCount, OUT ColourRGBF* transColour);

/*Image* __cdecl Image_LoadBMPTexture(const char* filename);
void __cdecl Image_SetMainViewport(Viewport* vp);
void __cdecl Image_SetAlphaIntensity(real32 level);
void __cdecl Image_DisplayScaled2(Image* image, const Area2F* src, const Point2F* destPos, const Point2F* destSize, Image_TextureMode textureMode, real32 level, const Point2F* uvs);
void __cdecl Image_DisplayScaled(Image* image, const Area2F* src, const Point2F* destPos, const Point2F* destSize);
void* __cdecl Image_LockSurface(Image* image, uint32* pitch, uint32* bpp);
void __cdecl Image_UnlockSurface(Image* image);
int __cdecl Image_CopyBMP(IDirectDrawSurface4* surface, void* hbm, uint32 x, uint32 y, uint32 dx, uint32 dy);

uint32 __cdecl Image_RGB2CR(unsigned char r, unsigned char g, unsigned char b);
void __cdecl Image_MyBlt(IDirectDrawSurface* dest, IDirectDrawSurface* src, uint32 sx, uint32 sy);
*/

#ifdef DEBUG
	#define Image_CheckInit()			if (!(imageGlobs.flags & Gods98::Image_GlobFlags::IMAGE_GLOB_FLAG_INITIALISED)) Gods98::Error_Fatal(true, "Error: Image_Intitialise() Has Not Been Called");
#else
	#define Image_CheckInit()
#endif

#define Image_LoadBMP(n)					Image_LoadBMPScaled((n),0,0)
#define Image_Display(p,l)					Image_DisplayScaled((p),nullptr,(l),nullptr)

#define Image_SetupTransBlack(p)			Image_SetupTrans((p),0.0f,0.0f,0.0f,0.0f,0.0f,0.0f)

#define Image_SetupTransBlack2(p,trunc16)	Image_SetupTrans2((p),0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,(trunc16))


// <inlined>
__inline sint32 Image_GetWidth(const Image* p) { return p->width; }

// <inlined>
__inline sint32 Image_GetHeight(const Image* p) { return p->height; }

#pragma endregion

}
