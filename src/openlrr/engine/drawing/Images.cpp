// Images.cpp : 
//

#include "../../platform/ddraw.h"
#include "../../platform/d3drm.h"

#include "../core/Errors.h"
#include "../core/Files.h"
#include "../core/Memory.h"
#include "../util/Dxbug.h"
#include "../Main.h"
#include "Bmp.h"
#include "DirectDraw.h"
#include "Draw.h"

#include "Images.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00534908>
Gods98::Image_Globs & Gods98::imageGlobs = *(Gods98::Image_Globs*)0x00534908;

Gods98::Image_ListSet Gods98::imageListSet = Gods98::Image_ListSet(Gods98::imageGlobs);

/// CUSTOM: If false, then the Fonts module will not render images at all.
static bool _renderEnabled = true;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

/// CUSTOM: Gets if the Images module rendering is enabled.
bool Gods98::Image_IsRenderEnabled()
{
	return _renderEnabled;
}

/// CUSTOM: Sets if the Images module rendering is enabled. For testing performance.
void Gods98::Image_SetRenderEnabled(bool enabled)
{
	_renderEnabled = enabled;
}


// <LegoRR.exe @0047d6d0>
void __cdecl Gods98::Image_Initialise(void)
{
	log_firstcall();

	if (imageGlobs.flags & Image_GlobFlags::IMAGE_GLOB_FLAG_INITIALISED) Error_Fatal(true, "Images already initialised");

	imageListSet.Initialise();
	imageGlobs.flags = Image_GlobFlags::IMAGE_GLOB_FLAG_INITIALISED;
}

// <LegoRR.exe @0047d6f0>
void __cdecl Gods98::Image_Shutdown(void)
{
	log_firstcall();

	Image_RemoveAll();

	imageListSet.Shutdown();
	imageGlobs.flags = Image_GlobFlags::IMAGE_GLOB_FLAG_NONE;
}

// <LegoRR.exe @0047d730>
void __cdecl Gods98::Image_Remove(Image* dead)
{
	log_firstcall();

	Image_CheckInit();
	Error_Fatal(!dead, "NULL passed to Image_Remove()");

	dead->surface->Release();

	imageListSet.Remove(dead);
}

// <LegoRR.exe @0047d750>
bool32 __cdecl Gods98::Image_CopyToDataToSurface(IDirectDrawSurface4* surface, const BMP_Image* image)
{
	log_firstcall();

	DDSURFACEDESC2 desc = { 0 };
	desc.dwSize = sizeof(desc);

	if (surface->Lock(nullptr, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, nullptr) == DD_OK) {

		DirectDraw_CopySurface(&desc, image, true);

		surface->Unlock(nullptr);

		return true;
	}
	else Error_Warn(true, "Could not lock surface");

	return false;
}

// <LegoRR.exe @0047d7e0>
bool32 __cdecl Gods98::Image_8BitSourceCopy(const DDSURFACEDESC2* desc, const BMP_Image* image)
{
	return DirectDraw_CopySurface(desc, image, true);
}

// <LegoRR.exe @0047d9c0>
uint32 __cdecl Gods98::Image_CountMaskBits(uint32 mask)
{
	return DirectDraw_CountMaskBits(mask);
}

// <LegoRR.exe @0047d9e0>
uint32 __cdecl Gods98::Image_CountMaskBitShift(uint32 mask)
{
	return DirectDraw_CountMaskBitShift(mask);
}

// <LegoRR.exe @0047da00>
void __cdecl Gods98::Image_FlipSurface(const DDSURFACEDESC2* desc)
{
	DirectDraw_FlipSurface(desc);
}

// <LegoRR.exe @0047dac0>
bool32 __cdecl Gods98::Image_24BitSourceCopy(const DDSURFACEDESC2* desc, const BMP_Image* image)
{
	return DirectDraw_CopySurface(desc, image, true);
}

