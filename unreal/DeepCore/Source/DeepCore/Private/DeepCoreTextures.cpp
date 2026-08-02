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
	 * Build a full mip chain for a runtime-imported texture.
	 *
	 * ImportBufferAsTexture2D returns a SINGLE-mip texture, and a single-mip texture has nothing to
	 * fall back to when it is minified: every screen pixel takes one point sample of a
	 * high-frequency image, and the result aliases. On rock at a 45 cm repeat that reads as a
	 * checkerboard moire across the lit faces. It was present from the moment the textures were
	 * introduced but invisible while they contributed nothing, and appeared the instant the
	 * normaliser was corrected -- a reminder that fixing one defect can reveal rather than cause
	 * the next one.
	 *
	 * Averaged in LINEAR space. Box-filtering sRGB bytes directly darkens every successive mip,
	 * because the mean of two encoded values is not the encoding of their mean.
	 */
	void BuildMipChain(UTexture2D* Tex)
	{
		FTexturePlatformData* PD = Tex ? Tex->GetPlatformData() : nullptr;
		if (!PD || PD->Mips.Num() != 1) { return; }

		int32 W = PD->Mips[0].SizeX, H = PD->Mips[0].SizeY;
		if (W < 2 || H < 2) { return; }

		TArray<FColor> Cur;
		Cur.SetNumUninitialized(W * H);
		{
			const void* Src = PD->Mips[0].BulkData.LockReadOnly();
			if (!Src) { PD->Mips[0].BulkData.Unlock(); return; }
			FMemory::Memcpy(Cur.GetData(), Src, static_cast<SIZE_T>(W) * H * 4);
			PD->Mips[0].BulkData.Unlock();
		}

		while (W > 1 || H > 1)
		{
			const int32 PW = W, PH = H;
			W = FMath::Max(1, W / 2);
			H = FMath::Max(1, H / 2);

			TArray<FColor> Next;
			Next.SetNumUninitialized(W * H);
			for (int32 y = 0; y < H; ++y)
			{
				for (int32 x = 0; x < W; ++x)
				{
					const int32 x0 = FMath::Min(x * 2, PW - 1), x1 = FMath::Min(x * 2 + 1, PW - 1);
					const int32 y0 = FMath::Min(y * 2, PH - 1), y1 = FMath::Min(y * 2 + 1, PH - 1);
					const FLinearColor L =
						(FLinearColor(Cur[y0 * PW + x0]) + FLinearColor(Cur[y0 * PW + x1]) +
						 FLinearColor(Cur[y1 * PW + x0]) + FLinearColor(Cur[y1 * PW + x1])) * 0.25f;
					Next[y * W + x] = L.ToFColor(true);   // re-encode, matching SRGB=true
				}
			}

			FTexture2DMipMap* Mip = new FTexture2DMipMap(W, H, 1);
			PD->Mips.Add(Mip);
			Mip->BulkData.Lock(LOCK_READ_WRITE);
			void* Dst = Mip->BulkData.Realloc(static_cast<int64>(W) * H * 4);
			FMemory::Memcpy(Dst, Next.GetData(), static_cast<SIZE_T>(W) * H * 4);
			Mip->BulkData.Unlock();

			Cur = MoveTemp(Next);
		}
	}

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
	// The chain is generated here rather than streamed from disk, so it must stay resident;
	// a streaming texture would try to load mips from a package that does not exist.
	Tex->NeverStream = true;

	const float Mean = ComputeMeanLinearLuma(Tex);   // from mip 0, before the chain is appended
	GMeanCache.Add(Name, Mean);

	BuildMipChain(Tex);
	Tex->UpdateResource();

	UE_LOG(LogTemp, Display, TEXT("DeepCore: loaded texture %s (%dx%d) mips=%d meanLinearLuma=%.4f"),
	       *Name, Tex->GetSizeX(), Tex->GetSizeY(),
	       Tex->GetPlatformData() ? Tex->GetPlatformData()->Mips.Num() : 0, Mean);
	GCache.Add(Name, Tex);
	return Tex;
}
