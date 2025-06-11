#include "sprite_manager.h"
#include <array>
#include <SDL3_image/SDL_image.h>
#include <iostream>

static std::array<SDL_Texture*, SPRITE_COUNT> gTextures;
static std::array<SDL_Rect, SPRITE_COUNT> gSrcRects;

SDL_Texture* LoadTexture(SDL_Renderer* renderer, const char* path) {
    SDL_Texture* tex = IMG_LoadTexture(renderer, path);
    if (!tex) {
        std::cerr << "Failed to load: " << path << "\nSDL_GetError: " << SDL_GetError() << "\n";
    }
    return tex;
}

void LoadAllSprites(SDL_Renderer* renderer) {
    gTextures[SPRITE_GOLD] = LoadTexture(renderer, "res/gold.png");
    gTextures[SPRITE_ROCK] = LoadTexture(renderer, "res/rock.png");
    gTextures[SPRITE_DIAMOND] = LoadTexture(renderer, "res/diamond.png");
    //    gTextures[SPRITE_MYSTERY_BAG] = LoadTexture(renderer, "res/mysteryBox.png");
    gTextures[SPRITE_BOMB] = LoadTexture(renderer, "res/bom.png");
    gTextures[SPRITE_PLAYER_IDLE] = LoadTexture(renderer, "res/player.png");
    gTextures[SPRITE_BACKGROUND] = LoadTexture(renderer, "res/background.png");
    gTextures[SPRITE_TREASURE_CHEST] = LoadTexture(renderer, "res/treasureChest.png");



    gSrcRects[SPRITE_GOLD] = {0, 0, 35, 30};
    gSrcRects[SPRITE_ROCK] = {0, 0, 77, 87};
    gSrcRects[SPRITE_DIAMOND] = {0, 0, 41, 32};
    //    gSrcRects[SPRITE_MYSTERY_BAG] = {737, 47, 520, 592};
    gSrcRects[SPRITE_BOMB] = {0, 0, 77, 67};
    gSrcRects[SPRITE_PLAYER_IDLE] = {0, 7, 164, 169};
    gSrcRects[SPRITE_BACKGROUND] = {0, 0, 1280, 720};
    gSrcRects[SPRITE_TREASURE_CHEST] = {33, 50, 88, 82};

}

void UnloadAllSprites() {
    for (auto& tex : gTextures) {
        if (tex) SDL_DestroyTexture(tex);
        tex = nullptr;
    }
}

SDL_Texture* GetSpriteTexture(SpriteID id) {
    return gTextures[id];
}

SDL_Rect GetSpriteSrcRect(SpriteID id) {
    return gSrcRects[id];
}