// <LegoRR.exe @0047dc90>
Gods98::Image* __cdecl Gods98::Image_LoadBMPScaled(const char* fileName, uint32 width, uint32 height)
{
	return Image_LoadBMPScaled2(fileName, width, height, FileFlags::FILE_FLAGS_DEFAULT);
}

/// CUSTOM: Support for FileFlags.
Gods98::Image* Gods98::Image_LoadBMPScaled2(const char* fileName, uint32 width, uint32 height, FileFlags fileFlags)
{
	log_firstcall();

	uint32 fileSize;
	void* buffer = File_LoadBinary2(fileName, &fileSize, fileFlags);
	if (buffer == nullptr || fileSize == 0)
		return nullptr; // Failed to load file, or file is invalid.


	ColourRGBAPacked penZero, pen255;
	penZero = pen255 = Image_RGB2CR(0, 0, 0);

	BMP_Image image = { 0 };  // D3DRMIMAGE
	BMP_Parse(buffer, fileSize, &image);
	if (!image.rgb) {
		penZero = Image_RGB2CR(image.palette[0].red, image.palette[0].green, image.palette[0].blue);
		pen255 = Image_RGB2CR(image.palette[255].red, image.palette[255].green, image.palette[255].blue);
	}
	BMP_SetupChannelMasks(&image, true); // BMP images treat 16-bit as 15-bit.

	DDSURFACEDESC2 ddsd = { 0 };
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd.dwWidth = image.width;
	ddsd.dwHeight = image.height;

	IDirectDrawSurface4* surface = nullptr;
	if (DirectDraw()->CreateSurface(&ddsd, &surface, nullptr) == DD_OK) {

		if (Image_CopyToDataToSurface(surface, &image)) {

			Image* newImage;
			if (newImage = Image_Create(surface, image.width, image.height, penZero, pen255)) {
				if (!image.rgb) newImage->flags |= ImageFlags::IMAGE_FLAG_FROMPALETTE;
				BMP_Cleanup(&image);

				if (buffer) Mem_Free(buffer);
				return newImage;
			}
			else Error_Warn(true, "Could not create image");
			
		}
		else Error_Warn(true, "Could not copy image to surface");

	}
	else Error_Warn(true, "Could not create surface");

	if (surface) surface->Release();
	BMP_Cleanup(&image);
	if (buffer) Mem_Free(buffer);
	return nullptr;
}

/// CUSTOM: Create an empty image without needing to load a file.
Gods98::Image* Gods98::Image_CreateNew(uint32 width, uint32 height)
{
	ColourRGBAPacked penZero, pen255;
	penZero = pen255 = Image_RGB2CR(0, 0, 0);

	DDSURFACEDESC2 ddsd = { 0 };
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;

	IDirectDrawSurface4* surface = nullptr;
	if (DirectDraw()->CreateSurface(&ddsd, &surface, nullptr) == DD_OK) {

		Image* newImage;
		if (newImage = Image_Create(surface, width, height, penZero, pen255)) {
			Image_ClearRGB(newImage, nullptr, penZero.red, penZero.green, penZero.blue, penZero.alpha);
			return newImage;
		}
		else Error_Warn(true, "Could not create image");

	}
	else Error_Warn(true, "Could not create surface");

	if (surface) surface->Release();
	return nullptr;
}

/// CUSTOM:
void Gods98::Image_ClearRGBF(Image* image, OPTIONAL const Area2F* window, real32 r, real32 g, real32 b, real32 a)
{
	DirectDraw_ClearSurfaceRGBF(image->surface, window, r, g, b, a);
}

/// CUSTOM:
void Gods98::Image_ClearRGB(Image* image, OPTIONAL const Area2F* window, uint8 r, uint8 g, uint8 b, uint8 a)
{
	DirectDraw_ClearSurfaceRGB(image->surface, window, r, g, b, a);
}

// <LegoRR.exe @0047de50>
ColourRGBAPacked __cdecl Gods98::Image_RGB2CR(uint8 r, uint8 g, uint8 b)
{
	return Image_RGBA2CR(r, g, b, 255);
}

