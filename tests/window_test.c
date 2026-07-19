#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#define GOBLIN_WINDOW_IMPLEMENTATION
#include "../include/goblin/window.h"

static void test_rgb_macro(void)
{
    assert(GOBLIN_RGB(0, 0, 0) == 0x00000000u);
    assert(GOBLIN_RGB(255, 255, 255) == 0x00FFFFFFu);
    assert(GOBLIN_RGB(0x12, 0x34, 0x56) == 0x00123456u);
    assert(GOBLIN_RGB(255, 0, 0) == 0x00FF0000u);
    assert(GOBLIN_RGB(0, 255, 0) == 0x0000FF00u);
    assert(GOBLIN_RGB(0, 0, 255) == 0x000000FFu);
}

/* Returns 1 if the live window checks actually ran, 0 if they were skipped
   because no display was available (e.g. a headless Linux CI runner with
   no X server / no Xvfb). Skipping is a valid outcome, not a failure: this
   library needs a real display to open a real window. */
static int test_window_lifecycle(void)
{
    goblin_window *win = goblin_window_create(320, 240, "goblin window test");

    if (!win) {
        puts("window_test: no display available, skipping live window checks");
        return 0;
    }

    assert(goblin_window_width(win) == 320);
    assert(goblin_window_height(win) == 240);
    assert(!goblin_window_should_close(win));

    /* fresh window: no keys/buttons held, cursor unmoved */
    assert(!goblin_window_key_down(win, GOBLIN_KEY_A));
    assert(!goblin_window_key_pressed(win, GOBLIN_KEY_A));
    assert(!goblin_window_key_released(win, GOBLIN_KEY_A));
    assert(!goblin_window_mouse_down(win, GOBLIN_MOUSE_LEFT));
    assert(!goblin_window_mouse_down(win, GOBLIN_MOUSE_RIGHT));
    assert(!goblin_window_mouse_down(win, GOBLIN_MOUSE_MIDDLE));

    int mx = -1, my = -1;
    goblin_window_mouse_pos(win, &mx, &my);
    assert(mx == 0 && my == 0);

    goblin_window_poll_events(win);

    /* framebuffer is writable and reads back exactly what we wrote */
    uint32_t *px = goblin_window_pixels(win);
    assert(px != NULL);

    int w = goblin_window_width(win);
    int h = goblin_window_height(win);
    assert(w > 0 && h > 0);

    for (int i = 0; i < w * h; i++) {
        px[i] = GOBLIN_RGB(10, 20, 30);
    }

    px = goblin_window_pixels(win);
    assert(px[0] == GOBLIN_RGB(10, 20, 30));
    assert(px[(size_t)w * (size_t)h - 1] == GOBLIN_RGB(10, 20, 30));

    /* a few real present/poll cycles, the way a real app would run them */
    for (int frame = 0; frame < 5; frame++) {
        goblin_window_poll_events(win);

        px = goblin_window_pixels(win);
        for (int i = 0; i < goblin_window_width(win) * goblin_window_height(win); i++) {
            px[i] = GOBLIN_RGB((frame * 10) % 256, 0, 0);
        }

        goblin_window_present(win);
        goblin_window_sleep_ms(1);
    }

    goblin_window_close(win);
    assert(goblin_window_should_close(win));

    double t1 = goblin_window_time();
    double t2 = goblin_window_time();
    assert(t2 >= t1);

    goblin_window_destroy(win);
    return 1;
}

int main(void)
{
    test_rgb_macro();
    int had_display = test_window_lifecycle();

    if (had_display) {
        puts("window tests passed (created and drove a real window)");
    } else {
        puts("window tests passed (no display; only static checks ran)");
    }

    return 0;
}
