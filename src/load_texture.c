#include "texture.h"

#include <stdio.h>

IWICImagingFactory *g_wic_factory;

BOOL Texture_Load(Texture *texture, const char *path)
{
    // Safety checks
    if (texture == NULL || path == NULL || path[0] == '\0') {
        return FALSE;
    }

    BOOL success = FALSE;
    IWICFormatConverter *converter = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;

    // Check WIC library is initialized
    if (g_wic_factory == NULL) {
        fprintf(stderr, "Failed to load image \"%s\": WIC library is not initialized\n", path);
        return FALSE;
    }

    // Create the WIC format converter
    if (g_wic_factory->lpVtbl->CreateFormatConverter(g_wic_factory, &converter) != S_OK) {
        fprintf(stderr, "Failed to load image \"%s\": Unable to create the WIC file format converter\n", path);
        goto _EXIT;
    }

    // Convert path from PCSTR to PCWSTR
    WCHAR path_wide[MAX_PATH];
    int path_wide_length = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
    MultiByteToWideChar(CP_ACP, 0, path, -1, path_wide, path_wide_length);

    // Create decoder from the file name
    if (g_wic_factory->lpVtbl->CreateDecoderFromFilename(g_wic_factory, path_wide, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder) != S_OK) {
        fprintf(stderr, "Failed to load image \"%s\": Path not found\n", path);
        goto _EXIT;
    }

    // Decode the first image frame (usually there are always just one frame, except for the .gif animations or something like this)
    if (decoder->lpVtbl->GetFrame(decoder, 0, &frame) != S_OK) {
        goto _EXIT;
    }

    // Get image data
    if (converter->lpVtbl->Initialize(converter, (IWICBitmapSource *)frame, &GUID_WICPixelFormat32bppBGR, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom) != S_OK) {
        goto _EXIT;
    }
    if (converter->lpVtbl->GetSize(converter, &texture->width, &texture->height) != S_OK) {
        goto _EXIT;
    }
    UINT step = texture->width * sizeof(DWORD);
    UINT buffer_size = texture->height * step;
    texture->data = HeapAlloc(GetProcessHeap(), 0, buffer_size);
    if (texture->data == NULL) {
        fprintf(stderr, "Failed to load image \"%s\": Not enough memory to allocate the surface\n", path);
        goto _EXIT;
    }
    if (converter->lpVtbl->CopyPixels(converter, NULL, step, buffer_size, (BYTE *)texture->data) != S_OK) {
        goto _EXIT;
    }

    success = TRUE; // if code executing reaches this line - everything is ok

_EXIT:
    // Release WIC objects
    if (converter != NULL) { converter->lpVtbl->Release(converter); }
    if (decoder != NULL) { decoder->lpVtbl->Release(decoder); }
    if (frame != NULL) { frame->lpVtbl->Release(frame); }

    return success;
}
