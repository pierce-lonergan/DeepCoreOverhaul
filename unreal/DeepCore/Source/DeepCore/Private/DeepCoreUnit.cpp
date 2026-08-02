#include "DeepCoreUnit.h"

#include "DeepCoreBrick.h"
#include "DeepCoreMaterials.h"
#include "DeepCoreTerrain.h"
#include "DeepCoreTune.h"
#include "Components/SpotLightComponent.h"
#include "ProceduralMeshComponent.h"

using namespace DeepCoreBrick;

ADeepCoreUnit::ADeepCoreUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Hips"));
	Pivot->SetupAttachment(Root);

	// The pivot chain is what the animation actually drives. Each joint is an empty scene
	// component so the geometry hanging off it can be built once and never touched again --
	// only transforms change per frame, which is what keeps this cheap.
	TorsoPivot = CreateDefaultSubobject<USceneComponent>(TEXT("TorsoPivot"));
	HeadPivot  = CreateDefaultSubobject<USceneComponent>(TEXT("HeadPivot"));
	HipL       = CreateDefaultSubobject<USceneComponent>(TEXT("HipL"));
	HipR       = CreateDefaultSubobject<USceneComponent>(TEXT("HipR"));
	KneeL      = CreateDefaultSubobject<USceneComponent>(TEXT("KneeL"));
	KneeR      = CreateDefaultSubobject<USceneComponent>(TEXT("KneeR"));
	ShoulderL  = CreateDefaultSubobject<USceneComponent>(TEXT("ShoulderL"));
	ShoulderR  = CreateDefaultSubobject<USceneComponent>(TEXT("ShoulderR"));

	TorsoPivot->SetupAttachment(Pivot);
	HeadPivot->SetupAttachment(TorsoPivot);
	ShoulderL->SetupAttachment(TorsoPivot);
	ShoulderR->SetupAttachment(TorsoPivot);
	HipL->SetupAttachment(Pivot);
	HipR->SetupAttachment(Pivot);
	KneeL->SetupAttachment(HipL);
	KneeR->SetupAttachment(HipR);
}

void ADeepCoreUnit::BeginPlay()
{
	Super::BeginPlay();
	if (!bBuilt)
	{
		Build(EDeepCoreUnitKind::Miner, FLinearColor(0.95f, 0.45f, 0.10f), 1.0f);
	}
}

UProceduralMeshComponent* ADeepCoreUnit::MakePart(USceneComponent* Parent, const TCHAR* Name)
{
	UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(this, Name);
	P->SetupAttachment(Parent);
	P->RegisterComponent();
	P->SetMobility(EComponentMobility::Movable);
	P->SetCastShadow(true);
	// Characters never need collision geometry: movement is grid-driven and validated against
	// the level, so cooking a body per limb per frame would be pure cost.
	P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Parts.Add(P);
	return P;
}

void ADeepCoreUnit::Build(EDeepCoreUnitKind InKind, const FLinearColor& Accent, float InScale)
{
	Kind   = InKind;
	Scale  = InScale;
	bBuilt = true;

	// Seed the roaming sequence per unit. Left at zero every creature would draw the identical
	// sequence of directions and the pack would move as one organism.
	WanderState = GetUniqueID() * 2654435761u + 17u;

	if (Kind == EDeepCoreUnitKind::Miner)
	{
		HipHeight = 62.0f;
		BuildMiner(Accent);
	}
	else
	{
		HipHeight = 56.0f;
		BuildMonster(Accent);
	}

	Pivot->SetRelativeLocation(FVector(0.0f, 0.0f, HipHeight));

	// Scale the root, so the hip height scales with everything else and a large creature still
	// stands ON the floor rather than in it.
	Root->SetWorldScale3D(FVector(Scale));
}

