#include "engine/presentation/asset_cache.h"
#include <string.h>

typedef struct {
    char filepath[128];
    Texture2D texture;
    bool loaded;
} CachedTextureEntry;

static CachedTextureEntry s_textures[ASSET_CACHE_MAX_TEXTURES];
static int s_textureCount = 0;

void AnimSprite_Init(AnimatedSprite* spr, Texture2D sheet, int frameCount, float frameDuration, int frameW, int frameH) {
    if (!spr) return;
    spr->sheet = sheet;
    spr->frameCount = frameCount > 0 ? frameCount : 1;
    spr->frameDuration = frameDuration > 0.0f ? frameDuration : 0.1f;
    spr->frameW = frameW > 0 ? frameW : sheet.width;
    spr->frameH = frameH > 0 ? frameH : sheet.height;
    spr->currentFrame = 0;
    spr->timer = 0.0f;
}

void AnimSprite_Update(AnimatedSprite* spr, float deltaTime) {
    if (!spr || spr->frameCount <= 1) return;
    spr->timer += deltaTime;
    if (spr->timer >= spr->frameDuration) {
        spr->timer -= spr->frameDuration;
        spr->currentFrame = (spr->currentFrame + 1) % spr->frameCount;
    }
}

Rectangle AnimSprite_GetFrame(const AnimatedSprite* spr) {
    if (!spr) return (Rectangle){0, 0, 0, 0};
    return (Rectangle){
        (float)(spr->currentFrame * spr->frameW),
        0.0f,
        (float)spr->frameW,
        (float)spr->frameH
    };
}

void AssetCache_Init(void) {
    s_textureCount = 0;
}

Texture2D AssetCache_LoadTexture(const char* filepath) {
    if (!filepath || !filepath[0]) return (Texture2D){0};

    // Verificar si ya está en caché
    for (int i = 0; i < s_textureCount; i++) {
        if (s_textures[i].loaded && strcmp(s_textures[i].filepath, filepath) == 0) {
            return s_textures[i].texture;
        }
    }

    if (s_textureCount < ASSET_CACHE_MAX_TEXTURES && FileExists(filepath)) {
        Texture2D tex = LoadTexture(filepath);
        int idx = s_textureCount++;
        strncpy(s_textures[idx].filepath, filepath, sizeof(s_textures[idx].filepath) - 1);
        s_textures[idx].texture = tex;
        s_textures[idx].loaded = true;
        return tex;
    }

    return (Texture2D){0};
}

Texture2D AssetCache_GetTexture(const char* filepath) {
    return AssetCache_LoadTexture(filepath);
}

void AssetCache_UnloadAll(void) {
    for (int i = 0; i < s_textureCount; i++) {
        if (s_textures[i].loaded) {
            UnloadTexture(s_textures[i].texture);
            s_textures[i].loaded = false;
        }
    }
    s_textureCount = 0;
}
