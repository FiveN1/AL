#ifndef AL_INPUT_INCLUDED
#define AL_INPUT_INCLUDED

/*
    al_input.h - simple input manager based on the polling approach.
    
    supports keyboard and mouse input. (as of 04.09.2026)
    every key/button can be queried anywhere in the program with a simple: 
    - al_input.keys[AL_KEY_A];
    additional information about the mouse is also accessible from the al_input state (mouse position, delta, scroll).

    each key/button includes information about when it was pressed or released (in sokol_time ticks).
    - stm_ticks
    this can be used for calculating the press duration or for buffering input.
    when key/button is pressed, stm_ticks will hold tick count at press.
    when key/button is released, stm_ticks will hold tick count at release
    note: we may want to store tick count both for press and release? (this is untested feature)

    if an event just occured, can be checked with:
    - frame_count
    if an event is in the same frame as the process, it has just occured.
    i.e. if .frame_count == sapp_frame_count();
    frame_count is set when an input is changed. (when pressed or released)

    key bindings are supported implicitly.
    the user can define a variable/constant (their 'action') with the value of a bound key/button and check for that 'action'.

    why polling?
    it results in much cleaner input handeling. no messy callbacks everwhere.
    polling also results in explicit control flow!
    inputs can be polled everwhere and at any time!

    INCLUDE BEFORE:
    - sokol_app.h (for events)
    - sokol_time.h (for key press duration)
	
    HOW TO USE:
    to use al_input put these function in your sokol program.
    - al_input_on_sokol_event() - in the on_event callback.
    - al_input_on_sokol_update() - in the on_frame callback. (at the very end!)
    then to query a key/button state simply do as such: 
        al_input.keys[AL_KEY_A];
        al_input.mouse_buttons[AL_MOUSE_LEFT];
    additional data about the mouse can be also queried:
        al_input.mouse_position;
        al_input.mouse_delta;
        al_input.mouse_scroll_delta;

    TODO:
    - key modifiers in the al_input state?
*/

