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
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../src/game/Game.h"
#include "../src/game/GameData.h"
#include "../src/game/NpcData.h"
#include "../src/game/World.h"
#include "../src/game/WorldBuilder.h"
// Raylib-free platform helpers; safe to include in the test build.
#include "../src/platform/BuildingData.h"
#include "../src/platform/BuildingLayout.h"
#include "../src/platform/CameraRelativeMove.h"
#include "../src/platform/TreeData.h"

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

    if (failures > 0) {
        std::cerr << failures << " check(s) failed.\n";
        return 1;
    }
    std::cout << "All tests passed.\n";
    return 0;
}
