#include "CoreComponents.hpp"

#include <cstdlib>
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
    this->value = strdup("");
}

Name::Name(const char *name) : value(nullptr) {
    if (!is_name_identifier(name)) {
        throw std::invalid_argument("invalid entity name");
    }
    this->value = strdup(name);
}

Name::Name(const std::string &name) : value(nullptr) {
    if (!is_name_identifier(name.c_str())) {
        throw std::invalid_argument("invalid entity name");
    }
    this->value = strdup(name.c_str());
}

void Name::onAdd(ecs::World &world, const ecs::Entity entity) {
    world.syncEntityName(entity);
}

void Name::onRemove(ecs::World &, const ecs::Entity, const Name *name) {
    free(const_cast<char *>(name->value));
}

void Name::onSet(ecs::World &world, const ecs::Entity entity, const Name *) {
    world.syncEntityName(entity);
}
