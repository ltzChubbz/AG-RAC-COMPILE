#include <SDL.h>
#include <math.h>
#include <stdio.h>

#include "window.h"
#include "pc/hal/pad/pad.h"
#include "pc/hal/gs/gs.h"
#include "pc/hal/gs/gl_loader.h"
#include "src/engine/ui/dashboard.h"
#include "src/engine/wad.h"
#include "src/renderer/mesh.h"
#include "src/renderer/tex.h"
#include "src/engine/math/math.h"

#ifdef TARGET_PC

typedef struct WindowState {
    SDL_Window   *window;
    SDL_GLContext context;
    int           running;
    float         delta_time;
    
    /* Camera */
    float cam_x, cam_y, cam_z;
    float cam_yaw, cam_pitch;
    int   wireframe;
    
    int     mesh_idx;
    WadFile *wad;
    PcMesh  *mesh;
    u32     tex_id;
    
    /* Terrain Fragments */
    PcMesh  **tfrag_meshes;
    u32     tfrag_count;
} WindowState;

static WindowState g_window = {0};

/* Analog Deadzone Helper (±15-20 units) to stop camera wandering */
static float apply_deadzone(u8 val, u8 center, u8 deadzone) {
    int diff = (int)val - (int)center;
    if (diff < -deadzone) return (diff + deadzone) / (float)(center - deadzone);
    if (diff >  deadzone) return (diff - deadzone) / (float)(255 - center - deadzone);
    return 0.0f;
}

int window_init(int scale) {
    (void)scale;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("[WINDOW] SDL init failed: %s\n", SDL_GetError());
        return -1;
    }

    g_window.window = SDL_CreateWindow("AG-RAC - Ratchet & Clank", 
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      1536, 1344, SDL_WINDOW_OPENGL);
    
    g_window.context = SDL_GL_CreateContext(g_window.window);
    gl_loader_init();
    pad_hal_init();
    
    dashboard_init();
    g_window.running = 1;
    g_window.cam_x = 0; g_window.cam_y = 0; g_window.cam_z = -10.0f;
    g_window.wireframe = 0;
    
    /* Load default level assets (Veldin Planet 0) */
    g_window.wad = wad_load("assets/wads/wad_106.bin");
    if (g_window.wad) {
        if (g_window.wad->chunk_count > 1000) {
            g_window.mesh_idx = 1003; /* Reset to confirmed high-density sector */
            g_window.mesh = mesh_from_chunk(&g_window.wad->chunks[g_window.mesh_idx], g_window.wad->data);
            
            /* Apply the Veldin Palette + Texture found via Forensics */
            const u8 *palette = &g_window.wad->data[0x42C400];
            const u8 *pixels  = &g_window.wad->data[0x42CC00];
            g_window.tex_id   = tex_from_wad(palette, pixels, 256, 256);
        }
        
        /* Load TFrags if core data is decompressed successfully */
        if (g_window.wad->core_data && g_window.wad->core_header_offset != 0xFFFFFFFF) {
            u32 base = g_window.wad->core_header_offset;
            u32 tfrags_off = *(u32*)&g_window.wad->decompressed_data[base + 8];
            
            printf("[WINDOW] Loading TFrags from core offset 0x%X...\n", tfrags_off);
            fflush(stdout);
            
            typedef struct {
                s32 table_offset;
                s32 count;
                f32 thingy;
                u32 mysterious;
            } TfragsHeader;
            
            typedef struct {
                float bsphere[4];
                s32 data_offset;
                u16 lod2_ofs;
                u16 shared_ofs;
                u8 padding[40];
            } TfragHeader;
            
            if (tfrags_off + sizeof(TfragsHeader) <= g_window.wad->core_data_size) {
                TfragsHeader *tf_hdr = (TfragsHeader*)&g_window.wad->core_data[tfrags_off];
                TfragHeader *tf_array = (TfragHeader*)((u8*)tf_hdr + tf_hdr->table_offset);
                
                g_window.tfrag_count = tf_hdr->count;
                printf("[WINDOW] Found %u terrain fragments in core header\n", g_window.tfrag_count);
                fflush(stdout);
                
                g_window.tfrag_meshes = calloc(g_window.tfrag_count, sizeof(PcMesh*));
                for (u32 i = 0; i < g_window.tfrag_count; i++) {
                    u32 data_off = tf_array[i].data_offset;
                    u32 next_off = (i < g_window.tfrag_count - 1) ? tf_array[i+1].data_offset : g_window.wad->core_data_size;
                    
                    if (data_off < g_window.wad->core_data_size && next_off <= g_window.wad->core_data_size && next_off > data_off) {
                        u32 size = next_off - data_off;
                        g_window.tfrag_meshes[i] = mesh_from_tfrag(&g_window.wad->core_data[data_off], size);
                    }
                }
            }
        }
    }
    return 0;
}

void window_request_quit(void) {
    g_window.running = 0;
}

