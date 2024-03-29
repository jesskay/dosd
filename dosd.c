/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include "draw.h"

typedef enum {
    ANCHOR_TOP,
    ANCHOR_LEFT,
    ANCHOR_MIDDLE,
    ANCHOR_BOTTOM,
    ANCHOR_RIGHT
} anchor_t;

#define DEFFONT "Monospace:pixelsize=12"

#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#define MIN(a, b)  ((a) < (b) ? (a) : (b))

static void cleanup(void);
static void drawbar(void);
static void run(void);
static void setup(void);
static void usage(void);

static char *text = NULL;
static int bh, bw;
static int bxperc = 0;
static int byperc = 0;
static anchor_t vanchor = ANCHOR_TOP;
static anchor_t hanchor = ANCHOR_LEFT;
static const char *font = NULL;
static const char *bgcolor = "#222222";
static const char *fgcolor = "#bbbbbb";
static int bgalpha = 0xff;
static ColorSet *col;
static struct itimerval timer;
static Bool timeout = False;
static Bool running = True;
static int ret = 0;
static DC *dc;
static Window win;

int
main(int argc, char *argv[]) {
	int i;

	memset(&timer, 0, sizeof(struct itimerval));

	for(i = 1; i < argc; i++)
		/* these options take no arguments */
		if(!strcmp(argv[i], "-v")) {      /* prints version information */
			puts("dosd-"VERSION", © 2014 dosd engineers (pfff), see LICENSE for details ONCE IT EXISTS");
			exit(EXIT_SUCCESS);
		}
		else if(i+1 == argc) {
			usage();
                }
		/* these options take one argument */
		else if(!strcmp(argv[i], "-x")) { /* set x percentage */
                        bxperc = atoi(argv[++i]);
                        bxperc = MIN(100, bxperc);
                        bxperc = MAX(0, bxperc);
                }
		else if(!strcmp(argv[i], "-y")) { /* set y percentage */
                        byperc = atoi(argv[++i]);
                        byperc = MIN(100, byperc);
                        byperc = MAX(0, byperc);
                }
		else if(!strcmp(argv[i], "-ax")) { /* set x anchor */
                        switch(argv[++i][0]) {
                            case 'l':
                            case 'L':
                                {
                                    hanchor = ANCHOR_LEFT;
                                    break;
                                }
                            case 'm':
                            case 'M':
                                {
                                    hanchor = ANCHOR_MIDDLE;
                                    break;
                                }
                            case 'r':
                            case 'R':
                                {
                                    hanchor = ANCHOR_RIGHT;
                                    break;
                                }
                        }
                }
		else if(!strcmp(argv[i], "-ay")) { /* set x anchor */
                        switch(argv[++i][0]) {
                            case 't':
                            case 'T':
                                {
                                    vanchor = ANCHOR_TOP;
                                    break;
                                }
                            case 'm':
                            case 'M':
                                {
                                    vanchor = ANCHOR_MIDDLE;
                                    break;
                                }
                            case 'b':
                            case 'B':
                                {
                                    vanchor = ANCHOR_BOTTOM;
                                    break;
                                }
                        }
                }
		else if(!strcmp(argv[i], "-fn")) {/* font or font set */
			font = argv[++i];
                }
		else if(!strcmp(argv[i], "-bb")) {/* background color */
			bgcolor = argv[++i];
                }
		else if(!strcmp(argv[i], "-ba")) { /* set alpha */
                        bgalpha = atoi(argv[++i]);
                        bgalpha = MIN(255, bgalpha);
                        bgalpha = MAX(0, bgalpha);
                }
		else if(!strcmp(argv[i], "-bf")) {/* foreground color */
			fgcolor = argv[++i];
                }
		else if(!strcmp(argv[i], "-t")) { /* display text */
			text = argv[++i];
                }
		else if(!strcmp(argv[i], "-d")) { /* automatically disappear after a delay */
			timeout = True;
			timer.it_value.tv_sec = atoi(argv[++i]);
                }
		else {
			usage();
                }

	if(text) {
		dc = initdc(bgalpha != 0xff);
		initfont(dc, font ? font : DEFFONT);
		col = initcolor(dc, fgcolor, bgcolor, bgalpha);

		setup();
		run();

		cleanup();
        } else {
            usage();
        }
	return ret;
}

