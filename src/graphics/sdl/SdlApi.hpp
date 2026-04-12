#pragma once

#include <memory>
#include <variant>
#include <vector>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "IDisplayModule.hpp"

struct SdlTextureDeleter {
    void operator()(SDL_Texture *texture) const {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
        }
    }
};

struct SdlSurfaceDeleter {
    void operator()(SDL_Surface *surface) const {
        if (surface != nullptr) {
            SDL_FreeSurface(surface);
        }
    }
};

struct SdlFontDeleter {
    void operator()(TTF_Font *font) const {
        if (font != nullptr) {
            TTF_CloseFont(font);
        }
    }
};

using SdlTexturePtr = std::unique_ptr<SDL_Texture, SdlTextureDeleter>;
using SdlSurfacePtr = std::unique_ptr<SDL_Surface, SdlSurfaceDeleter>;
using SdlFontPtr = std::unique_ptr<TTF_Font, SdlFontDeleter>;
using SdlResource = std::variant<SdlTexturePtr, SdlFontPtr>;

class SdlGraphicsApi : public IDisplayModule {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    std::vector<SdlResource> resources;
    bool mouseReleasedThisFrame = false;
    Color clearColor{0, 0, 0, 255};
    USize logicalSize{1920, 1080};
    USize windowSize{1920, 1080};

public:
    ~SdlGraphicsApi() override;

    void init() override;

    void shutdown() override;

    bool isWindowOpen() override;

    void beginFrame() override;

    void endFrame() override;

    void setWindowSize(USize rect) override;

    USize getWindowSize() const override;

    bool isKeyPressed(KeyboardCode code) override;

    void drawSprite(GlobalPosition pos, const Sprite &spr) override;

    void drawText(GlobalPosition pos, ResourceIndex handle, const char *str, Color color, uint32_t fontSize) override;

    IVec2 getMousePosition() const override;

    bool isMouseButtonPressed(int button) const override;

    bool wasMouseButtonReleased(int button) const override;

    void drawRect(GlobalPosition pos, Size size, Color color) override;

    void drawRectOutline(GlobalPosition pos, Size size, Color fillColor, Color outlineColor,
                         float outlineThickness) override;

    void loadResources(const std::vector<Resource> &resources) override;

    void setClearColor(Color color) override;
};
