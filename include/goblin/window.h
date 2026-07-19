/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    window.h - tiny header-only window creation + raw framebuffer drawing.

    * No SDL, no GLFW, no OpenGL, no external dependencies.
    * Just the OS: Win32 on Windows, Xlib on Linux/Unix.
    * You get an array of uint32_t pixels (0x00RRGGBB) and you draw into it
      yourself. goblin_window_present() blits it to the screen.

    Usage:
        #define GOBLIN_WINDOW_IMPLEMENTATION
        #include "goblin/window.h"

    On Linux you must link against X11:
        cc app.c -o app -lX11

    On Windows the required libs (gdi32, user32) are pulled in automatically
    via #pragma comment when compiling with MSVC; with MinGW add -lgdi32
    -luser32 if they are not linked by default.

    Quick example:

        goblin_window *win = goblin_window_create(640, 480, "hello");
        while (!goblin_window_should_close(win)) {
            goblin_window_poll_events(win);

            uint32_t *px = goblin_window_pixels(win);
            for (int i = 0; i < goblin_window_width(win) * goblin_window_height(win); i++)
                px[i] = 0x00202020;

            goblin_window_present(win);
        }
        goblin_window_destroy(win);
*/

#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef GOBLIN_WINDOW_H
#define GOBLIN_WINDOW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pack an 0x00RRGGBB pixel. */
#define GOBLIN_RGB(r, g, b) \
    ((uint32_t)(((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)))

typedef enum goblin_key {
    GOBLIN_KEY_UNKNOWN = 0,

    GOBLIN_KEY_A, GOBLIN_KEY_B, GOBLIN_KEY_C, GOBLIN_KEY_D, GOBLIN_KEY_E,
    GOBLIN_KEY_F, GOBLIN_KEY_G, GOBLIN_KEY_H, GOBLIN_KEY_I, GOBLIN_KEY_J,
    GOBLIN_KEY_K, GOBLIN_KEY_L, GOBLIN_KEY_M, GOBLIN_KEY_N, GOBLIN_KEY_O,
    GOBLIN_KEY_P, GOBLIN_KEY_Q, GOBLIN_KEY_R, GOBLIN_KEY_S, GOBLIN_KEY_T,
    GOBLIN_KEY_U, GOBLIN_KEY_V, GOBLIN_KEY_W, GOBLIN_KEY_X, GOBLIN_KEY_Y,
    GOBLIN_KEY_Z,

    GOBLIN_KEY_0, GOBLIN_KEY_1, GOBLIN_KEY_2, GOBLIN_KEY_3, GOBLIN_KEY_4,
    GOBLIN_KEY_5, GOBLIN_KEY_6, GOBLIN_KEY_7, GOBLIN_KEY_8, GOBLIN_KEY_9,

    GOBLIN_KEY_SPACE,
    GOBLIN_KEY_ENTER,
    GOBLIN_KEY_ESCAPE,
    GOBLIN_KEY_TAB,
    GOBLIN_KEY_BACKSPACE,

    GOBLIN_KEY_LEFT,
    GOBLIN_KEY_RIGHT,
    GOBLIN_KEY_UP,
    GOBLIN_KEY_DOWN,

    GOBLIN_KEY_LSHIFT,
    GOBLIN_KEY_RSHIFT,
    GOBLIN_KEY_LCTRL,
    GOBLIN_KEY_RCTRL,
    GOBLIN_KEY_LALT,
    GOBLIN_KEY_RALT,

    GOBLIN_KEY_COUNT
} goblin_key;

typedef enum goblin_mouse_button {
    GOBLIN_MOUSE_LEFT = 0,
    GOBLIN_MOUSE_RIGHT,
    GOBLIN_MOUSE_MIDDLE,
    GOBLIN_MOUSE_COUNT
} goblin_mouse_button;

typedef struct goblin_window goblin_window;

/* Creates and shows a window. Returns NULL on failure. */
goblin_window *goblin_window_create(int width, int height, const char *title);
void goblin_window_destroy(goblin_window *win);

/* true once the user clicked the close button / requested Alt+F4 etc. */
bool goblin_window_should_close(const goblin_window *win);
void goblin_window_close(goblin_window *win);

/* Pumps the OS message queue and updates input state. Call once per frame. */
void goblin_window_poll_events(goblin_window *win);

/* Blits the framebuffer returned by goblin_window_pixels() to the window. */
void goblin_window_present(goblin_window *win);

/* Raw framebuffer: width * height uint32_t pixels, 0x00RRGGBB, row-major,
   top-left origin. Pointer is only valid until the next poll_events() call
   that resizes the window - call goblin_window_pixels() again after that. */
uint32_t *goblin_window_pixels(goblin_window *win);
int goblin_window_width(const goblin_window *win);
int goblin_window_height(const goblin_window *win);

/* true for exactly the frame in which the window was resized. */
bool goblin_window_resized(const goblin_window *win);

bool goblin_window_key_down(const goblin_window *win, goblin_key key);
bool goblin_window_key_pressed(const goblin_window *win, goblin_key key);
bool goblin_window_key_released(const goblin_window *win, goblin_key key);

void goblin_window_mouse_pos(const goblin_window *win, int *x, int *y);
bool goblin_window_mouse_down(const goblin_window *win, goblin_mouse_button button);

/* Seconds since the first call to this function on this thread. Monotonic. */
double goblin_window_time(void);
/* Sleeps for roughly the given number of milliseconds. */
void goblin_window_sleep_ms(unsigned int ms);

#ifdef __cplusplus
}
namespace goblin {
    using Window = goblin_window;
    using Key = goblin_key;
    using MouseButton = goblin_mouse_button;

