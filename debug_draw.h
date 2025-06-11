#pragma once
#define SDL_ENABLE_OLD_NAMES
#include <SDL3/SDL.h>
#include <box2d/box2d.h>

void InitDebugDraw(SDL_Renderer* renderer);
extern b2DebugDraw gDebugDraw;

