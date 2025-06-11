#include "debug_draw.h"

#define SCALE 50.0f

b2DebugDraw gDebugDraw;

static void SDLColorFromHex(SDL_Renderer* renderer, b2HexColor hex) {
    Uint8 r = (hex >> 16) & 0xFF;
    Uint8 g = (hex >> 8) & 0xFF;
    Uint8 b = (hex) & 0xFF;
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
}

static void DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* ctx) {
    SDL_Renderer* renderer = (SDL_Renderer*)ctx;
    SDLColorFromHex(renderer, color);
    SDL_RenderDrawLineF(renderer, (int)(p1.x * SCALE), (int)(p1.y * SCALE), (int)(p2.x * SCALE), (int)(p2.y * SCALE));
}

static void DrawPolygon(const b2Vec2* vertices, int count, b2HexColor color, void* ctx) {
    SDL_Renderer* renderer = (SDL_Renderer*)ctx;
    SDLColorFromHex(renderer, color);
    for (int i = 0; i < count; ++i) {
        int j = (i + 1) % count;
        SDL_RenderDrawLineF(renderer,
                       (int)(vertices[i].x * SCALE), (int)(vertices[i].y * SCALE),
                       (int)(vertices[j].x * SCALE), (int)(vertices[j].y * SCALE));
    }
}

static void DrawCircle(b2Vec2 center, float radius, b2HexColor color, void* ctx) {
    SDL_Renderer* renderer = (SDL_Renderer*)ctx;
    SDLColorFromHex(renderer, color);
    const int segments = 32;
    float angleStep = 2.0f * 3.14159f / segments;
    for (int i = 0; i < segments; ++i) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;
        b2Vec2 p1 = { center.x + radius * cosf(angle1), center.y + radius * sinf(angle1) };
        b2Vec2 p2 = { center.x + radius * cosf(angle2), center.y + radius * sinf(angle2) };
        SDL_RenderDrawLineF(renderer, (int)(p1.x * SCALE), (int)(p1.y * SCALE), (int)(p2.x * SCALE), (int)(p2.y * SCALE));
    }
}

static void DrawTransform(b2Transform xf, void* ctx) {
    SDL_Renderer* renderer = (SDL_Renderer*)ctx;

    b2Vec2 p1 = xf.p;
    b2Vec2 p2x = { xf.p.x + xf.q.c, xf.p.y + xf.q.s };
    b2Vec2 p2y = { xf.p.x - xf.q.s, xf.p.y + xf.q.c };

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // x axis - red
    SDL_RenderDrawLineF(renderer, (int)(p1.x * SCALE), (int)(p1.y * SCALE), (int)(p2x.x * SCALE), (int)(p2x.y * SCALE));

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // y axis - green
    SDL_RenderDrawLineF(renderer, (int)(p1.x * SCALE), (int)(p1.y * SCALE), (int)(p2y.x * SCALE), (int)(p2y.y * SCALE));
}

void InitDebugDraw(SDL_Renderer* renderer) {
    gDebugDraw = b2DefaultDebugDraw();
    gDebugDraw.DrawSegmentFcn = DrawSegment;
    gDebugDraw.DrawPolygonFcn = DrawPolygon;
    gDebugDraw.DrawCircleFcn = DrawCircle;
    gDebugDraw.DrawTransformFcn = DrawTransform;
    gDebugDraw.context = renderer;
    gDebugDraw.drawShapes = true;
    gDebugDraw.drawJoints = true;
    gDebugDraw.drawBounds = true;
    gDebugDraw.drawMass = true;
    gDebugDraw.drawContacts = true;

}
