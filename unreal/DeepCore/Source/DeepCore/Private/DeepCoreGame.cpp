#include "DeepCoreGame.h"

#include "Camera/CameraComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "Components/SkyLightComponent.h"
#include "DeepCoreTerrain.h"
#include "DeepCoreTune.h"
#include "DeepCoreUnit.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "UnrealClient.h"
#include "SyntheticLevel.hpp"

using namespace Sandbox;

/**********************************************************************************
 ******** Camera
 **********************************************************************************/

ADeepCorePawn::ADeepCorePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
	Arm->SetupAttachment(RootComponent);
	Arm->TargetArmLength = Boom;
	Arm->SetRelativeRotation(FRotator(-52.0f, 0.0f, 0.0f));
	// No lag and no collision: this is a strategy camera, and an arm that dodges terrain or
	// smooths its own motion makes precise ordering feel imprecise.
	Arm->bDoCollisionTest = false;
	Arm->bEnableCameraLag = false;

	Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Cam"));
	Cam->SetupAttachment(Arm, USpringArmComponent::SocketName);

	// A narrow lens, and this is the single most important camera setting in the game.
	//
	// The engine default is 90 degrees, which is a first-person field of view. From a strategy
	// camera 11m up it takes in the ENTIRE 48x40 map, rakes the ground plane away to the
	// horizon, and compresses the far half into a few pixels -- so walls, floor and unexplored
	// rock all flatten into one texture and none of the brickwork reads. 38 degrees is close to
	// the long lens a tabletop diorama is photographed with: near-parallel sightlines, minimal
	// convergence, and a bounded area that the player can actually parse.
	Cam->SetFieldOfView(38.0f);

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void ADeepCorePawn::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
	}

	// -DeepCoreBoom=<cm> overrides the start distance. This exists for verification captures:
	// checking that the figures animate needs a close camera, checking that the cavern reads
	// needs a far one, and neither should require editing the gameplay default to find out.
	FString BoomArg;
	if (FParse::Value(FCommandLine::Get(), TEXT("DeepCoreBoom="), BoomArg))
	{
		Boom = FMath::Clamp(FCString::Atof(*BoomArg), 300.0f, 8000.0f);
		Arm->TargetArmLength = Boom;
	}

	// FCommandLine::Get() returns a raw TCHAR*, so the value form is the parse to use here.
	FString ShotArg;
	bCaptureLock = FParse::Value(FCommandLine::Get(), TEXT("DeepCoreShot="), ShotArg);

	ApplyGrade();

	// Pitch scales with distance: looking almost straight down works for a close view but
	// makes a wide view read as a map rather than a place.
	// A shallow pitch lets the ground plane run all the way to the horizon, which compresses
	// the far half of the map into a few pixels and flattens walls against floor. Looking
	// steeply down keeps the visible area bounded and lets wall height actually read.
	Arm->SetRelativeRotation(FRotator(Boom > 3200.0f ? -65.0f : -55.0f, 0.0f, 0.0f));
}

void ADeepCorePawn::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);
	if (!Input)
	{
		return;
	}
	Input->BindKey(EKeys::MouseScrollUp,     IE_Pressed, this, &ADeepCorePawn::ZoomIn);
	Input->BindKey(EKeys::MouseScrollDown,   IE_Pressed, this, &ADeepCorePawn::ZoomOut);
	Input->BindKey(EKeys::LeftMouseButton,   IE_Pressed, this, &ADeepCorePawn::OnClick);
}

