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
    int player_id = 0; // Default to 0 = no winner / tie
    bool game_over = false;
    SpriteID player1Sprite = SPRITE_PLAYER_IDLE;
    SpriteID player2Sprite = SPRITE_PLAYER_IDLE;


    using namespace bagel;

    /**
     * @brief Initializes the global Box2D physics world.
     *
     * This function sets up the Box2D world with Earth-like gravity and a minimal hit event threshold
     * to ensure precise collision detection. It also binds the debug draw instance to the world for visual debugging.
     *
     * Note: The `InitDebugDraw(renderer)` function should be called separately from `main.cpp`, after creating the SDL_Renderer.
     */
    void initBox2DWorld () {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = { 0.0f, 9.8f };
        gWorld = b2CreateWorld(&worldDef);
        b2World_SetHitEventThreshold(gWorld, 0.0001f);
        b2World_Draw(gWorld, &gDebugDraw);
    }


    //----------------------------------
    /// @section Entity Creation Functions
    //----------------------------------

        /**
    * @brief Creates a new player entity positioned inside their blue arch area.
    * Each player stands in the center of their screen half.
    *
    * @param playerID The identifier of the player (1 or 2).
    * @param sprite The sprite to use for this player.
    * @return The ID of the created entity.
    */
    id_type CreatePlayer(int playerID, SpriteID sprite, float time) {
        Entity e = Entity::create();

        // Calculate position based on screen half (each half is 640px wide)
        float startX = (playerID == 1) ? 310.0f : 785.0f; // Center of blue arch
        float startY = 45.0f;

        e.addAll(
            Position{startX, startY},
            Velocity{},
            Renderable{sprite},
            PlayerInfo{playerID},
            Score{0},
            GameTimer{time},
            PlayerInput{},
            PlayerTag{}
        );

        return e.entity().id;
    }

    /**
     * @brief Creates a rope entity with a Box2D dynamic body for physics-based interaction.
     *
     * This function locates the player by ID, determines the starting position of the rope
     * relative to the player sprite, and creates a dynamic Box2D body with a circular shape.
     * The rope entity is then initialized with multiple ECS components, enabling it to be rendered,
     * controlled, and detected in collisions during gameplay.
     *
     * Rope body characteristics:
     * - Type: Dynamic (affected by physics)
     * - Shape: Small circle
     * - Properties: Bullet, sensor disabled, hit events enabled
     *
     * Components added:
     * - Position: Starting top-left corner (for rendering)
     * - Rotation: Initialized to 0 (may be used for swinging logic)
     * - Length: Logical rope extension length
     * - RopeControl: Allows logic to control rope state
     * - RoperTag: Tag to identify rope entities
     * - Collidable: Enables collision detection with collectable items
     * - PlayerInfo: Associates rope with a specific player
     * - PhysicsBody: Box2D body handle for simulation
     *
     * @param playerID The ID of the player to whom the rope belongs
     * @return The ID of the created rope entity, or -1 if the player is not found
     */
    id_type CreateRope(int playerID) {
        Entity e = Entity::create();

        // Find player position
        Position playerPos;
        bool foundPlayer = false;

        Mask playerMask;
        playerMask.set(Component<Position>::Bit);
        playerMask.set(Component<PlayerInfo>::Bit);
        playerMask.set(Component<PlayerTag>::Bit);

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
     * @brief Creates a static gold entity with a physical body for collision detection.
     *
     * This function spawns a gold item at the specified coordinates and defines a Box2D static body
     * centered on the sprite. A circular collision shape is attached, allowing interaction with
     * other dynamic bodies such as the rope.
     *
     * Gold body characteristics:
     * - Type: Static (does not move)
     * - Shape: Circle (radius based on sprite width)
     * - Physical properties: friction and restitution set
     * - Collision filter: default category and mask
     *
     * Components added:
     * - Position: Top-left corner for rendering
     * - Renderable: Sprite ID for the gold image
     * - Collectable: Marks item as collectible
     * - ItemType: Set to gold type
     * - Value: Fixed value (e.g. 70)
     * - Weight: Physical weight used for rope pulling speed
     * - Collidable: Enables participation in collision checks
     * - PlayerInfo: Initially unclaimed (-1)
     * - PhysicsBody: Holds the Box2D body handle
     *
     * @param x X-coordinate (top-left) for placement
     * @param y Y-coordinate (top-left) for placement
     * @return The ID of the created gold entity
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
     * @brief Creates a static rock entity with a physical body for collision detection.
     *
     * This function spawns a rock item at the specified (x, y) position. It creates a static
     * Box2D body at the center of the sprite, using a circular collision shape to allow
     * interaction with dynamic objects such as the rope.
     *
     * Rock body characteristics:
     * - Type: Static (immovable)
     * - Shape: Circle (radius based on sprite width)
     * - Physical properties: moderate friction, low restitution
     * - Collision filter: default mask and category bits
     *
     * Components added:
     * - Position: Top-left corner of the sprite (used for rendering)
     * - Renderable: Sprite ID corresponding to rock
     * - Collectable: Marks the rock as collectible by the rope
     * - ItemType: Specifies the item type as Rock
     * - Value: Assigned value (e.g. 100 points)
     * - Weight: Used to influence rope pull-back speed (1.0f = light)
     * - Collidable: Enables detection in collision system
     * - PlayerInfo: Initially unassigned (-1)
     * - PhysicsBody: Stores the Box2D body handle for simulation
     *
     * @param x X-coordinate (top-left) of the rock sprite
     * @param y Y-coordinate (top-left) of the rock sprite
     * @return The entity ID of the created rock
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
                Value{30},
                Weight{1.0f},
                Collidable{},
                PlayerInfo{-1},
                PhysicsBody{bodyId}
        );

        return e.entity().id;
    }

    /**
     * @brief Creates a static diamond entity with a physical body for collision detection.
     *
     * This function places a diamond item in the game world at the specified top-left (x, y) position.
     * A static Box2D body is created at the center of the sprite with a circular collision shape,
     * allowing it to be detected by the rope.
     *
     * Diamond body characteristics:
     * - Type: Static (does not move)
     * - Shape: Circle (based on sprite width)
     * - Physical properties: standard friction and low bounce
     * - Collision filter: default configuration
     *
     * Components added:
     * - Position: Top-left corner (used for rendering)
     * - Renderable: Sprite ID for diamond graphics
     * - Collectable: Marks item as collectible
     * - ItemType: Set to Diamond
     * - Value: Assigned point value (e.g. 100)
     * - Weight: Used for rope pulling speed (1.0f = light)
     * - Collidable: Participates in collision system
     * - PlayerInfo: Initially unassigned (-1)
     * - PhysicsBody: Box2D handle for physics world
     *
     * @param x X-coordinate (top-left of sprite)
     * @param y Y-coordinate (top-left of sprite)
     * @return The ID of the created diamond entity
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
                Value{120},
                Weight{1.0f},
                Collidable{},
                PlayerInfo{-1},
                PhysicsBody{bodyId}
        );

        return e.entity().id;
    }

    /**
     * @brief Creates a static treasure chest entity with a random value and physical body.
     *
     * This function creates a treasure chest collectible at the given position, with a static
     * Box2D body for collision. The chest is given a random value from a fixed set, and rendered
     * using its associated sprite. The physical shape is a circle approximating the chest.
     *
     * Treasure chest body characteristics:
     * - Type: Static (immovable)
     * - Shape: Circle (based on sprite width)
     * - Physical properties: moderate friction, low restitution
     * - Collision filter: default mask and category
     *
     * Components added:
     * - Position: Top-left corner for rendering
     * - Renderable: Sprite ID for the treasure chest
     * - Collectable: Enables interaction with the rope
     * - ItemType: Type set to TreasureChest
     * - Value: Randomly chosen from {10, 20, 50, 70, 100}
     * - Weight: Affects rope pulling behavior (3.0f = heavy)
     * - Collidable: Enables collision system processing
     * - PlayerInfo: Initially unassigned (-1)
     * - PhysicsBody: Box2D body handle for simulation
     *
     * @param x X-coordinate (top-left of sprite)
     * @param y Y-coordinate (top-left of sprite)
     * @return The ID of the created treasure chest entity
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

        // Random value from predefined set
        const int options[] = {120, 20, 50, 70, 100};
        int randomIndex = rand() % 5;
        int randomValue = options[randomIndex];

        // Add ECS components to the entity
        e.addAll(
                Position{x, y},
                Renderable{SPRITE_TREASURE_CHEST},
                Collectable{},
                ItemType{ItemType::Type::TreasureChest},
                Value{randomValue},
                Weight{3.0f},
                Collidable{},
                PlayerInfo{-1},
                PhysicsBody{bodyId}
        );

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


    //----------------------------------
    /// @section System Implementations
    //----------------------------------


    /**
     * @brief Reads keyboard input and sets the rope command per player.
     *
     * Each player is assigned a unique key:
     * - Player 1 uses the SPACE key to send the rope.
     * - Player 2 uses the RETURN (Enter) key to send the rope.
     *
     * Input is ignored for a player whose timer has reached 0.
     *
     * @param event Pointer to SDL_Event (keyboard event)
     */
    void PlayerInputSystem(const SDL_Event* event) {
        if (!event) return;

        // Track key presses
        bool spacePressed = false;
        bool enterPressed = false;

        if (event->type == SDL_EVENT_KEY_DOWN) {
            if (event->key.key == SDLK_SPACE) {
                spacePressed = true;
                std::cout << "[PlayerInputSystem] SPACE pressed (Player 1)\n";
            }
            if (event->key.key == SDLK_RETURN) {
                enterPressed = true;
                std::cout << "[PlayerInputSystem] RETURN pressed (Player 2)\n";
            }
        }

        Mask mask;
        mask.set(Component<PlayerInput>::Bit);
        mask.set(Component<PlayerInfo>::Bit);

        Mask timerMask;
        timerMask.set(Component<GameTimer>::Bit);
        timerMask.set(Component<PlayerInfo>::Bit);

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type ent{id};
            if (!World::mask(ent).test(mask)) continue;

            auto& input = World::getComponent<PlayerInput>(ent);
            const auto& player = World::getComponent<PlayerInfo>(ent);
            int pid = player.playerID;

            // Check if this player's timer is still running
            bool hasTime = true;

            for (id_type tid = 0; tid <= World::maxId().id; ++tid) {
                ent_type timerEnt{tid};
                if (!World::mask(timerEnt).test(timerMask)) continue;

                const auto& timerPlayer = World::getComponent<PlayerInfo>(timerEnt);
                if (timerPlayer.playerID != pid) continue;

                const auto& timer = World::getComponent<GameTimer>(timerEnt);
                if (timer.timeLeft <= 0.0f) {
                    hasTime = false;
                    break;
                }
            }

            // Set input based on player ID and key pressed
            if (pid == 1) {
                input.sendRope = spacePressed && hasTime;
            } else if (pid == 2) {
                input.sendRope = enterPressed && hasTime;
            }
        }
    }


    /**
     * @brief Oscillates rope entities side-to-side while in rest state, simulating a swinging motion.
     *
     * This system applies a simple harmonic motion to ropes that are in the `AtRest` state.
     * Each rope is given an initial swing direction, and its rotation angle is updated frame-by-frame.
     * When the swing angle reaches a maximum threshold, the direction reverses, creating a back-and-forth effect.
     *
     * For each swinging rope:
     * - The matching player position is found (to calculate the rope origin).
     * - A visual swing angle is applied.
     * - The rope's physics body is repositioned to match the computed swing tip.
     * - Gravity is disabled while swinging and re-enabled when not at rest.
     *
     * Visual/physical parameters:
     * - `maxSwingAngle`: Maximum angle in degrees (positive or negative) the rope will reach
     * - `swingSpeed`: Speed of angular change (degrees per second)
     * - `ropeLength`: Logical length from player to rope tip, used to compute position
     * - `deltaTime`: Fixed timestep (assumes 60 FPS)
     * - `PPM`: Pixels-per-meter conversion factor
     *
     * Components required:
     * - `RoperTag`, `Rotation`, `RopeControl`, `PhysicsBody`, `PlayerInfo` for ropes
     * - `Position`, `PlayerInfo` for players (to locate origin)
     *
     * Debug output is printed for each rope showing its angle and tip position.
     *
     * @note Assumes player sprite dimensions and winch offsets match those in `CreateRope`.
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
        playerMask.set(Component<PlayerTag>::Bit);
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
     * @brief Controls rope extension and retraction, including physics-based movement and pull speed.
     *
     * This system advances the rope forward (extension) or pulls it back (retraction) based on
     * its control state and player input. When the rope is sent (`sendRope`), it enters the
     * `Extending` state and moves forward in the direction of its current angle.
     * If the maximum length is reached or an item is grabbed, the rope transitions to `Retracting`.
     *
     * During retraction, the rope’s speed is affected by the weight of the attached object
     * (if any), making heavier items pull back more slowly.
     *
     * Key behaviors:
     * - Calculates the tip of the rope from player's position and rotation
     * - Applies velocity to move the rope’s physics body to the target
     * - Handles return speed based on `Weight` component (if `GrabbedJoint` is present)
     * - When fully retracted, resets the state to `AtRest` and stops movement
     * - Cleans up rope joints upon finishing retraction
     *
     * Components required:
     * - `RoperTag`, `RopeControl`, `Length`, `Rotation`, `PhysicsBody`, `PlayerInfo`
     * - Also reads: `PlayerInput`, `Position`, `Weight`, `GrabbedJoint`
     *
     * @note This system assumes 60 FPS with a fixed timestep (1/60 seconds).
     */
    void RopeExtensionAndPullSystem() {
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
            playerMask.set(Component<PlayerTag>::Bit);
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
                float weightMultiplier = 1.0f;

                if (World::mask(rope).test(Component<GrabbedJoint>::Bit)) {
                    const auto& joint = World::getComponent<GrabbedJoint>(rope);
                    bagel::ent_type attached{joint.attachedEntityId};

                    std::cout << "[DEBUG] Checking weight for entity " << attached.id << std::endl;

                    if (World::mask(attached).test(Component<Weight>::Bit)) {
                        float itemWeight = World::getComponent<Weight>(attached).w;
                        std::cout << "[DEBUG] Weight = " << itemWeight << std::endl;
                        weightMultiplier = std::max(0.1f, itemWeight);
                    } else {
                        std::cout << "[DEBUG] No Weight component!" << std::endl;
                    }
                }

                float adjustedSpeed = RETRACTION_SPEED / weightMultiplier;
                length.value -= adjustedSpeed * deltaTime;

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
                        retractDir.x *= adjustedSpeed / PPM / retractDist;
                        retractDir.y *= adjustedSpeed / PPM / retractDist;
                        b2Body_SetLinearVelocity(phys.bodyId, retractDir);
                    } else {
                        b2Body_SetLinearVelocity(phys.bodyId, {0, 0});
                    }
                }
            }
            else if (ropeControl.state == RopeControl::State::AtRest) {
                b2Body_SetLinearVelocity(phys.bodyId, {0.0f, 0.0f});
            }

            // --- Joint cleanup when rope is at rest ---
            if (ropeControl.state == RopeControl::State::AtRest & World::mask(rope).test(Component<GrabbedJoint>::Bit)) {
                HandleRopeJointCleanup(rope);

                b2Body_SetLinearVelocity(phys.bodyId, {0.0f, 0.0f});
                b2Body_SetAngularVelocity(phys.bodyId, 0.0f);
                b2Body_SetGravityScale(phys.bodyId, 0.0f);
            }
        }
    }

    /**
     * @brief Detects and processes collisions between rope and collectible entities using Box2D events.
     *
     * This system reads all contact hit events from the Box2D world (`gWorld`) for the current frame.
     * It identifies rope-to-item collisions and attempts to attach the rope to a collectable if the rope
     * is not already holding another object.
     *
     * Conditions for attaching:
     * - One entity has `RoperTag`
     * - The other has `Collectable`
     * - The rope entity does not already contain a `GrabbedJoint` component
     *
     * Components checked:
     * - `RoperTag`, `Collectable`, `GrabbedJoint`
     *
     * @note Relies on Box2D v3 API and assumes `b2Body_SetUserData` was used to store `ent_type*`.
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

     /**
     * @brief Updates player scores based on collected items.
     *
     * This system scans all entities that:
     * - Are collectable (`Collectable`)
     * - Have a value (`Value`)
     * - Are currently grabbed by a rope (`GrabbedJoint`)
     * - Have NOT been scored yet (`ScoredTag`)
     *
     * For each such entity:
     * - The system finds the rope it's attached to via `GrabbedJoint.attachedEntityId`
     * - Uses the rope's `PlayerInfo` to determine which player collected the item
     * - Increases the player's `Score` by the item's `Value`
     * - Adds `ScoredTag` to mark it as already processed
     *
     * Expected components:
     * - Collectable
     * - Value
     * - GrabbedJoint
     * - Score (for each player)
     * - PlayerInfo (both for rope and score entities)
     *
     * Typical use: Call this system once per frame during the game loop,
     * after `PullObjectSystem()` has updated object positions and grab logic.
     */
    void ScoreSystem() {
        using namespace bagel;
        using namespace goldminer;

        Mask itemMask;
        itemMask.set(Component<Collectable>::Bit);
        itemMask.set(Component<Value>::Bit);
        itemMask.set(Component<GrabbedJoint>::Bit);

        Mask scoreMask;
        scoreMask.set(Component<Score>::Bit);
        scoreMask.set(Component<PlayerInfo>::Bit);

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type ent{id};

            if (!World::mask(ent).test(itemMask)) continue;
            if (World::mask(ent).test(Component<ScoredTag>::Bit)) continue; // ✅ already processed

            const Value& value = World::getComponent<Value>(ent);
            const GrabbedJoint& joint = World::getComponent<GrabbedJoint>(ent);
            if (joint.attachedEntityId == -1) continue;

            ent_type ropeEnt{joint.attachedEntityId};
            if (!World::mask(ropeEnt).test(Component<PlayerInfo>::Bit)) continue;

            const PlayerInfo& player = World::getComponent<PlayerInfo>(ropeEnt);
            int pid = player.playerID;

            for (id_type sid = 0; sid <= World::maxId().id; ++sid) {
                ent_type scoreEnt{sid};
                if (!World::mask(scoreEnt).test(scoreMask)) continue;

                const PlayerInfo& scorePlayer = World::getComponent<PlayerInfo>(scoreEnt);
                if (scorePlayer.playerID != pid) continue;

                Score& score = World::getComponent<Score>(scoreEnt);
                score.points += value.amount;

                World::addComponent<ScoredTag>(ent, {});
                break;
            }
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

            // Default scale
            float scale = 1.0f;

            // Apply scale only to player sprites (to shrink them)
            if (render.spriteID == SPRITE_PLAYER_AMAL || render.spriteID == SPRITE_PLAYER_NOA) {
                scale = 164.0f / 1000.0f; // match width to old size (~0.164)
            }
            if (render.spriteID == SPRITE_PLAYER_ELIZABETH || render.spriteID == SPRITE_PLAYER_OFEK) {
                scale = 164.0f / 900.0f;
            }

            SDL_FRect dest = {
                pos.x,
                pos.y,
                src.w * scale,
                src.h * scale
            };

            SDL_RenderTexture(renderer, texture, &src, &dest);
        }
    }


    /**
     * @brief Draws rope lines for all rope entities using their Box2D position.
     *
     * This system draws a black line between the player's center and the Box2D body
     * of the rope.
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
        playerMask.set(Component<PlayerTag>::Bit);

        for (bagel::id_type id = 0; id <= bagel::World::maxId().id; ++id) {
            bagel::ent_type ent{id};

            if (bagel::World::mask(ent).test(bagel::Component<goldminer::RoperTag>::Bit) &&
                bagel::World::mask(ent).test(bagel::Component<goldminer::PhysicsBody>::Bit)) {
                const auto& phys = bagel::World::getComponent<goldminer::PhysicsBody>(ent);
                b2Transform tf = b2Body_GetTransform(phys.bodyId);
                //std::cout << "ROPE at: " << tf.p.x * 50 << ", " << tf.p.y * 50 << std::endl;
                }

            if (bagel::World::mask(ent).test(bagel::Component<goldminer::ItemType>::Bit) &&
                bagel::World::mask(ent).test(bagel::Component<goldminer::PhysicsBody>::Bit)) {
                const auto& phys = bagel::World::getComponent<goldminer::PhysicsBody>(ent);
                b2Transform tf = b2Body_GetTransform(phys.bodyId);
                //std::cout << "ITEM at: " << tf.p.x * 50 << ", " << tf.p.y * 50 << std::endl;
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
     * @brief Decreases the remaining time for each player with a GameTimer.
     *
     * This system is called once per frame and is responsible for updating
     * the countdown timer of each player. It scans all entities that have both
     * a GameTimer and PlayerInfo component, subtracts the elapsed frame time
     * (`deltaTime`) from the timer, and clamps the result to zero if needed.
     *
     * This allows each player to have their own independent countdown.
     * Once the timer reaches 0, it will no longer decrease.
     *
     * Typical use: Call during the game loop when the game state is Playing.
     *
     * @param deltaTime The amount of time (in seconds) elapsed since the last frame.
     */
    void GameTimerSystem(float deltaTime) {
        using namespace bagel;

        Mask mask;
        mask.set(Component<GameTimer>::Bit);
        mask.set(Component<PlayerInfo>::Bit);

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type ent{id};
            if (!World::mask(ent).test(mask)) continue;

            GameTimer& timer = World::getComponent<GameTimer>(ent);
            timer.timeLeft -= deltaTime;

            if (timer.timeLeft < 0.0f)
                timer.timeLeft = 0.0f;
        }
    }

    /**
     * @brief Renders an integer number as a sequence of digit sprites on the screen.
     *
     * This function converts the input number to a string, then draws each digit
     * using its corresponding sprite texture (e.g., `SPRITE_DIGIT_0`, `SPRITE_DIGIT_1`, etc.).
     * The digits are drawn left to right starting from position `(x, y)` with a fixed scale.
     *
     * Drawing behavior:
     * - Each digit is scaled down by a constant factor (`SCALE`)
     * - A small spacing (2 pixels) is added between digits
     * - Uses `SDL_RenderTexture()` to draw each digit with `SDL_FRect` coordinates
     *
     * @param renderer SDL renderer used for drawing
     * @param number Integer number to display
     * @param x Starting x-coordinate on the screen
     * @param y Y-coordinate for all digits (baseline alignment)
     */
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
        constexpr float PLAYER_UI_SPACING_X = 640.0f;
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

        for (id_type id = 1; id <= World::maxId().id; ++id) {
            ent_type uiEnt{id};
            if (!World::mask(uiEnt).test(uiMask)) continue;

            const PlayerInfo& uiPlayer = World::getComponent<PlayerInfo>(uiEnt);
            int pid = uiPlayer.playerID;

            float offsetX = 5.0f + (pid-1) * PLAYER_UI_SPACING_X;

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
                if (seconds < 10) {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100);  // אדום שקוף
                    SDL_FRect bgRect = {
                        timeDst.x + timeDst.w + ICON_SPACING - 10,  // טיפה לפני הספרות
                        timeDst.y - 5,
                        55,
                        55
                    };
                    SDL_RenderFillRect(renderer, &bgRect);
                }
                DrawNumber(renderer, seconds, timeDst.x + timeDst.w + ICON_SPACING, timeDst.y );
                break;
            }
        }

    }

    void Box2DDebugRenderSystem(SDL_Renderer* renderer) {
        b2World_Draw(gWorld, &gDebugDraw);
    }

    /**
     * @brief Destroys all entities marked with the `DestroyTag` component.
     *
     * This system performs a two-phase destruction:
     * 1. Collection Phase – Iterates through all entities with `DestroyTag` using packed storage,
     *    and collects them into a `toDelete` list.
     * 2. Destruction Phase – For each entity in the list:
     *    - Removes all known components that the entity currently has.
     *    - Logs the entity destruction to the console.
     *
     * Design notes:
     * - Prevents modification of component storage during iteration by deferring actual deletion.
     * - Checks each component with `World::mask(e).test(...)` before attempting deletion.
     * - Handles all components currently used in the game (manual and exhaustive list).
     *
     * Components removed:
     * - Position, Velocity, Rotation, Length, Renderable, PlayerInfo, RopeControl,
     *   ItemType, Value, Weight, GrabbedJoint, PhysicsBody, PlayerInput, Collectable,
     *   RoperTag, GameOverTag, Collidable, DestroyTag
     *
     * @note This system assumes that deleting all components is sufficient for cleanup.
     * If an entity holds Box2D bodies or joints, their internal memory must be managed elsewhere.
     */
    void DestructionSystem() {
        Mask req = MaskBuilder{}.set<DestroyTag>().build();
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

    /**
 * @brief Determines if the game has ended and declares a winner or tie based on player scores.
 *
 * This system checks all `GameTimer` components to see if any players still have time left.
 * If no players have remaining time, it:
 * - Gathers all `Score` components
 * - Identifies the player(s) with the highest score
 * - Updates global game state flags (`game_over` and `player_id`) accordingly
 * - Prints the result to the console (either a win or tie)
 *
 * @note Assumes global variables `game_over` and `player_id` exist and are mutable.
 */
void CheckForGameOverSystem() {

    int playersWithTime = 0;
    std::vector<std::pair<int, int>> playerScores; // {playerID, score}

    // Step 1: Check if any player has time left
    for (id_type id = 0; id <= World::maxId().id; ++id) {
        ent_type ent{id};
        if (!World::mask(ent).test(Component<GameTimer>::Bit)) continue;
        if (!World::mask(ent).test(Component<PlayerInfo>::Bit)) continue;

        const GameTimer& timer = World::getComponent<GameTimer>(ent);
        if (timer.timeLeft > 0.0f)
            playersWithTime++;
    }

    // Step 2: If all players have time == 0, determine the winner
    if (playersWithTime == 0) {
        std::cout << "[CheckForGameOver] All players' timers reached 0\n";
        std::cout << "[CheckForGameOver] Gathering player scores:\n";

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type ent{id};
            if (!World::mask(ent).test(Component<Score>::Bit)) continue;
            if (!World::mask(ent).test(Component<PlayerInfo>::Bit)) continue;
            if (!World::mask(ent).test((Component<GameTimer>::Bit)))continue;

            const Score& score = World::getComponent<Score>(ent);
            const PlayerInfo& player = World::getComponent<PlayerInfo>(ent);


            playerScores.emplace_back(player.playerID, score.points);
        }

        if (!playerScores.empty()) {
            auto maxScoreIt = std::max_element(
                playerScores.begin(), playerScores.end(),
                [](const auto& a, const auto& b) {
                    return a.second < b.second;
                });

            int maxScore = maxScoreIt->second;
            std::vector<int> winners;

            for (const auto& p : playerScores) {
                if (p.second == maxScore) {
                    winners.push_back(p.first);
                }
            }

            if (winners.size() == 1) {
                player_id = winners[0];
                game_over = true;
                std::cout << "\nGAME OVER! Winner is Player " << winners[0]
                          << " with " << maxScore << " points!\n";
            } else {
                player_id = 0;
                game_over = true;
                std::cout << "\nGAME OVER! It's a tie between players with "
                          << maxScore << " points!\n";
            }
        }
    }
}


    //----------------------------------
    /// @section Helper Implementations
    //----------------------------------

    /**
     * @brief Creates a weld joint between a rope and a collectable entity, initiating retraction.
     *
     * This function attaches a collectable item (e.g. gold, rock, diamond) to a rope using a Box2D
     * weld joint. It ensures that the rope is not already holding an object, then:
     *
     * 1. Sets the item's body type to `dynamic` so it can move.
     * 2. Creates a `b2WeldJoint` to connect the rope and the item.
     * 3. Stores the joint reference in both entities via `GrabbedJoint` components.
     * 4. Switches the rope state to `Retracting` so it begins to pull the item back.
     * 5. Stops the collectable's movement by setting its velocity and angular velocity to zero.
     *
     * Preconditions:
     * - The rope must not already have a `GrabbedJoint` component.
     *
     * Components involved:
     * - Requires: `PhysicsBody` (for both entities), `RopeControl` (on rope)
     * - Adds: `GrabbedJoint` (to both rope and collectable)
     *
     * @param rope The rope entity attempting to grab
     * @param collectable The target item to be attached
     */
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
        World::addComponent<GrabbedJoint>(collectable, GrabbedJoint{jointId, rope.id});
        auto& ropeControl = World::getComponent<RopeControl>(rope);
        ropeControl.state = RopeControl::State::Retracting;

        //b2Body_SetAngularDamping(itemPhys.bodyId, 5.0f);
        b2Body_SetLinearVelocity(itemPhys.bodyId, {0, 0});
        b2Body_SetAngularVelocity(itemPhys.bodyId, 0);
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
     * @brief Destroys the joint between a rope and a grabbed item, and marks the item for destruction.
     *
     * This function is called when the rope finishes retracting and needs to release
     * the attached collectable. It checks whether the rope has a `GrabbedJoint` component,
     * and if so:
     * - Destroys the associated Box2D joint
     * - Removes the `GrabbedJoint` component from the rope
     * - Marks the attached item with `DestroyTag` so it will be cleaned up by `DestructionSystem`
     *
     * Preconditions:
     * - The rope must have a `GrabbedJoint` component; otherwise, the function returns immediately.
     *
     * Components involved:
     * - Requires: `GrabbedJoint` on the rope
     * - Affects: `GrabbedJoint` (removed from rope), `DestroyTag` (added to item)
     *
     * @param rope The rope entity that holds a joint with an item
     */
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
