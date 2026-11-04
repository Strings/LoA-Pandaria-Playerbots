# T-087: Taxi mount vanishes on long fast flights

**Agent:** claude
**Branch:** claude/T-087-taxi-mount
**Task status:** done (needs a real-client look on a long route)

## What changed
- Flight paths are capped at 50 yd/s, the 5.4.8 client's limit for flying splines (`MountedSpeed::ClientFlyingSplineCap`).
  At T-086j's 118 yd/s the server finished the spline first: `FlightPathMovementGenerator::DoFinalize` dismounted the
  player while the client was still flying the route at 50 yd/s, so the mount vanished mid-air.
- `MoveSplineInit` caps every flying spline again, flight paths included (T-086j had exempted them).
- `.mountspeed` prints the cap. `Taxi.SpeedOfTopFlight` keeps its meaning, clamped to 32 to 50 yd/s.

## How it was tested
- `unit_mounted_speed` (capped and under-cap cases); scenarios taxi-speed and mountspeed.
- Not reproducible headless: the cap lives in the client. Evidence: upstream TrinityCore caps flying splines at 50
  "the same way the client does", 5.4.8 has no unlimited-speed spline flag, and the symptom (mount gone, flight
  still moving) is exactly a server-side `Dismount` ahead of the client spline.

## Decisions made
- Flights now run at 50 yd/s (1.56x stock) instead of 118. Going faster needs a client exe patch; noted in KNOWLEDGE.md.

## For the other agent
- Never send a flying or Catmullrom spline above 50 yd/s: the client won't keep up, and server-side state runs ahead.

## Next
- If flights still feel slow, options: an exe patch for the cap, or shortening routes (fewer scenic nodes).

## Map additions
- None.
