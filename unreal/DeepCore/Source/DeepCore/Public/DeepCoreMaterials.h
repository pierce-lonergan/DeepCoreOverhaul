// DeepCoreMaterials.h : materials built in C++ at startup, so the project ships no assets.
//
// WHY THIS FILE EXISTS
// --------------------
// The whole point of moving to Unreal is the renderer: Lumen bounce light, real shadow maps,
// temporal anti-aliasing, tonemapping. All of that only applies to LIT materials. The engine
// does ship a vertex-colour material, but it lives in EngineDebugMaterials and is unlit --
// using it would give flat colour with no lighting, which is exactly what the OpenGL build
// already did. Adopting it would mean paying for Unreal and getting nothing back.
//
// The alternative is authoring a .uasset, and a .uasset is a binary blob that cannot be
// written by hand, reviewed in a diff, or regenerated from source. This project's rule is that
// everything in the repository is text that a human can read.
//
// So the material is constructed as a node graph in code and compiled at startup. Roughly
// twenty lines of C++ replace a binary asset, stay reviewable, and produce a genuinely lit
// surface driven by the per-vertex colour the brick builder writes.
//
// THE CATCH, STATED PLAINLY
// -------------------------
// Building a material graph is editor-only API: it needs the shader compiler, which is not
// present in a cooked shipping build. That is why the game is launched through the editor
// binary in -game mode. A cooked build would need these materials saved as real assets first.
// The fallback path below keeps the code compiling for a Game target, but it falls back to the
// unlit engine material and will look worse; it exists so the target builds, not so it ships.
//

#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;

/** The material set the game draws with. Built once, cached for the process lifetime. */
struct FDeepCorePalette
{
	/** Lit, vertex-coloured, dielectric. Terrain, crew, creatures -- almost everything. */
	UMaterialInterface* Surface = nullptr;

	/**
	 * Lit and very smooth, for mineral veins.
	 *
	 * Deliberately NOT emissive. Self-illuminating minerals are a fantasy-game convention and
	 * they were the loudest remaining toy signal in the frame: against a dark mine they clipped
	 * to pure white and bloomed, reading as glowing plastic gems. Real pegmatite and sulphide
	 * are simply SMOOTHER than their host rock, so they flare when a cap lamp sweeps across them
	 * and vanish when it moves on. That is better gameplay too -- finding ore becomes something
	 * you do by pointing a light at it.
	 */
	UMaterialInterface* Glow = nullptr;

	/** True when the lit graphs were built; false when the unlit fallback is in use. */
	bool bLit = false;
};

/**
 * Build (or return the cached) material set.
 *
 * Safe to call from any actor's BeginPlay; the first call does the work and the rest are a
 * pointer read. The materials are rooted so garbage collection cannot take them.
 */
const FDeepCorePalette& GetDeepCorePalette();
