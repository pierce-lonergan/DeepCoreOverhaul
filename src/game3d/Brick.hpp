// Brick.hpp : procedural construction-toy geometry.
//
// WHY BRICKS
// ----------
// The single biggest reason this game reads as a prototype is that everything is an
// untextured box. Bricks fix that without a texture, an artist, or a content pipeline:
// studs give every surface a repeating highlight, a silhouette that catches light, and a
// consistent unit of scale. A wall built from bricks reads as an object; a wall built from
// one big box reads as a placeholder. It is the same trick that makes voxel games look
// deliberate rather than unfinished.
//
// ON WHAT THIS IS AND IS NOT
// --------------------------
// These are GENERIC studded building blocks. The original patents on the stud-and-tube
// brick expired decades ago and compatible bricks are made by many companies, which is why
// generic brick geometry is fine to generate.
//
// What is NOT fine, and is deliberately not done here:
//   - the LEGO name, logo, or any branding. It appears nowhere, and no stud carries a mark.
//   - the minifigure. Its specific shape is a registered trade mark in several
//     jurisdictions, so the characters in this game are NOT minifigures: they have tapered
//     bodies, articulated limbs and rounded helmets rather than the cylindrical head, C-grip
//     hands and trapezoidal torso that define that design.
// Generic bricks, original characters. That distinction is the whole reason this file can
// exist.
//
// DIMENSIONS
// Real construction bricks use a 5:6 ratio -- a 1x1 brick is 8mm across and 9.6mm tall.
// Keeping that ratio is most of why a brick "looks right"; getting it wrong is immediately
// visible even to someone who could not say why. One world unit here is one stud.
//

#pragma once

#include <cmath>

#include <windows.h>
#include <gl/GL.h>

namespace Brick
{

const float STUD_PITCH   = 1.0f;    ///< centre-to-centre spacing of studs
const float PLATE_HEIGHT = 0.40f;   ///< a plate; three plates make a brick
const float BRICK_HEIGHT = 1.20f;   ///< 5:6 against the stud pitch, as real bricks are
const float STUD_RADIUS  = 0.30f;
const float STUD_HEIGHT  = 0.18f;

/// A stud: a short cylinder with a cap. Twelve segments is the sweet spot -- eight reads as
/// a polygon at close range, and twenty-four costs geometry nobody can see.
inline void Stud(float cx, float cy, float cz, float scale = 1.0f)
{
	const int SEG = 12;
	const float r = STUD_RADIUS * scale;
	const float h = STUD_HEIGHT * scale;

	// Wall
	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i <= SEG; i++) {
		const float a = (float)i / (float)SEG * 6.28318530718f;
		const float ca = std::cos(a), sa = std::sin(a);
		glNormal3f(ca, 0.0f, sa);
		glVertex3f(cx + ca * r, cy,     cz + sa * r);
		glVertex3f(cx + ca * r, cy + h, cz + sa * r);
	}
	glEnd();

	// Cap
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0.0f, 1.0f, 0.0f);
	glVertex3f(cx, cy + h, cz);
	for (int i = 0; i <= SEG; i++) {
		const float a = (float)i / (float)SEG * 6.28318530718f;
		glVertex3f(cx + std::cos(a) * r, cy + h, cz + std::sin(a) * r);
	}
	glEnd();
}

/// A plain box with normals. The body of every brick.
inline void Box(float cx, float cy, float cz, float hx, float hy, float hz)
{
	const float x0 = cx - hx, x1 = cx + hx;
	const float y0 = cy,      y1 = cy + hy;
	const float z0 = cz - hz, z1 = cz + hz;

	glBegin(GL_QUADS);
	glNormal3f(0, 1, 0);
	glVertex3f(x0,y1,z0); glVertex3f(x1,y1,z0); glVertex3f(x1,y1,z1); glVertex3f(x0,y1,z1);
	glNormal3f(0, -1, 0);
	glVertex3f(x0,y0,z1); glVertex3f(x1,y0,z1); glVertex3f(x1,y0,z0); glVertex3f(x0,y0,z0);
	glNormal3f(0, 0, -1);
	glVertex3f(x0,y0,z0); glVertex3f(x1,y0,z0); glVertex3f(x1,y1,z0); glVertex3f(x0,y1,z0);
	glNormal3f(0, 0, 1);
	glVertex3f(x1,y0,z1); glVertex3f(x0,y0,z1); glVertex3f(x0,y1,z1); glVertex3f(x1,y1,z1);
	glNormal3f(-1, 0, 0);
	glVertex3f(x0,y0,z1); glVertex3f(x0,y0,z0); glVertex3f(x0,y1,z0); glVertex3f(x0,y1,z1);
	glNormal3f(1, 0, 0);
	glVertex3f(x1,y0,z0); glVertex3f(x1,y0,z1); glVertex3f(x1,y1,z1); glVertex3f(x1,y1,z0);
	glEnd();
}

/// A studded brick, `studsX` by `studsZ`, `height` tall, with studs on top.
///
/// The origin is the CENTRE of the footprint at the BOTTOM face, because bricks stack
/// upward from a floor and that makes placement arithmetic trivial.
inline void Studded(float cx, float cy, float cz,
					int studsX, int studsZ, float height,
					bool topStuds = true, float studScale = 1.0f)
{
	const float hx = (float)studsX * STUD_PITCH * 0.5f;
	const float hz = (float)studsZ * STUD_PITCH * 0.5f;

	Box(cx, cy, cz, hx, height, hz);

	if (!topStuds) return;

	// Studs sit on a grid inset by half a pitch from the edges, which is where they are on
	// a real brick and why bricks tile seamlessly.
	const float x0 = cx - hx + STUD_PITCH * 0.5f;
	const float z0 = cz - hz + STUD_PITCH * 0.5f;
	for (int sz = 0; sz < studsZ; sz++) {
		for (int sx = 0; sx < studsX; sx++) {
			Stud(x0 + (float)sx * STUD_PITCH, cy + height,
				 z0 + (float)sz * STUD_PITCH, studScale);
		}
	}
}

/// A small brick for character parts, sized in world units rather than studs, with a single
/// optional stud on top. Characters are assembled from these.
inline void Part(float cx, float cy, float cz, float hx, float hy, float hz,
				 bool stud = false, float studScale = 0.55f)
{
	Box(cx, cy - hy, cz, hx, hy * 2.0f, hz);
	if (stud) Stud(cx, cy + hy, cz, studScale);
}

/// A rounded-ish part, for helmets and heads. Approximated by stacking three boxes of
/// decreasing width, which is far cheaper than a sphere and reads better against the
/// hard-edged bricks around it than a smooth sphere would.
inline void Domed(float cx, float cy, float cz, float r, float h)
{
	Box(cx, cy,               cz, r,         h * 0.45f, r);
	Box(cx, cy + h * 0.45f,   cz, r * 0.82f, h * 0.35f, r * 0.82f);
	Box(cx, cy + h * 0.80f,   cz, r * 0.55f, h * 0.25f, r * 0.55f);
}

} // namespace Brick
