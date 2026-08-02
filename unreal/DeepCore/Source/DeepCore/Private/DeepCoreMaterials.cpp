#include "DeepCoreMaterials.h"

#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#endif

namespace
{
	FDeepCorePalette GPalette;
	bool GBuilt = false;

#if WITH_EDITOR
	/**
	 * Assemble one material graph.
	 *
	 * BaseColor always comes from vertex colour. Roughness is a constant because a single
	 * value across the whole world is the correct look here: moulded plastic is uniformly
	 * satin, and varying it would imply a surface story the geometry does not tell.
	 */
	UMaterial* BuildVertexColorMaterial(const TCHAR* Name, float Roughness, float EmissiveGain)
	{
		UMaterial* M = NewObject<UMaterial>(GetTransientPackage(), Name, RF_Transient | RF_Public);
		if (!M)
		{
			return nullptr;
		}

		UMaterialEditorOnlyData* Ed = M->GetEditorOnlyData();
		if (!Ed)
		{
			return nullptr;
		}

		UMaterialExpressionVertexColor* VC = NewObject<UMaterialExpressionVertexColor>(M);
		M->GetExpressionCollection().AddExpression(VC);
		Ed->BaseColor.Expression = VC;

		UMaterialExpressionConstant* Rough = NewObject<UMaterialExpressionConstant>(M);
		Rough->R = Roughness;
		M->GetExpressionCollection().AddExpression(Rough);
		Ed->Roughness.Expression = Rough;

		// Everything in this world is plastic and stone. Nothing is metal, and leaving
		// metallic at its default would be the single most common way to make a stylised
		// scene read as "wrong" without anyone being able to say why.
		UMaterialExpressionConstant* Metal = NewObject<UMaterialExpressionConstant>(M);
		Metal->R = 0.0f;
		M->GetExpressionCollection().AddExpression(Metal);
		Ed->Metallic.Expression = Metal;

		if (EmissiveGain > 0.0f)
		{
			UMaterialExpressionConstant* Gain = NewObject<UMaterialExpressionConstant>(M);
			Gain->R = EmissiveGain;
			M->GetExpressionCollection().AddExpression(Gain);

			UMaterialExpressionMultiply* Mul = NewObject<UMaterialExpressionMultiply>(M);
			Mul->A.Expression = VC;
			Mul->B.Expression = Gain;
			M->GetExpressionCollection().AddExpression(Mul);
			Ed->EmissiveColor.Expression = Mul;
		}

		M->SetShadingModel(MSM_DefaultLit);
		M->TwoSided = false;
		// No usage flag is set: UProceduralMeshComponent renders through the static-mesh
		// vertex factory, so the default usage already covers it.

		// Compiles the shader. Synchronous enough that the first frame is already correct,
		// and it happens once at startup rather than per level.
		M->PostEditChange();
		return M;
	}
#endif // WITH_EDITOR
}

const FDeepCorePalette& GetDeepCorePalette()
{
	if (GBuilt)
	{
		return GPalette;
	}
	GBuilt = true;

#if WITH_EDITOR
	if (UMaterial* Surface = BuildVertexColorMaterial(TEXT("DeepCoreSurface"), 0.72f, 0.0f))
	{
		Surface->AddToRoot();
		GPalette.Surface = Surface;
	}
	if (UMaterial* Glow = BuildVertexColorMaterial(TEXT("DeepCoreGlow"), 0.14f, 0.0f))
	{
		Glow->AddToRoot();
		GPalette.Glow = Glow;
	}
	GPalette.bLit = (GPalette.Surface != nullptr);
#endif

	if (!GPalette.Surface)
	{
		// Fallback: unlit, but at least vertex-coloured, so a Game target still renders a
		// recognisable world instead of grey default material.
		GPalette.Surface = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
		GPalette.bLit = false;
	}
	if (!GPalette.Glow)
	{
		GPalette.Glow = GPalette.Surface;
	}

	UE_LOG(LogTemp, Display, TEXT("DeepCore: material set built (lit=%s)"),
	       GPalette.bLit ? TEXT("yes") : TEXT("no - unlit fallback"));

	return GPalette;
}
