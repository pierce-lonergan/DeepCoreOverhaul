// DeepCoreTune.h : every look-affecting constant, in one place, settable from the command line.
//
// WHY THIS EXISTS
// ---------------
// Tuning a look by editing constants and rebuilding is roughly a two-minute round trip, which
// is slow enough that you stop trying things. Worse, it means only ONE combination exists at a
// time, so there is no way to compare A against B except from memory -- and memory is terrible
// at comparing images seen two minutes apart.
//
// With every knob on the command line, a rebuild is needed once and then any number of variants
// can be captured back to back and laid side by side. That turns "what does this look like"
// from an argument into a measurement, which is the only way this particular problem gets
// solved: the previous lighting pass failed precisely because each change was evaluated alone,
// against a frame that had several unrelated faults in it at the same time.
//
//   -DeepCoreTune=ev=3.2,sky=0.4,wl=900,lamp=1500,amb=0.03
//
// Unknown keys are reported rather than ignored, because a silently misspelled key produces a
// variant identical to the last one and invites the conclusion that the setting does nothing.
//

#pragma once

#include "CoreMinimal.h"

// MEASURED, NOT GUESSED. These defaults are the best combination found by capturing variants and
// measuring the resulting pixels (median brightness, fraction near-black, fraction clipped) rather
// than by eye. The measurement mattered: several rounds of "make it brighter / darker" tuning were
// chasing settings that turned out to be inert, and only a pixel histogram made that visible.
//
// KNOWN REMAINING FAULT: roughly 18% of pixels still clip to pure white. Exposure cannot fix it --
// a six-stop change moves the median by about one stop and leaves the clipped fraction almost
// unchanged -- because it is inverse-square falloff from point lamps close to now-tall walls. The
// fix is softer/larger emitters or many more much dimmer ones, not another exposure value.
struct FDeepCoreTune
{
	// --- exposure and atmosphere ---------------------------------------------------------
	/** EV100. HIGHER IS DARKER. See DeepCoreGame.cpp for why that is worth shouting about. */
	float Ev          = 5.0f;
	/** Sky light intensity. Near-useless in a sealed cave; kept as a knob to prove that. */
	float Sky         = 0.20f;
	float FogDensity  = 0.009f;
	float FogExtinct  = 0.9f;
	/** Volumetric fog scattering anisotropy, 0 = isotropic, ->1 = forward-scattering beams. */
	float FogAniso    = 0.75f;

	// --- lamps ---------------------------------------------------------------------------
	float Worklight   = 800.0f;   ///< candelas
	float WorklightZ  = 185.0f;   ///< height above the floor; the rock roof is at 240
	int32 WorklightStep = 3;      ///< one lamp per N x N tiles of opened ground
	float CapLamp     = 600.0f;   ///< candelas, per crew member

	/**
	 * Emissive ambient, as a fraction of a surface's own albedo.
	 *
	 * A deliberate cheat and labelled as one. In a sealed cave there is no sky to gather from,
	 * so unlit rock has literally nothing lighting it and falls to absolute black -- which is
	 * physically right and completely unplayable. A small self-emission proportional to albedo
	 * behaves like an infinitely-bounced ambient term: it lifts the floor without flattening
	 * the lamp pools, and because it scales with albedo it keeps dark rock dark.
	 *
	 * Set to 0 to see what the renderer alone produces.
	 */
	float Ambient     = 0.300f;

	// --- surface -------------------------------------------------------------------------
	float Roughness   = 0.72f;
	float AlbedoScale = 2.0f;     ///< multiplies every rock colour, for overall value control
	float Displace    = 1.0f;     ///< multiplies rock displacement; 0 gives flat tile faces
	float Strata      = 1.0f;     ///< multiplies the strength of the banding

	/**
	 * Height of the rock mass above the floor, in cm.
	 *
	 * This turned out to be the most important number in the whole look. At 240cm the walls
	 * were waist-high to the strategy camera 21m above them, so the view went straight over the
	 * top of the entire level and out into empty space: more than half of every frame was void
	 * rather than rock. That is why raising the ambient fill 1500-fold barely moved the image --
	 * ambient lights SURFACES, and most of the frame had no surface in it at all.
	 *
	 * Tall rock turns the map from a thin slab floating in black into a canyon system that
	 * encloses the camera, which is both what a mine actually is and the only way the frame
	 * fills with something to light.
	 */
	float RockHeight  = 800.0f;

	/** Populate from -DeepCoreTune=k=v,k=v. Logs what it parsed and what it did not recognise. */
	static void ParseCommandLine();

	/** The one instance. Read everywhere; written only by ParseCommandLine. */
	static FDeepCoreTune& Get();
};
