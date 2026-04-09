/**
 * iop.c — PS2 IOP HAL Implementation (stub)
 *
 * Redirects PS2 IOP calls to PC-side equivalents:
 *   CD/DVD file I/O → fopen/fread on extracted assets directory
 *   SPU2 audio      → SDL2 audio (VAG ADPCM decoder TBD)
 *   Memory card     → Platform save file
 */

#include "iop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TARGET_PC

/* Max open file handles (matching a typical game limit) */
#define MAX_FILE_HANDLES 32

/* ── Internal State ─────────────────────────────────────────────────────────── */

typedef struct IopState {
    char   assets_root[512];
    FILE  *file_handles[MAX_FILE_HANDLES];
    int    initialized;
} IopState;

static IopState g_iop = {0};

/* ── Path Translation ────────────────────────────────────────────────────────── */

/**
 * Translate a PS2 CD path to a local filesystem path.
 * PS2 paths look like: "cdrom0:\\WADS\\LEVEL01.WAD;1" or "host:\\FILE.EXT"
 * We strip the prefix and version suffix, then prepend the assets root.
 */
static void translate_path(const char *ps2_path, char *out, u32 out_size) {
    const char *p = ps2_path;

    /* Strip device prefix (cdrom0:\, host:\, mc0:\, etc.) */
    const char *backslash = strrchr(p, '\\');
    const char *slash     = strrchr(p, '/');
    if (backslash && backslash > slash) slash = backslash;

    /* Find the filename portion */
    const char *filename = (slash) ? (slash + 1) : p;

    /* Strip ISO version suffix ";1" */
    char filename_clean[256];
    strncpy(filename_clean, filename, sizeof(filename_clean) - 1);
    filename_clean[sizeof(filename_clean) - 1] = '\0';
    char *semicolon = strchr(filename_clean, ';');
    if (semicolon) *semicolon = '\0';

    /* Build final path */
    snprintf(out, out_size, "%s/%s", g_iop.assets_root, filename_clean);
}

/* ── File I/O Implementation ─────────────────────────────────────────────────── */

int iop_file_open(const char *path, int flags) {
    (void)flags;  /* TODO: handle read/write flags */

    char local_path[512];
    translate_path(path, local_path, sizeof(local_path));

    /* Find a free handle slot */
    int fd = -1;
    for (int i = 0; i < MAX_FILE_HANDLES; i++) {
        if (!g_iop.file_handles[i]) { fd = i; break; }
    }
    if (fd < 0) {
        fprintf(stderr, "[IOP-HAL] No free file handles for: %s\n", path);
        return -1;
    }

    g_iop.file_handles[fd] = fopen(local_path, "rb");
    if (!g_iop.file_handles[fd]) {
        fprintf(stderr, "[IOP-HAL] fopen failed: %s (translated from %s)\n",
                local_path, path);
        return -1;
    }

    return fd;
}

int iop_file_read(int fd, void *buf, u32 size) {
    if (fd < 0 || fd >= MAX_FILE_HANDLES || !g_iop.file_handles[fd]) return -1;
    return (int)fread(buf, 1, size, g_iop.file_handles[fd]);
}

int iop_file_seek(int fd, s32 offset, int whence) {
    if (fd < 0 || fd >= MAX_FILE_HANDLES || !g_iop.file_handles[fd]) return -1;
    return fseek(g_iop.file_handles[fd], offset, whence);
}

int iop_file_close(int fd) {
    if (fd < 0 || fd >= MAX_FILE_HANDLES || !g_iop.file_handles[fd]) return -1;
    fclose(g_iop.file_handles[fd]);
    g_iop.file_handles[fd] = NULL;
    return 0;
}

int iop_file_exists(const char *path) {
    char local_path[512];
    translate_path(path, local_path, sizeof(local_path));
    FILE *f = fopen(local_path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* ── Audio Implementation (stub) ─────────────────────────────────────────────── */

void iop_audio_init(void) {
    /*
     * TODO: SDL_InitSubSystem(SDL_INIT_AUDIO);
     *       SDL_OpenAudioDevice(...);
     * VAG ADPCM decoder will be implemented here.
     * Reference decoder: https://github.com/simias/psx-rs (PlayStation ADPCM)
     */
    printf("[IOP-HAL] Audio init (stub)\n");
}

void iop_audio_shutdown(void) {
    /* TODO: SDL_CloseAudioDevice */
}

int iop_audio_load_vag(const void *data, u32 size) {
    (void)data; (void)size;
    /* TODO: Decode VAG header, decode ADPCM, return handle */
    return -1;
}

void iop_audio_play(int handle, int loop, u8 volume) {
    (void)handle; (void)loop; (void)volume;
    /* TODO: Queue decoded PCM to SDL2 audio device */
}

void iop_audio_stop(int handle) {
    (void)handle;
    /* TODO: Stop the SDL2 audio channel */
}

/* ── Memory Card Implementation ─────────────────────────────────────────────── */

static const char *get_save_path(void) {
    /* TODO: Use platform-appropriate path:
     *   Windows: %APPDATA%\AG-RAC\save.dat
     *   Linux:   ~/.local/share/ag-rac/save.dat
     *   macOS:   ~/Library/Application Support/AG-RAC/save.dat
     */
    return "ag_rac_save.dat";  /* Fallback: current directory */
}

int iop_memcard_save(const void *data, u32 size) {
    const char *path = get_save_path();
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    return (written == size) ? 0 : -1;
}

int iop_memcard_load(void *data, u32 max_size) {
    const char *path = get_save_path();
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    int bytes = (int)fread(data, 1, max_size, f);
    fclose(f);
    return bytes;
}

/* ── Init/Shutdown ─────────────────────────────────────────────────────────── */

void iop_hal_init(const char *assets_root) {
    memset(&g_iop, 0, sizeof(g_iop));
    strncpy(g_iop.assets_root, assets_root, sizeof(g_iop.assets_root) - 1);
    g_iop.initialized = 1;

    iop_audio_init();

    printf("[IOP-HAL] Initialized — assets root: %s\n", assets_root);
}

void iop_hal_shutdown(void) {
    /* Close any open file handles */
    for (int i = 0; i < MAX_FILE_HANDLES; i++) {
        if (g_iop.file_handles[i]) {
            fclose(g_iop.file_handles[i]);
            g_iop.file_handles[i] = NULL;
        }
    }
    iop_audio_shutdown();
    memset(&g_iop, 0, sizeof(g_iop));
}

#endif /* TARGET_PC */