void ADeepCoreUnit::BuildMiner(const FLinearColor& Accent)
{
	const FDeepCorePalette& Palette = GetDeepCorePalette();

	const FLinearColor Suit  = Accent;
	const FLinearColor Dark  = FLinearColor(0.16f, 0.17f, 0.20f);
	const FLinearColor Skin  = FLinearColor(0.92f, 0.74f, 0.52f);
	const FLinearColor Metal = FLinearColor(0.55f, 0.57f, 0.62f);

	// Joint placement. Hips sit at 62cm, which puts the eyeline near 150cm -- human
	// proportions, so the default camera and Lumen tuning behave without being fought.
	TorsoPivot->SetRelativeLocation(FVector(0, 0, 0));
	HeadPivot->SetRelativeLocation(FVector(0, 0, 52.0f));
	HipL->SetRelativeLocation(FVector(0, -11.0f, 0));
	HipR->SetRelativeLocation(FVector(0,  11.0f, 0));
	KneeL->SetRelativeLocation(FVector(0, 0, -34.0f));
	KneeR->SetRelativeLocation(FVector(0, 0, -34.0f));
	ShoulderL->SetRelativeLocation(FVector(0, -21.0f, 44.0f));
	ShoulderR->SetRelativeLocation(FVector(0,  21.0f, 44.0f));

	{	// Torso: tapered, with a stud on each shoulder so the figure reads as brick-built
		// even standing still.
		FBrickMesh M;
		M.SetInk(Suit);
		M.Part(FVector(0, 0, 26.0f), 15.0f, 11.0f, 26.0f);
		M.SetInk(Dark);
		M.Part(FVector(2.0f, 0, 14.0f), 14.0f, 12.0f, 8.0f);   // tool belt
		M.SetInk(Metal);
		M.Part(FVector(-9.0f, 0, 40.0f), 5.0f, 8.0f, 6.0f);    // back-mounted lamp housing
		M.Commit(MakePart(TorsoPivot, TEXT("Torso")), 0, false);
	}

	{	// Head and helmet. The helmet is domed rather than cylindrical -- both because the
		// cylinder-headed silhouette is somebody's trade mark, and because facets catch the
		// key light in a way a smooth dome does not.
		FBrickMesh M;
		M.SetInk(Skin);
		M.Part(FVector(0, 0, 9.0f), 9.0f, 9.0f, 9.0f);
		M.SetInk(Suit);
		M.Domed(FVector(0, 0, 16.0f), 13.0f, 16.0f);
		M.Commit(MakePart(HeadPivot, TEXT("Head")), 0, false);

		FBrickMesh L;
		L.SetInk(FLinearColor(1.0f, 0.92f, 0.62f));
		L.Part(FVector(11.0f, 0, 18.0f), 3.0f, 5.0f, 4.0f);    // headlamp lens
		L.Commit(MakePart(HeadPivot, TEXT("LampHousing")), 0, false);
		Parts.Last()->SetMaterial(0, Palette.Glow);

		// The light itself. A real cap lamp is roughly 200-400 lumens in a fairly tight cone;
		// in candelas over an 18/34 degree cone that lands near 1800. Warm, because they are
		// tungsten-temperature or a warm LED, and that warmth against cold fixed worklights is
		// most of the colour story in a working mine.
		Lamp = NewObject<USpotLightComponent>(this, TEXT("CapLamp"));
		Lamp->SetupAttachment(HeadPivot);
		Lamp->RegisterComponent();
		Lamp->SetRelativeLocation(FVector(11.0f, 0.0f, 18.0f));
		Lamp->SetMobility(EComponentMobility::Movable);
		Lamp->SetIntensityUnits(ELightUnits::Candelas);
		Lamp->SetIntensity(FDeepCoreTune::Get().CapLamp);
		Lamp->SetLightColor(FLinearColor(1.0f, 0.72f, 0.44f));   // ~3200K
		Lamp->SetInnerConeAngle(18.0f);
		Lamp->SetOuterConeAngle(34.0f);
		Lamp->SetAttenuationRadius(1400.0f);
		// Makes the beam itself visible in the dust rather than just its landing spot.
		Lamp->SetVolumetricScatteringIntensity(2.5f);
		Lamp->SetCastShadows(true);
		// Centimetre-scale cavity darkening. Tile ambient occlusion works at 100cm and can
		// never produce contact shading at the scale the eye actually looks for it.
		Lamp->ContactShadowLength = 0.03f;
		Lamp->ContactShadowLengthInWS = false;
	}

	auto Limb = [&](USceneComponent* Parent, const TCHAR* Name,
	                float HalfX, float HalfY, float Len, const FLinearColor& Col)
	{
		FBrickMesh M;
		M.SetInk(Col);
		// Parts hang DOWNWARD from their pivot, which is what makes a rotation about the
		// pivot swing the limb rather than spin it about its own middle.
		M.Part(FVector(0, 0, -Len * 0.5f), HalfX, HalfY, Len * 0.5f);
		M.Commit(MakePart(Parent, Name), 0, false);
	};

	Limb(HipL,      TEXT("ThighL"), 8.0f, 8.0f, 34.0f, Dark);
	Limb(HipR,      TEXT("ThighR"), 8.0f, 8.0f, 34.0f, Dark);
	Limb(KneeL,     TEXT("ShinL"),  8.5f, 8.5f, 28.0f, Suit);
	Limb(KneeR,     TEXT("ShinR"),  8.5f, 8.5f, 28.0f, Suit);
	Limb(ShoulderL, TEXT("ArmL"),   6.5f, 6.5f, 38.0f, Suit);
	Limb(ShoulderR, TEXT("ArmR"),   6.5f, 6.5f, 38.0f, Suit);

	for (UProceduralMeshComponent* P : Parts)
	{
		if (P && !P->GetMaterial(0))
		{
			P->SetMaterial(0, Palette.Surface);
		}
	}
	Speed = 260.0f;
}

