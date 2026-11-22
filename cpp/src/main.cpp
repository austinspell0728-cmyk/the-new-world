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
#include "platform/CameraRig.h"
#include "platform/InputReader.h"
#include "platform/RenderConfig.h"
#include "platform/Renderer.h"

// Finds a file in cpp/data/. Works when the game is started from cpp/ (as
// `make run` does) and when the exe is started from anywhere else, by also
// looking next to the exe's folder (cpp/build/../data).
std::string findDataFile(const std::string& name) {
    const std::string fromWorkingDir = "data/" + name;
    if (FileExists(fromWorkingDir.c_str())) {
        return fromWorkingDir;
    }
    return std::string(GetApplicationDirectory()) + "../data/" + name;
}

int main(int argc, char** argv) {
    // --screenshot <file>: save a frame and exit (see header comment).
    const char* screenshotPath = nullptr;
    if (argc == 3 && std::strcmp(argv[1], "--screenshot") == 0) {
        screenshotPath = argv[2];
    }
    const bool benchmark = (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0);

    // Build the world and load NPCs before opening a window, so a broken map
    // or data file reports a clear error instead of flashing an empty window.
    game::Game game = [] {
        try {
            // Terrain from the seed, then the hand-made town (WorldBuilder.h).
            game::World world = game::buildStartingWorld(game::data::kWorldSeed);
            std::vector<game::Npc> npcs = game::loadNpcFile(findDataFile("npcs.txt"));
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
    const platform::Renderer renderer(game.world());

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
        cameraRig.follow(game.player().position);
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
                          << (seconds * 1000.0 / kTimedFrames) << " ms per frame)\n";
                break;
            }
        }
    }

    CloseWindow();
    return 0;
}
