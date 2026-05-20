#include <SDL.h>
#include "window.h"
#include <stdio.h>

#undef main

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("  AG-RAC — Ratchet & Clank Native PC Port\n");
    printf("  =========================================\n");
    
    if (window_init(3) < 0) {
        return -1;
    }
    
    GameInit();
    
    printf("[WINDOW] Starting main loop\n");
    window_run();
    
    GameShutdown();
    window_shutdown();
    
    return 0;
}
