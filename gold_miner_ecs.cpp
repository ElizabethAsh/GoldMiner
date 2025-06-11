/**
 * @file gold_miner_ecs.cpp
 * @brief Implementation of entity creation functions and systems using BAGEL ECS for Gold Miner game.
 */
#include "gold_miner_ecs.h"
#include "bagel.h"
#include "sprite_manager.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include <iostream>

namespace goldminer {
    b2WorldId gWorld = b2_nullWorldId;

    using namespace bagel;

    void initBox2DWorld () {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = { 0.0f, 9.8f };
        gWorld = b2CreateWorld(&worldDef);
        b2World_SetHitEventThreshold(gWorld, 0.1f);

    }


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
        e.addAll(Position{570.0f, 10.0f}, Velocity{}, Renderable{SPRITE_PLAYER_IDLE}, PlayerInfo{playerID}, Score{0}, PlayerInput{});
        return e.entity().id;
    }

/**
 * @brief Creates a dynamic rope entity with a narrow rectangle body for collision testing.
 *
 * This version of the rope includes a Box2D dynamic body, allowing it to interact
 * with static objects such as gold, rocks, and treasure chests. The shape is a
 * narrow vertical rectangle (thin rope), with collision enabled.
 *
 * Components added:
 * - Position: top-left (for rendering)
 * - Rotation: (optional for future use)
 * - Length: logical rope length
 * - RopeControl, RoperTag: identify as rope
 * - Collidable: enables collision system
 * - PhysicsBody: Box2D body handle
 * - PlayerInfo: for multi-player logic
 *
 * @param playerID The rope's owning player
 * @return Entity ID
 */
    id_type CreateRope(int playerID) {
        Entity e = Entity::create();

        // Visual size in pixels
        float ropeW = 6.0f;
        float ropeH = 120.0f;

        float startX = 620.0f;
        float startY = 100.0f;

        constexpr float PPM = 50.0f;

        float centerX = startX + ropeW / 2.0f;
        float centerY = startY + ropeH / 2.0f;

        float hw = ropeW / 2.0f / PPM;
        float hh = ropeH / 2.0f / PPM;

        // Create dynamic Box2D body
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = {centerX / PPM, centerY / PPM};

        b2BodyId bodyId = b2CreateBody(gWorld, &bodyDef);
        b2Body_EnableHitEvents(bodyId, true);
        b2Vec2 velocity = {0.0f, 2.0f}; // Negative Y = upward
        b2Body_SetLinearVelocity(bodyId, velocity);


        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.material.friction = 0.5f;
        shapeDef.material.restitution = 0.1f;
        shapeDef.filter.categoryBits = 0x0001;
        shapeDef.filter.maskBits = 0xFFFF;

        shapeDef.isSensor = true;
        shapeDef.enableHitEvents = true;

        b2Polygon box = {};
        box.count = 4;
        box.vertices[0] = { -hw, -hh };
        box.vertices[1] = {  hw, -hh };
        box.vertices[2] = {  hw,  hh };
        box.vertices[3] = { -hw,  hh };

        b2CreatePolygonShape(bodyId, &shapeDef, &box);
        b2Body_SetUserData(bodyId, new bagel::ent_type{e.entity()});

        e.addAll(
                Position{startX, startY},
                Rotation{0.0f},
                Length{ropeH},
                RopeControl{},
                RoperTag{},
                PlayerInfo{playerID},
                Collidable{},
                PhysicsBody{bodyId}
        );

        return e.entity().id;
    }

