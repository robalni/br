struct UiEvent {
    uint16_t keyup;
    uint16_t keydown;
    bool quit;
    bool render;
    bool resize;
};

enum UiKey {
    UI_NOKEY,
    UI_LEFT,
    UI_RIGHT,
};

enum UiKeyState {
    UI_NOT_PRESSED,
    UI_PRESSED,
};

#define UI_MAX_KEYS 3
enum UiKeyState global_ui_keys[UI_MAX_KEYS];

static void
ui_set_key(enum UiKey key, enum UiKeyState state) {
    enum UiKeyState *keys = global_ui_keys;
    if (key < UI_MAX_KEYS) {
        keys[key] = state;
    }
}

static enum UiKeyState
ui_get_key(enum UiKey key) {
    enum UiKeyState *keys = global_ui_keys;
    if (key < UI_MAX_KEYS) {
        return keys[key];
    }
    return UI_NOT_PRESSED;
}
