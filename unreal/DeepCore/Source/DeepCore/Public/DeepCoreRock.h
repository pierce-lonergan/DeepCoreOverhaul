// DeepCoreRock.h : world-space rock displacement and strata.
//
// WHY DISPLACEMENT, AND WHY IN WORLD SPACE
// ----------------------------------------
// Silhouette dominates surface. The eye segments a scene by outline before it evaluates any
// shading, so a perfect procedural granite shader applied to a cube produces a granite-coloured
// cube. No amount of material work rescues axis-aligned boxes. Breaking the outline is therefore
// worth more than every shading improvement combined, and it has to happen first.
//
// THE CRACK PROBLEM, AND THE ONE RULE THAT SOLVES IT
// --------------------------------------------------
// The obvious way to rough up a surface is to push each vertex along its face normal. That
// splits open every shared edge in the mesh, because two faces meeting at a corner have
// different normals and push the same corner in two different directions.
//
// The fix is the rule this whole file is built on: DISPLACEMENT IS A PURE FUNCTION OF WORLD
// POSITION. Two vertices at the same world position always receive the same offset, no matter
// which tile, which face, or which draw call emitted them. Cracks become impossible by
// construction rather than by tolerance-fiddling, and neighbouring tiles stitch seamlessly
// without ever knowing about each other.
//
// GAMEPLAY IS NOT DISPLACED
// -------------------------
// The integer tile grid remains the sole authority for pathing, placement and collision. Only
// RENDER vertices move, and never by more than a fraction of a tile, so no displaced surface can
// cross a tile boundary and make a walkable tile look solid or vice versa. Collision is cooked
// from the undisplaced grid.
//

#pragma once

#include "CoreMinimal.h"

#include "DeepCoreTune.h"

namespace DeepCoreRock
{
	/** Maximum a render vertex may move. Well under half a tile, so topology is preserved. */
	static constexpr float MaxDisplace = 19.0f;

	/** Deterministic 3D value hash. Same input, same output, on every machine and every run. */
	FORCEINLINE float Hash3(int32 X, int32 Y, int32 Z)
	{
		uint32 H = (uint32)(X * 374761393) + (uint32)(Y * 668265263) + (uint32)(Z * 2147483647);
		H = (H ^ (H >> 13)) * 1274126177u;
		H ^= (H >> 16);
		return (float)(H & 0xFFFFFFu) / (float)0xFFFFFF;   // 0..1
	}

	/** Smooth value noise at a world point, period `Cell` cm. */
	FORCEINLINE float ValueNoise(const FVector& P, float Cell)
	{
		const FVector Q = P / Cell;
		const float Fx = FMath::FloorToFloat(Q.X), Fy = FMath::FloorToFloat(Q.Y), Fz = FMath::FloorToFloat(Q.Z);
		const int32 Ix = (int32)Fx, Iy = (int32)Fy, Iz = (int32)Fz;
		float Tx = Q.X - Fx, Ty = Q.Y - Fy, Tz = Q.Z - Fz;

		// Smoothstep the interpolants. Linear interpolation of value noise produces visible
		// grid-aligned creases, which is precisely the artefact we are here to remove.
		Tx = Tx * Tx * (3.0f - 2.0f * Tx);
		Ty = Ty * Ty * (3.0f - 2.0f * Ty);
		Tz = Tz * Tz * (3.0f - 2.0f * Tz);

		auto L = [](float A, float B, float T) { return A + (B - A) * T; };
		const float V00 = L(Hash3(Ix, Iy, Iz),         Hash3(Ix + 1, Iy, Iz),         Tx);
		const float V10 = L(Hash3(Ix, Iy + 1, Iz),     Hash3(Ix + 1, Iy + 1, Iz),     Tx);
		const float V01 = L(Hash3(Ix, Iy, Iz + 1),     Hash3(Ix + 1, Iy, Iz + 1),     Tx);
		const float V11 = L(Hash3(Ix, Iy + 1, Iz + 1), Hash3(Ix + 1, Iy + 1, Iz + 1), Tx);
		return L(L(V00, V10, Ty), L(V01, V11, Ty), Tz);
	}

	/** Several octaves, each finer and weaker. Roughly fractal, like a broken rock face. */
	FORCEINLINE float Fbm(const FVector& P, float Cell, int32 Octaves)
	{
		float Sum = 0.0f, Amp = 1.0f, Norm = 0.0f, C = Cell;
		for (int32 I = 0; I < Octaves; I++)
		{
			Sum  += ValueNoise(P, C) * Amp;
			Norm += Amp;
			Amp  *= 0.5f;
			C    *= 0.47f;   // not exactly 0.5, so octaves do not align into a visible lattice
		}
		return Sum / Norm;
	}

	/**
	 * How far the rock at this world point deviates from the ideal grid.
	 *
	 * Returns a full 3D offset rather than a scalar along a normal, because a vector field is
	 * what makes the result crack-free (see the file header) and because rock does not only
	 * bulge outward -- it also shears sideways along bedding and fracture planes.
	 */
	FORCEINLINE FVector Displace(const FVector& P)
	{
		// Two scales: metre-scale blockiness from blasting, and decimetre-scale rubble.
		const float Coarse = 190.0f;
		const float Fine   = 58.0f;

		// Offsetting the sample point per axis decorrelates the components; sampling the same
		// noise three times would push everything along the (1,1,1) diagonal.
		const FVector A(  0.0f,   0.0f,   0.0f);
		const FVector B(311.7f, 127.3f,  74.9f);
		const FVector C(-93.1f, 271.5f, 199.2f);

		FVector D(
			Fbm(P + A, Coarse, 3) - 0.5f,
			Fbm(P + B, Coarse, 3) - 0.5f,
			Fbm(P + C, Coarse, 3) - 0.5f);

		D += FVector(
			Fbm(P + B, Fine, 2) - 0.5f,
			Fbm(P + C, Fine, 2) - 0.5f,
			Fbm(P + A, Fine, 2) - 0.5f) * 0.45f;

		return D * (MaxDisplace * 2.0f * FDeepCoreTune::Get().Displace);
	}

	/**
	 * Per-vertex strata tint, 0.75..1.25 or so.
	 *
	 * Sedimentary and altered rock is banded, and horizontal banding is one of the strongest
	 * "this is real stone" cues available for free -- it is the thing that reads as geological
	 * history rather than as a texture. Bands are mostly a function of height, warped slightly by
	 * noise so they undulate instead of ruling perfectly level lines across the whole map.
	 */
	FORCEINLINE float Strata(const FVector& P)
	{
		const float Warp = (Fbm(P, 420.0f, 2) - 0.5f) * 130.0f;
		const float Band = FMath::Sin((P.Z + Warp) * 0.055f);
		// Squared so bands have soft wide bodies and tight dark partings, as bedding does.
		const float K = FDeepCoreTune::Get().Strata;
		const float Bedding = 1.0f - K * (0.14f - 0.14f * Band * Band);
		// Fine mottle stops any single band reading as a painted stripe.
		const float Mottle  = 1.0f - K * (0.10f - 0.20f * Fbm(P, 47.0f, 2));
		return Bedding * Mottle;
	}
}
