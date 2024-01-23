/*
 * mui.h
 *
 * Copyright (C) 2023 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * This is the main include file for the libmui UI library, it should be
 * the only one you need to include.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pixman.h>
#include "c2_arrays.h"
#include "bsd_queue.h"
#include "stb_ttc.h"

/* Four Character Constants are used everywhere. Wish this had become a standard,
 * as it is so handy -- but nope, thus the macro. Annoyingly, the little-
 * endianess of them makes it a pain to do a printf() with them, this is why
 * the values are reversed here.
 */
#ifndef FCC
#define FCC(_a,_b,_c,_d) (((_d)<<24)|((_c)<<16)|((_b)<<8)|(_a))
#endif

enum mui_event_e {
	MUI_EVENT_KEYUP = 0,
	MUI_EVENT_KEYDOWN,
	MUI_EVENT_BUTTONUP,
	MUI_EVENT_BUTTONDOWN,
	MUI_EVENT_WHEEL,
	MUI_EVENT_DRAG,
	// the following ones aren't supported yet
	MUI_EVENT_MOUSEENTER,
	MUI_EVENT_MOUSELEAVE,
	MUI_EVENT_RESIZE,
	MUI_EVENT_CLOSE,
	MUI_EVENT_COUNT,
};

enum mui_key_e {
	MUI_KEY_ESCAPE 	= 0x1b,
	MUI_KEY_LEFT 	= 0x80,
	MUI_KEY_UP,
	MUI_KEY_RIGHT,
	MUI_KEY_DOWN,
	MUI_KEY_INSERT,
	MUI_KEY_HOME,
	MUI_KEY_END,
	MUI_KEY_PAGEUP,
	MUI_KEY_PAGEDOWN,
	MUI_KEY_MODIFIERS = 0x90,
	MUI_KEY_LSHIFT 	= MUI_KEY_MODIFIERS,
	MUI_KEY_RSHIFT,
	MUI_KEY_LCTRL,
	MUI_KEY_RCTRL,
	MUI_KEY_LALT,
	MUI_KEY_RALT,
	MUI_KEY_RSUPER,
	MUI_KEY_LSUPER,
	MUI_KEY_MODIFIERS_LAST,
	MUI_KEY_F1 		= 0x100,
	MUI_KEY_F2,
	MUI_KEY_F3,
	MUI_KEY_F4,
	MUI_KEY_F5,
	MUI_KEY_F6,
	MUI_KEY_F7,
	MUI_KEY_F8,
	MUI_KEY_F9,
	MUI_KEY_F10,
	MUI_KEY_F11,
	MUI_KEY_F12,
};

enum mui_modifier_e {
	MUI_MODIFIER_LSHIFT 	= (1 << (MUI_KEY_LSHIFT - MUI_KEY_MODIFIERS)),
	MUI_MODIFIER_RSHIFT 	= (1 << (MUI_KEY_RSHIFT - MUI_KEY_MODIFIERS)),
	MUI_MODIFIER_LCTRL 		= (1 << (MUI_KEY_LCTRL - MUI_KEY_MODIFIERS)),
	MUI_MODIFIER_RCTRL 		= (1 << (MUI_KEY_RCTRL - MUI_KEY_MODIFIERS)),
	MUI_MODIFIER_LALT 		= (1 << (MUI_KEY_LALT - MUI_KEY_MODIFIERS)),
	MUI_MODIFIER_RALT 		= (1 << (MUI_KEY_RALT - MUI_KEY_MODIFIERS)),
	MUI_MODIFIER_RSUPER 	= (1 << (MUI_KEY_RSUPER - MUI_KEY_MODIFIERS)),
	MUI_MODIFIER_LSUPER 	= (1 << (MUI_KEY_LSUPER - MUI_KEY_MODIFIERS)),

	// special flag, trace events handling for this event
	MUI_MODIFIER_EVENT_TRACE= (1 << 15),
	MUI_MODIFIER_SHIFT 		= (MUI_MODIFIER_LSHIFT | MUI_MODIFIER_RSHIFT),
	MUI_MODIFIER_CTRL 		= (MUI_MODIFIER_LCTRL | MUI_MODIFIER_RCTRL),
	MUI_MODIFIER_ALT 		= (MUI_MODIFIER_LALT | MUI_MODIFIER_RALT),
	MUI_MODIFIER_SUPER 		= (MUI_MODIFIER_LSUPER | MUI_MODIFIER_RSUPER),
};

