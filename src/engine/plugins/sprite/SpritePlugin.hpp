#pragma once
#include <cstdint>

#include "engine/ecs/World.hpp"
#include "engine/ecs/addons/Timer.hpp"

struct SpriteAnimation {
    Timer timer;
    uint16_t start;
    uint16_t end;

    fields(
        field(uint16_t, "interval")
        field(uint16_t, "tile_height")
        field(uint16_t, "rows")
        field(uint16_t, "cols")
        field(uint16_t, "index")
    )
};

struct SpriteAtlas {
    uint16_t tile_width{};
    uint16_t tile_height{};
    uint16_t rows{};
    uint16_t cols{};
    uint16_t index = 0;

    fields(
        field(uint16_t, "tile_width")
        field(uint16_t, "tile_height")
        field(uint16_t, "rows")
        field(uint16_t, "cols")
        field(uint16_t, "index")
    )
};

namespace sprite_plugin_impl {
    inline uint16_t atlasFrameCount(const SpriteAtlas &atlas) {
        return static_cast<uint16_t>(atlas.rows * atlas.cols);
    }

    inline uint16_t clampFrame(const uint16_t frame, const SpriteAtlas &atlas) {
        const uint16_t frameCount = atlasFrameCount(atlas);
        if (frameCount == 0) {
            return 0;
        }
        return static_cast<uint16_t>(frame >= frameCount ? frameCount - 1 : frame);
    }

    inline void applyFrame(Sprite &sprite, const SpriteAtlas &atlas, const uint16_t frame) {
        if (atlas.tile_width == 0 || atlas.tile_height == 0 || atlas.cols == 0) {
            return;
        }

        const uint16_t col = frame % atlas.cols;
        const uint16_t row = frame / atlas.cols;

        sprite.rect.left = col * atlas.tile_width;
        sprite.rect.top = row * atlas.tile_height;
        sprite.rect.width = atlas.tile_width;
        sprite.rect.height = atlas.tile_height;
    }
}

SYSTEM(SpriteAnimationSys, With<Sprite, SpriteAnimation, SpriteAtlas>, On<Update>) {
    ITER(view) {
        auto *sprites = view.column<Sprite>();
        auto *animations = view.column<SpriteAnimation>();
        auto *atlases = view.column<SpriteAtlas>();

        for (uint32_t i = 0; i < view.count(); i += 1) {
            auto &sprite = sprites[i];
            auto &[timer, start, end] = animations[i];
            auto &atlas = atlases[i];

            if (const uint16_t frameCount = sprite_plugin_impl::atlasFrameCount(atlas);
                frameCount == 0 || atlas.tile_width == 0 || atlas.tile_height == 0 || atlas.cols == 0) {
                continue;
            }

            const uint16_t first = start <= end ? start : end;
            const uint16_t last = start <= end ? end : start;
            if (atlas.index < first || atlas.index > last) {
                atlas.index = first;
            } else if (timer.tick(view.world.deltaTime)) {
                atlas.index = atlas.index >= last ? first : static_cast<uint16_t>(atlas.index + 1);
            }

            sprite_plugin_impl::applyFrame(sprite, atlas, sprite_plugin_impl::clampFrame(atlas.index, atlas));
        }
    }
};

struct SpritePlugin {
    void load(ecs::World &world) {
        world.registerComponent<Sprite>();
        world.registerComponent<SpriteAnimation>();
        world.registerComponent<SpriteAtlas>();
        world.system<SpriteAnimationSys>();
    }

    void unload(ecs::World &world) {
        world.remove<SpriteAnimationSys>();
    }
};
