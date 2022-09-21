// Draw.cpp : 
//

#include "../../platform/ddraw.h"
#include "../../platform/d3drm.h"			// For Viewports.h

#include "../core/Errors.h"
#include "../core/Maths.h"
#include "../Main.h"
#include "DirectDraw.h"

#include "Draw.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @005417e8>
Gods98::Draw_Globs & Gods98::drawGlobs = *(Gods98::Draw_Globs*)0x005417e8; // = { nullptr };

/// CUSTOM: Basic handling for alpha (but not support for drawing with alpha other than 255).
static uint32 _drawAlphaBits  = 0;
static uint32 _drawAlphaMask  = 0;
static uint32 _drawRedShift   = 0;
static uint32 _drawGreenShift = 0;
static uint32 _drawBlueShift  = 0;
static uint32 _drawAlphaShift = 0;

/// CUSTOM: Draw_Begin() has been called, normal Draw operations will not unlock the drawing surface until Draw_End().
static bool _drawBegin = false;
/// CUSTOM: The current drawing effect while using Draw_Begin().
static Gods98::DrawEffect _drawEffect = Gods98::DrawEffect::None;

/// CUSTOM: If false, then the Draw module will not lock or draw to surfaces at all.
static bool _renderEnabled = true;
/// CUSTOM: True if the drawing surface is supposed to be locked, or Draw_Begin() has been called.
static bool _drawLocked = false;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define Draw_Pixel8Address(x, y)	static_cast<uint8*> (DirectDraw_PixelAddress(drawGlobs.buffer, drawGlobs.pitch,  8, x, y))
#define Draw_Pixel16Address(x, y)	static_cast<uint16*>(DirectDraw_PixelAddress(drawGlobs.buffer, drawGlobs.pitch, 16, x, y))
#define Draw_Pixel24Address(x, y)	static_cast<uint8*> (DirectDraw_PixelAddress(drawGlobs.buffer, drawGlobs.pitch, 24, x, y))
#define Draw_Pixel32Address(x, y)	static_cast<uint32*>(DirectDraw_PixelAddress(drawGlobs.buffer, drawGlobs.pitch, 32, x, y))

/// CUSTOM: Unlocks the surface during normal Draw calls while not using Draw_Begin().
#define Draw_TryUnlockSurface(surf)	if (!_drawBegin) Draw_UnlockSurface(surf)

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

/// CUSTOM: Gets if the Draw module rendering is enabled.
bool Gods98::Draw_IsRenderEnabled()
{
	return _renderEnabled;
}

/// CUSTOM: Sets if the Draw module rendering is enabled. For testing performance.
void Gods98::Draw_SetRenderEnabled(bool enabled)
{
	if (_renderEnabled != enabled) {
		if (!enabled && _drawBegin) {
			Draw_End();
		}
		_renderEnabled = enabled;
	}
}


/// CUSTOM: Returns true if the drawing surface is currently locked, i.e. after calling Draw_Begin().
bool Gods98::Draw_IsLocked()
{
	return _drawLocked || (drawGlobs.buffer != nullptr);
}

/// CUSTOM: Locks the drawing surface and waits for Draw_End() to be called before unlocking it.
bool Gods98::Draw_Begin()
{
	Error_Warn(Draw_IsLocked(), "Draw_Begin: Called more than once before calling Draw_End()");

	// Will still set new effect if already locked.
	if (Draw_LockSurface(DirectDraw_bSurf(), Gods98::DrawEffect::None)) {
		_drawBegin = true;
	}
	return _drawBegin;
}

/// CUSTOM: Unlocks the drawing surface after Draw_Begin() was called.
void Gods98::Draw_End()
{
	Error_Warn(!Draw_IsLocked(), "Draw_End: Called without first calling Draw_Begin()");

	Draw_UnlockSurface(DirectDraw_bSurf());
	_drawBegin = false;
}

/// CUSTOM: Gets the current effect while the drawing surface has been locked with Draw_Begin().
Gods98::DrawEffect Gods98::Draw_GetEffect()
{
	return _drawEffect;
}