/*
 * The following constants are in UTF8 format, and relate to glyphs in
 * the TTF fonts
 */
/* These are from the icon font */
#define MUI_ICON_FOLDER 		""
#define MUI_ICON_FOLDER_OPEN 	""
#define MUI_ICON_ROOT 			""
#define MUI_ICON_FILE 			""
#define MUI_ICON_POPUP_ARROWS	""
#define MUI_ICON_HOME			""
#define MUI_ICON_SBAR_UP		""
#define MUI_ICON_SBAR_DOWN		""

/* These are specific the Charcoal System font */
#define MUI_GLYPH_APPLE 		""	// solid apple
#define MUI_GLYPH_OAPPLE 		""	// open apple
#define MUI_GLYPH_COMMAND 		""
#define MUI_GLYPH_OPTION 		""
#define MUI_GLYPH_CONTROL 		""
#define MUI_GLYPH_SHIFT 		""
#define MUI_GLYPH_TICK			""	// tickmark for menus
#define MUI_GLYPH_SUBMENU		""	// custom, for the hierarchical menus
#define MUI_GLYPH_IIE			""	// custom, IIe glyph
/* These are also from Charcoal System font (added to the original) */
#define MUI_GLYPH_F1			""
#define MUI_GLYPH_F2			""
#define MUI_GLYPH_F3			""
#define MUI_GLYPH_F4			""
#define MUI_GLYPH_F5			""
#define MUI_GLYPH_F6			""
#define MUI_GLYPH_F7			""
#define MUI_GLYPH_F8			""
#define MUI_GLYPH_F9			""
#define MUI_GLYPH_F10			""
#define MUI_GLYPH_F11			""
#define MUI_GLYPH_F12			""

typedef uint64_t mui_time_t;

/*
 * Even description. pretty standard stuff here -- the 'when' field is
 * only used really to detect double clicks so far.
 *
 * Even handlers should return true if the event was handled, (in which case
 * even processing stops for that event) or false to continue passing the even
 * down the chain.
 *
 * Events are passed to the top window first, and then down the chain of
 * windows, until one of them returns true.
 * Implicitely, it means the menubar gets to see the events first, even clicks,
 * even if the click wasn't in the menubar. This is also true of key events of
 * course, which allows the menu to detect key combos, first.
 */
typedef struct mui_event_t {
	uint8_t 			type;
	mui_time_t 			when;
	uint32_t 			modifiers;
	union {
		struct key {
			uint32_t 		key;
			bool 			up;
		} 				key;
		struct {
			uint32_t 		button;
			c2_pt_t 		where;
		} 				mouse;
		struct {
			int32_t 		delta;
			c2_pt_t 		where;
		} 				wheel;
	};
} mui_event_t;

/*
 * Key equivalent, used to match key events to menu items
 * Might be extended to controls, right now only the 'key' is checked,
 * mostly for Return and ESC.
 */
typedef union mui_key_equ_t {
	struct {
		uint16_t mod;
		uint16_t key;
	};
	uint32_t value;
} mui_key_equ_t;

#define MUI_KEY_EQU(_mask, _key) \
		(mui_key_equ_t){ .mod = (_mask), .key = (_key) }

struct mui_t;

typedef struct mui_listbox_elem_t {
	uint32_t 					disabled : 1;
	char 						icon[8];
	void * 						elem; // char * or... ?
} mui_listbox_elem_t;

DECLARE_C_ARRAY(mui_listbox_elem_t, mui_listbox_elems, 2);
IMPLEMENT_C_ARRAY(mui_listbox_elems);

struct mui_control_t;
struct mui_window_t;
struct mui_listbox_elem_t;

/*
 * Window DEFinition -- Handle all related to a window, from drawing to
 * even handling.
 */
