// GameTests.cpp
//
// Tests for the game rules. These build WITHOUT raylib, which proves the
// game layer really is independent of rendering. Run with `make test`.
//
// Coordinates: the ground is X/Z and Y is height (see src/game/Vec3.h).
// In the test maps below, text columns are X and text rows are Z.

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../src/game/CameraOcclusion.h"
#include "../src/game/Game.h"
#include "../src/game/GameData.h"
#include "../src/game/NpcData.h"
#include "../src/game/World.h"
#include "../src/game/WorldBuilder.h"
// Raylib-free platform helpers; safe to include in the test build.
#include "../src/platform/BuildingData.h"
#include "../src/platform/BuildingLayout.h"
#include "../src/platform/CameraRelativeMove.h"
#include "../src/platform/ControlsData.h"
#include "../src/platform/InputMath.h"
#include "../src/platform/TreeData.h"
#include "../src/platform/TreeLayout.h"

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

// Parses NPC text the same way the game parses data/npcs.txt.
std::vector<game::Npc> parseNpcText(const std::string& text) {
    std::istringstream in(text);
    return game::parseNpcs(in, "test");
}

// True if parsing `text` fails with an error message containing `expected`.
bool npcTextFails(const std::string& text, const std::string& expected) {
    try {
        parseNpcText(text);
    } catch (const std::runtime_error& error) {
        return std::string(error.what()).find(expected) != std::string::npos;
    }
    return false;
}

// A one-NPC block at tile (x, z) for building test games.
game::Npc npcAt(const std::string& name, int x, int z) {
    return parseNpcText("[npc]\nname = " + name + "\ntile = " + std::to_string(x) + ", " +
                        std::to_string(z) + "\ncolor = #ffffff\nline = Hi.\n")
        .front();
}

constexpr float kPi = 3.14159265358979f;

// Parses building text the same way the game parses data/buildings.txt.
platform::BuildingData parseBuildingText(const std::string& text) {
    std::istringstream in(text);
    return platform::parseBuildings(in, "test");
}

// True if parsing `text` fails with an error message containing `expected`.
bool buildingTextFails(const std::string& text, const std::string& expected) {
    try {
        parseBuildingText(text);
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()).find(expected) != std::string::npos) {
            return true;
        }
        std::cerr << "  (got: " << error.what() << ")\n";
    }
    return false;
}

// A complete [style] block named `name`: 9 lines, starting with "[style]".
std::string styleBlock(const std::string& name) {
    return "[style]\nname = " + name +
           "\nwall = w.gltf\nwindow = win.gltf\nwindow_frame = frame.gltf\ndoor = d.gltf\n"
           "door_leaf = leaf.gltf\ncorner = c.gltf\n\n";
}

// A complete [roof] block: 6 lines, starting with "[roof]".
std::string roofBlock(const std::string& name, int across, int along) {
    return "[roof]\nname = " + name + "\nmodel = " + name + ".gltf\ngable = g.gltf\nfits = " +
           std::to_string(across) + ", " + std::to_string(along) + "\n\n";
}

// A roof made for `across` x `along` tiles, for layout tests.
platform::BuildingRoof roofFits(int across, int along, const std::string& name = "roof") {
    platform::BuildingRoof roof;
    roof.name = name;
    roof.fitsAcross = across;
    roof.fitsAlong = along;
    return roof;
}

// Parses tree text the same way the game parses data/trees.txt.
platform::TreeData parseTreeText(const std::string& text) {
    std::istringstream in(text);
    return platform::parseTrees(in, "test");
}

// True if parsing `text` fails with an error message containing `expected`.
bool treeTextFails(const std::string& text, const std::string& expected) {
    try {
        parseTreeText(text);
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()).find(expected) != std::string::npos) {
            return true;
        }
        std::cerr << "  (got: " << error.what() << ")\n";
    }
    return false;
}

// A complete [tree] block: 6 lines, starting with "[tree]".
std::string treeBlock(const std::string& name, int weight = 1) {
    return "[tree]\nname = " + name + "\nmodel = " + name + ".gltf\nscale = 0.7\ntrunk = 0, 0\nweight = " +
           std::to_string(weight) + "\n";
}

// A 32x32 grass field with a border wall, the start at (15, 15), and the
// given extra tiles stamped on: {x, z, char}.
struct Stamp {
    int x;
    int z;
    char tile;
};
game::World fieldWith(const std::vector<Stamp>& stamps) {
    std::vector<std::string> rows(32, std::string(32, '.'));
    for (int i = 0; i < 32; ++i) {
        rows[0][static_cast<std::size_t>(i)] = rows[31][static_cast<std::size_t>(i)] = '#';
        rows[static_cast<std::size_t>(i)][0] = rows[static_cast<std::size_t>(i)][31] = '#';
    }
    rows[15][15] = '@';
    for (const Stamp& s : stamps) {
        rows[static_cast<std::size_t>(s.z)][static_cast<std::size_t>(s.x)] = s.tile;
    }
    return game::World::fromRows(rows);
}

// The camera the game would like: 7 tiles from `lookAt`, facing `yawDeg`
// (0 = camera looks south, so it stands north), looking down `pitchDeg`.
game::Vec3 orbitCamera(game::Vec3 lookAt, float yawDeg, float pitchDeg, float distance = 7.0f) {
    const float yaw = yawDeg * 3.14159265f / 180.0f, pitch = pitchDeg * 3.14159265f / 180.0f;
    const float behind = distance * std::cos(pitch), above = distance * std::sin(pitch);
    return {lookAt.x - std::sin(yaw) * behind, lookAt.y + above, lookAt.z - std::cos(yaw) * behind};
}

float distanceBetween(game::Vec3 a, game::Vec3 b) {
    const game::Vec3 d = a - b;
    return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

bool isFinite(game::Vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

// True if `p` lies on the segment from `a` to `b`.
bool onSegment(game::Vec3 p, game::Vec3 a, game::Vec3 b) {
    return near(distanceBetween(a, p) + distanceBetween(p, b), distanceBetween(a, b), 0.001f);
}

// Parses controls text the same way the game parses data/controls.txt.
platform::ControlsData parseControlsText(const std::string& text) {
    std::istringstream in(text);
    return platform::parseControls(in, "test");
}

// True if parsing `text` fails with an error message containing `expected`.
bool controlsTextFails(const std::string& text, const std::string& expected) {
    try {
        parseControlsText(text);
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()).find(expected) != std::string::npos) {
            return true;
        }
        std::cerr << "  (got: " << error.what() << ")\n";
    }
    return false;
}

// A complete [gamepad] block (7 lines) and [buttons] block (3 lines).
const std::string kGamepadBlock = "[gamepad]\nmove_stick = left\ncamera_stick = right\ndead_zone = 0.2\n"
                                  "response_curve = 1.5\ncamera_speed = 150\ninvert_y = no\n";