/// CUSTOM: Sets the current effect while the drawing surface has been locked with Draw_Begin().
void Gods98::Draw_SetEffect(DrawEffect effect)
{
	Error_Warn(!Draw_IsLocked(), "Draw_SetEffect: Called without first calling Draw_Begin()");

	// We need to know the bitdepth to set the pixel function, so we must be locked.
	if (Draw_IsLocked()) {
		Draw_SetDrawPixelFunc(effect);
	}
}

/// CUSTOM:
void Gods98::Draw_AssertUnlocked(const char* caller)
{
	if (Draw_IsLocked()) {
		Error_FatalF(true, "%s: Draw_End() has not been called before the back surface is needed", caller);
		Draw_End(); // End for users that want to continue past the error dialog.
	}
}


// <LegoRR.exe @00486140>
void __cdecl Gods98::Draw_Initialise(const Area2F* window)
{
	log_firstcall();

	drawGlobs.flags |= Draw_GlobFlags::DRAW_GLOB_FLAG_INITIALISED;
	Draw_SetClipWindow(window);
}

// <LegoRR.exe @00486160>
void __cdecl Gods98::Draw_SetClipWindow(const Area2F* window)
{
	log_firstcall();

	IDirectDrawSurface4* surf = DirectDraw_bSurf();

	Error_Fatal(!(drawGlobs.flags & Draw_GlobFlags::DRAW_GLOB_FLAG_INITIALISED), "Draw not initialised");

	drawGlobs.clipStart.x = 0.0f;
	drawGlobs.clipStart.y = 0.0f;
	if (window) {
		drawGlobs.clipStart.x = std::max(drawGlobs.clipStart.x, window->x);
		drawGlobs.clipStart.y = std::max(drawGlobs.clipStart.y, window->y);
	}

	DDSURFACEDESC2 desc;
	std::memset(&desc, 0, sizeof(DDSURFACEDESC2));
	desc.dwSize = sizeof(DDSURFACEDESC2);
	if (surf->GetSurfaceDesc(&desc) == DD_OK) {
		drawGlobs.clipEnd.x = static_cast<real32>(desc.dwWidth);
		drawGlobs.clipEnd.y = static_cast<real32>(desc.dwHeight);
		if (window) {
			drawGlobs.clipEnd.x = std::min(drawGlobs.clipEnd.x, (window->x + window->width));
			drawGlobs.clipEnd.y = std::min(drawGlobs.clipEnd.y, (window->y + window->height));
		}
	}
	else {
		Error_Warn(true, "Draw_SetClipWindow: Failed to GetSurfaceDesc for DirectDraw_bSurf");
	}

//	drawGlobs.lockRect.left = (uint32) drawGlobs.clipStart.x;
//	drawGlobs.lockRect.top = (uint32) drawGlobs.clipStart.y;
//	drawGlobs.lockRect.right = (uint32) drawGlobs.clipEnd.x;
//	drawGlobs.lockRect.bottom = (uint32) drawGlobs.clipEnd.y;
}

// <LegoRR.exe @00486270>
void __cdecl Gods98::Draw_GetClipWindow(OUT Area2F* window)
{
	log_firstcall();

	window->x = drawGlobs.clipStart.x;
	window->y = drawGlobs.clipStart.y;
	window->width  = drawGlobs.clipEnd.x - drawGlobs.clipStart.x;
	window->height = drawGlobs.clipEnd.y - drawGlobs.clipStart.y;
}

// <unused>
void __cdecl Gods98::Draw_PixelListEx(const Point2F* pointList, uint32 count, real32 r, real32 g, real32 b, DrawEffect effect)
{
	log_firstcall();

	IDirectDrawSurface4* surf = DirectDraw_bSurf();

	Error_Fatal(!(drawGlobs.flags & Draw_GlobFlags::DRAW_GLOB_FLAG_INITIALISED), "Draw not initialised");

	if (Draw_LockSurface(surf, effect)) {

		uint32 colour = Draw_GetColour(r, g, b);

		for (uint32 loop=0 ; loop<count ; loop++){
			const Point2F* point = &pointList[loop];

			if (point->x >= drawGlobs.clipStart.x && point->y >= drawGlobs.clipStart.y && point->x < drawGlobs.clipEnd.x && point->y < drawGlobs.clipEnd.y){
				drawGlobs.drawPixelFunc((sint32) point->x, (sint32) point->y, colour);
			}
		}
		Draw_TryUnlockSurface(surf);
	}
}

