#include <criterion/criterion.h>
#include <unordered_set>

#include "engine/plugins/control/ControlPlugin.hpp"
#include "engine/plugins/tilemap/TileMapPlugin.hpp"

namespace {
    struct FakeDisplayModule : IDisplayModule {
        std::unordered_set<KeyboardCode> pressed;

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

        bool isKeyPressed(const KeyboardCode code) override {
            return pressed.contains(code);
        }

        void drawSprite(const GlobalPosition, const Sprite &) override {
        }

        void drawText(const GlobalPosition, const ResourceIndex, const char *, const Color, const uint32_t) override {
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

        void drawRect(const GlobalPosition, const Size, const Color) override {
        }

        void drawRectOutline(const GlobalPosition, const Size, const Color, const Color, const float) override {
        }

        void loadResources(const std::vector<Resource> &) override {
        }

        void setClearColor(const Color) override {
        }
    };
}

Test(control, character_controller_updates_velocity_from_keys) {
    ecs::World world;
    FakeDisplayModule display;
    world.api = &display;
    world.plugin<ControlPlugin>();

    const ecs::Entity entity = world.create().set(
        CharacterController{
            .left = Q,
            .right = D,
            .up = Z,
            .down = S,
            .speed = 180.f
        }
    ).entity();

    display.pressed = {D, Z};
    world.progress();

    cr_assert_float_eq(world.get<Velocity>(entity)->x, 180.f, 0.001f);
    cr_assert_float_eq(world.get<Velocity>(entity)->y, -180.f, 0.001f);

    display.pressed = {Q, D};
    world.progress();

    cr_assert_float_eq(world.get<Velocity>(entity)->x, 0.f, 0.001f);
    cr_assert_float_eq(world.get<Velocity>(entity)->y, 0.f, 0.001f);
}

Test(tilemap, straight_wall_becomes_one_entity) {
    ecs::World world;
    world.plugin<TileMapPlugin>();

    const ecs::EntityRef prefab = world.create().add<IsGround>().set(
        RigidBody::RIGID,
        Position{10.f, 20.f},
        Size{32.f, 32.f},
        Color::blue()
    );

    TileMapPlugin::spawn(prefab, {.map = "#####\n", .wall = '#'});

    int wall_count = 0;
    ecs::Entity wall{};

    world.fetch<IsGround>().iter([&](ArchetypeView &view) {
        for (uint32_t i = 0; i < view.count(); i += 1) {
            wall = view.entity(i);
            wall_count += 1;
        }
    });

    cr_assert_eq(wall_count, 1);
    cr_assert_eq(*world.get<RigidBody>(wall), RIGID);
    cr_assert_float_eq(world.get<Position>(wall)->x, 10.f, 0.001f);
    cr_assert_float_eq(world.get<Position>(wall)->y, 20.f, 0.001f);
    cr_assert_float_eq(world.get<Size>(wall)->width, 160.f, 0.001f);
    cr_assert_float_eq(world.get<Size>(wall)->height, 32.f, 0.001f);
}

Test(tilemap, base_entity_is_consumed_after_spawn) {
    ecs::World world;
    world.plugin<TileMapPlugin>();

    const ecs::EntityRef prefab = world.create().add<IsGround>().set(
        RigidBody::RIGID,
        Position{0.f, 0.f},
        Size{16.f, 16.f},
        Color::blue()
    );
    const ecs::Entity prefab_entity = prefab.entity();

    TileMapPlugin::spawn(prefab, {.map = "#\n", .wall = '#'});

    cr_assert(!world.isAlive(prefab_entity));
}
