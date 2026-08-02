#include "DeepCoreTextures.h"

#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	TMap<FString, TObjectPtr<UTexture2D>> GCache;
	TMap<FString, float> GMeanCache;

	/**
	 * Average linear luminance of mip 0.
	 *
	 * Read from the mip rather than tracked alongside the generator, so the number always describes
	 * the PNG actually on disk -- regenerate a lithology with a different prompt and this follows it
	 * with no second place to update. The bytes are sRGB-encoded (SRGB=true below), and the sampler
	 * decodes them before the shader ever sees them, so they are decoded here too; averaging the raw
	 * bytes would report a mean roughly a stop and a half too high on a dark rock.
	 */
	float ComputeMeanLinearLuma(UTexture2D* Tex)
	{
		FTexturePlatformData* PD = Tex ? Tex->GetPlatformData() : nullptr;
		if (!PD || PD->Mips.Num() == 0) { return 0.5f; }

		FTexture2DMipMap& Mip = PD->Mips[0];
		const int32 W = Mip.SizeX, H = Mip.SizeY;
		if (W <= 0 || H <= 0) { return 0.5f; }

		const uint8* Src = static_cast<const uint8*>(Mip.BulkData.LockReadOnly());
		if (!Src) { Mip.BulkData.Unlock(); return 0.5f; }

		// ImportBufferAsTexture2D produces BGRA8.
		double Sum = 0.0;
		const int64 Count = static_cast<int64>(W) * H;
		for (int64 i = 0; i < Count; ++i)
		{
			const FColor C(Src[i * 4 + 2], Src[i * 4 + 1], Src[i * 4 + 0], 255);
			const FLinearColor Lin(C);   // sRGB -> linear, matching what the sampler yields
			Sum += 0.2126 * Lin.R + 0.7152 * Lin.G + 0.0722 * Lin.B;
		}
		Mip.BulkData.Unlock();

		return FMath::Clamp(static_cast<float>(Sum / static_cast<double>(Count)), 0.01f, 1.0f);
	}
}

float GetGeneratedTextureMean(const FString& Name)
{
	if (const float* Found = GMeanCache.Find(Name)) { return *Found; }
	GetGeneratedTexture(Name);   // populates GMeanCache as a side effect
	const float* Found = GMeanCache.Find(Name);
	return Found ? *Found : 0.5f;
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

	const float Mean = ComputeMeanLinearLuma(Tex);
	GMeanCache.Add(Name, Mean);

	UE_LOG(LogTemp, Display, TEXT("DeepCore: loaded texture %s (%dx%d) meanLinearLuma=%.4f"),
	       *Name, Tex->GetSizeX(), Tex->GetSizeY(), Mean);
	GCache.Add(Name, Tex);
	return Tex;
}
