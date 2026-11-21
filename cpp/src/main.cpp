// main.cpp
//
// Entry point: opens the window and runs the main loop.
//
// Each frame does four things in order:
//   1. Read input   (platform/InputReader)
//   2. Update game  (game/Game, in fixed time steps)
//   3. Aim camera   (platform/CameraRig)
//   4. Draw         (platform/Renderer)
//
// Optional: `new-world --screenshot build/shot.png` runs a short moment,
// saves one frame to that file and quits. Handy for checking the view
// without playing.

#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>

#include "raylib.h"

#include "game/Game.h"
#include "game/GameData.h"
#include "game/World.h"
#include "platform/CameraRig.h"
#include "platform/InputReader.h"
#include "platform/RenderConfig.h"
#include "platform/Renderer.h"

int main(int argc, char** argv) {
    // --screenshot <file>: save a frame and exit (see header comment).
    const char* screenshotPath = nullptr;
    if (argc == 3 && std::strcmp(argv[1], "--screenshot") == 0) {
        screenshotPath = argv[2];
    }

    // Build the world before opening a window, so a broken map reports a
    // clear error instead of flashing an empty window.
    game::Game game = [] {
        try {
            return game::Game(game::World::fromRows(game::data::kStartingMap));
        } catch (const std::exception& error) {
            std::cerr << "Failed to load the world: " << error.what() << '\n';
            std::exit(1);
        }
    }();

    InitWindow(platform::render::kWindowWidth, platform::render::kWindowHeight,
               platform::render::kWindowTitle);
    SetTargetFPS(platform::render::kTargetFps);

    platform::CameraRig cameraRig;
    const platform::Renderer renderer;

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
    while (!WindowShouldClose()) { // Esc or the window's close button
        float frameTime = GetFrameTime();
        if (frameTime > kMaxFrameTime) {
            frameTime = kMaxFrameTime;
        }
        accumulator += frameTime;

        // Input is sampled once per frame and reused for every step in it.
        const game::PlayerInput input = platform::readPlayerInput();

        while (accumulator >= dt) {
            game.step(input, dt);
            accumulator -= dt;
        }

        cameraRig.follow(game.player().position);
        renderer.draw(game, cameraRig);

        // Screenshot mode: wait a few frames so the window has settled.
        if (screenshotPath != nullptr && ++frame == 30) {
            TakeScreenshot(screenshotPath);
            break;
        }
    }

    CloseWindow();
    return 0;
}
