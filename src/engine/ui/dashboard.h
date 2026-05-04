#ifndef AG_RAC_ENGINE_UI_DASHBOARD_H
#define AG_RAC_ENGINE_UI_DASHBOARD_H

#include "types.h"

/**
 * dashboard_init() — Initialize the dashboard UI resources.
 */
void dashboard_init(void);

/**
 * dashboard_update() — Update UI logic (animation, input tracking).
 */
void dashboard_update(void);

/**
 * dashboard_render() — Draw the dashboard to the screen.
 */
void dashboard_render(void);

/**
 * dashboard_shutdown() — Cleanup UI resources.
 */
void dashboard_shutdown(void);

#endif /* AG_RAC_ENGINE_UI_DASHBOARD_H */
