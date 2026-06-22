#pragma once

#include <glm/mat4x4.hpp>

namespace TramIntro
{
    void Initialize();
    void Update(float delta_time);
    bool IsActive();
    glm::mat4 GetModelMatrix();
}
