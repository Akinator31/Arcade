#include "NcursesApi.hpp"

#include <ncurses.h>
#include <cmath>

float NcursesGraphicsApi::scaleX(float x) const {
    if (referenceSize.width == 0) return x;
    return x * static_cast<float>(termSize.width) / static_cast<float>(referenceSize.width);
}

float NcursesGraphicsApi::scaleY(float y) const {
    if (referenceSize.height == 0) return y;
    return y * static_cast<float>(termSize.height) / static_cast<float>(referenceSize.height);
}

float NcursesGraphicsApi::invScaleX(float x) const {
    if (termSize.width == 0) return x;
    return x * static_cast<float>(referenceSize.width) / static_cast<float>(termSize.width);
}

float NcursesGraphicsApi::invScaleY(float y) const {
    if (termSize.height == 0) return y;
    return y * static_cast<float>(referenceSize.height) / static_cast<float>(termSize.height);
}

KeyboardCode NcursesGraphicsApi::ncursesToKeyboardCode(int ch) {
    if (ch >= 'a' && ch <= 'z')
        return static_cast<KeyboardCode>(ch - 'a');
    if (ch >= 'A' && ch <= 'Z')
        return static_cast<KeyboardCode>(ch - 'A');
    switch (ch) {
        case KEY_UP: return ArrowUp;
        case KEY_DOWN: return ArrowDown;
        case KEY_RIGHT: return ArrowRight;
        case KEY_LEFT: return ArrowLeft;
        case ' ': return Space;
        case 27: return Escape;
        case '\n':
        case KEY_ENTER: return Enter;
        case KEY_BACKSPACE:
        case 127:
        case 8: return Backspace;
        case KEY_F(11): return F11;
        default: return None;
    }
}

short NcursesGraphicsApi::toNcursesColor(const Color& c) {
    if (c.r > 200 && c.g < 100 && c.b < 100) return COLOR_RED;
    if (c.r < 100 && c.g > 200 && c.b < 100) return COLOR_GREEN;
    if (c.r < 100 && c.g < 100 && c.b > 200) return COLOR_BLUE;
    if (c.r > 200 && c.g > 200 && c.b < 100) return COLOR_YELLOW;
    if (c.r > 200 && c.g > 200 && c.b > 200) return COLOR_WHITE;
    if (c.r < 50 && c.g < 50 && c.b < 50) return COLOR_BLACK;
    if (c.r > 150 && c.g > 100 && c.b < 150) return COLOR_YELLOW;
    if (c.r > 150 && c.g < 100 && c.b > 150) return COLOR_MAGENTA;
    if (c.r < 100 && c.g > 150 && c.b > 150) return COLOR_CYAN;
    return COLOR_WHITE;
}

short NcursesGraphicsApi::getColorPair(const Color& fg, const Color& bg) {
    short fgColor = toNcursesColor(fg);
    short bgColor = toNcursesColor(bg);
    int key = (fgColor << 8) | bgColor;

    auto it = colorPairCache.find(key);
    if (it != colorPairCache.end())
        return it->second;

    init_pair(nextPairId, fgColor, bgColor);
    colorPairCache[key] = nextPairId;
    return nextPairId++;
}

void NcursesGraphicsApi::init() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE);
    start_color();
    use_default_colors();
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);

    int h, w;
    getmaxyx(stdscr, h, w);
    termSize = {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    referenceSize = termSize;

    windowOpen = true;
}

void NcursesGraphicsApi::shutdown() {
    endwin();
    windowOpen = false;
}

bool NcursesGraphicsApi::isWindowOpen() {
    return windowOpen;
}

void NcursesGraphicsApi::beginFrame() {
    pressedKeys.clear();
    mouseReleasedThisFrame = false;

    int ch;
    while ((ch = getch()) != ERR) {
        if (ch == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                mousePos = {event.x, event.y};
                if (event.bstate & BUTTON1_PRESSED)
                    mousePressed = true;
                if (event.bstate & BUTTON1_RELEASED) {
                    mousePressed = false;
                    mouseReleasedThisFrame = true;
                }
            }
            continue;
        }
        if (ch == KEY_RESIZE) {
            int h, w;
            getmaxyx(stdscr, h, w);
            termSize = {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
            continue;
        }
        KeyboardCode code = ncursesToKeyboardCode(ch);
        if (code != None)
            pressedKeys.insert(static_cast<int>(code));
        if (code == Escape)
            windowOpen = false;
    }

    short bgPair = getColorPair(clearColor, clearColor);
    bkgd(COLOR_PAIR(bgPair));
    erase();
}

