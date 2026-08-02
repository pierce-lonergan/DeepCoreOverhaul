#include "DeepCoreMaterials.h"

#include "DeepCoreTune.h"

#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "DeepCoreTextures.h"
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

	/**
	 * Shared HLSL prelude: a cheap hash-based value noise and an fBm on top of it.
	 *
	 * Inlined as a string rather than pulled from a .ush, because IncludeFilePaths needs a real
	 * file on disk registered as a virtual shader path -- which would put a shipped content file
	 * back into a project whose whole premise is that it has none.
	 */
	static const TCHAR* kNoiseHLSL = TEXT(
		"float dcHash(float3 p)\n"
		"{\n"
		"	p = frac(p * 0.3183099 + float3(0.71, 0.113, 0.419));\n"
		"	p *= 17.0;\n"
		"	return frac(p.x * p.y * p.z * (p.x + p.y + p.z));\n"
		"}\n"
		"float dcValue(float3 x)\n"
		"{\n"
		"	float3 i = floor(x);\n"
		"	float3 f = frac(x);\n"
		"	f = f * f * (3.0 - 2.0 * f);\n"
		"	float n000 = dcHash(i + float3(0,0,0)), n100 = dcHash(i + float3(1,0,0));\n"
		"	float n010 = dcHash(i + float3(0,1,0)), n110 = dcHash(i + float3(1,1,0));\n"
		"	float n001 = dcHash(i + float3(0,0,1)), n101 = dcHash(i + float3(1,0,1));\n"
		"	float n011 = dcHash(i + float3(0,1,1)), n111 = dcHash(i + float3(1,1,1));\n"
		"	return lerp(lerp(lerp(n000,n100,f.x), lerp(n010,n110,f.x), f.y),\n"
		"	            lerp(lerp(n001,n101,f.x), lerp(n011,n111,f.x), f.y), f.z);\n"
		"}\n"
		"float dcFbm(float3 p)\n"
		"{\n"
		"	float s = 0.0, a = 0.5;\n"
		"	for (int i = 0; i < 5; i++) { s += a * dcValue(p); p *= 2.03; a *= 0.5; }\n"
		"	return s;\n"
		"}\n");

	/**
	 * Publish the noise prelude as a preprocessor define instead of prepending it to Code.
	 *
	 * Code is emitted INSIDE the body of a generated function -- HLSLMaterialTranslator.cpp:14338
	 * wraps it in `CustomExpression%d(FMaterialPixelParameters Parameters, ...) { <Code> }` -- so an
	 * HLSL *function definition* placed there is illegal. The compiler reports "function definition
	 * is not allowed here", every later dcFbm call then fails as an undeclared identifier, and the
	 * whole material silently falls back to the engine Default Material. That fallback ignores
	 * vertex colour, which is what made the frame achromatic and every tuning knob inert.
	 *
	 * AdditionalDefines are emitted at GLOBAL scope instead, ahead of the wrapper
	 * (HLSLMaterialTranslator.cpp:14318-14325), and the translator wraps each one in
	 * `#ifndef NAME / #define NAME <value> / #endif` -- so all three Custom nodes can request the
	 * same block and the guard makes the repeat inclusion harmless. The value starts "1\n" so the
	 * #define directive terminates at that newline and the function bodies land as plain global HLSL.
	 */
	void AddNoiseDefine(UMaterialExpressionCustom* C)
	{
		FCustomDefine D;
		D.DefineName = TEXT("DC_NOISE");
		D.DefineValue = FString(TEXT("1\n")) + kNoiseHLSL;
		C->AdditionalDefines.Add(D);
	}

	/** Wire a Custom node input to an expression. */
	void AddInput(UMaterialExpressionCustom* C, const TCHAR* Name, UMaterialExpression* Expr)
	{
		FCustomInput In;
		In.InputName = FName(Name);
		In.Input.Expression = Expr;
		C->Inputs.Add(In);
	}

	UMaterialExpressionConstant* MakeConst(UMaterial* M, float V)
	{
		UMaterialExpressionConstant* C = NewObject<UMaterialExpressionConstant>(M);
		C->R = V;
		M->GetExpressionCollection().AddExpression(C);
		return C;
	}


	/**
	 * The rock material: procedural stone from Custom HLSL nodes. No textures, no UVs.
	 *
	 * Three things here matter more than everything else in the look, in order:
	 *
	 *  1. A NORMAL derived from the noise. The geometry is flat facets, and no amount of colour
	 *     variation rescues a flat facet -- it is the shading normal that tells the eye a
	 *     surface is broken. This is the cheapest realism available: it adds no triangles.
	 *  2. ROUGHNESS varying with the SAME noise as the colour. Constant roughness is the
	 *     definitive CGI tell, and roughness varying INDEPENDENTLY of colour reads as polished
	 *     marble. The correlation is what makes it read as one physical material.
	 *  3. Fracture partings. Real rock is crossed by thin dark lines, and the eye reads those
	 *     as evidence of a history the surface has been through.
	 *
	 * Everything keys off WORLD POSITION, so there are no UVs to stretch and no seam between one
	 * tile and the next -- which matters because FBrickMesh emits an arbitrary (0,0)-(1,1) UV per
	 * quad regardless of that quad's world size, making any UV-based approach hopeless here.
	 */
	UMaterial* BuildRockMaterial(const TCHAR* Name, const FDeepCoreTune& T, bool bSmooth)
	{
		UMaterial* M = NewObject<UMaterial>(GetTransientPackage(), Name, RF_Transient | RF_Public);
		if (!M) { return nullptr; }
		UMaterialEditorOnlyData* Ed = M->GetEditorOnlyData();
		if (!Ed) { return nullptr; }

		UMaterialExpressionWorldPosition* WP = NewObject<UMaterialExpressionWorldPosition>(M);
		M->GetExpressionCollection().AddExpression(WP);
		UMaterialExpressionVertexNormalWS* VN = NewObject<UMaterialExpressionVertexNormalWS>(M);
		M->GetExpressionCollection().AddExpression(VN);
		UMaterialExpressionVertexColor* VC = NewObject<UMaterialExpressionVertexColor>(M);
		M->GetExpressionCollection().AddExpression(VC);

		const float Inv = 1.0f / FMath::Max(10.0f, T.RockScale);

		// The generated rock photograph, as a texture object the Custom node can sample.
		UMaterialExpressionTextureObjectParameter* RockTex =
			NewObject<UMaterialExpressionTextureObjectParameter>(M);
		RockTex->ParameterName = TEXT("RockTex");
		const TCHAR* RockTexName = bSmooth ? TEXT("pegmatite") : TEXT("granodiorite");
		RockTex->Texture = GetGeneratedTexture(RockTexName);
		RockTex->SamplerType = SAMPLERTYPE_Color;
		M->GetExpressionCollection().AddExpression(RockTex);

		// --- base colour -----------------------------------------------------------------
		{
			UMaterialExpressionCustom* C = NewObject<UMaterialExpressionCustom>(M);
			C->OutputType = CMOT_Float3;
			C->Description = TEXT("RockAlbedo");
			AddNoiseDefine(C);
			C->Code =
				"float3 p = WP * Inv;\n"
				"// TRIPLANAR. There are no usable UVs on this mesh -- FBrickMesh emits an\n"
				"// arbitrary (0,0)-(1,1) per quad regardless of world size -- so the texture is\n"
				"// projected from the three world axes and blended by the surface normal. The\n"
				"// exponent sharpens the blend; too low and every surface looks doubly-exposed.\n"
				"float3 tp = WP * TexScale;\n"
				"float3 bw = pow(abs(normalize(VNn)), 6.0);\n"
				"bw /= max(bw.x + bw.y + bw.z, 1e-4);\n"
				"float3 tx = Texture2DSample(Tex, TexSampler, tp.yz).rgb * bw.x\n"
				"         + Texture2DSample(Tex, TexSampler, tp.xz).rgb * bw.y\n"
				"         + Texture2DSample(Tex, TexSampler, tp.xy).rgb * bw.z;\n"
				"// The texture carries GRAIN; the vertex colour carries which rock this is and\n"
				"// how occluded it is. Multiplying keeps both: tinting a grey photo by the\n"
				"// lithology colour is what makes one texture serve every rock class.\n"
				"// Normalise by the image's CONSTANT mean so the sample swings about 1.0 and\n"
				"// modulates the authored albedo. Dividing by the per-pixel mean instead --\n"
				"// dot(tx, 0.333) -- drives every sample to exactly 1.0 and deletes the grain:\n"
				"// measured, turning the texture off then moved the frame by a mean of 0.999/255\n"
				"// over 2% of pixels, i.e. the generated textures were doing nothing at all.\n"
				"tx /= max(TexMean, 1e-3);\n"
				"float broad = dcFbm(p * 0.5);\n"
				"float grain = dcFbm(p * 6.0);\n"
				"// Ridged noise isolates thin LINES rather than blobs: partings, not clouds.\n"
				"float ridge = 1.0 - abs(dcFbm(p * 1.6) * 2.0 - 1.0);\n"
				"float parting = smoothstep(0.80, 1.0, ridge);\n"
				"float mott = lerp(1.0, 0.60 + 0.85 * broad, Mottle);\n"
				"float grit = lerp(1.0, 0.82 + 0.34 * grain, Mottle);\n"
				"float3 c = VC * mott * grit;\n"
				"// Partings go dark and slightly cool, as damp fracture faces do.\n"
				"c = lerp(c, c * float3(0.30, 0.32, 0.36), parting * Mottle);\n"
				"c *= lerp(1.0.xxx, tx, saturate(TexAmt));\n"
				"return max(c, 0.0);\n";
			AddInput(C, TEXT("WP"), WP);
			AddInput(C, TEXT("VC"), VC);
			AddInput(C, TEXT("Inv"), MakeConst(M, Inv));
			AddInput(C, TEXT("Mottle"), MakeConst(M, T.Mottle));
			AddInput(C, TEXT("Tex"), RockTex);
			AddInput(C, TEXT("TexMean"), MakeConst(M, GetGeneratedTextureMean(RockTexName)));
			AddInput(C, TEXT("TexScale"), MakeConst(M, T.TexScale));
			AddInput(C, TEXT("TexAmt"), MakeConst(M, T.TexAmount));
			AddInput(C, TEXT("VNn"), VN);
			M->GetExpressionCollection().AddExpression(C);
			Ed->BaseColor.Expression = C;
		}

		// --- roughness -------------------------------------------------------------------
		{
			UMaterialExpressionCustom* C = NewObject<UMaterialExpressionCustom>(M);
			C->OutputType = CMOT_Float1;
			C->Description = TEXT("RockRoughness");
			AddNoiseDefine(C);
			C->Code =
				"float3 p = WP * Inv;\n"
				"// Deliberately the SAME frequencies the albedo uses, so a patch that reads\n"
				"// darker also reads rougher. That correlation is the whole point.\n"
				"float broad = dcFbm(p * 0.5);\n"
				"float grain = dcFbm(p * 6.0);\n"
				"float ridge = 1.0 - abs(dcFbm(p * 1.6) * 2.0 - 1.0);\n"
				"float parting = smoothstep(0.80, 1.0, ridge);\n"
				"float r = Base + Var * (0.34 * (broad - 0.5) + 0.16 * (grain - 0.5));\n"
				"// Fracture faces are freshly broken and damp: smoother than weathered rock.\n"
				"r = lerp(r, r * 0.45, parting * Var);\n"
				"return clamp(r, 0.04, 1.0);\n";
			AddInput(C, TEXT("WP"), WP);
			AddInput(C, TEXT("Inv"), MakeConst(M, Inv));
			AddInput(C, TEXT("Base"), MakeConst(M, bSmooth ? T.Roughness * 0.25f : T.Roughness));
			AddInput(C, TEXT("Var"), MakeConst(M, T.RoughVar));
			M->GetExpressionCollection().AddExpression(C);
			Ed->Roughness.Expression = C;
		}

		// --- world-space normal ----------------------------------------------------------
		{
			UMaterialExpressionCustom* C = NewObject<UMaterialExpressionCustom>(M);
			C->OutputType = CMOT_Float3;
			C->Description = TEXT("RockNormal");
			AddNoiseDefine(C);
			C->Code =
				"float3 p = WP * Inv;\n"
				"float3 N = normalize(VN);\n"
				"// Finite differences of the height field give its gradient. Projecting that\n"
				"// gradient onto the surface plane leaves only the part that TILTS the normal;\n"
				"// the component along N would merely scale it and do nothing visible.\n"
				"float e = 0.45;\n"
				"float h  = dcFbm(p * 2.0) + 0.5 * dcFbm(p * 8.0);\n"
				"float hx = dcFbm((p + float3(e,0,0)) * 2.0) + 0.5 * dcFbm((p + float3(e,0,0)) * 8.0);\n"
				"float hy = dcFbm((p + float3(0,e,0)) * 2.0) + 0.5 * dcFbm((p + float3(0,e,0)) * 8.0);\n"
				"float hz = dcFbm((p + float3(0,0,e)) * 2.0) + 0.5 * dcFbm((p + float3(0,0,e)) * 8.0);\n"
				"float3 g = float3(hx - h, hy - h, hz - h) / e;\n"
				"g = g - N * dot(g, N);\n"
				"return normalize(N - g * Strength);\n";
			AddInput(C, TEXT("WP"), WP);
			AddInput(C, TEXT("VN"), VN);
			AddInput(C, TEXT("Inv"), MakeConst(M, Inv));
			AddInput(C, TEXT("Strength"), MakeConst(M, T.Bump));
			M->GetExpressionCollection().AddExpression(C);
			Ed->Normal.Expression = C;
		}

		// The normal above is already in WORLD space. Declaring that avoids the tangent basis
		// entirely, which matters because FBrickMesh synthesises tangents as an arbitrary cross
		// product with no relation to any UV layout.
		M->bTangentSpaceNormal = false;

		Ed->Metallic.Expression = MakeConst(M, 0.0f);

		if (T.Ambient > 0.0f)
		{
			UMaterialExpressionMultiply* Mul = NewObject<UMaterialExpressionMultiply>(M);
			Mul->A.Expression = VC;
			Mul->B.Expression = MakeConst(M, T.Ambient);
			M->GetExpressionCollection().AddExpression(Mul);
			Ed->EmissiveColor.Expression = Mul;
		}

		M->SetShadingModel(MSM_DefaultLit);
		M->TwoSided = false;
		M->PostEditChange();
		return M;
	}

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
	const FDeepCoreTune& T = FDeepCoreTune::Get();
	if (UMaterial* Surface = BuildRockMaterial(TEXT("DeepCoreSurface"), T, false))
	{
		Surface->AddToRoot();
		GPalette.Surface = Surface;
	}
	if (UMaterial* Glow = BuildRockMaterial(TEXT("DeepCoreGlow"), T, true))
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
