#include "CoreComponents.hpp"

#include <cctype>
#include <stdexcept>
#include <string>

#include "World.hpp"
#include "engine/parsing/Scanner.hpp"

namespace {
    bool is_name_identifier(const char *value) {
        if (value == nullptr) {
            return false;
        }
        Scanner scanner(value);
        if (scanner.isDone()) {
            return false;
        }
        if (!scanner.expect([](const char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; })) {
            return false;
        }
        scanner.advance();
        scanner.take_while([](const char c) {
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '(' || c == ')' || c == '[' ||
                   c == ']' || c == ',' || c == ' ';
        });
        return scanner.isDone();
    }
}

Name::Name() : value("") {
}

Name::Name(const char *name) : value(name) {
    if (!is_name_identifier(name)) {
        throw std::invalid_argument("invalid entity name");
    }
}

void Name::onAdd(ecs::World &world, const ecs::Entity entity) {
    world.syncEntityName(entity);
}

void Name::onSet(ecs::World &world, const ecs::Entity entity, const Name *) {
    world.syncEntityName(entity);
}