void ADeepCoreUnit::BuildMonster(const FLinearColor& Accent)
{
	const FDeepCorePalette& Palette = GetDeepCorePalette();

	const FLinearColor Rock = Accent;
	const FLinearColor Deep = Accent * 0.55f;

	// Creatures are assembled from COARSER bricks than the crew. Stud size is a silhouette
	// cue: bigger studs read as a bigger creature at any distance, without needing a health
	// bar or an outline to say so.
	TorsoPivot->SetRelativeLocation(FVector(0, 0, 0));
	HeadPivot->SetRelativeLocation(FVector(24.0f, 0, 40.0f));
	HipL->SetRelativeLocation(FVector(-8.0f, -20.0f, 0));
	HipR->SetRelativeLocation(FVector(-8.0f,  20.0f, 0));
	KneeL->SetRelativeLocation(FVector(0, 0, -30.0f));
	KneeR->SetRelativeLocation(FVector(0, 0, -30.0f));
	ShoulderL->SetRelativeLocation(FVector(14.0f, -28.0f, 34.0f));
	ShoulderR->SetRelativeLocation(FVector(14.0f,  28.0f, 34.0f));

	{	// Hunched body, pitched forward. The forward pitch is most of what separates a
		// predator silhouette from an upright one.
		FBrickMesh M;
		M.SetInk(Rock);
		M.Part(FVector(0, 0, 26.0f), 24.0f, 20.0f, 24.0f);
		M.SetInk(Deep);
		M.Part(FVector(-18.0f, 0, 30.0f), 12.0f, 14.0f, 14.0f);
		// Back plates, largest at the shoulders, tapering to the tail.
		M.SetInk(Rock * 1.15f);
		M.Part(FVector(8.0f,  0, 50.0f), 6.0f, 5.0f, 10.0f, true, 0.8f);
		M.Part(FVector(-6.0f, 0, 48.0f), 5.0f, 4.5f, 8.0f,  true, 0.7f);
		M.Commit(MakePart(TorsoPivot, TEXT("Body")), 0, false);
	}

	{
		FBrickMesh M;
		M.SetInk(Deep);
		M.Part(FVector(0, 0, 0), 18.0f, 15.0f, 13.0f);
		M.SetInk(Rock);
		M.Part(FVector(14.0f, 0, -4.0f), 8.0f, 11.0f, 7.0f);   // jaw
		M.Commit(MakePart(HeadPivot, TEXT("Skull")), 0, false);

		FBrickMesh E;
		E.SetInk(FLinearColor(1.0f, 0.55f, 0.10f));
		E.Part(FVector(13.0f, -7.0f, 7.0f), 3.5f, 3.5f, 3.0f);
		E.Part(FVector(13.0f,  7.0f, 7.0f), 3.5f, 3.5f, 3.0f);
		E.Commit(MakePart(HeadPivot, TEXT("Eyes")), 0, false);
		Parts.Last()->SetMaterial(0, Palette.Glow);
	}

	auto Limb = [&](USceneComponent* Parent, const TCHAR* Name,
	                float HalfX, float HalfY, float Len, const FLinearColor& Col)
	{
		FBrickMesh M;
		M.SetInk(Col);
		M.Part(FVector(0, 0, -Len * 0.5f), HalfX, HalfY, Len * 0.5f);
		M.Commit(MakePart(Parent, Name), 0, false);
	};

	Limb(HipL,      TEXT("ThighL"), 11.0f, 11.0f, 30.0f, Deep);
	Limb(HipR,      TEXT("ThighR"), 11.0f, 11.0f, 30.0f, Deep);
	Limb(KneeL,     TEXT("ShinL"),  10.0f, 10.0f, 26.0f, Rock);
	Limb(KneeR,     TEXT("ShinR"),  10.0f, 10.0f, 26.0f, Rock);
	Limb(ShoulderL, TEXT("ArmL"),   9.0f,  9.0f,  40.0f, Rock);
	Limb(ShoulderR, TEXT("ArmR"),   9.0f,  9.0f,  40.0f, Rock);

	for (UProceduralMeshComponent* P : Parts)
	{
		if (P && !P->GetMaterial(0))
		{
			P->SetMaterial(0, Palette.Surface);
		}
	}
	Speed = 175.0f;
}

