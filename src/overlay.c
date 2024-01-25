#include "arc.h"
#include "mui/mui.h"
#include "plat_video.h"

static mui_t          mui;
static mui_drawable_t dr;
static mui_window_t*  menubar;

mui_menu_item_t m_machine_menu[] = {
    { .title = "Reset", .uid = FCC('r','e','s','e')},
    { }
};

#endif

void overlay_set_size(int w, int h)
{
    if (w < 1 || h < 1)
        abort();
    mui.screen_size = C2_PT(w, h);

    dr.pix.size.x = w;
    dr.pix.size.y = h;
    dr.pix.bpp = 32;
    dr.pix.row_bytes = dr.pix.size.x * (dr.pix.bpp / 8);
    dr.pix.pixels = realloc(dr.pix.pixels, dr.pix.row_bytes * dr.pix.size.y);

    rpclog("mui size set to %d,%d 0x%p\n", w, h, dr.pix.pixels);
    if (!dr.pix.pixels)
        abort();
}

void overlay_init(int w, int h)
{
    mui_init(&mui);
    mui.clear_color = (mui_color_t){.r = 255, .g = 255, .b = 255, .a = 0};
    dr.pix.pixels = NULL;
    overlay_set_size(w, h);
    menubar = mui_menubar_new(&mui);
    mui_menubar_add_simple(menubar, "Machine", FCC('M', 'a', 'c', 'h'), m_machine_menu);
}

void overlay_run()
{
    mui_run(&mui);
}

void overlay_draw()
{
    mui_draw(&mui, &dr, 1);
    video_renderer_update_overlay(dr.pix.pixels, 0, 0, 0, 0, dr.pix.size.x, dr.pix.size.y);
}

#include <SDL2/SDL_events.h>

static uint32_t keycode_convert_to_mui(SDL_Keysym sym)
{
    switch (sym.scancode)
    {
    case SDL_SCANCODE_F1 ... SDL_SCANCODE_F12:
        return MUI_KEY_F1 + (sym.scancode - SDL_SCANCODE_F1);
    case SDL_SCANCODE_ESCAPE:
        return MUI_KEY_ESCAPE;
    case SDL_SCANCODE_LEFT:
        return MUI_KEY_LEFT;
    case SDL_SCANCODE_UP:
        return MUI_KEY_UP;
    case SDL_SCANCODE_RIGHT:
        return MUI_KEY_RIGHT;
    case SDL_SCANCODE_DOWN:
        return MUI_KEY_DOWN;
    // XK_Begin
    case SDL_SCANCODE_INSERT:
        return MUI_KEY_INSERT;
    case SDL_SCANCODE_HOME:
        return MUI_KEY_HOME;
    case SDL_SCANCODE_END:
        return MUI_KEY_END;
    case SDL_SCANCODE_PAGEUP:
        return MUI_KEY_PAGEUP;
    case SDL_SCANCODE_PAGEDOWN:
        return MUI_KEY_PAGEDOWN;
    case SDL_SCANCODE_RSHIFT:
        return MUI_KEY_RSHIFT;
    case SDL_SCANCODE_LSHIFT:
        return MUI_KEY_LSHIFT;
    case SDL_SCANCODE_RCTRL:
        return MUI_KEY_RCTRL;
    case SDL_SCANCODE_LCTRL:
        return MUI_KEY_LCTRL;
    case SDL_SCANCODE_LALT:
        return MUI_KEY_LALT;
    case SDL_SCANCODE_RALT:
        return MUI_KEY_RALT;
    case SDL_SCANCODE_LGUI:
        return MUI_KEY_LSUPER;
    case SDL_SCANCODE_RGUI:
        return MUI_KEY_RSUPER;
    default:
        return sym.scancode & 0xff;
    }
}

bool overlay_event(SDL_Event *e)
{
    int x, y;
    SDL_GetMouseState(&x, &y);

    switch (e->type)
    {
    case SDL_WINDOWEVENT: {
        switch (e->window.event)
        {
        case SDL_WINDOWEVENT_FOCUS_LOST:
            mui.modifier_keys = 0;
            return false;
        }
        return false;
    }
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        int down = e->type == SDL_KEYDOWN;
        mui_event_t ev = {
            .type = down ? MUI_EVENT_KEYDOWN : MUI_EVENT_KEYUP,
            .key.key = keycode_convert_to_mui(e->key.keysym),
            .key.up = 0
        };
        bool is_modifier = ev.key.key >= MUI_KEY_MODIFIERS &&
                           ev.key.key <= MUI_KEY_MODIFIERS_LAST;
        if (is_modifier)
        {
            if (down)
                mui.modifier_keys |= (1 << (ev.key.key - MUI_KEY_MODIFIERS));
            else
                mui.modifier_keys &= ~(1 << (ev.key.key - MUI_KEY_MODIFIERS));
        }
        ev.modifiers = mui.modifier_keys;
        return mui_handle_event(&mui, &ev);
    }
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        if (e->button.button == SDL_BUTTON_LEFT) {
            mui_event_t ev = {
                .type = (e->button.state == SDL_PRESSED) ? MUI_EVENT_BUTTONDOWN : MUI_EVENT_BUTTONUP,
                .mouse.where.x = e->button.x,
                .mouse.where.y = e->button.y,
                .modifiers = mui.modifier_keys, // | MUI_MODIFIER_EVENT_TRACE,
            };
            return mui_handle_event(&mui, &ev);
        }
        return false;
    }
    case SDL_MOUSEWHEEL: {
        mui_event_t ev = {
            .type = MUI_EVENT_WHEEL,
            .modifiers = mui.modifier_keys, // | MUI_MODIFIER_EVENT_TRACE,
            .wheel.where.x = x,
            .wheel.where.y = y,
            .wheel.delta = e->wheel.y
        };
        return mui_handle_event(&mui, &ev);
    }
    case SDL_MOUSEMOTION: {
        mui_event_t ev = {
            .type = MUI_EVENT_DRAG,
            .mouse.where.x = e->motion.x,
            .mouse.where.y = e->motion.y,
            .modifiers = mui.modifier_keys,
        };
        return mui_handle_event(&mui, &ev);
    }
    }

    return false;
}