// LightEffects.cpp : 
//

#include "../../engine/core/Maths.h"
#include "../../engine/core/Utils.h"

#include "../Game.h"

#include "LightEffects.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004ebdd8>
LegoRR::LightEffects_Globs & LegoRR::lightGlobs = *(LegoRR::LightEffects_Globs*)0x004ebdd8;

/// CUSTOM: Extra settings to allow toggling on/off specific LightEffects functionality.
static bool _blinkEnabled = true;
static bool _fadeEnabled = true;
static bool _moveEnabled = true;

static Vector3F _initialSpotlightPos = { 0.0f, 0.0f, 0.0f };

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define LightEffects_ID(...) Config_ID(gameName, "LightEffects", __VA_ARGS__ )

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0044c9d0>
void __cdecl LegoRR::LightEffects_Initialise(Gods98::Container* rootSpotlight, Gods98::Container* rootLight,
											 real32 initialRed, real32 initialGreen, real32 initialBlue)
{
	std::memset(&lightGlobs, 0, sizeof(lightGlobs));

	lightGlobs.rootSpotlight = rootSpotlight;
	lightGlobs.rootLight = rootLight;
	lightGlobs.initialRGB = ColourRGBF { initialRed, initialGreen, initialBlue };
	lightGlobs.currentRGB = lightGlobs.initialRGB;
	lightGlobs.flags = LIGHTFX_GLOB_FLAG_NONE;
	LightEffects_ResetSpotlightColour();
}


/// CUSTOM: Reset all state values and start from the beginning.
void LegoRR::LightEffects_Restart()
{
	// Reset blink.
	lightGlobs.blinkChange = 0.0f;
	lightGlobs.blinkWait   = 0.0f;

	// Reset fade.
	lightGlobs.fadeDestRGB  = ColourRGBF { 0.0f, 0.0f, 0.0f };
	lightGlobs.fadeSpeedRGB = ColourRGBF { 0.0f, 0.0f, 0.0f };
	lightGlobs.fadePosRGB   = ColourRGBF { 0.0f, 0.0f, 0.0f };
	lightGlobs.fadeWait = 0.0f;
	lightGlobs.fadeHold = 0.0f;
	lightGlobs.flags &= ~(LIGHTFX_GLOB_FLAG_FADING|LIGHTFX_GLOB_FLAG_FADE_FORWARD|LIGHTFX_GLOB_FLAG_FADE_REVERSE);

	// Reset move.
	lightGlobs.moveVector = Vector3F { 0.0f, 0.0f, 0.0f };
	lightGlobs.moveWait  = 0.0f;
	lightGlobs.moveSpeed = 0.0f;
	lightGlobs.moveDist  = 0.0f;
	lightGlobs.flags &= ~(LIGHTFX_GLOB_FLAG_MOVING);

	// Reset dimmer.
	lightGlobs.flags &= ~(LIGHTFX_GLOB_FLAG_DIMOUT|LIGHTFX_GLOB_FLAG_DIMIN_DONE|LIGHTFX_GLOB_FLAG_DIMOUT_DONE);

	// Reset colour and container.
	lightGlobs.currentRGB = lightGlobs.initialRGB;
	LightEffects_ResetSpotlightColour();
	LightEffects_ResetSpotlightPosition();
}

// <LegoRR.exe @0044ca20>
void __cdecl LegoRR::LightEffects_ResetSpotlightColour(void)
{
	/// SANITY: Null check, since its done elsewhere.
	if (lightGlobs.rootSpotlight != nullptr) {
		Gods98::Container_SetColourAlpha(lightGlobs.rootSpotlight, lightGlobs.initialRGB.r,
										 lightGlobs.initialRGB.g, lightGlobs.initialRGB.b, 1.0f);
	}
}

