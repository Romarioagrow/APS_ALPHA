# Dev 2 checkpoint journal

## DEV2-FS-001 — Accepted full-scale visual baseline preservation

### Done

- Overview uses the distant home-system HISM proxy instead of exposing the huge materialized star sphere.
- Preview smoke ties the home catalogue record to its exact HISM `InstanceIndex`, world anchor and materialized `StableSystemId`.
- Gameplay stellar lighting prefers a materialized physical `AStar`; compressed cluster HISM is fallback-only.
- Logical star/system radius envelopes are hard non-colliding in constructor and `BeginPlay`.
- Added a read-only active stellar-target diagnostic and a gameplay handoff regression gate.
- Recorded the full-scale/history audit, evolution-over-rewrite ADR and Dev Surface ownership handoff.

### Accepted baselines

- Current SYSTEM layout, orbital composition, camera flow and planet-to-planet navigation are frozen pending causal evidence.
- Current STAR beside PLANET screen-space proportionality is accepted. Physical-radius validation remains separate; secondary-star readability must preserve the primary-star/planet angular relationship.
- Ocean material and living-world macro terrain direction are accepted and excluded from this checkpoint.
- Menu/orbital reference remains about 120 FPS / 8.3–8.4 ms on the user scene; surface references are tracked separately.

### Evidence and verification

- Scoped source diff and ownership audit complete.
- `git diff --cached --check` required clean before commit.
- Previous isolated generated handoff passed after the logical-zone collision fix, but the newly staged physical-star target gate has not been rebuilt or rendered yet.
- New build/render is deferred while the user's Unreal Editor owns the only heavy-process slot.

### Remaining

- Resource-safe suffix build and rendered Overview → Galaxy → Cluster → System → Star → Planet round-trip.
- Same-camera secondary-star readability A/B without changing SYSTEM layout, primary-star/planet angular size or canonical radii.
- Full-scale coordinate migration remains read-only pending an explicit architecture decision.
- Deep-Space Environment & Astronomical Object Diversity is a later Story after scaling, identity, stellar material and lighting P0 work; no implementation has started.
- Dev Surface owns ground material/LOD/foliage realization. Dev 2 only validates canonical planet profile/seed and starfield identity at gameplay handoff.

### Latest surface evidence handoff

- 2026-08-14 04:35:39: landing/gravity/character still functional, 108.4 FPS / 9.2 ms, about 2352 MB under a different camera/scene condition.
- Red/orange ground, weak shape cues and horizon bands remain Dev Surface material/LOD work. Dev 2 must verify selected planet type/subtype/profile/seed parity before any shared-contract change.
