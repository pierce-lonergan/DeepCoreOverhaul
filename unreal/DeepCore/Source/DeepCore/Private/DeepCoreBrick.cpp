#include "DeepCoreBrick.h"

#include "DeepCoreRock.h"

// Unreal is Z-up; the OpenGL original was Y-up. Every "height" here therefore runs along Z.
// Getting this wrong produces a world lying on its side, which is obvious the moment it is
// seen but easy to write.

using namespace DeepCoreBrick;

int32 FBrickMesh::Vert(const FVector& P, const FVector& N, const FVector2D& UV)
{
	const int32 Index = Vertices.Add(P);
	Normals.Add(N);
	UVs.Add(UV);

	if (bStrata)
	{
		// ReinterpretAsLinear, NOT FLinearColor(Ink). SetInk stores the ink with ToFColor(false),
		// which writes the linear value into the byte with no sRGB encode -- so the byte IS the
		// linear value scaled by 255, and dividing by 255 is its exact inverse. FLinearColor(FColor)
		// instead runs sRGBToLinearTable (Color.h:838), decoding a value that was never encoded.
		// On country rock that turned albedo 0.098 into 0.0097 -- a 10x crush that squeezed the
		// whole strata band into about two byte levels, hiding the banding entirely and pushing the
		// worklights to 900 cd to compensate for walls that were never meant to be that dark.
		const float T = DeepCoreRock::Strata(P);
		FLinearColor Tinted = Ink.ReinterpretAsLinear();
		Tinted.R *= T; Tinted.G *= T; Tinted.B *= T;
		Colors.Add(Tinted.ToFColor(false));
	}
	else
	{
		Colors.Add(Ink);
	}
	// A tangent perpendicular to the normal. Nothing here samples a normal map, but the
	// component wants tangents and a degenerate one would break any material that ever does.
	const FVector T = FMath::Abs(N.Z) > 0.9f ? FVector(1, 0, 0) : FVector(0, 0, 1) ^ N;
	Tangents.Add(FProcMeshTangent(T.GetSafeNormal(), false));
	return Index;
}

void FBrickMesh::Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D,
                      const FVector& N)
{
	const int32 I0 = Vert(A, N, FVector2D(0, 0));
	const int32 I1 = Vert(B, N, FVector2D(1, 0));
	const int32 I2 = Vert(C, N, FVector2D(1, 1));
	const int32 I3 = Vert(D, N, FVector2D(0, 1));

	Triangles.Add(I0); Triangles.Add(I1); Triangles.Add(I2);
	Triangles.Add(I0); Triangles.Add(I2); Triangles.Add(I3);
}

void FBrickMesh::Box(const FVector& Centre, float HalfX, float Height, float HalfZ)
{
	const float X0 = Centre.X - HalfX, X1 = Centre.X + HalfX;
	const float Y0 = Centre.Y - HalfZ, Y1 = Centre.Y + HalfZ;
	const float Z0 = Centre.Z,         Z1 = Centre.Z + Height;

	Quad(FVector(X0, Y0, Z1), FVector(X1, Y0, Z1), FVector(X1, Y1, Z1), FVector(X0, Y1, Z1), FVector( 0,  0,  1));
	Quad(FVector(X0, Y1, Z0), FVector(X1, Y1, Z0), FVector(X1, Y0, Z0), FVector(X0, Y0, Z0), FVector( 0,  0, -1));
	Quad(FVector(X0, Y0, Z0), FVector(X1, Y0, Z0), FVector(X1, Y0, Z1), FVector(X0, Y0, Z1), FVector( 0, -1,  0));
	Quad(FVector(X1, Y1, Z0), FVector(X0, Y1, Z0), FVector(X0, Y1, Z1), FVector(X1, Y1, Z1), FVector( 0,  1,  0));
	Quad(FVector(X0, Y1, Z0), FVector(X0, Y0, Z0), FVector(X0, Y0, Z1), FVector(X0, Y1, Z1), FVector(-1,  0,  0));
	Quad(FVector(X1, Y0, Z0), FVector(X1, Y1, Z0), FVector(X1, Y1, Z1), FVector(X1, Y0, Z1), FVector( 1,  0,  0));
}

