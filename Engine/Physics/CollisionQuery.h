#pragma once

#include <vector>
#include "Trace.h"
#include "../World/Brush.h"

namespace Engine {

// Sweeps a point from `start` to `end` against a set of (already
// hull-expanded) brushes and returns the nearest impact. Because the hull's
// size was already baked into the brushes via Brush::ExpandForHull, the
// mover itself is treated as a single point here — this is the GoldSrc
// trick of colliding a point against expanded geometry instead of a box
// against the real geometry.
TraceResult TraceMove(const glm::vec3& start, const glm::vec3& end, const std::vector<Brush>& brushes);

// Removes the velocity component driving into the plane, leaving the
// component along it — the "slide along the wall" response used by every
// Quake-lineage movement model.
glm::vec3 ClipVelocity(const glm::vec3& velocity, const glm::vec3& normal, float overbounce = 1.0f);

// Sweeps `position` by `velocity * dt`, sliding along up to a few brush
// surfaces per frame (so corners don't stick) rather than simply stopping
// dead on first contact. Mutates position and velocity in place.
void SlideMove(glm::vec3& position, glm::vec3& velocity, float deltaTime, const std::vector<Brush>& brushes);

// Checks whether `position` is resting on top of (or within maxDistance
// above) some upward-facing brush surface, roughly under its footprint.
// This is deliberately NOT implemented by sweeping a short TraceMove
// straight down: a point already touching a surface at both the start and
// end of that probe never *crosses* the plane, so a sweep test reports no
// hit for the single most common ground-check case (resting still on
// flat ground) — this needs its own point-vs-brush "am I supported here"
// test instead.
TraceResult CheckGroundContact(const glm::vec3& position, float maxDistance, const std::vector<Brush>& brushes);

} // namespace Engine
