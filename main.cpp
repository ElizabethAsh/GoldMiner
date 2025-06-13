#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <box2d/box2d.h>
#include "debug_draw.h"
#include "gold_miner_ecs.h"
#include "sprite_manager.h"
#include "bagel.h"

#include <iostream>

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

enum class GameState {
    MainMenu,
    Playing
};

int main() {
    std::cout << "Starting Gold Miner ECS...\n";

    SDL_Window* window = SDL_CreateWindow("Gold Miner ECS", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    // Load main menu image
    SDL_Texture* menuTexture = IMG_LoadTexture(renderer, "res/menu_screen.png");
    if (!menuTexture) {
        std::cerr << "Failed to load menu image: " << SDL_GetError() << std::endl;
        return 1;
    }

    GameState gameState = GameState::MainMenu;
    bool running = true;
    SDL_Event e;

    // Initialize debug drawing with your renderer
    InitDebugDraw(renderer);
    goldminer::initBox2DWorld();
    // Load sprite textures from "res" folder
    LoadAllSprites(renderer);

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
            goldminer::PlayerInputSystem(&e);
            if (e.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode key = e.key.key;

                if (gameState == GameState::MainMenu && key == SDLK_RETURN) {
                    // Start the game
                    // Create some initial entities
                    goldminer::CreatePlayer(1);
                    goldminer::CreateRope(1);
                    goldminer::CreateGold(100.0f, 500.0f);
                    goldminer::CreateDiamond(600.0f, 520.0f);
                    goldminer::CreateRock(1000.0f, 530.0f);
                    goldminer::CreateTreasureChest(300.0f, 510.0f);

                    gameState = GameState::Playing;
                } else if (gameState == GameState::Playing && key == SDLK_ESCAPE) {
                    // Return to main menu
                    gameState = GameState::MainMenu;
                    // Optional: you can also reset game state here
                }
            }
        }

        constexpr float timeStep = 1.0f / 120.0f;  // 120 FPS simulation
        constexpr int velocityIterations = 12;
        constexpr int positionIterations = 6;
        b2World_Step(goldminer::gWorld, timeStep, velocityIterations);



        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (gameState == GameState::MainMenu) {
            // Render main menu image
            SDL_FRect dstRect = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
            SDL_RenderTexture(renderer, menuTexture, nullptr, &dstRect);
        }
        else if (gameState == GameState::Playing) {
            // Step Box2D world
            constexpr float timeStep = 1.0f / 60.0f;
            constexpr int velocityIterations = 8;
            constexpr int positionIterations = 3;
            b2World_Step(goldminer::gWorld, timeStep, velocityIterations);

            // Draw background
            SDL_FRect bg = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
            SDL_RenderTexture(renderer, GetSpriteTexture(SPRITE_BACKGROUND), nullptr, &bg);
            goldminer::RopeSwingSystem();
            goldminer::RopeExtensionSystem();
            goldminer::PhysicsSyncSystem();
            goldminer::CollisionSystem();
            goldminer::DebugCollisionSystem();
            // Render ECS entities
            goldminer::RenderSystem(renderer);
            goldminer::RopeRenderSystem(renderer);
            goldminer::Box2DDebugRenderSystem(renderer); //not redundant

        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    SDL_DestroyTexture(menuTexture);
    UnloadAllSprites();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