/// CUSTOM:
ColourRGBAPacked __cdecl Gods98::Image_RGBA2CR(uint8 r, uint8 g, uint8 b, uint8 a)
{
	log_firstcall();

	ColourRGBAPacked colour = { 0 }; // dummy init
	colour.red   = r;
	colour.green = g;
	colour.blue  = b;
	colour.alpha = a;
	return colour;//.rgbaColour;
}

void Gods98::_Image_SetupTrans(Gods98::Image* image, uint32 surfLow, uint32 surfHigh, bool truncateTo16Bit)
{
	log_firstcall();

	Error_Fatal(!image, "_Image_SetupTrans: NULL passed as image");

	//Error_Warn(low != high, "_Image_SetupTrans: Differing low and high transparency colours not supported");
	//high = low; // Since this is only a warning, we at least want to follow through with the low range.

	//static uint32 transCount = 0;
	//static uint32 transMutateCount = 0;
	//static uint32 transApproxCount = 0;
	//transCount++;

	bool approxTrans = false;

	// When transitioning to True Colour support, many images didn't have an exact match for transparency anymore.
	// Some non-paletted and even paletted images were slightly off on colour, just small enough that it would be
	// ignored when truncated to 16-bit colour. We need to manually correct this in True Colour mode.
	// Especially because SetColourKey doesn't actually support ranges. So along with True Colour mode,
	//  force a conversion when the passed low and high colour values don't match.
	if (surfLow != surfHigh || DirectDraw_BitDepth() > 16) {

		uint8 lowr, lowg, lowb, highr, highg, highb;
		DirectDraw_FromColourToRGB(image->surface, surfLow,  &lowr,  &lowg,  &lowb);
		DirectDraw_FromColourToRGB(image->surface, surfHigh, &highr, &highg, &highb);

		const uint8 exactr = lowr, exactg = lowg, exactb = lowb;

		if (DirectDraw_BitDepth() > 16) {
			DirectDraw_ColourKeyTo16BitRange(&lowr, &lowg, &lowb, &highr, &highg, &highb);
		}

		DDSURFACEDESC2 desc = { 0 };
		desc.dwSize = sizeof(DDSURFACEDESC2);
		if (image->surface->Lock(nullptr, &desc, DDLOCK_WAIT, nullptr) == DD_OK) {

			//transMutateCount++;

			const uint32 rMask = desc.ddpfPixelFormat.dwRBitMask;
			const uint32 gMask = desc.ddpfPixelFormat.dwGBitMask;
			const uint32 bMask = desc.ddpfPixelFormat.dwBBitMask;
			const uint32 aMask = desc.ddpfPixelFormat.dwRGBAlphaBitMask;

			const uint32 rBitCount = DirectDraw_CountMaskBits(rMask);
			const uint32 gBitCount = DirectDraw_CountMaskBits(gMask);
			const uint32 bBitCount = DirectDraw_CountMaskBits(bMask);
			const uint32 aBitCount = DirectDraw_CountMaskBits(aMask);

			const uint32 rBitShift = DirectDraw_CountMaskBitShift(rMask);
			const uint32 gBitShift = DirectDraw_CountMaskBitShift(gMask);
			const uint32 bBitShift = DirectDraw_CountMaskBitShift(bMask);
			const uint32 aBitShift = DirectDraw_CountMaskBitShift(aMask);

			const uint32 pitch = desc.lPitch;

			const uint32 bitDepth = desc.ddpfPixelFormat.dwRGBBitCount;
			const uint32 byteDepth = DirectDraw_BitToByteDepth(bitDepth);

			uint8* data = static_cast<uint8*>(desc.lpSurface);

			for (uint32 y = 0; y < image->height; y++) {
				for (uint32 x = 0; x < image->width; x++) {
					uint32 srcValue = 0;
					for (uint32 k = 0; k < bitDepth; k += 8) {
						srcValue |= (data[(k/8)] << k);
					}

					const uint8 r = DirectDraw_UnshiftChannelByte(srcValue, rBitCount, rBitShift);
					const uint8 g = DirectDraw_UnshiftChannelByte(srcValue, gBitCount, gBitShift);
					const uint8 b = DirectDraw_UnshiftChannelByte(srcValue, bBitCount, bBitShift);
					const uint32 aValue = srcValue & aMask;

					if (r >= lowr && r <= highr && g >= lowg && g <= highg && b >= lowb && b <= highb) {
							
						if (r != exactr || g != exactg || b != exactb) {
							approxTrans = true;
							const uint32 dstValue =
								DirectDraw_ShiftChannelByte(exactr, rBitCount, rBitShift) |
								DirectDraw_ShiftChannelByte(exactg, gBitCount, gBitShift) |
								DirectDraw_ShiftChannelByte(exactb, bBitCount, bBitShift) |
								aValue;
							for (uint32 k = 0; k < bitDepth; k += 8) {
								data[(k/8)] = static_cast<uint8>(dstValue >> k);
							}
						}
					}
					data += byteDepth;
				}
				data += pitch - (image->width * byteDepth);
			}

			image->surface->Unlock(nullptr);
		}
	}

	//if (approxTrans)
	//	transApproxCount++;

	//Error_InfoF("_Image_SetupTrans: %i (checked=%i, approx=%i)", transCount, transMutateCount, transApproxCount);
	
	// Ranges aren't actually supported, so only pass low,low instead of low,high.
	DDCOLORKEY colourKey = { surfLow, surfLow };
	image->surface->SetColorKey(DDCKEY_SRCBLT, &colourKey);
	image->flags |= ImageFlags::IMAGE_FLAG_TRANS;
}

