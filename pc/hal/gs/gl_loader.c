/**
 * gl_loader.c — Implementation of the minimal OpenGL loader.
 */

#include "gl_loader.h"
#include <SDL2/SDL.h>
#include <stdio.h>

PFNGLGENBUFFERSPROC glGenBuffers = NULL;
PFNGLBINDBUFFERPROC glBindBuffer = NULL;
PFNGLBUFFERDATAPROC glBufferData = NULL;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
PFNGLCREATESHADERPROC glCreateShader = NULL;
PFNGLSHADERSOURCEPROC glShaderSource = NULL;
PFNGLCOMPILESHADERPROC glCompileShader = NULL;
PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
PFNGLATTACHSHADERPROC glAttachShader = NULL;
PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
PFNGLUSEPROGRAMPROC glUseProgram = NULL;
PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = NULL;
PFNGLUNIFORM3FPROC glUniform3f = NULL;
PFNGLUNIFORM1IPROC glUniform1i = NULL;
PFNGLPRIMITIVERESTARTINDEXPROC glPrimitiveRestartIndex = NULL;
PFNGLACTIVETEXTUREPROC glActiveTextureExt = NULL;

#define LOAD_GL(name, type) \
    name = (type)SDL_GL_GetProcAddress(#name); \
    if (!name) { \
        fprintf(stderr, "[GL-LOADER] ERROR: Failed to load function: %s\n", #name); \
        return -1; \
    }

int gl_loader_init(void) {
    LOAD_GL(glGenBuffers, PFNGLGENBUFFERSPROC);
    LOAD_GL(glBindBuffer, PFNGLBINDBUFFERPROC);
    LOAD_GL(glBufferData, PFNGLBUFFERDATAPROC);
    LOAD_GL(glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC);
    LOAD_GL(glBindVertexArray, PFNGLBINDVERTEXARRAYPROC);
    LOAD_GL(glCreateShader, PFNGLCREATESHADERPROC);
    LOAD_GL(glShaderSource, PFNGLSHADERSOURCEPROC);
    LOAD_GL(glCompileShader, PFNGLCOMPILESHADERPROC);
    LOAD_GL(glCreateProgram, PFNGLCREATEPROGRAMPROC);
    LOAD_GL(glAttachShader, PFNGLATTACHSHADERPROC);
    LOAD_GL(glLinkProgram, PFNGLLINKPROGRAMPROC);
    LOAD_GL(glUseProgram, PFNGLUSEPROGRAMPROC);
    LOAD_GL(glGetShaderiv, PFNGLGETSHADERIVPROC);
    LOAD_GL(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);
    LOAD_GL(glGetProgramiv, PFNGLGETPROGRAMIVPROC);
    LOAD_GL(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
    LOAD_GL(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);
    LOAD_GL(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
    LOAD_GL(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
    LOAD_GL(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC);
    LOAD_GL(glUniform3f, PFNGLUNIFORM3FPROC);
    LOAD_GL(glUniform1i, PFNGLUNIFORM1IPROC);
    LOAD_GL(glPrimitiveRestartIndex, PFNGLPRIMITIVERESTARTINDEXPROC);
    glActiveTextureExt = (PFNGLACTIVETEXTUREPROC)SDL_GL_GetProcAddress("glActiveTexture");
    if (!glActiveTextureExt) {
        fprintf(stderr, "[GL-LOADER] ERROR: Failed to load function: glActiveTexture\n");
        return -1;
    }
    return 0;
}
