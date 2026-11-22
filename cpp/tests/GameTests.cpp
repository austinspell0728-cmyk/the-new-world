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

    if (failures > 0) {
        std::cerr << failures << " check(s) failed.\n";
        return 1;
    }
    std::cout << "All tests passed.\n";
    return 0;
}
