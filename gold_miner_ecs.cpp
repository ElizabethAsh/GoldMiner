/**
 * @file gold_miner_ecs.cpp
 * @brief Implementation of entity creation functions and systems using BAGEL ECS for Gold Miner game.
 */

#include "gold_miner_ecs.h"
#include "bagel.h"
#include <iostream>

namespace goldminer {

using namespace bagel;

//----------------------------------
/// @section Entity Creation Functions
//----------------------------------

/**
 * @brief Creates a new player entity with base components.
 * @param playerID The identifier of the player.
 * @return The ID of the created entity.
 */
id_type CreatePlayer(int playerID) {
    Entity e = Entity::create();
    e.addAll(Position{100.0f, 550.0f}, Velocity{}, Renderable{0}, PlayerInfo{playerID}, Score{0}, PlayerInput{});
    return e.entity().id;
}

/**
 * @brief Creates the rope entity for a given player.
 * @param playerID The identifier of the player who owns the rope.
 * @return The ID of the created entity.
 */
id_type CreateRope(int playerID) {
    Entity e = Entity::create();
    e.addAll(Position{120.0f, 540.0f}, Rotation{0.0f}, Length{0.0f}, RopeControl{}, RoperTag{}, PlayerInfo{playerID}, Collidable{});
    return e.entity().id;
}

/**
 * @brief Creates a gold item at the given coordinates.
 */
id_type CreateGold(float x, float y) {
    Entity e = Entity::create();
    e.addAll(Position{x, y}, Renderable{1}, Collectable{}, ItemType{ItemType::Type::Gold}, Value{100}, Weight{1.0f}, Collidable{}, PlayerInfo{-1});
    return e.entity().id;
}

/**
 * @brief Creates a rock item at the given coordinates.
 */
id_type CreateRock(float x, float y) {
    Entity e = Entity::create();
    e.addAll(Position{x, y}, Renderable{2}, Collectable{}, ItemType{ItemType::Type::Rock}, Value{10}, Weight{3.0f}, Collidable{}, PlayerInfo{-1});
    return e.entity().id;
}

/**
 * @brief Creates a diamond item at the given coordinates.
 */
id_type CreateDiamond(float x, float y) {
    Entity e = Entity::create();
    e.addAll(Position{x, y}, Renderable{3}, Collectable{}, ItemType{ItemType::Type::Diamond}, Value{300}, Weight{0.5f}, Collidable{}, PlayerInfo{-1});
    return e.entity().id;
}

/**
 * @brief Creates a mystery bag item at the given coordinates.
 */
id_type CreateMysteryBag(float x, float y) {
    Entity e = Entity::create();
    e.addAll(Position{x, y}, Renderable{4}, Collectable{}, ItemType{ItemType::Type::MysteryBag}, Value{0}, Weight{1.0f}, Collidable{}, PlayerInfo{-1});
    return e.entity().id;
}

/**
 * @brief Creates the game timer entity.
 */
id_type CreateTimer() {
    Entity e = Entity::create();
    e.add(GameTimer{60.0f});
    return e.entity().id;
}

/**
 * @brief Creates a UI entity for a given player.
 */
id_type CreateUIEntity(int playerID) {
    Entity e = Entity::create();
    e.addAll(UIComponent{0}, PlayerInfo{playerID});
    return e.entity().id;
}

/**
 * @brief Creates a mole entity at the given position.
 */
id_type CreateMole(float x, float y) {
    Entity e = Entity::create();
    e.addAll(Position{x, y}, Velocity{1.5f, 0.0f}, Renderable{5}, MoleAI{100.0f, true}, Collidable{});
    return e.entity().id;
}

//----------------------------------
/// @section System Implementations (Skeletons)
//----------------------------------

/**
 * @brief Reads player input and stores it in PlayerInput component.
 */
void PlayerInputSystem() {
    Mask mask;
    mask.set(Component<PlayerInfo>::Bit);
    mask.set(Component<PlayerInput>::Bit);

    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(mask)) continue;
        // No logic implemented yet
    }
}