// <LegoRR.exe @0047de80>
void __cdecl Gods98::Image_SetPenZeroTrans(Image* image)
{
	log_firstcall();

	_Image_SetupTrans(image, image->penZero, image->penZero, true); // Truncate colour range to 16-bit.
}

// <LegoRR.exe @0047deb0>
void __cdecl Gods98::Image_SetupTrans(Image* image, real32 lowr, real32 lowg, real32 lowb, real32 highr, real32 highg, real32 highb)
{
	log_firstcall();

	const uint32 surfLow  = DirectDraw_ToColourFromRGBF(image->surface, lowr,  lowg,  lowb);
	const uint32 surfHigh = DirectDraw_ToColourFromRGBF(image->surface, highr, highg, highb);

	_Image_SetupTrans(image, surfLow, surfHigh, true); // Truncate colour range to 16-bit.
}

// <LegoRR.exe @0047df70>
void __cdecl Gods98::Image_DisplayScaled(Image* image, OPTIONAL const Area2F* src,
										 OPTIONAL const Point2F* destPos, OPTIONAL const Point2F* destSize)
{
	// function taken from: `if (!(image->flags & ImageFlags::IMAGE_FLAG_TEXTURE))` of `Image_DisplayScaled2`

	log_firstcall();

	Error_Fatal(!image, "NULL passed as image to Image_DisplayScaled()");

	RECT r_src = { 0 }, r_dst = { 0 }; // dummy inits
	Point2F dummy = { 0.0f }; // Dummy to assign destPos to when using Main_Scale.

	if (src) {
		r_src.left = (sint32)src->x;
		r_src.top  = (sint32)src->y;
		r_src.right  = (sint32)(src->x + src->width);
		r_src.bottom = (sint32)(src->y + src->height);
	}

	if (destPos) {
/*
		if (destPos->x < 0) destPos->x = appWidth()  + destPos->x;
		if (destPos->y < 0) destPos->y = appHeight() + destPos->y;
*/
		//r_dst.left = (uint32)destPos->x;
		//r_dst.top  = (uint32)destPos->y;
		r_dst.left = (sint32)destPos->x;
		r_dst.top  = (sint32)destPos->y;
		if (destSize) {
			r_dst.right  = (sint32)(destPos->x + destSize->x);
			r_dst.bottom = (sint32)(destPos->y + destSize->y);
		}
		else if (src) {
			r_dst.right  = (sint32)(destPos->x + src->width);
			r_dst.bottom = (sint32)(destPos->y + src->height);
		}
		else {
			r_dst.right  = (sint32)(destPos->x + image->width);
			r_dst.bottom = (sint32)(destPos->y + image->height);
		}
	}
	else if (Main_RenderScale() != 1) {
		// Force using destination rect to scale images.
		destPos = &dummy;

		r_dst.left   = 0;
		r_dst.top    = 0;
		r_dst.right  = (sint32)(image->width);
		r_dst.bottom = (sint32)(image->height);
	}

	r_dst.left   *= Main_RenderScale();
	r_dst.top    *= Main_RenderScale();
	r_dst.right  *= Main_RenderScale();
	r_dst.bottom *= Main_RenderScale();

	Draw_AssertUnlocked("Image_DisplayScaled");
	if (Image_IsRenderEnabled()) {
		uint32 flags = DDBLT_WAIT;
		if (image->flags & ImageFlags::IMAGE_FLAG_TRANS) flags |= DDBLT_KEYSRC;
		DirectDraw_bSurf()->Blt((destPos) ? &r_dst : nullptr, image->surface, (src) ? &r_src : nullptr, flags, nullptr);
	}
}

