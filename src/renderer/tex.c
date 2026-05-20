#include "tex.h"
#include "../../pc/hal/gs/gl_loader.h"
#include <stdlib.h>
#include <string.h>

u32 tex_from_wad(const u8 *clut, const u8 *pixels, u32 width, u32 height) {
    u32 total_pixels = width * height;
    u8 *rgba = malloc(total_pixels * 4);
    
    for (u32 i = 0; i < total_pixels; i++) {
        u8 idx = pixels[i];
        
        rgba[i*4 + 0] = clut[idx*4 + 0]; // R
        rgba[i*4 + 1] = clut[idx*4 + 1]; // G
        rgba[i*4 + 2] = clut[idx*4 + 2]; // B
        
        rgba[i*4 + 3] = 255; /* Force opaque for visual debugging */
        
        /* Debug: If everything is zero, create a yellow/black checkerboard */
        if (rgba[i*4+0] == 0 && rgba[i*4+1] == 0 && rgba[i*4+2] == 0) {
            u32 x = (i % width) / 16;
            u32 y = (i / width) / 16;
            if ((x + y) % 2 == 0) {
                rgba[i*4+0] = 255; rgba[i*4+1] = 255; rgba[i*4+2] = 0;
            }
        }
    }
    
    u32 tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    
    // Using GL_NEAREST for that crispy PS2 look
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    
    free(rgba);
    return tex_id;
}
