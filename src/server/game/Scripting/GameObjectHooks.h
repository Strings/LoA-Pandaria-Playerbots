#ifndef ULTIMATEMOP_GAMEOBJECT_HOOKS_H
#define ULTIMATEMOP_GAMEOBJECT_HOOKS_H
class GameObject;
class WorldObject;
// Per-viewer gameobject hooks for custom systems (per-character treasures first). A small header of its own so
// GameObject.cpp reaches them without touching GameObject.h or ScriptMgr.h.
namespace GameObjectHooks
{
    // True when `go` must be hidden from `seer`. Runs on map threads for every visibility check of every
    // gameobject: return early on the entry and read only `go` and `seer`.
    using HideFilter = bool (*)(GameObject const* go, WorldObject const* seer);
    // True when `go` should sparkle and be usable for `seer` (dynamic flags are built per viewer).
    using SparkleFilter = bool (*)(GameObject const* go, WorldObject const* seer);

    // Register at script startup only; filters live for the server lifetime.
    bool RegisterHideFilter(HideFilter filter);
    bool RegisterSparkleFilter(SparkleFilter filter);
    bool Hides(GameObject const* go, WorldObject const* seer);
    bool Sparkles(GameObject const* go, WorldObject const* seer);
}
#endif