enum {
	MUI_WDEF_INIT = 0,
	MUI_WDEF_DISPOSE,
	MUI_WDEF_DRAW,
	MUI_WDEF_EVENT,
};
typedef bool (*mui_wdef_p)(
			struct mui_window_t * win,
			uint8_t 		what,
			void * 			param);
enum {
	MUI_CDEF_INIT = 0,
	MUI_CDEF_DISPOSE,
	MUI_CDEF_DRAW,
	MUI_CDEF_EVENT,
	MUI_CDEF_SET_STATE,
	MUI_CDEF_SET_VALUE,
	MUI_CDEF_SET_TITLE,
	MUI_CDEF_SELECT,
};
typedef bool (*mui_cdef_p)(
				struct mui_control_t * 	c,
				uint8_t 	what,
				void * 		param);
typedef void (*mui_ldef_p)(
				struct mui_control_t * 	c,
				uint32_t 	elem_index,
				struct mui_listbox_elem_t * elem);

/*
 * Timer callback definition. Behaves in a pretty standard way; the timer
 * returns 0 to be cancelled (for one shot timers for example) or return
 * the delay to the next call.
 */
typedef mui_time_t (*mui_timer_p)(
				struct mui_t * mui,
				mui_time_t 	now,
				void * 		param);
/*
 * Actions are the provided way to add custom response to events for the
 * application; action handlers are called for a variety of things, from clicks
 * in controls, to menu selections, to window close etc.
 *
 * The 'what' parameter is a 4 character code, that can be used to identify
 * the action, and the 'param' is a pointer to a structure that depends on
 * the 'what' action (hopefully documented with that action constant)
 *
 * the 'cb_param' is specific to this action function pointer and is passed as
 * is to the callback.
 */
typedef int (*mui_window_action_p)(
				struct mui_window_t * win,
				void * 		cb_param,
				uint32_t 	what,
				void * 		param);
typedef int (*mui_control_action_p)(
				struct mui_control_t *c,
				void * 		cb_param,
				uint32_t 	what,
				void * 		param);
/*
 * This is a standardized way of installing 'action' handlers onto windows
 * and controls. The 'current' field is used to prevent re-entrance. This structure
 * is opaque and is not accessible by the application, typically.
 */
typedef struct mui_action_t {
	STAILQ_ENTRY(mui_action_t) 	self;
	uint32_t 					current; // prevents re-entrance
	union {
		mui_window_action_p 	window_cb;
		mui_control_action_p 	control_cb;
	};
	void *						cb_param;
} mui_action_t;

typedef STAILQ_HEAD(, mui_action_t) mui_action_queue_t;

struct cg_surface_t;
struct cg_ctx_t;

/*
 * Describes a pixmap. Currently only used for the screen destination pixels.
 * And really, only bpp:43 for ARGB is supported.
 */
typedef struct mui_pixmap_t {
	uint8_t * 					pixels;
	uint32_t 					bpp : 8;
	c2_pt_t 					size;
	uint32_t 					row_bytes;
} mui_pixmap_t;

typedef pixman_region32_t 	mui_region_t;

DECLARE_C_ARRAY(mui_region_t, mui_clip_stack, 2);

/*
 * The Drawable is a drawing context -- currently there's only one for the
 * whole screen, but technically we could have several. The important feature
 * of this is that it keeps a context for the pixman library destination
 * image, AND also the context for the 'cg' vectorial library.
 * Furthermore it keeps track of a stack of clipping rectangles, and is able
 * to 'sync' the current clipping area for either (or both) cg and libpixman.
 */
typedef struct mui_drawable_t {
	mui_pixmap_t				pix;	// *has* to be first in struct
	void * 						_pix_hash; // used to detect if pix has changed
	struct cg_surface_t * 		cg_surface;
	struct cg_ctx_t *			cg;
	union pixman_image *		pixman;	// (try) not to use these directly
	unsigned int				pixman_clip_dirty: 1,
								cg_clip_dirty : 1;
	mui_clip_stack_t			clip;
} mui_drawable_t;

/*
 * Drawable related
 */
void
mui_drawable_dispose(
		mui_drawable_t * dr);
