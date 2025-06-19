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
    CharacterSelect,
    Playing,
    GameOver
};

SpriteID player1Sprite = SPRITE_PLAYER_AMAL;
SpriteID player2Sprite = SPRITE_PLAYER_OFEK;

int selectionP1 = 0;
int selectionP2 = 0;
bool p1Confirmed = false;
bool p2Confirmed = false;

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
                    gameState = GameState::CharacterSelect;
                    selectionP1 = 0;
                    selectionP2 = 0;
                    p1Confirmed = false;
                    p2Confirmed = false;
                }
                else if (gameState == GameState::CharacterSelect) {
                    if (!p1Confirmed) {
                        if (key == SDLK_LEFT || key == SDLK_RIGHT) selectionP1 = 1 - selectionP1;
                        else if (key == SDLK_RETURN) p1Confirmed = true;
                    } else if (!p2Confirmed) {
                        if (key == SDLK_LEFT || key == SDLK_RIGHT) selectionP2 = 1 - selectionP2;
                        else if (key == SDLK_RETURN) p2Confirmed = true;
                    }

                    if (p1Confirmed && p2Confirmed) {
                        player1Sprite = (selectionP1 == 0) ? SPRITE_PLAYER_AMAL : SPRITE_PLAYER_OFEK;
                        player2Sprite = (selectionP2 == 0) ? SPRITE_PLAYER_AMAL : SPRITE_PLAYER_OFEK;
                        goldminer::CreatePlayer(1, player1Sprite);
                        goldminer::CreatePlayer(2,player2Sprite);
                        goldminer::CreateRope(1);
                        goldminer::CreateRope(2);

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
                    }
                }
                else if (gameState == GameState::Playing && key == SDLK_ESCAPE) {
                    gameState = GameState::MainMenu;
                }
            }
        }

        constexpr float timeStep = 1.0f / 60.0f;
        constexpr int velocityIterations = 8;
        b2World_Step(goldminer::gWorld, timeStep, velocityIterations);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (gameState == GameState::MainMenu) {
            SDL_FRect dstRect = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
            SDL_RenderTexture(renderer, menuTexture, nullptr, &dstRect);
        }
        else if (gameState == GameState::CharacterSelect) {
            SDL_FRect dst1 = {SCREEN_WIDTH / 4 - 82, 200, 164, 169};
            SDL_FRect dst2 = {3 * SCREEN_WIDTH / 4 - 82, 200, 164, 169};

            SpriteID p1 = (selectionP1 == 0) ? SPRITE_PLAYER_AMAL : SPRITE_PLAYER_OFEK;
            SpriteID p2 = (selectionP2 == 0) ? SPRITE_PLAYER_AMAL : SPRITE_PLAYER_OFEK;

            SDL_Rect src1 = GetSpriteSrcRect(p1);
            SDL_Rect src2 = GetSpriteSrcRect(p2);

            SDL_RenderTexture(renderer, GetSpriteTexture(p1), nullptr, &dst1);
            SDL_RenderTexture(renderer, GetSpriteTexture(p2), nullptr, &dst2);

            if (!p1Confirmed) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                SDL_RenderRect(renderer, &dst1);
            } else if (!p2Confirmed) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                SDL_RenderRect(renderer, &dst2);
            }
        }
        else if (gameState == GameState::Playing) {
            SDL_FRect bg1 = {0, 0, 1280, 720};
            SpriteID bgID = SPRITE_BACKGROUND_LEVEL1;
            for (bagel::id_type i = 0; i <= bagel::World::maxId().id; ++i) {
                bagel::ent_type ent{i};
                if (bagel::World::mask(ent).test(bagel::Component<goldminer::LevelInfo>::Bit)) {
                    bgID = bagel::World::getComponent<goldminer::LevelInfo>(ent).background;
                    break;
                }
            }

            SDL_RenderTexture(renderer, GetSpriteTexture(bgID), nullptr, &bg1);

            goldminer::GameTimerSystem(timeStep);
            goldminer::RopeSwingSystem();
            goldminer::ScoreSystem();
            goldminer::RopeExtensionAndPullSystem();
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
            if (winner == 1) winTexture = IMG_LoadTexture(renderer, "res/Player1WINS.png");
            else if (winner == 2) winTexture = IMG_LoadTexture(renderer, "res/Player2WINS.png");
            else winTexture = IMG_LoadTexture(renderer, "res/tie.png");

            if (winTexture) {
                SDL_FRect dstRect = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
                SDL_RenderTexture(renderer, winTexture, nullptr, &dstRect);
                SDL_DestroyTexture(winTexture);
            } else {
                std::cerr << "Failed to load win/tie screen.\n";
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(menuTexture);
    UnloadAllSprites();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