// <LegoRR.exe @004862b0>
void __cdecl Gods98::Draw_LineListEx(const Point2F* fromList, const Point2F* toList, uint32 count, real32 r, real32 g, real32 b, DrawEffect effect)
{
	log_firstcall();

	IDirectDrawSurface4* surf = DirectDraw_bSurf();

	Error_Fatal(!(drawGlobs.flags & Draw_GlobFlags::DRAW_GLOB_FLAG_INITIALISED), "Draw not initialised");

	if (Draw_LockSurface(surf, effect)) {
		
		uint32 colour = Draw_GetColour(r, g, b);
		
		for (uint32 loop=0 ; loop<count ; loop++){
			const Point2F* from = &fromList[loop];
			const Point2F* to = &toList[loop];
			Draw_LineActual((sint32) from->x, (sint32) from->y, (sint32) to->x, (sint32) to->y, colour);
		}
		Draw_TryUnlockSurface(surf);
	}
}

// <unused>
//void __cdecl Gods98::Draw_WorldLineListEx(Viewport* vp, const Vector3F* fromList, const Vector3F* toList, uint32 count, real32 r, real32 g, real32 b, real32 a, DrawEffect effect)
//{
//}

// <LegoRR.exe @00486350>
void __cdecl Gods98::Draw_RectListEx(const Area2F* rectList, uint32 count, real32 r, real32 g, real32 b, DrawEffect effect)
{
	log_firstcall();

	IDirectDrawSurface4* surf = DirectDraw_bSurf();

	Error_Fatal(!(drawGlobs.flags & Draw_GlobFlags::DRAW_GLOB_FLAG_INITIALISED), "Draw not initialised");

	if (Draw_LockSurface(surf, effect)) {

		uint32 colour = Draw_GetColour(r, g, b);

		for (uint32 loop=0 ; loop<count ; loop++){
			Area2F rect = rectList[loop];
			Point2F end = {
				rect.x + rect.width,
				rect.y + rect.height
			};

			if (rect.x < drawGlobs.clipStart.x) rect.x = (real32) drawGlobs.clipStart.x;
			if (rect.y < drawGlobs.clipStart.y) rect.y = (real32) drawGlobs.clipStart.y;
			if (end.x >= drawGlobs.clipEnd.x) end.x = (real32) (drawGlobs.clipEnd.x-1);
			if (end.y >= drawGlobs.clipEnd.y) end.y = (real32) (drawGlobs.clipEnd.y-1);
			sint32 ex = (sint32) end.x;
			sint32 ey = (sint32) end.y;
			for (sint32 y=(sint32)rect.y ; y<ey ; y++){
				for (sint32 x=(sint32)rect.x ; x<ex ; x++){
					drawGlobs.drawPixelFunc(x, y, colour);
				}
			}
		}
		Draw_TryUnlockSurface(surf);
	}
}

// <LegoRR.exe @004864d0>
void __cdecl Gods98::Draw_RectList2Ex(const Draw_Rect* rectList, uint32 count, DrawEffect effect)
{
	log_firstcall();

	IDirectDrawSurface4* surf = DirectDraw_bSurf();
	/*uint32 loop, colour;
	sint32 x, y, ex, ey;
	Area2F rect;
	Point2F end;*/

	Error_Fatal(!(drawGlobs.flags & Draw_GlobFlags::DRAW_GLOB_FLAG_INITIALISED), "Draw not initialised");

	if (Draw_LockSurface(surf, effect)) {

		for (uint32 loop=0 ; loop<count ; loop++){

			Area2F rect = rectList[loop].rect;
			uint32 colour = Draw_GetColour(rectList[loop].r, rectList[loop].g, rectList[loop].b);

			Point2F end = {
				rect.x + rect.width,
				rect.y + rect.height
			};
			if (rect.x < drawGlobs.clipStart.x) rect.x = (real32) drawGlobs.clipStart.x;
			if (rect.y < drawGlobs.clipStart.y) rect.y = (real32) drawGlobs.clipStart.y;
			if (end.x >= drawGlobs.clipEnd.x) end.x = (real32) (drawGlobs.clipEnd.x-1);
			if (end.y >= drawGlobs.clipEnd.y) end.y = (real32) (drawGlobs.clipEnd.y-1);
			sint32 ex = (sint32) end.x;
			sint32 ey = (sint32) end.y;
			for (sint32 y=(sint32)rect.y ; y<ey ; y++){
				for (sint32 x=(sint32)rect.x ; x<ex ; x++){
					drawGlobs.drawPixelFunc(x, y, colour);

				}
			}
		}
		Draw_TryUnlockSurface(surf);
	}
}