typedef enum al_input_key_code { // all keys
	AL_KEY_INVALID = 0,
    // numbers
	AL_KEY_NUM_0,
    AL_KEY_NUM_1,
    AL_KEY_NUM_2,
    AL_KEY_NUM_3,
    AL_KEY_NUM_4,
    AL_KEY_NUM_5,
    AL_KEY_NUM_6,
    AL_KEY_NUM_7,
    AL_KEY_NUM_8,
    AL_KEY_NUM_9,
    // characters
    AL_KEY_A,
    AL_KEY_B,
    AL_KEY_C,
    AL_KEY_D,
    AL_KEY_E,
    AL_KEY_F,
    AL_KEY_G,
    AL_KEY_H,
    AL_KEY_I,
    AL_KEY_J,
    AL_KEY_K,
    AL_KEY_L,
    AL_KEY_M,
    AL_KEY_N,
    AL_KEY_O,
    AL_KEY_P,
    AL_KEY_Q,
    AL_KEY_R,
    AL_KEY_S,
    AL_KEY_T,
    AL_KEY_U,
    AL_KEY_V,
    AL_KEY_W,
    AL_KEY_X,
    AL_KEY_Y,
    AL_KEY_Z,
    // special characters
    AL_KEY_SPACE,
    AL_KEY_APOSTROPHE,      /* ' */
    AL_KEY_COMMA,           /* , */
    AL_KEY_MINUS,           /* - */
    AL_KEY_PERIOD,          /* . */
    AL_KEY_SLASH,           /* / */
    AL_KEY_SEMICOLON,       /* ; */
    AL_KEY_EQUAL,           /* = */
    AL_KEY_LEFT_BRACKET,    /* [ */
    AL_KEY_BACKSLASH,       /* \ */
    AL_KEY_RIGHT_BRACKET,   /* ] */
    AL_KEY_GRAVE_ACCENT,    /* ` */
    AL_KEY_WORLD_1,         /* non-US #1 */
    AL_KEY_WORLD_2,         /* non-US #2 */
    // other
    AL_KEY_ESCAPE,
    AL_KEY_ENTER,
    AL_KEY_TAB,
    AL_KEY_BACKSPACE,
    AL_KEY_INSERT,
    AL_KEY_DELETE,
    AL_KEY_RIGHT,
    AL_KEY_LEFT,
    AL_KEY_DOWN,
    AL_KEY_UP,
    AL_KEY_PAGE_UP,
    AL_KEY_PAGE_DOWN,
    AL_KEY_HOME,
    AL_KEY_END,
    AL_KEY_CAPS_LOCK,
    AL_KEY_SCROLL_LOCK,
    AL_KEY_NUM_LOCK,
    AL_KEY_PRINT_SCREEN,
    AL_KEY_PAUSE,
    // function keys
    AL_KEY_F1,
    AL_KEY_F2,
    AL_KEY_F3,
    AL_KEY_F4,
    AL_KEY_F5,
    AL_KEY_F6,
    AL_KEY_F7,
    AL_KEY_F8,
    AL_KEY_F9,
    AL_KEY_F10,
    AL_KEY_F11,
    AL_KEY_F12,
    AL_KEY_F13,
    AL_KEY_F14,
    AL_KEY_F15,
    AL_KEY_F16,
    AL_KEY_F17,
    AL_KEY_F18,
    AL_KEY_F19,
    AL_KEY_F20,
    AL_KEY_F21,
    AL_KEY_F22,
    AL_KEY_F23,
    AL_KEY_F24,
    AL_KEY_F25,
    // keypad keys
    AL_KEY_KEYPAD_0,
    AL_KEY_KEYPAD_1,
    AL_KEY_KEYPAD_2,
    AL_KEY_KEYPAD_3,
    AL_KEY_KEYPAD_4,
    AL_KEY_KEYPAD_5,
    AL_KEY_KEYPAD_6,
    AL_KEY_KEYPAD_7,
    AL_KEY_KEYPAD_8,
    AL_KEY_KEYPAD_9,
    AL_KEY_KEYPAD_DECIMAL,
    AL_KEY_KEYPAD_DIVIDE,
    AL_KEY_KEYPAD_MULTIPLY,
    AL_KEY_KEYPAD_SUBTRACT,
    AL_KEY_KEYPAD_ADD,
    AL_KEY_KEYPAD_ENTER,
    AL_KEY_KEYPAD_EQUAL,
    // modifier keys
    AL_KEY_LEFT_SHIFT,
    AL_KEY_LEFT_CONTROL,
    AL_KEY_LEFT_ALT,
    AL_KEY_LEFT_SUPER,
    AL_KEY_RIGHT_SHIFT,
    AL_KEY_RIGHT_CONTROL,
    AL_KEY_RIGHT_ALT,
    AL_KEY_RIGHT_SUPER,
    AL_KEY_MENU,
    // total key count
	_AL_KEY_COUNT
} al_input_key_code;

typedef enum al_input_mouse_button { // all mouse buttons
	AL_MOUSE_INVALID = 0,
	AL_MOUSE_LEFT,
    AL_MOUSE_RIGHT,
    AL_MOUSE_MIDDLE,
	_AL_MOUSE_COUNT
} al_input_mouse_button;

typedef struct al_input_key {
	bool pressed;           // true if pressed
    bool repeated;          // if key is repeated
    uint64_t stm_ticks;     // at what tick count was pressed. (with this the user can calculate the press duration & buffer input, using sokol_time)
    uint64_t frame_count;   // at what frame did it get pressed/released. used to compare with sapp_frame_count(), to check if input has just changed
} al_input_key;

typedef struct al_input_mouse {
    bool pressed;           // true if pressed
    uint64_t stm_ticks;     // at what tick count was pressed. (with this the user can calculate the press duration & buffer input, using sokol_time)
    uint64_t frame_count;   // at what frame did it get pressed/released. used to compare with sapp_frame_count(), to check if input has just changed
} al_input_mouse;

typedef struct al_input_state {
	al_input_key keys[_AL_KEY_COUNT];
    al_input_mouse mouse_buttons[_AL_MOUSE_COUNT];
	vec2_t mouse_position;
	vec2_t mouse_delta;
	vec2_t mouse_scroll_delta;
} al_input_state;

extern al_input_state al_input; // al_input is public!

// put this in your sokol functions!
void al_input_on_sokol_event(const sapp_event* event); // (put at very begining of on_event!) - changes key states based on sokol event
void al_input_on_sokol_update(); // (put at very end of on_frame!) - some internal state has to be updated every frame.

#endif AL_INPUT_INCLUDED
//#define AL_IMPL
#ifdef AL_IMPL 

al_input_state al_input; // al_input state definition (not static since its public)