/// CUSTOM:
void LegoRR::LightEffects_ResetSpotlightPosition()
{
	if ((lightGlobs.flags & LIGHTFX_GLOB_FLAG_HASMOVE) && lightGlobs.rootLight != nullptr) {

		//Gods98::Container_SetPosition(lightGlobs.rootSpotlight, lightGlobs.rootLight,
		//							  lightGlobs.movePosition.x, lightGlobs.movePosition.y, lightGlobs.movePosition.z);
		Gods98::Container_SetPosition(lightGlobs.rootSpotlight, lightGlobs.rootLight,
									  _initialSpotlightPos.x, _initialSpotlightPos.y, _initialSpotlightPos.z);

		lightGlobs.movePosition = _initialSpotlightPos;
	}
}

// <LegoRR.exe @0044ca50>
void __cdecl LegoRR::LightEffects_SetDisabled(bool32 disabled)
{
	if (disabled) {
		lightGlobs.flags |= LIGHTFX_GLOB_FLAG_DISABLED;
		LightEffects_ResetSpotlightColour();
	}
	else {
		lightGlobs.flags &= ~LIGHTFX_GLOB_FLAG_DISABLED;
	}
}

// <LegoRR.exe @0044ca80>
bool32 __cdecl LegoRR::LightEffects_Load(const Gods98::Config* config, const char* gameName)
{
	/// CHANGE: Actually return the results of each load.
	bool result = true;
	result &= static_cast<bool>(LightEffects_LoadBlink(config, gameName));
	result &= static_cast<bool>(LightEffects_LoadFade(config, gameName));
	result &= static_cast<bool>(LightEffects_LoadMove(config, gameName));
	return result;
}

// <LegoRR.exe @0044cab0>
bool32 __cdecl LegoRR::LightEffects_LoadBlink(const Gods98::Config* config, const char* gameName)
{
	ColourRGBF rgbMax = { 0.0f }; // dummy init
	if (!Gods98::Config_GetRGBValue(config, LightEffects_ID("BlinkRGBMax"), &rgbMax.r, &rgbMax.g, &rgbMax.b))
		return false;

	real32 maxChange = Config_GetRealValue(config, LightEffects_ID("MaxChangeAllowed"));
	if (maxChange != 0.0f)
		maxChange /= 255.0f; // Colour channel to real.

	Range2F wait;
	if (!Gods98::Config_GetRange2F(config, LightEffects_ID("RandomRangeForTimeBetweenBlinks"), ":", &wait))
		return false;

	// Seconds to standard framerate.
	Gods98::Maths_Vector2DScale(&wait.xy, &wait.xy, STANDARD_FRAMERATE);


	LightEffects_SetBlink(rgbMax.r, rgbMax.g, rgbMax.b,
						  maxChange, wait.min, wait.max);

	return true;
}

// <LegoRR.exe @0044cc30>
void __cdecl LegoRR::LightEffects_SetBlink(real32 redMax, real32 greenMax, real32 blueMax,
										   real32 changeMax, real32 waitMin, real32 waitMax)
{
	lightGlobs.blinkRGBMax = ColourRGBF { redMax, greenMax, blueMax };
	lightGlobs.blinkChangeMax = changeMax;
	lightGlobs.blinkWaitRange = Range2F { waitMin, waitMax };
	lightGlobs.flags |= LIGHTFX_GLOB_FLAG_HASBLINK;
}

// <LegoRR.exe @0044cc80>
bool32 __cdecl LegoRR::LightEffects_LoadFade(const Gods98::Config* config, const char* gameName)
{
	ColourRGBF rgbMin = { 0.0f }; // dummy init
	if (!Gods98::Config_GetRGBValue(config, LightEffects_ID("FadeRGBMin"), &rgbMin.r, &rgbMin.g, &rgbMin.b))
		return false;

	ColourRGBF rgbMax = { 0.0f }; // dummy init
	if (!Gods98::Config_GetRGBValue(config, LightEffects_ID("FadeRGBMax"), &rgbMax.r, &rgbMax.g, &rgbMax.b))
		return false;

	Range2F wait;
	if (!Gods98::Config_GetRange2F(config, LightEffects_ID("RandomRangeForTimeBetweenFades"), ":", &wait))
		return false;

	Range2F speed;
	if (!Gods98::Config_GetRange2F(config, LightEffects_ID("RandomRangeForFadeTimeFade"), ":", &speed))
		return false;

	Range2F hold;
	if (!Gods98::Config_GetRange2F(config, LightEffects_ID("RandomRangeForHoldTimeOfFade"), ":", &hold))
		return false;

	// Seconds to standard framerate.
	Gods98::Maths_Vector2DScale(&wait.xy, &wait.xy, STANDARD_FRAMERATE);
	Gods98::Maths_Vector2DScale(&speed.xy, &speed.xy, STANDARD_FRAMERATE);
	Gods98::Maths_Vector2DScale(&hold.xy, &hold.xy, STANDARD_FRAMERATE);


	LightEffects_SetFade(rgbMin.r, rgbMin.g, rgbMin.b,
						 rgbMax.r, rgbMax.g, rgbMax.b,
						 wait.min, wait.max,
						 speed.min, speed.max,
						 hold.min, hold.max);

	return true;
}

