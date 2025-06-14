/**
 * @file gold_miner_ecs.cpp
 * @brief Implementation of entity creation functions and systems using BAGEL ECS for Gold Miner game.
 */
#include "gold_miner_ecs.h"
#include "bagel.h"
#include "sprite_manager.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <cmath>
#include <iostream>
#include "debug_draw.h"
#include <unordered_map>
#include <vector>


namespace goldminer {
    b2WorldId gWorld = b2_nullWorldId;

    using namespace bagel;

    void initBox2DWorld () {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = { 0.0f, 9.8f };
        gWorld = b2CreateWorld(&worldDef);
        b2World_SetHitEventThreshold(gWorld, 0.0001f);

        // This line is correct and should be called once in initBox2DWorld
        // to associate the world with your debug draw instance.
        // The InitDebugDraw(renderer) call should be in main.cpp after renderer creation.
        b2World_Draw(gWorld, &gDebugDraw);
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

        // Find player position
        Position playerPos;
        bool foundPlayer = false;

        Mask playerMask;
        playerMask.set(Component<Position>::Bit);
        playerMask.set(Component<PlayerInfo>::Bit);

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type ent{id};
            if (!World::mask(ent).test(playerMask)) continue;

            const auto& pinfo = World::getComponent<PlayerInfo>(ent);
            if (pinfo.playerID == playerID) {
                playerPos = World::getComponent<Position>(ent);
                foundPlayer = true;
                break;
            }
        }

        if (!foundPlayer) {
            std::cerr << "[CreateRope] ERROR: Could not find player " << playerID << " to attach rope!\n";
            return -1;
        }

        // Get player sprite size
        SDL_Rect rect = GetSpriteSrcRect(SPRITE_PLAYER_IDLE);
        float playerWidth = rect.w;
        float playerHeight = rect.h;

        // Compute winch offset visually
        float winchOffsetX = playerWidth * 0.001f;; // adjust this visually!
        float winchOffsetY = playerHeight*1.1;                     // adjust this visually!

        // Compute rope start position
        float startX = playerPos.x - winchOffsetX;
        float startY = playerPos.y + winchOffsetY;

        constexpr float PPM = 50.0f;
        float centerX = startX;
        float centerY = startY;

        // Create dynamic Box2D body
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.fixedRotation = false;
        bodyDef.position = {centerX / PPM, centerY / PPM};
        bodyDef.isBullet = true;
        b2BodyId bodyId = b2CreateBody(gWorld, &bodyDef);
        b2Body_EnableHitEvents(bodyId, true);

        // Circle shape
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.material.friction = 0.5f;
        shapeDef.material.restitution = 0.2f;
        shapeDef.enableHitEvents = true;
        shapeDef.isSensor = false;

        b2Circle circle;
        circle.center = {0.0f, 0.0f};
        circle.radius = 0.3f;

        b2CreateCircleShape(bodyId, &shapeDef, &circle);
        b2Body_SetLinearVelocity(bodyId, {0.0f, 0.0f}); // No initial motion
        b2Body_SetUserData(bodyId, new ent_type{e.entity()});

        e.addAll(
                Position{startX, startY},
                Rotation{0.0f},
                Length{0.0f},
                RopeControl{},
                RoperTag{},
                PlayerInfo{playerID},
                Collidable{},
                PhysicsBody{bodyId}
        );

        std::cout << "[CreateRope] Rope created at (" << startX << ", " << startY << ")\n";

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
                Value{70},
                Weight{5.0f},
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

        // Get sprite dimensions for gold
        SDL_Rect rect = GetSpriteSrcRect(SPRITE_ROCK);
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
                Renderable{SPRITE_ROCK},
                Collectable{},
                ItemType{ItemType::Type::Rock},
                Value{100},
                Weight{1.0f},
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

        // Get sprite dimensions for gold
        SDL_Rect rect = GetSpriteSrcRect(SPRITE_DIAMOND);
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
                Renderable{SPRITE_DIAMOND},
                Collectable{},
                ItemType{ItemType::Type::Diamond},
                Value{100},
                Weight{1.0f},
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