/**
 * @brief Creates a gold item at the given coordinates.
 */
    id_type CreateGold(float x, float y) {
        Entity e = Entity::create();

        // Get sprite dimensions for gold
        SDL_Rect rect = GetSpriteSrcRect(SPRITE_GOLD);
        float width = static_cast<float>(rect.w);
        float height = static_cast<float>(rect.h);

        // Calculate center of sprite for Box2D positioning
        float centerX = x + width / 2.0f;
        float centerY = y + height / 2.0f;

        constexpr float PPM = 50.0f; // Pixels per meter

        // Define a static Box2D body at the gold's center
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        bodyDef.position = {centerX / PPM, centerY / PPM};

        b2BodyId bodyId = b2CreateBody(gWorld, &bodyDef);

        // Create a circle shape for the gold body
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.material.friction = 0.3f;
        shapeDef.material.restitution = 0.1f;
        shapeDef.filter.categoryBits = 0x0001;
        shapeDef.filter.maskBits = 0xFFFF;

        b2Circle circle;
        circle.center = {0.0f, 0.0f};
        circle.radius = (width / 2.0f) / PPM;

        b2CreateCircleShape(bodyId, &shapeDef, &circle);

        // Attach entity ID to body for reference if needed
        b2Body_SetUserData(bodyId, new bagel::ent_type{e.entity()});

        // Add ECS components to the entity
        e.addAll(
                Position{x, y},
                Renderable{SPRITE_GOLD},
                Collectable{},
                ItemType{ItemType::Type::Gold},
                Value{100},
                Weight{1.0f},
                Collidable{},
                PlayerInfo{-1},
                PhysicsBody{bodyId}
        );

        return e.entity().id;
    }


/**
 * @brief Creates a rock entity using a 6-vertex polygon to match its visual shape.
 *
 * This function generates a static physics body shaped like the visible rock sprite.
 * The hitbox uses 6 vertices to closely follow the stone's curved silhouette,
 * allowing more accurate collision detection than a simple rectangle.
 *
 * Components added:
 * - Position: screen-space top-left pixel coordinates
 * - Renderable: uses SPRITE_ROCK
 * - Collidable: for collision detection
 * - PhysicsBody: holds the Box2D body
 * - ItemType: set to Rock
 * - Value, Weight, PlayerInfo: standard properties
 *
 * @param x The top-left x position on screen in pixels.
 * @param y The top-left y position on screen in pixels.
 * @return The ID of the created entity.
 */
    id_type CreateRock(float x, float y) {
        Entity e = Entity::create();

        SDL_Rect rect = GetSpriteSrcRect(SPRITE_ROCK);
        float width = static_cast<float>(rect.w);
        float height = static_cast<float>(rect.h);

        float centerX = x + width / 2.0f;
        float centerY = y + height / 2.0f;

        constexpr float PPM = 50.0f; // Pixels per meter
        float hw = width / 2.0f / PPM;
        float hh = height / 2.0f / PPM;

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        bodyDef.position = {centerX / PPM, centerY / PPM};

        b2BodyId bodyId = b2CreateBody(gWorld, &bodyDef);

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.material.friction = 0.6f;
        shapeDef.material.restitution = 0.1f;
        shapeDef.filter.categoryBits = 0x0001;
        shapeDef.filter.maskBits = 0xFFFF;


        // Six-point polygon approximating the rock's shape
        b2Vec2 verts[6] = {
                { -hw * 0.6f, -hh * 0.8f }, // p0 - top-left
                { -hw * 0.9f,  0.0f       }, // p1 - far left
                { -hw * 0.5f,  hh * 0.9f  }, // p2 - bottom-left
                {  hw * 0.6f,  hh * 0.9f  }, // p3 - bottom-right
                {  hw * 0.9f,  0.0f       }, // p4 - far right
                {  hw * 0.4f, -hh * 0.8f  }  // p5 - top-right
        };

        b2Polygon shape = {};
        shape.count = 6;
        for (int i = 0; i < 6; ++i) {
            shape.vertices[i] = verts[i];
        }

        b2CreatePolygonShape(bodyId, &shapeDef, &shape);
        b2Body_SetUserData(bodyId, new bagel::ent_type{e.entity()});

        e.addAll(
                Position{x, y},
                Renderable{SPRITE_ROCK},
                Collectable{},
                ItemType{ItemType::Type::Rock},
                Value{10},
                Weight{3.0f},
                Collidable{},
                PlayerInfo{-1},
                PhysicsBody{bodyId}
        );

        return e.entity().id;
    }