    static inline Window *window_create(int width, int height, const char *title) { return goblin_window_create(width, height, title); }
    static inline void window_destroy(Window *win) { goblin_window_destroy(win); }
    static inline bool window_should_close(const Window *win) { return goblin_window_should_close(win); }
    static inline void window_close(Window *win) { goblin_window_close(win); }
    static inline void window_poll_events(Window *win) { goblin_window_poll_events(win); }
    static inline void window_present(Window *win) { goblin_window_present(win); }
    static inline uint32_t *window_pixels(Window *win) { return goblin_window_pixels(win); }
    static inline int window_width(const Window *win) { return goblin_window_width(win); }
    static inline int window_height(const Window *win) { return goblin_window_height(win); }
    static inline bool window_resized(const Window *win) { return goblin_window_resized(win); }
    static inline bool key_down(const Window *win, Key key) { return goblin_window_key_down(win, key); }
    static inline bool key_pressed(const Window *win, Key key) { return goblin_window_key_pressed(win, key); }
    static inline bool key_released(const Window *win, Key key) { return goblin_window_key_released(win, key); }
    static inline void mouse_pos(const Window *win, int *x, int *y) { goblin_window_mouse_pos(win, x, y); }
    static inline bool mouse_down(const Window *win, MouseButton button) { return goblin_window_mouse_down(win, button); }
    static inline double time_seconds(void) { return goblin_window_time(); }
    static inline void sleep_ms(unsigned int ms) { goblin_window_sleep_ms(ms); }
}
#endif

/* ------------------------------------------------------------------- */
/*                          IMPLEMENTATION                             */
/* ------------------------------------------------------------------- */
#ifdef GOBLIN_WINDOW_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <time.h>

struct goblin_window_input_state {
    bool keys[GOBLIN_KEY_COUNT];
    bool keys_prev[GOBLIN_KEY_COUNT];
    bool mouse_buttons[GOBLIN_MOUSE_COUNT];
    int mouse_x, mouse_y;
};

/* ===================================================================== */
/*  WIN32 BACKEND                                                        */
/* ===================================================================== */
#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>

#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

struct goblin_window {
    HWND hwnd;
    int width, height;
    bool should_close;
    bool resized;
    uint32_t *pixels;
    BITMAPINFO bmi;
    struct goblin_window_input_state input;
};

