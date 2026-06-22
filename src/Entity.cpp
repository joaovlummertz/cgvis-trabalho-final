#include "Entity.h"
#include "Interaction.h"
#include "InputHandler.h"
#include "Player.h"

#include <cmath>
#include <algorithm>
#include <limits>
#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>

namespace
{
    constexpr float kZombieGravity = 18.0f;
    constexpr float kZombieMaxFallSpeed = 40.0f;
    constexpr float kZombieStepSize = 0.05f;
    constexpr float kZombieSlopeLimitY = 0.55f;
    constexpr float kZombieCollisionRadius = 0.35f;
    constexpr float kZombieAttackRange = 2.0f;

    void BuildBoxMesh(const glm::vec3 &boxMin, const glm::vec3 &boxMax, std::vector<glm::vec3> &vertices)
    {
        vertices.clear();
        vertices.reserve(36);

        const glm::vec3 v000(boxMin.x, boxMin.y, boxMin.z);
        const glm::vec3 v001(boxMin.x, boxMin.y, boxMax.z);
        const glm::vec3 v010(boxMin.x, boxMax.y, boxMin.z);
        const glm::vec3 v011(boxMin.x, boxMax.y, boxMax.z);
        const glm::vec3 v100(boxMax.x, boxMin.y, boxMin.z);
        const glm::vec3 v101(boxMax.x, boxMin.y, boxMax.z);
        const glm::vec3 v110(boxMax.x, boxMax.y, boxMin.z);
        const glm::vec3 v111(boxMax.x, boxMax.y, boxMax.z);

        auto addQuad = [&vertices](const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d) {
            vertices.push_back(a);
            vertices.push_back(b);
            vertices.push_back(c);
            vertices.push_back(a);
            vertices.push_back(c);
            vertices.push_back(d);
        };

        addQuad(v001, v101, v111, v011);
        addQuad(v100, v000, v010, v110);
        addQuad(v000, v001, v011, v010);
        addQuad(v101, v100, v110, v111);
        addQuad(v010, v011, v111, v110);
        addQuad(v000, v100, v101, v001);
    }

    glm::vec3 ClosestPointTriangle(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c)
    {
        glm::vec3 ab = b - a;
        glm::vec3 ac = c - a;
        glm::vec3 ap = p - a;
        float d1 = glm::dot(ab, ap);
        float d2 = glm::dot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f)
            return a;

