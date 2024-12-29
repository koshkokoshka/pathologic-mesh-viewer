#pragma once

#include <windows.h>
#include <wincodec.h>

typedef struct {
    UINT width;
    UINT height;
    DWORD *data;
} Texture;

// WIC factory instance for loading game textures
extern IWICImagingFactory *g_wic_factory;

BOOL Texture_Load(Texture *texture, const char *path);
