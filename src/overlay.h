/* */
void overlay_init();

/* Pass mouse clicks through to the UI */
int overlay_click(int x, int y, int button);
int overlay_key(int key, int action, int mods);

/* Process events for the overlay */
void overlay_run();

/* Redraw the overlay using video_renderer_update_overlay */
void overlay_draw();
