#include "mui/mui.h"
#include "plat_video.h"

static mui_t          mui;
static mui_drawable_t dr;
static mui_window_t*  menubar;

mui_menu_item_t m_machine_menu[] = {
    { .title = "Reset", .uid = FCC('r','e','s','e')},
    { }
};

void overlay_init()
{
    mui_init(&mui);
    mui.screen_size = C2_PT(1024, 768);
    mui.clear_color = (mui_color_t){ .r = 255, .g = 255, .b = 255, .a = 0 };
    menubar = mui_menubar_new(&mui);
    mui_menubar_add_simple(menubar, "Machine", FCC('M', 'a', 'c', 'h'), m_machine_menu);

    dr.pix.size.x = 1024;
    dr.pix.size.y = 768;
    dr.pix.bpp = 32;
    dr.pix.row_bytes = dr.pix.size.x * (dr.pix.bpp / 8);
    dr.pix.pixels = malloc(dr.pix.row_bytes * dr.pix.size.y);
    if (!dr.pix.pixels)
        abort();
}

int overlay_click(int x, int y, int button, int down)
{
    mui_event_t ev = {
        .type = down ? MUI_EVENT_BUTTONDOWN : MUI_EVENT_BUTTONUP,
        .mouse.where = { .x = x, .y = y },
    };
    return mui_handle_event(&mui, &ev);
}

int overlay_key(int key, int down, int mods)
{
    return 0;
}

void overlay_run()
{
    mui_run(&mui);
}

void overlay_draw()
{
    mui_draw(&mui, &dr, 1);
    video_renderer_update_overlay(dr.pix.pixels, 0, 0, 0, 0, 1024, 768);
}