static goblin_key goblin_window__vk_to_key(WPARAM vk)
{
    if (vk >= 'A' && vk <= 'Z') return (goblin_key)(GOBLIN_KEY_A + (vk - 'A'));
    if (vk >= '0' && vk <= '9') return (goblin_key)(GOBLIN_KEY_0 + (vk - '0'));

    switch (vk) {
        case VK_SPACE:     return GOBLIN_KEY_SPACE;
        case VK_RETURN:    return GOBLIN_KEY_ENTER;
        case VK_ESCAPE:    return GOBLIN_KEY_ESCAPE;
        case VK_TAB:       return GOBLIN_KEY_TAB;
        case VK_BACK:      return GOBLIN_KEY_BACKSPACE;
        case VK_LEFT:      return GOBLIN_KEY_LEFT;
        case VK_RIGHT:     return GOBLIN_KEY_RIGHT;
        case VK_UP:        return GOBLIN_KEY_UP;
        case VK_DOWN:      return GOBLIN_KEY_DOWN;
        case VK_LSHIFT:    return GOBLIN_KEY_LSHIFT;
        case VK_RSHIFT:    return GOBLIN_KEY_RSHIFT;
        case VK_LCONTROL:  return GOBLIN_KEY_LCTRL;
        case VK_RCONTROL:  return GOBLIN_KEY_RCTRL;
        case VK_LMENU:     return GOBLIN_KEY_LALT;
        case VK_RMENU:     return GOBLIN_KEY_RALT;
        default:           return GOBLIN_KEY_UNKNOWN;
    }
}

static void goblin_window__resize_buffer(goblin_window *win, int width, int height)
{
    if (width < 1) width = 1;
    if (height < 1) height = 1;

    free(win->pixels);
    win->pixels = (uint32_t *)calloc((size_t)width * (size_t)height, sizeof(uint32_t));
    win->width = width;
    win->height = height;
    win->resized = true;

    ZeroMemory(&win->bmi, sizeof(win->bmi));
    win->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    win->bmi.bmiHeader.biWidth = width;
    win->bmi.bmiHeader.biHeight = -height; /* top-down DIB */
    win->bmi.bmiHeader.biPlanes = 1;
    win->bmi.bmiHeader.biBitCount = 32;
    win->bmi.bmiHeader.biCompression = BI_RGB;
}

