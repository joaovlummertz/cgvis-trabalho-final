#pragma once

#include <string>
#include <vector>
#include <map>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "SceneData.h"
#include "Renderer.h"
#include "matrices.h" // Assuming this contains Matrix_Translate, Matrix_Scale, etc.

enum class DoorState
{
    Closed,
    Opening,
    Open,
    Closing
};

class Entity
{
public:
    std::string name;
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    Entity(std::string entityName) : name(entityName) {}
    virtual ~Entity() = default;

    virtual void Update(float deltaTime) = 0;
    virtual void Draw(Renderer &renderer, const std::map<std::string, SceneObject> &virtualScene) = 0;
    virtual bool IntersectsRay(const glm::vec3 &origin, const glm::vec3 &direction, float &distance) const = 0;
    virtual bool IsDead() const { return false; }
};

class SlidingDoor : public Entity
{
public:
    DoorState state = DoorState::Closed;

    std::string meshName;

    // Smooth translation configurations
    glm::vec3 closedPosition;
    glm::vec3 openOffset;
    float yaw;
    float animationProgress = 0.0f; // 0.0 = Closed, 1.0 = Open
    float speed = 2.0f;             // Animation speed modifier

    SlidingDoor(std::string entityName, glm::vec3 startPos, glm::vec3 slideOffset, float rotationYaw = 0.0f, std::string renderMesh = "");

    void Toggle();
    void Update(float deltaTime) override;
    void Draw(Renderer &renderer, const std::map<std::string, SceneObject> &virtualScene) override;
    bool IntersectsRay(const glm::vec3 &origin, const glm::vec3 &direction, float &distance) const override;

private:
    struct CollisionBinding
    {
        CollisionObject *target;
        CollisionObject original;
    };

    std::vector<CollisionBinding> m_CollisionBindings;
    glm::vec3 m_ClosedBoundsMin;
    glm::vec3 m_ClosedBoundsMax;
    glm::vec3 m_CurrentBoundsMin;
    glm::vec3 m_CurrentBoundsMax;
};

class ChasingZombie : public Entity
{
public:
    glm::vec3 position;
    glm::vec3 spawnPosition;
    glm::vec3 velocity = glm::vec3(0.0f);
    float yaw = 0.0f;
    float movementSpeed = 1.8f;
    float renderScale = 0.02f;
    bool awake = false;
    int health = 3;
    bool isDead = false;
    float m_AttackCooldown = 0.0f;

    ChasingZombie(std::string meshName, glm::vec3 startPos, float speed = 1.8f, float scale = 0.02f);

    void Update(float deltaTime) override;
    void Draw(Renderer &renderer, const std::map<std::string, SceneObject> &virtualScene) override;
    bool IntersectsRay(const glm::vec3 &origin, const glm::vec3 &direction, float &distance) const override;
    bool IsDead() const override { return isDead; }
    void Hit();

private:
    struct CollisionBinding
    {
        CollisionObject *target;
        CollisionObject original;
    };

    std::vector<CollisionBinding> m_CollisionBindings;
    glm::vec3 m_CollisionLocalMin = glm::vec3(0.0f);
    glm::vec3 m_CollisionLocalMax = glm::vec3(0.0f);
    bool m_HasCollisionBounds = false;

    bool OwnsCollisionObject(const CollisionObject *object) const;
    void SynchronizeCollisionGeometry();
};