void ADeepCorePawn::ApplyGrade()
{
	const FDeepCoreTune& T = FDeepCoreTune::Get();

	Cam->PostProcessBlendWeight = 1.0f;
	FPostProcessSettings& S = Cam->PostProcessSettings;

	// FIXED exposure, expressed in EV100 -- HIGHER IS DARKER.
	//
	// Pinning Min == Max is how you get a fixed stop while leaving the exposure feature itself
	// enabled. Disabling the feature (r.DefaultFeature.AutoExposure=False) does NOT give fixed
	// exposure; it makes these values inert.
	S.bOverride_AutoExposureMethod        = true; S.AutoExposureMethod        = AEM_Histogram;
	S.bOverride_AutoExposureMinBrightness = true; S.AutoExposureMinBrightness = T.Ev;
	S.bOverride_AutoExposureMaxBrightness = true; S.AutoExposureMaxBrightness = T.Ev;
	S.bOverride_AutoExposureBias          = true; S.AutoExposureBias          = T.Bias;
	S.bOverride_AutoExposureSpeedUp       = true; S.AutoExposureSpeedUp       = 100.0f;
	S.bOverride_AutoExposureSpeedDown     = true; S.AutoExposureSpeedDown     = 100.0f;

	// The filmic toe is what produces underground blacks. Without it the tonemapper lifts the
	// shadows into the washed-out grey that is the signature UE5 default, and a dark scene
	// reads as an underexposed bright scene rather than as a dark place.
	S.bOverride_FilmToe       = true; S.FilmToe       = 0.62f;
	S.bOverride_FilmSlope     = true; S.FilmSlope     = 0.92f;
	S.bOverride_FilmShoulder  = true; S.FilmShoulder  = 0.30f;
	S.bOverride_FilmBlackClip = true; S.FilmBlackClip = 0.0f;
	S.bOverride_FilmWhiteClip = true; S.FilmWhiteClip = 0.04f;

	S.bOverride_BloomIntensity    = true; S.BloomIntensity    = 0.28f;
	S.bOverride_BloomThreshold    = true; S.BloomThreshold    = 1.0f;
	S.bOverride_VignetteIntensity = true; S.VignetteIntensity = 0.40f;

	// 40cm, not 90. Real cavity darkening happens at centimetre scale; 90cm is most of a tile
	// and darkens open floor rather than corners.
	S.bOverride_AmbientOcclusionIntensity = true; S.AmbientOcclusionIntensity = 0.45f;
	S.bOverride_AmbientOcclusionRadius    = true; S.AmbientOcclusionRadius    = 40.0f;
}

void ADeepCorePawn::ZoomIn()  { if (!bCaptureLock) { Boom = FMath::Max(700.0f,  Boom * 0.88f); } }
void ADeepCorePawn::ZoomOut() { if (!bCaptureLock) { Boom = FMath::Min(5200.0f, Boom * 1.14f); } }

void ADeepCorePawn::OnClick()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// Trace against the terrain's collision, which is why the terrain mesh cooks collision
	// while the characters deliberately do not.
	FHitResult Hit;
	if (PC->GetHitResultUnderCursor(ECC_WorldStatic, false, Hit) && Hit.bBlockingHit)
	{
		if (ADeepCoreGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ADeepCoreGameMode>() : nullptr)
		{
			GM->OrderNearestTo(Hit.ImpactPoint);
		}
	}
}

void ADeepCorePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || bCaptureLock)
	{
		return;
	}

	// Pan speed scales with zoom. At full zoom-out the same key press should cover more
	// ground, or crossing the map feels like wading.
	const float Pan = (700.0f + Boom * 0.55f) * DeltaSeconds;
	FVector Delta = FVector::ZeroVector;

	if (PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up))    { Delta.X += Pan; }
	if (PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down))  { Delta.X -= Pan; }
	if (PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::Left))  { Delta.Y -= Pan; }
	if (PC->IsInputKeyDown(EKeys::D) || PC->IsInputKeyDown(EKeys::Right)) { Delta.Y += Pan; }

	if (!Delta.IsNearlyZero())
	{
		// Pan relative to where the camera is FACING, not to world axes, so rotating the view
		// does not invert the controls.
		const FRotator YawOnly(0.0f, Arm->GetRelativeRotation().Yaw, 0.0f);
		AddActorWorldOffset(YawOnly.RotateVector(Delta));
	}

	if (PC->IsInputKeyDown(EKeys::Q)) { Arm->AddRelativeRotation(FRotator(0,  60.0f * DeltaSeconds, 0)); }
	if (PC->IsInputKeyDown(EKeys::E)) { Arm->AddRelativeRotation(FRotator(0, -60.0f * DeltaSeconds, 0)); }

	// Zoom eases toward its target so the wheel feels like a dial rather than a ratchet.
	Arm->TargetArmLength = FMath::FInterpTo(Arm->TargetArmLength, Boom, DeltaSeconds, 9.0f);
}

