# TacticalFPS

A tactical first-person shooter inspired by Counter-Strike 1.6/Source, built
entirely from scratch in C++17 and raw OpenGL 3.3. There is no commercial
game engine, no existing FPS framework, and no high-level rendering library
anywhere in this codebase — every renderer, physics system, and gameplay
system is original.

The only third-party code is a handful of low-level platform libraries that
replace things no engine should reinvent (windowing, math, audio device I/O,
font rasterization, network transport). Everything built on top of them —
the OpenGL function loader, the collision system, the movement model, the
weapons, the bot AI, the UI, the wire protocol — is hand-written for this
project.

## What's here

Built across 10 milestones, in order:

1. **Engine basics** — window/input/GL context via GLFW, a hand-rolled
   OpenGL 3.3 core function loader (no GLAD/GLEW), a free-fly camera.
2. **Movement** — Quake-lineage acceleration/friction/air-accel, gravity,
   jump, crouch (with a smoothly-interpolated eye height and hull swap),
   sweep-and-slide collision, and stair-stepping.
3. **Weapons** — data-driven hitscan weapons with recoil, spread (moving/
   crouching/first-shot modifiers), magazine + reserve ammo, and reload.
4. **Maps** — brush-based convex level geometry (GoldSrc/Quake-style),
   hull-expansion collision (so movement is a point-vs-expanded-brush
   problem instead of box-vs-real-brush), a text `.tmap` format, and a
   material system driving per-surface color/sound.
5. **Rounds** — a Buy → Active → Round-End loop, two teams (Assault/
   Guardian), an economy (round win/loss/kill/objective payouts), and an
   original plant-and-defuse objective mode.
6. **Bots** — a waypoint nav graph (auto-linked by line-of-sight, not
   hand-authored edges) with A* pathfinding, vision-cone + line-of-sight +
   hearing perception, and bots that play through the same
   `PlayerController`/`Weapon`/`PlayerHealth` code a human uses.
7. **Audio, particles, grenades** — a miniaudio-backed 3D audio system with
   procedurally synthesized sounds (no audio assets), particle/decal
   systems for bullet impacts, and four grenade types (Fragmentation,
   Smoke, Flashbang, Decoy) with real physics and gameplay effects.
8. **UI** — a from-scratch text renderer (stb_truetype glyph atlas +
   batched textured quads) and 2D quad renderer, driving a HUD, an
   interactive buy menu, and a pause menu.
9. **Debugging tools** — in-engine toggles for nav-graph visualization,
   collision wireframe, a live stats panel, noclip, and god mode.
10. **Networking** — an authoritative client-server model over ENet
    (reliable UDP) where the host runs the exact same simulation code for
    every connected player, and clients are a thin presentation layer.

## Controls

| Key | Action |
|---|---|
| `WASD` | Move |
| Mouse | Look |
| `Space` | Jump |
| `Left Ctrl` | Crouch |
| `Left Shift` | Walk (quieter, slower) |
| `Left Click` | Fire |
| `R` | Reload |
| `1` – `4` | Switch weapon slot (Sidearm / Rifle / Shotgun / Knife) |
| `G` / `H` / `J` / `K` | Throw Frag / Smoke / Flash / Decoy grenade |
| `E` (hold) | Plant / defuse the charge |
| `B` | Open the buy menu (Buy phase only) |
| `Esc` | Pause |
| `F1` | Toggle nav graph visualization |
| `F2` | Toggle collision wireframe |
| `F3` | Toggle the debug stats panel |
| `F4` | Toggle noclip |
| `F5` | Toggle god mode |
| `T` | (debug) Swap the local player's team |

**Buy menu** (numbered hotkeys, no mouse needed): `1` Carbine ($2500),
`2` Street Sweeper ($1700), `3` Full Armor ($650), `4` Ammo Resupply ($200),
`5`–`8` Frag/Smoke/Flash/Decoy grenades ($200–$300). The Sidearm and Combat
Knife are free and always owned; grenades are consumed on throw and must be
rebought each round.

**Pause menu**: `R` or `Esc` to resume, `Q` to quit.

## Building

Requires CMake 3.20+ and a C++17 compiler (developed against MSVC / Visual
Studio 2022 on Windows). All dependencies are fetched automatically via
CMake `FetchContent` — no manual setup needed.

```
cmake -S . -B build
cmake --build build --config Debug
```

The executable is written to `build/Debug/TacticalFPS.exe`. Run it from
that directory (or via the generated Visual Studio solution) so it can find
`Assets/` relative to the project root, which is baked in at configure time.

### Dependencies (all fetched, none vendored)

| Library | Role |
|---|---|
| [GLFW](https://github.com/glfw/glfw) | Window, input, GL context creation |
| [GLM](https://github.com/g-truc/glm) | Vector/matrix math |
| [miniaudio](https://github.com/mackron/miniaudio) | Low-level audio device I/O |
| [stb](https://github.com/nothings/stb) (`stb_truetype`) | TrueType glyph rasterization |
| [ENet](https://github.com/lsalzman/enet) | Reliable/unreliable UDP transport |

## Running multiplayer

```
# Host (listens on port 7717 by default)
TacticalFPS.exe --host
TacticalFPS.exe --host=<port>

# Client
TacticalFPS.exe --connect=127.0.0.1
TacticalFPS.exe --connect=<host>:<port>
```

With no arguments, the game runs fully offline against bots — every
milestone's behavior is preserved exactly as-is regardless of networking.

The host is authoritative and runs the real simulation (physics, hit
registration, round/economy state) for itself and every connected client;
clients send input and render whatever the host's snapshot says, with no
client-side prediction. Known scope boundaries of the current networking
layer: remote clients always join the host's team with a fixed Carbine
loadout (no networked buy menu yet), and bots only perceive/target the
local host player.

## Project layout

```
Engine/     Reusable, game-agnostic systems (rendering, physics, audio,
            networking transport, UI primitives, world/collision data)
Game/       Everything specific to this game (weapons, bots, rounds,
            economy, HUD, menus, the game-facing network protocol)
Assets/     Shaders, the test map, and material definitions
```
