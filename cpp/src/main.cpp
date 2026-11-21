// main.cpp
//
// Entry point: opens the window and runs the main loop.
//
// Each frame does three things in order:
//   1. Read input  (platform/InputReader)
//   2. Update game (game/Game, in fixed time steps)
//   3. Draw        (platform/Renderer)

#include <cstdlib>
#include <exception>
#include <iostream>

#include "raylib.h"

#include "game/Game.h"
#include "game/GameData.h"
#include "game/World.h"
#include "platform/InputReader.h"
#include "platform/RenderConfig.h"
#include "platform/Renderer.h"

int main() {
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

    platform::Renderer renderer;

    // Fixed time step: real elapsed time is collected in `accumulator`, and
    // the game advances in equal slices of kFixedTimeStep. Rendering can run
    // at any rate without changing how fast or how far the player moves.
    const float dt = game::data::kFixedTimeStep;
    float accumulator = 0.0f;

    // Cap on how much time one frame may simulate. After a long stall (e.g.
    // dragging the window) we skip the lost time instead of running hundreds
    // of catch-up steps at once.
    constexpr float kMaxFrameTime = 0.25f;

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

        renderer.draw(game);
    }

    CloseWindow();
    return 0;
}
