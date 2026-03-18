#pragma once
#include <cstdint>
#include "Graphics.hpp"

class IEngine;
using EntityFunc = void(*)(uint64_t, IEngine *);
using Callback = void(*)(IEngine *);

enum class Phase {
    PreUpdate,
    Update,
    PostUpdate,
};

enum class RigidBody {
    SENSOR,
    NORMAL
};

class IEngine {
public:
    virtual ~IEngine() = default;

    virtual uint64_t entity() = 0;

    virtual void setPosition(uint64_t entity, GlobalPosition position) = 0;

    virtual GlobalPosition getPosition(uint64_t entity) = 0;

    virtual void setVelocity(uint64_t entity, Velocity position) = 0;

    virtual Velocity getVelocity(uint64_t entity) = 0;

    virtual uint32_t loadImage(const std::string &path) = 0;

    virtual void setImage(uint64_t entity, uint32_t image_handle);

    virtual void setSprite(uint64_t entity, SpriteRect rect) = 0;

    virtual void setColor(uint64_t entity, Color color) = 0;

    virtual void setSize(uint64_t entity, Color color) = 0;

    virtual void setRigidBody(uint64_t entity, RigidBody body) = 0;

    virtual void addEntityFunc(uint64_t entity, Phase phase, EntityFunc func) = 0;

    virtual void addOnKeyPressed(KeyboardCode code, Callback func) = 0;

    virtual void addOnKeyReleased(KeyboardCode code, Callback func) = 0;

    virtual void onMouseReleased(KeyboardCode code, Callback func) = 0;

    virtual void onCollisionStart(EntityFunc func) = 0;

    virtual void onCollisionEnd(EntityFunc func) = 0;
};

