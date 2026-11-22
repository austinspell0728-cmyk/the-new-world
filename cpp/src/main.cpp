// main.cpp
//
// Entry point: opens the window and runs the main loop.
//
// Each frame does these things in order:
//   1. Turn camera   (platform/InputReader -> platform/CameraRig)
//   2. Read movement (platform/InputReader, relative to the camera)
//   3. Update game   (game/Game, in fixed time steps)
//   4. Aim camera at the player and draw (platform/CameraRig, Renderer)
//
// Optional flags (both ignore the mouse and keyboard camera controls):
//   --screenshot build/shot.png  runs a short moment, saves one frame to
//                                that file and quits; for checking the view.
//   --screenshot build/shot.png x z yaw pitch
//                                the same, with the camera aimed at ground
//                                point (x, z) from the given angles (degrees).
//   --benchmark                  draws as fast as possible (no 60 FPS cap)
//                                for a few seconds, prints the average frame
//                                rate and quits; for measuring render cost.

#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "raylib.h"

#include "game/Game.h"
#include "game/GameData.h"
#include "game/NpcData.h"
#include "game/World.h"
#include "game/WorldBuilder.h"
#include "platform/BuildingData.h"
#include "platform/BuildingLayout.h"
#include "platform/CameraRig.h"
#include "platform/InputReader.h"
#include "platform/RenderConfig.h"
#include "platform/Renderer.h"

// Finds a file or folder under cpp/ (e.g. "data/npcs.txt"). Works when the
// game is started from cpp/ (as `make run` does) and when the exe is started
// from anywhere else, by also looking next to the exe's folder (cpp/build/..).
std::string findCppPath(const std::string& relative) {
    if (FileExists(relative.c_str()) || DirectoryExists(relative.c_str())) {
        return relative;
    }
    return std::string(GetApplicationDirectory()) + "../" + relative;
}