// <LegoRR.exe @00486650>
void __cdecl Gods98::Draw_DotCircle(const Point2F* pos, uint32 radius, uint32 dots, real32 r, real32 g, real32 b, DrawEffect effect)
{
	log_firstcall();

	IDirectDrawSurface4* surf = DirectDraw_bSurf();
	real32 step = (2.0f * M_PI) / dots;

	Error_Fatal(!(drawGlobs.flags & Draw_GlobFlags::DRAW_GLOB_FLAG_INITIALISED), "Draw not initialised");

	if (Draw_LockSurface(surf, effect)) {

		uint32 colour = Draw_GetColour(r, g, b);

		for (uint32 loop=0 ; loop<dots ; loop++){
			real32 angle = step * loop;
			uint32 x = (uint32) (pos->x + (Maths_Sin(angle) * radius));
			uint32 y = (uint32) (pos->y + (Maths_Cos(angle) * radius));
			if (x >= drawGlobs.clipStart.x && y >= drawGlobs.clipStart.y && x < drawGlobs.clipEnd.x && y < drawGlobs.clipEnd.y){
				drawGlobs.drawPixelFunc(x, y, colour);
			}
		}
		Draw_TryUnlockSurface(surf);
	}
}

// <LegoRR.exe @00486790>
uint32 __cdecl Gods98::Draw_GetColour(real32 r, real32 g, real32 b)
{
	log_firstcall();

	if (!Draw_IsRenderEnabled()) {
		return 0; // Behave as if colour was returned.
	}

	Error_Fatal(!Draw_IsLocked(), "Draw_GetColour: Must be called after Draw_LockSurface()");
	
	if (drawGlobs.bpp == 8) {
		/// FIXME: not implemented by GODS98 or LegoRR
		return 0;
	}
	else {
		return
			DirectDraw_ShiftChannelByte(static_cast<uint8>(r * 255.0f), drawGlobs.redBits,   _drawRedShift)   |
			DirectDraw_ShiftChannelByte(static_cast<uint8>(g * 255.0f), drawGlobs.greenBits, _drawGreenShift) |
			DirectDraw_ShiftChannelByte(static_cast<uint8>(b * 255.0f), drawGlobs.blueBits,  _drawBlueShift)  |
			_drawAlphaMask;
	}
}