bool ADeepCoreUnit::PickWanderTarget()
{
	if (!Terrain)
	{
		return false;
	}

	const Sandbox::Level& L = Terrain->GetLevel();
	int32 Cx = 0, Cy = 0;
	ADeepCoreTerrain::WorldToTile(GetActorLocation(), Cx, Cy);

	// Search outward from where the creature already is rather than anywhere on the map, so
	// roaming looks local. A creature that teleports its intent across the cavern reads as a
	// pathfinding demo, not as a thing living somewhere.
	for (int32 Try = 0; Try < 40; Try++)
	{
		WanderState = WanderState * 1103515245u + 12345u;
		const uint32 R = (WanderState >> 16) & 0x7fffu;
		const int32 Dx = (int32)(R % 13u) - 6;
		const int32 Dy = (int32)((R / 13u) % 13u) - 6;
		const int32 Nx = Cx + Dx, Ny = Cy + Dy;

		if ((Dx == 0 && Dy == 0) || !L.InBounds(Nx, Ny) || !Terrain->IsWalkable(Nx, Ny))
		{
			continue;
		}
		OrderMoveTo(ADeepCoreTerrain::TileToWorld(Nx, Ny, GetActorLocation().Z));
		return true;
	}
	return false;
}

void ADeepCoreUnit::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bBuilt)
	{
		return;
	}

	const FVector Was = GetActorLocation();
	FVector Pos = Was;

	// Creatures roam. The pause between moves is as important as the movement: something that
	// walks continuously reads as a patrol route, whereas something that stops, waits, then
	// picks a new direction reads as an animal deciding.
	if (bWanders && !bHasOrder)
	{
		WanderPause -= DeltaSeconds;
		if (WanderPause <= 0.0f)
		{
			if (PickWanderTarget())
			{
				WanderPause = 2.0f;
			}
			else
			{
				WanderPause = 1.0f;   // nowhere to go; try again shortly
			}
		}
	}

	if (bHasOrder)
	{
		FVector To = Destination - Pos;
		To.Z = 0.0f;
		const float Dist = To.Size();
		if (Dist < 12.0f)
		{
			bHasOrder = false;
		}
		else
		{
			const FVector Dir = To / Dist;
			Pos += Dir * FMath::Min(Speed * DeltaSeconds, Dist);

			// Turn toward travel rather than snapping to it. LookAtYaw limits the rate, so a
			// unit ordered to reverse pivots through the turn instead of teleporting to face
			// the other way -- one of the cheapest wins in the whole animation set.
			const float Desired = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
			Yaw = Anim::LookAtYaw(Yaw, Desired, 420.0f * DeltaSeconds);
			SetActorLocation(Pos);
		}
	}

	// THE RULE THAT MATTERS: phase advances by distance moved, not by time. A unit that is
	// blocked, slowed, or stopped cannot moonwalk, because a stationary unit advances no
	// phase at all. This is why the feet stay planted without any tuning.
	const float Moved = FVector::Dist2D(Was, Pos);
	const float Stride = (Kind == EDeepCoreUnitKind::Miner) ? 85.0f : 105.0f;
	Gait = Anim::Wrap01(Gait + Moved / Stride);

	const bool bMoving = Moved > KINDA_SMALL_NUMBER;
	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	SetActorRotation(FRotator(0.0f, Yaw, 0.0f));

	// --- legs ------------------------------------------------------------------------
	const Anim::LegPose L = Anim::WalkLeg(Gait,          bMoving ? 26.0f : 0.0f);
	const Anim::LegPose R = Anim::WalkLeg(Gait + 0.5f,   bMoving ? 26.0f : 0.0f);

	HipL->SetRelativeRotation(FRotator(-L.hip, 0, 0));
	HipR->SetRelativeRotation(FRotator(-R.hip, 0, 0));
	KneeL->SetRelativeRotation(FRotator(L.knee, 0, 0));
	KneeR->SetRelativeRotation(FRotator(R.knee, 0, 0));

	// --- body ------------------------------------------------------------------------
	float BodyZ = bMoving ? Anim::WalkBob(Gait) * 100.0f : Anim::IdleBreath(Time) * 100.0f;
	const float Sway = bMoving ? Anim::WalkSway(Gait) * 60.0f : 0.0f;

	if (bAttacking)
	{
		AttackTime += DeltaSeconds;
		if (AttackTime > 0.9f)
		{
			bAttacking = false;
		}
	}
	const float Swing = bAttacking ? Anim::AttackCurve(AttackTime / 0.9f) : 0.0f;

	// Secondary motion: the torso chases the hips instead of being welded to them, so a stop
	// settles rather than snaps. This, more than any pose, is what reads as weight.
	TorsoLag.Step(Sway + Swing * 18.0f, DeltaSeconds);
	HeadLag.Step(TorsoLag.value * 0.6f, DeltaSeconds);

	const float Hunch = (Kind == EDeepCoreUnitKind::Monster) ? 22.0f : 0.0f;
	Pivot->SetRelativeLocation(FVector(0, 0, HipHeight + BodyZ));
	TorsoPivot->SetRelativeRotation(FRotator(Hunch + TorsoLag.value * 0.35f, 0, TorsoLag.value));
	HeadPivot->SetRelativeRotation(FRotator(-Hunch * 0.7f - HeadLag.value * 0.25f, 0, -HeadLag.value * 0.5f));

	// --- arms ------------------------------------------------------------------------
	const float ArmSwing = bMoving ? Anim::WalkArm(Gait) : 0.0f;
	const float StrikeL  = Swing * 70.0f;
	ShoulderL->SetRelativeRotation(FRotator(-ArmSwing - StrikeL, 0, 0));
	ShoulderR->SetRelativeRotation(FRotator( ArmSwing - StrikeL, 0, 0));
}