// <LegoRR.exe @0047e120>
void* __cdecl Gods98::Image_LockSurface(Image* image, OUT uint32* pitch, OUT uint32* bpp)
{
	log_firstcall();

	DDSURFACEDESC2 desc;
	std::memset(&desc, 0, sizeof(DDSURFACEDESC2));
	desc.dwSize = sizeof(DDSURFACEDESC2);

	if (image->surface->Lock(nullptr, &desc, DDLOCK_WAIT, nullptr) == DD_OK) {
		*pitch = desc.lPitch;
		*bpp = desc.ddpfPixelFormat.dwRGBBitCount;
		return desc.lpSurface;
	}

	return nullptr;
}

// <LegoRR.exe @0047e190>
void __cdecl Gods98::Image_UnlockSurface(Image* image)
{
	log_firstcall();

	image->surface->Unlock(nullptr);
}

// <LegoRR.exe @0047e1b0>
uint32 __cdecl Gods98::Image_GetPen255BigEndian(Image* image)
{
	log_firstcall();

	DDPIXELFORMAT pf;
	std::memset(&pf, 0, sizeof(DDPIXELFORMAT));
	pf.dwSize = sizeof(DDPIXELFORMAT);

	if (image->surface->GetPixelFormat(&pf) == DD_OK) {
		uint32 pen = 0;

//		if (pf.dwRGBBitCount == 8) return 0xff000000;
		uint8* s = (uint8*)&pen;
		const uint8* t = (uint8*)&image->pen255;
		s[0] = t[3];
		s[1] = t[2];
		s[2] = t[1];
		s[3] = t[0];
//		return image->pen255 << (32 - pf.dwRGBBitCount);
		return pen;
	}

	return 0;
}

// <LegoRR.exe @0047e210>
uint32 __cdecl Gods98::Image_GetPixelMaskBigEndian(Image* image)
{
	log_firstcall();

	DDPIXELFORMAT pf;
	std::memset(&pf, 0, sizeof(DDPIXELFORMAT));
	pf.dwSize = sizeof(DDPIXELFORMAT);

	if (image->surface->GetPixelFormat(&pf) == DD_OK) {
		// This seems to be used for A<RGB/BGR> order,
		// where A is the least-significant byte,
		// and the RGB channels cover the most significant bytes(?)
		// Mask for a 24-bit colour would be  0xffffff00
		// Mask for a 16-bit colour would be  0xffff0000
		// See Image_GetPen255BigEndian and Font_Load

		return 0xffffffff << (32 - pf.dwRGBBitCount);
	}
	return 0;
}