// <LegoRR.exe @00486810>
bool32 __cdecl Gods98::Draw_LockSurface(IDirectDrawSurface4* surf, DrawEffect effect)
{
	log_firstcall();

	if (Draw_IsLocked()) {
		Draw_SetDrawPixelFunc(effect);
		return true; // Already locked.
	}

	if (!Draw_IsRenderEnabled()) {
		_drawLocked = true;
		Draw_SetDrawPixelFunc(effect);
		return true; // Behave as if locked.
	}

	DDSURFACEDESC2 desc = { 0 };
	desc.dwSize = sizeof(DDSURFACEDESC2);

//	if (surf->Lock(reinterpret_cast<RECT*>(&drawGlobs.lockRect), &desc, DDLOCK_WAIT, nullptr) == DD_OK) {
	if (surf->Lock(nullptr, &desc, DDLOCK_WAIT, nullptr) == DD_OK) {

		drawGlobs.buffer    = desc.lpSurface;
		drawGlobs.pitch     = desc.lPitch;
		drawGlobs.redMask   = desc.ddpfPixelFormat.dwRBitMask;
		drawGlobs.greenMask = desc.ddpfPixelFormat.dwGBitMask;
		drawGlobs.blueMask  = desc.ddpfPixelFormat.dwBBitMask;
		_drawAlphaMask      = desc.ddpfPixelFormat.dwRGBAlphaBitMask;
		drawGlobs.bpp       = desc.ddpfPixelFormat.dwRGBBitCount;

		drawGlobs.redBits   = DirectDraw_CountMaskBits(drawGlobs.redMask);
		drawGlobs.greenBits = DirectDraw_CountMaskBits(drawGlobs.greenMask);
		drawGlobs.blueBits  = DirectDraw_CountMaskBits(drawGlobs.blueMask);
		_drawAlphaBits      = DirectDraw_CountMaskBits(_drawAlphaMask);

		_drawRedShift       = DirectDraw_CountMaskBitShift(drawGlobs.redMask);
		_drawGreenShift     = DirectDraw_CountMaskBitShift(drawGlobs.greenMask);
		_drawBlueShift      = DirectDraw_CountMaskBitShift(drawGlobs.blueMask);
		_drawAlphaShift     = DirectDraw_CountMaskBitShift(_drawAlphaMask);

		_drawLocked = true;

		if (Draw_SetDrawPixelFunc(effect)) {
			return true;
		}

		Draw_UnlockSurface(surf);
	}

	return false;
}

// <LegoRR.exe @00486910>
void __cdecl Gods98::Draw_UnlockSurface(IDirectDrawSurface4* surf)
{
	log_firstcall();

	if (!Draw_IsRenderEnabled()) {
		// Behave as if unlocked.
	}
	else if (Draw_IsLocked()) {

	//	surf->Unlock(reinterpret_cast<RECT*>(&drawGlobs.lockRect));
		surf->Unlock(nullptr);
		drawGlobs.buffer    = nullptr;
		drawGlobs.pitch     = 0;
		drawGlobs.redMask   = 0;
		drawGlobs.greenMask = 0;
		drawGlobs.blueMask  = 0;
		_drawAlphaMask      = 0;
		drawGlobs.bpp       = 0;
		drawGlobs.drawPixelFunc = nullptr;
	}

	_drawEffect = DrawEffect::None;
	_drawBegin = false;
	_drawLocked = false;
	_drawRenderTarget = nullptr;
}

// <LegoRR.exe @00486950>
bool32 __cdecl Gods98::Draw_SetDrawPixelFunc(DrawEffect effect)
{
	log_firstcall();

	if (!Draw_IsRenderEnabled()) {
		drawGlobs.drawPixelFunc = Draw_PixelDummy;
		_drawEffect = effect;
		return true; // Behave as if pixel function is set.
	}

	switch (drawGlobs.bpp) {
	case 8:
		drawGlobs.drawPixelFunc = Draw_Pixel8;
		_drawEffect = DrawEffect::None;
		return true;

	case 16:
		switch (effect) {
		case DrawEffect::None:		drawGlobs.drawPixelFunc = Draw_Pixel16; break;
		case DrawEffect::XOR:		drawGlobs.drawPixelFunc = Draw_Pixel16XOR; break;
		case DrawEffect::HalfTrans:	drawGlobs.drawPixelFunc = Draw_Pixel16HalfTrans; break;
		default:					return false;
		}
		_drawEffect = effect;
		return true;

	case 24:
		switch (effect) {
		case DrawEffect::None:		drawGlobs.drawPixelFunc = Draw_Pixel24; break;
		case DrawEffect::XOR:		drawGlobs.drawPixelFunc = Draw_Pixel24XOR; break;
		case DrawEffect::HalfTrans:	drawGlobs.drawPixelFunc = Draw_Pixel24HalfTrans; break;
		default:					return false;
		}
		_drawEffect = effect;
		return true;

	case 32:
		switch (effect) {
		case DrawEffect::None:		drawGlobs.drawPixelFunc = Draw_Pixel32; break;
		case DrawEffect::XOR:		drawGlobs.drawPixelFunc = Draw_Pixel32XOR; break;
		case DrawEffect::HalfTrans:	drawGlobs.drawPixelFunc = Draw_Pixel32HalfTrans; break;
		default:					return false;
		}
		_drawEffect = effect;
		return true;

	default:
		drawGlobs.drawPixelFunc = nullptr;
		return false;
	}
}

