/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include "draw.h"

#define DEFFONT "Monospace:pixelsize=12"

static void cleanup(void);
static void drawbar(void);
static void run(void);
static void setup(void);
static void usage(void);

static char *text = NULL;
static int bh, bw;
static const char *font = NULL;
static const char *bgcolor = "#222222";
static const char *fgcolor = "#bbbbbb";
static ColorSet *col;
static struct itimerval timer;
static Bool timeout = False;
static Bool topbar = True;
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
		else if(!strcmp(argv[i], "-b"))   /* appears at the bottom of the screen */
			topbar = False;
		else if(i+1 == argc)
			usage();
		/* these options take one argument */
		else if(!strcmp(argv[i], "-fn"))  /* font or font set */
			font = argv[++i];
		else if(!strcmp(argv[i], "-bb"))  /* background color */
			bgcolor = argv[++i];
		else if(!strcmp(argv[i], "-bf"))  /* foreground color */
			fgcolor = argv[++i];
		else if(!strcmp(argv[i], "-t"))   /* display text */
			text = argv[++i];
		else if(!strcmp(argv[i], "-d")) { /* automatically disappear after a delay */
			timeout = True;
			timer.it_value.tv_sec = atoi(argv[++i]);
			}
		else
			usage();

	if(text) {
		dc = initdc();
		initfont(dc, font ? font : DEFFONT);
		col = initcolor(dc, fgcolor, bgcolor);

		setup();
		run();

		cleanup();
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

	dc->w = textw(dc, text);
	drawtext(dc, text, col);
	dc->x = dc->w;

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
	y = topbar ? 0 : DisplayHeight(dc->dpy, screen) - bh;
	bw = DisplayWidth(dc->dpy, screen);

	/* create bar window */
	swa.override_redirect = True;
	swa.background_pixel = col->BG;
	swa.event_mask = ExposureMask | VisibilityChangeMask;
	win = XCreateWindow(dc->dpy, root, x, y, bw, bh, 0,
	                    DefaultDepth(dc->dpy, screen), CopyFromParent,
	                    DefaultVisual(dc->dpy, screen),
	                    CWOverrideRedirect | CWBackPixel | CWEventMask, &swa);

	XMapRaised(dc->dpy, win);
	resizedc(dc, bw, bh);
	drawbar();
}

void
usage(void) {
	fputs("usage: dosd [-b] [-fn font] [-bb color] [-bf color]\n"
	      "            [-t TEXT] [-d SECS] [-v]\n", stderr);
	exit(EXIT_FAILURE);
}
