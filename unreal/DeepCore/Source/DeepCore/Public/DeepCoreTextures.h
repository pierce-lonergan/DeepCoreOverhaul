// DeepCoreTextures.h : load generated PNGs into textures at runtime.
//
// The textures are produced by tools/gen/genimage.py and live in content/textures as ordinary
// PNGs. They are loaded here at startup with FImageUtils rather than imported as .uasset, which
// keeps the project's zero-cooked-asset property intact: the repository contains a generator, a
// prompt and a PNG, and every one of those is inspectable without opening the editor.
//
// A PNG on disk is also regenerable. If a lithology looks wrong, the fix is a prompt edit and a
// re-run, not an art request -- which is the whole reason this path is worth having.

#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** Load content/textures/<Name>.png. Returns nullptr and logs if missing. Cached per name. */
UTexture2D* GetGeneratedTexture(const FString& Name);

/**
 * Mean LINEAR luminance of that texture, in 0..1. Returns 0.5 if the texture is missing.
 *
 * The material needs this to use a photograph as a detail multiplier without also using it as a
 * brightness change: dividing the sample by the image's average makes the result swing about 1.0,
 * so it modulates the authored albedo instead of replacing it. The average has to be a CONSTANT
 * for that to work. Dividing each pixel by its own mean -- which is what the shader did before --
 * forces every sample to 1.0 and deletes exactly the grain the texture was fetched for.
 *
 * Linear, not sRGB: the sampler hands the shader linear values, so the normaliser must match.
 */
float GetGeneratedTextureMean(const FString& Name);