// get/allocate a pixman structure for this drawable
union pixman_image *
mui_drawable_get_pixman(
		mui_drawable_t * dr);
// get/allocate a cg drawing context for this
struct cg_ctx_t *
mui_drawable_get_cg(
		mui_drawable_t * dr);
// return 0 (no intersect), 1: fully contained and 2: partial contains
int
mui_drawable_clip_intersects(
		mui_drawable_t * dr,
		c2_rect_p r );
void
mui_drawable_set_clip(
		mui_drawable_t * dr,
		c2_rect_array_p clip );
int
mui_drawable_clip_push(
		mui_drawable_t * dr,
		c2_rect_p 		r );
int
mui_drawable_clip_push_region(
		mui_drawable_t * dr,
		pixman_region32_t * rgn );
int
mui_drawable_clip_substract_region(
		mui_drawable_t * dr,
		pixman_region32_t * rgn );
void
mui_drawable_clip_pop(
		mui_drawable_t * dr );
pixman_region32_t *
mui_drawable_clip_get(
		mui_drawable_t * dr);


/*
 * Your typical ARGB color. Note that the components are NOT
 * alpha-premultiplied at this stage.
 * This struct should be able to be passed as a value, not a pointer
 */
typedef union mui_color_t {
	struct {
		uint8_t a,r,g,b;
	} __attribute__((packed));
	uint32_t value;
	uint8_t v[4];
} mui_color_t;

typedef struct mui_control_color_t {
	mui_color_t fill, frame, text;
} mui_control_color_t;

#define MUI_COLOR(_v) ((mui_color_t){ .value = (_v)})

#define CG_COLOR(_c) (struct cg_color_t){ \
			.a = (_c).a / 255.0, .r = (_c).r / 255.0, \
			.g = (_c).g / 255.0, .b = (_c).b / 255.0 }
/*
 * Pixman use premultiplied alpha values
 */
#define PIXMAN_COLOR(_c) (pixman_color_t){ \
			.alpha = (_c).a * 257, .red = (_c).r * (_c).a, \
			.green = (_c).g * (_c).a, .blue = (_c).b * (_c).a }


typedef struct mui_font_t {
	mui_drawable_t			 	font;	// points to ttc pixels!
	char * 						name;	// not filename, internal name, aka 'main'
	unsigned int 				size;	// in pixels
	TAILQ_ENTRY(mui_font_t) 	self;
	struct stb_ttc_info  		ttc;
} mui_font_t;

/*
 * Font related
 */
void
mui_font_init(
		struct mui_t *	ui);
void
mui_font_dispose(
		struct mui_t *	ui);

mui_font_t *
mui_font_find(
		struct mui_t *	ui,
		const char *	name);
void
mui_font_text_draw(
		mui_font_t *	font,
		mui_drawable_t *dr,
		c2_pt_t 		where,
		const char *	text,
		unsigned int 	text_len,
		mui_color_t 	color);
int
mui_font_text_measure(
		mui_font_t *	font,
		const char *	text,
		struct stb_ttc_measure *m );

enum mui_text_align_e {
	MUI_TEXT_ALIGN_LEFT 	= 0,
	MUI_TEXT_ALIGN_CENTER	= (1 << 0),
	MUI_TEXT_ALIGN_RIGHT	= (1 << 1),
	MUI_TEXT_ALIGN_TOP		= 0,
	MUI_TEXT_ALIGN_MIDDLE	= (MUI_TEXT_ALIGN_CENTER << 2),
	MUI_TEXT_ALIGN_BOTTOM	= (MUI_TEXT_ALIGN_RIGHT << 2),
};

void
mui_font_textbox(
		mui_font_t *	font,
		mui_drawable_t *dr,
		c2_rect_t 		bbox,
		const char *	text,
		unsigned int 	text_len,
		mui_color_t 	color,
		uint16_t 		flags );
DECLARE_C_ARRAY(stb_ttc_g*, mui_glyph_array, 8, int x, y, w; );
DECLARE_C_ARRAY(mui_glyph_array_t, mui_glyph_line_array, 8);

/*
 * Measure a text string, return the number of lines, and each glyphs
 * position already aligned to the MUI_TEXT_ALIGN_* flags.
 */
