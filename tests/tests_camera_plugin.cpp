#include <criterion/criterion.h>
#include <vector>

#include "engine/plugins/render/CameraPlugin/CameraPlugin.hpp"
#include "engine/plugins/render/RenderPlugin/RenderPlugin.hpp"

namespace {
    struct FakeDisplayModule : IDisplayModule {
        std::vector<GlobalPosition> rect_positions;
        std::vector<GlobalPosition> sprite_positions;

        void init() override {
        }

        void shutdown() override {
        }

        bool isWindowOpen() override {
            return true;
        }

        void beginFrame() override {
        }

        void endFrame() override {
        }

        void setWindowSize(const USize) override {
        }

        [[nodiscard]] USize getWindowSize() const override {
            return {800, 600};
        }

        bool isKeyPressed(const KeyboardCode) override {
            return false;
        }

        void drawSprite(const GlobalPosition pos, const Sprite &) override {
            sprite_positions.push_back(pos);
        }

        void drawText(const GlobalPosition, const ResourceIndex, const char *, const Color) override {
        }

        [[nodiscard]] IVec2 getMousePosition() const override {
            return {0, 0};
        }

        [[nodiscard]] bool isMouseButtonPressed(const int) const override {
            return false;
        }

        [[nodiscard]] bool wasMouseButtonReleased(const int) const override {
            return false;
        }

        void drawRect(const GlobalPosition pos, const Size, const Color) override {
            rect_positions.push_back(pos);
        }

        void drawRectOutline(const GlobalPosition, const Size, const Color, const Color, const float) override {
        }

        void loadResources(const std::vector<Resource> &) override {
        }
    };
}

Test(camera_plugin, creates_default_main_camera) {
    ecs::World world;

    world.plugin<CameraPlugin>();

    auto main_cameras = world.fetch<MainCamera>();
    int count = 0;
    ecs::Entity main_camera{};
    main_cameras.iter([&](ArchetypeView &view) {
        for (uint32_t i = 0; i < view.count(); i += 1) {
            main_camera = view.entity(i);
            count += 1;
        }
    });

    cr_assert_eq(count, 1);
    cr_assert(world.has<Camera>(main_camera));
    cr_assert(world.has<GlobalPosition>(main_camera));
}

Test(render_plugin, uses_main_camera_for_entities_without_target) {
    ecs::World world;
    FakeDisplayModule display;
    world.api = &display;

    world.plugin<RenderPlugin>();

    ecs::Entity main_camera{};
    world.fetch<MainCamera>().iter([&](ArchetypeView &view) {
        if (view.count() > 0) {
            main_camera = view.entity(0);
        }
    });

    world.set<Position>(main_camera, {100.f, 40.f});
    const ecs::Entity entity = world.create().set(
        Position{130.f, 55.f},
        Size{10.f, 20.f},
        Color::WHITE()
    ).entity();

    world.progress();

    cr_assert_eq(display.rect_positions.size(), 1);
    cr_assert_float_eq(display.rect_positions[0].x, 30.f, 0.001f);
    cr_assert_float_eq(display.rect_positions[0].y, 15.f, 0.001f);
    cr_assert(world.isAlive(entity));
}

Test(render_plugin, uses_camera_target_when_present) {
    ecs::World world;
    FakeDisplayModule display;
    world.api = &display;

    world.plugin<RenderPlugin>();

    const ecs::Entity ui_camera = world.create().add<Camera>().set(Position{10.f, 5.f}, Name{"UICamera"}).entity();
    world.create().set(
        Position{32.f, 18.f},
        Size{8.f, 8.f},
        Color::BLUE(),
        CameraTarget{ui_camera}
    );

    world.progress();

    cr_assert_eq(display.rect_positions.size(), 1);
    cr_assert_float_eq(display.rect_positions[0].x, 22.f, 0.001f);
    cr_assert_float_eq(display.rect_positions[0].y, 13.f, 0.001f);
}
