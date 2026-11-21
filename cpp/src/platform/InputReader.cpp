// InputReader.cpp

#include "InputReader.h"

#include "raylib.h"

namespace platform {

game::PlayerInput readPlayerInput() {
    const bool left = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
    const bool right = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
    const bool up = IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
    const bool down = IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);

    // bool converts to 0 or 1, so right - left gives -1, 0 or +1.
    // Top-down view: screen right is +X and screen down is +Z.
    game::PlayerInput input;
    input.moveX = static_cast<float>(static_cast<int>(right) - static_cast<int>(left));
    input.moveZ = static_cast<float>(static_cast<int>(down) - static_cast<int>(up));
    return input;
}

} // namespace platform
