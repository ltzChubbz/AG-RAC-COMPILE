#include "dashboard.h"
#include "../../pc/hal/gs/gl_loader.h"
#include "../../pc/hal/pad/pad.h"
#include <stdio.h>

/* ── Shaders ────────────────────────────────────────────────────────────────── */

static const char *VS_SRC = 
    "#version 460 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec4 aCol;\n"
    "out vec4 vCol;\n"
    "void main() {\n"
    "   gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "   vCol = aCol;\n"
    "}\n";

static const char *FS_SRC = 
    "#version 460 core\n"
    "in vec4 vCol;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "   FragColor = vCol;\n"
    "}\n";

/* ── State ──────────────────────────────────────────────────────────────────── */

static u32 g_program = 0;
static u32 g_vao = 0, g_vbo = 0;

typedef struct GuiVertex {
    f32 x, y;
    f32 r, g, b, a;
} GuiVertex;

/* ── UI Components ─────────────────────────────────────────────────────────── */

static void draw_rect(GuiVertex *verts, int *idx, f32 x, f32 y, f32 w, f32 h, float r, float g, float b) {
    verts[(*idx)++] = (GuiVertex){x,   y,   r, g, b, 1.0f};
    verts[(*idx)++] = (GuiVertex){x+w, y,   r, g, b, 1.0f};
    verts[(*idx)++] = (GuiVertex){x,   y-h, r, g, b, 1.0f};
    
    verts[(*idx)++] = (GuiVertex){x+w, y,   r, g, b, 1.0f};
    verts[(*idx)++] = (GuiVertex){x+w, y-h, r, g, b, 1.0f};
    verts[(*idx)++] = (GuiVertex){x,   y-h, r, g, b, 1.0f};
}

void dashboard_init(void) {
    u32 vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &VS_SRC, NULL);
    glCompileShader(vs);

    u32 fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &FS_SRC, NULL);
    glCompileShader(fs);

    g_program = glCreateProgram();
    glAttachShader(g_program, vs);
    glAttachShader(g_program, fs);
    glLinkProgram(g_program);

    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);

    printf("[DASHBOARD] UI initialized\n");
}

void dashboard_render(void) {
    static GuiVertex verts[1024];
    int v_idx = 0;

    const PadData *pad = pad_hal_get_data(0);
    u16 b = pad ? ~pad->buttons : 0;

    /* ── Render Controller Visualization ── */
    /* Background rect */
    draw_rect(verts, &v_idx, -0.4f, -0.4f, 0.8f, 0.4f, 0.2f, 0.2f, 0.25f);

    /* DPAD */
    draw_rect(verts, &v_idx, -0.3f, -0.5f, 0.05f, 0.05f, (b & PAD_UP)   ? 0.9f : 0.4f, 0.4f, 0.4f);
    draw_rect(verts, &v_idx, -0.3f, -0.6f, 0.05f, 0.05f, (b & PAD_DOWN) ? 0.9f : 0.4f, 0.4f, 0.4f);
    draw_rect(verts, &v_idx, -0.35f,-0.55f, 0.05f, 0.05f,(b & PAD_LEFT) ? 0.9f : 0.4f, 0.4f, 0.4f);
    draw_rect(verts, &v_idx, -0.25f,-0.55f, 0.05f, 0.05f,(b & PAD_RIGHT)? 0.9f : 0.4f, 0.4f, 0.4f);

    /* Face Buttons */
    draw_rect(verts, &v_idx, 0.25f, -0.5f, 0.05f, 0.05f, (b & PAD_TRIANGLE) ? 0.2f : 0.4f, 0.9f, 0.2f);
    draw_rect(verts, &v_idx, 0.25f, -0.6f, 0.05f, 0.05f, (b & PAD_CROSS)    ? 0.2f : 0.4f, 0.2f, 0.9f);
    draw_rect(verts, &v_idx, 0.2f,  -0.55f, 0.05f, 0.05f,(b & PAD_SQUARE)   ? 0.9f : 0.4f, 0.2f, 0.9f);
    draw_rect(verts, &v_idx, 0.3f,  -0.55f, 0.05f, 0.05f,(b & PAD_CIRCLE)   ? 0.9f : 0.2f, 0.2f, 0.2f);

    /* Analog Sticks (Visual indicator) */
    f32 lx = pad ? (pad->lx - 128) / 128.0f * 0.1f : 0;
    f32 ly = pad ? (128 - pad->ly) / 128.0f * 0.1f : 0;
    draw_rect(verts, &v_idx, -0.15f + lx, -0.65f + ly, 0.04f, 0.04f, 0.8f, 0.8f, 0.8f);

    /* Render */
    glUseProgram(g_program);
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, v_idx * sizeof(GuiVertex), verts, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GuiVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GuiVertex), (void*)(sizeof(f32)*2));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES, 0, v_idx);
}

void dashboard_update(void) { }
void dashboard_shutdown(void) { }