void
mui_font_measure(
		mui_font_t *	font,
		c2_rect_t 		bbox,
		const char *	text,
		unsigned int 	text_len,
		mui_glyph_line_array_t *lines,
		uint16_t 		flags);
// clear all the lines, and glyph lists. Use it after mui_font_measure
void
mui_font_measure_clear(
		mui_glyph_line_array_t *lines);


enum mui_window_layer_e {
	MUI_WINDOW_LAYER_NORMAL = 0,
	MUI_WINDOW_LAYER_MODAL = 3,
	MUI_WINDOW_LAYER_ALERT = 5,
	MUI_WINDOW_LAYER_TOP = 15,
	// Menubar and Menus (popups) are also windows
	MUI_WINDOW_MENUBAR_LAYER = MUI_WINDOW_LAYER_TOP - 1,
	MUI_WINDOW_MENU_LAYER,
};

enum mui_window_action_e {
	MUI_WINDOW_ACTION_NONE 		= 0,
	MUI_WINDOW_ACTION_CLOSE		= FCC('w','c','l','s'),
};

typedef struct mui_window_t {
	TAILQ_ENTRY(mui_window_t)	self;
	struct mui_t *				ui;
	mui_wdef_p 					wdef;
	uint32_t					uid;		// optional, pseudo unique id
	struct {
		unsigned long				hidden: 1,
									zombie: 1,	// is in pre-delete ui->zombies
									layer : 4,
									hit_part : 8;
	}							flags;
	c2_pt_t 					click_loc;
	struct mui_drawable_t *		dr;
	// both these rectangles are in screen coordinates, even tho
	// 'contents' is fully included in 'frame'
	c2_rect_t					frame, content;
	char *						title;
	mui_action_queue_t			actions;
	TAILQ_HEAD(controls, mui_control_t) controls;
	// anything deleted during an action goes in zombies
	TAILQ_HEAD(zombies, mui_control_t) zombies;
	struct mui_control_t *		control_clicked;
	mui_region_t				inval;
} mui_window_t;

/*
 * Window related
 */
mui_window_t *
mui_window_create(
		struct mui_t *	ui,
		c2_rect_t 		frame,
		mui_wdef_p 		wdef,
		uint8_t 		layer,
		const char *	title,
		uint32_t 		instance_size);
// Dispose a window and it's content (controls).
/*
 * Note: if an action is in progress the window is not freed immediately
 * but added to the zombie list, and freed when the action is done.
 * This is to prevent re-entrance problems. This allows window actions to
 * delete their own window without crashing.
 */
void
mui_window_dispose(
		mui_window_t *	win);
// Invalidate 'r' in window coordinates, or the whole window if 'r' is NULL
void
mui_window_inval(
		mui_window_t *	win,
		c2_rect_t * 	r);
// return true if the window is the frontmost window (in that window's layer)
bool
mui_window_isfront(
		mui_window_t *	win);
// return the top (non menubar/menu) window
mui_window_t *
mui_window_front(
		struct mui_t *ui);

// move win to the front (of its layer), return true if it was moved
bool
mui_window_select(
		mui_window_t *	win);
// call the window action callback, if any
void
mui_window_action(
		mui_window_t * 	c,
		uint32_t 		what,
		void * 			param );
// add an action callback for this window
void
mui_window_set_action(
		mui_window_t * 	c,
		mui_window_action_p	cb,
		void * 			param );
// return the window whose UID is 'uid', or NULL if not found
mui_window_t *
mui_window_get_by_id(
		struct mui_t *	ui,
		uint32_t 		uid );
// set the window UID
void
mui_window_set_id(
		mui_window_t *	win,
		uint32_t 		uid);

struct mui_menu_items_t;

/*
 * This is a menu item descriptor (also used for the titles, bar a few bits).
 * This does not a *control* in the window, instead this is used to describe
 * the menus and menu item controls that are created when the menu becomes visible.
 */