void FBrickMesh::Stud(const FVector& Centre, float Scale)
{
	// Eight segments, not twelve. At one stud per tile a stud was a metre across and needed
	// the extra facets; at 25cm there are sixteen times as many of them and each covers a few
	// pixels, so the segments that were paying for themselves no longer are.
	const int32 Seg = 8;
	const float R = StudRadius * Scale;
	const float H = StudHeight * Scale;

	// Wall. Each segment is its own quad with its own normal, so the stud shades as a faceted
	// cylinder rather than a smooth one -- which is what a moulded plastic stud actually
	// looks like under a hard key light.
	for (int32 I = 0; I < Seg; I++)
	{
		const float A0 = (float)I       / (float)Seg * 2.0f * PI;
		const float A1 = (float)(I + 1) / (float)Seg * 2.0f * PI;
		const FVector D0(FMath::Cos(A0), FMath::Sin(A0), 0.0f);
		const FVector D1(FMath::Cos(A1), FMath::Sin(A1), 0.0f);
		const FVector N = ((D0 + D1) * 0.5f).GetSafeNormal();

		Quad(Centre + D0 * R,
		     Centre + D1 * R,
		     Centre + D1 * R + FVector(0, 0, H),
		     Centre + D0 * R + FVector(0, 0, H),
		     N);
	}

	// Cap.
	const FVector Up(0, 0, 1);
	const int32 Hub = Vert(Centre + FVector(0, 0, H), Up, FVector2D(0.5f, 0.5f));
	int32 First = INDEX_NONE, Prev = INDEX_NONE;
	for (int32 I = 0; I <= Seg; I++)
	{
		const float A = (float)I / (float)Seg * 2.0f * PI;
		const FVector P = Centre + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, H);
		const int32 Cur = Vert(P, Up, FVector2D(0.5f + FMath::Cos(A) * 0.5f, 0.5f + FMath::Sin(A) * 0.5f));
		if (Prev != INDEX_NONE)
		{
			Triangles.Add(Hub); Triangles.Add(Prev); Triangles.Add(Cur);
		}
		else
		{
			First = Cur;
		}
		Prev = Cur;
	}
	(void)First;
}

void FBrickMesh::Studded(const FVector& Centre, int32 TilesX, int32 TilesY, float Height,
                         bool bTopStuds, float StudScale)
{
	const float HalfX = (float)TilesX * TileSize * 0.5f;
	const float HalfY = (float)TilesY * TileSize * 0.5f;

	Box(Centre, HalfX, Height, HalfY);

	if (!bTopStuds)
	{
		return;
	}

	// Studs sit on a grid inset by half a pitch from the edges, which is where they are on a
	// real brick and why bricks tile seamlessly against each other.
	const int32 Nx = TilesX * StudsPerTile;
	const int32 Ny = TilesY * StudsPerTile;
	const float X0 = Centre.X - HalfX + StudPitch * 0.5f;
	const float Y0 = Centre.Y - HalfY + StudPitch * 0.5f;
	for (int32 Sy = 0; Sy < Ny; Sy++)
	{
		for (int32 Sx = 0; Sx < Nx; Sx++)
		{
			Stud(FVector(X0 + Sx * StudPitch, Y0 + Sy * StudPitch, Centre.Z + Height), StudScale);
		}
	}
}