/// CUSTOM: Replacement for Image_GetPenZero. Returns colour as it is stored in the surface.
uint32 Gods98::Image_GetPaletteEntry0(Image* image)
{
	return image->penZero;
}

/// CUSTOM: Replacement for Image_GetPen255BigEndian. Returns colour as it is stored in the surface.
uint32 Gods98::Image_GetPaletteEntry255(Image* image)
{
	return image->pen255;
}

/// CUSTOM: Replacement for Image_GetPixelMaskBigEndian. Returns pixel mask as it is stored in the surface.
uint32 Gods98::Image_GetPixelMask(Image* image)
{
	DDPIXELFORMAT pf = { 0 };
	pf.dwSize = sizeof(DDPIXELFORMAT);

	if (image->surface->GetPixelFormat(&pf) == DD_OK) {
		return DirectDraw_BitCountToMask(pf.dwRGBBitCount);
	}
	return 0;
}

// <LegoRR.exe @0047e260>
bool32 __cdecl Gods98::Image_GetPixel(Image* image, uint32 x, uint32 y, OUT uint32* colour)
{
	log_firstcall();

	DDSURFACEDESC2 desc;
	std::memset(&desc, 0, sizeof(DDSURFACEDESC2));
	desc.dwSize = sizeof(DDSURFACEDESC2);

	if (x < image->width && y < image->height) {
		if (image->surface->Lock(nullptr, &desc, DDLOCK_WAIT|DDLOCK_READONLY, nullptr) == DD_OK) {
			const uint32 bpp = desc.ddpfPixelFormat.dwRGBBitCount;

			const uint8* src = static_cast<uint8*>(DirectDraw_PixelAddress(desc.lpSurface, desc.lPitch, bpp, x, y));
			switch (bpp) {
			case 8:
				*colour = *src;
				break;
			case 16:
				*colour = *reinterpret_cast<const uint16*>(src);
				break;
			case 24:
				*colour = (src[0] | (src[1] << 8) | (src[2] << 16));
				break;
			case 32:
				*colour = *reinterpret_cast<const uint32*>(src);
				break;
			default:
				*colour = 0;
				break;
			}
			
			image->surface->Unlock(nullptr);
			return true;
		}
	}

	return false;
}

