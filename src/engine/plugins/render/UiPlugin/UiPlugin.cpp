#include "UiPlugin.hpp"

bool mouseInRect(const IVec2 &mousePos, const Position &position, const Size &size) {
    const auto mousePosX = static_cast<float>(mousePos.x);
    const auto mousePosY = static_cast<float>(mousePos.y);

    return mousePosX >= position.x && mousePosX <= position.x + size.width && mousePosY >= position.y && mousePosY <= position.y + size.height;
}