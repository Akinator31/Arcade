#pragma once
#include <chrono>
#include <functional>
#include <utility>

#include "arcade/IGameModule.hpp"
#include "datastructures/SparseSet.hpp"
#include "ecs/World.hpp"
#include "reflection/TypeCounter.hpp"


struct SceneFamily;
using SceneTypeCounter = reflection::TypeCounter<SceneFamily>;

class Engine : IGameModule {
    datastructures::SparseSet<ecs::World *> scenes;
    ecs::World *currentScene = nullptr;
    std::string name = "NoName";
    std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();
    bool hasRenderedFrame = false;
    std::vector<Resource> _resources;

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
    explicit Engine(std::string name, const std::function<void(Engine &)> &init,
                    const std::vector<Resource> &resources) : name(std::move(name)), _resources(resources) {
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

    void update(IDisplayModule *api) override {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

        this->currentScene->api = api;
        this->currentScene->deltaTime = this->hasRenderedFrame
                                            ? std::chrono::duration<float>(now - this->lastFrameTime).count()
                                            : 0.f;
        this->lastFrameTime = now;
        this->hasRenderedFrame = true;
        this->progress();
    };

    const std::vector<Resource> &getResources() override {
        return this->_resources;
    }
};
