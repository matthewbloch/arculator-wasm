#include <SDL2/SDL.h>

/* Initialise UI, or change its canvas size */
void overlay_init(int w, int h);
void overlay_set_size(int w, int h);

/* Pass events through to UI (TODO: scale co-ordinates?) */
int overlay_event(SDL_Event *e);

/* Process events for the overlay */
void overlay_run();

/* Redraw the overlay using video_renderer_update_overlay */
void overlay_draw();
