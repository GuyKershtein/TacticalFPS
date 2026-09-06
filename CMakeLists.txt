cmake_minimum_required(VERSION 3.20)
project(TacticalFPS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependencies -----------------------------------------------------------
# We only pull in low-level libraries that replace platform-specific plumbing
# (windowing/input/context creation, math). We do NOT pull in any engine,
# rendering framework, or GL loader library — the OpenGL function pointers
# are loaded by our own Engine/Graphics/GLFunctions module.
include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

# miniaudio: a single-header low-level audio playback/mixing library (the
# "low-level library for Audio" the design brief allows) — not an engine or
# gameplay framework, just device I/O and mixing, exactly the same role
# GLFW plays for windowing. We still write our own AudioSystem/SoundBank on
# top of it, including the procedural sound synthesis (see Engine/Audio) —
# no real audio assets exist to load, so placeholder sounds are generated
# in-memory rather than shipped as files.
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.21
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(miniaudio)

# stb_truetype: a single-header low-level TrueType rasterizer (the "low-level
# library for image/font loading" the design brief allows) — parses glyph
# outlines and bakes a bitmap atlas, nothing more. It has no opinion about
# OpenGL, layout, or widgets; Engine/UI writes all of that itself, the same
# division of responsibility as miniaudio (device I/O only) vs AudioSystem.
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
# ENet: a small, portable reliable-UDP library (connection management,
# ordered/unordered channels, reliable and unreliable delivery) — the
# "low-level library for networking" the design brief allows. It knows
# nothing about game state; Engine/Networking + Game/Networking write the
# packet protocol, serialization, and client/host roles on top of it, the
# same division of responsibility as GLFW (windowing) or miniaudio (audio).
FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(enet)

FetchContent_MakeAvailable(stb)

# --- Engine + Game sources ---------------------------------------------------
add_executable(TacticalFPS
    Game/Main.cpp
    Game/GameApplication.cpp
    Game/Player/PlayerController.cpp
    Game/Player/WeaponInventory.cpp
    Game/Player/PlayerHealth.cpp
    Game/Weapons/Weapon.cpp
    Game/Weapons/WeaponPresets.cpp
    Game/Combat/HitscanSystem.cpp
    Game/Combat/TargetDummy.cpp
    Game/Economy/Wallet.cpp
    Game/Objectives/ObjectiveSite.cpp
    Game/Objectives/DemolitionCharge.cpp
    Game/Rounds/RoundManager.cpp
    Game/Bots/Bot.cpp
    Game/Bots/BotPerception.cpp
    Engine/Core/Window.cpp
    Engine/Core/Time.cpp
    Engine/Core/Tokenizer.cpp
    Engine/Input/InputManager.cpp
    Engine/Graphics/GLFunctions.cpp
    Engine/Graphics/Shader.cpp
    Engine/Graphics/Camera.cpp
    Engine/Graphics/Mesh.cpp
    Engine/Graphics/DebugLineRenderer.cpp
    Engine/Math/Plane.cpp
    Engine/Math/Random.cpp
    Engine/Navigation/NavGraph.cpp
    Engine/Physics/CollisionQuery.cpp
    Engine/World/Brush.cpp
    Engine/World/CollisionWorld.cpp
    Engine/World/Entity.cpp
    Engine/World/SpawnPointEntity.cpp
    Engine/World/NavWaypointEntity.cpp
    Engine/World/Level.cpp
    Engine/World/MapLoader.cpp
    Engine/World/MaterialDatabase.cpp
    Engine/Audio/AudioSystem.cpp
    Engine/Audio/SoundBank.cpp
    Engine/Audio/ProceduralSoundGenerator.cpp
    Engine/Audio/MiniaudioImpl.cpp
    Engine/Graphics/ParticleSystem.cpp
    Engine/Graphics/DecalSystem.cpp
    Game/Grenades/Grenade.cpp
    Game/Grenades/GrenadeTypes.cpp
    Game/Grenades/SmokeCloud.cpp
    Engine/UI/StbTrueTypeImpl.cpp
    Engine/UI/Font.cpp
    Engine/UI/TextRenderer.cpp
    Engine/UI/UIRenderer.cpp
    Game/UI/Hud.cpp
    Game/UI/BuyMenu.cpp
    Game/UI/PauseMenu.cpp
    Engine/Networking/NetChannel.cpp
    Game/Networking/NetworkManager.cpp
)

target_include_directories(TacticalFPS PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${miniaudio_SOURCE_DIR}
    ${stb_SOURCE_DIR}
    ${enet_SOURCE_DIR}/include
)

target_link_libraries(TacticalFPS PRIVATE glfw glm::glm opengl32 ole32 winmm enet ws2_32)

if (MSVC)
    target_compile_definitions(TacticalFPS PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(TacticalFPS PRIVATE /W4)
endif()

# Lets the engine locate Assets/ regardless of the process's working directory.
target_compile_definitions(TacticalFPS PRIVATE PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

set_target_properties(TacticalFPS PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