// <LegoRR.exe @0044ced0>
void __cdecl LegoRR::LightEffects_SetFade(real32 redMin, real32 greenMin, real32 blueMin, real32 redMax, real32 greenMax, real32 blueMax,
										  real32 waitMin, real32 waitMax, real32 speedMin, real32 speedMax, real32 holdMin, real32 holdMax)
{
	lightGlobs.fadeRGBMin = ColourRGBF { redMin, greenMin, blueMin };
	lightGlobs.fadeRGBMax = ColourRGBF { redMax, greenMax, blueMax };
	lightGlobs.fadeWaitRange = Range2F { waitMin, waitMax };
	lightGlobs.fadeSpeedRange = Range2F { speedMin, speedMax };
	lightGlobs.fadeHoldRange = Range2F { holdMin, holdMax };
	lightGlobs.flags |= LIGHTFX_GLOB_FLAG_HASFADE;
}

// <LegoRR.exe @0044cf60>
bool32 __cdecl LegoRR::LightEffects_LoadMove(const Gods98::Config* config, const char* gameName)
{
	Range2F wait;
	if (!Gods98::Config_GetRange2F(config, LightEffects_ID("RandomRangeForTimeBetweenMoves"), ":", &wait))
		return false;

	Range2F speed;
	if (!Gods98::Config_GetRange2F(config, LightEffects_ID("RandomRangeForSpeedOfMove"), ":", &speed))
		return false;

	Range2F dist;
	if (!Gods98::Config_GetRange2F(config, LightEffects_ID("RandomRangeForDistOfMove"), ":", &dist))
		return false;

	Vector3F limit;
	if (!Gods98::Config_GetVector3F(config, LightEffects_ID("MoveLimit"), ":", &limit))
		return false;

	// Seconds to standard framerate (only wait and speed are time units).
	Gods98::Maths_Vector2DScale(&wait.xy, &wait.xy, STANDARD_FRAMERATE);
	Gods98::Maths_Vector2DScale(&speed.xy, &speed.xy, STANDARD_FRAMERATE);


	LightEffects_SetMove(wait.min, wait.max,
						 speed.min, speed.max,
						 dist.min, dist.max,
						 limit.x, limit.y, limit.z);

	return true;
}

// <LegoRR.exe @0044d1b0>
void __cdecl LegoRR::LightEffects_SetMove(real32 waitMin, real32 waitMax, real32 speedMin, real32 speedMax,
										  real32 distMin, real32 distMax, real32 limitX, real32 limitY, real32 limitZ)
{
	lightGlobs.moveWaitRange = Range2F { waitMin, waitMax };
	lightGlobs.moveSpeedRange = Range2F { speedMin, speedMax };
	lightGlobs.moveDistRange = Range2F { distMin, distMax };
	lightGlobs.moveLimit = Vector3F { limitX, limitY, limitZ };
	// Get movePosition.
	LightEffects_InvalidatePosition();
	// Get _initialSpotlightPos, so that we have the ORIGINAL spotlight relative position.
	if (lightGlobs.rootSpotlight != nullptr) {
		Gods98::Container_GetPosition(lightGlobs.rootSpotlight, lightGlobs.rootLight, &_initialSpotlightPos);
	}
	lightGlobs.flags |= LIGHTFX_GLOB_FLAG_HASMOVE;
}

