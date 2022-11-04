#pragma once

namespace sp::ecs {

class System {
public:
    virtual void update(float delta) = 0;
};

}