/**********************************************************************************
 ******** Game mode
 **********************************************************************************/

ADeepCoreGameMode::ADeepCoreGameMode()
{
	DefaultPawnClass = ADeepCorePawn::StaticClass();
	PrimaryActorTick.bCanEverTick = false;
}

void ADeepCoreGameMode::BuildLighting()
{
	const FDeepCoreTune& T = FDeepCoreTune::Get();
	UWorld* W = GetWorld();
	if (!W)
	{
		return;
	}

	// THERE IS NO SUN UNDERGROUND.
	//
	// The single loudest wrong note in this game so far was a directional light: parallel
	// shadows of uniform density falling across every tile of a sealed cavern, plus a blue sky
	// visible past the map edge. No amount of rock shading survives being lit by daylight. So
	// there is deliberately no ADirectionalLight spawned here, and there never should be.
	//
	// Everything is lit by what the crew carried down (see ADeepCoreUnit's cap lamp) and by a
	// handful of fixed worklights. That is also why hardware ray tracing matters so much: with
	// the sun gone, indirect bounce from those few small sources IS the ambient light, and
	// before this commit Lumen could not see any of this geometry to bounce off.

	// A near-black floor, not a light source. Real fill now comes from Lumen bounce; this only
	// stops the shadow side of geometry clipping to absolute zero, which reads as a hole in the
	// world rather than as darkness.
	if (ASkyLight* Sky = W->SpawnActor<ASkyLight>())
	{
		if (USkyLightComponent* C = Sky->GetLightComponent())
		{
			C->SetMobility(EComponentMobility::Movable);
			C->SourceType = SLS_CapturedScene;
			C->SetIntensity(T.Sky);
			C->SetLightColor(FLinearColor(0.44f, 0.40f, 0.36f));
			C->bLowerHemisphereIsBlack = false;
			C->RecaptureSky();
		}
	}

	// Dust. An underground heading is defined by what hangs in the air: without participating
	// media a cap lamp is just a bright spot on a wall, and with it the beam itself becomes
	// visible and the space acquires depth.
	//
	// The old inscattering colour here was (0.16, 0.20, 0.30) -- that is Rayleigh scatter, the
	// colour of outdoor sky, and it was tinting the whole mine blue.
	if (AExponentialHeightFog* Fog = W->SpawnActor<AExponentialHeightFog>())
	{
		Fog->SetActorLocation(FVector(0, 0, -400.0f));
		if (UExponentialHeightFogComponent* C = Fog->GetComponent())
		{
			C->SetFogDensity(T.FogDensity);
			C->SetFogInscatteringColor(FLinearColor(0.022f, 0.019f, 0.016f));
			C->SetFogHeightFalloff(0.02f);
			C->SetStartDistance(200.0f);

			C->SetVolumetricFog(true);
			// Forward-scattering, so beams bloom toward the viewer instead of glowing evenly
			// in all directions. This is most of what makes a light shaft look like a shaft.
			C->SetVolumetricFogScatteringDistribution(T.FogAniso);
			C->SetVolumetricFogAlbedo(FColor(190, 178, 162));   // warm rock dust, not white
			C->SetVolumetricFogExtinctionScale(T.FogExtinct);
			C->SetVolumetricFogDistance(4500.0f);
		}
	}

	// Post-processing is applied on the CAMERA (see ADeepCorePawn::ApplyGrade), not here.
	//
	// This used to spawn an unbound APostProcessVolume, and it silently never reached the view:
	// changing exposure by five stops produced a byte-identical frame. That one fact had been
	// mimicked as "the lights are wrong" for several rounds of tuning. A camera component's own
	// PostProcessSettings belong to the view being rendered and cannot miss.
}