typedef struct mui_menu_item_t {
	uint32_t 					disabled : 1, hilited : 1;
	uint32_t 					index: 9;
	uint32_t 					uid;
	char * 						title;
	char  						mark[8];		// UTF8 -- Charcoal
	char						icon[8];		// UTF8 -- Wider, icon font
	char 						kcombo[16];		// UTF8 -- display only
	mui_key_equ_t 				key_equ;		// keystroke to select this item
	struct mui_menu_item_t *	submenu;
	c2_pt_t						location;		// calculated by menu creation code
} mui_menu_item_t;

/*
 * The menu item array is atypical as the items ('e' field) are not allocated
 * by the array, but by the menu creation code. This is because the menu
 * reuses the pointer to the items that is passed when the menu is added to
 * the menubar.
 * the 'read only' field is used to prevent the array from trying to free the
 * items when being disposed.
 */
DECLARE_C_ARRAY(mui_menu_item_t, mui_menu_items, 2,
				bool read_only; );
IMPLEMENT_C_ARRAY(mui_menu_items);

enum {
	// parameter is a mui_menu_item_t* for the first item of the menu,
	// this is exactly the parameter passed to add_simple()
	// you can use this to disable/enable menu items etc
	MUI_MENUBAR_ACTION_PREPARE 		= FCC('m','b','p','r'),
	// parameter 'target' is a mui_menuitem_t*
	MUI_MENUBAR_ACTION_SELECT 		= FCC('m','b','a','r'),
};
/*
 * Menu related.
 * Menubar, and menus/popups are windows as well, in a layer above the
 * normal ones.
 */
mui_window_t *
mui_menubar_new(
		struct mui_t *	ui );
// return the previously created menubar (or NULL)
mui_window_t *
mui_menubar_get(
		struct mui_t *	ui );

/*
 * Add a menu to the menubar. 'items' is an array of mui_menu_item_t
 * terminated by an element with a NULL title.
 * Note: The array is NOT const, it will be tweaked for storing items position,
 * it can also be tweaked to set/reset the disabled state, check marks etc
 */
struct mui_control_t *
mui_menubar_add_simple(
		mui_window_t *	win,
		const char * 	title,
		uint32_t 		menu_uid,
		mui_menu_item_t * items );
/* Turn off any highlighted menu titles */
mui_window_t *
mui_menubar_highlight(
		mui_window_t *	win,
		bool 			ignored );

enum mui_control_type_e {
	MUI_CONTROL_NONE = 0,
	MUI_CONTROL_BUTTON,
	MUI_CONTROL_GROUP,
	MUI_CONTROL_SEPARATOR,
	MUI_CONTROL_TEXTBOX,
	MUI_CONTROL_GROUPBOX,
	MUI_CONTROL_LISTBOX,
	MUI_CONTROL_SCROLLBAR,
	MUI_CONTROL_MENUTITLE,
	MUI_CONTROL_MENUITEM,
	MUI_CONTROL_SUBMENUITEM,
	MUI_CONTROL_POPUP,
};

enum {
	MUI_BUTTON_STYLE_NORMAL = 0,
	MUI_BUTTON_STYLE_DEFAULT = 1,
	MUI_BUTTON_STYLE_RADIO,
	MUI_BUTTON_STYLE_CHECKBOX,
};

enum {
	MUI_CONTROL_STATE_NORMAL = 0,
	MUI_CONTROL_STATE_HOVER,
	MUI_CONTROL_STATE_CLICKED,
	MUI_CONTROL_STATE_DISABLED,
	MUI_CONTROL_STATE_COUNT
};

enum {
	MUI_CONTROL_ACTION_NONE = 0,
	MUI_CONTROL_ACTION_VALUE_CHANGED	= FCC('c','v','a','l'),
	MUI_CONTROL_ACTION_CLICKED			= FCC('c','l','k','d'),
	MUI_CONTROL_ACTION_SELECT			= FCC('c','s','e','l'),
	MUI_CONTROL_ACTION_DOUBLECLICK		= FCC('c','d','c','l'),
};

/*
 * Control record... this are the 'common' fields, most of the controls
 * have their own 'extended' record using their own fields.
 */
