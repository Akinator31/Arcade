#include "SdlApi.hpp"

#include <cmath>
#include <stdexcept>

namespace {
    SDL_Color toSdlColor(const Color color) {
        return {color.r, color.g, color.b, color.a};
    }

    SDL_FRect toRect(const GlobalPosition pos, const Size size) {
        return {pos.x, pos.y, size.width, size.height};
    }

    float clampf(const float value, const float minValue, const float maxValue) {
        return value < minValue ? minValue : (value > maxValue ? maxValue : value);
    }

    SDL_Scancode toScancode(const KeyboardCode code) {
        switch (code) {
            case A: return SDL_SCANCODE_A;
            case B: return SDL_SCANCODE_B;
            case C: return SDL_SCANCODE_C;
            case D: return SDL_SCANCODE_D;
            case E: return SDL_SCANCODE_E;
            case F: return SDL_SCANCODE_F;
            case G: return SDL_SCANCODE_G;
            case H: return SDL_SCANCODE_H;
            case I: return SDL_SCANCODE_I;
            case J: return SDL_SCANCODE_J;
            case K: return SDL_SCANCODE_K;
            case L: return SDL_SCANCODE_L;
            case M: return SDL_SCANCODE_M;
            case N: return SDL_SCANCODE_N;
            case O: return SDL_SCANCODE_O;
            case P: return SDL_SCANCODE_P;
            case Q: return SDL_SCANCODE_Q;
            case R: return SDL_SCANCODE_R;
            case S: return SDL_SCANCODE_S;
            case T: return SDL_SCANCODE_T;
            case U: return SDL_SCANCODE_U;
            case V: return SDL_SCANCODE_V;
            case W: return SDL_SCANCODE_W;
            case X: return SDL_SCANCODE_X;
            case Y: return SDL_SCANCODE_Y;
            case Z: return SDL_SCANCODE_Z;
            case ArrowUp: return SDL_SCANCODE_UP;
            case ArrowDown: return SDL_SCANCODE_DOWN;
            case ArrowRight: return SDL_SCANCODE_RIGHT;
            case ArrowLeft: return SDL_SCANCODE_LEFT;
            case Space: return SDL_SCANCODE_SPACE;
            case CtrlLeft: return SDL_SCANCODE_LCTRL;
            case AltLeft: return SDL_SCANCODE_LALT;
            case Backspace: return SDL_SCANCODE_BACKSPACE;
            case Shift: return SDL_SCANCODE_LSHIFT;
            case F11: return SDL_SCANCODE_F11;
            case Escape: return SDL_SCANCODE_ESCAPE;
            case Enter: return SDL_SCANCODE_RETURN;
            default: return SDL_SCANCODE_UNKNOWN;
        }
    }
}

SdlGraphicsApi::~SdlGraphicsApi() {
    this->SdlGraphicsApi::shutdown();
}

void SdlGraphicsApi::init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        throw std::runtime_error(IMG_GetError());
    }
    if (TTF_Init() != 0) {
        throw std::runtime_error(TTF_GetError());
    }

    this->window = SDL_CreateWindow(
        "game",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        static_cast<int>(this->windowSize.width),
        static_cast<int>(this->windowSize.height),
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (this->window == nullptr) {
        throw std::runtime_error(SDL_GetError());
    }

    this->renderer = SDL_CreateRenderer(
        this->window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (this->renderer == nullptr) {
        throw std::runtime_error(SDL_GetError());
    }

    SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(
        this->renderer,
        static_cast<int>(this->logicalSize.width),
        static_cast<int>(this->logicalSize.height)
    );
}

void SdlGraphicsApi::shutdown() {
    this->resources.clear();

    if (this->renderer != nullptr) {
        SDL_DestroyRenderer(this->renderer);
        this->renderer = nullptr;
    }
    if (this->window != nullptr) {
        SDL_DestroyWindow(this->window);
        this->window = nullptr;
    }

    if (TTF_WasInit() != 0) {
        TTF_Quit();
    }
    IMG_Quit();
    if (SDL_WasInit(SDL_INIT_VIDEO) != 0) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        SDL_Quit();
    }
}

