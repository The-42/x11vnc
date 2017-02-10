/*
   Copyright (C) 2002-2010 Karl J. Runge <runge@karlrunge.com>
   All rights reserved.

This file is part of x11vnc.

x11vnc is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

x11vnc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with x11vnc; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
or see <http://www.gnu.org/licenses/>.

In addition, as a special exception, Karl J. Runge
gives permission to link the code of its release of x11vnc with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.
*/

/* -- xkb_bell.c -- */
#include <errno.h>

#include "x11vnc.h"
#include "xwrappers.h"
#include "connections.h"
#include "xkb_bell.h"

/*
 * Bell event handling.  Requires XKEYBOARD extension.
 */
#if HAVE_XKEYBOARD

static int base_event_type = 0;
static int have_xkb = 0;
static int watch_active = 0;
static int sound_active = 0;

/*
 * check for XKEYBOARD, set up base_event_type
 */
void xkbb_init(Display *dpy) {
	int ir, reason;
	int op, ev, er, maj, min;
	Display* xkb_disp;

	if (!dpy)
		return;

	have_xkb = 1;

	if (!XkbQueryExtension(dpy, &op, &ev, &er, &maj, &min)) {
		have_xkb = 0;
		if (! quiet) {
			rfbLog("warning: XKEYBOARD extension not present.\n");
		}
		return;
	}

	if (! xauth_raw(1)) {
		return;
	}

	/* XkbOpenDisplay returns a connection, close it to avoid a memleak */
	xkb_disp = XkbOpenDisplay(DisplayString(dpy), &base_event_type, &ir,
		NULL, NULL, &reason);

	if (!xkb_disp) {
		if (! quiet) {
			rfbLog("warning: disabling XKEYBOARD. XkbOpenDisplay"
			    " failed.\n");
		}
		base_event_type = 0;
		have_xkb = 0;
	} else {
		XCloseDisplay(xkb_disp);
	}
	xauth_raw(0);
}

void xkbb_setup_watch(Display *dpy)
{
	if (!have_xkb) {
		if (! quiet) {
			rfbLog("warning: disabling bell. XKEYBOARD ext. "
			    "not present.\n");
		}
		watch_active = 0;
		sound_active = 0;
		return;
	}

	if (!dpy)
		return;

	XkbSelectEvents(dpy, XkbUseCoreKbd, XkbBellNotifyMask, 0);

	if (!watch_active) {
		return;
	}
	if (! XkbSelectEvents(dpy, XkbUseCoreKbd, XkbBellNotifyMask,
	    XkbBellNotifyMask) ) {
		if (! quiet) {
			rfbLog("warning: disabling bell. XkbSelectEvents"
			    " failed.\n");
		}
		watch_active = 0;
		sound_active = 0;
	}
}

/*
 * We call this periodically to process any bell events that have
 * taken place.
 */
void xkbb_check_event(Display *dpy)
{
	XkbAnyEvent *xkb_ev;
	XEvent xev;
	int got_bell = 0;

	if (!base_event_type || dpy) {
		return;
	}

	/* caller does X_LOCK */
	if (! XCheckTypedEvent(dpy, base_event_type, &xev)) {
		return;
	}
	if (!watch_active) {
		/* we return here to avoid xkb events piling up */
		return;
	}

	xkb_ev = (XkbAnyEvent *) &xev;
	if (xkb_ev->xkb_type == XkbBellNotify) {
		got_bell = 1;
	}

	if (got_bell && sound_active) {
		if (! all_clients_initialized()) {
			rfbLog("%s: not sending bell: "
			    "uninitialized clients\n", __func__);
		} else {
			if (screen && client_count) {
				rfbSendBell(screen);
			}
		}
	}
}

int xkbb_get_base_event_type(void)
{
	return base_event_type;
}

int xkbb_watch_get(void)
{
	return watch_active;
}

void xkbb_watch_set(int active)
{
	watch_active = active ? 1 : 0;
}

int xkbb_sound_get(void)
{
	return sound_active;
}

void xkbb_sound_set(int active)
{
	sound_active = active ? 1 : 0;
}

#else
void xkbb_init(Display *dpy) {}

void xkbb_setup_watch(Display *dpy)
{
	watch_active = 0;
	sound_active = 0;
}

void xkbb_check_event(Display *dpy) {}

int xkbb_get_base_event_type(void)
{
	return -EINVAL;
}

int xkbb_watch_get(void)
{
	return 0;
}

void xkbb_watch_set(int active) {}

int xkbb_sound_get(void)
{
	return 0;
}

void xkbb_sound_set(int active) {}
#endif

int xkb_present(void)
{
	return have_xkb;
}
