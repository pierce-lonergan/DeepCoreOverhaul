#include "DeepCoreTextures.h"

#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	TMap<FString, TObjectPtr<UTexture2D>> GCache;
}

UTexture2D* GetGeneratedTexture(const FString& Name)
{
	if (TObjectPtr<UTexture2D>* Found = GCache.Find(Name))
	{
		return Found->Get();
	}

	// ProjectDir is unreal/DeepCore/; the textures live two levels up in the repo proper, so
	// that they sit next to the generator that makes them rather than inside the UE project.
	const FString Path = FPaths::ProjectDir() / TEXT("../../content/textures") / (Name + TEXT(".png"));

	TArray<uint8> Raw;
	if (!FFileHelper::LoadFileToArray(Raw, *Path))
	{
		UE_LOG(LogTemp, Warning, TEXT("DeepCore: texture not found: %s"), *FPaths::ConvertRelativePathToFull(Path));
		GCache.Add(Name, nullptr);
		return nullptr;
	}

	UTexture2D* Tex = FImageUtils::ImportBufferAsTexture2D(Raw);
	if (!Tex)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeepCore: texture failed to decode: %s"), *Name);
		GCache.Add(Name, nullptr);
		return nullptr;
	}

	// Rooted because these are transient objects with no package referencing them; without this
	// they are collected the first time GC runs and the material samples a null texture.
	Tex->AddToRoot();
	Tex->SRGB = true;
	Tex->UpdateResource();

	UE_LOG(LogTemp, Display, TEXT("DeepCore: loaded texture %s (%dx%d)"),
	       *Name, Tex->GetSizeX(), Tex->GetSizeY());
	GCache.Add(Name, Tex);
	return Tex;
}