typedef struct mui_control_t {
	TAILQ_ENTRY(mui_control_t) 	self;
	struct mui_window_t *		win;
	mui_cdef_p 					cdef;
	uint32_t					state;
	uint32_t 					type;
	uint32_t					style;
	struct {
		unsigned int 			hidden : 1,
								zombie : 1,
								hit_part : 8;
	}							flags;
	uint32_t					value;
	uint32_t 					uid;
	uint32_t 					uid_mask;		// for radio buttons
	c2_rect_t					frame;
	mui_key_equ_t				key_equ; // keystroke to select this control
	char *						title;
	mui_action_queue_t			actions;
} mui_control_t;

/*
 * Control related
 */
mui_control_t *
mui_control_new(
		mui_window_t * 	win,
		uint8_t 		type,
		mui_cdef_p 		cdef,
		c2_rect_t 		frame,
		const char *	title,
		uint32_t 		uid,
		uint32_t 		instance_size );
void
mui_control_dispose(
		mui_control_t * c );
uint32_t
mui_control_get_type(
		mui_control_t * c );
uint32_t
mui_control_get_uid(
		mui_control_t * c );
mui_control_t *
mui_control_locate(
		mui_window_t * 	win,
		c2_pt_t 		pt );
mui_control_t *
mui_control_get_by_id(
		mui_window_t * 	win,
		uint32_t 		uid );
void
mui_control_inval(
		mui_control_t * c );
void
mui_control_action(
		mui_control_t * c,
		uint32_t 		what,
		void * 			param );
void
mui_control_set_action(
		mui_control_t * c,
		mui_control_action_p	cb,
		void * 			param );
void
mui_control_set_state(
		mui_control_t * c,
		uint32_t 		state );
uint32_t
mui_control_get_state(
		mui_control_t * c );

int32_t
mui_control_get_value(
		mui_control_t * c);
int32_t
mui_control_set_value(
		mui_control_t * c,
		int32_t 		selected);
const char *
mui_control_get_title(
		mui_control_t * c );
void
mui_control_set_title(
		mui_control_t * c,
		const char * 	text );

mui_control_t *
mui_button_new(
		mui_window_t * 	win,
		c2_rect_t 		frame,
		uint8_t			style,
		const char *	title,
		uint32_t 		uid );
/*
 * Create a static text box. Font is optional, flags correponds to the MUI_TEXT_ALIGN_*
 * PLUS the extrast listed below.
 */
enum mui_textbox_e {
	MUI_CONTROL_TEXTBOX_FRAME		= (1 << 8),
};
mui_control_t *
mui_textbox_new(
		mui_window_t * 	win,
		c2_rect_t 		frame,
		const char *	text,
		const char * 	font,
		uint16_t		flags );
mui_control_t *
mui_groupbox_new(
		mui_window_t * 	win,
		c2_rect_t 		frame,
		const char *	title,
		uint16_t		flags );

mui_control_t *
mui_scrollbar_new(
		mui_window_t * 	win,
		c2_rect_t 		frame,
		uint32_t 		uid );
uint32_t
mui_scrollbar_get_max(
		mui_control_t * c);
void
mui_scrollbar_set_max(
		mui_control_t * c,
		uint32_t 		max);
void
mui_scrollbar_set_page(
		mui_control_t * c,
		uint32_t 		page);

mui_control_t *
mui_listbox_new(
		mui_window_t * 	win,
		c2_rect_t 		frame,
		uint32_t 		uid );
void
mui_listbox_prepare(
		mui_control_t * c);
mui_listbox_elems_t *
mui_listbox_get_elems(
		mui_control_t * c);
mui_control_t *
mui_separator_new(
		mui_window_t * 	win,
		c2_rect_t 		frame);
mui_control_t *
mui_popupmenu_new(
		mui_window_t *	win,
		c2_rect_t 		frame,
		const char * 	title,
		uint32_t 		uid );
mui_menu_items_t *
mui_popupmenu_get_items(
		mui_control_t * c);
void
mui_popupmenu_prepare(
		mui_control_t * c);

/*
 * Standard file dialog
 */
enum mui_std_action_e {
	MUI_STDF_ACTION_NONE 		= 0,
	// parameter 'target' is a char * with full pathname of selected file
	MUI_STDF_ACTION_SELECT 		= FCC('s','t','d','s'),
	MUI_STDF_ACTION_CANCEL 		= FCC('s','t','d','c'),
};