void FBrickMesh::Courses(const FVector& Base, int32 TilesX, int32 TilesY, int32 CourseCount,
                         float CourseHeight, const FLinearColor& Colour, float Shade)
{
	const float HalfX = (float)TilesX * TileSize * 0.5f;
	const float HalfY = (float)TilesY * TileSize * 0.5f;

	for (int32 C = 0; C < CourseCount; C++)
	{
		const bool bTop   = (C == CourseCount - 1);
		const bool bAlt   = (C & 1) != 0;
		const float Inset = bAlt ? 1.2f : 0.0f;
		const float Z     = Base.Z + C * CourseHeight;

		// Alternating courses are a shade apart as well as a hair narrower. Real brickwork
		// reads as courses largely through slight colour variation between bricks, and the
		// eye picks that up even when the geometric step is too small to resolve.
		SetInk(bAlt ? Colour * Shade : Colour);
		Box(FVector(Base.X, Base.Y, Z), HalfX - Inset, CourseHeight, HalfY - Inset);

		if (bTop)
		{
			const int32 Nx = TilesX * StudsPerTile;
			const int32 Ny = TilesY * StudsPerTile;
			const float X0 = Base.X - HalfX + StudPitch * 0.5f;
			const float Y0 = Base.Y - HalfY + StudPitch * 0.5f;
			for (int32 Sy = 0; Sy < Ny; Sy++)
			{
				for (int32 Sx = 0; Sx < Nx; Sx++)
				{
					Stud(FVector(X0 + Sx * StudPitch, Y0 + Sy * StudPitch, Z + CourseHeight));
				}
			}
		}
	}
}

void FBrickMesh::Part(const FVector& Centre, float HalfX, float HalfY, float HalfZ,
                      bool bStud, float StudScale)
{
	Box(FVector(Centre.X, Centre.Y, Centre.Z - HalfZ), HalfX, HalfZ * 2.0f, HalfY);
	if (bStud)
	{
		Stud(FVector(Centre.X, Centre.Y, Centre.Z + HalfZ), StudScale);
	}
}

void FBrickMesh::Domed(const FVector& Centre, float Radius, float Height)
{
	Box(Centre,                                              Radius,         Height * 0.45f, Radius);
	Box(Centre + FVector(0, 0, Height * 0.45f),              Radius * 0.82f, Height * 0.35f, Radius * 0.82f);
	Box(Centre + FVector(0, 0, Height * 0.80f),              Radius * 0.55f, Height * 0.25f, Radius * 0.55f);
}

void FBrickMesh::Commit(UProceduralMeshComponent* Comp, int32 Section, bool bCollision) const
{
	if (!Comp)
	{
		return;
	}

	if (Vertices.Num() == 0)
	{
		Comp->ClearMeshSection(Section);
		return;
	}

	Comp->CreateMeshSection(Section, Vertices, Triangles, Normals, UVs, Colors, Tangents,
	                        bCollision);
}

void FBrickMesh::RockQuad(const FVector& A, const FVector& B, const FVector& C, const FVector& D,
                          int32 Subdiv)
{
	Subdiv = FMath::Clamp(Subdiv, 1, 8);

	// Displace once per grid sample and reuse, rather than once per sub-quad corner. Each
	// interior sample is shared by four sub-quads, so this is a 4x saving on the noise, which
	// is by far the most expensive thing in a rebuild.
	const int32 N = Subdiv + 1;
	TArray<FVector, TInlineAllocator<81>> P;
	P.SetNumUninitialized(N * N);

	for (int32 J = 0; J < N; J++)
	{
		const float V = (float)J / (float)Subdiv;
		const FVector Left  = FMath::Lerp(A, D, V);
		const FVector Right = FMath::Lerp(B, C, V);
		for (int32 I = 0; I < N; I++)
		{
			const float U = (float)I / (float)Subdiv;
			const FVector Flat = FMath::Lerp(Left, Right, U);
			P[J * N + I] = Flat + DeepCoreRock::Displace(Flat);
		}
	}

	for (int32 J = 0; J < Subdiv; J++)
	{
		for (int32 I = 0; I < Subdiv; I++)
		{
			const FVector& Q0 = P[J * N + I];
			const FVector& Q1 = P[J * N + I + 1];
			const FVector& Q2 = P[(J + 1) * N + I + 1];
			const FVector& Q3 = P[(J + 1) * N + I];

			// Normal from the displaced diagonal, so the shading follows the broken surface
			// rather than the ideal plane it was generated from.
			FVector Nrm = FVector::CrossProduct(Q1 - Q0, Q3 - Q0).GetSafeNormal();
			if (Nrm.IsNearlyZero())
			{
				continue;   // degenerate sample; dropping it is cheaper than a NaN normal
			}
			Quad(Q0, Q1, Q2, Q3, Nrm);
		}
	}
}
