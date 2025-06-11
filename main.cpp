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
    // Initialize debug drawing with your renderer
    InitDebugDraw(renderer);
    goldminer::initBox2DWorld();
    // Load sprite textures from "res" folder
    LoadAllSprites(renderer);
    // Create some initial entities
    goldminer::CreatePlayer(1);
    goldminer::CreateRope(1);
    goldminer::CreateGold(1000.0f, 530.0f);
    goldminer::CreateDiamond(200.0f, 300.0f);
    goldminer::CreateRock(400.0f, 520.0f);
    goldminer::CreateTreasureChest(600.0f, 520.0f);

    bool running = true;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        constexpr float timeStep = 1.0f / 120.0f;  // 120 FPS simulation
        constexpr int velocityIterations = 12;
        constexpr int positionIterations = 6;

        b2World_Step(goldminer::gWorld, timeStep, velocityIterations);

        goldminer::PhysicsSyncSystem();
        goldminer::CollisionSystem();
        goldminer::DebugCollisionSystem();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw background
        SDL_FRect bg = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
        SDL_RenderTexture(renderer, GetSpriteTexture(SPRITE_BACKGROUND), nullptr, &bg);

        // Render ECS entities
        goldminer::RenderSystem(renderer);
        goldminer::RopeRenderSystem(renderer);
        goldminer::Box2DDebugRenderSystem(renderer);


        SDL_RenderPresent(renderer);
        SDL_Delay(16); // Approximate 60 FPS
    }

    // Cleanup
    UnloadAllSprites();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
