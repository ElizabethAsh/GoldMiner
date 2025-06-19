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

enum SpriteID {
    SPRITE_GOLD = 0,
    SPRITE_ROCK,
    SPRITE_DIAMOND,
    SPRITE_TREASURE_CHEST,
    SPRITE_BOMB,
    SPRITE_PLAYER_IDLE,
    SPRITE_PLAYER_AMAL,
    SPRITE_PLAYER_OFEK,
    SPRITE_TITLE_MONEY,
    SPRITE_TITLE_TIME,
    SPRITE_BACKGROUND,
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

namespace bagel
{
    struct ent_type;
}

namespace goldminer
{
    // Global Box2D world for physics (preview API)
    extern b2WorldId gWorld;
    using id_type = int;
    extern id_type player_id;
    extern bool game_over ;
    extern SpriteID player1Sprite;
    extern SpriteID player2Sprite;


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
    void RopeExtensionAndPullSystem();
    void CollisionSystem();
    void ScoreSystem();
    void RenderSystem(SDL_Renderer* renderer);
    void GameTimerSystem(float deltaTime);
    void UISystem(SDL_Renderer* renderer);
    void PhysicsSyncSystem();
    void RopeRenderSystem(SDL_Renderer* renderer);
    void Box2DDebugRenderSystem(SDL_Renderer* renderer);
    void DestructionSystem();
    void CheckForGameOverSystem();

    // helpers:
    SDL_FPoint GetSpriteOffset(int spriteID);
    void TryAttachCollectable(bagel::ent_type rope, bagel::ent_type collectable);
    void HandleRopeJointCleanup(bagel::ent_type rope);


//----------------------------------
/// @section Entity Creation
//----------------------------------

    id_type CreatePlayer(int playerID);
    id_type CreateRope(int playerID);
    id_type CreateGold(float x, float y);
    id_type CreateRock(float x, float y);
    id_type CreateDiamond(float x, float y);
    id_type CreateTreasureChest(float x, float y);
    id_type CreateUIEntity(int playerID);

    //----------------------------------
    /// @section Game's Layout
    //----------------------------------

    void LoadLayout1();
    void LoadLayout2();
    void LoadLayout3();

} // namespace goldminer


#endif // GOLD_MINER_ECS_H