// Bresenham's algorithm (line drawing)
// see: <http://www.brackeen.com/vga/shapes.html#shapes3.0>
// also see: <https://github.com/trigger-segfault/AsciiArtist/blob/75c75467d4f5fa2da02c1ba9712deb16529bbbde/PowerConsoleLib/PowerConsole/Drawing/Graphics.cpp#L459-L499>
// <LegoRR.exe @004869e0>
void __cdecl Gods98::Draw_LineActual(sint32 x1, sint32 y1, sint32 x2, sint32 y2, uint32 colour)
{
	log_firstcall();

	sint32 numpixels;
	sint32 d, dinc1, dinc2;

	// Sign of the distance
	sint32 xinc1, xinc2;
	sint32 yinc1, yinc2;

	// Absolute distance
	sint32 deltax = std::abs(x2 - x1);
	sint32 deltay = std::abs(y2 - y1);

	// different drawing behavior depending on the larger coordinate plane distance
	if (deltax >= deltay) {
		numpixels = deltax + 1;
		d = 2 * deltay - deltax;
		dinc1 = deltay << 1;
		dinc2 = (deltay - deltax) << 1;
		xinc1 = 1;
		xinc2 = 1;
		yinc1 = 0;
		yinc2 = 1;
	} else {
		numpixels = deltay + 1;
		d = (2 * deltax) - deltay;
		dinc1 = deltax << 1;
		dinc2 = (deltax - deltay) << 1;
		xinc1 = 0;
		xinc2 = 1;
		yinc1 = 1;
		yinc2 = 1;
	}
	
	if (x1>x2) {
		xinc1 = -xinc1;
		xinc2 = -xinc2;
	}
	if (y1>y2) {
		yinc1 = -yinc1;
		yinc2 = -yinc2;
	}
	sint32 x = x1;
	sint32 y = y1;
	
	for (sint32 loop=1 ; loop<=numpixels ; loop++) {
		
		if (x >= drawGlobs.clipStart.x && y >= drawGlobs.clipStart.y && x < drawGlobs.clipEnd.x && y < drawGlobs.clipEnd.y){
			drawGlobs.drawPixelFunc(x, y, colour);
		}
		
		if (d < 0) {
			d = d + dinc1;
			x = x + xinc1;
			y = y + yinc1;
		} else {
			d = d + dinc2;
			x = x + xinc2;
			y = y + yinc2;
		}
	}
}

/// CUSTOM:
uint32 Gods98::_Draw_ConvertHalfTrans(uint32 pixel, uint32 value)
{
	const uint32 pix_r = ((pixel & drawGlobs.redMask)   >> _drawRedShift);
	const uint32 pix_g = ((pixel & drawGlobs.greenMask) >> _drawGreenShift);
	const uint32 pix_b = ((pixel & drawGlobs.blueMask)  >> _drawBlueShift);

	const uint32 val_r = ((value & drawGlobs.redMask)   >> _drawRedShift);
	const uint32 val_g = ((value & drawGlobs.greenMask) >> _drawGreenShift);
	const uint32 val_b = ((value & drawGlobs.blueMask)  >> _drawBlueShift);

	const uint32 r = (((pix_r + val_r) / 2) << _drawRedShift)   & drawGlobs.redMask;
	const uint32 g = (((pix_g + val_g) / 2) << _drawGreenShift) & drawGlobs.greenMask;
	const uint32 b = (((pix_b + val_b) / 2) << _drawBlueShift)  & drawGlobs.blueMask;

	return (r|g|b);
}

/// CUSTOM:
void __cdecl Gods98::Draw_PixelDummy(sint32 x, sint32 y, uint32 value)
{
	// Do nothing.
}

// <LegoRR.exe @00486b40>
void __cdecl Gods98::Draw_Pixel8(sint32 x, sint32 y, uint32 value)
{
	uint8* addr = Draw_Pixel8Address(x, y);
	*addr = static_cast<uint8>(value);
}

