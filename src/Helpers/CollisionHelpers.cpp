
#include "../../includes/Helpers/CollisionHelpers.hpp"
#include "../../includes/EntityManagement/Entity.hpp"
#include "../../includes/EntityManagement/EntityTags.hpp"
#include <bitset>
#include <iostream>

enum Boundaries { TOP, BOTTOM, LEFT, RIGHT };

bool hasNullComponentPointers(const std::shared_ptr<Entity> &entity) {
  const bool cTransformIsNullPtr = !entity->cTransform;
  return cTransformIsNullPtr;
}

namespace CollisionHelpers {
  std::bitset<4> detectOutOfBounds(const std::shared_ptr<Entity> &entity,
                                   const Vec2                    &window_size) {
    if (hasNullComponentPointers(entity)) {
      throw std::runtime_error("Entity " + entity->tag() + ", with ID " +
                               std::to_string(entity->id()) +
                               " lacks a transform or collision component.");
    }

    const std::shared_ptr<CShape> &entityCShape = entity->cShape;

    const Vec2 &topLeftCornerPos = entity->cTransform->topLeftCornerPos;

    const bool collidesWithTop    = topLeftCornerPos.y <= 0;
    const bool collidesWithBottom = topLeftCornerPos.y + entityCShape->rect.h >= window_size.y;
    const bool collidesWithLeft   = topLeftCornerPos.x <= 0;
    const bool collidesWithRight  = topLeftCornerPos.x + entityCShape->rect.w >= window_size.x;

    std::bitset<4> collidesWithBoundary;
    collidesWithBoundary[TOP]    = collidesWithTop;
    collidesWithBoundary[BOTTOM] = collidesWithBottom;
    collidesWithBoundary[LEFT]   = collidesWithLeft;
    collidesWithBoundary[RIGHT]  = collidesWithRight;

    return collidesWithBoundary;
  }

  void enforcePlayerBounds(const std::shared_ptr<Entity> &entity,
                           const std::bitset<4>          &collides,
                           const Vec2                    &window_size) {
    if (hasNullComponentPointers(entity)) {
      throw std::runtime_error("Entity " + entity->tag() + ", with ID " +
                               std::to_string(entity->id()) +
                               " lacks a transform or collision component.");
    }
    const std::shared_ptr<CShape> &cShape             = entity->cShape;
    Vec2                          &leftCornerPosition = entity->cTransform->topLeftCornerPos;

    if (collides[TOP]) {
      leftCornerPosition.y = 0;
    }
    if (collides[BOTTOM]) {
      leftCornerPosition.y = window_size.y - cShape->rect.h;
    }
    if (collides[LEFT]) {
      leftCornerPosition.x = 0;
    }
    if (collides[RIGHT]) {
      leftCornerPosition.x = window_size.x - cShape->rect.w;
    }
  }

  void enforceNonPlayerBounds(const std::shared_ptr<Entity> &entity,
                              const std::bitset<4>          &collides,
                              const Vec2                    &window_size) {

    if (hasNullComponentPointers(entity)) {
      throw std::runtime_error("Entity " + entity->tag() + ", with ID " +
                               std::to_string(entity->id()) +
                               " lacks a transform or collision component.");
    }
    if (entity->tag() == EntityTags::Player) {
      return;
    }
    const std::shared_ptr<CShape> &cShape             = entity->cShape;
    Vec2                          &leftCornerPosition = entity->cTransform->topLeftCornerPos;
    Vec2                          &velocity           = entity->cTransform->velocity;
    // Bounce the entity off the boundaries
    if (collides[TOP]) {
      leftCornerPosition.y = 0;
      velocity.y           = -velocity.y;
    }
    if (collides[BOTTOM]) {
      leftCornerPosition.y = window_size.y - cShape->rect.h;
      velocity.y           = -velocity.y;
    }
    if (collides[LEFT]) {
      leftCornerPosition.x = 0;
      velocity.x           = -velocity.x;
    }
    if (collides[RIGHT]) {
      leftCornerPosition.x = window_size.x - cShape->rect.w;
      velocity.x           = -velocity.x;
    }
  };

  bool calculateCollisionBetweenEntities(const std::shared_ptr<Entity> &entityA,
                                         const std::shared_ptr<Entity> &entityB) {
    if (hasNullComponentPointers(entityA)) {
      throw std::runtime_error("Entity " + entityA->tag() + ", with ID " +
                               std::to_string(entityA->id()) +
                               " lacks a transform or collision component.");
    }
    if (hasNullComponentPointers(entityB)) {
      throw std::runtime_error("Entity " + entityB->tag() + ", with ID " +
                               std::to_string(entityB->id()) +
                               " lacks a transform or collision component.");
    }

    const SDL_Rect &rectA = entityA->cShape->rect;
    const SDL_Rect &rectB = entityB->cShape->rect;

    const Vec2 &posA = entityA->cTransform->topLeftCornerPos;
    const Vec2 &posB = entityB->cTransform->topLeftCornerPos;

    const bool leftSideCollision   = posA.x + rectA.w >= posB.x;
    const bool rightSideCollision  = posA.x <= posB.x + rectB.w;
    const bool topSideCollision    = posA.y + rectA.h >= posB.y;
    const bool bottomSideCollision = posA.y <= posB.y + rectB.h;

    return leftSideCollision && rightSideCollision && topSideCollision && bottomSideCollision;
  }

} // namespace CollisionHelpers