// <LegoRR.exe @0044d230>
void __cdecl LegoRR::LightEffects_InvalidatePosition(void)
{
	/// SANITY: Null check, since its done elsewhere.
	// Get movePosition.
	if (lightGlobs.rootSpotlight != nullptr) {
		Gods98::Container_GetPosition(lightGlobs.rootSpotlight, lightGlobs.rootLight, &lightGlobs.movePosition);
	}
	lightGlobs.flags &= ~LIGHTFX_GLOB_FLAG_MOVING;
}

// <LegoRR.exe @0044d260>
void __cdecl LegoRR::LightEffects_Update(real32 elapsed)
{
	if (!(lightGlobs.flags & LIGHTFX_GLOB_FLAG_DISABLED) && lightGlobs.rootSpotlight != nullptr) {
		LightEffects_UpdateBlink(elapsed);
		LightEffects_UpdateFade(elapsed);
		LightEffects_UpdateMove(elapsed);
		LightEffects_UpdateDimmer(elapsed);
		LightEffects_UpdateSpotlightColour();
	}
}

// <LegoRR.exe @0044d2b0>
void __cdecl LegoRR::LightEffects_UpdateSpotlightColour(void)
{
	const real32 r = std::clamp(lightGlobs.currentRGB.r, 0.0f, 1.0f);
	const real32 g = std::clamp(lightGlobs.currentRGB.g, 0.0f, 1.0f);
	const real32 b = std::clamp(lightGlobs.currentRGB.b, 0.0f, 1.0f);

	Gods98::Container_SetColourAlpha(lightGlobs.rootSpotlight, r, g, b, 1.0f);
}

// <LegoRR.exe @0044d390>
void __cdecl LegoRR::LightEffects_UpdateBlink(real32 elapsed)
{
	if (!_blinkEnabled || !(lightGlobs.flags & LIGHTFX_GLOB_FLAG_HASBLINK))
		return; // Blink not setup, can't update.

	lightGlobs.blinkWait -= elapsed;
	if (lightGlobs.blinkWait > 0.0f)
		return; // Not done waiting, no need to update.

	lightGlobs.blinkWait = Gods98::Maths_RandRange(lightGlobs.blinkWaitRange.min,
												   lightGlobs.blinkWaitRange.max);

	/// CHANGE: Just use Maths_RandRange, why the extra work?
	const real32 value = Gods98::Maths_RandRange(0.0f, 1.0f);
	//const real32 value = ((sint32)Gods98::Maths_Rand() % 1000) / 1000.0f;

	// Range between [-Max,+Max]
	// rgb = randRange(blinkRGBMax * 2) - blinkRGBMax
	ColourRGBF rgb = { 0.0f }; // dummy init
	Gods98::Maths_Vector3DScale(&rgb.xyz, &lightGlobs.blinkRGBMax.xyz, (value * 2.0f));
	Gods98::Maths_Vector3DSubtract(&rgb.xyz, &rgb.xyz, &lightGlobs.blinkRGBMax.xyz);

	// Yup, it's just red...
	if (lightGlobs.blinkChangeMax <= std::abs(rgb.r + lightGlobs.blinkChange)) {
		Gods98::Maths_Vector3DNegate(&rgb.xyz);
		//LightEffects_FlipSign(&rgb.r);
		//LightEffects_FlipSign(&rgb.g);
		//LightEffects_FlipSign(&rgb.b);

		// Red again...
		if (lightGlobs.blinkChangeMax <= std::abs(rgb.r + lightGlobs.blinkChange))
			return;
	}

	Gods98::Maths_Vector3DAdd(&lightGlobs.currentRGB.xyz, &lightGlobs.currentRGB.xyz, &rgb.xyz);
	lightGlobs.blinkChange += rgb.r; // Yep, still only red...
}

// <LegoRR.exe @0044d510>
void __cdecl LegoRR::LightEffects_FlipSign(IN OUT real32* value)
{
	*value = -*value;
}