void
cleanup(void) {
	freecol(dc, col);
	XDestroyWindow(dc->dpy, win);
	freedc(dc);
}

void
drawbar(void) {
	dc->x = 0;
	dc->y = 0;
	dc->h = bh;
	drawrect(dc, 0, 0, bw, bh, True, col->BG);

        dc->x += (bw * bxperc) / 100;
        if(hanchor == ANCHOR_RIGHT) {
            dc->x -= textw(dc, text);
        } else if (hanchor == ANCHOR_MIDDLE) {
            dc->x -= textw(dc, text) / 2;
        }
	dc->w = textw(dc, text);
	drawtext(dc, text, col);
	dc->x += dc->w;

	mapdc(dc, win, bw, bh);
}

void
handlealarm(int unused_signum) {
	XClientMessageEvent dummy_event;

	running = False;

	memset(&dummy_event, 0, sizeof(XClientMessageEvent));
	dummy_event.type = ClientMessage;
	dummy_event.window = win;
	dummy_event.format = 32;
	XSendEvent(dc->dpy, win, 0, 0, (XEvent*)&dummy_event);
	XFlush(dc->dpy);
}

void
run(void) {
	XEvent ev;

	if(timeout)
		setitimer(ITIMER_REAL, &timer, NULL);

	while(running && !XNextEvent(dc->dpy, &ev)) {
		if(XFilterEvent(&ev, win))
			continue;
		switch(ev.type) {
		case Expose:
			if(ev.xexpose.count == 0)
				mapdc(dc, win, bw, bh);
			break;
		case VisibilityNotify:
			if(ev.xvisibility.state != VisibilityUnobscured)
				XRaiseWindow(dc->dpy, win);
			break;
		}
	}
}

void
setup(void) {
	int x, y, screen = DefaultScreen(dc->dpy);
	Window root = RootWindow(dc->dpy, screen);
	XSetWindowAttributes swa;
	struct sigaction alarm;

	/* register signal handler for alarm */
	alarm.sa_handler = handlealarm;
	sigemptyset(&alarm.sa_mask);
	alarm.sa_flags = 0;
	sigaction(SIGALRM, &alarm, NULL);

	/* calculate bar geometry */
	bh = dc->font.height + 2;
	x = 0;
        y = ((DisplayHeight(dc->dpy, screen) * byperc) / 100);
        if(vanchor == ANCHOR_BOTTOM) {
            y -= bh;
        } else if (vanchor == ANCHOR_MIDDLE) {
            y -= bh / 2;
        }
	bw = DisplayWidth(dc->dpy, screen);

	/* create bar window */
	swa.override_redirect = True;
	swa.background_pixel = col->BG;
	swa.border_pixel = col->BG;
	swa.colormap = dc->cmap;
	swa.event_mask = ExposureMask | VisibilityChangeMask;
	win = XCreateWindow(dc->dpy, root, x, y, bw, bh, 0,
	                    dc->depth, CopyFromParent,
	                    dc->vis,
	                    CWOverrideRedirect | CWBackPixel | CWEventMask
                            | CWBorderPixel | CWColormap, &swa);

	XMapRaised(dc->dpy, win);
	resizedc(dc, bw, bh);
	drawbar();
}

void
usage(void) {
	fputs("usage: dosd [-fn FONT] [-bb COLOR] [-bf COLOR] [-ba ALPHA]\n"
	      "            [-x PERCENT] [-y PERCENT]\n"
	      "            [-ax LEFT|MIDDLE|RIGHT] [-ay TOP|MIDDLE|BOTTOM]\n"
	      "            [-t TEXT] [-d SECS] [-v]\n", stderr);
	exit(EXIT_FAILURE);
}
