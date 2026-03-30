#include "NcursesApi.hpp"

#include <ncurses.h>

#include "NcursesApi.hpp"

void NcursesGraphicsApi::init() {
     initscr();
     noecho();
     cbreak();
     keypad(stdscr, TRUE);
     curs_set(0);
     nodelay(stdscr, TRUE);
     start_color();
     use_default_colors();

     start_color();

     int h, w;
     getmaxyx(stdscr, h, w);
     windowSize = {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};

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
    clear();
}

void NcursesGraphicsApi::endFrame() {
    refresh();
}

void NcursesGraphicsApi::setWindowSize(USize size) {
    windowSize = size;
}

USize NcursesGraphicsApi::getWindowSize() const {
    return windowSize;
}

bool NcursesGraphicsApi::isKeyPressed(KeyboardCode code) {
    int ch = getch();
    return ch == static_cast<int>(code);
}

IVec2 NcursesGraphicsApi::getMousePosition() const {
    MEVENT event;
    if (getmouse(&event) == OK) {
        return {event.x, event.y};
    }
    return {0, 0};
}

bool NcursesGraphicsApi::isMouseButtonPressed(int button) const {
    (void)button;
    MEVENT event;
    if (getmouse(&event) == OK) {
        return event.bstate & BUTTON1_PRESSED;
    }
    return false;
}

bool NcursesGraphicsApi::wasMouseButtonReleased(int button) const {
    (void)button;
    MEVENT event;
    if (getmouse(&event) == OK) {
        return event.bstate & BUTTON1_RELEASED;
    }
    return false;
}

void NcursesGraphicsApi::drawRect(GlobalPosition pos, Size size, Color color) {
    short pair = getColorPair(color, color);
    attron(COLOR_PAIR(pair));

    for (int y = 0; y < size.height; ++y) {
        for (int x = 0; x < size.width; ++x) {
            mvaddch(pos.y + y, pos.x + x, ' ');
        }
    }

    attroff(COLOR_PAIR(pair));
}

void NcursesGraphicsApi::drawRectOutline(GlobalPosition pos, Size size,
                    Color fillColor, Color outlineColor,
                    float thickness) {

    (void)thickness;
    drawRect(pos, size, fillColor);

    short pair = getColorPair(outlineColor, fillColor);
    attron(COLOR_PAIR(pair));

    for (int x = 0; x < size.width; ++x) {
        mvaddch(pos.y, pos.x + x, '#');
        mvaddch(pos.y + size.height - 1, pos.x + x, '#');
    }

    for (int y = 0; y < size.height; ++y) {
        mvaddch(pos.y + y, pos.x, '#');
        mvaddch(pos.y + y, pos.x + size.width - 1, '#');
    }
    attroff(COLOR_PAIR(pair));
}

void NcursesGraphicsApi::drawSprite(GlobalPosition pos, const Sprite& spr) {
    (void)pos;
    (void)spr;
}

void NcursesGraphicsApi::drawText(GlobalPosition pos,
              ResourceIndex,
              const char* str,
              Color color,
              uint32_t) {

    short pair = getColorPair(color, Color{0,0,0,255});
    attron(COLOR_PAIR(pair));

    mvprintw(pos.y, pos.x, "%s", str);

    attroff(COLOR_PAIR(pair));
}

void NcursesGraphicsApi::setClearColor(Color) {
}

void NcursesGraphicsApi::loadResources(const std::vector<Resource>&) {
}
