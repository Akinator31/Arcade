#include "engine/ecs/World.hpp"
#include <criterion/criterion.h>

Test(rayflect, basic) {
    struct Health {
        int value;

        rayflect(health, { health->member<int>("value"); })
    };

    Health my_health = {10};
    const StructDef *def = Health::def();

    cr_assert_eq(def->get_member_value<int>(&my_health, "value"), 10);
}