static al_input_key_code _al_input_translate_sokol_key_code(sapp_keycode sokol_key_code) {
	switch (sokol_key_code) {
        case SAPP_KEYCODE_0: return AL_KEY_NUM_0;
        case SAPP_KEYCODE_1: return AL_KEY_NUM_1;
        case SAPP_KEYCODE_2: return AL_KEY_NUM_2;
        case SAPP_KEYCODE_3: return AL_KEY_NUM_3;
        case SAPP_KEYCODE_4: return AL_KEY_NUM_4;
        case SAPP_KEYCODE_5: return AL_KEY_NUM_5;
        case SAPP_KEYCODE_6: return AL_KEY_NUM_6;
        case SAPP_KEYCODE_7: return AL_KEY_NUM_7;
        case SAPP_KEYCODE_8: return AL_KEY_NUM_8;
        case SAPP_KEYCODE_9: return AL_KEY_NUM_9;
        case SAPP_KEYCODE_A: return AL_KEY_A;
        case SAPP_KEYCODE_B: return AL_KEY_B;
        case SAPP_KEYCODE_C: return AL_KEY_C;
        case SAPP_KEYCODE_D: return AL_KEY_D;
        case SAPP_KEYCODE_E: return AL_KEY_E;
        case SAPP_KEYCODE_F: return AL_KEY_F;
        case SAPP_KEYCODE_G: return AL_KEY_G;
        case SAPP_KEYCODE_H: return AL_KEY_H;
        case SAPP_KEYCODE_I: return AL_KEY_I;
        case SAPP_KEYCODE_J: return AL_KEY_J;
        case SAPP_KEYCODE_K: return AL_KEY_K;
        case SAPP_KEYCODE_L: return AL_KEY_L;
        case SAPP_KEYCODE_M: return AL_KEY_M;
        case SAPP_KEYCODE_N: return AL_KEY_N;
        case SAPP_KEYCODE_O: return AL_KEY_O;
        case SAPP_KEYCODE_P: return AL_KEY_P;
        case SAPP_KEYCODE_Q: return AL_KEY_Q;
        case SAPP_KEYCODE_R: return AL_KEY_R;
        case SAPP_KEYCODE_S: return AL_KEY_S;
        case SAPP_KEYCODE_T: return AL_KEY_T;
        case SAPP_KEYCODE_U: return AL_KEY_U;
        case SAPP_KEYCODE_V: return AL_KEY_V;
        case SAPP_KEYCODE_W: return AL_KEY_W;
        case SAPP_KEYCODE_X: return AL_KEY_X;
        case SAPP_KEYCODE_Y: return AL_KEY_Y;
        case SAPP_KEYCODE_Z: return AL_KEY_Z;
        case SAPP_KEYCODE_SPACE: return AL_KEY_SPACE;
        case SAPP_KEYCODE_APOSTROPHE: return;
        case SAPP_KEYCODE_COMMA: return AL_KEY_COMMA;
        case SAPP_KEYCODE_MINUS: return AL_KEY_MINUS;
        case SAPP_KEYCODE_PERIOD: return AL_KEY_PERIOD;
        case SAPP_KEYCODE_SLASH: return AL_KEY_SLASH;
        case SAPP_KEYCODE_SEMICOLON: return AL_KEY_SEMICOLON;
        case SAPP_KEYCODE_EQUAL: return AL_KEY_EQUAL;
        case SAPP_KEYCODE_LEFT_BRACKET: return AL_KEY_LEFT_BRACKET;
        case SAPP_KEYCODE_BACKSLASH: return AL_KEY_BACKSLASH;
        case SAPP_KEYCODE_RIGHT_BRACKET: return AL_KEY_RIGHT_BRACKET;
        case SAPP_KEYCODE_GRAVE_ACCENT: return AL_KEY_GRAVE_ACCENT;
        case SAPP_KEYCODE_WORLD_1: return  AL_KEY_WORLD_1;
        case SAPP_KEYCODE_WORLD_2: return AL_KEY_WORLD_2;
        case SAPP_KEYCODE_ESCAPE: return AL_KEY_ESCAPE;
        case SAPP_KEYCODE_ENTER: return AL_KEY_ENTER;
        case SAPP_KEYCODE_TAB: return AL_KEY_TAB;
        case SAPP_KEYCODE_BACKSPACE: return AL_KEY_BACKSPACE;
        case SAPP_KEYCODE_INSERT: return AL_KEY_INSERT;
        case SAPP_KEYCODE_DELETE: return AL_KEY_DELETE;
        case SAPP_KEYCODE_RIGHT: return AL_KEY_RIGHT;
        case SAPP_KEYCODE_LEFT: return AL_KEY_LEFT;
        case SAPP_KEYCODE_DOWN: return AL_KEY_DOWN;
        case SAPP_KEYCODE_UP: return AL_KEY_UP;
        case SAPP_KEYCODE_PAGE_UP: return AL_KEY_PAGE_UP;
        case SAPP_KEYCODE_PAGE_DOWN: return  AL_KEY_PAGE_DOWN;
        case SAPP_KEYCODE_HOME: return AL_KEY_HOME;
        case SAPP_KEYCODE_END: return AL_KEY_END;
        case SAPP_KEYCODE_CAPS_LOCK: return AL_KEY_CAPS_LOCK;
        case SAPP_KEYCODE_SCROLL_LOCK: return AL_KEY_SCROLL_LOCK;
        case SAPP_KEYCODE_NUM_LOCK: return AL_KEY_NUM_LOCK;
        case SAPP_KEYCODE_PRINT_SCREEN: return AL_KEY_PRINT_SCREEN;
        case SAPP_KEYCODE_PAUSE: return AL_KEY_PAUSE;
        case SAPP_KEYCODE_F1: return AL_KEY_F1;
        case SAPP_KEYCODE_F2: return AL_KEY_F2;
        case SAPP_KEYCODE_F3: return AL_KEY_F3;
        case SAPP_KEYCODE_F4: return AL_KEY_F4;
        case SAPP_KEYCODE_F5: return AL_KEY_F5;
        case SAPP_KEYCODE_F6: return AL_KEY_F6;
        case SAPP_KEYCODE_F7: return AL_KEY_F7;
        case SAPP_KEYCODE_F8: return AL_KEY_F8;
        case SAPP_KEYCODE_F9: return AL_KEY_F9;
        case SAPP_KEYCODE_F10: return AL_KEY_F10;
        case SAPP_KEYCODE_F11: return AL_KEY_F11;
        case SAPP_KEYCODE_F12: return AL_KEY_F12;
        case SAPP_KEYCODE_F13: return AL_KEY_F13;
        case SAPP_KEYCODE_F14: return AL_KEY_F14;
        case SAPP_KEYCODE_F15: return AL_KEY_F15;
        case SAPP_KEYCODE_F16: return AL_KEY_F16;
        case SAPP_KEYCODE_F17: return AL_KEY_F17;
        case SAPP_KEYCODE_F18: return AL_KEY_F18;
        case SAPP_KEYCODE_F19: return AL_KEY_F19;
        case SAPP_KEYCODE_F20: return AL_KEY_F20;
        case SAPP_KEYCODE_F21: return AL_KEY_F21;
        case SAPP_KEYCODE_F22: return AL_KEY_F22;
        case SAPP_KEYCODE_F23: return AL_KEY_F23;
        case SAPP_KEYCODE_F24: return AL_KEY_F24;
        case SAPP_KEYCODE_F25: return AL_KEY_F25;
        case SAPP_KEYCODE_KP_0: return AL_KEY_KEYPAD_0;
        case SAPP_KEYCODE_KP_1: return AL_KEY_KEYPAD_1;
        case SAPP_KEYCODE_KP_2: return AL_KEY_KEYPAD_2;
        case SAPP_KEYCODE_KP_3: return AL_KEY_KEYPAD_3;
        case SAPP_KEYCODE_KP_4: return AL_KEY_KEYPAD_4;
        case SAPP_KEYCODE_KP_5: return AL_KEY_KEYPAD_5;
        case SAPP_KEYCODE_KP_6: return AL_KEY_KEYPAD_6;
        case SAPP_KEYCODE_KP_7: return AL_KEY_KEYPAD_7;
        case SAPP_KEYCODE_KP_8: return AL_KEY_KEYPAD_8;
        case SAPP_KEYCODE_KP_9: return AL_KEY_KEYPAD_9;
        case SAPP_KEYCODE_KP_DECIMAL: return AL_KEY_KEYPAD_DECIMAL;
        case SAPP_KEYCODE_KP_DIVIDE: return AL_KEY_KEYPAD_DIVIDE;
        case SAPP_KEYCODE_KP_MULTIPLY: return AL_KEY_KEYPAD_MULTIPLY;
        case SAPP_KEYCODE_KP_SUBTRACT: return AL_KEY_KEYPAD_SUBTRACT;
        case SAPP_KEYCODE_KP_ADD: return AL_KEY_KEYPAD_ADD;
        case SAPP_KEYCODE_KP_ENTER: return AL_KEY_KEYPAD_ENTER;
        case SAPP_KEYCODE_KP_EQUAL: return AL_KEY_KEYPAD_EQUAL;
        case SAPP_KEYCODE_LEFT_SHIFT: return AL_KEY_LEFT_SHIFT;
        case SAPP_KEYCODE_LEFT_CONTROL: return AL_KEY_LEFT_CONTROL;
        case SAPP_KEYCODE_LEFT_ALT: return AL_KEY_LEFT_ALT;
        case SAPP_KEYCODE_LEFT_SUPER: return AL_KEY_LEFT_SUPER;
        case SAPP_KEYCODE_RIGHT_SHIFT: return AL_KEY_RIGHT_SHIFT;
        case SAPP_KEYCODE_RIGHT_CONTROL: return AL_KEY_RIGHT_CONTROL;
        case SAPP_KEYCODE_RIGHT_ALT: return AL_KEY_RIGHT_ALT;
        case SAPP_KEYCODE_RIGHT_SUPER: return AL_KEY_RIGHT_SUPER;
        case SAPP_KEYCODE_MENU: return AL_KEY_MENU;
	}
	return AL_KEY_INVALID;
}