bool SdlGraphicsApi::isWindowOpen() {
    return this->window != nullptr;
}

void SdlGraphicsApi::beginFrame() {
    this->mouseReleasedThisFrame = false;

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            this->shutdown();
            return;
        }
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
            this->mouseReleasedThisFrame = true;
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            this->windowSize = {
                static_cast<uint32_t>(event.window.data1),
                static_cast<uint32_t>(event.window.data2)
            };
        }
    }

    SDL_SetRenderDrawColor(this->renderer, this->clearColor.r, this->clearColor.g, this->clearColor.b,
                           this->clearColor.a);
    SDL_RenderClear(this->renderer);
}

void SdlGraphicsApi::endFrame() {
    if (this->renderer != nullptr) {
        SDL_RenderPresent(this->renderer);
    }
}

void SdlGraphicsApi::setWindowSize(const USize rect) {
    this->logicalSize = rect;
    if (this->window != nullptr) {
        SDL_SetWindowSize(this->window, static_cast<int>(rect.width), static_cast<int>(rect.height));
        this->windowSize = rect;
        SDL_RenderSetLogicalSize(this->renderer, static_cast<int>(rect.width), static_cast<int>(rect.height));
    }
}

USize SdlGraphicsApi::getWindowSize() const {
    return this->logicalSize;
}

bool SdlGraphicsApi::isKeyPressed(const KeyboardCode code) {
    const Uint8 *keyboardState = SDL_GetKeyboardState(nullptr);
    return keyboardState[toScancode(code)] != 0;
}

void SdlGraphicsApi::drawSprite(const GlobalPosition pos, const Sprite &spr) {
    if (spr.texture >= this->resources.size()) {
        return;
    }

    auto *texture = std::get_if<SdlTexturePtr>(&this->resources[spr.texture]);
    if (texture == nullptr || *texture == nullptr) {
        return;
    }

    SDL_Rect src{
        static_cast<int>(spr.rect.left),
        static_cast<int>(spr.rect.top),
        static_cast<int>(spr.rect.width),
        static_cast<int>(spr.rect.height)
    };
    SDL_FRect dst{
        pos.x,
        pos.y,
        static_cast<float>(spr.rect.width) * std::abs(spr.scale.x),
        static_cast<float>(spr.rect.height) * std::abs(spr.scale.y)
    };

    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if (spr.scale.x < 0.f) {
        flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_HORIZONTAL);
    }
    if (spr.scale.y < 0.f) {
        flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_VERTICAL);
    }

    SDL_RenderCopyExF(this->renderer, texture->get(), &src, &dst, 0.0, nullptr, flip);
}

void SdlGraphicsApi::drawText(const GlobalPosition pos,
                              const ResourceIndex handle,
                              const char *str,
                              const Color color,
                              const uint32_t fontSize) {
    if (handle >= this->resources.size() || str == nullptr) {
        return;
    }

    auto *fontResource = std::get_if<SdlFontPtr>(&this->resources[handle]);
    if (fontResource == nullptr || *fontResource == nullptr) {
        return;
    }

    TTF_Font *font = fontResource->get();
    if (TTF_SetFontSize(font, static_cast<int>(fontSize)) != 0) {
        return;
    }

    SdlSurfacePtr surface(TTF_RenderUTF8_Blended(font, str, toSdlColor(color)));
    if (surface == nullptr) {
        return;
    }

    SdlTexturePtr texture(SDL_CreateTextureFromSurface(this->renderer, surface.get()));
    if (texture == nullptr) {
        return;
    }

    SDL_FRect dst{
        pos.x,
        pos.y,
        static_cast<float>(surface->w),
        static_cast<float>(surface->h)
    };
    SDL_RenderCopyF(this->renderer, texture.get(), nullptr, &dst);
}

