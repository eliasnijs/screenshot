#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "types.h"

enum SelectionState { COMPLETE, IN_PROGRESS, READY, };
char *SELECTION_STATE_REPR_TABLE[] = {
	"COMPLETE", "IN_PROGRESS", "READY"
};

void
bgra2rgba(U8 *pixels, U64 size)
{
	for (U32 i = 0; i < size; i += 4) {
		U8 R = pixels[i];
		pixels[i] = pixels[i + 2];
		pixels[i + 2] = R;
	}
}

U32
screenshot(char *path)
{
	U32 status = 0;

	Display *display = XOpenDisplay(0);
	Window root = DefaultRootWindow(display);

	Cursor cursor1 = XCreateFontCursor(display, XC_diamond_cross);
	XGrabPointer(display, root, False,
		     ButtonMotionMask | ButtonPressMask | ButtonReleaseMask,
		     GrabModeAsync, GrabModeAsync, root, cursor1, CurrentTime);

	XGCValues gcv;
	gcv.function = GXinvert;
	gcv.subwindow_mode = IncludeInferiors;
	GC gc = XCreateGC(display, root,
                    GCFunction | GCSubwindowMode,
                    &gcv);


	enum SelectionState state = READY;
	S32 x = 0, y = 0, rx = 0, ry = 0, w = 0, h = 0;
	S32 prev_rx = 0, prev_ry = 0, prev_w = 0, prev_h = 0;
	Bool rect_drawn = False;
	XEvent event;

	while (!(state == COMPLETE)) {
		while (!(state == COMPLETE) && XPending(display)) {
			XNextEvent(display, &event);
			switch (event.type) {
			case (MotionNotify): {
				if (state == IN_PROGRESS) {
					if (rect_drawn) {
						XDrawRectangle(display, root, gc,
							       prev_rx, prev_ry,
							       prev_w, prev_h);
					}
					w = event.xmotion.x - x;
					h = event.xmotion.y - y;
					rx = x;
					ry = y;

					if (w < 0) {
						rx = x + w;
						w = -1 * w;
					}
					if (h < 0) {
						ry = y + h;
						h = -1 * h;
					}
					XDrawRectangle(display, root, gc,
						       rx, ry,
						       w, h);
					prev_rx = rx;
					prev_ry = ry;
					prev_w = w;
					prev_h = h;
					rect_drawn = True;
					XFlush(display);
				}
			} break;
			case (ButtonPress): {
				x = event.xbutton.x;
				y = event.xbutton.y;
				state = IN_PROGRESS;
			} break;
			case (ButtonRelease): {
				state = COMPLETE;
			} break;
			}
		}

	}

	if (rect_drawn) {
		XDrawRectangle(display, root, gc, prev_rx, prev_ry, prev_w, prev_h);
	}
	XSync(display, 1);

	if (w && h) {
		XImage *image = XGetImage(display, root, rx, ry, w, h, AllPlanes, ZPixmap);
		bgra2rgba((U8 *)image->data, w * h * 4);
		status = stbi_write_png(path, w, h, 4, image->data, w * 4);
		XDestroyImage(image);
	} else {
		status = 1;
	}

	XFreeGC(display, gc);
	XCloseDisplay(display);
	return(status);
}


S32
clipboard_screenshot()
{
	char filepath[256] = {};
	time_t rawtime;
	time(&rawtime);
	strftime(filepath, sizeof(filepath),
		 "/tmp/screenshot-%Y-%m-%d-%H:%M:%S.png",
		 localtime(&rawtime));

	screenshot(filepath);

	char command[512];
	snprintf(command, sizeof(command),
		 "xclip -selection clipboard -t image/png -i %s", filepath);
	system(command);

	remove(filepath);
	return 0;
}

S32
save_screenshot(char *inputstr)
{
	char filepath[256] = {};
	if (strcmp(inputstr, "-t") == 0) {
		time_t rawtime;
		time(&rawtime);
		strftime(filepath, sizeof(filepath),
			 "./screenshot-%Y-%m-%d-%H:%M:%S.png",
			 localtime(&rawtime));
	} else {
		strcpy(&filepath[0], inputstr);
	}


	if (access(filepath, F_OK) != -1) {
		printf("file already exists\n");
		return 1;
	}

	return screenshot(filepath);
}

S32
main(S32 argc, char *argv[])
{
	argv = &argv[1];
	argc -= 1;
	switch (argc) {
	case 0: return clipboard_screenshot();
	case 1: return save_screenshot(argv[0]);
	default:
		printf("invalid arguments!\n");
		return 1;
	}
	return 0;
}