static al_input_mouse_button _al_input_translate_sokol_mouse_button(sapp_mousebutton sokol_mouse_button) {
    switch (sokol_mouse_button) {
        case SAPP_MOUSEBUTTON_LEFT: return AL_MOUSE_LEFT;
        case SAPP_MOUSEBUTTON_RIGHT: return AL_MOUSE_RIGHT;
        case SAPP_MOUSEBUTTON_MIDDLE: return AL_MOUSE_MIDDLE;
    }
	return AL_MOUSE_INVALID;
}

void al_input_on_sokol_event(const sapp_event* event) {
	switch (event->type) {
    case SAPP_EVENTTYPE_KEY_DOWN: {
        al_input_key* key = &al_input.keys[_al_input_translate_sokol_key_code(event->key_code)];
        if (key->pressed) {
            key->repeated = true; // if was already pressed before
        }
        else {
            key->pressed = true;
            key->stm_ticks = stm_now(); // set time since press. note: we do not want to update the counter on repeat events
            key->frame_count = sapp_frame_count();
        }
        break;
    }
    case SAPP_EVENTTYPE_KEY_UP: {
        al_input_key* key = &al_input.keys[_al_input_translate_sokol_key_code(event->key_code)];
        key->pressed = false;
        key->repeated = false;
        key->stm_ticks = stm_now(); // set time since release
        key->frame_count = sapp_frame_count();
        break;
    }
    case SAPP_EVENTTYPE_MOUSE_DOWN: {
        al_input_mouse* mouse = &al_input.mouse_buttons[_al_input_translate_sokol_mouse_button(event->mouse_button)];
        mouse->pressed = true;
        mouse->stm_ticks = stm_now();
        mouse->frame_count = sapp_frame_count();
        break;
    }
    case SAPP_EVENTTYPE_MOUSE_UP: {
        al_input_mouse* mouse = &al_input.mouse_buttons[_al_input_translate_sokol_mouse_button(event->mouse_button)];
        mouse->pressed = false;
        mouse->stm_ticks = stm_now();
        mouse->frame_count = sapp_frame_count();
        break;
    }
    case SAPP_EVENTTYPE_MOUSE_MOVE: {
        float aspect = sapp_widthf() / sapp_heightf();
        // normalize mouse position to (-1.0f, 1.0f) where x adapts to aspect ratio. (y stays -1.0f to 1.0f)
        al_input.mouse_position = vec2_mulf(vec2_subf(vec2(event->mouse_x / sapp_widthf(), 1.0f - event->mouse_y / sapp_heightf()), 0.5f), 2.0f);
        al_input.mouse_position.x *= aspect;
        // normalize mouse delta
        al_input.mouse_delta = vec2(-event->mouse_dx / sapp_widthf(), event->mouse_dy / sapp_heightf());
        al_input.mouse_delta.x *= aspect;
        break;
    }
    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        al_input.mouse_scroll_delta = vec2(event->scroll_x, event->scroll_y);
        break;
	}
}

void al_input_on_sokol_update() {
    // reset when not updating
    al_input.mouse_delta = vec2f(0.0f);
    al_input.mouse_scroll_delta = vec2f(0.0f);
}

#endif AL_IMPL