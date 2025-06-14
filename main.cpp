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
    Playing,
    GameOver
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

    SDL_Texture* menuTexture = IMG_LoadTexture(renderer, "res/gameStart.png");
    if (!menuTexture) {
        std::cerr << "Failed to load menu image: " << SDL_GetError() << std::endl;
        return 1;
    }

    GameState gameState = GameState::MainMenu;
    bool running = true;
    SDL_Event e;

    InitDebugDraw(renderer);
    goldminer::initBox2DWorld();
    LoadAllSprites(renderer);

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            goldminer::PlayerInputSystem(&e);

            if (e.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode key = e.key.key;

                if (gameState == GameState::MainMenu && key == SDLK_RETURN) {
                    // === Initialize game ===
                    goldminer::CreatePlayer(1);
                    goldminer::CreatePlayer(2);

                    goldminer::CreateRope(1);
                    goldminer::CreateRope(2);

                    // Random layout
                    int layout = rand() % 3;
                    switch (layout) {
                        case 0: goldminer::LoadLayout1(); break;
                        case 1: goldminer::LoadLayout2(); break;
                        case 2: goldminer::LoadLayout3(); break;
                    }

                    goldminer::CreateUIEntity(1);
                    goldminer::CreateUIEntity(2);

                    bagel::Entity score1 = bagel::Entity::create();
                    score1.addAll(goldminer::Score{0}, goldminer::PlayerInfo{1});

                    bagel::Entity score2 = bagel::Entity::create();
                    score2.addAll(goldminer::Score{0}, goldminer::PlayerInfo{2});

                    bagel::Entity timer1 = bagel::Entity::create();
                    timer1.addAll(goldminer::GameTimer{30.0f}, goldminer::PlayerInfo{1});

                    bagel::Entity timer2 = bagel::Entity::create();
                    timer2.addAll(goldminer::GameTimer{30.0f}, goldminer::PlayerInfo{2});

                    gameState = GameState::Playing;
                } else if (gameState == GameState::Playing && key == SDLK_ESCAPE) {
                    gameState = GameState::MainMenu;
                }
            }
        }

        constexpr float timeStep = 1.0f / 60.0f;
        constexpr int velocityIterations = 8;
        constexpr int positionIterations = 3;
        b2World_Step(goldminer::gWorld, timeStep, velocityIterations);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (gameState == GameState::MainMenu) {
            SDL_FRect dstRect = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
            SDL_RenderTexture(renderer, menuTexture, nullptr, &dstRect);
        }
        else if (gameState == GameState::Playing) {
            // === Two backgrounds side by side ===
            SDL_FRect bg1 = {0, 0, 640, 720};
            SDL_FRect bg2 = {640, 0, 640, 720};
            SDL_RenderTexture(renderer, GetSpriteTexture(SPRITE_BACKGROUND), nullptr, &bg1);
            SDL_RenderTexture(renderer, GetSpriteTexture(SPRITE_BACKGROUND), nullptr, &bg2);


            // Systems
            goldminer::GameTimerSystem(timeStep);
            goldminer::RopeSwingSystem();
            goldminer::ScoreSystem();
            goldminer::RopeExtensionSystem();
            goldminer::PlayerInputSystem(nullptr);
            goldminer::PhysicsSyncSystem();
            goldminer::CollisionSystem();
            goldminer::CheckForGameOverSystem();


            goldminer::RenderSystem(renderer);
            goldminer::RopeRenderSystem(renderer);
            goldminer::UISystem(renderer);
            goldminer::DestructionSystem();

            if (goldminer::game_over) {
                gameState = GameState::GameOver;
            }


        }
        else if (gameState == GameState::GameOver) {
            int winner = goldminer::player_id;

            SDL_Texture* winTexture = nullptr;

            if (winner == 1) {
                winTexture = IMG_LoadTexture(renderer, "res/Player1WINS.png");
            } else if (winner == 2) {
                winTexture = IMG_LoadTexture(renderer, "res/Player2WINS.png");
            } else {
                winTexture = IMG_LoadTexture(renderer, "res/Tie.png");
            }

            if (winTexture) {
                SDL_FRect dstRect = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
                SDL_RenderTexture(renderer, winTexture, nullptr, &dstRect);
                SDL_DestroyTexture(winTexture);
            } else {
                std::cerr << "Failed to load win/tie screen.\n";
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // ~60 FPS
    }

    SDL_DestroyTexture(menuTexture);
    UnloadAllSprites();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
