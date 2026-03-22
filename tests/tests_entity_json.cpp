#include <criterion/criterion.h>

#include "ecs/World.hpp"
#include "ecs/entity/EntityJson.hpp"


Test(entity_json, basique) {
    ecs::World world;

    const auto entity = world.create().set(Name{"suleyman"}).entity();

    const auto result = ecs::entityToJson(world, entity);

    const auto *expected =
            "{\n"
            "  \"name\": \"suleyman\"\n"
            "}";

    cr_assert_eq(result, expected);
}
