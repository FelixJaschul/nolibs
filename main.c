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

#define WIDTH 1270
#define HEIGHT 800

void draw(const int screen, const unsigned int bs)
{
    /* drawing */ {
        /* background */ for (int i = 0; i < WIDTH*HEIGHT; i++) {
            if (((i % WIDTH)/bs + (i / WIDTH)/bs) % 2 == 0) s.pixels[i] = 0x676767;
            else s.pixels[i] = 0x767676;
        }
    } XPutImage(s.display, s.window, DefaultGC(s.display, screen), s.ximage, 0,0,0,0, WIDTH, HEIGHT);
}

int main()
{
    s.display = XOpenDisplay(NULL);
    A(s.display);

    auto screen = DefaultScreen(s.display);

    s.pixels = malloc(sizeof(unsigned int)*WIDTH*HEIGHT);
    A(s.pixels);

    s.window = XCreateSimpleWindow(
        s.display, RootWindow(s.display, screen), None, None, WIDTH, HEIGHT, None, None, BlackPixel(s.display, screen));

    XStoreName(s.display, s.window, "xeleven _f");

    auto wm_delete = XInternAtom(s.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(s.display, s.window, &wm_delete, 1);

    XSelectInput(s.display, s.window, ExposureMask | KeyPressMask | StructureNotifyMask);
    XMapWindow(s.display, s.window);

    s.ximage = XCreateImage(
        s.display, DefaultVisual(s.display, screen), DefaultDepth(s.display, screen), ZPixmap, 0, (char*)s.pixels, WIDTH, HEIGHT, 32, 0);

    s.running = 1;
    unsigned int bs = 50;
    while (s.running)
    {
        XNextEvent(s.display, &s.event);

        switch (s.event.type)
        {
            case Expose:
                draw(screen, bs);
            case KeyPress:
                const KeySym key = XLookupKeysym(&s.event.xkey, 0);
                if (key == XK_Escape) s.running = 0;
                if (key == XK_plus) if(bs <= 200)bs+=1;
                if (key == XK_minus) if(bs >= 10)bs-=1;
                draw(screen, bs);
            case ClientMessage:
                if ((Atom)s.event.xclient.data.l[0] == wm_delete) s.running = 0;
                break;
            default: break;
        }
    }

#undef WIDTH
#undef HEIGHT
    XDestroyWindow(s.display, s.window);
    XCloseDisplay(s.display);
    return 0;
}

#undef A
#undef L