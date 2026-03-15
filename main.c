#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct {
    Display* display;
    Window window;
    XEvent event;
    unsigned int running;
} s = {0};

#define A(x) do { if (!(x)) { printf("Assertion failed: %s\n", #x); exit(1); } } while(0)
#define L(x) do { printf("%s\n", x); } while(0)

int main(void)
{
    s.display = XOpenDisplay(NULL);
    A(s.display);

#define screen DefaultScreen(s.display)
#define root RootWindow(s.display, screen)

#define width 1270
#define height 800

    s.window = XCreateSimpleWindow(
        s.display, root, None, None, width, height, None, None, BlackPixel(s.display, screen));

    XStoreName(s.display, s.window, "xeleven _f");

    Atom wm_delete = XInternAtom(s.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(s.display, s.window, &wm_delete, 1);

    XSelectInput(s.display, s.window, ExposureMask | KeyPressMask | StructureNotifyMask);
    XMapWindow(s.display, s.window);

    s.running = 1;
    while (s.running)
    {
        XNextEvent(s.display, &s.event);

        switch (s.event.type)
        {
            case Expose:
                /* No drawing yet. */
                break;
            case KeyPress:
#define x XLookupKeysym(&s.event.xkey, 0)
                if (x == XK_Escape) s.running = 0;
                if (x == XK_Return) L("hi");
#undef x
                break;
            case ClientMessage:
                if ((Atom)s.event.xclient.data.l[0] == wm_delete) s.running = 0;
                break;
            default:
                break;
        }
    }

#undef screen
#undef root
#undef width
#undef height
    XDestroyWindow(s.display, s.window);
    XCloseDisplay(s.display);
    return 0;
}

#undef A
#undef L