        // Get sprite dimensions for gold
        SDL_Rect rect = GetSpriteSrcRect(SPRITE_TREASURE_CHEST);
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
                Renderable{SPRITE_TREASURE_CHEST},
                Collectable{},
                ItemType{ItemType::Type::TreasureChest},
                Value{100},
                Weight{3.0f},
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
    void PlayerInputSystem(const SDL_Event* event) {
        if (!event) return;

        bool spacePressed = false;

        if (event->type == SDL_EVENT_KEY_DOWN) {
            if (event->key.key == SDLK_SPACE) {
                spacePressed = true;
                std::cout << "[PlayerInputSystem] Space key pressed, sending rope command.\n";
            }
        }

        Mask mask;
        mask.set(Component<PlayerInput>::Bit);

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type ent{id};
            if (!World::mask(ent).test(mask)) continue;

            auto& input = World::getComponent<PlayerInput>(ent);

            input.sendRope = spacePressed;
        }
    }
    /**
     * @brief Oscillates rope entities that are currently at rest.
     */
    void RopeSwingSystem() {
        static std::unordered_map<id_type, float> swingDirections;

        Mask ropeMask;
        ropeMask.set(Component<RoperTag>::Bit);
        ropeMask.set(Component<Rotation>::Bit);
        ropeMask.set(Component<RopeControl>::Bit);
        ropeMask.set(Component<PhysicsBody>::Bit);
        ropeMask.set(Component<PlayerInfo>::Bit);

        Mask playerMask;
        playerMask.set(Component<Position>::Bit);
        playerMask.set(Component<PlayerInfo>::Bit);

        const float maxSwingAngle = 75.0f; // Bigger swing range → looks better
        const float swingSpeed = 90.0f;    // degrees per second → faster swing
        const float deltaTime = 1.0f / 60.0f; // assuming ~60 FPS fixed timestep

        constexpr float PPM = 50.0f;
        constexpr float ropeLength = 80.0f; // rope visible length → tune visually

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type rope{id};
            if (!World::mask(rope).test(ropeMask)) continue;

            auto& rotation = World::getComponent<Rotation>(rope);
            auto& ropeControl = World::getComponent<RopeControl>(rope);
            auto& phys = World::getComponent<PhysicsBody>(rope);
            auto& ropePlayerInfo = World::getComponent<PlayerInfo>(rope);

            if (ropeControl.state == RopeControl::State::AtRest) {
                // Initialize swing direction if first time
                if (swingDirections.find(id) == swingDirections.end()) {
                    swingDirections[id] = 1.0f;
                }

                // Update angle
                rotation.angle += swingDirections[id] * swingSpeed * deltaTime;

                // Clamp angle and reverse direction
                if (rotation.angle > maxSwingAngle) {
                    rotation.angle = maxSwingAngle;
                    swingDirections[id] = -1.0f;
                } else if (rotation.angle < -maxSwingAngle) {
                    rotation.angle = -maxSwingAngle;
                    swingDirections[id] = 1.0f;
                }

                // Find matching player
                Position playerPos{};
                bool foundPlayer = false;

                for (id_type pid = 0; pid <= World::maxId().id; ++pid) {
                    ent_type player{pid};
                    if (!World::mask(player).test(playerMask)) continue;

                    const auto& playerInfo = World::getComponent<PlayerInfo>(player);
                    if (playerInfo.playerID == ropePlayerInfo.playerID) {
                        playerPos = World::getComponent<Position>(player);
                        foundPlayer = true;
                        break;
                    }
                }

                if (!foundPlayer) {
                    std::cerr << "[RopeSwingSystem] ERROR: Could not find player for rope " << id << "\n";
                    continue;
                }

                // Use the same winch offset as your current CreateRope()
                SDL_Rect rect = GetSpriteSrcRect(SPRITE_PLAYER_IDLE);
                float playerWidth = rect.w;
                float playerHeight = rect.h;

                float winchOffsetX = -playerWidth * 0.001f;
                float winchOffsetY = playerHeight * 1.1f;

                // Starting point of the rope
                float originX = playerPos.x + winchOffsetX;
                float originY = playerPos.y + winchOffsetY;

                // Compute tip position based on swing angle (downwards)
                float angleRad = rotation.angle * (M_PI / 180.0f);

                float tipX = originX + ropeLength * sin(angleRad);
                float tipY = originY + ropeLength * cos(angleRad);

                // Update rope body position to match swing tip
                b2Transform tf = b2Body_GetTransform(phys.bodyId);
                tf.p.x = tipX / PPM;
                tf.p.y = tipY / PPM;
                b2Body_SetTransform(phys.bodyId, tf.p, tf.q);

                // Disable gravity while swinging
                b2Body_SetLinearVelocity(phys.bodyId, {0.0f, 0.0f});
                b2Body_SetGravityScale(phys.bodyId, 0.0f);

                // Debug print
                std::cout << "[RopeSwingSystem] Rope " << id
                          << " angle=" << rotation.angle
                          << " tip=(" << tipX << ", " << tipY << ")\n";
            }
            else {
                // Not at rest → allow gravity
                b2Body_SetGravityScale(phys.bodyId, 1.0f);
            }
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
        mask.set(Component<PhysicsBody>::Bit);

        constexpr float MAX_LENGTH = 800.0f;
        constexpr float EXTENSION_SPEED = 600.0f; // pixels/sec
        constexpr float RETRACTION_SPEED = 900.0f;
        constexpr float PPM = 50.0f;
        const float deltaTime = 1.0f / 60.0f;

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type rope{id};
            if (!World::mask(rope).test(mask)) continue;

            auto& ropeControl = World::getComponent<RopeControl>(rope);
            auto& length = World::getComponent<Length>(rope);
            auto& rotation = World::getComponent<Rotation>(rope);
            auto& phys = World::getComponent<PhysicsBody>(rope);
            auto& ropeOwner = World::getComponent<PlayerInfo>(rope);

            // Find player position
            Position playerPos{};
            bool foundPlayer = false;
            Mask playerMask;
            playerMask.set(Component<Position>::Bit);
            playerMask.set(Component<PlayerInfo>::Bit);
            ent_type playerEntity = {};
            for (id_type pid = 0; pid <= World::maxId().id; ++pid) {
                ent_type player{pid};
                if (!World::mask(player).test(playerMask)) continue;
                const auto& pinfo = World::getComponent<PlayerInfo>(player);
                if (pinfo.playerID == ropeOwner.playerID) {
                    playerPos = World::getComponent<Position>(player);
                    playerEntity = player;
                    foundPlayer = true;
                    break;
                }
            }
            if (!foundPlayer) continue;

            // Handle input: if at rest and Enter pressed, start extending
            auto& input = World::getComponent<PlayerInput>(playerEntity);
            if (ropeControl.state == RopeControl::State::AtRest && input.sendRope) {
                ropeControl.state = RopeControl::State::Extending;
                input.sendRope = false; // consume input
            }

            // Calculate rope origin (winch)
            SDL_Rect rect = GetSpriteSrcRect(SPRITE_PLAYER_IDLE);
            float winchOffsetX = -rect.w * 0.001f;
            float winchOffsetY = rect.h * 1.1f;
            float originX = playerPos.x + winchOffsetX;
            float originY = playerPos.y + winchOffsetY;

            float angleRad = rotation.angle * (M_PI / 180.0f);

            // Calculate target tip position
            float tipX = originX + length.value * sin(angleRad);
            float tipY = originY + length.value * cos(angleRad);
            b2Vec2 currentPos = b2Body_GetPosition(phys.bodyId);
            b2Vec2 targetPos = { tipX / PPM, tipY / PPM };
            b2Vec2 direction = { targetPos.x - currentPos.x, targetPos.y - currentPos.y };
            float dist = sqrt(direction.x * direction.x + direction.y * direction.y);


            if (ropeControl.state == RopeControl::State::Extending) {
                length.value += EXTENSION_SPEED * deltaTime;
                if (length.value > MAX_LENGTH) {
                    length.value = MAX_LENGTH;
                    ropeControl.state = RopeControl::State::Retracting;
                }
                // Set velocity towards the target
                if (dist > 0.01f) {
                    direction.x *= EXTENSION_SPEED / PPM / dist;
                    direction.y *= EXTENSION_SPEED / PPM / dist;
                    b2Body_SetLinearVelocity(phys.bodyId, direction);
                } else {
                    b2Body_SetLinearVelocity(phys.bodyId, {0, 0});
                }
            }
            else if (ropeControl.state == RopeControl::State::Retracting) {
                length.value -= RETRACTION_SPEED * deltaTime;
                if (length.value <= 0.0f) {
                    length.value = 0.0f;
                    ropeControl.state = RopeControl::State::AtRest;
                    b2Body_SetLinearVelocity(phys.bodyId, {0, 0});
                } else {
                    // Set velocity back toward the origin
                    b2Vec2 retractTarget = { originX / PPM, originY / PPM };
                    b2Vec2 retractDir = { retractTarget.x - currentPos.x, retractTarget.y - currentPos.y };
                    float retractDist = sqrt(retractDir.x * retractDir.x + retractDir.y * retractDir.y);
                    if (retractDist > 0.01f) {
                        retractDir.x *= RETRACTION_SPEED / PPM / retractDist;
                        retractDir.y *= RETRACTION_SPEED / PPM / retractDist;
                        b2Body_SetLinearVelocity(phys.bodyId, retractDir);
                    } else {
                        b2Body_SetLinearVelocity(phys.bodyId, {0, 0});
                    }
                }
            }
            else if (ropeControl.state == RopeControl::State::AtRest) {
                // If at rest, just keep the body in place
                b2Body_SetLinearVelocity(phys.bodyId, {0.0f, 0.0f});
            }

            // --- Joint cleanup when rope is at rest ---
            if (ropeControl.state == RopeControl::State::AtRest & World::mask(rope).test(Component<GrabbedJoint>::Bit)) {
                HandleRopeJointCleanup(rope);

                // Reset rope physics after releasing item
                b2Body_SetLinearVelocity(phys.bodyId, {0.0f, 0.0f});
                b2Body_SetAngularVelocity(phys.bodyId, 0.0f);
                b2Body_SetGravityScale(phys.bodyId, 0.0f); // or 1.0f if you want gravity at rest

            }
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
            b2BodyId bodyA = b2Shape_GetBody(hit.shapeIdA);
            b2BodyId bodyB = b2Shape_GetBody(hit.shapeIdB);

            auto *userDataA = static_cast<bagel::ent_type *>(b2Body_GetUserData(bodyA));
            auto *userDataB = static_cast<bagel::ent_type *>(b2Body_GetUserData(bodyB));
            if (!userDataA || !userDataB) {
                std::cout << "One of the entities has no user data.\n";
                continue;
            }

            ent_type entA = *userDataA;
            ent_type entB = *userDataB;
            std::cout << "Hit detected between Entity " << entA.id << " and Entity " << entB.id << std::endl;

            // Rope vs Collectable
            bool isRopeA = World::mask(entA).test(Component<RoperTag>::Bit);
            bool isRopeB = World::mask(entB).test(Component<RoperTag>::Bit);
            bool isCollectA = World::mask(entA).test(Component<Collectable>::Bit);
            bool isCollectB = World::mask(entB).test(Component<Collectable>::Bit);

            // Only grab if rope is not already holding something
            if (isRopeA && isCollectB && !World::mask(entA).test(Component<GrabbedJoint>::Bit)) {
                TryAttachCollectable(entA, entB);
            }
            else if (isRopeB && isCollectA && !World::mask(entB).test(Component<GrabbedJoint>::Bit)) {
                TryAttachCollectable(entB, entA);
            }
        }
    }

    void TryAttachCollectable(ent_type rope, ent_type collectable) {
        if (World::mask(rope).test(Component<GrabbedJoint>::Bit)) return;

        auto& ropePhys = World::getComponent<PhysicsBody>(rope);
        auto& itemPhys = World::getComponent<PhysicsBody>(collectable);

        b2Body_SetType(itemPhys.bodyId, b2_dynamicBody);

        b2WeldJointDef jointDef = b2DefaultWeldJointDef();
        jointDef.bodyIdA = ropePhys.bodyId;
        jointDef.bodyIdB = itemPhys.bodyId;
        jointDef.collideConnected = false;

        b2JointId jointId = b2CreateWeldJoint(goldminer::gWorld, &jointDef);
        World::addComponent<GrabbedJoint>(rope, GrabbedJoint{jointId, collectable.id});
        auto& ropeControl = World::getComponent<RopeControl>(rope);
        ropeControl.state = RopeControl::State::Retracting;

        //b2Body_SetAngularDamping(itemPhys.bodyId, 5.0f);
        b2Body_SetLinearVelocity(itemPhys.bodyId, {0, 0});
        b2Body_SetAngularVelocity(itemPhys.bodyId, 0);
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
                               playerPos.x +40 , playerPos.y + 120,  // Approx. center of player
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

    void DrawNumber(SDL_Renderer* renderer, int number, float x, float y) {
        constexpr float SCALE = 0.75f;
        std::string numStr = std::to_string(number);
        float offsetX = x;

        for (char c : numStr) {
            int digit = c - '0';
            SpriteID spriteID = static_cast<SpriteID>(SPRITE_DIGIT_0 + digit);

            SDL_Texture* tex = GetSpriteTexture(spriteID);
            SDL_Rect src = GetSpriteSrcRect(spriteID);
            SDL_FRect dst = {
                offsetX,
                y,
                src.w * SCALE,
                src.h * SCALE
            };
            SDL_FRect srcF = {(float)src.x, (float)src.y, (float)src.w, (float)src.h};

            SDL_RenderTexture(renderer, tex, &srcF, &dst);
            offsetX += dst.w + 2;
        }
    }




      /**
     * @brief Renders the score and remaining time for each player using digit sprites.
     *
     * This system loops through all UI entities (entities with UIComponent and PlayerInfo),
     * and for each player, it draws:
     * - The money icon + their current score
     * - The time icon + their remaining time
     *
     * Each player's data is matched by their PlayerInfo.playerID field.
     * The score and timer are drawn using digit sprites (SPRITE_DIGIT_0 to SPRITE_DIGIT_9),
     * instead of dynamic fonts.
     *
     * Sprite assets used:
     * - SPRITE_TITLE_MONEY: Icon shown before the score
     * - SPRITE_TITLE_TIME: Icon shown before the timer
     * - SPRITE_DIGIT_0 ... SPRITE_DIGIT_9: Used to render numeric values
     *
     * Expected components:
     * - UI entities: UIComponent, PlayerInfo
     * - Score entities: Score, PlayerInfo
     * - Timer entities: GameTimer, PlayerInfo
     *
     * @param renderer Pointer to the SDL_Renderer used for rendering
     */
    void UISystem(SDL_Renderer* renderer) {
        using namespace bagel;
        using namespace goldminer;

        constexpr float UI_BASE_Y = 4.0f;
        constexpr float PLAYER_UI_SPACING_X = 10.0f;//לשנות את הערך שנוסיף עוד שחקן
        constexpr float ICON_SPACING = 10.0f;
        //constexpr float NUMBER_Y_OFFSET = 4.0f;


        Mask uiMask;
        uiMask.set(Component<UIComponent>::Bit);
        uiMask.set(Component<PlayerInfo>::Bit);

        Mask scoreMask;
        scoreMask.set(Component<Score>::Bit);
        scoreMask.set(Component<PlayerInfo>::Bit);

        Mask timerMask;
        timerMask.set(Component<GameTimer>::Bit);
        timerMask.set(Component<PlayerInfo>::Bit);

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type uiEnt{id};
            if (!World::mask(uiEnt).test(uiMask)) continue;

            const PlayerInfo& uiPlayer = World::getComponent<PlayerInfo>(uiEnt);
            int pid = uiPlayer.playerID;

            float offsetX = 5.0f + pid * PLAYER_UI_SPACING_X;

            // === Score ===
            SDL_Texture* moneyIcon = GetSpriteTexture(SPRITE_TITLE_MONEY);
            SDL_Rect moneySrc = GetSpriteSrcRect(SPRITE_TITLE_MONEY);
            SDL_FRect moneyDst = {offsetX, UI_BASE_Y, (float)moneySrc.w, (float)moneySrc.h};
            SDL_FRect moneySrcF = {(float)moneySrc.x, (float)moneySrc.y, (float)moneySrc.w, (float)moneySrc.h};
            SDL_RenderTexture(renderer, moneyIcon, &moneySrcF, &moneyDst);

            for (id_type sid = 0; sid <= World::maxId().id; ++sid) {
                ent_type scoreEnt{sid};
                if (!World::mask(scoreEnt).test(scoreMask)) continue;

                const PlayerInfo& scorePlayer = World::getComponent<PlayerInfo>(scoreEnt);
                if (scorePlayer.playerID != pid) continue;

                const Score& score = World::getComponent<Score>(scoreEnt);
                DrawNumber(renderer, score.points, moneyDst.x + moneyDst.w + ICON_SPACING, moneyDst.y);

                break;
            }

            // === Time ===
            SDL_Texture* timeIcon = GetSpriteTexture(SPRITE_TITLE_TIME);
            SDL_Rect timeSrc = GetSpriteSrcRect(SPRITE_TITLE_TIME);
            SDL_FRect timeDst = {offsetX, UI_BASE_Y + 60.0f, (float)timeSrc.w, (float)timeSrc.h};
            SDL_FRect timeSrcF = {(float)timeSrc.x, (float)timeSrc.y, (float)timeSrc.w, (float)timeSrc.h};
            SDL_RenderTexture(renderer, timeIcon, &timeSrcF, &timeDst);

            for (id_type tid = 0; tid <= World::maxId().id; ++tid) {
                ent_type timerEnt{tid};
                if (!World::mask(timerEnt).test(timerMask)) continue;

                const PlayerInfo& timerPlayer = World::getComponent<PlayerInfo>(timerEnt);
                if (timerPlayer.playerID != pid) continue;

                const GameTimer& timer = World::getComponent<GameTimer>(timerEnt);
                int seconds = (int)std::ceil(timer.timeLeft);
                DrawNumber(renderer, seconds, timeDst.x + timeDst.w + ICON_SPACING, timeDst.y );


                break;
            }
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

    void Box2DDebugRenderSystem(SDL_Renderer* renderer) {
        b2World_Draw(gWorld, &gDebugDraw);
    }


    void DestructionSystem() {
        Mask req = MaskBuilder{}.set<DestroyTag>().build();

        // // Clean up Box2D body and user data if present
        // if (World::mask(ent).test(Component<PhysicsBody>::Bit)) {
        //     auto& phys = World::getComponent<PhysicsBody>(ent);
        //     if (b2Body_IsValid(phys.bodyId)) {
        //         void* userData = b2Body_GetUserData(phys.bodyId);
        //         if (userData) delete static_cast<bagel::ent_type*>(userData);
        //         b2DestroyBody(phys.bodyId);
        //         phys.bodyId = b2_nullBodyId;
        //     }
        // }

        std::vector<ent_type> toDelete;
        toDelete.reserve(PackedStorage<DestroyTag>::size());

        for (std::size_t i = 0; i < PackedStorage<DestroyTag>::size(); ++i) {
            ent_type e = PackedStorage<DestroyTag>::entity(i);
            if (World::mask(e).test(req)) {
                toDelete.push_back(e);
            }
        }

        for (ent_type e : toDelete) {
            std::cout << "[DestructionSystem] Destroying entity " << e.id << "\n";
            if (World::mask(e).test(Component<Position>::Bit)) World::delComponent<Position>(e);
            if (World::mask(e).test(Component<Velocity>::Bit)) World::delComponent<Velocity>(e);
            if (World::mask(e).test(Component<Rotation>::Bit)) World::delComponent<Rotation>(e);
            if (World::mask(e).test(Component<Length>::Bit)) World::delComponent<Length>(e);
            if (World::mask(e).test(Component<Renderable>::Bit)) World::delComponent<Renderable>(e);
            if (World::mask(e).test(Component<PlayerInfo>::Bit)) World::delComponent<PlayerInfo>(e);
            if (World::mask(e).test(Component<RopeControl>::Bit)) World::delComponent<RopeControl>(e);
            if (World::mask(e).test(Component<ItemType>::Bit)) World::delComponent<ItemType>(e);
            if (World::mask(e).test(Component<Value>::Bit)) World::delComponent<Value>(e);
            if (World::mask(e).test(Component<Weight>::Bit)) World::delComponent<Weight>(e);
            if (World::mask(e).test(Component<GrabbedJoint>::Bit)) World::delComponent<GrabbedJoint>(e);
            if (World::mask(e).test(Component<PhysicsBody>::Bit)) World::delComponent<PhysicsBody>(e);
            if (World::mask(e).test(Component<PlayerInput>::Bit)) World::delComponent<PlayerInput>(e);
            if (World::mask(e).test(Component<Collectable>::Bit)) World::delComponent<Collectable>(e);
            if (World::mask(e).test(Component<RoperTag>::Bit)) World::delComponent<RoperTag>(e);
            if (World::mask(e).test(Component<GameOverTag>::Bit)) World::delComponent<GameOverTag>(e);
            if (World::mask(e).test(Component<Collidable>::Bit)) World::delComponent<Collidable>(e);
            if (World::mask(e).test(Component<DestroyTag>::Bit)) World::delComponent<DestroyTag>(e);
        }

    }

    // --- Helper Implementations ---


    // Helper: Cleans up rope joint and deletes the attached collectable (if any)
    void HandleRopeJointCleanup(bagel::ent_type rope) {
        using namespace bagel;
        if (!World::mask(rope).test(Component<GrabbedJoint>::Bit)) return;

        auto& grabbed = World::getComponent<GrabbedJoint>(rope);
        b2DestroyJoint(grabbed.joint);
        World::delComponent<GrabbedJoint>(rope);
        ent_type item{grabbed.attachedEntityId};
        World::addComponent<DestroyTag>(item, {});
    }


} // namespace goldminer