// <LegoRR.exe @0044d540>
void __cdecl LegoRR::LightEffects_UpdateFade(real32 elapsed)
{
	if (!_fadeEnabled || !(lightGlobs.flags & LIGHTFX_GLOB_FLAG_HASFADE))
		return; // Fade not setup, can't update.

	if (!(lightGlobs.flags & LIGHTFX_GLOB_FLAG_FADING)) {
		/// STATE: 1 START FADE

		lightGlobs.fadeWait -= elapsed;
		if (lightGlobs.fadeWait > 0.0f)
			return; // Not done fading, no need to update.

		lightGlobs.fadeWait = Gods98::Maths_RandRange(lightGlobs.fadeWaitRange.min,
													  lightGlobs.fadeWaitRange.max);

		lightGlobs.fadeHold = Gods98::Maths_RandRange(lightGlobs.fadeHoldRange.min,
													  lightGlobs.fadeHoldRange.max);

		lightGlobs.fadePosRGB = ColourRGBF { 0.0f, 0.0f, 0.0f };

		/// CHANGE: Just use Maths_RandRange, why the extra work?
		const real32 value = Gods98::Maths_RandRange(0.0f, 1.0f);
		//const real32 value = ((sint32)Gods98::Maths_Rand() % 1000) / 1000.0f;

		// fadeDestRGB = randRange(abs(fadeRGBMax - fadeRGBMin)) + fadeRGBMin;
		Gods98::Maths_Vector3DSubtract(&lightGlobs.fadeDestRGB.xyz, &lightGlobs.fadeRGBMax.xyz, &lightGlobs.fadeRGBMin.xyz);
		Gods98::Maths_Vector3DAbs(&lightGlobs.fadeDestRGB.xyz);

		Gods98::Maths_RayEndPoint(&lightGlobs.fadeDestRGB.xyz, &lightGlobs.fadeRGBMin.xyz, &lightGlobs.fadeDestRGB.xyz, value);
		//Gods98::Maths_Vector3DScale(&lightGlobs.fadeDestRGB.xyz, &lightGlobs.fadeDestRGB.xyz, value);
		//Gods98::Maths_Vector3DAdd(&lightGlobs.fadeDestRGB.xyz, &lightGlobs.fadeDestRGB.xyz, &lightGlobs.fadeRGBMin.xyz);

		LightEffects_RandomizeFadeSpeedRGB();
		lightGlobs.flags |= (LIGHTFX_GLOB_FLAG_FADE_FORWARD|LIGHTFX_GLOB_FLAG_FADING);
	}
	else if (lightGlobs.flags & LIGHTFX_GLOB_FLAG_FADE_FORWARD) {
		/// STATE: 2 FADING FORWARD

		// currentRGB += elapsed * fadeSpeedRGB;
		Gods98::Maths_RayEndPoint(&lightGlobs.currentRGB.xyz, &lightGlobs.currentRGB.xyz, &lightGlobs.fadeSpeedRGB.xyz, elapsed);

		// fadePosRGB += elapsed * fadeSpeedRGB;
		Gods98::Maths_RayEndPoint(&lightGlobs.fadePosRGB.xyz, &lightGlobs.fadePosRGB.xyz, &lightGlobs.fadeSpeedRGB.xyz, elapsed);

		// Is negative speed?
		if ((elapsed * lightGlobs.fadeSpeedRGB.r) < 0.0f) {

			if (lightGlobs.fadePosRGB.r <= lightGlobs.fadeDestRGB.r) {
				// Count up to fadeDestRGB.
				// currentRGB += (fadeDestRGB - fadePosRGB);
				Gods98::Maths_Vector3DAdd(&lightGlobs.currentRGB.xyz, &lightGlobs.currentRGB.xyz, &lightGlobs.fadeDestRGB.xyz);
				Gods98::Maths_Vector3DSubtract(&lightGlobs.currentRGB.xyz, &lightGlobs.currentRGB.xyz, &lightGlobs.fadePosRGB.xyz);

				lightGlobs.flags &= ~LIGHTFX_GLOB_FLAG_FADE_FORWARD;
			}
		}
		else {
			if (lightGlobs.fadePosRGB.r >= lightGlobs.fadeDestRGB.r) {
				// Count down to fadeDestRGB.
				// currentRGB -= (fadePosRGB - fadeDestRGB);
				// It's the exact same result as above, just with extra steps.
				Gods98::Maths_Vector3DSubtract(&lightGlobs.currentRGB.xyz, &lightGlobs.currentRGB.xyz, &lightGlobs.fadePosRGB.xyz);
				Gods98::Maths_Vector3DAdd(&lightGlobs.currentRGB.xyz, &lightGlobs.currentRGB.xyz, &lightGlobs.fadeDestRGB.xyz);

				lightGlobs.flags &= ~LIGHTFX_GLOB_FLAG_FADE_FORWARD;
			}
		}
	}
	else if (!(lightGlobs.flags & LIGHTFX_GLOB_FLAG_FADE_REVERSE)) {
		/// STATE: 3 START REVERSE

		lightGlobs.fadeHold -= elapsed;
		if (lightGlobs.fadeHold > 0.0f)
			return; // Not done holding, no need to update.

		LightEffects_RandomizeFadeSpeedRGB();

		// fadePosRGB = fadeDestRGB;
		lightGlobs.fadePosRGB = lightGlobs.fadeDestRGB;
		lightGlobs.flags |= LIGHTFX_GLOB_FLAG_FADE_REVERSE;
	}
	else {
		/// STATE: 4 FADING REVERSE

		// currentRGB -= elapsed * fadeSpeedRGB;
		Gods98::Maths_RayEndPoint(&lightGlobs.currentRGB.xyz, &lightGlobs.currentRGB.xyz, &lightGlobs.fadeSpeedRGB.xyz, -elapsed);

		// fadePosRGB -= elapsed * fadeSpeedRGB;
		Gods98::Maths_RayEndPoint(&lightGlobs.fadePosRGB.xyz, &lightGlobs.fadePosRGB.xyz, &lightGlobs.fadeSpeedRGB.xyz, -elapsed);

		// Is negative speed?
		if ((elapsed * lightGlobs.fadeSpeedRGB.r) < 0.0f) {

			if (lightGlobs.fadePosRGB.r >= 0.0f) {
				// Count down to zero.
				Gods98::Maths_Vector3DSubtract(&lightGlobs.currentRGB.xyz, &lightGlobs.currentRGB.xyz, &lightGlobs.fadePosRGB.xyz);
				//lightGlobs.currentRGB.r -= lightGlobs.fadePosRGB.r;
				//lightGlobs.currentRGB.g -= lightGlobs.fadePosRGB.g;
				//lightGlobs.currentRGB.b -= lightGlobs.fadePosRGB.b;
				lightGlobs.flags &= ~(LIGHTFX_GLOB_FLAG_FADE_REVERSE|LIGHTFX_GLOB_FLAG_FADING);
			}
		}
		else {
			if (lightGlobs.fadePosRGB.r <= 0.0f) {
				// Count up to zero.
				Gods98::Maths_Vector3DSubtract(&lightGlobs.currentRGB.xyz, &lightGlobs.currentRGB.xyz, &lightGlobs.fadePosRGB.xyz);
				//lightGlobs.currentRGB.r -= lightGlobs.fadePosRGB.r;
				//lightGlobs.currentRGB.g -= lightGlobs.fadePosRGB.g;
				//lightGlobs.currentRGB.b -= lightGlobs.fadePosRGB.b;
				lightGlobs.flags &= ~(LIGHTFX_GLOB_FLAG_FADE_REVERSE|LIGHTFX_GLOB_FLAG_FADING);
			}
		}
	}
}

