// main.cpp
//
// Entry point: opens the window and runs the main loop.
//
// Each frame does these things in order:
//   1. Read the gamepad, and turn the camera (mouse, arrows and right stick;
//      platform/InputReader + Gamepad -> platform/CameraRig)
//   2. Read movement (WASD and left stick, relative to the camera)
//   3. Update game   (game/Game, in fixed time steps)
//   4. Aim camera at the player and draw (platform/CameraRig, Renderer)
// Keyboard, mouse and gamepad all work at the same time; their inputs are
// added together (platform/InputMath), so the game never knows which
// device was used.
//
// Optional flags (screenshot and benchmark ignore the mouse and keyboard
// camera controls and any real gamepad, so their results are repeatable):
//   --screenshot build/shot.png  runs a short moment, saves one frame to
//                                that file and quits; for checking the view.
//   --screenshot build/shot.png x z yaw pitch
//                                the same, with the camera aimed at ground
//                                point (x, z) from the given angles (degrees).
//   --benchmark                  draws as fast as possible (no 60 FPS cap)
//                                for a few seconds, prints the average frame
//                                rate and quits; for measuring render cost.
//   --fake-pad lx,ly,rx,ry       (hidden, for testing without a controller)
//                                pretends a gamepad is connected and holds
//                                its left and right sticks at these values
//                                (x right, y up, -1..1) for the whole run.
//                                Works alone or with --screenshot; prints
//                                where the player and camera ended up.

#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <optional>
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
#include "platform/CameraRelativeMove.h"
#include "platform/CameraRig.h"
#include "platform/ControlsData.h"
#include "platform/Gamepad.h"
#include "platform/InputMath.h"
#include "platform/InputReader.h"
#include "platform/RenderConfig.h"
#include "platform/Renderer.h"
#include "platform/TreeData.h"
#include "platform/TreeLayout.h"

// Finds a file or folder under cpp/ (e.g. "data/npcs.txt"). Works when the
// game is started from cpp/ (as `make run` does) and when the exe is started
// from anywhere else, by also looking next to the exe's folder (cpp/build/..).
std::string findCppPath(const std::string& relative) {
    if (FileExists(relative.c_str()) || DirectoryExists(relative.c_str())) {
        return relative;
    }
    return std::string(GetApplicationDirectory()) + "../" + relative;
}

// The command line flags (see the header comment).
struct Options {
    const char* screenshotPath = nullptr;
    bool customView = false;     // --screenshot file x z yaw pitch
    float view[4] = {};          // x, z, yaw, pitch
    bool benchmark = false;
    std::optional<platform::FakePad> fakePad;
};

// True if `text` is a number like "12.5" or "-3".
bool isNumber(const char* text) {
    char* end = nullptr;
    std::strtof(text, &end);
    return end != text && *end == '\0';
}

// Reads the flags; prints what is wrong and exits on a bad one.
Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string flag = argv[i];
        if (flag == "--benchmark") {
            options.benchmark = true;
        } else if (flag == "--screenshot" && i + 1 < argc) {
            options.screenshotPath = argv[++i];
            // Optional: x z yaw pitch. Aims the camera at ground point (x, z)
            // instead of the player, facing `yaw` degrees (0 = south, 180 =
            // north) and looking down `pitch` degrees.
            if (i + 4 < argc && isNumber(argv[i + 1]) && isNumber(argv[i + 2]) && isNumber(argv[i + 3]) &&
                isNumber(argv[i + 4])) {
                options.customView = true;
                for (float& v : options.view) {
                    v = std::strtof(argv[++i], nullptr);
                }
            }
        } else if (flag == "--fake-pad" && i + 1 < argc) {
            platform::FakePad pad;
            char extra = 0;
            if (std::sscanf(argv[++i], "%f,%f,%f,%f%c", &pad.move.x, &pad.move.y, &pad.camera.x, &pad.camera.y,
                            &extra) != 4) {
                std::cerr << "--fake-pad needs four numbers: lx,ly,rx,ry (e.g. 0,1,0.5,0)\n";
                std::exit(1);
            }
            options.fakePad = pad;
        } else {
            std::cerr << "Unknown option '" << flag << "'. Options: --screenshot <file> [x z yaw pitch], "
                      << "--benchmark\n";
            std::exit(1);
        }
    }
    return options;
}