// REPLACEMENT FOR: Image_GetPixel, because the functions that use GetPixel are checking specifically for black (0).
// <LegoRR.exe @0047e260>
bool32 __cdecl Gods98::Image_GetPixelTruncate(Image* image, uint32 x, uint32 y, OUT uint32* colour)
{
	DDSURFACEDESC2 desc;
	std::memset(&desc, 0, sizeof(DDSURFACEDESC2));
	desc.dwSize = sizeof(DDSURFACEDESC2);

	if (x < image->width && y < image->height) {
		if (image->surface->Lock(nullptr, &desc, DDLOCK_WAIT|DDLOCK_READONLY, nullptr) == DD_OK) {
			const uint32 bpp = desc.ddpfPixelFormat.dwRGBBitCount;

			const uint8* src = static_cast<uint8*>(DirectDraw_PixelAddress(desc.lpSurface, desc.lPitch, bpp, x, y));
			switch (bpp) {
			case 8:
				*colour = *src;
				break;
			case 16:
				*colour = *reinterpret_cast<const uint16*>(src);
				break;
			case 24:
			case 32:
				{
					const uint32 rMask = desc.ddpfPixelFormat.dwRBitMask;
					const uint32 gMask = desc.ddpfPixelFormat.dwGBitMask;
					const uint32 bMask = desc.ddpfPixelFormat.dwBBitMask;
					const uint32 aMask = desc.ddpfPixelFormat.dwRGBAlphaBitMask;

					const uint32 rBitCount = DirectDraw_CountMaskBits(rMask);
					const uint32 gBitCount = DirectDraw_CountMaskBits(gMask);
					const uint32 bBitCount = DirectDraw_CountMaskBits(bMask);
					const uint32 aBitCount = DirectDraw_CountMaskBits(aMask);

					const uint32 rBitShift = DirectDraw_CountMaskBitShift(rMask);
					const uint32 gBitShift = DirectDraw_CountMaskBitShift(gMask);
					const uint32 bBitShift = DirectDraw_CountMaskBitShift(bMask);
					const uint32 aBitShift = DirectDraw_CountMaskBitShift(aMask);

					uint32 value;
					if (bpp == 24)
						value = (src[0] | (src[1] << 8) | (src[2] << 16));
					else
						value = *reinterpret_cast<const uint32*>(src) & ~desc.ddpfPixelFormat.dwRGBAlphaBitMask;

					/// 16-BIT: Truncate colours so that anything that *would* be black in 16-bit mode *is* black.
					const uint8 r = DirectDraw_UnshiftChannelByte(value, rBitCount, rBitShift) & 0xf8; // 5-bit red
					const uint8 g = DirectDraw_UnshiftChannelByte(value, gBitCount, gBitShift) & 0xfc; // 6-bit green
					const uint8 b = DirectDraw_UnshiftChannelByte(value, bBitCount, bBitShift) & 0xf8; // 5-bit blue

					// Don't return alpha value
					*colour =
						DirectDraw_ShiftChannelByte(r, rBitCount, rBitShift) |
						DirectDraw_ShiftChannelByte(g, gBitCount, gBitShift) |
						DirectDraw_ShiftChannelByte(b, bBitCount, bBitShift);
				}
				//*colour = ((src[0] & 0xf8) | ((src[1] & 0xfc) << 8) | ((src[2] & 0xf8) << 16));
				break;
			//case 32:
			//	//*colour = *reinterpret_cast<uint32*>(src) & ~desc.ddpfPixelFormat.dwRGBAlphaBitMask;
			//	break;
			default:
				*colour = 0;
				break;
			}

			image->surface->Unlock(nullptr);
			return true;
		}
	}

	return false;
}

// <LegoRR.exe @0047e310>
Gods98::Image* __cdecl Gods98::Image_Create(IDirectDrawSurface4* surface, uint32 width, uint32 height,
											ColourRGBAPacked penZero, ColourRGBAPacked pen255)
{
	log_firstcall();

	Image_CheckInit();

	Image* newImage = imageListSet.Add();

	newImage->flags = ImageFlags::IMAGE_FLAG_NONE;
	newImage->width = width;
	newImage->height = height;
	newImage->surface = surface;
	newImage->penZeroRGB = penZero;
	newImage->penZero = Image_DDColorMatch(surface, penZero);
	newImage->pen255 = Image_DDColorMatch(surface, pen255);

	return newImage;
}

// <LegoRR.exe @0047e380>
void __cdecl Gods98::Image_AddList(void)
{
	// NOTE: This function is no longer called, imageListSet.Add already handles this.
	imageListSet.AddList();
}

// <LegoRR.exe @0047e3f0>
void __cdecl Gods98::Image_RemoveAll(void)
{
	log_firstcall();

	for (Image* image : imageListSet.EnumerateAlive()) {
		Image_Remove(image);
	}
}

// <LegoRR.exe @0047e450>
uint32 __cdecl Gods98::Image_DDColorMatch(IDirectDrawSurface4* surface, ColourRGBAPacked cr)
{
	log_firstcall();

	uint8 r, g, b, a;
	Image_CR2RGBA(cr, &r, &g, &b, &a);

	return DirectDraw_ToColourFromRGB(surface, r, g, b, a);
}

// <LegoRR.exe @0047e590>
void __cdecl Gods98::Image_CR2RGB(ColourRGBAPacked cr, OPTIONAL OUT uint8* r, OPTIONAL OUT uint8* g, OPTIONAL OUT uint8* b)
{
	return Image_CR2RGBA(cr, r, g, b, nullptr);
}

