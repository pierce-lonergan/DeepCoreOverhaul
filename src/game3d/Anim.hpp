// Anim.hpp : what makes a character look alive.
//
// Pure maths, stdlib only -- no OpenGL, no Windows, no game state. That is deliberate and
// follows the same rule as DeepCoreLogic.hpp: the decisions live somewhere testable, and the
// renderer only draws the answer. tools/harness/ can therefore assert that a walk cycle's
// feet stay planted and a spring never explodes, without a window.
//
// WHAT ACTUALLY SELLS "ALIVE"
// ---------------------------
// Not polygon count. A figure made of eight boxes reads as alive if it obeys a handful of
// rules that animators formalised decades ago, and reads as a toy if it does not:
//
//   1. Nothing moves linearly. Real motion eases; linear interpolation is the single most
//      recognisable tell of programmer animation.
//   2. Things overlap. A limb lags the body that drives it; a head settles after the torso
//      stops. Everything arriving simultaneously looks mechanical.
//   3. Weight is communicated by timing, not by size. A heavy creature anticipates longer
//      and settles slower. Scale alone does not read as mass.
//   4. Nothing is ever perfectly still. An idle character breathes, shifts and blinks; a
//      frozen one looks dead even mid-scene.
//   5. Contact matters. A foot that slides during its stance destroys the illusion faster
//      than almost anything else, which is why the walk cycle here is built around foot
//      plant rather than around leg rotation.
//

#pragma once

#include <cmath>
#include <cstddef>

