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

    /* buffer */
    unsigned int* pixels;
    XImage* ximage;
} s = {0};

#define A(x) do { if (!(x)) { printf("Assertion failed: %s\n", #x); exit(1); } } while(0)
#define L(x) do { printf("%s\n", x); } while(0)

int main(void)
{
    s.display = XOpenDisplay(NULL);
    A(s.display);

    auto screen = DefaultScreen(s.display);

#define width 1270
#define height 800

    s.pixels = malloc(sizeof(unsigned int)*width*height);
    A(s.pixels);

    s.window = XCreateSimpleWindow(
        s.display, RootWindow(s.display, screen), None, None, width, height, None, None, BlackPixel(s.display, screen));

    XStoreName(s.display, s.window, "xeleven _f");

    auto wm_delete = XInternAtom(s.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(s.display, s.window, &wm_delete, 1);

    XSelectInput(s.display, s.window, ExposureMask | KeyPressMask | StructureNotifyMask);
    XMapWindow(s.display, s.window);

    s.ximage = XCreateImage(
        s.display, DefaultVisual(s.display, screen), DefaultDepth(s.display, screen), ZPixmap, 0, (char*)s.pixels, width, height, 32, 0);

    s.running = 1;
    while (s.running)
    {
        XNextEvent(s.display, &s.event);

        switch (s.event.type)
        {
            case Expose:
                /* drawing */ {
                    const int bs = 50;
                    for (int i = 0; i < width*height; i++) {
                        if (((i % width)/bs + (i / width)/bs) % 2 == 0) s.pixels[i] = 0x670067;
                        else s.pixels[i] = 0x67;
                    }
                } XPutImage(s.display, s.window, DefaultGC(s.display, screen), s.ximage, 0,0,0,0, width, height);
                break;
            case KeyPress:
                const KeySym key = XLookupKeysym(&s.event.xkey, 0);
                if (key == XK_Escape) s.running = 0;
                if (key == XK_Return) L("hi");
                break;
            case ClientMessage:
                if ((Atom)s.event.xclient.data.l[0] == wm_delete) s.running = 0;
                break;
            default:
                break;
        }
    }

#undef width
#undef height
    XDestroyWindow(s.display, s.window);
    XCloseDisplay(s.display);
    return 0;
}

#undef A
#undef L