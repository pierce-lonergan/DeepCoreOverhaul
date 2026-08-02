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