const std::string kButtonsBlock = "[buttons]\ninteract = a\npause = start\n";

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

    // --- NPC data file ------------------------------------------------------------

    run("NPC text parses every field, skipping comments and blank lines", [] {
        const auto npcs = parseNpcText(
            "# a comment\n"
            "\n"
            "[npc]\n"
            "name   = Mara\n"
            "tile   = 20, 13\n"
            "facing = 90\n"
            "color  = #C0504d\n"
            "line   = Fresh bread = warm bread!\n"
            "\n"
            "[npc]\n"
            "name = Edda\n"
            "tile = 3, 4\n"
            "color = #000000\n"
            "line = Halt.\n");
        check(npcs.size() == 2, "two NPCs");
        const game::Npc& mara = npcs[0];
        check(mara.name == "Mara" && mara.line == "Fresh bread = warm bread!",
              "name and line (an '=' inside the line is kept)");
        check(near(mara.position.x, 20.5f) && near(mara.position.z, 13.5f) && near(mara.position.y, 0),
              "stands in the centre of tile 20, 13 on the ground");
        check(near(mara.facingYaw, kPi / 2.0f), "facing 90 degrees is +X");
        check(mara.color == 0xC0504D, "color parsed from hex");
        check(near(npcs[1].facingYaw, 0.0f), "facing defaults to 0");
        check(near(mara.halfSize, game::data::kNpcHalfSize) && near(mara.height, game::data::kNpcHeight),
              "size comes from GameData");
    });

    run("NPC text errors name the problem and the line", [] {
        const std::string ok = "[npc]\nname = A\ntile = 1, 1\ncolor = #ffffff\nline = Hi.\n";
        check(npcTextFails("[npc]\nname = A\ntile = 1, 1\ncolor = #ffffff\n", "test:1: [npc] is missing 'line'"),
              "missing line reported at the [npc] line");
        check(npcTextFails("name = A\n", "test:1: 'name = A' must come after an [npc] line"),
              "key before any [npc]");
        check(npcTextFails(ok + "[npc]\nname = B\nname = C\n", "test:8: 'name' is set twice"),
              "repeated key");
        check(npcTextFails("[npc]\nmood = grumpy\n", "unknown key 'mood'"), "unknown key");
        check(npcTextFails("[npc]\ntile = 1 2\n", "tile must be two whole numbers"), "tile without comma");
        check(npcTextFails("[npc]\ntile = 1, 2, 3\n", "tile must be two whole numbers"), "tile with 3 numbers");
        check(npcTextFails("[npc]\ncolor = red\n", "color must look like #RRGGBB"), "named color");
        check(npcTextFails("[npc]\ncolor = #12345\n", "color must look like #RRGGBB"), "short hex color");
        check(npcTextFails("[npc]\nfacing = east\n", "facing must be a number"), "non-numeric facing");
        check(npcTextFails("[npc]\nname =\n", "'name' has no value"), "empty value");
        check(npcTextFails("[npc]\njust some words\n", "expected '[npc]' or 'key = value'"), "stray text");
    });

    run("The real data/npcs.txt loads and every NPC fits the starting world", [] {
        // `make test` runs from cpp/, so the data folder is found relative to it.
        const auto npcs = game::loadNpcFile("data/npcs.txt");
        check(npcs.size() >= 3 && npcs.size() <= 4, "3 to 4 NPCs");
        // The Game constructor throws if any NPC is misplaced.
        const game::Game g(game::buildStartingWorld(game::data::kWorldSeed), npcs);
        check(g.npcs().size() == npcs.size(), "all NPCs placed");
    });

    run("Missing NPC file is a clear error", [] {
        bool threw = false;
        try {
            game::loadNpcFile("data/no-such-file.txt");
        } catch (const std::runtime_error& error) {
            threw = std::string(error.what()).find("Could not open NPC file") != std::string::npos;
        }
        check(threw, "should say the file could not be opened");
    });

    // --- NPCs in the world --------------------------------------------------------

    run("NPCs in bad places are rejected", [] {
        auto rejected = [](game::Npc npc, std::vector<game::Npc> others = {}) {
            others.push_back(std::move(npc));
            try {
                game::Game g(smallRoom(), others);
            } catch (const std::runtime_error&) {
                return true;
            }
            return false;
        };
        check(rejected(npcAt("Wally", 0, 0)), "on a wall");
        check(rejected(npcAt("Lost", 20, 20)), "outside the world");
        check(rejected(npcAt("Lost", -1, 2)), "negative tile");
        check(rejected(npcAt("Squatter", 3, 2)), "on the player's start tile");
        check(rejected(npcAt("Twin", 1, 1), {npcAt("Twin", 1, 1)}), "two NPCs on one tile");
        check(!rejected(npcAt("Fine", 1, 1)), "a free floor tile is accepted");
    });

    run("NPCs block the tile they stand on", [] {
        game::Game g(smallRoom(), {npcAt("Post", 5, 2)});
        hold(g, {1, 0}, 120);
        const float rightEdge = g.player().position.x + g.player().halfSize;
        check(rightEdge < 5.0f && rightEdge > 4.99f, "player stops flush against the NPC's tile");
    });

    run("NPC lookup by distance finds the nearest one in range", [] {
        const game::Game g(smallRoom(), {npcAt("Near", 1, 1), npcAt("Far", 5, 3)});
        const game::Vec3 point{2.0f, 0.0f, 2.0f}; // 0.71 from Near, 3.8 from Far
        const game::Npc* found = g.nearestNpcWithin(point, 5.0f);
        check(found != nullptr && found->name == "Near", "the closer NPC wins");
        check(g.nearestNpcWithin(point, 0.5f) == nullptr, "nobody within half a tile");
        const game::Npc* far = g.nearestNpcWithin({5.5f, 0.0f, 1.5f}, 2.0f);
        check(far != nullptr && far->name == "Far", "from the other side, Far is nearest");
        const game::Npc* edge = g.nearestNpcWithin({1.5f, 0.0f, 3.0f}, 1.5f);
        check(edge != nullptr && edge->name == "Near", "an NPC exactly at the range counts");
    });

    run("The player's talk reach uses kTalkRange", [] {
        // Player starts at (3.5, 2.5); NPC centres at (5.5, 2.5) are 2.0 away.
        game::Game g(smallRoom(), {npcAt("Bob", 5, 2)});
        check(game::data::kTalkRange < 2.0f, "test assumes a talk range under 2 tiles");
        check(g.npcInReach() == nullptr, "2 tiles away is out of reach");
        hold(g, {1, 0}, 120); // walk up to Bob
        check(g.npcInReach() != nullptr && g.npcInReach()->name == "Bob", "next to Bob he is in reach");
    });

    // --- Talking (E) ----------------------------------------------------------------

    run("Talking near an NPC opens their line; talking again closes it", [] {
        game::Game g(smallRoom(), {npcAt("Bob", 5, 2)});
        hold(g, {1, 0}, 120); // walk up to Bob
        check(g.activeDialogue() == nullptr, "no dialogue before pressing E");
        g.interact();
        check(g.activeDialogue() != nullptr && g.activeDialogue()->name == "Bob", "Bob is talking");
        check(g.activeDialogue()->line == "Hi.", "his line comes from his data");
        g.interact();
        check(g.activeDialogue() == nullptr, "pressing E again closes it");
    });

    run("Talking with nobody in reach does nothing", [] {
        game::Game g(smallRoom(), {npcAt("Bob", 5, 2)}); // 2 tiles away at the start
        g.interact();
        check(g.activeDialogue() == nullptr, "no dialogue opens");
    });

    run("Walking out of reach closes the dialogue", [] {
        game::Game g(smallRoom(), {npcAt("Bob", 5, 2)});
        hold(g, {1, 0}, 120);
        g.interact();
        hold(g, {-1, 0}, 2); // a small step back is still in reach
        check(g.activeDialogue() != nullptr, "still talking after a small step");
        hold(g, {-1, 0}, 120);
        check(g.activeDialogue() == nullptr, "walking away ends the conversation");
    });

    run("With two NPCs in reach, talking picks the nearer one", [] {
        // Player starts at (3.5, 2.5). "Left" stands at (2.5, 2.5), 1.0 away;
        // "Up" stands at (4.5, 1.5), 1.41 away. Both are within kTalkRange.
        game::Game g(game::World::fromRows(std::vector<std::string_view>{
                         "#######",
                         "#.....#",
                         "#..@..#",
                         "#######",
                     }),
                     {npcAt("Left", 2, 2), npcAt("Up", 4, 1)});
        g.interact();
        check(g.activeDialogue() != nullptr && g.activeDialogue()->name == "Left",
              "Left (1.0 away) is nearer than Up (1.41 away)");
    });

    // --- Building data file (data/buildings.txt) -----------------------------
    // styleBlock() is lines 1-9 of a text, roofBlock() 6 lines more.

    run("Building text reads styles, roofs and buildings", [] {
        const platform::BuildingData data = parseBuildingText(
            "# a comment\n" + styleBlock("plaster") +                       // lines 1-10
            "[style]\nname = tall\nwall = w2.gltf\nwindow = win2.gltf\nwindow_frame = f2.gltf\n"
            "door = d2.gltf\ndoor_leaf = leaf2.gltf\ncorner = c2.gltf\nscale = 1.5\n" // lines 11-19
            "[roof]\nname = tiles\nmodel = roof.gltf\ngable = gable.gltf\nfits = 4, 6\n"  // lines 20-24
            "[building]\ntile = 29, 24\nstyle = plaster\nfacing = 180\n"
            "[building]\ntile = 3, 4\nstyle = tall\nfacing = 90\noffset = 0.5, -0.1, 0\nroof = tiles\n");
        check(data.source == "test", "source name kept for later errors");
        check(data.styles.size() == 2 && data.roofs.size() == 1 && data.buildings.size() == 2,
              "two styles, one roof, two buildings");

        const platform::BuildingStyle& plaster = data.styles[0];
        check(plaster.name == "plaster" && plaster.wall == "w.gltf" && plaster.window == "win.gltf" &&
                  plaster.windowFrame == "frame.gltf" && plaster.door == "d.gltf" &&
                  plaster.doorLeaf == "leaf.gltf" && plaster.corner == "c.gltf",
              "every piece file read");
        check(near(plaster.heightScale, 1.0f), "scale defaults to 1");
        check(plaster.line == 2, "style remembers its line");
        check(near(data.styles[1].heightScale, 1.5f), "scale read");

        const platform::BuildingRoof& roof = data.roofs[0];
        check(roof.name == "tiles" && roof.model == "roof.gltf" && roof.gable == "gable.gltf" &&
                  roof.fitsAcross == 4 && roof.fitsAlong == 6 && roof.line == 20,
              "roof read");

        const platform::BuildingEntry& first = data.buildings[0];
        check(first.tileX == 29 && first.tileZ == 24 && first.style == "plaster" &&
                  first.facingDegrees == 180 && first.roof.empty(),
              "first building read, no roof named");
        check(near(first.offsetX, 0) && near(first.offsetY, 0) && near(first.offsetZ, 0),
              "offset defaults to 0, 0, 0");
        const platform::BuildingEntry& second = data.buildings[1];
        check(second.facingDegrees == 90 && near(second.offsetX, 0.5f) && near(second.offsetY, -0.1f) &&
                  near(second.offsetZ, 0.0f) && second.roof == "tiles",
              "second building read with offset and roof");
    });

    run("Building styles and roofs are looked up by name", [] {
        const platform::BuildingData data = parseBuildingText(
            styleBlock("plaster") + styleBlock("brick") + roofBlock("small", 4, 6) + roofBlock("big", 6, 8));
        const platform::BuildingStyle* brick = data.findStyle("brick");
        check(brick != nullptr && brick->name == "brick", "brick found");
        check(data.findStyle("plaster") == &data.styles[0], "plaster found");
        check(data.findStyle("Brick") == nullptr, "names are case-sensitive");
        check(data.findStyle("stone") == nullptr, "unknown style gives nullptr");
        const platform::BuildingRoof* big = data.findRoof("big");
        check(big != nullptr && big->fitsAcross == 6 && big->fitsAlong == 8, "roof found");
        check(data.findRoof("plaster") == nullptr, "styles and roofs have separate names");
    });

    run("A building may use a style and roof defined later in the file", [] {
        const platform::BuildingData data = parseBuildingText(
            "[building]\ntile = 1, 1\nstyle = late\nfacing = 0\nroof = later\n" + styleBlock("late") +
            roofBlock("later", 4, 6));
        check(data.buildings.size() == 1 && data.findStyle(data.buildings[0].style) != nullptr &&
                  data.findRoof(data.buildings[0].roof) != nullptr,
              "forward references resolved");
    });

    run("The real data/buildings.txt is valid", [] {
        // make test runs from cpp/, like make run.
        const platform::BuildingData data = platform::loadBuildingFile("data/buildings.txt");
        check(!data.styles.empty() && !data.roofs.empty() && !data.buildings.empty(),
              "has styles, roofs and buildings");
        for (const platform::BuildingEntry& b : data.buildings) {
            check(data.findStyle(b.style) != nullptr, "style '" + b.style + "' exists");
        }
        // And every listed tile really is one of the town's buildings.
        const platform::BuildingMap map =
            platform::findBuildings(game::buildStartingWorld(game::data::kWorldSeed));
        check(platform::matchBuildings(map, data).size() == data.buildings.size(), "every entry matches");
    });

    run("Bad building text is reported with file and line", [] {
        const std::string style = styleBlock("plaster");  // lines 1-9
        const std::string roof = roofBlock("r", 6, 8);   // lines 10-15 after the style
        const auto building = [](const std::string& extra) {
            return "[building]\ntile = 1, 1\nstyle = plaster\nfacing = 0\n" + extra;
        };
        check(buildingTextFails("wall = w.gltf\n",
                                "test:1: 'wall = w.gltf' must come after a [style], [roof] or [building] line"),
              "key before any section");
        check(buildingTextFails("[house]\n", "test:1: unknown section '[house]'"), "unknown section");
        check(buildingTextFails("[style]\njust words\n", "test:2: expected a [section] or 'key = value'"),
              "stray text");
        check(buildingTextFails("[style]\ncolour = red\n", "test:2: unknown key 'colour' in [style]"),
              "unknown style key");
        check(buildingTextFails("[roof]\nwall = w.gltf\n", "test:2: unknown key 'wall' in [roof]"),
              "style key in a roof");
        check(buildingTextFails("[building]\nmodel = r.gltf\n", "test:2: unknown key 'model' in [building]"),
              "roof key in a building");
        check(buildingTextFails("[style]\nname = a\nname = b\n", "test:3: 'name' is set twice in the same [style]"),
              "repeated key");
        check(buildingTextFails("[style]\nwall =\n", "test:2: 'wall' has no value"), "empty value");
        check(buildingTextFails("\n\n[style]\nname = a\n", "test:3: [style] is missing 'wall'"),
              "missing style key reported at the [style] line");
        check(buildingTextFails("[roof]\nname = r\nmodel = m.gltf\nfits = 4, 6\n", "test:1: [roof] is missing 'gable'"),
              "missing roof key reported at the [roof] line");
        check(buildingTextFails(style + roof + "[building]\ntile = 1, 1\nstyle = plaster\n",
                                "test:16: [building] is missing 'facing'"),
              "missing building key reported at the [building] line");
        check(buildingTextFails(style + style, "test:10: style 'plaster' is defined twice"),
              "duplicate style name");
        check(buildingTextFails(roof + roof, "test:7: roof 'r' is defined twice"), "duplicate roof name");
        check(buildingTextFails(style + roof + "[building]\ntile = 1, 1\nstyle = brick\nfacing = 0\n",
                                "test:18: unknown style 'brick'"),
              "unknown style reported at its style line");
        check(buildingTextFails(style + roof + building("roof = slate\n"), "test:20: unknown roof 'slate'"),
              "unknown roof reported at its roof line");
        check(buildingTextFails(style + building(""), "test:10: there are buildings but no [roof] for them"),
              "buildings with no roofs at all");
        check(buildingTextFails(style + roof + building("") + building(""),
                                "test:20: tile 1, 1 is already used by the [building] on line 16"),
              "two buildings on the same tile");

        // Bad values.
        check(buildingTextFails("[building]\nfacing = 45\n", "test:2: facing must be 0, 90, 180 or 270"),
              "facing not a right angle");
        check(buildingTextFails("[building]\nfacing = north\n", "facing must be 0, 90, 180 or 270"),
              "facing as a word");
        check(buildingTextFails("[building]\ntile = 1 2\n", "test:2: tile must be two whole numbers"),
              "tile without comma");
        check(buildingTextFails("[building]\ntile = 1, 2, 3\n", "tile must be two whole numbers"),
              "tile with 3 numbers");
        check(buildingTextFails("[building]\ntile = 1.5, 2\n", "tile must be two whole numbers"),
              "tile with a fraction");
        check(buildingTextFails("[building]\noffset = 0, 1\n", "test:2: offset must be three numbers"),
              "offset with 2 numbers");
        check(buildingTextFails("[building]\noffset = 0, up, 0\n", "offset must be three numbers"),
              "offset with a word");
        check(buildingTextFails("[roof]\nfits = 6\n", "test:2: fits must be two whole numbers"),
              "fits with 1 number");
        check(buildingTextFails("[roof]\nfits = 0, 8\n", "fits must be two whole numbers of at least 1"),
              "fits of 0");
        check(buildingTextFails("[roof]\nfits = 4.5, 8\n", "fits must be two whole numbers of at least 1"),
              "fits with a fraction");
        check(buildingTextFails("[style]\nscale = 0\n", "test:2: scale must be more than 0"), "scale of 0");
        check(buildingTextFails("[style]\nscale = big\n", "scale must be a number"), "scale as a word");
    });

    run("A missing building file is a clear error", [] {
        bool threw = false;
        try {
            platform::loadBuildingFile("data/no-such-file.txt");
        } catch (const std::runtime_error& error) {
            threw = std::string(error.what()) == "Could not open building file 'data/no-such-file.txt'.";
        }
        check(threw, "names the missing file");
    });

    // --- Assembled building layout (platform/BuildingLayout.h) ----------------

    // Building A: x 2..5, z 2..3, with a path touching its south side at x 3.
    // Building B: an L shape at (7,3), (8,3), (7,4).
    const auto layoutTestWorld = [] {
        return game::World::fromRows(std::vector<std::string_view>{
            "##########",
            "#........#",
            "#.BBBB...#",
            "#.BBBB.BB#",
            "#..,...B.#",
            "#..@.....#",
            "##########",
        });
    };

    run("Buildings are found with their footprints", [&] {
        const platform::BuildingMap map = platform::findBuildings(layoutTestWorld());
        check(map.footprints.size() == 2, "two buildings");
        const platform::Footprint& a = map.footprints[0];
        check(a.x0 == 2 && a.z0 == 2 && a.width == 4 && a.depth == 2 && a.rectangular, "A is a 4x2 rectangle");
        check(!map.footprints[1].rectangular, "B (L shape) is not a rectangle");
        check(map.idAt(5, 3) == 0 && map.idAt(7, 4) == 1 && map.idAt(1, 1) == -1 && map.idAt(-1, 0) == -1,
              "tile lookup");
    });

    run("Building entries are matched to buildings, with file and line errors", [&] {
        const platform::BuildingMap map = platform::findBuildings(layoutTestWorld());
        // Lines 1-9 style, 10-15 roof "wide", 16-21 roof "narrow"; entries from line 22.
        const std::string header = styleBlock("s") + roofBlock("wide", 6, 8) + roofBlock("narrow", 2, 4);
        const auto entry = [](int x, int z, const std::string& extra = "") {
            return "[building]\ntile = " + std::to_string(x) + ", " + std::to_string(z) +
                   "\nstyle = s\nfacing = 0\n" + extra;
        };
        const std::vector<platform::PlacedBuilding> placed =
            platform::matchBuildings(map, parseBuildingText(header + entry(5, 3)));
        check(placed.size() == 1 && placed[0].building == 0 && placed[0].style.name == "s",
              "any tile of A picks A");
        check(placed[0].roof.name == "narrow", "A (4x2) gets the closest roof, narrow (2x4)");
        check(platform::matchBuildings(map, parseBuildingText(header + entry(5, 3, "roof = wide\n")))[0].roof.name ==
                  "wide",
              "a named roof wins over the closest one");

        const auto matchFails = [&](const std::string& text, const std::string& expected) {
            try {
                platform::matchBuildings(map, parseBuildingText(text));
            } catch (const std::runtime_error& error) {
                if (std::string(error.what()).find(expected) != std::string::npos) {
                    return true;
                }
                std::cerr << "  (got: " << error.what() << ")\n";
            }
            return false;
        };
        check(matchFails(header + entry(1, 1), "test:22: tile 1, 1 is not part of a building"),
              "tile that is not a building");
        check(matchFails(header + entry(99, 99), "test:22: tile 99, 99 is not part of a building"),
              "tile outside the world");
        check(matchFails(header + entry(2, 2) + entry(5, 3),
                         "test:26: tile 5, 3 is the same building as the [building] on line 22"),
              "two entries for one building");
        check(matchFails(header + entry(7, 4), "test:22: the building at tile 7, 4 is not a plain rectangle"),
              "L-shaped building");
    });

    run("Each town footprint gets the roof that needs the least stretching", [] {
        const std::vector<platform::BuildingRoof> roofs = {
            roofFits(4, 6, "4x6"), roofFits(4, 8, "4x8"), roofFits(6, 6, "6x6"), roofFits(6, 8, "6x8")};
        const auto pick = [&](int w, int d) { return platform::chooseRoof({0, 0, w, d, true}, roofs)->name; };
        check(pick(8, 6) == "6x8" && pick(6, 8) == "6x8", "8x6 either way round: 6x8 exactly");
        check(pick(6, 6) == "6x6", "6x6 exactly");
        check(pick(6, 4) == "4x6", "6x4: 4x6 exactly");
        check(pick(6, 5) == "6x6", "6x5: 6x6 (squash 5/6) beats 4x6 (stretch 5/4)");
        check(pick(7, 4) == "4x8", "7x4: 4x8 (squash 7/8) beats 4x6 (stretch 7/6)");
        check(pick(7, 5) == "6x8", "7x5: 6x8");
        check(pick(7, 6) == "6x8", "7x6: 6x8");
        check(platform::chooseRoof({0, 0, 6, 6, true}, {}) == nullptr, "no roofs: nullptr");
        const std::vector<platform::BuildingRoof> twins = {roofFits(6, 6, "first"), roofFits(6, 6, "second")};
        check(platform::chooseRoof({0, 0, 6, 6, true}, twins)->name == "first", "a tie goes to the first listed");
    });

    run("The door goes where a path meets the door side", [&] {
        const game::World world = layoutTestWorld();
        const platform::Footprint a = platform::findBuildings(world).footprints[0];
        check(platform::findDoorTile(world, a, 0) == 3, "south side: the path at x 3");
        check(platform::findDoorTile(world, a, 180) == -1, "north side: no path");
        check(platform::findDoorTile(world, a, 90) == -1, "east side: no path");
    });

    // The town's biggest building: 8 x 6 tiles at (29, 24), door to the north.
    const platform::Footprint big{29, 24, 8, 6, true};
    const platform::BuildingStyle anyStyle;
    const auto facingEntry = [](int facing) {
        platform::BuildingEntry e;
        e.facingDegrees = facing;
        return e;
    };
    const auto countOf = [](const std::vector<platform::PiecePlacement>& pieces, platform::Piece kind) {
        int n = 0;
        for (const platform::PiecePlacement& p : pieces) {
            n += (p.piece == kind) ? 1 : 0;
        }
        return n;
    };
    const auto allOf = [](const std::vector<platform::PiecePlacement>& pieces, platform::Piece kind) {
        std::vector<platform::PiecePlacement> found;
        for (const platform::PiecePlacement& p : pieces) {
            if (p.piece == kind) {
                found.push_back(p);
            }
        }
        return found;
    };
    const auto firstOf = [&](const std::vector<platform::PiecePlacement>& pieces, platform::Piece kind) {
        const auto found = allOf(pieces, kind);
        return found.empty() ? platform::PiecePlacement{} : found.front();
    };
    const auto isPanel = [](const platform::PiecePlacement& p) {
        return p.piece == platform::Piece::Wall || p.piece == platform::Piece::Window ||
               p.piece == platform::Piece::Door;
    };

    run("An 8x6 building gets 14 wall panels, windows, a door, corners, a roof and gables", [&] {
        const auto pieces = platform::planBuilding(big, anyStyle, roofFits(6, 8), facingEntry(180), 32);
        int panels = 0;
        for (const platform::PiecePlacement& p : pieces) {
            panels += isPanel(p) ? 1 : 0;
        }
        check(panels == 14, "4 + 4 panels on the 8-tile sides, 3 + 3 on the 6-tile sides");
        check(countOf(pieces, platform::Piece::Door) == 1 && countOf(pieces, platform::Piece::DoorLeaf) == 1,
              "one door and its leaf");
        check(countOf(pieces, platform::Piece::Window) == 8 && countOf(pieces, platform::Piece::WindowFrame) == 8,
              "panels 0 and 2 of each side are windows, each with a frame");
        check(countOf(pieces, platform::Piece::Corner) == 4 && countOf(pieces, platform::Piece::Roof) == 1 &&
                  countOf(pieces, platform::Piece::Gable) == 2,
              "four corners, one roof, two gables");

        // Every panel stands on the footprint's edge, facing out.
        bool onEdges = true;
        for (const platform::PiecePlacement& p : pieces) {
            if (isPanel(p)) {
                const bool northSouth = (near(p.z, 24) && near(p.yawDegrees, 180)) || (near(p.z, 30) && near(p.yawDegrees, 0));
                const bool eastWest = (near(p.x, 37) && near(p.yawDegrees, 90)) || (near(p.x, 29) && near(p.yawDegrees, 270));
                onEdges = onEdges && (northSouth || eastWest) && near(p.y, 0) && near(p.scaleX, 1.0f);
            }
        }
        check(onEdges, "panels on the edges, facing outward, 2 tiles each (no stretch)");
    });

    run("Window frames sit exactly in their window panels", [&] {
        const auto pieces = platform::planBuilding({0, 0, 7, 5, true}, anyStyle, roofFits(6, 8), facingEntry(0), -1);
        const auto windows = allOf(pieces, platform::Piece::Window);
        const auto frames = allOf(pieces, platform::Piece::WindowFrame);
        bool matched = !windows.empty() && windows.size() == frames.size();
        for (std::size_t i = 0; matched && i < windows.size(); ++i) {
            matched = near(windows[i].x, frames[i].x) && near(windows[i].z, frames[i].z) &&
                      near(windows[i].yawDegrees, frames[i].yawDegrees) && near(windows[i].scaleX, frames[i].scaleX);
        }
        check(matched, "same position, turn and stretch as the panel");
    });

    run("The door panel faces its path and the leaf sits inside the doorway", [&] {
        const auto pieces = platform::planBuilding(big, anyStyle, roofFits(6, 8), facingEntry(180), 32);
        const platform::PiecePlacement door = firstOf(pieces, platform::Piece::Door);
        check(near(door.x, 32) && near(door.z, 24) && near(door.yawDegrees, 180),
              "door on the north edge, over the path tile at x 32, facing north");
        const platform::PiecePlacement leaf = firstOf(pieces, platform::Piece::DoorLeaf);
        check(near(leaf.yawDegrees, 180) && std::abs(leaf.x - door.x) < 1.0f && leaf.z > 24.0f && leaf.z < 24.5f,
              "leaf turned with the door, just inside the outside face");
    });

    run("Each facing puts the door on that side", [&] {
        const auto doorFor = [&](int facing) {
            return firstOf(platform::planBuilding(big, anyStyle, roofFits(6, 8), facingEntry(facing), -1),
                           platform::Piece::Door);
        };
        const platform::PiecePlacement south = doorFor(0), east = doorFor(90), north = doorFor(180),
                                       west = doorFor(270);
        check(near(south.z, 30) && near(south.yawDegrees, 0), "0 = south edge");
        check(near(east.x, 37) && near(east.yawDegrees, 90), "90 = east edge");
        check(near(north.z, 24) && near(north.yawDegrees, 180), "180 = north edge");
        check(near(west.x, 29) && near(west.yawDegrees, 270), "270 = west edge");
        check(north.x > 29.0f && north.x < 37.0f && west.z > 24.0f && west.z < 30.0f,
              "with no path the door is still on the side");
    });

    run("Odd side lengths stretch the panels to fit exactly", [&] {
        // 7 wide: 4 panels of 1.75. 5 deep: 3 panels of 1.667.
        const auto pieces = platform::planBuilding({0, 0, 7, 5, true}, anyStyle, roofFits(6, 8), facingEntry(0), -1);
        bool stretched = true;
        for (const platform::PiecePlacement& p : pieces) {
            if (isPanel(p)) {
                const bool alongX = near(p.yawDegrees, 0) || near(p.yawDegrees, 180);
                stretched = stretched && near(p.scaleX, alongX ? 0.875f : 5.0f / 6.0f);
            }
        }
        check(stretched, "panel stretch = side length / panels / 2");
    });

    run("The roof's ridge runs along the longer side, stretched to the footprint", [&] {
        // 8x6 is longer in X: the roof (ridge along its own Z) is turned 90 degrees.
        const platform::PiecePlacement turned =
            firstOf(platform::planBuilding(big, anyStyle, roofFits(6, 8), facingEntry(0), -1), platform::Piece::Roof);
        check(near(turned.yawDegrees, 90) && near(turned.scaleX, 1) && near(turned.scaleZ, 1),
              "6x8 roof on an 8x6 building is turned, not stretched");
        check(near(turned.x, 33) && near(turned.z, 27) && near(turned.y, platform::kit::kWallTop),
              "centred on the footprint, on top of the walls");

        // 6x5 with a 6x6 roof: turned, squashed across the ridge (5 of 6).
        const platform::PiecePlacement small = firstOf(
            platform::planBuilding({0, 0, 6, 5, true}, anyStyle, roofFits(6, 6), facingEntry(0), -1),
            platform::Piece::Roof);
        check(near(small.yawDegrees, 90) && near(small.scaleX, 5.0f / 6.0f) && near(small.scaleZ, 1.0f),
              "turned; across = 5/6, along = 6/6");

        // 5x7 is longer in Z: not turned.
        const platform::PiecePlacement same = firstOf(
            platform::planBuilding({0, 0, 5, 7, true}, anyStyle, roofFits(6, 8), facingEntry(0), -1),
            platform::Piece::Roof);
        check(near(same.yawDegrees, 0) && near(same.scaleX, 5.0f / 6.0f) && near(same.scaleZ, 7.0f / 8.0f),
              "not turned; across = 5/6, along = 7/8");
    });

    run("Gables close both ends of the ridge", [&] {
        // Ridge along X (8x6): gables on the east and west walls.
        const auto across = allOf(platform::planBuilding(big, anyStyle, roofFits(6, 8), facingEntry(0), -1),
                                  platform::Piece::Gable);
        check(across.size() == 2 && near(across[0].x, 37) && near(across[0].yawDegrees, 90) &&
                  near(across[1].x, 29) && near(across[1].yawDegrees, 270),
              "east gable faces east, west gable faces west");
        check(near(across[0].z, 27) && near(across[0].y, platform::kit::kWallTop) && near(across[0].scaleX, 1.0f),
              "centred on the wall, on the wall top, 6 wide like the roof");

        // Ridge along Z (5x7): gables on the south and north walls, squashed like the roof.
        const auto along = allOf(
            platform::planBuilding({0, 0, 5, 7, true}, anyStyle, roofFits(6, 8), facingEntry(0), -1),
            platform::Piece::Gable);
        check(along.size() == 2 && near(along[0].z, 7) && near(along[0].yawDegrees, 0) && near(along[1].z, 0) &&
                  near(along[1].yawDegrees, 180) && near(along[0].x, 2.5f) && near(along[0].scaleX, 5.0f / 6.0f),
              "south and north gables, 5/6 wide");
    });

    run("Height scale and offset move every piece", [&] {
        platform::BuildingStyle style;
        style.heightScale = 1.5f;
        platform::BuildingEntry entry = facingEntry(0);
        entry.offsetX = 0.25f;
        entry.offsetY = -0.1f;
        const auto plain = platform::planBuilding(big, anyStyle, roofFits(6, 8), facingEntry(0), -1);
        const auto moved = platform::planBuilding(big, style, roofFits(6, 8), entry, -1);
        bool allMoved = plain.size() == moved.size();
        for (std::size_t i = 0; allMoved && i < plain.size(); ++i) {
            // Pieces that sit on the wall top (roof, gables) rise with the taller walls.
            const float expectedY = plain[i].y * 1.5f - 0.1f;
            allMoved = near(moved[i].x, plain[i].x + 0.25f) && near(moved[i].y, expectedY) &&
                       near(moved[i].z, plain[i].z) && near(moved[i].scaleY, 1.5f);
        }
        check(allMoved, "x and y offset added, heights scaled by 1.5, roof and gables raised to the taller walls");
    });

    // --- Tree data file (data/trees.txt) -------------------------------------

    run("Tree text reads variants and rules", [] {
        const platform::TreeData data = parseTreeText(
            "# trees\n"
            "[tree]\nname = oak\nmodel = CommonTree_1.gltf\nscale = 0.7\ntrunk = 0.1, -0.3\nweight = 3\n"
            "[tree]\nname = pine\nmodel = Pine_1.gltf\nscale = 0.65\ntrunk = 0, 0\n"
            "[rules]\nscale_variation = 0.2\nseed = 42\n");
        check(data.source == "test" && data.variants.size() == 2, "two variants");
        const platform::TreeVariant& oak = data.variants[0];
        check(oak.name == "oak" && oak.model == "CommonTree_1.gltf" && near(oak.scale, 0.7f) &&
                  near(oak.trunkX, 0.1f) && near(oak.trunkZ, -0.3f) && oak.weight == 3 && oak.line == 2,
              "every key read");
        check(data.variants[1].weight == 1, "weight defaults to 1");
        check(near(data.rules.scaleVariation, 0.2f) && data.rules.seed == 42, "rules read");
        check(data.findVariant("pine") == &data.variants[1] && data.findVariant("birch") == nullptr,
              "lookup by name");
    });

    run("Tree rules have defaults, and no trees is allowed", [] {
        const platform::TreeData data = parseTreeText("# nothing yet\n");
        check(data.variants.empty(), "no variants: every tree stays a box tree");
        check(near(data.rules.scaleVariation, 0.15f) && data.rules.seed == 0, "default rules");
    });

    run("The real data/trees.txt is valid", [] {
        const platform::TreeData data = platform::loadTreeFile("data/trees.txt");
        check(data.variants.size() >= 2, "several variants");
    });

    run("Bad tree text is reported with file and line", [] {
        const std::string oak = treeBlock("oak"); // lines 1-6
        check(treeTextFails("name = oak\n", "test:1: 'name = oak' must come after a [tree] or [rules] line"),
              "key before any section");
        check(treeTextFails("[bush]\n", "test:1: unknown section '[bush]'"), "unknown section");
        check(treeTextFails("[tree]\nleafy\n", "test:2: expected a [section] or 'key = value'"), "stray text");
        check(treeTextFails("[tree]\ncolour = green\n", "test:2: unknown key 'colour' in [tree]"),
              "unknown tree key");
        check(treeTextFails("[rules]\nscale = 1\n", "test:2: unknown key 'scale' in [rules]"),
              "tree key in the rules");
        check(treeTextFails("[tree]\nname = a\nname = b\n", "test:3: 'name' is set twice in the same [tree]"),
              "repeated key");
        check(treeTextFails("[tree]\nmodel =\n", "test:2: 'model' has no value"), "empty value");
        check(treeTextFails("\n[tree]\nname = a\nmodel = a.gltf\nscale = 1\n", "test:2: [tree] is missing 'trunk'"),
              "missing key reported at the [tree] line");
        check(treeTextFails(oak + oak, "test:7: tree 'oak' is defined twice"), "duplicate name");
        check(treeTextFails("[rules]\nseed = 1\n[rules]\n", "test:3: [rules] appears twice (first on line 1)"),
              "two [rules]");

        // Bad values.
        check(treeTextFails("[tree]\nscale = 0\n", "test:2: scale must be more than 0"), "scale of 0");
        check(treeTextFails("[tree]\nscale = big\n", "scale must be a number"), "scale as a word");
        check(treeTextFails("[tree]\ntrunk = 0\n", "test:2: trunk must be two numbers"), "trunk with 1 number");
        check(treeTextFails("[tree]\ntrunk = 0, 0, 0\n", "trunk must be two numbers"), "trunk with 3 numbers");
        check(treeTextFails("[tree]\nweight = 0\n", "test:2: weight must be a whole number from 1 to 1000"),
              "weight of 0");
        check(treeTextFails("[tree]\nweight = 1.5\n", "weight must be a whole number from 1 to 1000"),
              "weight with a fraction");
        check(treeTextFails("[rules]\nscale_variation = 0.6\n", "test:2: scale_variation must be a number from 0 to 0.5"),
              "variation too big");
        check(treeTextFails("[rules]\nscale_variation = -0.1\n", "scale_variation must be a number from 0 to 0.5"),
              "negative variation");
        check(treeTextFails("[rules]\nseed = -1\n", "test:2: seed must be a whole number from 0 to 1000000"),
              "negative seed");
        check(treeTextFails("[rules]\nseed = 2.5\n", "seed must be a whole number from 0 to 1000000"),
              "seed with a fraction");
    });

    run("A missing tree file is a clear error", [] {
        bool threw = false;
        try {
            platform::loadTreeFile("data/no-such-trees.txt");
        } catch (const std::runtime_error& error) {
            threw = std::string(error.what()) == "Could not open tree file 'data/no-such-trees.txt'.";
        }
        check(threw, "names the missing file");
    });

    // --- Tree choice per tile (platform/TreeLayout.h) -------------------------

    // oak (weight 3), pine and birch (weight 1 each), 20% size variation.
    const platform::TreeData treeData = parseTreeText(
        treeBlock("oak", 3) + treeBlock("pine") + treeBlock("birch") + "[rules]\nscale_variation = 0.2\nseed = 7\n");
    const auto samePlacement = [](const platform::TreePlacement& a, const platform::TreePlacement& b) {
        return a.variant == b.variant && a.x == b.x && a.z == b.z && a.yawDegrees == b.yawDegrees &&
               a.scale == b.scale;
    };

    run("The same tile always gets the same tree", [&] {
        bool same = true;
        for (int x = -5; x < 40; x += 3) {
            for (int z = -5; z < 40; z += 4) {
                // A freshly parsed copy of the data: nothing is remembered between calls.
                const platform::TreeData copy = parseTreeText(
                    treeBlock("oak", 3) + treeBlock("pine") + treeBlock("birch") +
                    "[rules]\nscale_variation = 0.2\nseed = 7\n");
                same = same && samePlacement(platform::placeTree(x, z, treeData), platform::placeTree(x, z, copy));
            }
        }
        check(same, "identical results for the same tile and data");
        check(platform::tileHash(12, 34, 7, 1) == platform::tileHash(12, 34, 7, 1), "hash is stable");
        check(platform::tileHash(12, 34, 7, 1) != platform::tileHash(34, 12, 7, 1), "x and z are not interchangeable");
    });

    run("Different tiles spread over variants, turns and sizes", [&] {
        int counts[3] = {0, 0, 0};
        int quarterTurns[4] = {0, 0, 0, 0};
        float smallest = 10.0f, largest = 0.0f;
        bool inRange = true;
        for (int x = 0; x < 40; ++x) {
            for (int z = 0; z < 40; ++z) { // 1600 tiles
                const platform::TreePlacement t = platform::placeTree(x, z, treeData);
                ++counts[t.variant];
                inRange = inRange && t.yawDegrees >= 0.0f && t.yawDegrees < 360.0f;
                ++quarterTurns[static_cast<int>(t.yawDegrees / 90.0f) % 4];
                smallest = std::min(smallest, t.scale);
                largest = std::max(largest, t.scale);
            }
        }
        check(counts[0] > 0 && counts[1] > 0 && counts[2] > 0, "every variant is used");
        // Expected 960 / 320 / 320 for weights 3 / 1 / 1.
        check(counts[0] > 840 && counts[0] < 1080 && counts[1] > 240 && counts[1] < 400 &&
                  counts[2] > 240 && counts[2] < 400,
              "picks follow the weights (oak about 3x as often)");
        check(quarterTurns[0] > 300 && quarterTurns[1] > 300 && quarterTurns[2] > 300 && quarterTurns[3] > 300,
              "turns spread around the full circle");
        check(inRange, "turn between 0 and 360 degrees");
        check(smallest >= 0.7f * 0.8f - 0.0001f && largest <= 0.7f * 1.2f + 0.0001f, "size within +-20% of 0.7");
        check(smallest < 0.7f * 0.85f && largest > 0.7f * 1.15f, "sizes use most of that range");
    });

    run("Neighbouring tiles do not repeat the same tree", [&] {
        // Along a row of 30 tiles, count tiles with the same variant AND
        // nearly the same turn as the tile before: should almost never happen.
        int repeats = 0;
        for (int x = 0; x < 30; ++x) {
            const platform::TreePlacement a = platform::placeTree(x, 5, treeData);
            const platform::TreePlacement b = platform::placeTree(x + 1, 5, treeData);
            repeats += (a.variant == b.variant && std::abs(a.yawDegrees - b.yawDegrees) < 10.0f) ? 1 : 0;
        }
        check(repeats <= 2, "neighbours look different");
    });

    run("The seed changes the picks", [&] {
        const platform::TreeData otherSeed = parseTreeText(
            treeBlock("oak", 3) + treeBlock("pine") + treeBlock("birch") + "[rules]\nscale_variation = 0.2\nseed = 8\n");
        int differences = 0;
        for (int x = 0; x < 20; ++x) {
            differences += samePlacement(platform::placeTree(x, 3, treeData), platform::placeTree(x, 3, otherSeed)) ? 0 : 1;
        }
        check(differences >= 18, "almost every tile changes");
    });

    run("The trunk lands on the tile centre whatever the turn and size", [] {
        const platform::TreeData offset = parseTreeText(
            "[tree]\nname = leaning\nmodel = m.gltf\nscale = 0.5\ntrunk = 0.4, -0.6\n");
        bool centred = true;
        for (int x = 0; x < 12; ++x) {
            const platform::TreePlacement t = platform::placeTree(x, 9, offset);
            // Where the trunk ends up: the origin plus the turned, scaled trunk position.
            const float yaw = t.yawDegrees * kPi / 180.0f;
            const float tx = 0.4f * t.scale, tz = -0.6f * t.scale;
            const float trunkX = t.x + tx * std::cos(yaw) + tz * std::sin(yaw);
            const float trunkZ = t.z - tx * std::sin(yaw) + tz * std::cos(yaw);
            centred = centred && near(trunkX, x + 0.5f, 0.001f) && near(trunkZ, 9.5f, 0.001f);
        }
        check(centred, "trunk at (x + 0.5, z + 0.5) for every tile");
        const platform::TreePlacement plain = platform::placeTree(3, 4, parseTreeText(treeBlock("oak")));
        check(near(plain.x, 3.5f) && near(plain.z, 4.5f), "trunk 0, 0: origin on the tile centre");
    });

    run("Every tree tile in the town gets a tree, and nothing else does", [] {
        const game::World world = game::buildStartingWorld(game::data::kWorldSeed);
        const platform::TreeData data = platform::loadTreeFile("data/trees.txt");
        const std::vector<platform::TreePlacement> trees = platform::placeTrees(world, data);
        int treeTiles = 0;
        for (int z = 0; z < world.depth(); ++z) {
            for (int x = 0; x < world.width(); ++x) {
                treeTiles += world.tileAt(x, z) == game::Tile::Tree ? 1 : 0;
            }
        }
        bool onTreeTiles = true;
        for (const platform::TreePlacement& t : trees) {
            onTreeTiles = onTreeTiles && world.tileAt(t.tileX, t.tileZ) == game::Tile::Tree;
        }
        check(treeTiles == 20 && static_cast<int>(trees.size()) == treeTiles, "20 tree tiles, 20 trees");
        check(onTreeTiles, "each tree on its own tree tile");
        check(platform::placeTrees(world, parseTreeText("")).empty(), "no variants: no model trees (box trees stay)");
    });

    // --- Camera occlusion (game/CameraOcclusion.h) -----------------------------
    // The player's head (look point) at (15.5, 1.3, 15.5); the camera wanted
    // south of it (yaw 180), 7 tiles away, 35 degrees down: about 5.7 tiles
    // south and 4 tiles up.

    const game::Vec3 head{15.5f, 1.3f, 15.5f};
    const game::Vec3 southCamera = orbitCamera(head, 180.0f, 35.0f);

    run("Camera: open ground leaves the camera where it is", [&] {
        const game::World world = fieldWith({});
        const game::Vec3 result = game::unblockedCameraPosition(world, head, southCamera);
        check(near(result.x, southCamera.x) && near(result.y, southCamera.y) && near(result.z, southCamera.z),
              "unchanged");
    });

    run("Camera: a building between player and camera pulls it in", [&] {
        // A row of building tiles 2.5 tiles south of the player.
        std::vector<Stamp> row;
        for (int x = 10; x <= 21; ++x) {
            row.push_back({x, 18, 'B'});
        }
        const game::World world = fieldWith(row);
        const game::Vec3 result = game::unblockedCameraPosition(world, head, southCamera);
        const float distance = distanceBetween(head, result);
        check(distance < 2.0f, "pulled in to before the building (and its eaves)");
        check(distance >= game::data::kCameraMinDistance - 0.001f, "not closer than the minimum");
        check(onSegment(result, head, southCamera), "stays on the line from the player");
        check(!game::cameraBlockedAt(world, result) && result.z < 18.0f, "in front of the building");
    });

    run("Camera: a line passing over a low wall is not blocked", [&] {
        // The stone wall is 2 m tall; the line is already about 3 m up there.
        std::vector<Stamp> row;
        for (int x = 10; x <= 21; ++x) {
            row.push_back({x, 18, '#'});
        }
        const game::Vec3 result = game::unblockedCameraPosition(fieldWith(row), head, southCamera);
        check(near(distanceBetween(head, result), 7.0f), "camera keeps its full distance");
    });

    run("Camera: a tree crown pulls it in, but not a tree the line passes above", [&] {
        // A tree just beside the line: its trunk tile is not on the line, but its crown is.
        const game::World beside = fieldWith({{16, 17, 'T'}});
        const game::Vec3 pulled = game::unblockedCameraPosition(beside, head, southCamera);
        check(distanceBetween(head, pulled) < 3.0f, "pulled in before the crown");
        check(!game::cameraBlockedAt(beside, pulled), "not inside the crown");

        // A tree under the far end of a steep line. Its crown counts over the
        // tiles z 7-9 up to 7.5 m; the line is above 8.6 m from z 10 on.
        const game::World under = fieldWith({{15, 8, 'T'}});
        const game::Vec3 high = {15.5f, 9.3f, 9.5f}; // 6 tiles north, 8 m higher than the head
        const game::Vec3 kept = game::unblockedCameraPosition(under, head, high);
        check(near(kept.x, high.x) && near(kept.y, high.y) && near(kept.z, high.z), "line above the tree: unchanged");
    });

    run("Camera: the minimum distance is kept when only the clearance is tight", [&] {
        // A building wall running alongside the line, 0.3 tiles to its side:
        // too close for the camera's 0.4 clearance anywhere along it.
        std::vector<Stamp> column;
        for (int z = 14; z <= 24; ++z) {
            column.push_back({16, z, 'B'});
        }
        const game::World world = fieldWith(column);
        const game::Vec3 beside{15.7f, 1.3f, 15.5f};
        const game::Vec3 result = game::unblockedCameraPosition(world, beside, orbitCamera(beside, 180.0f, 35.0f));
        check(near(distanceBetween(beside, result), game::data::kCameraMinDistance, 0.001f),
              "exactly the minimum distance");
        check(!game::cameraBlockedAt(world, result), "and not inside anything");
    });

    run("Camera: pressed against a wall, the result is finite and not inside", [&] {
        // Player touching a building on its east side; the camera wanted inside the building.
        std::vector<Stamp> column;
        for (int z = 10; z <= 20; ++z) {
            column.push_back({17, z, 'B'});
        }
        const game::World world = fieldWith(column);
        const game::Vec3 againstWall{16.65f, 1.3f, 15.5f};
        const game::Vec3 into = orbitCamera(againstWall, 270.0f, 20.0f); // camera east of the player
        const game::Vec3 result = game::unblockedCameraPosition(world, againstWall, into);
        check(isFinite(result) && !game::cameraBlockedAt(world, result), "finite, not inside the building");
        check(distanceBetween(againstWall, result) >= game::data::kCameraNearestDistance - 0.001f,
              "not on top of the look point (the view would collapse)");
        check(isFinite(game::unblockedCameraPosition(world, againstWall, againstWall)), "camera on the look point");
        game::CameraOcclusion smooth;
        bool allFinite = true;
        for (int i = 0; i < 20; ++i) {
            allFinite = allFinite && isFinite(smooth.update(world, againstWall, into, 1.0f / 60.0f));
        }
        allFinite = allFinite && isFinite(smooth.update(world, againstWall, into, 0.0f));
        const float nan = std::numeric_limits<float>::quiet_NaN();
        allFinite = allFinite && isFinite(game::unblockedCameraPosition(world, againstWall, {nan, 1.0f, 1.0f}));
        check(allFinite, "smoothed too, and with a zero frame time or a NaN input");
    });

    run("Camera: a look point inside a tree still gives a usable camera", [&] {
        // Only possible with --screenshot aimed at a tree tile, but the view
        // must not collapse.
        const game::World world = fieldWith({{15, 15, 'T'}, {3, 3, '@'}});
        const game::Vec3 result = game::unblockedCameraPosition(world, head, southCamera);
        check(isFinite(result) && near(distanceBetween(head, result), game::data::kCameraNearestDistance, 0.001f),
              "kept at the nearest distance");
    });

    run("Camera: in the real town it never ends inside anything", [] {
        const game::World world = game::buildStartingWorld(game::data::kWorldSeed);
        int inside = 0, checked = 0;
        for (int z = 1; z < world.depth() - 1; ++z) {
            for (int x = 1; x < world.width() - 1; ++x) {
                if (game::isSolid(world.tileAt(x, z))) {
                    continue; // the player can't stand here
                }
                const game::Vec3 lookAt{x + 0.5f, 1.3f, z + 0.5f};
                for (float yaw = 0.0f; yaw < 360.0f; yaw += 30.0f) {
                    for (const float pitch : {5.0f, 35.0f, 75.0f}) {
                        const game::Vec3 result =
                            game::unblockedCameraPosition(world, lookAt, orbitCamera(lookAt, yaw, pitch));
                        ++checked;
                        inside += (!isFinite(result) || game::cameraBlockedAt(world, result)) ? 1 : 0;
                    }
                }
            }
        }
        check(checked > 20000 && inside == 0, "every walkable tile, 12 turns, 3 pitches: never inside");
    });

    run("Camera: smoothing moves at most a small step per frame, both ways", [&] {
        std::vector<Stamp> row;
        for (int x = 10; x <= 21; ++x) {
            row.push_back({x, 18, 'B'});
        }
        const game::World open = fieldWith({}), blocked = fieldWith(row);
        const float dt = 1.0f / 60.0f;
        // Steady rate: (7 - 1) tiles in 0.10 s going in = 1.0 tile per frame;
        // in 0.15 s going out = 0.67 per frame.
        const float maxIn = 6.0f / game::data::kCameraPullInSeconds * dt + 0.001f;
        const float maxOut = 6.0f / game::data::kCameraReleaseSeconds * dt + 0.001f;

        game::CameraOcclusion camera;
        camera.update(open, head, southCamera, dt);
        check(near(camera.distance(), 7.0f), "starts at the full distance");

        // A building appears in the way (as when walking past a corner).
        const float allowed = distanceBetween(head, game::unblockedCameraPosition(blocked, head, southCamera));
        float previous = camera.distance(), biggestStep = 0.0f;
        int framesIn = 0;
        while (!near(camera.distance(), allowed, 0.001f) && framesIn < 100) {
            camera.update(blocked, head, southCamera, dt);
            biggestStep = std::max(biggestStep, std::abs(camera.distance() - previous));
            previous = camera.distance();
            ++framesIn;
        }
        check(biggestStep <= maxIn, "pull-in: at most 1 tile per frame at 60 FPS");
        check(framesIn >= 3 && framesIn <= 7, "pull-in takes a few frames (about 0.1 s), not one jump");

        // And it goes away again.
        biggestStep = 0.0f;
        int framesOut = 0;
        while (!near(camera.distance(), 7.0f, 0.001f) && framesOut < 100) {
            camera.update(open, head, southCamera, dt);
            biggestStep = std::max(biggestStep, std::abs(camera.distance() - previous));
            previous = camera.distance();
            ++framesOut;
        }
        check(biggestStep <= maxOut, "release: at most 0.67 tiles per frame at 60 FPS");
        check(framesOut >= 6 && framesOut <= 10, "release takes about 0.15 s");
    });

    // --- Gamepad input maths (platform/InputMath.h) ----------------------------

    const auto stickLength = [](platform::Stick s) { return std::sqrt(s.x * s.x + s.y * s.y); };

    run("Dead zone: inside and at the edge give 0, full gives 1, no jump", [&] {
        const float dz = 0.2f;
        check(stickLength(platform::applyRadialDeadZone({0.1f, 0.1f}, dz)) == 0.0f, "inside (0.14 from centre): 0");
        check(stickLength(platform::applyRadialDeadZone({0.2f, 0.0f}, dz)) == 0.0f, "exactly at the edge: 0");
        check(stickLength(platform::applyRadialDeadZone({0.0f, -0.2f}, dz)) == 0.0f, "at the edge, other axis: 0");
        check(near(platform::applyRadialDeadZone({1.0f, 0.0f}, dz).x, 1.0f), "full right: 1");
        check(near(platform::applyRadialDeadZone({0.0f, -1.0f}, dz).y, -1.0f), "full down: -1");
        check(near(platform::applyRadialDeadZone({0.6f, 0.0f}, dz).x, 0.5f), "halfway out of the dead zone: 0.5");

        // Sweep from the centre to full tilt: the output never jumps.
        float previous = 0.0f, biggestJump = 0.0f;
        for (int i = 0; i <= 1000; ++i) {
            const float in = static_cast<float>(i) / 1000.0f;
            const float out = stickLength(platform::applyRadialDeadZone({in * 0.6f, in * 0.8f}, dz));
            biggestJump = std::max(biggestJump, out - previous);
            check(out >= previous, "never goes down while tilting further");
            previous = out;
        }
        check(biggestJump < 0.002f, "steps of 0.001 in give at most ~0.00125 out: no jump at the edge");

        // Direction is kept.
        const platform::Stick d = platform::applyRadialDeadZone({0.3f, 0.4f}, dz); // length 0.5
        check(near(d.y / d.x, 0.4f / 0.3f), "direction kept");
        const float nan = std::numeric_limits<float>::quiet_NaN();
        check(stickLength(platform::applyRadialDeadZone({nan, 0.5f}, dz)) == 0.0f, "NaN counts as centred");
    });

    run("Diagonals are no faster than straight", [&] {
        // A real pad's corner can read (1, 1): length 1.41 before capping.
        const platform::Stick corner = platform::shapeStick({1.0f, 1.0f}, 0.2f, 1.5f);
        const platform::Stick straight = platform::shapeStick({1.0f, 0.0f}, 0.2f, 1.5f);
        check(near(stickLength(corner), 1.0f) && near(stickLength(straight), 1.0f), "both have length 1");
        check(near(corner.x, corner.y), "and the corner still points diagonally");
        const platform::MoveAxes move = platform::stickToMove(corner);
        check(near(std::sqrt(move.forward * move.forward + move.right * move.right), 1.0f), "as movement too");
    });

    run("Response curve is monotonic, 0 -> 0 and 1 -> 1, finer near the centre", [] {
        float previous = -1.0f;
        bool monotonic = true;
        for (int i = 0; i <= 100; ++i) {
            const float out = platform::responseCurve(static_cast<float>(i) / 100.0f, 1.5f);
            monotonic = monotonic && out >= previous;
            previous = out;
        }
        check(monotonic, "never goes down");
        check(platform::responseCurve(0.0f, 1.5f) == 0.0f && near(platform::responseCurve(1.0f, 1.5f), 1.0f), "ends");
        check(platform::responseCurve(0.5f, 1.5f) < 0.5f, "half tilt gives less than half: finer control");
        check(near(platform::responseCurve(0.5f, 1.0f), 0.5f), "exponent 1 is linear");
        check(near(platform::responseCurve(0.5f, 0.2f), 0.5f), "exponents below 1 count as 1");
    });

    run("Sticks map to movement and camera turns with the right signs", [] {
        const platform::MoveAxes up = platform::stickToMove({0.0f, 1.0f});
        check(near(up.forward, 1.0f) && near(up.right, 0.0f), "stick up walks forward (away from the camera)");
        check(near(platform::stickToMove({1.0f, 0.0f}).right, 1.0f), "stick right walks right");

        const float dt = 0.5f; // half a second at 90 degrees per second = 45 degrees
        const platform::CameraTurn right = platform::stickToCameraTurn({1.0f, 0.0f}, 90.0f, false, dt);
        check(near(right.yawDelta, -kPi / 4.0f) && right.pitchDelta == 0.0f, "right turns the view right (like the mouse)");
        const platform::CameraTurn upTurn = platform::stickToCameraTurn({0.0f, 1.0f}, 90.0f, false, dt);
        check(near(upTurn.pitchDelta, -kPi / 4.0f), "up tilts the view up");
        check(near(platform::stickToCameraTurn({0.0f, 1.0f}, 90.0f, true, dt).pitchDelta, kPi / 4.0f),
              "invert Y: up tilts the view down");
        const platform::CameraTurn none = platform::stickToCameraTurn({1.0f, 1.0f}, 90.0f, false, 0.0f);
        check(none.yawDelta == 0.0f && none.pitchDelta == 0.0f, "no time, no turn");
    });

    run("Keyboard and mouse behave exactly as before when the pad is idle", [] {
        // Every WASD combination at several camera angles: adding a centred
        // stick gives bit-for-bit the same world direction as before.
        const platform::Stick idle = platform::shapeStick({0.05f, -0.1f}, 0.2f, 1.5f); // drift inside the dead zone
        bool same = true;
        for (const float forward : {-1.0f, 0.0f, 1.0f}) {
            for (const float right : {-1.0f, 0.0f, 1.0f}) {
                for (const float yaw : {0.0f, 0.7f, kPi, -2.1f}) {
                    const game::PlayerInput before = platform::cameraRelativeMove(forward, right, yaw);
                    const platform::MoveAxes combined =
                        platform::combineMove({forward, right}, platform::stickToMove(idle));
                    const game::PlayerInput after = platform::cameraRelativeMove(combined.forward, combined.right, yaw);
                    same = same && before.moveX == after.moveX && before.moveZ == after.moveZ;
                }
            }
        }
        check(same, "movement identical");

        // Mouse and arrow turns pass through unchanged too.
        bool turnSame = true;
        for (const float yaw : {-0.3f, 0.0f, 0.0123f}) {
            for (const float pitch : {-0.05f, 0.0f, 0.2f}) {
                const platform::CameraTurn mouse{yaw, pitch};
                const platform::CameraTurn combined =
                    platform::combineTurn(mouse, platform::stickToCameraTurn(idle, 150.0f, false, 1.0f / 60.0f));
                turnSame = turnSame && combined.yawDelta == mouse.yawDelta && combined.pitchDelta == mouse.pitchDelta;
            }
        }
        check(turnSame, "camera turn identical");
    });

    run("Hints follow the device used last", [] {
        using platform::InputDevice;
        platform::LastUsedDevice device;
        check(device.current() == InputDevice::KeyboardMouse, "starts on keyboard/mouse");
        check(device.update(false, true, true) == InputDevice::Gamepad, "pad used -> gamepad");
        check(device.update(false, false, true) == InputDevice::Gamepad, "nothing used -> no change");
        check(device.update(true, true, true) == InputDevice::Gamepad, "both used -> no change");
        check(device.update(true, false, true) == InputDevice::KeyboardMouse, "keyboard used -> keyboard/mouse");
        check(device.update(true, true, true) == InputDevice::KeyboardMouse, "both used -> still no change");
        device.update(false, true, true);
        check(device.update(false, false, false) == InputDevice::KeyboardMouse, "pad unplugged -> keyboard/mouse");
        check(device.update(false, true, false) == InputDevice::KeyboardMouse, "no pad: pad activity ignored");
    });

    // --- Controls file (data/controls.txt) ------------------------------------

    run("Controls text reads the sticks, feel and buttons", [] {
        const platform::ControlsData c = parseControlsText(
            "# pad\n[gamepad]\nmove_stick = right\ncamera_stick = left\ndead_zone = 0.25\n"
            "response_curve = 2\ncamera_speed = 200\ninvert_y = yes\n"
            "[buttons]\ninteract = x\npause = back\nattack = rt\ndodge = none\n");
        check(c.moveStick == platform::PadStick::Right && c.cameraStick == platform::PadStick::Left, "sticks");
        check(near(c.deadZone, 0.25f) && near(c.responseCurve, 2.0f) && near(c.cameraSpeed, 200.0f) && c.invertY,
              "feel");
        check(c.interact == platform::PadButton::X && c.pause == platform::PadButton::Back &&
                  c.attack == platform::PadButton::RT && c.dodge == platform::PadButton::None &&
                  c.cast == platform::PadButton::None,
              "buttons; cast not listed defaults to none");
        check(std::string(platform::buttonLabel(platform::PadButton::A)) == "A" &&
                  std::string(platform::buttonLabel(platform::PadButton::Start)) == "Start",
              "on-screen labels");
    });

    run("The real data/controls.txt is valid: A talks, Start quits, no combat buttons yet", [] {
        const platform::ControlsData c = platform::loadControlsFile("data/controls.txt");
        check(c.interact == platform::PadButton::A && c.pause == platform::PadButton::Start, "A and Start");
        check(c.attack == platform::PadButton::None && c.dodge == platform::PadButton::None &&
                  c.cast == platform::PadButton::None,
              "attack, dodge and cast reserved but unassigned");
        check(c.moveStick == platform::PadStick::Left && near(c.deadZone, 0.2f), "left stick walks, dead zone 0.2");
    });

    run("Bad controls text is reported with file and line", [] {
        const std::string& pad = kGamepadBlock;      // lines 1-7
        const std::string& buttons = kButtonsBlock;  // 3 lines
        check(controlsTextFails("interact = a\n", "test:1: 'interact = a' must come after a [gamepad] or [buttons] line"),
              "key before any section");
        check(controlsTextFails("[keyboard]\n", "test:1: unknown section '[keyboard]'"), "unknown section");
        check(controlsTextFails("[gamepad]\nrumble\n", "test:2: expected a [section] or 'key = value'"), "stray text");
        check(controlsTextFails("[gamepad]\nrumble = yes\n", "test:2: unknown key 'rumble' in [gamepad]"),
              "unknown gamepad key");
        check(controlsTextFails("[buttons]\njump = a\n", "test:2: unknown key 'jump' in [buttons]"),
              "unknown action (no jumping)");
        check(controlsTextFails("[gamepad]\ndead_zone = 0.2\ndead_zone = 0.3\n", "test:3: 'dead_zone' is set twice"),
              "repeated key");
        check(controlsTextFails("[buttons]\npause =\n", "test:2: 'pause' has no value"), "empty value");
        check(controlsTextFails(pad + buttons + pad, "test:11: [gamepad] appears twice (first on line 1)"),
              "section twice");
        check(controlsTextFails("[gamepad]\nmove_stick = left\n" + buttons, "test:1: [gamepad] is missing 'camera_stick'"),
              "missing gamepad key, reported at the section line");
        check(controlsTextFails(pad + "[buttons]\npause = start\n", "test:8: [buttons] is missing 'interact'"),
              "missing interact");
        check(controlsTextFails(buttons, "test:1: the file has no [gamepad] section"), "no [gamepad]");
        check(controlsTextFails(pad, "test:1: the file has no [buttons] section"), "no [buttons]");

        // Bad values.
        check(controlsTextFails("[gamepad]\nmove_stick = middle\n", "test:2: move_stick must be left or right"),
              "unknown stick");
        check(controlsTextFails("[gamepad]\nmove_stick = left\ncamera_stick = left\ndead_zone = 0.2\n"
                                "response_curve = 1.5\ncamera_speed = 150\ninvert_y = no\n" + buttons,
                                "test:3: move_stick and camera_stick are both the left stick"),
              "one stick for both, reported at camera_stick");
        check(controlsTextFails("[gamepad]\ndead_zone = 0.95\n", "test:2: dead_zone must be a number from 0 to 0.9"),
              "dead zone too big");
        check(controlsTextFails("[gamepad]\ndead_zone = small\n", "dead_zone must be a number from 0 to 0.9"),
              "dead zone as a word");
        check(controlsTextFails("[gamepad]\nresponse_curve = 0.5\n", "test:2: response_curve must be a number from 1 to 3"),
              "curve too low");
        check(controlsTextFails("[gamepad]\ncamera_speed = 5\n", "test:2: camera_speed must be a number from 10 to 720"),
              "camera too slow");
        check(controlsTextFails("[gamepad]\ninvert_y = maybe\n", "test:2: invert_y must be yes or no"), "invert_y");
        check(controlsTextFails("[buttons]\ninteract = z\n", "test:2: unknown button 'z'"), "unknown button");
        check(controlsTextFails(pad + "[buttons]\ninteract = a\npause = a\n",
                                "test:10: button 'a' is already used for 'interact' on line 9"),
              "one button for two actions");
        check(!controlsTextFails(pad + buttons + "attack = none\ndodge = none\ncast = none\n", "x"),
              "several actions on none is fine");
    });

    run("A missing controls file is a clear error", [] {
        bool threw = false;
        try {
            platform::loadControlsFile("data/no-such-controls.txt");
        } catch (const std::runtime_error& error) {
            threw = std::string(error.what()) == "Could not open controls file 'data/no-such-controls.txt'.";
        }
        check(threw, "names the missing file");
    });

    if (failures > 0) {
        std::cerr << failures << " check(s) failed.\n";
        return 1;
    }
    std::cout << "All tests passed.\n";
    return 0;
}