int main(int argc, char** argv) {
    // --screenshot <file>: save a frame and exit (see header comment).
    const char* screenshotPath = nullptr;
    if ((argc == 3 || argc == 7) && std::strcmp(argv[1], "--screenshot") == 0) {
        screenshotPath = argv[2];
    }
    // Optional for --screenshot: x z yaw pitch. Aims the camera at ground
    // point (x, z) instead of the player, facing `yaw` degrees (0 = south,
    // 180 = north) and looking down `pitch` degrees. For checking parts of
    // the town the default view does not show.
    const bool customView = (screenshotPath != nullptr && argc == 7);
    game::Vec3 viewTarget{};
    if (customView) {
        viewTarget = {std::strtof(argv[3], nullptr), 0.0f, std::strtof(argv[4], nullptr)};
    }
    const bool benchmark = (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0);

    // Build the world and load the data files before opening a window, so a
    // broken map or data file reports a clear error instead of flashing an
    // empty window.
    platform::BuildingMap buildings;
    std::vector<platform::PlacedBuilding> placedBuildings;
    game::Game game = [&] {
        try {
            // Terrain from the seed, then the hand-made town (WorldBuilder.h).
            game::World world = game::buildStartingWorld(game::data::kWorldSeed);
            std::vector<game::Npc> npcs = game::loadNpcFile(findCppPath("data/npcs.txt"));
            // Which buildings are drawn from kit models (look only).
            buildings = platform::findBuildings(world);
            placedBuildings = platform::matchBuildings(
                buildings, platform::loadBuildingFile(findCppPath("data/buildings.txt")));
            return game::Game(std::move(world), std::move(npcs));
        } catch (const std::exception& error) {
            std::cerr << "Failed to load the world: " << error.what() << '\n';
            std::exit(1);
        }
    }();

    InitWindow(platform::render::kWindowWidth, platform::render::kWindowHeight,
               platform::render::kWindowTitle);
    // 0 = no frame cap, so the benchmark measures how fast we can draw.
    SetTargetFPS(benchmark ? 0 : platform::render::kTargetFps);

    // Hide and lock the mouse to the window so moving it orbits the camera
    // without the pointer leaving the screen. Esc still quits.
    // Screenshot mode skips this and ignores camera input, so the saved
    // image always shows the default view.
    const bool interactive = (screenshotPath == nullptr && !benchmark);
    bool mouseCaptured = false;
    // Frames of mouse movement to ignore. Right after capturing, the first
    // reported delta can include the jump to the window centre.
    int ignoreMouseFrames = 0;
    if (interactive) {
        DisableCursor();
        mouseCaptured = true;
        ignoreMouseFrames = 1;
    }

    platform::CameraRig cameraRig;
    if (customView) {
        cameraRig.orbit(std::strtof(argv[5], nullptr) * DEG2RAD - cameraRig.yaw(),
                        std::strtof(argv[6], nullptr) * DEG2RAD - cameraRig.pitch());
    }
    const platform::Renderer renderer(game.world(), buildings, placedBuildings,
                                      findCppPath("assets/medieval") + "/");

    // Fixed time step: real elapsed time is collected in `accumulator`, and
    // the game advances in equal slices of kFixedTimeStep. Rendering can run
    // at any rate without changing how fast or how far the player moves.
    const float dt = game::data::kFixedTimeStep;
    float accumulator = 0.0f;

    // Cap on how much time one frame may simulate. After a long stall (e.g.
    // dragging the window) we skip the lost time instead of running hundreds
    // of catch-up steps at once.
    constexpr float kMaxFrameTime = 0.25f;

    int frame = 0;
    double benchmarkStart = 0.0;
    while (!WindowShouldClose()) { // Esc or the window's close button
        float frameTime = GetFrameTime();
        if (frameTime > kMaxFrameTime) {
            frameTime = kMaxFrameTime;
        }
        accumulator += frameTime;

        // 1. Turn the camera first, so movement below uses this frame's view.
        //    The camera is presentation, so it turns once per frame using real
        //    frame time rather than in fixed game steps.
        if (interactive) {
            // Tab frees the mouse for other windows, and captures it again.
            if (platform::toggleMouseCapturePressed()) {
                mouseCaptured = !mouseCaptured;
                if (mouseCaptured) {
                    DisableCursor();
                    ignoreMouseFrames = 1;
                } else {
                    EnableCursor();
                }
            }

            const bool useMouse = mouseCaptured && ignoreMouseFrames == 0;
            if (ignoreMouseFrames > 0) {
                --ignoreMouseFrames;
            }

            const platform::CameraInput look = platform::readCameraInput(frameTime, useMouse);
            cameraRig.orbit(look.yawDelta, look.pitchDelta);
        }

        // 2. WASD relative to the camera, turned into a world direction here
        //    in the platform layer. Sampled once per frame and reused for
        //    every game step in it.
        const game::PlayerInput input = platform::readPlayerInput(cameraRig.yaw());

        //    E is a one-off press, so it goes to the game once per frame here,
        //    not once per fixed step (which could apply it twice).
        if (interactive && platform::interactPressed()) {
            game.interact();
        }

        // 3. Advance the game in fixed steps.
        while (accumulator >= dt) {
            game.step(input, dt);
            accumulator -= dt;
        }

        // 4. Aim the camera at the player's new position, then draw.
        cameraRig.follow(customView ? viewTarget : game.player().position);
        renderer.draw(game, cameraRig);

        // Screenshot mode: wait a few frames so the window has settled.
        if (screenshotPath != nullptr && ++frame == 30) {
            TakeScreenshot(screenshotPath);
            break;
        }

        // Benchmark mode: skip a warm-up, then time a fixed number of frames.
        if (benchmark) {
            constexpr int kWarmupFrames = 60;
            constexpr int kTimedFrames = 600;
            ++frame;
            if (frame == kWarmupFrames) {
                benchmarkStart = GetTime();
            } else if (frame == kWarmupFrames + kTimedFrames) {
                const double seconds = GetTime() - benchmarkStart;
                std::cout << "Benchmark: " << kTimedFrames << " frames in " << seconds
                          << " s = " << (kTimedFrames / seconds) << " FPS ("
                          << (seconds * 1000.0 / kTimedFrames) << " ms per frame), "
                          << renderer.buildingDrawCount() << " building mesh draw(s) per frame\n";
                break;
            }
        }
    }

    CloseWindow();
    return 0;
}
