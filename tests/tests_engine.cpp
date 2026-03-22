#include <criterion/criterion.h>

#include "engine/Engine.hpp"

struct EngineTickSystem {
    static void run(EngineTickSystem *, ecs::World &world) {
        world.state<uint16_t>(world.getState<uint16_t>() + 1);
    }
};

Test(engine, scene) {
    Engine engine("test", [](Engine &, IDisplayModule *) {
    }, {});

    struct MainScene;
    auto &scene = engine.scene<MainScene>();

    cr_assert_eq(scene.findEntityByName("player").has_value(), false);
    scene.create().set(Name{"player"});
    cr_assert_eq(engine.scene<MainScene>().findEntityByName("player").has_value(), true);

    struct OtherScene;
    cr_assert_eq(engine.scene<OtherScene>().findEntityByName("player").has_value(), false);
    cr_assert_eq(engine.scene<MainScene>().findEntityByName("player").has_value(), true);
}

Test(engine, progress_scene_type) {
    Engine engine("test", [](Engine &, IDisplayModule *) {
    }, {});

    struct MainScene;
    struct OtherScene;

    auto &mainScene = engine.scene<MainScene>();
    mainScene.state<uint16_t>(0);
    mainScene.system<EngineTickSystem>();

    auto &otherScene = engine.scene<OtherScene>();
    otherScene.state<uint16_t>(0);
    otherScene.system<EngineTickSystem>();

    engine.progress<MainScene>();
    engine.progress<MainScene>();
    engine.progress<OtherScene>();

    cr_assert_eq(mainScene.getState<uint16_t>(), 2);
    cr_assert_eq(otherScene.getState<uint16_t>(), 1);
}

Test(engine, progress_current_scene) {
    Engine engine("test", [](Engine &, IDisplayModule *) {
    }, {});

    struct MainScene;
    struct OtherScene;

    auto &mainScene = engine.scene<MainScene>();
    mainScene.state<uint16_t>(0);
    mainScene.system<EngineTickSystem>();

    auto &otherScene = engine.scene<OtherScene>();
    otherScene.state<uint16_t>(0);
    otherScene.system<EngineTickSystem>();

    engine.setScene<MainScene>();
    engine.progress();
    engine.progress();

    engine.setScene<OtherScene>();
    engine.progress();

    cr_assert_eq(mainScene.getState<uint16_t>(), 2);
    cr_assert_eq(otherScene.getState<uint16_t>(), 1);
}