// <LegoRR.exe @0044d9d0>
void __cdecl LegoRR::LightEffects_RandomizeFadeSpeedRGB(void)
{
	real32 value = Gods98::Maths_RandRange(lightGlobs.fadeSpeedRange.min,
										   lightGlobs.fadeSpeedRange.max);

	/// SANITY: Avoid divide by zero from someone messing with the config.
	if (value <= 0.0f) value = 0.000001f;

	// fadeSpeedRGB = fadeDestRGB / value;
	Gods98::Maths_Vector3DScale(&lightGlobs.fadeSpeedRGB.xyz, &lightGlobs.fadeDestRGB.xyz, (1.0f / value));
}

// <LegoRR.exe @0044da20>
void __cdecl LegoRR::LightEffects_UpdateMove(real32 elapsed)
{
	if (!_moveEnabled || !(lightGlobs.flags & LIGHTFX_GLOB_FLAG_HASMOVE))
		return; // Move not setup, can't update.

	if (!(lightGlobs.flags & LIGHTFX_GLOB_FLAG_MOVING)) {
		// Calculate a new movement direction + speed.
		lightGlobs.moveWait -= elapsed;
		if (lightGlobs.moveWait > 0.0f)
			return; // Not done waiting, no need to update.

		lightGlobs.moveWait = Gods98::Maths_RandRange(lightGlobs.moveWaitRange.min,
													  lightGlobs.moveWaitRange.max);

		lightGlobs.moveSpeed = Gods98::Maths_RandRange(lightGlobs.moveSpeedRange.min,
													   lightGlobs.moveSpeedRange.max);

		lightGlobs.moveDist = Gods98::Maths_RandRange(lightGlobs.moveDistRange.min,
													  lightGlobs.moveDistRange.max);

		// Create a speed vector in a random direction.
		// moveVector = setLength(randVec(), moveSpeed);
		/// SANITY: Ensure non-zero random vector.
		Gods98::Maths_Vector3DRandom(&lightGlobs.moveVector);
		if (Gods98::Maths_Vector3DModulus(&lightGlobs.moveVector) == 0.0f)
			lightGlobs.moveVector = Vector3F { 1.0f, 0.0f, 0.0f };

		Gods98::Maths_Vector3DSetLength(&lightGlobs.moveVector, &lightGlobs.moveVector, lightGlobs.moveSpeed);

		// New movement calculated, start moving next update
		lightGlobs.flags |= LIGHTFX_GLOB_FLAG_MOVING;
	}
	else {
		// Update current moving.
		Vector3F lastPos = { 0.0f }; // dummy init
		Gods98::Container_GetPosition(lightGlobs.rootSpotlight, lightGlobs.rootLight, &lastPos);

		Vector3F newPos; // dummy init
		Gods98::Maths_Vector3DAdd(&newPos, &lastPos, &lightGlobs.moveVector);

		// Ensure we haven't moved past the limit, and recalculate new movement we have.
		if (!LightEffects_CheckMoveLimit(&newPos)) {

			// This is the same deal as the `LightEffects_FlipSign(float*)` tuple seen in UpdateBlink,
			//  it's just that blink doesn't have extra math involved.
			Gods98::Maths_Vector3DSubtract(&newPos, &lastPos, &lightGlobs.moveVector);
			if (!LightEffects_CheckMoveLimit(&newPos)) {

				// Moved past limit, recalculate vector.
				lightGlobs.flags &= ~LIGHTFX_GLOB_FLAG_MOVING;
				return;
			}
		}

		// Update position.
		Gods98::Container_SetPosition(lightGlobs.rootSpotlight, lightGlobs.rootLight, newPos.x, newPos.y, newPos.z);

		lightGlobs.moveDist -= lightGlobs.moveSpeed;
		if (lightGlobs.moveDist > 0.0f)
			return; // Still moving, no need to update.

		// Done moving, calculate a new move direction.
		lightGlobs.flags &= ~LIGHTFX_GLOB_FLAG_MOVING;
	}
}

