// GameTests.cpp
//
// Tests for the game rules. These build WITHOUT raylib, which proves the
// game layer really is independent of rendering. Run with `make test`.

#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../src/game/Game.h"
#include "../src/game/GameData.h"
#include "../src/game/World.h"

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "  FAIL: " << message << '\n';
        ++failures;
    }
}

void run(const char* name, const std::function<void()>& test) {
    const int before = failures;
    try {
        test();
    } catch (const std::exception& error) {
        std::cerr << "  FAIL: unexpected exception: " << error.what() << '\n';
        ++failures;
    }
    std::cout << (failures == before ? "PASS  " : "FAIL  ") << name << '\n';
}

// Runs `steps` fixed steps holding the same input.
void hold(game::Game& g, game::PlayerInput input, int steps) {
    for (int i = 0; i < steps; ++i) {
        g.step(input, game::data::kFixedTimeStep);
    }
}

// A 7x5 room with walls all round and the player in the middle.
game::World smallRoom() {
    return game::World::fromRows(std::vector<std::string_view>{
        "#######",
        "#.....#",
        "#..@..#",
        "#.....#",
        "#######",
    });
}

bool near(float a, float b, float tolerance = 0.0001f) { return std::abs(a - b) <= tolerance; }

} // namespace

int main() {
    run("Starting map loads and player spawns on a walkable tile", [] {
        const game::World world = game::World::fromRows(game::data::kStartingMap);
        const game::Vec2 start = world.playerStart();
        check(!game::isSolid(world.tileAt(static_cast<int>(start.x), static_cast<int>(start.y))),
              "spawn tile should be walkable");
    });

    run("Player spawns in the centre of the '@' tile", [] {
        const game::Game g(smallRoom());
        check(near(g.player().position.x, 3.5f) && near(g.player().position.y, 2.5f),
              "expected spawn at (3.5, 2.5)");
    });

    run("No input means no movement", [] {
        game::Game g(smallRoom());
        const game::Vec2 before = g.player().position;
        hold(g, {0, 0}, 30);
        check(near(g.player().position.x, before.x) && near(g.player().position.y, before.y),
              "player should not move");
        check(!g.player().moving, "player should not be marked as moving");
    });

    run("Moving right covers speed * time", [] {
        game::Game g(smallRoom());
        const float startX = g.player().position.x;
        hold(g, {1, 0}, 6); // 0.1 s at 5 tiles/s = 0.5 tiles, stays clear of the wall
        check(near(g.player().position.x, startX + 0.5f, 0.001f), "expected to move 0.5 tiles");
        check(g.player().facing == game::Facing::Right, "should face right");
    });

    run("Diagonal movement is not faster than straight movement", [] {
        game::Game straight(smallRoom());
        game::Game diagonal(smallRoom());
        const game::Vec2 start = straight.player().position;
        hold(straight, {1, 0}, 3);
        hold(diagonal, {1, 1}, 3);
        const float straightDist = game::length(straight.player().position - start);
        const float diagonalDist = game::length(diagonal.player().position - start);
        check(near(straightDist, diagonalDist, 0.001f), "distances should match");
    });

    run("Walls stop the player flush against them", [] {
        game::Game g(smallRoom());
        hold(g, {1, 0}, 120); // far longer than needed to reach the wall
        const float rightEdge = g.player().position.x + g.player().halfSize;
        check(rightEdge < 6.0f, "player must not enter the wall at x = 6");
        check(rightEdge > 5.99f, "player should end up touching the wall");
    });

    run("Player slides along a wall when moving diagonally into it", [] {
        game::Game g(smallRoom());
        hold(g, {1, 0}, 120); // pin against the right wall
        const float pinnedX = g.player().position.x;
        const float startY = g.player().position.y;
        hold(g, {1, 1}, 6);
        check(near(g.player().position.x, pinnedX), "x should stay pinned to the wall");
        check(g.player().position.y > startY, "y should still increase (sliding)");
    });

    run("Water blocks movement", [] {
        game::Game g(game::World::fromRows(std::vector<std::string_view>{
            "#####",
            "#@.~#",
            "#####",
        }));
        hold(g, {1, 0}, 120);
        check(g.player().position.x + g.player().halfSize < 3.0f, "player must not enter water");
    });

    run("Player cannot leave a map with no border walls", [] {
        game::Game g(game::World::fromRows(std::vector<std::string_view>{"@.."}));
        hold(g, {-1, -1}, 120);
        check(g.player().position.x - g.player().halfSize >= 0.0f, "stays inside left edge");
        check(g.player().position.y - g.player().halfSize >= 0.0f, "stays inside top edge");
    });

    run("Out-of-range input is clamped to normal speed", [] {
        game::Game normal(smallRoom());
        game::Game extreme(smallRoom());
        hold(normal, {1, 0}, 3);
        hold(extreme, {50, 0}, 3);
        check(near(normal.player().position.x, extreme.player().position.x),
              "input of 50 should move like input of 1");
    });

    run("Invalid maps are rejected", [] {
        auto rejects = [](std::vector<std::string_view> rows) {
            try {
                game::World::fromRows(rows);
            } catch (const std::runtime_error&) {
                return true;
            }
            return false;
        };
        check(rejects({}), "empty map");
        check(rejects({"#@#", "##"}), "uneven rows");
        check(rejects({"#.#", "###"}), "no start marker");
        check(rejects({"@@"}), "two start markers");
        check(rejects({"@x"}), "unknown character");
    });

    if (failures > 0) {
        std::cerr << failures << " check(s) failed.\n";
        return 1;
    }
    std::cout << "All tests passed.\n";
    return 0;
}