void NcursesGraphicsApi::endFrame() {
    refresh();
}

void NcursesGraphicsApi::setWindowSize(USize size) {
    referenceSize = size;
}

USize NcursesGraphicsApi::getWindowSize() const {
    return referenceSize;
}

bool NcursesGraphicsApi::isKeyPressed(KeyboardCode code) {
    return pressedKeys.count(static_cast<int>(code)) > 0;
}

IVec2 NcursesGraphicsApi::getMousePosition() const {
    return {
        static_cast<int>(invScaleX(static_cast<float>(mousePos.x))),
        static_cast<int>(invScaleY(static_cast<float>(mousePos.y)))
    };
}

bool NcursesGraphicsApi::isMouseButtonPressed(int button) const {
    if (button == 0)
        return mousePressed;
    return false;
}

bool NcursesGraphicsApi::wasMouseButtonReleased(int button) const {
    if (button == 0)
        return mouseReleasedThisFrame;
    return false;
}

void NcursesGraphicsApi::drawRect(GlobalPosition pos, Size size, Color color) {
    short pair = getColorPair(color, color);
    attron(COLOR_PAIR(pair));

    int px = static_cast<int>(scaleX(pos.x));
    int py = static_cast<int>(scaleY(pos.y));
    int w = std::max(1, static_cast<int>(scaleX(pos.x + size.width)) - px);
    int h = std::max(1, static_cast<int>(scaleY(pos.y + size.height)) - py);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            mvaddch(py + y, px + x, ' ');
        }
    }

    attroff(COLOR_PAIR(pair));
}

void NcursesGraphicsApi::drawRectOutline(GlobalPosition pos, Size size,
                    Color fillColor, Color outlineColor,
                    float thickness) {
    (void)thickness;
    drawRect(pos, size, fillColor);

    int px = static_cast<int>(scaleX(pos.x));
    int py = static_cast<int>(scaleY(pos.y));
    int w = std::max(1, static_cast<int>(scaleX(pos.x + size.width)) - px);
    int h = std::max(1, static_cast<int>(scaleY(pos.y + size.height)) - py);

    short pair = getColorPair(outlineColor, fillColor);
    attron(COLOR_PAIR(pair));

    for (int x = 0; x < w; ++x) {
        mvaddch(py, px + x, '#');
        mvaddch(py + h - 1, px + x, '#');
    }

    for (int y = 0; y < h; ++y) {
        mvaddch(py + y, px, '#');
        mvaddch(py + y, px + w - 1, '#');
    }
    attroff(COLOR_PAIR(pair));
}

void NcursesGraphicsApi::drawSprite(GlobalPosition pos, const Sprite& spr) {
    float pw = static_cast<float>(spr.rect.width) * std::abs(spr.scale.x);
    float ph = static_cast<float>(spr.rect.height) * std::abs(spr.scale.y);

    int px = static_cast<int>(scaleX(pos.x));
    int py = static_cast<int>(scaleY(pos.y));
    int w = std::max(1, static_cast<int>(scaleX(pos.x + pw)) - px);
    int h = std::max(1, static_cast<int>(scaleY(pos.y + ph)) - py);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            mvaddch(py + y, px + x, '#');
        }
    }
}

void NcursesGraphicsApi::drawText(GlobalPosition pos,
              ResourceIndex,
              const char* str,
              Color color,
              uint32_t) {
    short pair = getColorPair(color, Color{0, 0, 0, 255});
    attron(COLOR_PAIR(pair));
    mvprintw(static_cast<int>(scaleY(pos.y)), static_cast<int>(scaleX(pos.x)), "%s", str);
    attroff(COLOR_PAIR(pair));
}

void NcursesGraphicsApi::setClearColor(Color color) {
    clearColor = color;
}

void NcursesGraphicsApi::loadResources(const std::vector<Resource>&) {
}

extern "C" {
IDisplayModule* create() {
    return new NcursesGraphicsApi();
}

void destroy(const IDisplayModule* api) {
    delete api;
}
}
