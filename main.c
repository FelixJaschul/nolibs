#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct {
    Display* display;
    Window window;
    XEvent event;
    unsigned int running;

    /* Framebuffer */
    unsigned int width, height;
    unsigned int* pixels;
    XImage* ximage;
} s = {0};

#define A(x) do { if (!(x)) { printf("Assertion failed: %s\n", #x); exit(1); } } while(0)
#define L(x) do { printf("%s\n", x); } while(0)

typedef struct { float x, y, z; } vec3;

static inline vec3 add(const vec3 a, const vec3 b) { return (vec3){a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline vec3 sub(const vec3 a, const vec3 b) { return (vec3){a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline vec3 mul(const vec3 a, const float f) { return (vec3){a.x*f, a.y*f, a.z*f}; }
static inline float dot(const vec3 a, const vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline float len(const vec3 v) { return sqrtf(dot(v,v)); }
static inline vec3 norm(const vec3 v) { float l = len(v); return mul(v, 1.0f/l); }

float map(const vec3 p)
{
    const float dx = fmaxf(fabsf(p.x)-0.5f, 0.0f);
    const float dy = fmaxf(fabsf(p.y)-0.5f, 0.0f);
    const float dz = fmaxf(fabsf(p.z)-0.5f, 0.0f);
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

float raymarch(const vec3 ro, const vec3 rd, const int steps, const float maxDist)
{
    float t = 0.0f;
    for(int i=0;i<steps;i++)
    {
        const float d = map(add(ro, mul(rd, t)));
        if (d < 0.001f) return t;
        t += d;
        if (t >= maxDist) break;
    }
    return maxDist;
}

vec3 normal(const vec3 p)
{
    constexpr float eps = 0.001f;
    const float dx = map(add(p, (vec3){eps,0,0})) - map(add(p, (vec3){-eps,0,0}));
    const float dy = map(add(p, (vec3){0,eps,0})) - map(add(p, (vec3){0,-eps,0}));
    const float dz = map(add(p, (vec3){0,0,eps})) - map(add(p, (vec3){0,0,-eps}));
    return norm((vec3){dx, dy, dz});
}

unsigned int vec3_to_rgb(const vec3 c)
{
    const int r = (int)(fminf(fmaxf(c.x,0.0f),1.0f)*255.0f);
    const int g = (int)(fminf(fmaxf(c.y,0.0f),1.0f)*255.0f);
    const int b = (int)(fminf(fmaxf(c.z,0.0f),1.0f)*255.0f);
    return (r<<16) | (g<<8) | b;
}

void render_framebuffer()
{
    const vec3 cam = (vec3){0.8f,0.8f,0.8f};
    const vec3 target = (vec3){0,0,0};

    const vec3 forward = norm(sub(target, cam));
    const vec3 right = norm((vec3){forward.z, 0, -forward.x});
    const vec3 up = norm(sub((vec3){0,1,0}, mul(forward, dot((vec3){0,1,0},forward))));

    const float fov = 1.0f;

    for(int y=0;y<s.height;y++)
    for(int x=0;x<s.width;x++)
    {
        const float u = (2.0f*(x+0.5f)/s.width - 1.0f) * fov;
        const float v = (1.0f - 2.0f*(y+0.5f)/s.height) * fov;
        const vec3 dir = norm(add(add(forward, mul(right,u)), mul(up,v)));

        const float t = raymarch(cam, dir, 64, 10.0f);
        vec3 col = (vec3){0,0,0};
        if (t < 10.0f)
        {
            const vec3 p = add(cam, mul(dir, t));
            const vec3 n = normal(p);
            const float diff = fmaxf(dot(n, norm((vec3){1,1,1})), 0.0f);
            col = mul((vec3){1,0.5f,0.25f}, diff);
        }
        s.pixels[y*s.width+x] = vec3_to_rgb(col);
    }
}

int main()
{
    s.width = 640 *2;
    s.height = 570*2;
    s.pixels = malloc(sizeof(unsigned int)*s.width*s.height);
    A(s.pixels);

    s.display = XOpenDisplay(NULL);
    A(s.display);

#define screen DefaultScreen(s.display)

    s.window = XCreateSimpleWindow(
        s.display, RootWindow(s.display, screen), None, None, s.width, s.height, None, None, BlackPixel(s.display, screen));

    XStoreName(s.display, s.window, "xeleven _f");

    auto wm_delete = XInternAtom(s.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(s.display, s.window, &wm_delete, 1);

    XSelectInput(s.display, s.window, ExposureMask | KeyPressMask | StructureNotifyMask);
    XMapWindow(s.display, s.window);

    s.ximage = XCreateImage(
        s.display, DefaultVisual(s.display, screen), DefaultDepth(s.display, screen), ZPixmap, 0, (char*)s.pixels, s.width, s.height, 32, 0);

    s.running = 1;
    while (s.running)
    {
        XNextEvent(s.display, &s.event);

        switch (s.event.type)
        {
            case Expose:
                render_framebuffer();
                XPutImage(
                    s.display, s.window, DefaultGC(s.display, screen), s.ximage, 0,0,0,0, s.width, s.height);
                break;
            case KeyPress:
                const KeySym x = XLookupKeysym(&s.event.xkey, 0);
                if (x == XK_Escape) s.running = 0;
                if (x == XK_Return) L("hi");
                break;
            case ClientMessage:
                if ((Atom)s.event.xclient.data.l[0] == wm_delete) s.running = 0;
                break;
            default:
                break;
        }
    }

#undef screen

    XDestroyImage(s.ximage);
    XDestroyWindow(s.display, s.window);
    XCloseDisplay(s.display);
    free(s.pixels);
    return 0;
}

#undef A
#undef L
