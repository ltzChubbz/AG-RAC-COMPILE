#include "mesh.h"
#include "../../pc/hal/gs/gl_loader.h"
#include "src/engine/math/math.h"
#include "vif.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct Vertex {
    f32 x, y, z;
    f32 nx, ny, nz;
    f32 u, v;
} Vertex;

static const char *WORLD_VS = 
    "#version 460 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNorm;\n"
    "layout (location = 2) in vec2 aTex;\n"
    "uniform mat4 uProj;\n"
    "uniform mat4 uView;\n"
    "out vec3 vPos;\n"
    "out vec2 vTex;\n"
    "void main() {\n"
    "   gl_Position = uProj * uView * vec4(aPos, 1.0);\n"
    "   vPos = aPos;\n"
    "   vTex = aTex;\n"
    "}\n";

static const char *WORLD_FS = 
    "#version 460 core\n"
    "in vec3 vPos;\n"
    "in vec2 vTex;\n"
    "uniform sampler2D uTex;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "   FragColor = texture(uTex, vTex);\n"
    "}\n";

static u32 g_world_program = 0;

static void check_shader_errors(u32 shader, const char *type) {
    s32 success;
    char info_log[1024];
    if (strcmp(type, "PROGRAM") != 0) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, info_log);
            printf("[SHADER-ERROR] %s Compilation Failed:\n%s\n", type, info_log);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, info_log);
            printf("[SHADER-ERROR] Program Linking Failed:\n%s\n", info_log);
        }
    }
}

static u32 compile_shader(const char *vs, const char *fs) {
    u32 v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vs, NULL);
    glCompileShader(v);
    check_shader_errors(v, "VERTEX");

    u32 f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fs, NULL);
    glCompileShader(f);
    check_shader_errors(f, "FRAGMENT");

    u32 p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    check_shader_errors(p, "PROGRAM");
    return p;
}

PcMesh* mesh_from_chunk(const WadChunk *chunk, const u8 *wad_data) {
    PcMesh *m = calloc(1, sizeof(PcMesh));
    
    u32 max_v = 10000;
    VifVertex *vif_verts = calloc(max_v, sizeof(VifVertex));
    
    /* Use the new VIF Parser to decode the sector */
    u32 count = vif_parse_sector(&wad_data[chunk->offset], 2048, vif_verts, max_v);
    
    if (count == 0) {
        free(vif_verts);
        free(m);
        return NULL;
    }

    Vertex *verts = malloc(sizeof(Vertex) * count);
    u16 *indices = malloc(sizeof(u16) * (count * 4 + 4));
    u32 idx_ptr = 0;
    
    for (u32 i = 0; i < count; i++) {
        float scale = 1.0f / 1024.0f;
        verts[i].x = vif_verts[i].x * scale;
        verts[i].y = vif_verts[i].y * scale;
        verts[i].z = vif_verts[i].z * scale;
        verts[i].u = vif_verts[i].u;
        verts[i].v = vif_verts[i].v;
        verts[i].nx = 0.0f; verts[i].ny = 1.0f; verts[i].nz = 0.0f;

        /* 
         * ADC Bit Logic (Bit 15):
         * If ADC is 0, this vertex marks a 'break' in the strip.
         */
        if (i >= 2 && (vif_verts[i].flag & 0x8000) == 0) {
             indices[idx_ptr++] = 0xFFFF;
             indices[idx_ptr++] = (u16)(i-1); // Restart strip with prev two
             indices[idx_ptr++] = (u16)i;
        }
        indices[idx_ptr++] = (u16)i;
    }

    glGenVertexArrays(1, &m->vao);
    glGenVertexArrays(1, &m->vao);
    glGenBuffers(1, &m->vbo);
    glGenBuffers(1, &m->ebo);

    glBindVertexArray(m->vao);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(Vertex), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx_ptr * sizeof(u16), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(f32)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(f32)));
    glEnableVertexAttribArray(2);

    m->vert_count = count;
    m->index_count = idx_ptr;
    m->initialized = 1;
    
    if (g_world_program == 0) {
        g_world_program = compile_shader(WORLD_VS, WORLD_FS);
    }

    free(vif_verts);
    free(verts);
    free(indices);
    return m;
}

PcMesh* mesh_from_tfrag(const u8 *vif_data, u32 vif_size) {
    PcMesh *m = calloc(1, sizeof(PcMesh));
    
    u32 max_v = 10000;
    VifVertex *vif_verts = calloc(max_v, sizeof(VifVertex));
    
    /* Use VIF Parser to decode the TFrag payload */
    u32 count = vif_parse_sector(vif_data, vif_size, vif_verts, max_v);
    
    if (count == 0) {
        free(vif_verts);
        free(m);
        return NULL;
    }

    Vertex *verts = malloc(sizeof(Vertex) * count);
    u16 *indices = malloc(sizeof(u16) * (count * 4 + 4));
    u32 idx_ptr = 0;
    
    for (u32 i = 0; i < count; i++) {
        float scale = 1.0f / 1024.0f;
        verts[i].x = vif_verts[i].x * scale;
        verts[i].y = vif_verts[i].y * scale;
        verts[i].z = vif_verts[i].z * scale;
        verts[i].u = vif_verts[i].u;
        verts[i].v = vif_verts[i].v;
        verts[i].nx = 0.0f; verts[i].ny = 1.0f; verts[i].nz = 0.0f;

        /* ADC Bit Logic (Bit 15) */
        if (i >= 2 && (vif_verts[i].flag & 0x8000) == 0) {
             indices[idx_ptr++] = 0xFFFF;
             indices[idx_ptr++] = (u16)(i-1); // Restart strip with prev two
             indices[idx_ptr++] = (u16)i;
        }
        indices[idx_ptr++] = (u16)i;
    }

    glGenVertexArrays(1, &m->vao);
    glGenBuffers(1, &m->vbo);
    glGenBuffers(1, &m->ebo);

    glBindVertexArray(m->vao);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(Vertex), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx_ptr * sizeof(u16), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(f32)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(f32)));
    glEnableVertexAttribArray(2);

    m->vert_count = count;
    m->index_count = idx_ptr;
    m->initialized = 1;
    
    if (g_world_program == 0) {
        g_world_program = compile_shader(WORLD_VS, WORLD_FS);
    }

    free(vif_verts);
    free(verts);
    free(indices);
    return m;
}

void mesh_draw(PcMesh *mesh, float *view_matrix, float cam_x, float cam_y, float cam_z) {
    if (!mesh || !mesh->initialized) return;
    
    u32 p = g_world_program;
    glUseProgram(p);

    u32 uProj = glGetUniformLocation(p, "uProj");
    u32 uView = glGetUniformLocation(p, "uView");
    u32 uTex  = glGetUniformLocation(p, "uTex");
    glUniform1i(uTex, 0); // Use GL_TEXTURE0

    f32 fov = 60.0f * (3.14159f / 180.0f);
    f32 aspect = 1536.0f / 1344.0f;
    Mat4 projM = Mat4Perspective(fov, aspect, 0.1f, 2000.0f);
    
    glUniformMatrix4fv(uProj, 1, GL_FALSE, &projM.m[0][0]);
    if (view_matrix) {
        glUniformMatrix4fv(uView, 1, GL_FALSE, view_matrix);
    }
    
    /* glUniform3f(uCamPos, cam_x, cam_y, cam_z); - Removed (No longer in shader) */
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFF);
    
    glEnable(GL_DEPTH_TEST); 
    glDisable(GL_CULL_FACE);

    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLE_STRIP, mesh->index_count, GL_UNSIGNED_SHORT, 0);
}