namespace Anim
{

const float PI  = 3.14159265358979f;
const float TAU = 6.28318530717959f;

inline float Clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float Lerp(float a, float b, float t)    { return a + (b - a) * t; }

/// Wrap to [0,1). Cycle phases are used everywhere below and must never go negative.
inline float Wrap01(float t)
{
	t = std::fmod(t, 1.0f);
	return (t < 0.0f) ? t + 1.0f : t;
}

/**********************************************************************************
 ******** Easing -- rule 1
 **********************************************************************************/

/// Smoothstep. The cheapest possible fix for linear motion, and the highest value one.
inline float EaseInOut(float t)
{
	t = Clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

inline float EaseOut(float t) { t = Clamp(t, 0.0f, 1.0f); return 1.0f - (1.0f - t) * (1.0f - t); }
inline float EaseIn(float t)  { t = Clamp(t, 0.0f, 1.0f); return t * t; }

/// Overshoot and settle. For anything that snaps into place -- a head turn, a tool raise.
/// Nothing in nature arrives exactly on target and stops.
inline float EaseBack(float t, float overshoot = 1.70158f)
{
	t = Clamp(t, 0.0f, 1.0f);
	const float s = overshoot;
	const float u = t - 1.0f;
	return u * u * ((s + 1.0f) * u + s) + 1.0f;
}

/// Decaying bounce, for landings and impacts.
inline float Bounce(float t, float decay = 6.0f, float freq = 14.0f)
{
	t = Clamp(t, 0.0f, 1.0f);
	return 1.0f - std::exp(-decay * t) * std::cos(freq * t);
}

/**********************************************************************************
 ******** Spring-damper -- rule 2, and the best value per line in this file
 **********************************************************************************/

/// A critically-damped-ish spring. Point it at a target every frame and it produces lag,
/// overshoot and settle for free. Used for head look-at, tool sway, body lean, camera
/// follow -- anywhere something should CHASE rather than SNAP.
///
/// Stability note: with a large dt and a stiff spring this can diverge, so dt is clamped.
/// A visual system that explodes when the window is dragged is not acceptable.
struct Spring
{
	float value = 0.0f;
	float vel = 0.0f;
	float stiffness = 90.0f;
	float damping = 14.0f;

	void Step(float target, float dt)
	{
		if (dt > 0.05f) dt = 0.05f;
		const float a = (target - value) * stiffness - vel * damping;
		vel += a * dt;
		value += vel * dt;
	}
	void Snap(float v) { value = v; vel = 0.0f; }
};

/**********************************************************************************
 ******** Walk cycle -- rule 5
 **********************************************************************************/

/// One leg's pose at a point in the cycle.
struct LegPose
{
	float hip = 0.0f;    ///< degrees, positive swings forward
	float knee = 0.0f;   ///< degrees, positive bends back
	float lift = 0.0f;   ///< world units the foot is off the ground
};

/// Phase 0.0-0.5 is stance (foot planted, body passes over it); 0.5-1.0 is swing.
///
/// The asymmetry is the point. A sine wave on both hip and knee gives the classic
/// "marionette" walk, because it spends equal time in stance and swing and lifts the foot
/// symmetrically. Real walks plant hard and lift fast: the foot is down for slightly more
/// than half the cycle, the knee stays nearly straight through stance to carry weight, and
/// bends sharply during swing so the toe clears the floor.
inline LegPose WalkLeg(float phase, float stride = 26.0f)
{
	phase = Wrap01(phase);
	LegPose p;

	if (phase < 0.5f) {
		// STANCE. The hip rotates back at a constant rate because the foot is fixed to the
		// ground and the body is travelling over it -- this is what stops the foot sliding.
		const float t = phase / 0.5f;
		p.hip  = Lerp(stride, -stride, t);
		p.knee = 4.0f + 8.0f * std::sin(t * PI);   // a little give under load
		p.lift = 0.0f;
	}
	else {
		// SWING. Faster than stance in wall-clock terms because it eases; the knee bends
		// hard early to clear the floor, then extends to reach for the next plant.
		const float t = (phase - 0.5f) / 0.5f;
		const float e = EaseInOut(t);
		p.hip  = Lerp(-stride, stride, e);
		p.knee = 42.0f * std::sin(t * PI);
		p.lift = 0.16f * std::sin(t * PI);
	}
	return p;
}

/// Torso bob. Runs at TWICE the leg frequency -- the body rises once per step, not once per
/// cycle -- and that doubling is what makes a walk read as a walk. Getting it wrong is the
/// most common mistake in hand-written locomotion.
inline float WalkBob(float phase, float amount = 0.045f)
{
	return -std::abs(std::sin(Wrap01(phase) * TAU)) * amount;
}

/// Side-to-side weight shift, at the leg frequency, a quarter-cycle behind the bob.
inline float WalkSway(float phase, float amount = 0.03f)
{
	return std::sin(Wrap01(phase) * TAU - PI * 0.5f) * amount;
}

/// Arms counter-swing the legs: opposite arm to leg, and slightly damped relative to the
/// hips because arms are passive in a walk rather than driven.
inline float WalkArm(float phase, float amount = 18.0f)
{
	return std::sin(Wrap01(phase) * TAU + PI) * amount;
}

/**********************************************************************************
 ******** Idle -- rule 4
 **********************************************************************************/

/// Breathing. Two incommensurate frequencies so it never visibly loops; a single sine on a
/// short period is recognisable as a loop within a few seconds.
inline float IdleBreath(float t, float amount = 0.018f)
{
	return (std::sin(t * 1.7f) * 0.7f + std::sin(t * 0.9f) * 0.3f) * amount;
}

/// Occasional weight shift, so a standing figure is not a statue. Returns roughly -1..1 and
/// is near zero most of the time.
inline float IdleShift(float t)
{
	const float slow = std::sin(t * 0.31f);
	return slow * slow * slow;    // cubed: mostly flat, with occasional excursions
}

/**********************************************************************************
 ******** Squash and stretch -- rule 3
 **********************************************************************************/

struct Squash { float x = 1.0f, y = 1.0f, z = 1.0f; };

/// Vertical velocity drives the deformation, and volume is preserved: stretch on the way up
/// and thin out, squash on landing and widen. Preserving volume is what separates squash and
/// stretch from simply scaling, which reads as a bug.
inline Squash SquashFromVelocity(float verticalVel, float strength = 0.10f, float maxV = 6.0f)
{
	const float v = Clamp(verticalVel / maxV, -1.0f, 1.0f);
	Squash s;
	s.y = 1.0f + v * strength;
	const float inv = 1.0f / std::sqrt(s.y > 0.05f ? s.y : 0.05f);
	s.x = s.z = inv;
	return s;
}

/// A one-shot impact squash that recovers with a bounce. Feed it seconds since the impact.
inline Squash SquashImpact(float since, float duration = 0.35f, float depth = 0.28f)
{
	Squash s;
	if (since < 0.0f || since > duration) return s;
	const float t = since / duration;
	const float amount = depth * (1.0f - Bounce(t)) * (1.0f - t);
	s.y = 1.0f - amount;
	const float inv = 1.0f / std::sqrt(s.y > 0.05f ? s.y : 0.05f);
	s.x = s.z = inv;
	return s;
}

/**********************************************************************************
 ******** Two-bone IK
 **********************************************************************************/

struct IkResult { float upper = 0.0f; float lower = 0.0f; bool reached = true; };

/// Law of cosines. Given a target distance and two bone lengths, produce the joint angles.
/// Used to plant feet on uneven ground and to make a drilling arm actually touch the rock
/// rather than wave near it.
///
/// Out of reach is not an error: the limb straightens and points at the target, which is
/// exactly what a real limb does when it cannot reach.
inline IkResult TwoBoneIK(float targetDist, float upperLen, float lowerLen)
{
	IkResult r;
	const float maxReach = upperLen + lowerLen;
	const float minReach = std::abs(upperLen - lowerLen);
	const float d = Clamp(targetDist, minReach + 1e-4f, maxReach - 1e-4f);

	if (targetDist >= maxReach) { r.reached = false; return r; }

	const float cosUpper = (upperLen * upperLen + d * d - lowerLen * lowerLen) / (2.0f * upperLen * d);
	const float cosKnee  = (upperLen * upperLen + lowerLen * lowerLen - d * d) / (2.0f * upperLen * lowerLen);

	r.upper = std::acos(Clamp(cosUpper, -1.0f, 1.0f)) * 180.0f / PI;
	r.lower = 180.0f - std::acos(Clamp(cosKnee, -1.0f, 1.0f)) * 180.0f / PI;
	return r;
}

/**********************************************************************************
 ******** Look-at with limits
 **********************************************************************************/

/// Shortest signed angular difference in degrees, wrapped to [-180,180]. Without this a head
/// turning from 350 to 10 degrees takes the long way round, which looks like a seizure.
inline float AngleDelta(float from, float to)
{
	float d = std::fmod(to - from + 540.0f, 360.0f) - 180.0f;
	return d;
}

/// Head yaw toward a target, limited so a character never turns its head further than a neck
/// allows. Beyond the limit the body should turn instead -- the caller's job.
inline float LookAtYaw(float currentYaw, float desiredYaw, float limitDeg = 75.0f)
{
	const float d = Clamp(AngleDelta(currentYaw, desiredYaw), -limitDeg, limitDeg);
	return currentYaw + d;
}

/**********************************************************************************
 ******** Anticipation
 **********************************************************************************/

/// A wind-up curve: pull back, then strike, then recover. Returns -1..1 where negative is
/// the anticipation. Telegraphing an attack this way is what makes a creature feel like it
/// DECIDED to attack rather than teleporting into a hit -- the same principle as the wave
/// director's telegraph, applied to a single animation.
inline float AttackCurve(float t, float windUp = 0.35f, float strike = 0.15f)
{
	t = Clamp(t, 0.0f, 1.0f);
	if (t < windUp)              return -EaseInOut(t / windUp);
	if (t < windUp + strike)     return Lerp(-1.0f, 1.0f, EaseIn((t - windUp) / strike));
	const float r = (t - windUp - strike) / (1.0f - windUp - strike);
	return 1.0f - EaseOut(r);
}

/// Emerging from the ground: a slow push, a burst through, then a settle. Height fraction
/// 0..1 for a creature climbing out of rock.
inline float EmergeCurve(float t)
{
	t = Clamp(t, 0.0f, 1.0f);
	if (t < 0.55f) return EaseIn(t / 0.55f) * 0.55f;          // straining upward
	return 0.55f + EaseBack((t - 0.55f) / 0.45f) * 0.45f;      // bursts out and settles
}

} // namespace Anim