void ADeepCoreGameMode::PlaceWorklight(const FVector& Where)
{
	const FDeepCoreTune& T = FDeepCoreTune::Get();
	UWorld* W = GetWorld();
	if (!W)
	{
		return;
	}

	AActor* Holder = W->SpawnActor<AActor>(AActor::StaticClass());
	if (!Holder)
	{
		return;
	}

	UPointLightComponent* L = NewObject<UPointLightComponent>(Holder);
	Holder->SetRootComponent(L);
	L->RegisterComponent();
	L->SetMobility(EComponentMobility::Movable);

	// Position AFTER the root exists. SpawnActor applies its transform to the actor's root
	// component, and this actor has none at spawn time, so passing the location there silently
	// discarded it and stacked every worklight at the world origin -- which is inside solid
	// rock, so the entire map rendered as unlit black silhouettes against the fog.
	L->SetWorldLocation(Where);

	// Cool white, against the warm 3200K cap lamps. That colour contrast between fixed
	// installation lighting and the lamps people carry is a large part of why an industrial
	// interior reads as industrial rather than as a cave with lights in it.
	L->SetIntensityUnits(ELightUnits::Candelas);
	L->SetIntensity(T.Worklight);
	L->SetLightColor(FLinearColor(0.82f, 0.88f, 1.0f));
	L->SetAttenuationRadius(1700.0f);
	L->SetSourceRadius(T.SourceRad);
	L->SetSoftSourceRadius(T.SourceRad * 2.0f);
	L->SetVolumetricScatteringIntensity(1.8f);
	L->SetCastShadows(true);

	Worklights.Add(L);

	if (Worklights.Num() == 1)
	{
		// Proof the fix holds: this used to print (0, 0, 0) for every lamp.
		const FVector P = L->GetComponentLocation();
		UE_LOG(LogTemp, Display, TEXT("DeepCore: first worklight at (%.0f, %.0f, %.0f)"), P.X, P.Y, P.Z);
	}
}

bool ADeepCoreGameMode::FindOpenTile(int32& OutX, int32& OutY, int32 Attempt) const
{
	if (!Terrain)
	{
		return false;
	}
	const Level& L = Terrain->GetLevel();

	// Deterministic scan seeded by attempt number, so a given map always populates the same
	// way. Reproducibility beats variety here: a bug that only appears on one spawn layout is
	// much easier to chase when the layout is a function of the seed.
	Rng R((uint32)(9176u + Attempt * 2654435761u));
	for (int32 Try = 0; Try < 4000; Try++)
	{
		const int32 X = (int32)R.Below((uint32)L.Width());
		const int32 Y = (int32)R.Below((uint32)L.Height());
		if (Terrain->IsWalkable(X, Y))
		{
			OutX = X; OutY = Y;
			return true;
		}
	}
	return false;
}

void ADeepCoreGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* W = GetWorld();
	if (!W)
	{
		return;
	}

	FDeepCoreTune::ParseCommandLine();

	// Confirm the renderer is actually in the mode the ini asked for. The entire lighting
	// design depends on hardware ray tracing being live -- without it Lumen cannot see a single
	// procedural mesh in this game -- and a silent fallback to software would look like a
	// tuning problem for hours before anyone suspected the renderer.
	{
		auto CVarInt = [](const TCHAR* Name) -> int32
		{
			IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name);
			return V ? V->GetInt() : -1;
		};
		UE_LOG(LogTemp, Display,
		       TEXT("DeepCore: renderer -- RayTracing=%d LumenHWRT=%d LumenHWRTMode=%d SkinCache=%d VolFog=%d"),
		       CVarInt(TEXT("r.RayTracing")), CVarInt(TEXT("r.Lumen.HardwareRayTracing")),
		       CVarInt(TEXT("r.Lumen.HardwareRayTracing.LightingMode")),
		       CVarInt(TEXT("r.SkinCache.CompileShaders")), CVarInt(TEXT("r.VolumetricFog")));
	}

	// The startup map is an engine map and arrives with its own sky. Anything that lights the
	// world from outside is wrong here by definition, and it was also washing out the frame
	// wherever the camera could see past the level's edge.
	{
		int32 Removed = 0;
		for (TActorIterator<AActor> It(W); It; ++It)
		{
			AActor* A = *It;
			if (!A) { continue; }
			const FString N = A->GetName();
			if (A->IsA(ADirectionalLight::StaticClass()) || A->IsA(ASkyAtmosphere::StaticClass())
			    || N.Contains(TEXT("Sky")) || N.Contains(TEXT("Atmospher")) || N.Contains(TEXT("Sun")))
			{
				A->Destroy();
				Removed++;
			}
		}
		UE_LOG(LogTemp, Display, TEXT("DeepCore: removed %d sky/sun actors from the startup map"), Removed);
	}

	BuildLighting();

	Terrain = W->SpawnActor<ADeepCoreTerrain>();
	if (!Terrain)
	{
		UE_LOG(LogTemp, Error, TEXT("DeepCore: terrain failed to spawn"));
		return;
	}
	Terrain->Generate(20260731u);

	// Crew. Distinct accent colours so individual units stay tellable apart at strategy-camera
	// distance, which is the only distance this game is played at.
	static const FLinearColor Accents[] = {
		FLinearColor(0.95f, 0.45f, 0.10f),
		FLinearColor(0.20f, 0.62f, 0.90f),
		FLinearColor(0.85f, 0.80f, 0.20f),
		FLinearColor(0.30f, 0.75f, 0.40f),
		FLinearColor(0.80f, 0.30f, 0.55f),
	};

	for (int32 I = 0; I < 5; I++)
	{
		int32 X = 0, Y = 0;
		if (!FindOpenTile(X, Y, I))
		{
			continue;
		}
		// Deferred spawn matters here. A plain SpawnActor runs BeginPlay before this function
		// gets the pointer back, so the unit would build itself with default parts and then be
		// built a second time -- which is what the "resource cleanup on a UObject overwrite"
		// warnings were: every limb constructed twice and the first copy thrown away.
		const FTransform Xf(FRotator::ZeroRotator, ADeepCoreTerrain::TileToWorld(X, Y, DeepCoreBrick::FloorTop));
		ADeepCoreUnit* U = W->SpawnActorDeferred<ADeepCoreUnit>(ADeepCoreUnit::StaticClass(), Xf);
		if (!U)
		{
			continue;
		}
		U->Terrain = Terrain;
		U->Build(EDeepCoreUnitKind::Miner, Accents[I % UE_ARRAY_COUNT(Accents)], 1.0f);
		U->FinishSpawning(Xf);
		Crew.Add(U);
	}

	// Creatures. Three discrete size classes rather than a continuous random scale, because a
	// player can learn three silhouettes and cannot learn a distribution.
	static const float SizeClass[] = { 0.85f, 1.15f, 1.55f };
	for (int32 I = 0; I < 4; I++)
	{
		int32 X = 0, Y = 0;
		if (!FindOpenTile(X, Y, 100 + I))
		{
			continue;
		}
		const FTransform Xf(FRotator::ZeroRotator, ADeepCoreTerrain::TileToWorld(X, Y, DeepCoreBrick::FloorTop));
		ADeepCoreUnit* U = W->SpawnActorDeferred<ADeepCoreUnit>(ADeepCoreUnit::StaticClass(), Xf);
		if (!U)
		{
			continue;
		}
		U->Terrain = Terrain;
		U->Build(EDeepCoreUnitKind::Monster, FLinearColor(0.34f, 0.26f, 0.22f),
		         SizeClass[I % UE_ARRAY_COUNT(SizeClass)]);
		U->SetWanders(true);
		U->FinishSpawning(Xf);
		Creatures.Add(U);
	}

	// Fixed worklights through the opened ground. Five cap lamps over a 48x40m map leave
	// almost the whole level black, which is authentic and unplayable; a developed heading is
	// strung with installed lighting anyway, so this is the honest fix rather than a cheat.
	// They are placed on discovered floor, so lit space and known space are the same thing.
	{
		const Level& L = Terrain->GetLevel();
		const int32 Step = FDeepCoreTune::Get().WorklightStep;
		int32 Placed = 0;
		for (int32 Y = Step / 2; Y < L.Height(); Y += Step)
		{
			for (int32 X = Step / 2; X < L.Width(); X += Step)
			{
				// Search the neighbourhood for a walkable tile, so a lamp is never stranded
				// inside rock just because the fixed lattice landed there.
				bool bDone = false;
				for (int32 R = 0; R <= 3 && !bDone; R++)
				{
					for (int32 Dy = -R; Dy <= R && !bDone; Dy++)
					{
						for (int32 Dx = -R; Dx <= R && !bDone; Dx++)
						{
							if (!Terrain->IsWalkable(X + Dx, Y + Dy))
							{
								continue;
							}
							PlaceWorklight(ADeepCoreTerrain::TileToWorld(X + Dx, Y + Dy, FDeepCoreTune::Get().WorklightZ));
							Placed++;
							bDone = true;
						}
					}
				}
			}
		}
		UE_LOG(LogTemp, Display, TEXT("DeepCore: %d worklights placed"), Placed);
	}

	// Put the camera over the middle of the map rather than over the origin, which is a map
	// corner and may well be solid rock.
	if (APlayerController* PC = W->GetFirstPlayerController())
	{
		if (APawn* P = PC->GetPawn())
		{
			// Start looking at the crew, not at the middle of the map. The middle is usually
			// unexplored rock, and a first frame of undifferentiated dark stone tells the
			// player nothing about what they control.
			// The centroid of the crew, not one member of it. FindOpenTile can legitimately
			// place crew[0] in a corner chamber, and framing on that one unit points the
			// camera off the edge of the map.
			const Level& L = Terrain->GetLevel();
			FVector Centre = ADeepCoreTerrain::TileToWorld(L.Width() / 2, L.Height() / 2);
			if (Crew.Num() > 0)
			{
				Centre = FVector::ZeroVector;
				for (ADeepCoreUnit* U : Crew) { Centre += U->GetActorLocation(); }
				Centre /= (float)Crew.Num();
			}
			P->SetActorLocation(Centre);
			P->SetActorRotation(FRotator::ZeroRotator);

			if (const UCameraComponent* Cam = P->FindComponentByClass<UCameraComponent>())
			{
				const FVector CamPos = Cam->GetComponentLocation();
				const FRotator CamRot = Cam->GetComponentRotation();
				// Logged because a wrong camera is indistinguishable from a wrong world in a
				// screenshot, and the first capture of this build was in fact the camera.
				UE_LOG(LogTemp, Display,
				       TEXT("DeepCore: camera at (%.0f, %.0f, %.0f) rot (%.1f, %.1f, %.1f)"),
				       CamPos.X, CamPos.Y, CamPos.Z, CamRot.Pitch, CamRot.Yaw, CamRot.Roll);
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT("DeepCore: world ready -- %d crew, %d creatures"),
	       Crew.Num(), Creatures.Num());

	// Log where a unit actually IS and how big its geometry actually is. A screenshot with no
	// visible character cannot distinguish "not built" from "built somewhere else", and this
	// build has already been wrong in both ways.
	if (Crew.Num() > 0 && Crew[0])
	{
		const FVector P = Crew[0]->GetActorLocation();
		const FBox B = Crew[0]->GetComponentsBoundingBox(true);
		UE_LOG(LogTemp, Display,
		       TEXT("DeepCore: crew[0] at (%.0f, %.0f, %.0f), bounds %.0f x %.0f x %.0f"),
		       P.X, P.Y, P.Z, B.GetSize().X, B.GetSize().Y, B.GetSize().Z);
	}

	// -DeepCoreShot=<seconds> renders for a while, saves a screenshot, then quits.
	//
	// This exists so that "it looks right" can be checked rather than asserted. A build that
	// compiles and logs cleanly can still be drawing a black screen, and without a captured
	// frame there is no way to tell those apart from a log file.
	FString ShotArg;
	if (FParse::Value(FCommandLine::Get(), TEXT("DeepCoreShot="), ShotArg))
	{
		const float Delay = FMath::Clamp(FCString::Atof(*ShotArg), 1.0f, 120.0f);
		const uint64 StartFrame = GFrameCounter;
		const double StartTime  = FPlatformTime::Seconds();

		FTimerHandle Shot, Quit;
		W->GetTimerManager().SetTimer(Shot, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			// Shaders and Lumen both need a few frames to settle; capturing on frame one
			// photographs the loading state rather than the game.
			//
			// Routed through FScreenshotRequest rather than a console command: HighResShot is
			// handled by the viewport client, and UWorld::Exec silently does not reach it.
			FScreenshotRequest::RequestScreenshot(TEXT("DeepCore"), false, false);
			UE_LOG(LogTemp, Display, TEXT("DeepCore: screenshot requested -> %s"),
			       *FScreenshotRequest::GetFilename());

			// The POV that actually rendered this frame, read from the camera manager rather than
			// inferred from the spring arm. Whether the view even comes from our camera is exactly
			// the thing in question, so asking the arm what it thinks would beg it.
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				if (PC->PlayerCameraManager)
				{
					const FVector L = PC->PlayerCameraManager->GetCameraLocation();
					const FRotator R = PC->PlayerCameraManager->GetCameraRotation();
					UE_LOG(LogTemp, Display,
					       TEXT("DeepCore: POV (%.0f, %.0f, %.0f) pitch %.1f fov %.1f pawn %s"),
					       L.X, L.Y, L.Z, R.Pitch, PC->PlayerCameraManager->GetFOVAngle(),
					       *GetNameSafe(PC->GetPawn()));
				}
			}
		}), Delay, false);

		W->GetTimerManager().SetTimer(Quit, FTimerDelegate::CreateWeakLambda(this,
			[this, StartFrame, StartTime]()
		{
			const double Elapsed = FPlatformTime::Seconds() - StartTime;
			const uint64 Frames  = GFrameCounter - StartFrame;
			UE_LOG(LogTemp, Display, TEXT("DeepCore: %llu frames in %.1fs = %.1f fps average"),
			       Frames, Elapsed, Elapsed > 0.0 ? (double)Frames / Elapsed : 0.0);
			UE_LOG(LogTemp, Display, TEXT("DeepCore: capture complete, exiting"));
			FPlatformMisc::RequestExit(false);
		}), Delay + 4.0f, false);
	}
}

void ADeepCoreGameMode::OrderNearestTo(const FVector& World)
{
	ADeepCoreUnit* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();

	// Prefer an idle unit; fall back to the nearest of any. Ordering a busy unit when an idle
	// one is closer is the classic RTS annoyance, and it is one comparison to avoid.
	for (ADeepCoreUnit* U : Crew)
	{
		if (!U)
		{
			continue;
		}
		const float D = FVector::Dist2D(U->GetActorLocation(), World)
		              + (U->HasArrived() ? 0.0f : 4000.0f);
		if (D < BestDist)
		{
			BestDist = D;
			Best = U;
		}
	}

	if (Best)
	{
		Best->OrderMoveTo(FVector(World.X, World.Y, Best->GetActorLocation().Z));
	}
}
