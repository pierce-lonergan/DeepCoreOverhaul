#include "DeepCoreGame.h"

#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "DeepCoreTerrain.h"
#include "DeepCoreUnit.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
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

void ADeepCorePawn::ZoomIn()  { Boom = FMath::Max(700.0f,  Boom * 0.88f); }
void ADeepCorePawn::ZoomOut() { Boom = FMath::Min(5200.0f, Boom * 1.14f); }

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
	if (!PC)
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
	UWorld* W = GetWorld();
	if (!W)
	{
		return;
	}

	// Key light. Warm, low, and strongly angled: a single hard key is what gives stacked
	// bricks their edge definition, and a low angle throws the long shadows that make a
	// cavern floor read as a floor rather than as a texture.
	if (ADirectionalLight* Key = W->SpawnActor<ADirectionalLight>())
	{
		Key->SetMobility(EComponentMobility::Movable);
		Key->SetActorRotation(FRotator(-42.0f, 35.0f, 0.0f));
		if (UDirectionalLightComponent* C = Cast<UDirectionalLightComponent>(Key->GetLightComponent()))
		{
			// Intensity is in lux. 4 lux is roughly deep twilight, which is what made the first
			// capture read as an unlit scene; 10 is the engine's own daylight default and the
			// right order of magnitude once exposure is pinned.
			C->SetIntensity(10.0f);
			C->SetLightColor(FLinearColor(1.0f, 0.92f, 0.78f));
			C->SetDynamicShadowCascades(4);
			C->SetShadowBias(0.4f);
		}
	}

	// Fill. Cool, dim, omnidirectional -- the bounce a real cavern would give. Without it the
	// shadow side of every brick goes to pure black and the geometry disappears.
	if (ASkyLight* Sky = W->SpawnActor<ASkyLight>())
	{
		// ASkyLight derives from AInfo, which has no mobility of its own; the mobility lives
		// on the component.
		if (USkyLightComponent* C = Sky->GetLightComponent())
		{
			C->SetMobility(EComponentMobility::Movable);
			// Captured scene, NOT SpecifiedCubemap: the latter needs a cubemap asset, and with
			// none assigned the sky light contributes exactly nothing -- which is why every
			// shadowed brick face went to pure black.
			C->SourceType = SLS_CapturedScene;
			C->SetIntensity(1.6f);
			C->SetLightColor(FLinearColor(0.62f, 0.72f, 0.92f));
			C->bLowerHemisphereIsBlack = false;
			C->RecaptureSky();
		}
	}

	// Fog. Depth cueing is what stops a large map reading flat; distant caverns should sit
	// behind near ones without the player having to work it out from perspective alone.
	if (AExponentialHeightFog* Fog = W->SpawnActor<AExponentialHeightFog>())
	{
		Fog->SetActorLocation(FVector(0, 0, -400.0f));
		if (UExponentialHeightFogComponent* C = Fog->GetComponent())
		{
			C->SetFogDensity(0.008f);
			C->SetFogInscatteringColor(FLinearColor(0.16f, 0.20f, 0.30f));
			C->SetFogHeightFalloff(0.12f);
			C->SetStartDistance(600.0f);
		}
	}

	// Post. Fixed exposure is the important part: auto-exposure would ramp brightness as the
	// player pans between an open chamber and a tight corridor, which reads as the game
	// flickering rather than as the eye adapting.
	if (APostProcessVolume* PP = W->SpawnActor<APostProcessVolume>())
	{
		PP->bUnbound = true;
		FPostProcessSettings& S = PP->Settings;

		// Fixed exposure, done the reliable way: histogram metering with its min and max
		// brightness pinned to the same value. AEM_Manual sounds like the right tool but it
		// derives exposure from physical camera settings (ISO, aperture, shutter), so setting
		// only a bias leaves the actual stop undefined -- which crushed every lit surface to
		// black while the emissive crystals blew out to pure white.
		S.bOverride_AutoExposureMethod        = true; S.AutoExposureMethod        = AEM_Histogram;
		// These clamp the METERED scene luminance, and exposure is its reciprocal, so a smaller
		// number means a brighter image. 1.0 metered a cave lit at 10 lux about two stops
		// under; 0.35 lands it where the rock reads as rock rather than as silhouette.
		S.bOverride_AutoExposureMinBrightness = true; S.AutoExposureMinBrightness = 0.35f;
		S.bOverride_AutoExposureMaxBrightness = true; S.AutoExposureMaxBrightness = 0.35f;
		S.bOverride_AutoExposureBias          = true; S.AutoExposureBias          = 0.0f;
		S.bOverride_BloomIntensity     = true;  S.BloomIntensity     = 0.45f;
		S.bOverride_BloomThreshold     = true;  S.BloomThreshold     = 1.4f;
		S.bOverride_VignetteIntensity  = true;  S.VignetteIntensity  = 0.32f;
		S.bOverride_AmbientOcclusionIntensity = true; S.AmbientOcclusionIntensity = 0.55f;
		S.bOverride_AmbientOcclusionRadius    = true; S.AmbientOcclusionRadius    = 90.0f;
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
