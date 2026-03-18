#pragma once
#include <functional>
#include <utility>

#include "arcade/IGame.hpp"
#include "datastructures/SparseSet.hpp"
#include "ecs/World.hpp"
#include "reflection/TypeCounter.hpp"


struct SceneFamily;
using SceneTypeCounter = reflection::TypeCounter<SceneFamily>;

class Engine : IGame {
    datastructures::SparseSet<ecs::World *> scenes;
    ecs::World *currentScene = nullptr;
    std::string name = "NoName";

    template<typename Scene>
    static uint16_t id() {
        return SceneTypeCounter::id<Scene>();
    }

    template<typename Scene>
    ecs::World *scenePtr() {
        ecs::World *&scene = this->scenes.getOrCreate(this->id<Scene>());
        if (scene == nullptr) {
            scene = new ecs::World();
            if constexpr (ecs::IsPlugin<Scene>) {
                scene->plugin<Scene>();
            }
        }
        return scene;
    }

public:
    explicit Engine(std::string name, const std::function<void(Engine &)> &init) : name(std::move(name)) {
        init(*this);
    }

    ~Engine() override {
        for (const ecs::World *scene: this->scenes.getDense()) {
            delete scene;
        }
    }

    template<typename Scene>
    ecs::World &scene() {
        return *this->scenePtr<Scene>();
    }

    template<typename Scene>
    void progress() {
        this->scenes.get(this->id<Scene>())->progress();
    }

    template<typename Scene>
    void setScene() {
        this->currentScene = this->scenePtr<Scene>();
    }

    void progress() const {
        this->currentScene->progress();
    }

    std::string &getName() override {
        return this->name;
    };

    void update(GraphicsApi *api) override {
        this->currentScene->api = api;
        this->progress();
    };
};
