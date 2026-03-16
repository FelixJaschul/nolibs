#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

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

struct {
    float x, y, z, yaw, pitch;
} camera = {0, 0, -5, 0, 0};

void draw(const int screen)
{
    /* background */
	const unsigned int bs = 50;
    for (int i = 0; i < WIDTH*HEIGHT; i++) {
        if (((i % WIDTH)/bs + (i / WIDTH)/bs) % 2 == 0) s.pixels[i] = 0x141414;
        else s.pixels[i] = 0x101010;
    }

    /* triangle vertices in world space */
    const float triangle[3][3] = { {0, 1.5, 0}, {-2, -1, 0}, {2, -1, 0} };

    /* project */
    int px[3], py[3];
    for (int i = 0; i < 3; i++)
    {
        float dx = triangle[i][0] - camera.x;
        float dy = triangle[i][1] - camera.y;
        float dz = triangle[i][2] - camera.z;

        /* apply yaw rotation around Y */
        const float rx = dx * cosf(camera.yaw) - dz * sinf(camera.yaw);
        const float rz = dx * sinf(camera.yaw) + dz * cosf(camera.yaw);
        dx = rx;
        dz = rz;

        /* apply pitch rotation around X */
        const float ry = dy * cosf(camera.pitch) - dz * sinf(camera.pitch);
        const float rz2 = dy * sinf(camera.pitch) + dz * cosf(camera.pitch);
        dy = ry;
        dz = rz2;

        if (dz < 0.1f) dz = 0.1f;

        px[i] = WIDTH/2  + (int)(dx*(500.0f / dz));
        py[i] = HEIGHT/2 - (int)(dy*(500.0f / dz));
    }

    /* fill triangle with interpolated RGB (rainbow) */
    const int ax = px[0], ay = py[0];
    const int bx = px[1], by = py[1];
    const int cx = px[2], cy = py[2];
    float area = (float)((bx - ax) * (cy - ay) - (by - ay) * (cx - ax));
    if (area != 0.0f)
	{
        const int min_x = (ax < bx ? (ax < cx ? ax : cx) : (bx < cx ? bx : cx));
        const int max_x = (ax > bx ? (ax > cx ? ax : cx) : (bx > cx ? bx : cx));
        const int min_y = (ay < by ? (ay < cy ? ay : cy) : (by < cy ? by : cy));
        const int max_y = (ay > by ? (ay > cy ? ay : cy) : (by > cy ? by : cy));

        const int x0 = min_x < 0 ? 0 : min_x;
        const int x1 = max_x >= WIDTH ? WIDTH - 1 : max_x;
        const int y0 = min_y < 0 ? 0 : min_y;
        const int y1 = max_y >= HEIGHT ? HEIGHT - 1 : max_y;

        const float inv_area = 1.0f / area;
        for (int y = y0; y <= y1; y++)
		{
            for (int x = x0; x <= x1; x++)
			{
                float w0 = (float)((bx - cx) * (y - cy) + (cy - by) * (x - cx)) * inv_area;
                float w1 = (float)((cx - ax) * (y - ay) + (ay - cy) * (x - ax)) * inv_area;
                float w2 = (float)((ax - bx) * (y - by) + (by - ay) * (x - bx)) * inv_area;

                if (area < 0.0f) { w0 = -w0; w1 = -w1; w2 = -w2; }
                if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
				{
                    int r = (int)(w0 * 255.0f);
                    int g = (int)(w1 * 255.0f);
                    int b = (int)(w2 * 255.0f);
                    if (r < 0) r = 0; if (r > 255) r = 255;
                    if (g < 0) g = 0; if (g > 255) g = 255;
                    if (b < 0) b = 0; if (b > 255) b = 255;
                    s.pixels[y*WIDTH + x] = (unsigned int)((r << 16) | (g << 8) | b);
                }
            }
        }
    }

    XPutImage(s.display, s.window, DefaultGC(s.display, screen), s.ximage, 0,0,0,0, WIDTH, HEIGHT);
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

    XSelectInput(s.display, s.window, ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | StructureNotifyMask);
    XMapWindow(s.display, s.window);

    s.ximage = XCreateImage(
        s.display, DefaultVisual(s.display, screen), DefaultDepth(s.display, screen), ZPixmap, 0, (char*)s.pixels, WIDTH, HEIGHT, 32, 0);

    int shift_down = 0, grab_active = 0;
    const float mouse_sens = 0.005f;

    Pixmap blank = XCreatePixmap(s.display, s.window, 1, 1, 1);
    XColor dummy;
    Cursor invisible = XCreatePixmapCursor(s.display, blank, blank, &dummy, &dummy, 0, 0);
    XFreePixmap(s.display, blank);

    s.running = 1;
    while (s.running) {
		while (XPending(s.display)) {
        XNextEvent(s.display, &s.event);
        switch (s.event.type)
        {
            case Expose: draw(screen);
            case KeyPress:
                const KeySym key = XLookupKeysym(&s.event.xkey, 0);
                if (key == XK_Escape) s.running = 0;
                if (key == XK_Shift_L || key == XK_Shift_R) {
                    shift_down = 1;
                    if (!grab_active) {
                        XGrabPointer(s.display, s.window, True, PointerMotionMask, GrabModeAsync, GrabModeAsync, s.window, invisible, CurrentTime);
                        XWarpPointer(s.display, None, s.window, 0, 0, 0, 0, WIDTH / 2, HEIGHT / 2);
                        grab_active = 1;
                    }
                }
                /* move camera */ float speed = 0.1f;
                if (key == XK_w) { camera.x += sinf(camera.yaw)*speed; camera.z += cosf(camera.yaw)*speed; }
                if (key == XK_s) { camera.x -= sinf(camera.yaw)*speed; camera.z -= cosf(camera.yaw)*speed; }
                if (key == XK_a) { camera.x -= cosf(camera.yaw)*speed; camera.z += sinf(camera.yaw)*speed; }
                if (key == XK_d) { camera.x += cosf(camera.yaw)*speed; camera.z -= sinf(camera.yaw)*speed; }
				break;
            case KeyRelease:
                const KeySym key_release = XLookupKeysym(&s.event.xkey, 0);
                if (key_release == XK_Shift_L || key_release == XK_Shift_R) {
                    shift_down = 0;
                    if (grab_active) {
                        XUngrabPointer(s.display, CurrentTime);
                        grab_active = 0;
                    }
                } break;
            case ClientMessage:
                if ((Atom)s.event.xclient.data.l[0] == wm_delete) s.running = 0;
                break;
            default: break;
        }
    	}

    	if (shift_down && grab_active)
		{
        	Window root, child;
        	int root_x, root_y, win_x, win_y;
        	unsigned int mask;
        	if (XQueryPointer(s.display, s.window, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask))
			{
            	const int dx = win_x - WIDTH / 2;
            	const int dy = win_y - HEIGHT / 2;
            	if (dx != 0 || dy != 0) {
                	camera.yaw += dx * mouse_sens;
                	camera.pitch -= dy * mouse_sens;
                	if (camera.pitch > 1.5f) camera.pitch = 1.5f;
                	if (camera.pitch < -1.5f) camera.pitch = -1.5f;
                	XWarpPointer(s.display, None, s.window, 0, 0, 0, 0, WIDTH / 2, HEIGHT / 2);
            	}
        	}
		}
		draw(screen);
	}

#undef WIDTH
#undef HEIGHT
    XDestroyWindow(s.display, s.window);
    XCloseDisplay(s.display);
    free(s.pixels);
    return 0;
}

#undef A
#undef L