int main(int argc, char** argv) {
    const Options options = parseOptions(argc, argv);
    const char* screenshotPath = options.screenshotPath;
    const bool customView = options.customView;
    const game::Vec3 viewTarget{options.view[0], 0.0f, options.view[1]};
    const bool benchmark = options.benchmark;

    // Build the world and load the data files before opening a window, so a
    // broken map or data file reports a clear error instead of flashing an
    // empty window.
    platform::BuildingMap buildings;
    std::vector<platform::PlacedBuilding> placedBuildings;
    platform::TreeData treeData;
    std::vector<platform::TreePlacement> trees;
    platform::ControlsData controls;
    game::Game game = [&] {
        try {
            // Terrain from the seed, then the hand-made town (WorldBuilder.h).
            game::World world = game::buildStartingWorld(game::data::kWorldSeed);
            std::vector<game::Npc> npcs = game::loadNpcFile(findCppPath("data/npcs.txt"));
            // Which buildings are drawn from kit models (look only).
            buildings = platform::findBuildings(world);
            placedBuildings = platform::matchBuildings(
                buildings, platform::loadBuildingFile(findCppPath("data/buildings.txt")));
            // Which tree model, turn and size each tree tile gets (look only).
            treeData = platform::loadTreeFile(findCppPath("data/trees.txt"));
            trees = platform::placeTrees(world, treeData);
            // The gamepad's sticks and buttons.
            controls = platform::loadControlsFile(findCppPath("data/controls.txt"));
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
        cameraRig.orbit(options.view[2] * DEG2RAD - cameraRig.yaw(), options.view[3] * DEG2RAD - cameraRig.pitch());
    }
    // The renderer owns GPU data (meshes, textures, a shader), which must be
    // freed while the window is still open, so it is held in an optional
    // and destroyed explicitly before CloseWindow() below.
    std::optional<const platform::Renderer> renderer;
    renderer.emplace(game.world(), buildings, placedBuildings, findCppPath("assets/medieval") + "/", treeData,
                     trees, findCppPath("assets/nature") + "/");

    // Fixed time step: real elapsed time is collected in `accumulator`, and
    // the game advances in equal slices of kFixedTimeStep. Rendering can run
    // at any rate without changing how fast or how far the player moves.
    const float dt = game::data::kFixedTimeStep;
    float accumulator = 0.0f;

    // Cap on how much time one frame may simulate. After a long stall (e.g.
    // dragging the window) we skip the lost time instead of running hundreds
    // of catch-up steps at once.
    constexpr float kMaxFrameTime = 0.25f;

    // The gamepad: real ones only when playing (screenshots and benchmarks
    // stay repeatable); the scripted test pad whenever it is asked for.
    platform::GamepadReader gamepad(controls, interactive, options.fakePad);
    platform::LastUsedDevice lastDevice;
    platform::HudInfo hud;
    hud.talkButton = platform::buttonLabel(controls.interact);
    hud.quitButton = platform::buttonLabel(controls.pause);
    hud.moveStick = controls.moveStick == platform::PadStick::Left ? "left stick" : "right stick";
    hud.cameraStick = controls.cameraStick == platform::PadStick::Left ? "left stick" : "right stick";

    int frame = 0;
    double benchmarkStart = 0.0;
    int framesRun = 0; // for the --fake-pad report
    bool quit = false; // the gamepad's quit button (Start), like Esc
    while (!quit && !WindowShouldClose()) { // Esc or the window's close button
        float frameTime = GetFrameTime();
        if (frameTime > kMaxFrameTime) {
            frameTime = kMaxFrameTime;
        }
        accumulator += frameTime;

        // 1. Read the gamepad (if any) and note which device was used last,
        //    for the on-screen hints.
        const platform::GamepadFrame pad = gamepad.read();
        lastDevice.update(interactive && platform::keyboardOrMouseUsed(mouseCaptured), pad.used, pad.connected);
        hud.gamepadHints = (lastDevice.current() == platform::InputDevice::Gamepad);
        hud.gamepadName = pad.connected ? pad.name : "";

        //    Turn the camera first, so movement below uses this frame's view.
        //    The camera is presentation, so it turns once per frame using real
        //    frame time rather than in fixed game steps.
        platform::CameraTurn mouseAndKeys;
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
            mouseAndKeys = {look.yawDelta, look.pitchDelta};
        }
        if (interactive || pad.connected) {
            const platform::CameraTurn turn = platform::combineTurn(
                mouseAndKeys, platform::stickToCameraTurn(pad.camera, controls.cameraSpeed, controls.invertY, frameTime));
            cameraRig.orbit(turn.yawDelta, turn.pitchDelta);
        }

        // 2. WASD and the move stick together, relative to the camera, turned
        //    into a world direction here in the platform layer. Sampled once
        //    per frame and reused for every game step in it.
        const platform::MoveAxes move =
            platform::combineMove(platform::readKeyboardMove(), platform::stickToMove(pad.move));
        const game::PlayerInput input = platform::cameraRelativeMove(move.forward, move.right, cameraRig.yaw());

        //    E or the gamepad's talk button (A) is a one-off press, so it goes
        //    to the game once per frame here, not once per fixed step (which
        //    could apply it twice).
        if ((interactive && platform::interactPressed()) || pad.interactPressed) {
            game.interact();
        }
        if (pad.pausePressed) {
            quit = true; // no pause menu yet: quit, like Esc
        }

        // 3. Advance the game in fixed steps.
        while (accumulator >= dt) {
            game.step(input, dt);
            accumulator -= dt;
        }

        // 4. Aim the camera at the player's new position, then draw.
        cameraRig.follow(customView ? viewTarget : game.player().position, game.world(), frameTime);
        renderer->draw(game, cameraRig, hud);
        ++framesRun;

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
                          << renderer->buildingDrawCount() << " building and " << renderer->treeDrawCount()
                          << " tree mesh draw(s) per frame\n";
                break;
            }
        }
    }

    if (options.fakePad) {
        // For checking the gamepad paths without a controller.
        const game::Vec3 p = game.player().position;
        std::cout << "Fake pad: after " << framesRun << " frame(s) the player is at (" << p.x << ", " << p.z
                  << "), camera yaw " << cameraRig.yaw() * RAD2DEG << " deg, pitch " << cameraRig.pitch() * RAD2DEG
                  << " deg.\n";
    }

    renderer.reset(); // free GPU data first, while the window is open
    CloseWindow();
    return 0;
}
