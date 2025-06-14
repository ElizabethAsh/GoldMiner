/**
 * @file gold_miner_ecs.h
 * @brief Gold Miner game module using BAGEL ECS architecture.
 *
 * This module defines the game components, systems, and entity creation functions
 * for the "Gold Miner" game, following the ECS model implemented with BAGEL.
 */

#ifndef GOLD_MINER_ECS_H
#define GOLD_MINER_ECS_H

#include <cstdint>
#include <string>
#include <SDL3/SDL.h>
#include <box2d/box2d.h>


namespace bagel
{
    struct ent_type;
}

namespace goldminer
{
    // Global Box2D world for physics (preview API)
    extern b2WorldId gWorld;
    using id_type = int;

    //----------------------------------
    /// @section Components
    //----------------------------------

    struct Position {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Velocity {
        float dx = 0.0f;
        float dy = 0.0f;
    };

    struct Rotation {
        float angle = 0.0f; ///< Rope rotation
    };

    struct Length {
        float value = 0.0f; ///< Rope length
    };

    struct Renderable {
        int spriteID = -1; ///< Sprite index or enum
    };

    struct PlayerInfo {
        int playerID = -1;
    };

    struct RopeControl {
        enum class State { AtRest, Extending, Retracting } state = State::AtRest;
    };

    struct ItemType {
        enum class Type { Gold, Rock, Diamond, TreasureChest, MysteryBag  } type = Type::Gold;
    };

    struct Value {
        int amount = 0;
    };

    struct Weight {
        float w = 1.0f;
    };

    struct Score {
        int points = 0;
    };

    struct GameTimer {
        float timeLeft = 60.0f;
    };

    struct UIComponent {
        int uiID = -1;
    };

    struct SoundEffect {
        int soundID = -1; ///< Placeholder for sound index
    };

    struct Name {
        std::string label;
    };

    struct Health {
        int hp = 1;
    };

    struct Mole {
        float speed = 100.0f;
        bool movingRight = true;
    };

    struct LifeTime {
        float remaining = 1.5f;
    };

    struct GrabbedJoint {
        b2JointId joint = b2_nullJointId;
        int attachedEntityId = -1;
    };

    struct PhysicsBody {
        b2BodyId bodyId;
    };

    struct PlayerInput {
        bool sendRope = false;
        bool retractRope = false;
    };

    struct ScoredTag {};


    //----------------------------------
    /// @section Tags
    //----------------------------------

    struct Collectable {};
    struct RoperTag {};
    struct GameOverTag {};
    struct Collidable {};
    struct DestroyTag {};

//----------------------------------
/// @section System Declarations
//----------------------------------
    void initBox2DWorld();
    void PlayerInputSystem(const SDL_Event* event);
    void RopeSwingSystem();
    void RopeExtensionSystem();
    void CollisionSystem();
    void TryAttachCollectable(bagel::ent_type rope, bagel::ent_type collectable);
    void PullObjectSystem();
    void ScoreSystem();
    void TreasureChestSystem();
    void RenderSystem(SDL_Renderer* renderer);
    void GameTimerSystem(float deltaTime);
    void UISystem(SDL_Renderer* renderer);
    void MoleSystem();
    void LifeTimeSystem();
    void PhysicsSyncSystem();
    void CollectableVanishSystem();
    void DebugCollisionSystem();
    void RopeRenderSystem(SDL_Renderer* renderer);
    void Box2DDebugRenderSystem(SDL_Renderer* renderer);
    void HandleRopeJointCleanup(bagel::ent_type rope);
    void DestructionSystem();


//----------------------------------
/// @section Entity Creation
//----------------------------------

    id_type CreatePlayer(int playerID);
    id_type CreateRope(int playerID);
    id_type CreateGold(float x, float y);
    id_type CreateRock(float x, float y);
    id_type CreateDiamond(float x, float y);
    id_type CreateMysteryBag(float x, float y);
    id_type CreateTreasureChest(float x, float y);
    id_type CreateTimer();
    id_type CreateUIEntity(int playerID);
    id_type CreateMole(float x, float y);

} // namespace goldminer

enum SpriteID {
    SPRITE_GOLD = 0,
    SPRITE_ROCK,
    SPRITE_DIAMOND,
    SPRITE_TREASURE_CHEST,
    SPRITE_MYSTERY_BAG,
    SPRITE_BOMB,
    SPRITE_PLAYER_IDLE,
    SPRITE_PLAYER_PULL1,
    SPRITE_PLAYER_PULL2,
    SPRITE_TITLE_MONEY,
    SPRITE_TITLE_TIME,
    SPRITE_TIMER,
    SPRITE_BACKGROUND,
    SPRITE_PRESS_ENTER_TO_START, // New sprite for the main menu text
    SPRITE_PAUSED_TEXT,          // New sprite for the pause menu text
    SPRITE_DIGIT_0,
    SPRITE_DIGIT_1,
    SPRITE_DIGIT_2,
    SPRITE_DIGIT_3,
    SPRITE_DIGIT_4,
    SPRITE_DIGIT_5,
    SPRITE_DIGIT_6,
    SPRITE_DIGIT_7,
    SPRITE_DIGIT_8,
    SPRITE_DIGIT_9,


    SPRITE_COUNT
};

#endif // GOLD_MINER_ECS_H