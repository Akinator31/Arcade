#pragma once

#include <string>

class GraphicsApi;

class IGame {
public:
    virtual ~IGame() = default;

    virtual std::string &getName() = 0;
    virtual void update(GraphicsApi *api) = 0;
};
