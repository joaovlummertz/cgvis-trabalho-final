#include "Entity.h"
#include "Interaction.h"

#include <cmath>
#include <limits>

SlidingDoor::SlidingDoor(std::string meshName, glm::vec3 startPos, glm::vec3 slideOffset, float rotationYaw)
    : Entity(meshName), closedPosition(startPos), openOffset(slideOffset), yaw(rotationYaw)
{
    m_ClosedBoundsMin = glm::vec3(std::numeric_limits<float>::max());
    m_ClosedBoundsMax = glm::vec3(std::numeric_limits<float>::lowest());

    // Bind every collision sub-mesh that belongs to this door.
    for (auto &colObj : g_CollisionScene)
    {
        if (colObj.name.rfind(meshName, 0) == 0)
        {
            m_CollisionBindings.push_back({&colObj, colObj});
            m_ClosedBoundsMin = glm::min(m_ClosedBoundsMin, colObj.bbox_min);
            m_ClosedBoundsMax = glm::max(m_ClosedBoundsMax, colObj.bbox_max);
        }
    }

    m_CurrentBoundsMin = m_ClosedBoundsMin;
    m_CurrentBoundsMax = m_ClosedBoundsMax;

    // Use the same transform composition as the animation at progress 0.
    modelMatrix = Matrix_Translate(closedPosition.x, closedPosition.y, closedPosition.z) *
                  Matrix_Rotate_Y(yaw) *
                  Matrix_Scale(0.02f, 0.02f, 0.02f);
}

void SlidingDoor::Toggle()
{
    if (state == DoorState::Closed || state == DoorState::Closing)
        state = DoorState::Opening;
    else if (state == DoorState::Open || state == DoorState::Opening)
        state = DoorState::Closing;
}

void SlidingDoor::Update(float deltaTime)
{
    bool physicsNeedsUpdate = false;

    if (state == DoorState::Opening)
    {
        animationProgress += speed * deltaTime;
        if (animationProgress >= 1.0f)
        {
            animationProgress = 1.0f;
            state = DoorState::Open;
        }
        physicsNeedsUpdate = true;
    }
    else if (state == DoorState::Closing)
    {
        animationProgress -= speed * deltaTime;
        if (animationProgress <= 0.0f)
        {
            animationProgress = 0.0f;
            state = DoorState::Closed;
        }
        physicsNeedsUpdate = true;
    }

    if (physicsNeedsUpdate)
    {
        glm::vec3 currentOffset = openOffset * animationProgress;
        glm::vec3 worldOffset(
            std::cos(yaw) * currentOffset.x + std::sin(yaw) * currentOffset.z,
            currentOffset.y,
            -std::sin(yaw) * currentOffset.x + std::cos(yaw) * currentOffset.z);

        // 1. Update the render model matrix
        modelMatrix = Matrix_Translate(closedPosition.x, closedPosition.y, closedPosition.z) *
                      Matrix_Rotate_Y(yaw) *
                      Matrix_Translate(currentOffset.x, currentOffset.y, currentOffset.z) *
                      Matrix_Scale(0.02f, 0.02f, 0.02f);

        // 2. Synchronize physics data so player collisions and raycasts follow the moving door!
        for (auto &binding : m_CollisionBindings)
        {
            binding.target->bbox_min = binding.original.bbox_min + worldOffset;
            binding.target->bbox_max = binding.original.bbox_max + worldOffset;

            for (size_t i = 0; i < binding.target->vertices.size(); ++i)
            {
                binding.target->vertices[i] = binding.original.vertices[i] + worldOffset;
            }
        }

        m_CurrentBoundsMin = m_ClosedBoundsMin + worldOffset;
        m_CurrentBoundsMax = m_ClosedBoundsMax + worldOffset;
    }
}

void SlidingDoor::Draw(Renderer &renderer, const std::map<std::string, SceneObject> &virtualScene)
{
    renderer.SetModelMatrix(modelMatrix);
    renderer.DrawVirtualObject(name.c_str(), virtualScene);
}

bool SlidingDoor::IntersectsRay(const glm::vec3 &origin, const glm::vec3 &direction, float &distance) const
{
    return Interaction::RayIntersectsAABB(
        origin, direction, m_CurrentBoundsMin, m_CurrentBoundsMax, distance);
}