void window_run(void) {
    printf("AG-RAC v21 HEARTBEAT\n");
    u64 last_time = SDL_GetPerformanceCounter();
    u64 freq = SDL_GetPerformanceFrequency();

    while (g_window.running) {
        u64 now = SDL_GetPerformanceCounter();
        g_window.delta_time = (float)(now - last_time) / freq;
        if (g_window.delta_time > 0.1f) g_window.delta_time = 0.1f;
        last_time = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) window_request_quit();
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) window_request_quit();
            }
            else if (event.type == SDL_CONTROLLERDEVICEADDED) {
                pad_hal_handle_device_added(event.cdevice.which);
            }
            else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                pad_hal_handle_device_removed(event.cdevice.which);
            }
        }

            /* ── Update ─────────────────────────────────────────────────────── */
        pad_hal_update();
        
        const PadData *pad = pad_hal_get_data(0);
        const PadData *prev = pad_hal_get_prev_data(0);
        if (pad && prev) {
            u16 b = ~pad->buttons;
            u16 p = ~prev->buttons;

            /* Turbo (R2) and Slow (L2) multipliers */
            float speed_mult = 1.0f;
            if (b & PAD_R2) speed_mult = 5.0f;
            if (b & PAD_L2) speed_mult = 0.2f;
            
            float speed = 2.0f * speed_mult * g_window.delta_time;

            /* Deadzoned Stick Reading (Prevents drifting) */
            float rx = apply_deadzone(pad->rx, 128, 20);
            float ry = apply_deadzone(pad->ry, 128, 20);
            float lx = apply_deadzone(pad->lx, 128, 20);
            float ly = apply_deadzone(pad->ly, 128, 20);

            /* Mesh Browser (L1/R1) */
            int next_idx = g_window.mesh_idx;
            if ((b & PAD_R1) && !(p & PAD_R1)) next_idx++;
            if ((b & PAD_L1) && !(p & PAD_L1)) next_idx--;
            
            if (next_idx != g_window.mesh_idx && g_window.wad) {
                if (next_idx < 0) next_idx = 0;
                if (next_idx >= (int)g_window.wad->chunk_count) next_idx = g_window.wad->chunk_count - 1;
                
                g_window.mesh_idx = next_idx;
                printf("[CHUNK-BROWSER] Planet: Veldin | Sector: %d\n", g_window.mesh_idx);
                /* Note: In a production build, we would free the old mesh here */
                g_window.mesh = mesh_from_chunk(&g_window.wad->chunks[g_window.mesh_idx], g_window.wad->data);
            }

            /* L3 Reset shortcut */
            if (b & PAD_L3) {
                g_window.cam_x = 0; g_window.cam_y = 0; g_window.cam_z = -10;
                g_window.cam_yaw = 0; g_window.cam_pitch = 0;
            }

            /* Camera Orientation Logic */
            float sens = 2.0f * g_window.delta_time;
            g_window.cam_yaw   += rx * sens;
            g_window.cam_pitch += -ry * sens;

            if (g_window.cam_pitch > 1.5f) g_window.cam_pitch = 1.5f;
            if (g_window.cam_pitch < -1.5f) g_window.cam_pitch = -1.5f;

            float fwd_x = sinf(g_window.cam_yaw) * cosf(g_window.cam_pitch);
            float fwd_y = sinf(g_window.cam_pitch);
            float fwd_z = -cosf(g_window.cam_yaw) * cosf(g_window.cam_pitch);
            
            float right_x = cosf(g_window.cam_yaw);
            float right_z = sinf(g_window.cam_yaw);

            /* Fly movement aligned to look direction */
            g_window.cam_x += (fwd_x * -ly + right_x * lx) * speed;
            g_window.cam_y += fwd_y * -ly * speed;
            g_window.cam_z += (fwd_z * -ly + right_z * lx) * speed;

            /* Debug Toggles */
            if ((b & PAD_TRIANGLE) && !(p & PAD_TRIANGLE)) {
                g_window.wireframe = !g_window.wireframe;
            }
        }

        GameUpdate();

        /* ── Render ─────────────────────────────────────────────────────── */
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        GameRender();
        
        if (g_window.mesh || g_window.tfrag_meshes) {
            Vec3 cam_pos = { g_window.cam_x, g_window.cam_y, g_window.cam_z };
            Mat4 viewM = Mat4ViewFPS(cam_pos, g_window.cam_yaw, g_window.cam_pitch);
            
            glPolygonMode(GL_FRONT_AND_BACK, g_window.wireframe ? GL_LINE : GL_FILL);
            
            /* Bind the current level texture */
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, g_window.tex_id);
            
            if (g_window.mesh) {
                mesh_draw(g_window.mesh, &viewM.m[0][0], g_window.cam_x, g_window.cam_y, g_window.cam_z); 
            }
            
            if (g_window.tfrag_meshes) {
                for (u32 i = 0; i < g_window.tfrag_count; i++) {
                    if (g_window.tfrag_meshes[i]) {
                        mesh_draw(g_window.tfrag_meshes[i], &viewM.m[0][0], g_window.cam_x, g_window.cam_y, g_window.cam_z);
                    }
                }
            }
            
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        
        dashboard_render();
        SDL_GL_SwapWindow(g_window.window);
    }
}

void window_shutdown(void) {
    dashboard_shutdown();
    if (g_window.tfrag_meshes) {
        for (u32 i = 0; i < g_window.tfrag_count; i++) {
            if (g_window.tfrag_meshes[i]) {
                free(g_window.tfrag_meshes[i]);
            }
        }
        free(g_window.tfrag_meshes);
    }
    if (g_window.wad) {
        wad_free(g_window.wad);
    }
    SDL_GL_DeleteContext(g_window.context);
    SDL_DestroyWindow(g_window.window);
    SDL_Quit();
}

#endif