        glm::vec3 bp = p - b;
        float d3 = glm::dot(ab, bp);
        float d4 = glm::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3)
            return b;

        float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        {
            float v = d1 / (d1 - d3);
            return a + v * ab;
        }

        glm::vec3 cp = p - c;
        float d5 = glm::dot(ab, cp);
        float d6 = glm::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6)
            return c;

        float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        {
            float w = d2 / (d2 - d6);
            return a + w * ac;
        }

        float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        {
            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return b + w * (c - b);
        }

        float denom = 1.0f / (va + vb + vc);
        float v = vb * denom;
        float w = vc * denom;
        return a + ab * v + ac * w;
    }

    glm::vec4 ResolveZombieCollisions(glm::vec4 position,
                                      const std::vector<const CollisionObject *> &ignoredObjects,
                                      bool *hit_floor = nullptr,
                                      bool *hit_ceiling = nullptr,
                                      glm::vec3 *ground_normal = nullptr)
    {
        float r = kZombieCollisionRadius;
        float sphere_heights[3] = {0.25f, 0.75f, 1.25f};

        glm::vec3 pos = glm::vec3(position);

        if (hit_floor != nullptr)
            *hit_floor = false;
        if (hit_ceiling != nullptr)
            *hit_ceiling = false;
        if (ground_normal != nullptr)
            *ground_normal = glm::vec3(0.0f, -1.0f, 0.0f);

        for (int iter = 0; iter < 4; ++iter)
        {
            bool collided = false;

            for (float h_offset : sphere_heights)
            {
                glm::vec3 sphere_center = pos + glm::vec3(0.0f, h_offset, 0.0f);
                glm::vec3 a_min = sphere_center - glm::vec3(r);
                glm::vec3 a_max = sphere_center + glm::vec3(r);

                for (const auto &obj : g_CollisionScene)
                {
                    if (std::find(ignoredObjects.begin(), ignoredObjects.end(), &obj) != ignoredObjects.end())
                        continue;

                    bool overlap = (a_min.x <= obj.bbox_max.x) && (a_max.x >= obj.bbox_min.x) &&
                                   (a_min.y <= obj.bbox_max.y) && (a_max.y >= obj.bbox_min.y) &&
                                   (a_min.z <= obj.bbox_max.z) && (a_max.z >= obj.bbox_min.z);
                    if (!overlap)
                        continue;

                    for (size_t i = 0; i + 2 < obj.vertices.size(); i += 3)
                    {
                        glm::vec3 a = obj.vertices[i];
                        glm::vec3 b = obj.vertices[i + 1];
                        glm::vec3 c = obj.vertices[i + 2];

                        glm::vec3 q = ClosestPointTriangle(sphere_center, a, b, c);
                        float dist2 = glm::dot(sphere_center - q, sphere_center - q);

                        if (dist2 < r * r)
                        {
                            collided = true;
                            float dist = std::sqrt(dist2);

                            glm::vec3 normal;
                            if (dist > 1e-6f)
                                normal = (sphere_center - q) / dist;
                            else
                                normal = glm::normalize(glm::cross(b - a, c - a));

                            float penetration = r - dist;
                            pos += normal * penetration;

                            if (hit_floor != nullptr && normal.y > 0.5f)
                                *hit_floor = true;
                            if (hit_ceiling != nullptr && normal.y < -0.5f)
                                *hit_ceiling = true;
                            if (ground_normal != nullptr && normal.y > kZombieSlopeLimitY && normal.y > ground_normal->y)
                                *ground_normal = normal;

                            sphere_center = pos + glm::vec3(0.0f, h_offset, 0.0f);
                            a_min = sphere_center - glm::vec3(r);
                            a_max = sphere_center + glm::vec3(r);
                        }
                    }
                }
            }

            if (!collided)
                break;
        }

        return glm::vec4(pos, 1.0f);
    }

    glm::vec4 ResolveZombieMovement(glm::vec4 start_position,
                                    glm::vec4 displacement,
                                    const std::vector<const CollisionObject *> &ignoredObjects,
                                    bool *hit_floor = nullptr,
                                    bool *hit_ceiling = nullptr,
                                    glm::vec3 *ground_normal = nullptr)
    {
        if (hit_floor != nullptr)
            *hit_floor = false;
        if (hit_ceiling != nullptr)
            *hit_ceiling = false;
        if (ground_normal != nullptr)
            *ground_normal = glm::vec3(0.0f, -1.0f, 0.0f);

        float dist = glm::length(displacement);
        int num_steps = std::max(1, (int)std::ceil(dist / kZombieStepSize));
        glm::vec4 step_displacement = displacement / (float)num_steps;
        glm::vec4 current_pos = start_position;

        for (int i = 0; i < num_steps; ++i)
        {
            glm::vec4 next_pos = current_pos + step_displacement;
            glm::vec3 before = glm::vec3(current_pos);
            glm::vec3 after = glm::vec3(next_pos);
            bool step_hit_floor = false;
            bool step_hit_ceiling = false;
            glm::vec3 step_ground_normal(0.0f, -1.0f, 0.0f);

            current_pos = ResolveZombieCollisions(next_pos, ignoredObjects, &step_hit_floor, &step_hit_ceiling, &step_ground_normal);

            glm::vec3 resolved = glm::vec3(current_pos);
            if (hit_floor != nullptr && after.y < before.y && resolved.y >= after.y)
                *hit_floor = *hit_floor || step_hit_floor;
            if (hit_ceiling != nullptr && after.y > before.y && resolved.y <= after.y)
                *hit_ceiling = *hit_ceiling || step_hit_ceiling;
            if (ground_normal != nullptr && step_ground_normal.y > ground_normal->y)
                *ground_normal = step_ground_normal;
        }

        return current_pos;
    }
}

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

ChasingZombie::ChasingZombie(std::string meshName, glm::vec3 startPos, float speed, float scale)
    : Entity(meshName), position(startPos), spawnPosition(startPos), movementSpeed(speed), renderScale(scale)
{
    // Bind the zombie collision mesh so the player and the zombie share the same world obstacles.
    for (auto &colObj : g_CollisionScene)
    {
        if (colObj.name.rfind(meshName, 0) == 0)
        {
            m_CollisionBindings.push_back({&colObj, colObj});
            if (!m_HasCollisionBounds)
            {
                m_CollisionLocalMin = colObj.bbox_min - spawnPosition;
                m_CollisionLocalMax = colObj.bbox_max - spawnPosition;
                m_HasCollisionBounds = true;
            }
        }
    }

    if (m_HasCollisionBounds)
    {
        for (auto &binding : m_CollisionBindings)
        {
            BuildBoxMesh(spawnPosition + m_CollisionLocalMin, spawnPosition + m_CollisionLocalMax, binding.target->vertices);
            binding.target->bbox_min = spawnPosition + m_CollisionLocalMin;
            binding.target->bbox_max = spawnPosition + m_CollisionLocalMax;
        }
    }

    modelMatrix = Matrix_Translate(position.x, position.y, position.z) *
                  Matrix_Scale(renderScale, renderScale, renderScale);
    SynchronizeCollisionGeometry();
}