/// CUSTOM:
void __cdecl Gods98::Image_CR2RGBA(ColourRGBAPacked cr, OPTIONAL OUT uint8* r, OPTIONAL OUT uint8* g, OPTIONAL OUT uint8* b, OPTIONAL OUT uint8* a)
{
	log_firstcall();
	
	if (r) *r = cr.red;
	if (g) *g = cr.green;
	if (b) *b = cr.blue;
	if (a) *a = cr.alpha;
}


// <LegoRR.exe @0047e5c0>
void __cdecl Gods98::Image_GetScreenshot(OUT Image* image, uint32 xsize, uint32 ysize)
{
	log_firstcall();

	HRESULT hr;
	IDirectDrawSurface4* surf;
	// Create surface
	DDSURFACEDESC2 desc;
	std::memset(&desc, 0, sizeof(DDSURFACEDESC2));
	desc.dwSize = sizeof(DDSURFACEDESC2);
	desc.dwWidth = xsize;
	desc.dwHeight = ysize;
	desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;

	if ((hr = DirectDraw()->CreateSurface(&desc, &surf, nullptr)) != DD_OK) {
		CHKDD(hr);
		return;
	}

	// Blit back buffer onto here.
	{
		RECT dest = {
			0, 0,
			(sint32)xsize,
			(sint32)ysize,
		};
		Draw_AssertUnlocked("Image_GetScreenshot");
		surf->Blt(&dest, DirectDraw_bSurf(), nullptr, 0, nullptr);
	}
	// Create image
	Image_InitFromSurface(image, surf, xsize, ysize, ColourRGBAPacked { 0 }, ColourRGBAPacked { 0 });
}

// <LegoRR.exe @0047e6a0>
void __cdecl Gods98::Image_InitFromSurface(Image* newImage, IDirectDrawSurface4* surface,
								uint32 width, uint32 height, ColourRGBAPacked penZero, ColourRGBAPacked pen255)
{
	log_firstcall();

	Image_CheckInit();

	// This serves no purpose, as only existing images (or images stored outside the listSet) would be passed here.
	//if (imageGlobs.freeList == nullptr) imageListSet.AddList();
	
	/// FIXME: Figure out how best to handle re-assignment of nextFree here.
	///        Are there any checks where this could cause issues if left as is or if changed?
	///        How should items not stored in the listSet have this field assigned?
	newImage->nextFree = newImage;

	newImage->flags = ImageFlags::IMAGE_FLAG_NONE;
	newImage->width = width;
	newImage->height = height;
	newImage->surface = surface;
	newImage->penZeroRGB = penZero;
	newImage->penZero = Image_DDColorMatch(surface, penZero);
	newImage->pen255 = Image_DDColorMatch(surface, pen255);
}

// <LegoRR.exe @0047e700>
bool32 __cdecl Gods98::Image_SaveBMP(Image* image, const char* fname)
{
	return Image_SaveBMP2(image, fname, FileFlags::FILE_FLAGS_DEFAULT);
}

/// CUSTOM: Support for FileFlags.
bool Gods98::Image_SaveBMP2(Image* image, const char* fname, FileFlags fileFlags)
{
	log_firstcall();

	return DirectDraw_SaveBMP2(image->surface, fname, fileFlags);
}

// <missing>
/*void __cdecl Gods98::Image_GetPenZero(const Image* image, OPTIONAL OUT real32* r, OPTIONAL OUT real32* g, OPTIONAL OUT real32* b)
{
	log_firstcall();

	/// FIX GODS98: Include masking for red channel
	if (r) *r = (real32)((image->penZeroRGB >> 16) & 0xff) / 255.0f;
	if (g) *g = (real32)((image->penZeroRGB >>  8) & 0xff) / 255.0f;
	if (b) *b = (real32)((image->penZeroRGB)       & 0xff) / 255.0f;
}*/
