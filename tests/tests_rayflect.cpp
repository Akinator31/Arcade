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

Test(rayflect, unsigned_small_primitives) {
    struct PackedValues {
        uint8_t flags;
        uint16_t id;

        rayflect(packed_values, {
            packed_values->member<uint8_t>("flags");
            packed_values->member<uint16_t>("id");
        })
    };

    PackedValues values{3, 42};
    const StructDef *def = PackedValues::def();

    cr_assert_eq(def->get_member_value<uint8_t>(&values, "flags"), 3);
    cr_assert_eq(def->get_member_value<uint16_t>(&values, "id"), 42);

    def->from_string(&values, "flags", "7");
    def->from_string(&values, "id", "512");

    cr_assert_eq(values.flags, 7);
    cr_assert_eq(values.id, 512);
}