// <LegoRR.exe @00486b60>
void __cdecl Gods98::Draw_Pixel16(sint32 x, sint32 y, uint32 value)
{
	uint16* addr = Draw_Pixel16Address(x, y);
	*addr = static_cast<uint16>(value);
}

// <LegoRR.exe @00486b90>
void __cdecl Gods98::Draw_Pixel16XOR(sint32 x, sint32 y, uint32 value)
{
	uint16* addr = Draw_Pixel16Address(x, y);
	*addr ^= static_cast<uint16>(value);
}

// <LegoRR.exe @00486bc0>
void __cdecl Gods98::Draw_Pixel16HalfTrans(sint32 x, sint32 y, uint32 value)
{
	uint16* addr = Draw_Pixel16Address(x, y);

	value = _Draw_ConvertHalfTrans(*addr, static_cast<uint16>(value));
	*addr = static_cast<uint16>(value);

	//uint16 r = (((*addr >> 12) + ((uint16) value >> 12)) << 11) & (uint16) drawGlobs.redMask;
	//uint16 g = ((((*addr & (uint16) drawGlobs.greenMask) >> 6) + (((uint16) value & (uint16) drawGlobs.greenMask) >> 6)) << 5) & (uint16) drawGlobs.greenMask;
	//uint16 b = (((*addr & (uint16) drawGlobs.blueMask) >> 1) + (((uint16) value & (uint16) drawGlobs.blueMask) >> 1)) & (uint16) drawGlobs.blueMask;

	//*addr = (r|g|b);
}

// <LegoRR.exe @00486c60>
void __cdecl Gods98::Draw_Pixel24(sint32 x, sint32 y, uint32 value)
{
	uint8* addr = Draw_Pixel24Address(x, y);
	addr[0] = static_cast<uint8>(value      );
	addr[1] = static_cast<uint8>(value >>  8);
	addr[2] = static_cast<uint8>(value >> 16);
}

/// CUSTOM:
void __cdecl Gods98::Draw_Pixel24XOR(sint32 x, sint32 y, uint32 value)
{
	uint8* addr = Draw_Pixel24Address(x, y);
	addr[0] ^= static_cast<uint8>(value      );
	addr[1] ^= static_cast<uint8>(value >>  8);
	addr[2] ^= static_cast<uint8>(value >> 16);
}

/// CUSTOM:
void __cdecl Gods98::Draw_Pixel24HalfTrans(sint32 x, sint32 y, uint32 value)
{
	uint8* addr = Draw_Pixel24Address(x, y);

	const uint32 pixel = (addr[0] | (addr[1] << 8) | (addr[2] << 16));
	value = _Draw_ConvertHalfTrans(pixel, value);
	addr[0] = static_cast<uint8>(value      );
	addr[1] = static_cast<uint8>(value >>  8);
	addr[2] = static_cast<uint8>(value >> 16);
}

// <LegoRR.exe @00486c90>
void __cdecl Gods98::Draw_Pixel32(sint32 x, sint32 y, uint32 value)
{
	uint32* addr = Draw_Pixel32Address(x, y);
	// Ignore alpha, Draw isn't designed to work with it.
	*addr = (value | (*addr & _drawAlphaMask));
}

/// CUSTOM:
void __cdecl Gods98::Draw_Pixel32XOR(sint32 x, sint32 y, uint32 value)
{
	uint32* addr = Draw_Pixel32Address(x, y);
	// Ignore alpha, Draw isn't designed to work with it.
	*addr ^= (value & ~_drawAlphaMask);
}

/// CUSTOM:
void __cdecl Gods98::Draw_Pixel32HalfTrans(sint32 x, sint32 y, uint32 value)
{
	uint32* addr = Draw_Pixel32Address(x, y);

	// Ignore alpha, Draw isn't designed to work with it.
	value = _Draw_ConvertHalfTrans(*addr & ~_drawAlphaMask, value & ~_drawAlphaMask);
	*addr = (value | (*addr & _drawAlphaMask));
}


#pragma endregion