IVec2 SdlGraphicsApi::getMousePosition() const {
    int x = 0;
    int y = 0;
    SDL_GetMouseState(&x, &y);

    if (this->windowSize.width == 0 || this->windowSize.height == 0 ||
        this->logicalSize.width == 0 || this->logicalSize.height == 0) {
        return {x, y};
    }

    const float scale = std::min(
        static_cast<float>(this->windowSize.width) / static_cast<float>(this->logicalSize.width),
        static_cast<float>(this->windowSize.height) / static_cast<float>(this->logicalSize.height)
    );
    if (scale <= 0.f) {
        return {x, y};
    }

    const float viewportWidth = static_cast<float>(this->logicalSize.width) * scale;
    const float viewportHeight = static_cast<float>(this->logicalSize.height) * scale;
    const float offsetX = (static_cast<float>(this->windowSize.width) - viewportWidth) * 0.5f;
    const float offsetY = (static_cast<float>(this->windowSize.height) - viewportHeight) * 0.5f;

    const float logicalX = clampf((static_cast<float>(x) - offsetX) / scale, 0.f,
                                  static_cast<float>(this->logicalSize.width));
    const float logicalY = clampf((static_cast<float>(y) - offsetY) / scale, 0.f,
                                  static_cast<float>(this->logicalSize.height));
    return {static_cast<int>(logicalX), static_cast<int>(logicalY)};
}

bool SdlGraphicsApi::isMouseButtonPressed(const int button) const {
    const Uint32 state = SDL_GetMouseState(nullptr, nullptr);
    if (button == 0) {
        return (state & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    }
    if (button == 1) {
        return (state & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    }
    return false;
}

bool SdlGraphicsApi::wasMouseButtonReleased(const int button) const {
    return button == 0 && this->mouseReleasedThisFrame;
}

void SdlGraphicsApi::drawRect(const GlobalPosition pos, const Size size, const Color color) {
    SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
    const SDL_FRect rect = toRect(pos, size);
    SDL_RenderFillRectF(this->renderer, &rect);
}

void SdlGraphicsApi::drawRectOutline(const GlobalPosition pos,
                                     const Size size,
                                     const Color fillColor,
                                     const Color outlineColor,
                                     const float outlineThickness) {
    this->drawRect(pos, size, fillColor);
    SDL_SetRenderDrawColor(this->renderer, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);

    for (int i = 0; i < static_cast<int>(std::ceil(outlineThickness)); ++i) {
        const SDL_FRect rect{
            pos.x + static_cast<float>(i),
            pos.y + static_cast<float>(i),
            size.width - static_cast<float>(i * 2),
            size.height - static_cast<float>(i * 2)
        };
        SDL_RenderDrawRectF(this->renderer, &rect);
    }
}

void SdlGraphicsApi::loadResources(const std::vector<Resource> &newResources) {
    this->resources.clear();
    this->resources.reserve(newResources.size());

    for (const auto &[path, type]: newResources) {
        if (type == ResourceType::Texture) {
            SdlSurfacePtr surface(IMG_Load(path.c_str()));
            if (surface == nullptr) {
                throw std::runtime_error(IMG_GetError());
            }

            SdlTexturePtr texture(SDL_CreateTextureFromSurface(this->renderer, surface.get()));
            if (texture == nullptr) {
                throw std::runtime_error(SDL_GetError());
            }
            this->resources.emplace_back(std::move(texture));
            continue;
        }

        if (type == ResourceType::Font) {
            SdlFontPtr font(TTF_OpenFont(path.c_str(), 16));
            if (font == nullptr) {
                throw std::runtime_error(TTF_GetError());
            }
            this->resources.emplace_back(std::move(font));
        }
    }
}

void SdlGraphicsApi::setClearColor(const Color color) {
    this->clearColor = color;
}

extern "C" IDisplayModule *load() {
    return new SdlGraphicsApi();
}

extern "C" void unload(IDisplayModule *api) {
    delete api;
}

extern "C" {
LibType LIB_TYPE = GRAPHIC;
}