static LRESULT CALLBACK goblin_window__wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    goblin_window *win = (goblin_window *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if (msg == WM_NCCREATE) {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    if (!win) return DefWindowProcW(hwnd, msg, wparam, lparam);

    switch (msg) {
        case WM_CLOSE:
            win->should_close = true;
            return 0;

        case WM_SIZE: {
            int w = LOWORD(lparam);
            int h = HIWORD(lparam);
            if (w > 0 && h > 0 && (w != win->width || h != win->height))
                goblin_window__resize_buffer(win, w, h);
            return 0;
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            goblin_key k = goblin_window__vk_to_key(wparam);
            if (k != GOBLIN_KEY_UNKNOWN) win->input.keys[k] = true;
            return 0;
        }

        case WM_KEYUP:
        case WM_SYSKEYUP: {
            goblin_key k = goblin_window__vk_to_key(wparam);
            if (k != GOBLIN_KEY_UNKNOWN) win->input.keys[k] = false;
            return 0;
        }

        case WM_LBUTTONDOWN: win->input.mouse_buttons[GOBLIN_MOUSE_LEFT] = true; return 0;
        case WM_LBUTTONUP:   win->input.mouse_buttons[GOBLIN_MOUSE_LEFT] = false; return 0;
        case WM_RBUTTONDOWN: win->input.mouse_buttons[GOBLIN_MOUSE_RIGHT] = true; return 0;
        case WM_RBUTTONUP:   win->input.mouse_buttons[GOBLIN_MOUSE_RIGHT] = false; return 0;
        case WM_MBUTTONDOWN: win->input.mouse_buttons[GOBLIN_MOUSE_MIDDLE] = true; return 0;
        case WM_MBUTTONUP:   win->input.mouse_buttons[GOBLIN_MOUSE_MIDDLE] = false; return 0;

        case WM_MOUSEMOVE:
            win->input.mouse_x = GET_X_LPARAM(lparam);
            win->input.mouse_y = GET_Y_LPARAM(lparam);
            return 0;

        case WM_DESTROY:
            return 0;

        default:
            return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

static wchar_t *goblin_window__utf8_to_wide(const char *s)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    wchar_t *w = (wchar_t *)malloc((size_t)len * sizeof(wchar_t));
    if (w) MultiByteToWideChar(CP_UTF8, 0, s, -1, w, len);
    return w;
}

goblin_window *goblin_window_create(int width, int height, const char *title)
{
    static const wchar_t *class_name = L"GoblinWindowClass";
    static bool class_registered = false;

    goblin_window *win = (goblin_window *)calloc(1, sizeof(goblin_window));
    if (!win) return NULL;

    HINSTANCE hinstance = GetModuleHandleW(NULL);

    if (!class_registered) {
        WNDCLASSW wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.lpfnWndProc = goblin_window__wndproc;
        wc.hInstance = hinstance;
        wc.lpszClassName = class_name;
        wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        if (!RegisterClassW(&wc)) {
            free(win);
            return NULL;
        }
        class_registered = true;
    }

    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    wchar_t *wtitle = goblin_window__utf8_to_wide(title ? title : "goblin window");

    win->hwnd = CreateWindowExW(
        0, class_name, wtitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hinstance, win);

    free(wtitle);

    if (!win->hwnd) {
        free(win);
        return NULL;
    }

    goblin_window__resize_buffer(win, width, height);

    ShowWindow(win->hwnd, SW_SHOW);
    UpdateWindow(win->hwnd);

    return win;
}

void goblin_window_destroy(goblin_window *win)
{
    if (!win) return;
    if (win->hwnd) DestroyWindow(win->hwnd);
    free(win->pixels);
    free(win);
}

void goblin_window_poll_events(goblin_window *win)
{
    MSG msg;

    memcpy(win->input.keys_prev, win->input.keys, sizeof(win->input.keys));
    win->resized = false;

    while (PeekMessageW(&msg, win->hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void goblin_window_present(goblin_window *win)
{
    HDC hdc = GetDC(win->hwnd);
    StretchDIBits(hdc,
        0, 0, win->width, win->height,
        0, 0, win->width, win->height,
        win->pixels, &win->bmi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(win->hwnd, hdc);
}

double goblin_window_time(void)
{
    static LARGE_INTEGER freq;
    static LARGE_INTEGER start;
    static bool init = false;
    LARGE_INTEGER now;

    if (!init) {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
        init = true;
    }

    QueryPerformanceCounter(&now);
    return (double)(now.QuadPart - start.QuadPart) / (double)freq.QuadPart;
}

void goblin_window_sleep_ms(unsigned int ms)
{
    Sleep(ms);
}

/* ===================================================================== */
/*  X11 BACKEND                                                          */
/* ===================================================================== */
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <sys/time.h>

struct goblin_window {
    Display *display;
    Window handle;
    GC gc;
    Atom wm_delete_window;
    int width, height;
    bool should_close;
    bool resized;
    uint32_t *pixels;
    struct goblin_window_input_state input;
};

static goblin_key goblin_window__keysym_to_key(KeySym sym)
{
    if (sym >= XK_a && sym <= XK_z) return (goblin_key)(GOBLIN_KEY_A + (sym - XK_a));
    if (sym >= XK_A && sym <= XK_Z) return (goblin_key)(GOBLIN_KEY_A + (sym - XK_A));
    if (sym >= XK_0 && sym <= XK_9) return (goblin_key)(GOBLIN_KEY_0 + (sym - XK_0));

    switch (sym) {
        case XK_space:     return GOBLIN_KEY_SPACE;
        case XK_Return:    return GOBLIN_KEY_ENTER;
        case XK_Escape:    return GOBLIN_KEY_ESCAPE;
        case XK_Tab:       return GOBLIN_KEY_TAB;
        case XK_BackSpace: return GOBLIN_KEY_BACKSPACE;
        case XK_Left:      return GOBLIN_KEY_LEFT;
        case XK_Right:     return GOBLIN_KEY_RIGHT;
        case XK_Up:        return GOBLIN_KEY_UP;
        case XK_Down:      return GOBLIN_KEY_DOWN;
        case XK_Shift_L:   return GOBLIN_KEY_LSHIFT;
        case XK_Shift_R:   return GOBLIN_KEY_RSHIFT;
        case XK_Control_L: return GOBLIN_KEY_LCTRL;
        case XK_Control_R: return GOBLIN_KEY_RCTRL;
        case XK_Alt_L:     return GOBLIN_KEY_LALT;
        case XK_Alt_R:     return GOBLIN_KEY_RALT;
        default:           return GOBLIN_KEY_UNKNOWN;
    }
}

static void goblin_window__resize_buffer(goblin_window *win, int width, int height)
{
    if (width < 1) width = 1;
    if (height < 1) height = 1;

    free(win->pixels);
    win->pixels = (uint32_t *)calloc((size_t)width * (size_t)height, sizeof(uint32_t));
    win->width = width;
    win->height = height;
    win->resized = true;
}

goblin_window *goblin_window_create(int width, int height, const char *title)
{
    goblin_window *win = (goblin_window *)calloc(1, sizeof(goblin_window));
    if (!win) return NULL;

    win->display = XOpenDisplay(NULL);
    if (!win->display) {
        free(win);
        return NULL;
    }

    int screen = DefaultScreen(win->display);
    Window root = RootWindow(win->display, screen);

    unsigned long black = BlackPixel(win->display, screen);
    unsigned long white = WhitePixel(win->display, screen);

    win->handle = XCreateSimpleWindow(win->display, root, 0, 0,
        (unsigned int)width, (unsigned int)height, 0, black, white);

    XSelectInput(win->display, win->handle,
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
        StructureNotifyMask);

    XStoreName(win->display, win->handle, title ? title : "goblin window");

    win->wm_delete_window = XInternAtom(win->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(win->display, win->handle, &win->wm_delete_window, 1);

    win->gc = XCreateGC(win->display, win->handle, 0, NULL);

    XMapWindow(win->display, win->handle);
    XFlush(win->display);

    goblin_window__resize_buffer(win, width, height);
    win->resized = false; /* initial allocation is not a user resize */

    return win;
}

void goblin_window_destroy(goblin_window *win)
{
    if (!win) return;
    if (win->gc) XFreeGC(win->display, win->gc);
    if (win->handle) XDestroyWindow(win->display, win->handle);
    if (win->display) XCloseDisplay(win->display);
    free(win->pixels);
    free(win);
}

void goblin_window_poll_events(goblin_window *win)
{
    XEvent ev;

    memcpy(win->input.keys_prev, win->input.keys, sizeof(win->input.keys));
    win->resized = false;

    while (XPending(win->display) > 0) {
        XNextEvent(win->display, &ev);

        switch (ev.type) {
            case ClientMessage:
                if ((Atom)ev.xclient.data.l[0] == win->wm_delete_window)
                    win->should_close = true;
                break;

            case ConfigureNotify:
                if (ev.xconfigure.width != win->width || ev.xconfigure.height != win->height)
                    goblin_window__resize_buffer(win, ev.xconfigure.width, ev.xconfigure.height);
                break;

            case KeyPress:
            case KeyRelease: {
                KeySym sym = XLookupKeysym(&ev.xkey, 0);
                goblin_key k = goblin_window__keysym_to_key(sym);
                if (k != GOBLIN_KEY_UNKNOWN)
                    win->input.keys[k] = (ev.type == KeyPress);
                break;
            }

            case ButtonPress:
            case ButtonRelease: {
                bool down = (ev.type == ButtonPress);
                if (ev.xbutton.button == Button1) win->input.mouse_buttons[GOBLIN_MOUSE_LEFT] = down;
                else if (ev.xbutton.button == Button3) win->input.mouse_buttons[GOBLIN_MOUSE_RIGHT] = down;
                else if (ev.xbutton.button == Button2) win->input.mouse_buttons[GOBLIN_MOUSE_MIDDLE] = down;
                break;
            }

            case MotionNotify:
                win->input.mouse_x = ev.xmotion.x;
                win->input.mouse_y = ev.xmotion.y;
                break;

            default:
                break;
        }
    }
}

void goblin_window_present(goblin_window *win)
{
    XImage *image = XCreateImage(win->display,
        DefaultVisual(win->display, DefaultScreen(win->display)),
        (unsigned int)DefaultDepth(win->display, DefaultScreen(win->display)),
        ZPixmap, 0, (char *)win->pixels,
        (unsigned int)win->width, (unsigned int)win->height,
        32, 0);

    if (!image) return;

    XPutImage(win->display, win->handle, win->gc, image,
        0, 0, 0, 0, (unsigned int)win->width, (unsigned int)win->height);

    /* The pixel buffer is owned by goblin_window, not by the XImage: detach
       it before destroying so XDestroyImage doesn't free() our buffer. */
    image->data = NULL;
    XDestroyImage(image);

    XFlush(win->display);
}

double goblin_window_time(void)
{
    static struct timespec start;
    static bool init = false;
    struct timespec now;

    if (!init) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        init = true;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)(now.tv_sec - start.tv_sec) +
           (double)(now.tv_nsec - start.tv_nsec) / 1e9;
}

void goblin_window_sleep_ms(unsigned int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

#else
#error "goblin_window.h: unsupported platform (only Win32 and X11/Xlib are implemented)"
#endif

/* ===================================================================== */
/*  SHARED (PLATFORM-INDEPENDENT) API                                    */
/* ===================================================================== */

bool goblin_window_should_close(const goblin_window *win)
{
    return win->should_close;
}

void goblin_window_close(goblin_window *win)
{
    win->should_close = true;
}

uint32_t *goblin_window_pixels(goblin_window *win)
{
    return win->pixels;
}

int goblin_window_width(const goblin_window *win)
{
    return win->width;
}

int goblin_window_height(const goblin_window *win)
{
    return win->height;
}

bool goblin_window_resized(const goblin_window *win)
{
    return win->resized;
}

bool goblin_window_key_down(const goblin_window *win, goblin_key key)
{
    if (key <= GOBLIN_KEY_UNKNOWN || key >= GOBLIN_KEY_COUNT) return false;
    return win->input.keys[key];
}

bool goblin_window_key_pressed(const goblin_window *win, goblin_key key)
{
    if (key <= GOBLIN_KEY_UNKNOWN || key >= GOBLIN_KEY_COUNT) return false;
    return win->input.keys[key] && !win->input.keys_prev[key];
}

bool goblin_window_key_released(const goblin_window *win, goblin_key key)
{
    if (key <= GOBLIN_KEY_UNKNOWN || key >= GOBLIN_KEY_COUNT) return false;
    return !win->input.keys[key] && win->input.keys_prev[key];
}

void goblin_window_mouse_pos(const goblin_window *win, int *x, int *y)
{
    if (x) *x = win->input.mouse_x;
    if (y) *y = win->input.mouse_y;
}

bool goblin_window_mouse_down(const goblin_window *win, goblin_mouse_button button)
{
    if (button < 0 || button >= GOBLIN_MOUSE_COUNT) return false;
    return win->input.mouse_buttons[button];
}

#endif /* GOBLIN_WINDOW_IMPLEMENTATION */

#endif /* GOBLIN_WINDOW_H */