void ChasingZombie::Update(float deltaTime)
{
    if (!InputHandler::inputState.g_HasCrowbar)
    {
        position = spawnPosition;
        velocity = glm::vec3(0.0f);
        yaw = 0.0f;
        awake = false;
        modelMatrix = Matrix_Translate(position.x, position.y, position.z) *
                      Matrix_Scale(renderScale, renderScale, renderScale);
        SynchronizeCollisionGeometry();
        return;
    }

    awake = true;

    glm::vec3 target = glm::vec3(Player::playerState.g_PlayerPosition);
    glm::vec3 toPlayer = target - position;
    glm::vec3 horizontalToPlayer = toPlayer;
    horizontalToPlayer.y = 0.0f;

    float horizontalDistance = glm::length(horizontalToPlayer);
    if (horizontalDistance > kZombieAttackRange && horizontalDistance > 0.000001f)
    {
        glm::vec3 direction = glm::normalize(horizontalToPlayer);
        float maxTravel = horizontalDistance - kZombieAttackRange;
        float travel = std::min(movementSpeed * deltaTime, maxTravel);
        glm::vec4 horizontalDisplacement(direction * travel, 0.0f);
        bool hit_floor = false;
        bool hit_ceiling = false;
        glm::vec3 ground_normal(0.0f, 1.0f, 0.0f);
        std::vector<const CollisionObject *> ignoredObjects;
        ignoredObjects.reserve(m_CollisionBindings.size());
        for (const auto &binding : m_CollisionBindings)
            ignoredObjects.push_back(binding.target);
        glm::vec4 moved = ResolveZombieMovement(
            glm::vec4(position, 1.0f),
            horizontalDisplacement,
            ignoredObjects,
            &hit_floor,
            &hit_ceiling,
            &ground_normal);
        position = glm::vec3(moved);
        yaw = std::atan2(direction.x, direction.z);
    }
    else if (horizontalDistance > 0.000001f)
    {
        glm::vec3 direction = glm::normalize(horizontalToPlayer);
        yaw = std::atan2(direction.x, direction.z);
    }

    if (horizontalDistance <= kZombieAttackRange)
    {
        velocity = glm::vec3(0.0f);
        modelMatrix = Matrix_Translate(position.x, position.y, position.z) *
                      Matrix_Rotate_Y(yaw) *
                      Matrix_Scale(renderScale, renderScale, renderScale);
        SynchronizeCollisionGeometry();
        return;
    }

    velocity.y -= kZombieGravity * deltaTime;
    if (velocity.y < -kZombieMaxFallSpeed)
        velocity.y = -kZombieMaxFallSpeed;

    bool hit_floor = false;
    bool hit_ceiling = false;
    glm::vec3 contact_ground_normal(0.0f, 1.0f, 0.0f);
    std::vector<const CollisionObject *> ignoredObjects;
    ignoredObjects.reserve(m_CollisionBindings.size());
    for (const auto &binding : m_CollisionBindings)
        ignoredObjects.push_back(binding.target);

    glm::vec4 verticalDisplacement(0.0f, velocity.y * deltaTime, 0.0f, 0.0f);
    glm::vec4 resolved = ResolveZombieMovement(
        glm::vec4(position, 1.0f),
        verticalDisplacement,
        ignoredObjects,
        &hit_floor,
        &hit_ceiling,
        &contact_ground_normal);
    position = glm::vec3(resolved);

    if (hit_floor && velocity.y <= 0.0f && contact_ground_normal.y >= kZombieSlopeLimitY)
    {
        velocity.y = 0.0f;
    }
    else if (hit_ceiling && velocity.y > 0.0f)
    {
        velocity.y = 0.0f;
    }

    modelMatrix = Matrix_Translate(position.x, position.y, position.z) *
                  Matrix_Rotate_Y(yaw) *
                  Matrix_Scale(renderScale, renderScale, renderScale);
    SynchronizeCollisionGeometry();
}

void ChasingZombie::Draw(Renderer &renderer, const std::map<std::string, SceneObject> &virtualScene)
{
    renderer.SetModelMatrix(modelMatrix);
    renderer.DrawVirtualObject(name.c_str(), virtualScene);
}

bool ChasingZombie::IntersectsRay(const glm::vec3 &origin, const glm::vec3 &direction, float &distance) const
{
    if (m_CollisionBindings.empty())
        return Interaction::RayIntersectsAABB(
            origin, direction, position - glm::vec3(kZombieCollisionRadius), position + glm::vec3(kZombieCollisionRadius), distance);

    glm::vec3 boundsMin(std::numeric_limits<float>::max());
    glm::vec3 boundsMax(std::numeric_limits<float>::lowest());
    for (const auto &binding : m_CollisionBindings)
    {
        boundsMin = glm::min(boundsMin, binding.target->bbox_min);
        boundsMax = glm::max(boundsMax, binding.target->bbox_max);
    }

    return Interaction::RayIntersectsAABB(origin, direction, boundsMin, boundsMax, distance);
}

bool ChasingZombie::OwnsCollisionObject(const CollisionObject *object) const
{
    for (const auto &binding : m_CollisionBindings)
    {
        if (binding.target == object)
            return true;
    }
    return false;
}

void ChasingZombie::SynchronizeCollisionGeometry()
{
    for (auto &binding : m_CollisionBindings)
    {
        BuildBoxMesh(position + m_CollisionLocalMin, position + m_CollisionLocalMax, binding.target->vertices);
        binding.target->bbox_min = position + m_CollisionLocalMin;
        binding.target->bbox_max = position + m_CollisionLocalMax;
    }
}
