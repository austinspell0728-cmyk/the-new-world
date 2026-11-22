// GameTests.cpp
//
// Tests for the game rules. These build WITHOUT raylib, which proves the
// game layer really is independent of rendering. Run with `make test`.
//
// Coordinates: the ground is X/Z and Y is height (see src/game/Vec3.h).
// In the test maps below, text columns are X and text rows are Z.

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../src/game/Game.h"
#include "../src/game/GameData.h"
#include "../src/game/World.h"
#include "../src/game/WorldBuilder.h"
// Raylib-free platform helper; safe to include in the test build.
#include "../src/platform/CameraRelativeMove.h"

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

constexpr float kPi = 3.14159265358979f;

} // namespace

int main() {
    // --- Tests carried over from the 2D prototype ----------------------------

    run("Starting map loads and player spawns on a walkable tile", [] {
        // Now built from terrain + the town layout (was a single fixed map).
        const game::World world = game::buildStartingWorld(game::data::kWorldSeed);
        const game::Vec3 start = world.playerStart();
        check(!game::isSolid(world.tileAt(static_cast<int>(start.x), static_cast<int>(start.z))),
              "spawn tile should be walkable");
    });

    run("Player spawns in the centre of the '@' tile", [] {
        const game::Game g(smallRoom());
        check(near(g.player().position.x, 3.5f) && near(g.player().position.z, 2.5f),
              "expected spawn at x = 3.5, z = 2.5");
        check(near(g.player().position.y, 0.0f), "expected spawn at ground height");
    });

    run("No input means no movement", [] {
        game::Game g(smallRoom());
        const game::Vec3 before = g.player().position;
        hold(g, {0, 0}, 30);
        check(near(g.player().position.x, before.x) && near(g.player().position.z, before.z),
              "player should not move");
        check(!g.player().moving, "player should not be marked as moving");
    });

    run("Moving right covers speed * time", [] {
        game::Game g(smallRoom());
        const float startX = g.player().position.x;
        hold(g, {1, 0}, 6); // 0.1 s at 5 tiles/s = 0.5 tiles, stays clear of the wall
        check(near(g.player().position.x, startX + 0.5f, 0.001f), "expected to move 0.5 tiles");
        check(near(g.player().facingYaw, kPi / 2.0f), "should face +X (yaw pi/2)");
    });

    run("Diagonal movement is not faster than straight movement", [] {
        game::Game straight(smallRoom());
        game::Game diagonal(smallRoom());
        const game::Vec3 start = straight.player().position;
        hold(straight, {1, 0}, 3);
        hold(diagonal, {1, 1}, 3);
        const float straightDist = game::lengthXZ(straight.player().position - start);
        const float diagonalDist = game::lengthXZ(diagonal.player().position - start);
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
        hold(g, {1, 0}, 120); // pin against the +X wall
        const float pinnedX = g.player().position.x;
        const float startZ = g.player().position.z;
        hold(g, {1, 1}, 6);
        check(near(g.player().position.x, pinnedX), "x should stay pinned to the wall");
        check(g.player().position.z > startZ, "z should still increase (sliding)");
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
        check(g.player().position.x - g.player().halfSize >= 0.0f, "stays inside the -X edge");
        check(g.player().position.z - g.player().halfSize >= 0.0f, "stays inside the -Z edge");
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

    // --- New for the 3D conversion ---------------------------------------------

    run("Map rows become Z and columns become X", [] {
        // '@' is in column 1, row 2, and the only water is at column 2, row 0.
        const game::World world = game::World::fromRows(std::vector<std::string_view>{
            "..~",
            "...",
            ".@.",
        });
        check(world.width() == 3 && world.depth() == 3, "width runs along X, depth along Z");
        check(near(world.playerStart().x, 1.5f) && near(world.playerStart().z, 2.5f),
              "start should be at x = 1.5, z = 2.5");
        check(world.tileAt(2, 0) == game::Tile::Water, "tileAt(x = 2, z = 0) should be water");
    });

    run("Player stays at ground height while walking", [] {
        game::Game g(smallRoom());
        hold(g, {1, 1}, 120);
        check(near(g.player().position.y, 0.0f), "y should stay 0 (no vertical movement yet)");
        check(near(g.player().height, game::data::kPlayerHeight), "height comes from GameData");
    });

    run("Any-angle direction moves at full speed in that direction", [] {
        // A camera rotated to an arbitrary angle produces directions like this.
        game::Game g(smallRoom());
        const game::Vec3 start = g.player().position;
        hold(g, {0.6f, 0.8f}, 3); // 0.05 s at 5 tiles/s = 0.25 tiles
        const game::Vec3 moved = g.player().position - start;
        check(near(game::lengthXZ(moved), 0.25f, 0.001f), "should move 0.25 tiles");
        check(near(moved.x / moved.z, 0.75f, 0.001f), "should keep the 0.6 : 0.8 direction");
    });

    run("Small inputs still walk at full speed", [] {
        game::Game full(smallRoom());
        game::Game tiny(smallRoom());
        hold(full, {1, 0}, 3);
        hold(tiny, {0.1f, 0}, 3);
        check(near(full.player().position.x, tiny.player().position.x),
              "direction is normalized, so length does not change speed");
    });

    run("Facing follows the walking direction and is kept when stopping", [] {
        game::Game g(smallRoom());
        hold(g, {0, 1}, 1);
        check(near(g.player().facingYaw, 0.0f), "+Z should be yaw 0");
        hold(g, {-1, 0}, 1);
        check(near(g.player().facingYaw, -kPi / 2.0f), "-X should be yaw -pi/2");
        hold(g, {0, 0}, 10);
        check(near(g.player().facingYaw, -kPi / 2.0f), "stopping should keep the last facing");
    });

    run("Sliding works at an arbitrary angle", [] {
        game::Game g(smallRoom());
        hold(g, {1, 0}, 120); // pin against the +X wall
        const float pinnedX = g.player().position.x;
        const float startZ = g.player().position.z;
        hold(g, {0.9f, -0.3f}, 6);
        check(near(g.player().position.x, pinnedX), "x should stay pinned to the wall");
        check(g.player().position.z < startZ, "z should decrease along the wall");
    });

    run("Non-finite input is treated as no input", [] {
        game::Game g(smallRoom());
        const game::Vec3 before = g.player().position;
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float inf = std::numeric_limits<float>::infinity();
        hold(g, {nan, 0}, 5);
        hold(g, {inf, inf}, 5);
        check(near(g.player().position.x, before.x) && near(g.player().position.z, before.z),
              "player should not move or become NaN");
    });

    // --- Camera-relative movement (platform/CameraRelativeMove.h) ---------------

    run("Default camera: W walks north (-Z) and D walks east (+X)", [] {
        const float yaw = kPi; // camera starts facing -Z
        const game::PlayerInput w = platform::cameraRelativeMove(1, 0, yaw);
        const game::PlayerInput d = platform::cameraRelativeMove(0, 1, yaw);
        check(near(w.moveX, 0.0f) && near(w.moveZ, -1.0f), "W should be (0, -1)");
        check(near(d.moveX, 1.0f) && near(d.moveZ, 0.0f), "D should be (1, 0)");
    });

    run("Camera facing east: W walks +X and D walks +Z", [] {
        const float yaw = kPi / 2.0f;
        const game::PlayerInput w = platform::cameraRelativeMove(1, 0, yaw);
        const game::PlayerInput d = platform::cameraRelativeMove(0, 1, yaw);
        check(near(w.moveX, 1.0f) && near(w.moveZ, 0.0f), "W should be (1, 0)");
        check(near(d.moveX, 0.0f) && near(d.moveZ, 1.0f), "D should be (0, 1)");
    });

    run("S and A are the exact opposites of W and D at any angle", [] {
        const float yaw = 0.7f;
        const game::PlayerInput w = platform::cameraRelativeMove(1, 0, yaw);
        const game::PlayerInput s = platform::cameraRelativeMove(-1, 0, yaw);
        const game::PlayerInput d = platform::cameraRelativeMove(0, 1, yaw);
        const game::PlayerInput a = platform::cameraRelativeMove(0, -1, yaw);
        check(near(w.moveX, -s.moveX) && near(w.moveZ, -s.moveZ), "S should mirror W");
        check(near(d.moveX, -a.moveX) && near(d.moveZ, -a.moveZ), "A should mirror D");
        check(near(w.moveX * d.moveX + w.moveZ * d.moveZ, 0.0f), "forward and right are perpendicular");
    });

    run("W + D with a rotated camera is no faster than W alone", [] {
        const float yaw = 0.7f;
        game::Game straight(smallRoom());
        game::Game diagonal(smallRoom());
        const game::Vec3 start = straight.player().position;
        hold(straight, platform::cameraRelativeMove(1, 0, yaw), 3);
        hold(diagonal, platform::cameraRelativeMove(1, 1, yaw), 3);
        check(near(game::lengthXZ(straight.player().position - start),
                   game::lengthXZ(diagonal.player().position - start), 0.001f),
              "distances should match");
    });

    run("Walking into a wall with a rotated camera slides along it", [] {
        game::Game g(smallRoom());
        hold(g, {1, 0}, 120); // pin against the +X wall
        const float pinnedX = g.player().position.x;
        const float startZ = g.player().position.z;
        // Camera turned 30 degrees off east: W pushes into the wall at an angle.
        hold(g, platform::cameraRelativeMove(1, 0, kPi / 2.0f + kPi / 6.0f), 6);
        check(near(g.player().position.x, pinnedX), "x should stay pinned to the wall");
        check(std::abs(g.player().position.z - startZ) > 0.1f, "player should slide along z");
    });

    // --- Town: buildings, props and the world builder ----------------------------

    run("Buildings block movement and the player stops flush against them", [] {
        game::Game g(game::World::fromRows(std::vector<std::string_view>{
            "#######",
            "#@..BB#",
            "#...BB#",
            "#######",
        }));
        hold(g, {1, 0}, 120);
        const float rightEdge = g.player().position.x + g.player().halfSize;
        check(rightEdge < 4.0f, "player must not enter the building at x = 4");
        check(rightEdge > 3.99f, "player should end up touching the building");
    });

    run("Player slides along a building wall", [] {
        game::Game g(game::World::fromRows(std::vector<std::string_view>{
            "#######",
            "#...BB#",
            "#@..BB#",
            "#.....#",
            "#######",
        }));
        hold(g, {1, 0}, 120); // pin against the building's west face
        const float pinnedX = g.player().position.x;
        const float startZ = g.player().position.z;
        hold(g, {1, 1}, 6);   // push into it while moving south, still beside it
        check(near(g.player().position.x, pinnedX), "x should stay pinned to the building");
        check(g.player().position.z > startZ, "player should slide south along the building");
        hold(g, {1, 1}, 60);  // keep going: past the building's corner
        check(g.player().position.x > pinnedX + 0.5f, "past the corner, east is open again");
    });

    run("Trees, stalls and the well block movement; plaza paving does not", [] {
        for (const char prop : {'T', 'S', 'W'}) {
            const std::string row = std::string("#@:") + prop + "#";
            game::Game g(game::World::fromRows(std::vector<std::string>{"#####", row, "#####"}));
            hold(g, {1, 0}, 120);
            check(g.player().position.x + g.player().halfSize < 3.0f,
                  std::string("player must not enter a '") + prop + "' tile");
            check(g.player().position.x > 2.0f, "player should cross the walkable plaza tile");
        }
    });

    run("Terrain is grass inside a border wall, the same for the same seed", [] {
        const game::MapRows terrain = game::generateTerrain(7, 6, 4);
        check(terrain.size() == 4 && terrain[0].size() == 6, "terrain should be 6 wide, 4 deep");
        check(terrain[0] == "######" && terrain[3] == "######", "top and bottom rows are wall");
        check(terrain[1] == "#....#" && terrain[2] == "#....#", "inside is grass, edges are wall");
        check(terrain == game::generateTerrain(7, 6, 4), "same seed gives the same terrain");
    });

    run("Terrain rejects sizes too small to have an inside", [] {
        bool threw = false;
        try {
            game::generateTerrain(1, 2, 5);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        check(threw, "a 2-wide terrain should be rejected");
    });

    run("Stamping a layout keeps terrain where the layout has spaces", [] {
        game::MapRows rows = game::generateTerrain(1, 6, 5);
        game::stampLayout(rows, {"B ", " @"}, 2, 1);
        check(rows[1] == "#.B..#", "row 1: 'B' stamped at x = 2, space kept grass");
        check(rows[2] == "#..@.#", "row 2: '@' stamped at x = 3");
    });

    run("Stamping rejects layouts that do not fit or have uneven rows", [] {
        auto rejects = [](std::vector<std::string_view> layout, int ox, int oz) {
            game::MapRows rows = game::generateTerrain(1, 5, 5);
            try {
                game::stampLayout(rows, layout, ox, oz);
            } catch (const std::invalid_argument&) {
                return true;
            }
            return false;
        };
        check(rejects({"BBB"}, 3, 1), "layout running off the east edge");
        check(rejects({"B", "B"}, 1, 4), "layout running off the south edge");
        check(rejects({"B"}, -1, 0), "negative offset");
        check(rejects({"BB", "B"}, 1, 1), "uneven layout rows");
    });

    run("The town is stamped at its offset in the starting world", [] {
        const game::World world = game::buildStartingWorld(game::data::kWorldSeed);
        check(world.width() == game::data::kWorldWidth && world.depth() == game::data::kWorldDepth,
              "world size comes from GameData");
        // The well is at column 21, row 15 of the town layout.
        check(world.tileAt(game::data::kTownOffsetX + 21, game::data::kTownOffsetZ + 15) ==
                  game::Tile::Well,
              "well should sit at its layout position plus the offset");
        check(world.tileAt(1, 1) == game::Tile::Grass, "terrain shows outside the town");
        check(world.tileAt(0, 0) == game::Tile::Wall, "terrain border wall is kept");
    });

    if (failures > 0) {
        std::cerr << failures << " check(s) failed.\n";
        return 1;
    }
    std::cout << "All tests passed.\n";
    return 0;
}