/**
 * @brief Oscillates rope entities that are currently at rest.
 */
void RopeSwingSystem() {
    Mask mask;
    mask.set(Component<RoperTag>::Bit);
    mask.set(Component<Rotation>::Bit);
    mask.set(Component<RopeControl>::Bit);

    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(mask)) continue;
        // No logic implemented yet
    }
}

/**
 * @brief Handles rope extension and retraction logic.
 */
void RopeExtensionSystem() {
    Mask mask;
    mask.set(Component<RoperTag>::Bit);
    mask.set(Component<RopeControl>::Bit);
    mask.set(Component<Length>::Bit);
    mask.set(Component<Position>::Bit);
    mask.set(Component<PlayerInfo>::Bit);

    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(mask)) continue;
        // No logic implemented yet
    }
}

/**
 * @brief Detects collisions between rope and collectable items.
 */
void CollisionSystem() {
    Mask mask;
    mask.set(Component<Position>::Bit);
    mask.set(Component<Collidable>::Bit);

    Mask optional;
    optional.set(Component<RoperTag>::Bit);
    optional.set(Component<Collectable>::Bit);
    optional.set(Component<ItemType>::Bit);
    optional.set(Component<PlayerInfo>::Bit);

    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(mask)) continue;
        // No logic implemented yet
    }
}

/**
 * @brief Pulls collected items towards the player.
 */
void PullObjectSystem() {
    Mask mask;
    mask.set(Component<Collidable>::Bit);
    mask.set(Component<Position>::Bit);

    Mask optional;
    optional.set(Component<RoperTag>::Bit);
    optional.set(Component<Collectable>::Bit);
    optional.set(Component<ItemType>::Bit);
    optional.set(Component<PlayerInfo>::Bit);
    optional.set(Component<Weight>::Bit);

    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(mask)) continue;
        // No logic implemented yet
    }
}

/**
 * @brief Adds score to players based on collected items.
 */
void ScoreSystem() {
    Mask mask;
    mask.set(Component<ItemType>::Bit);
    mask.set(Component<PlayerInfo>::Bit);

    Mask optional;
    optional.set(Component<Value>::Bit);

    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(mask)) continue;
        // No logic implemented yet
    }
}

/**
 * @brief Assigns random value to mystery bag items when collected.
 */
void MysteryBagSystem() {
    Mask mask;
    mask.set(Component<PlayerInfo>::Bit);
    mask.set(Component<Value>::Bit);

    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(mask)) continue;
        // No logic implemented yet
    }
}

/**
 * @brief Renders all entities with a position and sprite.
 */
void RenderSystem() {
    Mask mask;
    mask.set(Component<Renderable>::Bit);
    mask.set(Component<Position>::Bit);

    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(mask)) continue;
        // No logic implemented yet
    }
}

/**
 * @brief Updates the global game timer.
 */
void TimerSystem() {
    Mask mask;
    mask.set(Component<GameTimer>::Bit);

    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(mask)) continue;
        // No logic implemented yet
    }
}

/**
 * @brief Displays score and time for each player.
 */
void UISystem() {
    Mask mask;
    mask.set(Component<UIComponent>::Bit);

    Mask optional;
    optional.set(Component<Score>::Bit);
    optional.set(Component<PlayerInfo>::Bit);

    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(mask)) continue;
        // No logic implemented yet
    }
}




/**
 * @brief Controls the mole's horizontal movement.
 */
void MoleSystem() {
    Mask mask;
    mask.set(Component<Mole>::Bit);
    mask.set(Component<Position>::Bit);
    mask.set(Component<Velocity>::Bit);

    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(mask)) continue;
        // No logic implemented yet
    }
}

/**
 * @brief Removes entities with a lifetime timer that expired.
 */
void LifeTimeSystem() {
    Mask mask;
    mask.set(Component<LifeTime>::Bit);

    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(mask)) continue;
        // No logic implemented yet
    }
}


} // namespace goldminer