/**
 * @brief Creates a diamond entity using a 6-vertex polygon matching the visual sprite shape.
 *
 * The diamond is represented as a static Box2D body shaped like a stylized gemstone:
 * flat top, tapered sides, and a pointed bottom.
 * This allows more accurate collision detection than a circle or rectangle.
 *
 * Components added:
 * - Position: top-left pixel coordinate (for rendering)
 * - Renderable: uses SPRITE_DIAMOND
 * - Collidable: for collision
 * - PhysicsBody: holds Box2D body ID
 * - ItemType: set to Diamond
 * - Value, Weight, PlayerInfo: game logic metadata
 *
 * @param x Top-left x pixel coordinate
 * @param y Top-left y pixel coordinate
 * @return ID of the created entity
 */
    id_type CreateDiamond(float x, float y) {
        Entity e = Entity::create();

        SDL_Rect rect = GetSpriteSrcRect(SPRITE_DIAMOND);
        float width = static_cast<float>(rect.w);  // ~41 px
        float height = static_cast<float>(rect.h); // ~32 px

        float centerX = x + width / 2.0f;
        float centerY = y + height / 2.0f;

        constexpr float PPM = 50.0f;
        float hw = width / 2.0f / PPM;
        float hh = height / 2.0f / PPM;

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        bodyDef.position = {centerX / PPM, centerY / PPM};

        b2BodyId bodyId = b2CreateBody(gWorld, &bodyDef);
        b2Body_EnableHitEvents(bodyId, true);
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.material.friction = 0.4f;
        shapeDef.material.restitution = 0.2f;
        shapeDef.filter.categoryBits = 0x0001;
        shapeDef.filter.maskBits = 0xFFFF;

        shapeDef.isSensor = true;
        shapeDef.enableHitEvents = true;

        // Six-point diamond shape
        b2Vec2 verts[6] = {
                { 0.0f,     -hh },         // top center
                { -hw,      -hh * 0.5f },  // top-left
                { -hw * 0.7f, hh * 0.1f }, // mid-left
                { 0.0f,      hh },         // bottom point
                { hw * 0.7f, hh * 0.1f },  // mid-right
                { hw,       -hh * 0.5f }   // top-right
        };

        b2Polygon shape = {};
        shape.count = 6;
        for (int i = 0; i < 6; ++i) {
            shape.vertices[i] = verts[i];
        }

        b2CreatePolygonShape(bodyId, &shapeDef, &shape);
        b2Body_SetUserData(bodyId, new bagel::ent_type{e.entity()});

        e.addAll(
                Position{x, y},
                Renderable{SPRITE_DIAMOND},
                Collectable{},
                ItemType{ItemType::Type::Diamond},
                Value{300},
                Weight{0.5f},
                Collidable{},
                PlayerInfo{-1},
                PhysicsBody{bodyId}
        );

        return e.entity().id;
    }

    /**
 * @brief Creates a treasure chest entity with scaled rendering and accurate physics shape.
 *
 * This function instantiates a static ECS entity representing a treasure chest.
 * The chest uses a scaled-down version of the original sprite (SCALE = 0.23)
 * so that it visually matches the size of other objects (like rocks or gold).
 *
 * The hitbox is defined as a 6-vertex polygon that closely matches the chest's visible outline.
 * No scaling is applied later in rendering or physics — all dimensions are pre-scaled here.
 *
 * Components added:
 * - Position: top-left pixel position (already scaled)
 * - Renderable: uses SPRITE_TREASURE_CHEST
 * - Collidable: participates in collision detection
 * - PhysicsBody: stores the Box2D body handle
 * - ItemType: set to TreasureChest
 * - Value, Weight, PlayerInfo: standard item properties
 *
 * @param x Top-left x position in pixels (screen coordinates)
 * @param y Top-left y position in pixels (screen coordinates)
 * @return ID of the created entity
 */

    id_type CreateTreasureChest(float x, float y) {
        Entity e = Entity::create();

        SDL_Rect rect = GetSpriteSrcRect(SPRITE_TREASURE_CHEST);
        float originalW = static_cast<float>(rect.w); // 356
        float originalH = static_cast<float>(rect.h); // 447

        constexpr float PPM = 50.0f;

        float scaledW = originalW;
        float scaledH = originalH   ;

        float centerX = x + scaledW / 2.0f;
        float centerY = y + scaledH / 2.0f;

        float hw = scaledW / 2.0f / PPM;
        float hh = scaledH / 2.0f / PPM;

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        bodyDef.position = {centerX / PPM, centerY / PPM};

        b2BodyId bodyId = b2CreateBody(gWorld, &bodyDef);

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.material.friction = 0.5f;
        shapeDef.material.restitution = 0.1f;
        shapeDef.filter.categoryBits = 0x0001;
        shapeDef.filter.maskBits = 0xFFFF;

        // Six-point polygon representing the chest outline (scaled)
        b2Vec2 verts[6] = {
                { -hw * 0.9f, -hh * 0.5f },   // top-left
                { -hw * 0.7f,  hh * 0.4f },   // bottom-left
                {  0.0f,       hh * 0.6f },   // center-bottom
                {  hw * 0.7f,  hh * 0.4f },   // bottom-right
                {  hw * 0.9f, -hh * 0.5f },   // top-right
                {  0.0f,      -hh * 0.8f }    // top-center arch
        };

        b2Polygon shape = {};
        shape.count = 6;
        for (int i = 0; i < 6; ++i) {
            shape.vertices[i] = verts[i];
        }

        b2CreatePolygonShape(bodyId, &shapeDef, &shape);
        b2Body_SetUserData(bodyId, new bagel::ent_type{e.entity()});

        // Final position stored already scaled
        e.addAll(
                Position{x, y},
                Renderable{SPRITE_TREASURE_CHEST},
                Collectable{},
                ItemType{ItemType::Type::TreasureChest},
                Value{0},
                Weight{1.0f},
                Collidable{},
                PlayerInfo{-1},
                PhysicsBody{bodyId}
        );

        return e.entity().id;
    }


    /**
 * @brief Creates a mystery bag item at the given coordinates.
 */
    id_type CreateMysteryBag(float x, float y) {
        Entity e = Entity::create();

        SDL_Rect rect = GetSpriteSrcRect(SPRITE_MYSTERY_BAG);
        float width = static_cast<float>(rect.w);
        float height = static_cast<float>(rect.h);

        float centerX = x + width / 2.0f;
        float centerY = y + height / 2.0f;

        constexpr float PPM = 50.0f;
        float hw = width / 2.0f / PPM;
        float hh = height / 2.0f / PPM;

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        bodyDef.position = {centerX / PPM, centerY / PPM};

        b2BodyId bodyId = b2CreateBody(gWorld, &bodyDef);

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.material.friction = 0.4f;
        shapeDef.material.restitution = 0.2f;
        shapeDef.filter.categoryBits = 0x0001;
        shapeDef.filter.maskBits = 0xFFFF;

        // Five-point polygon that mimics the mystery sack
        b2Vec2 verts[5] = {
                { 0.0f, -hh * 0.9f },    // top (tie)
                { -hw * 0.8f, -hh * 0.3f }, // upper left
                { -hw, hh * 0.6f },        // bottom left
                { hw, hh * 0.6f },         // bottom right
                { hw * 0.8f, -hh * 0.3f }  // upper right
        };

        b2Polygon sackShape = {};
        sackShape.count = 5;
        for (int i = 0; i < 5; ++i) {
            sackShape.vertices[i] = verts[i];
        }

        b2CreatePolygonShape(bodyId, &shapeDef, &sackShape);
        b2Body_SetUserData(bodyId, new bagel::ent_type{e.entity()});

        e.addAll(
                Position{x, y},
                Renderable{SPRITE_MYSTERY_BAG},
                Collectable{},
                ItemType{ItemType::Type::MysteryBag},
                Value{0},
                Weight{1.0f},
                Collidable{},
                PlayerInfo{-1},
                PhysicsBody{bodyId}
        );

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
        e.addAll(Position{x, y}, Velocity{1.5f, 0.0f}, Renderable{5}, Mole{100.0f, true}, Collidable{});
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
 * @brief Detects and handles hit events between entities using Box2D's Hit system.
 *
 * This system listens to Box2D's hit events to identify when the rope
 * hits any item like diamond, gold, rock, etc. It uses user data to identify the ECS entities.
 *
 * Prerequisite: You must enable hit events on the relevant bodies using b2Body_EnableHitEvents().
 */
    void CollisionSystem() {
        std::cout << "\n[CollisionSystem] Checking Box2D hit events...\n";

        if (!b2World_IsValid(gWorld)){
            std::cerr << "[CollisionSystem] gWorld is null!\n";
            return;
        }

        b2ContactEvents events = b2World_GetContactEvents(gWorld);
        std::cout << "[CollisionSystem] hitCount = " << events.hitCount << "\n";

        if (events.hitCount == 0) {
            std::cout << "No hits detected by Box2D this frame.\n";
        }

        for (int i = 0; i < events.hitCount; ++i) {
            const b2ContactHitEvent &hit = events.hitEvents[i];

            b2ShapeId shapeA = hit.shapeIdA;
            b2ShapeId shapeB = hit.shapeIdB;

            b2BodyId bodyA = b2Shape_GetBody(shapeA);
            b2BodyId bodyB = b2Shape_GetBody(shapeB);

            auto *userDataA = static_cast<bagel::ent_type *>(b2Body_GetUserData(bodyA));
            auto *userDataB = static_cast<bagel::ent_type *>(b2Body_GetUserData(bodyB));

            if (!userDataA || !userDataB) {
                std::cout << "One of the entities has no user data.\n";
                continue;
            }

            bagel::ent_type entA = *userDataA;
            bagel::ent_type entB = *userDataB;

            std::cout << "Hit detected between Entity " << entA.id << " and Entity " << entB.id << std::endl;

            if (bagel::World::mask(entA).test(bagel::Component<Collectable>::Bit)) {
                std::cout << "Collectable A got hit!\n";
            }
            if (bagel::World::mask(entB).test(bagel::Component<Collectable>::Bit)) {
                std::cout << "Collectable B got hit!\n";
            }
        }
    }

    /**
 * @brief Debug system to detect collisions approximately by comparing positions.
 *
 * This system is useful when Box2D contact events are not working as expected.
 * It checks for overlapping rectangles (AABB) between entities that have
 * Position and Collidable components.
 *
 * It logs rope vs item collisions if their positions intersect.
 */
    void DebugCollisionSystem() {

        for (id_type a = 0; a <= World::maxId().id; ++a) {
            ent_type entA{a};
            if (!World::mask(entA).test(Component<Position>::Bit)) continue;
            if (!World::mask(entA).test(Component<Collidable>::Bit)) continue;

            const Position& posA = World::getComponent<Position>(entA);

            for (id_type b = a + 1; b <= World::maxId().id; ++b) {
                ent_type entB{b};
                if (!World::mask(entB).test(Component<Position>::Bit)) continue;
                if (!World::mask(entB).test(Component<Collidable>::Bit)) continue;

                const Position& posB = World::getComponent<Position>(entB);

                float sizeA = 20.0f;
                float sizeB = 20.0f;
                SDL_FRect rectA = {posA.x, posA.y, sizeA, sizeA};
                SDL_FRect rectB = {posB.x, posB.y, sizeB, sizeB};

                if (SDL_HasRectIntersectionFloat(&rectA, &rectB)) {
                    std::cout << "[DEBUG] Approximate collision: " << a << " vs " << b << std::endl;

                    bool aIsRope = World::mask(entA).test(Component<RoperTag>::Bit);
                    bool bIsItem = World::mask(entB).test(Component<ItemType>::Bit);
                    bool bIsRope = World::mask(entB).test(Component<RoperTag>::Bit);
                    bool aIsItem = World::mask(entA).test(Component<ItemType>::Bit);

                    if ((aIsRope && bIsItem) || (bIsRope && aIsItem)) {
                        std::cout << "Rope touched item! (by position)" << std::endl;
                    }
                }
            }
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
    void TreasureChestSystem() {
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
    void RenderSystem(SDL_Renderer* renderer) {
        using namespace bagel;
        using namespace goldminer;

        Mask mask;
        mask.set(Component<Renderable>::Bit);
        mask.set(Component<Position>::Bit);

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type ent{id};
            if (!World::mask(ent).test(mask)) continue;

            const Position& pos = World::getComponent<Position>(ent);
            const Renderable& render = World::getComponent<Renderable>(ent);

            if (render.spriteID < 0 || render.spriteID >= SPRITE_COUNT) continue;

            SDL_Rect rect = GetSpriteSrcRect(static_cast<SpriteID>(render.spriteID));
            SDL_Texture* texture = GetSpriteTexture(static_cast<SpriteID>(render.spriteID));

            SDL_FRect src = {
                static_cast<float>(rect.x),
                static_cast<float>(rect.y),
                static_cast<float>(rect.w),
                static_cast<float>(rect.h)
            };

            SDL_FRect dest = {
                pos.x,
                pos.y,
                src.w,
                src.h
            };

            SDL_RenderTexture(renderer, texture, &src, &dest);
        }
    }

    /**
 * @brief Draws rope lines for all rope entities.
 *
 * This system draws a black line between each rope and its associated player,
 * visually representing the rope without using textures or Renderable components.
 *
 * The line starts at the center of the player's sprite and ends at the rope's position.
 * It is useful for debugging or as a simple visual before adding full sprite animation.
 *
 * Requirements:
 * - Rope entity must have: RoperTag, Position, PlayerInfo.
 * - Player entity must have: Position, PlayerInfo (matching the rope).
 *
 * @param renderer The SDL renderer used for drawing.
 */
/**
 * @brief Draws rope lines for all rope entities using their Box2D position.
 *
 * This system draws a black line between the player's center and the Box2D body
 * of the rope. It avoids relying on the Position component, which may be offset
 * for sprite rendering purposes.
 *
 * Requirements:
 * - Rope entity must have: RoperTag, PhysicsBody, PlayerInfo.
 * - Player entity must have: Position, PlayerInfo.
 *
 * @param renderer The SDL renderer used for drawing.
 */
    void RopeRenderSystem(SDL_Renderer* renderer) {
        using namespace bagel;
        using namespace goldminer;

        constexpr float PPM = 50.0f;

        Mask ropeMask;
        ropeMask.set(Component<RoperTag>::Bit);
        ropeMask.set(Component<PhysicsBody>::Bit);
        ropeMask.set(Component<PlayerInfo>::Bit);

        Mask playerMask;
        playerMask.set(Component<Position>::Bit);
        playerMask.set(Component<PlayerInfo>::Bit);

        for (bagel::id_type id = 0; id <= bagel::World::maxId().id; ++id) {
            bagel::ent_type ent{id};

            if (bagel::World::mask(ent).test(bagel::Component<goldminer::RoperTag>::Bit) &&
                bagel::World::mask(ent).test(bagel::Component<goldminer::PhysicsBody>::Bit)) {
                const auto& phys = bagel::World::getComponent<goldminer::PhysicsBody>(ent);
                b2Transform tf = b2Body_GetTransform(phys.bodyId);
                std::cout << "ROPE at: " << tf.p.x * 50 << ", " << tf.p.y * 50 << std::endl;
            }

            if (bagel::World::mask(ent).test(bagel::Component<goldminer::ItemType>::Bit) &&
                bagel::World::mask(ent).test(bagel::Component<goldminer::PhysicsBody>::Bit)) {
                const auto& phys = bagel::World::getComponent<goldminer::PhysicsBody>(ent);
                b2Transform tf = b2Body_GetTransform(phys.bodyId);
                std::cout << "ITEM at: " << tf.p.x * 50 << ", " << tf.p.y * 50 << std::endl;
            }
        }

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type rope{id};
            if (!World::mask(rope).test(ropeMask)) continue;

            const PhysicsBody& phys = World::getComponent<PhysicsBody>(rope);
            const PlayerInfo& ropeOwner = World::getComponent<PlayerInfo>(rope);

            if (!b2Body_IsValid(phys.bodyId)) continue;

            b2Transform tf = b2Body_GetTransform(phys.bodyId);
            SDL_FPoint ropeTip = {
                    tf.p.x * PPM,
                    tf.p.y * PPM
            };

            // Find the matching player
            for (id_type pid = 0; pid <= World::maxId().id; ++pid) {
                ent_type player{pid};
                if (!World::mask(player).test(playerMask)) continue;

                const PlayerInfo& playerInfo = World::getComponent<PlayerInfo>(player);
                if (playerInfo.playerID != ropeOwner.playerID) continue;

                const Position& playerPos = World::getComponent<Position>(player);

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderLine(renderer,
                               playerPos.x + 80, playerPos.y + 80,  // Approx. center of player
                               ropeTip.x, ropeTip.y);               // From Box2D rope center

                break;
            }
        }
    }

    /**
 * @brief Returns the visual center offset of a sprite based on its ID.
 *
 * Given a sprite ID, this function retrieves its source rectangle and computes
 * half of its width and height (in pixels), scaled as needed.
 * This is used to convert from Box2D center-based coordinates to SDL top-left positioning.
 *
 * @param spriteID The sprite ID from the Renderable component.
 * @return SDL_FPoint containing {half width, half height} of the sprite.
 */
    SDL_FPoint GetSpriteOffset(int spriteID) {
        SDL_Rect rect = GetSpriteSrcRect(static_cast<SpriteID>(spriteID));
        float width = static_cast<float>(rect.w);
        float height = static_cast<float>(rect.h);

        constexpr float scale = 1.0f; // Adjust this if rendering uses a scale

        return SDL_FPoint{
                (width * scale) / 2.0f,
                (height * scale) / 2.0f
        };
    }

/**
 * @brief Synchronizes ECS Position components with their Box2D physics bodies.
 *
 * This system updates the Position (in pixels, top-left) of entities that have
 * both PhysicsBody and Renderable components. It uses the Box2D transform (center-based)
 * and applies an offset based on the sprite's size to align rendering with SDL.
 *
 * Requirements:
 * - Components: PhysicsBody, Position, Renderable
 *
 * Notes:
 * - Assumes PIXELS_PER_METER is defined globally.
 * - This system is essential for aligning sprite rendering with physics movement.
 */
    void PhysicsSyncSystem() {
        using namespace bagel;

        constexpr float PIXELS_PER_METER = 50.0f;

        Mask mask;
        mask.set(Component<PhysicsBody>::Bit);
        mask.set(Component<Position>::Bit);
        mask.set(Component<Renderable>::Bit);

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type ent{id};
            if (!World::mask(ent).test(mask)) continue;

            auto& phys = World::getComponent<PhysicsBody>(ent);
            auto& pos = World::getComponent<Position>(ent);
            const auto& render = World::getComponent<Renderable>(ent);

            if (!b2Body_IsValid(phys.bodyId)) continue;

            b2Transform transform = b2Body_GetTransform(phys.bodyId);
            SDL_FPoint offset = GetSpriteOffset(render.spriteID);

            pos.x = transform.p.x * PIXELS_PER_METER - offset.x;
            pos.y = transform.p.y * PIXELS_PER_METER - offset.y;
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