mui_window_t *
mui_stdfile_get(
		struct mui_t * 	ui,
		c2_pt_t 		where,
		const char * 	prompt,
		const char * 	regexp,
		const char * 	start_path );
// return the curently selected pathname -- caller must free() it
char *
mui_stdfile_get_selected_path(
		mui_window_t * w );

/*
 * Alert dialog
 */
enum {
	MUI_ALERT_FLAG_OK 		= (1 << 0),
	MUI_ALERT_FLAG_CANCEL 	= (1 << 1),

	MUI_ALERT_ICON_INFO 	= (1 << 8),

	MUI_ALERT_INFO 			= (MUI_ALERT_FLAG_OK | MUI_ALERT_ICON_INFO),
	MUI_ALERT_WARN 			= (MUI_ALERT_FLAG_OK | MUI_ALERT_FLAG_CANCEL),
};

enum {
	MUI_ALERT_BUTTON_OK		= FCC('o','k',' ',' '),
	MUI_ALERT_BUTTON_CANCEL = FCC('c','a','n','c'),
};

mui_window_t *
mui_alert(
		struct mui_t * 	ui,
		c2_pt_t 		where, // (0,0) will center it
		const char * 	title,
		const char * 	message,
		uint16_t 		flags );

enum {
	MUI_TIME_RES		= 1,
	MUI_TIME_SECOND		= 1000000,
	MUI_TIME_MS			= (MUI_TIME_SECOND/1000),
};
mui_time_t
mui_get_time();

typedef struct mui_timer_group_t {
	uint64_t 					map;
	struct {
		mui_time_t 					when;
		mui_timer_p 				cb;
		void * 						param;
	} 						timers[64];
} mui_timer_group_t;

/*
 * Register 'cb' to be called after 'delay'. Returns a timer id (0 to 63)
 * or 0xff if no timer is available. The timer function cb can return 0 for a one
 * shot timer, or another delay that will be added to the current stamp for a further
 * call of the timer.
 * 'param' will be also passed to the timer callback.
 */
uint8_t
mui_timer_register(
		struct mui_t *	ui,
		mui_timer_p 	cb,
		void *			param,
		uint32_t 		delay);

typedef struct mui_t {
	c2_pt_t 					screen_size;
	mui_color_t 				clear_color;
	uint16_t 					modifier_keys;
	int 						draw_debug;
	// this is the sum of all the window's dirty regions, inc moved windows etc
	mui_region_t 				inval;
	// once the pixels have been refreshed, 'inval' is copied to 'redraw'
	// to push the pixels to the screen.
	mui_region_t 				redraw;

	TAILQ_HEAD(, mui_font_t) 	fonts;
	TAILQ_HEAD(windows, mui_window_t) 	windows;
	mui_window_t *				menubar;
	TAILQ_HEAD(, mui_window_t) 	zombies;
	// this is used to track any active action callbacks to
	// prevent recursion problem and track any 'delete' happening
	// during an action callback
	uint32_t 					action_active;
	mui_window_t *				event_capture;
	mui_timer_group_t			timer;
	char * 						pref_directory; /* optional */
} mui_t;

void
mui_init(
		mui_t *			ui);
void
mui_dispose(
		mui_t *			ui);
void
mui_draw(
		mui_t *			ui,
		mui_drawable_t *dr,
		uint16_t 		all);
void
mui_run(
		mui_t *			ui);
// return true if the event was handled by the ui
bool
mui_handle_event(
		mui_t *			ui,
		mui_event_t *	ev);
// return true if event 'ev' is a key combo matching key_equ
bool
mui_event_match_key(
		mui_event_t *	ev,
		mui_key_equ_t 	key_equ);
/* Return true if the ui has any active windows, ie, not hidden, zombie;
 * This does not include the menubar, but it does include any menus or popups
 *
 * This is used to decide wether to hide the mouse cursor or not
 */
bool
mui_has_active_windows(
		mui_t *			ui);

/* Return a hash value for string inString */
uint32_t
mui_hash(
	const char * inString );
