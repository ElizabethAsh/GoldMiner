#ifndef SPRITE_MANAGER_H
#define SPRITE_MANAGER_H

#include <SDL3/SDL.h>
#include "gold_miner_ecs.h"

void LoadAllSprites(SDL_Renderer* renderer);
void UnloadAllSprites();
SDL_Texture* GetSpriteTexture(SpriteID id);
SDL_Rect GetSpriteSrcRect(SpriteID id);

#endif // SPRITE_MANAGER_H