// <LegoRR.exe @0044dc60>
bool32 __cdecl LegoRR::LightEffects_CheckMoveLimit(const Vector3F* vector)
{
	if (std::abs(vector->x - lightGlobs.movePosition.x) > lightGlobs.moveLimit.x)
		return false;
	if (std::abs(vector->y - lightGlobs.movePosition.y) > lightGlobs.moveLimit.y)
		return false;
	if (std::abs(vector->z - lightGlobs.movePosition.z) > lightGlobs.moveLimit.z)
		return false;

	return true;
}

// <LegoRR.exe @0044dce0>
void __cdecl LegoRR::LightEffects_SetDimmerMode(bool32 isDimoutMode)
{
	if (isDimoutMode)
		lightGlobs.flags |= LIGHTFX_GLOB_FLAG_DIMOUT;
	else
		lightGlobs.flags &= ~LIGHTFX_GLOB_FLAG_DIMOUT;

	/// CHANGE: Just unset both of these flags since they're only used by their respective dimmer mode.
	lightGlobs.flags &= ~(LIGHTFX_GLOB_FLAG_DIMIN_DONE|LIGHTFX_GLOB_FLAG_DIMOUT_DONE);
}

// <LegoRR.exe @0044dd10>
void __cdecl LegoRR::LightEffects_UpdateDimmer(real32 elapsed)
{
	if (!(lightGlobs.flags & LIGHTFX_GLOB_FLAG_DIMOUT)) {
		// Dim-in mode.
		if (!(lightGlobs.flags & LIGHTFX_GLOB_FLAG_DIMIN_DONE)) {
			// currentRGB += (elapsed * 0.1);
			const real32 diminAmount = (elapsed / 0.1f);
			lightGlobs.currentRGB.r += diminAmount;
			lightGlobs.currentRGB.g += diminAmount;
			lightGlobs.currentRGB.b += diminAmount;

			if (lightGlobs.currentRGB.r >= lightGlobs.initialRGB.r) {
				/// FIX APPLY: Original code was assigning blue to red, after bounds check...
				/// CHANGE: Assign all channels to initialRGB instead of just red.
				// For all intents and purposes, there is no distinction between setting this now vs. during the 'done' update.
				lightGlobs.currentRGB = lightGlobs.initialRGB;

				lightGlobs.flags |= LIGHTFX_GLOB_FLAG_DIMIN_DONE; // end of mode, now do nothing
			}
		}
		else {
			// do nothing
		}
	}
	else {
		// Dim-out mode.
		if (!(lightGlobs.flags & LIGHTFX_GLOB_FLAG_DIMOUT_DONE)) {
			// currentRGB -= (elapsed * 0.1);
			const real32 dimoutAmount = (elapsed / 0.1f);
			lightGlobs.currentRGB.r -= dimoutAmount;
			lightGlobs.currentRGB.g -= dimoutAmount;
			lightGlobs.currentRGB.b -= dimoutAmount;

			if (lightGlobs.currentRGB.r <= 0.0f) {
				/// CHANGE: Assign all channels to zero instead of just red.
				/// CHANGE: Move zero assignment here instead of in the else block.
				// For all intents and purposes, there is no distinction between setting this now vs. during the 'done' update.
				lightGlobs.currentRGB = ColourRGBF { 0.0f, 0.0f, 0.0f };

				lightGlobs.flags |= LIGHTFX_GLOB_FLAG_DIMOUT_DONE; // end of mode, next time set currentRGB to 0
			}
		}
		else {
			//lightGlobs.currentRGB = ColourRGBF { 0.0f, 0.0f, 0.0f };
		}
	}
}



/// CUSTOM:
bool LegoRR::LightEffects_IsBlinkEnabled()
{
	return _blinkEnabled;
}

/// CUSTOM:
void LegoRR::LightEffects_SetBlinkEnabled(bool enabled)
{
	_blinkEnabled = enabled;
}

/// CUSTOM:
bool LegoRR::LightEffects_IsFadeEnabled()
{
	return _fadeEnabled;
}

/// CUSTOM:
void LegoRR::LightEffects_SetFadeEnabled(bool enabled)
{
	_fadeEnabled = enabled;
}

/// CUSTOM:
bool LegoRR::LightEffects_IsMoveEnabled()
{
	return _moveEnabled;
}

/// CUSTOM:
void LegoRR::LightEffects_SetMoveEnabled(bool enabled)
{
	_moveEnabled = enabled;
}

#pragma endregion
