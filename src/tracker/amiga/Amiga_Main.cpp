
/* *  tracker/amiga/CGX_Main.cpp
 *
 *  Copyright 2017 Marlon Beijer
 *
 *  This file is part of Milkytracker.
 *
 *  Milkytracker is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Milkytracker is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Milkytracker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 *  CGX_Main.cpp
 *  MilkyTracker CybergraphX (RTG) Amiga front end
 *
 *  Created by Marlon Beijer on 17.09.17.
 *
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#if defined(__AMIGA__) || defined(__amigaos4__)
#	include <exec/exec.h>
#	ifndef AFB_68080
#		define AFB_68080 10
#	endif
#	ifndef AFF_68080
#		define AFF_68080 (1<<AFB_68080)
#	endif
#	if defined(WARPOS) && !defined(AFF68060)
#		define AFF_68060 1
#	endif
#endif
#if defined(__AMIGA__) || defined(WARPUP) || defined(__WARPOS__) || defined(AROS) || defined(__amigaos4__) || defined(__morphos__)
#	include "amigaversion.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

//#include <SDL.h>
//#include "SDL_KeyTranslation.h"
// ---------------------------- Tracker includes ----------------------------
#include "PPUI.h"
#include "DisplayDevice_Amiga.h"
#include "Screen.h"
#include "Tracker.h"
#include "PPMutex.h"
#include "PPSystem_POSIX.h"
#include "PPPath_POSIX.h"
// --------------------------------------------------------------------------

// Amiga specifics
extern struct ExecBase *SysBase;

static int cpuType;
static bool hasFPU = false;

// SDL surface screen
//SDL_TimerID timer;

// Tracker globals
static PPScreen* myTrackerScreen = NULL;
static Tracker* myTracker = NULL;
static DisplayDevice_Amiga * myDisplayDevice = NULL;

// Okay what else do we need?
PPMutex* globalMutex = NULL;
static PPMutex* timerMutex = NULL;
static bool ticking = false;

static pp_uint32 lmyTime;
static PPPoint llastClickPosition = PPPoint(0, 0);
static pp_uint16 lClickCount = 0;

static pp_uint32 rmyTime;
static PPPoint rlastClickPosition = PPPoint(0, 0);
static pp_uint16 rClickCount = 0;

static bool lMouseDown = false;
static pp_uint32 lButtonDownStartTime;

static bool rMouseDown = false;
static pp_uint32 rButtonDownStartTime;

static pp_uint32 timerTicker = 0;

static PPPoint p;

static bool exitDone;

pp_uint32 PPGetTickCount() {
	// @todo
	//return SDL_GetTicks();
	return 0;
}

void QueryKeyModifiers() {
	// @todo
	/*pp_uint32 mod = SDL_GetModState();

	if ((mod & KMOD_LSHIFT) || (mod & KMOD_RSHIFT))
		setKeyModifier(KeyModifierSHIFT);
	else
		clearKeyModifier(KeyModifierSHIFT);

	if ((mod & KMOD_LCTRL) || (mod & KMOD_RCTRL))
		setKeyModifier(KeyModifierCTRL);
	else
		clearKeyModifier(KeyModifierCTRL);

	if ((mod & KMOD_LALT) || (mod & KMOD_RALT))
		setKeyModifier(KeyModifierALT);
	else
		clearKeyModifier(KeyModifierALT);*/
}

static void RaiseEventSerialized(PPEvent* event) {
	if (myTrackerScreen && myTracker) {
		globalMutex->lock();
		myTrackerScreen->raiseEvent(event);
		globalMutex->unlock();
	}
}

/*
enum SDLUserEvents {
	SDLUserEventTimer,
	SDLUserEventLMouseRepeat,
	SDLUserEventRMouseRepeat,
#if defined(AMIGA_SAGA_PIP)
	SDLUserRefreshSAGAPiP
#endif
};

static Uint32 timerCallback(Uint32 interval) {
	timerMutex->lock();

	if (myTrackerScreen && myTracker && ticking) {
		SDL_UserEvent ev;
		ev.type = SDL_USEREVENT;

		if (!(timerTicker % 1)) {
			ev.code = SDLUserEventTimer;
			SDL_PushEvent((SDL_Event*) & ev);
		}

#if defined(AMIGA_SAGA_PIP)
		ev.code = SDLUserRefreshSAGAPiP;
		SDL_PushEvent((SDL_Event*) & ev);
#endif

		timerTicker++;

		if (lMouseDown && (timerTicker - lButtonDownStartTime) > 25) {
			ev.code = SDLUserEventLMouseRepeat;
			ev.data1 = (void*) p.x;
			ev.data2 = (void*) p.y;
			SDL_PushEvent((SDL_Event*) & ev);
		}

		if (rMouseDown && (timerTicker - rButtonDownStartTime) > 25) {
			ev.code = SDLUserEventRMouseRepeat;
			ev.data1 = (void*) p.x;
			ev.data2 = (void*) p.y;
			SDL_PushEvent((SDL_Event*) & ev);
		}
	}

	timerMutex->unlock();

	return interval;
}*/

static void translateMouseDownEvent(pp_int32 mouseButton, pp_int32 localMouseX, pp_int32 localMouseY) {
	if (mouseButton > 2 || !mouseButton)
		return;

	// -----------------------------
	p.x = localMouseX;
	p.y = localMouseY;

	if (mouseButton == 1) {
		PPEvent myEvent(eLMouseDown, &p, sizeof (PPPoint));

		RaiseEventSerialized(&myEvent);

		lMouseDown = true;
		lButtonDownStartTime = timerTicker;

		if (!lClickCount) {
			lmyTime = PPGetTickCount();
			llastClickPosition.x = localMouseX;
			llastClickPosition.y = localMouseY;
		} else if (lClickCount == 2) {
			pp_uint32 deltat = PPGetTickCount() - lmyTime;

			if (deltat > 500) {
				lClickCount = 0;
				lmyTime = PPGetTickCount();
				llastClickPosition.x = localMouseX;
				llastClickPosition.y = localMouseY;
			}
		}

		lClickCount++;

	} else if (mouseButton == 2) {
		PPEvent myEvent(eRMouseDown, &p, sizeof (PPPoint));

		RaiseEventSerialized(&myEvent);

		rMouseDown = true;
		rButtonDownStartTime = timerTicker;

		if (!rClickCount) {
			rmyTime = PPGetTickCount();
			rlastClickPosition.x = localMouseX;
			rlastClickPosition.y = localMouseY;
		} else if (rClickCount == 2) {
			pp_uint32 deltat = PPGetTickCount() - rmyTime;

			if (deltat > 500) {
				rClickCount = 0;
				rmyTime = PPGetTickCount();
				rlastClickPosition.x = localMouseX;
				rlastClickPosition.y = localMouseY;
			}
		}

		rClickCount++;
	}
}

static void translateMouseUpEvent(pp_int32 mouseButton, pp_int32 localMouseX, pp_int32 localMouseY) {
	// @todo
	/*
	if (mouseButton == SDL_BUTTON_WHEELDOWN) {
		TMouseWheelEventParams mouseWheelParams;
		mouseWheelParams.pos.x = localMouseX;
		mouseWheelParams.pos.y = localMouseY;
		mouseWheelParams.deltaX = -1;
		mouseWheelParams.deltaY = -1;

		PPEvent myEvent(eMouseWheelMoved, &mouseWheelParams, sizeof (mouseWheelParams));
		RaiseEventSerialized(&myEvent);
	} else if (mouseButton == SDL_BUTTON_WHEELUP) {
		TMouseWheelEventParams mouseWheelParams;
		mouseWheelParams.pos.x = localMouseX;
		mouseWheelParams.pos.y = localMouseY;
		mouseWheelParams.deltaX = 1;
		mouseWheelParams.deltaY = 1;

		PPEvent myEvent(eMouseWheelMoved, &mouseWheelParams, sizeof (mouseWheelParams));
		RaiseEventSerialized(&myEvent);
	} else if (mouseButton > 2 || !mouseButton)
		return;
	*/

	// -----------------------------
	if (mouseButton == 1) {
		lClickCount++;

		if (lClickCount >= 4) {
			pp_uint32 deltat = PPGetTickCount() - lmyTime;

			if (deltat < 500) {
				p.x = localMouseX;
				p.y = localMouseY;
				if (abs(p.x - llastClickPosition.x) < 4 &&
						abs(p.y - llastClickPosition.y) < 4) {
					PPEvent myEvent(eLMouseDoubleClick, &p, sizeof (PPPoint));
					RaiseEventSerialized(&myEvent);
				}
			}

			lClickCount = 0;
		}

		p.x = localMouseX;
		p.y = localMouseY;
		PPEvent myEvent(eLMouseUp, &p, sizeof (PPPoint));
		RaiseEventSerialized(&myEvent);
		lMouseDown = false;
	} else if (mouseButton == 2) {
		rClickCount++;

		if (rClickCount >= 4) {
			pp_uint32 deltat = PPGetTickCount() - rmyTime;

			if (deltat < 500) {
				p.x = localMouseX;
				p.y = localMouseY;
				if (abs(p.x - rlastClickPosition.x) < 4 &&
						abs(p.y - rlastClickPosition.y) < 4) {
					PPEvent myEvent(eRMouseDoubleClick, &p, sizeof (PPPoint));
					RaiseEventSerialized(&myEvent);
				}
			}

			rClickCount = 0;
		}

		p.x = localMouseX;
		p.y = localMouseY;
		PPEvent myEvent(eRMouseUp, &p, sizeof (PPPoint));
		RaiseEventSerialized(&myEvent);
		rMouseDown = false;
	}
}

static void translateMouseMoveEvent(pp_int32 mouseButton, pp_int32 localMouseX, pp_int32 localMouseY) {
	if (mouseButton == 0) {
		p.x = localMouseX;
		p.y = localMouseY;
		PPEvent myEvent(eMouseMoved, &p, sizeof (PPPoint));
		RaiseEventSerialized(&myEvent);
	} else {
		if (mouseButton > 2 || !mouseButton)
			return;

		p.x = localMouseX;
		p.y = localMouseY;
		if (mouseButton == 1 && lMouseDown) {
			PPEvent myEvent(eLMouseDrag, &p, sizeof (PPPoint));
			RaiseEventSerialized(&myEvent);
		} else if (rMouseDown) {
			PPEvent myEvent(eRMouseDrag, &p, sizeof (PPPoint));
			RaiseEventSerialized(&myEvent);
		}
	}
}

/*static void translateKeyDownEvent(const SDL_Event& event) {
	SDL_keysym keysym = event.key.keysym;

	// ALT+RETURN = Fullscreen toggle
	if (keysym.sym == SDLK_RETURN && (keysym.mod & KMOD_LALT)) {
		PPEvent myEvent(eFullScreen);
		RaiseEventSerialized(&myEvent);
		return;
	}

	pp_uint16 character = event.key.keysym.unicode;

	pp_uint16 chr[3] = {toVK(keysym), toSC(keysym), character};

#ifndef NOT_PC_KB
	// Hack for azerty keyboards (num keys are shifted, so we use the scancodes)
	if (stdKb) {
		if (chr[1] >= 2 && chr[1] <= 10)
			chr[0] = chr[1] + 47; // 1-9
		else if (chr[1] == 11)
			chr[0] = 48; // 0
	}
#endif

	PPEvent myEvent(eKeyDown, &chr, sizeof (chr));
	RaiseEventSerialized(&myEvent);

	if (character == 127) character = VK_BACK;

	if (character >= 32 && character <= 127) {
		PPEvent myEvent2(eKeyChar, &character, sizeof (character));
		RaiseEventSerialized(&myEvent2);
	}
}*/

/*static void translateKeyUpEvent(const SDL_Event& event) {
	SDL_keysym keysym = event.key.keysym;

	pp_uint16 character = event.key.keysym.unicode;

	pp_uint16 chr[3] = {toVK(keysym), toSC(keysym), character};

#ifndef NOT_PC_KB
	if (stdKb) {
		if (chr[1] >= 2 && chr[1] <= 10)
			chr[0] = chr[1] + 47;
		else if (chr[1] == 11)
			chr[0] = 48;
	}
#endif

	PPEvent myEvent(eKeyUp, &chr, sizeof (chr));
	RaiseEventSerialized(&myEvent);
}*/

/*void processSDLEvents(const SDL_Event& event) {
	pp_uint32 mouseButton = 0;

	switch (event.type) {
		case SDL_MOUSEBUTTONDOWN:
			mouseButton = event.button.button;
			if (mouseButton > 1 && mouseButton <= 3)
				mouseButton = 2;
			translateMouseDownEvent(mouseButton, event.button.x, event.button.y);
			break;

		case SDL_MOUSEBUTTONUP:
			mouseButton = event.button.button;
			if (mouseButton > 1 && mouseButton <= 3)
				mouseButton = 2;
			translateMouseUpEvent(mouseButton, event.button.x, event.button.y);
			break;

		case SDL_MOUSEMOTION:
			translateMouseMoveEvent(event.button.button, event.motion.x, event.motion.y);
			break;

		case SDL_KEYDOWN:
			translateKeyDownEvent(event);
			break;

		case SDL_KEYUP:
			translateKeyUpEvent(event);
			break;
	}
}*/

/*void processSDLUserEvents(const SDL_UserEvent& event) {

	union {
		void *ptr;
		pp_int32 i32;
	} data1, data2;
	data1.ptr = event.data1;
	data2.ptr = event.data2;

	switch (event.code) {
		case SDLUserEventTimer:
		{
			PPEvent myEvent(eTimer);
			RaiseEventSerialized(&myEvent);
			break;
		}

#if defined(AMIGA_SAGA_PIP)
		case SDLUserRefreshSAGAPiP:
		{
			myDisplayDevice->setSAGAPiPSize();
			break;
		}
#endif

		case SDLUserEventLMouseRepeat:
		{
			PPPoint p;
			p.x = data1.i32;
			p.y = data2.i32;
			PPEvent myEvent(eLMouseRepeat, &p, sizeof (PPPoint));
			RaiseEventSerialized(&myEvent);
			break;
		}

		case SDLUserEventRMouseRepeat:
		{
			PPPoint p;
			p.x = data1.i32;
			p.y = data2.i32;
			PPEvent myEvent(eRMouseRepeat, &p, sizeof (PPPoint));
			RaiseEventSerialized(&myEvent);
			break;
		}

	}
}*/

static void initTracker(pp_uint32 bpp, bool fullScreen, bool noSplash) {

	//SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	//SDL_EnableUNICODE(1);

	//SDL_WM_SetCaption("Loading MilkyTracker...", "MilkyTracker");

	// ------------ initialise tracker ---------------
	myTracker = new Tracker();

	PPSize windowSize = myTracker->getWindowSizeFromDatabase();
	if (!fullScreen)
		fullScreen = myTracker->getFullScreenFlagFromDatabase();
	pp_int32 scaleFactor = myTracker->getScreenScaleFactorFromDatabase();

#ifdef __LOWRES__
	windowSize.width = DISPLAYDEVICE_WIDTH;
	windowSize.height = DISPLAYDEVICE_HEIGHT;
#endif

	myDisplayDevice = new DisplayDevice_Amiga(windowSize.width, windowSize.height, scaleFactor, bpp, fullScreen);
	myDisplayDevice->init();

	myTrackerScreen = new PPScreen(myDisplayDevice, myTracker);
	myTracker->setScreen(myTrackerScreen);

	// Startup procedure
	myTracker->startUp(noSplash);

	// try to create timer
	// @todo
	//SDL_SetTimer(20, timerCallback);

	timerMutex->lock();
	ticking = true;
	timerMutex->unlock();
}

/*
void exitSDLEventLoop(bool serializedEventInvoked) {
	PPEvent event(eAppQuit);
	RaiseEventSerialized(&event);

	// it's necessary to make this mutex lock because the SDL modal event loop
	// used in the modal dialogs expects modal dialogs to be invoked by
	// events within these mutex lock calls
	if (!serializedEventInvoked)
		globalMutex->lock();

	bool res = myTracker->shutDown();

	if (!serializedEventInvoked)
		globalMutex->unlock();

	if (res)
		exitDone = 1;
}*/

static void SendFile(char *file) {
	PPSystemString finalFile(file);
	PPSystemString* strPtr = &finalFile;

	PPEvent event(eFileDragDropped, &strPtr, sizeof (PPSystemString*));
	RaiseEventSerialized(&event);
}

int main(int argc, char *argv[])
{
	// @todo replace by __AMIGA_CLASSIC__ maybe?
#if !defined(__amigaos4__) && !defined(MORPHOS) && !defined(WARPOS) && defined(__AMIGA__)
	// find out what type of CPU we have
	if ((SysBase->AttnFlags & AFF_68080) != 0)
		cpuType = 68080;
	else if ((SysBase->AttnFlags & AFF_68060) != 0)
		cpuType = 68060;
	else if ((SysBase->AttnFlags & AFF_68040) != 0)
		cpuType = 68040;
	else if ((SysBase->AttnFlags & AFF_68030) != 0)
		cpuType = 68030;
	else if ((SysBase->AttnFlags & AFF_68020) != 0)
		cpuType = 68020;
	else if ((SysBase->AttnFlags & AFF_68010) != 0)
		cpuType = 68010;
	else
		cpuType = 68000;

	if ((SysBase->AttnFlags & AFF_FPU40) != 0)
		hasFPU = 1;

	printf("Your CPU is a %i. Has FPU? %s\n", cpuType, hasFPU ? "Yes" : "No");
	if (!hasFPU) {
		fprintf(stderr, "Sorry, you need minimum a 68040 processor with FPU to run this application!\n");
		exit(1);
	}
#endif

	//SDL_Event event;
	char *loadFile = 0;

	pp_int32 defaultBPP = -1;
	bool fullScreen = false, noSplash = false;

	// Parse command line
	while (argc > 1) {
		--argc;
		if (strcmp(argv[argc - 1], "-bpp") == 0) {
			defaultBPP = atoi(argv[argc]);
			--argc;
		} else if (strcmp(argv[argc], "-nosplash") == 0) {
			noSplash = true;
		} else if (strcmp(argv[argc], "-fullscreen") == 0) {
			fullScreen = true;
		} else {
			if (argv[argc][0] == '-') {
				fprintf(stderr, "Usage: %s [-bpp N] [-fullscreen] [-nosplash] [-recvelocity]\n", argv[0]);
				exit(1);
			} else {
				loadFile = argv[argc];
			}
		}
	}

	/* Initialize SDL */
	/*if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}*/

	timerMutex = new PPMutex();
	globalMutex = new PPMutex();

	// Store current working path (init routine is likely to change it)
	PPPath_POSIX path;
	PPSystemString oldCwd = path.getCurrent();

	globalMutex->lock();

	initTracker(defaultBPP, fullScreen, noSplash);

	globalMutex->unlock();

	if (loadFile) {
		PPSystemString newCwd = path.getCurrent();
		path.change(oldCwd);
		SendFile(loadFile);
		path.change(newCwd);
		pp_uint16 chr[3] = {VK_RETURN, 0, 0};
		PPEvent event(eKeyDown, &chr, sizeof (chr));
		RaiseEventSerialized(&event);
	}

	/* Main event loop */
	/*exitDone = 0;
	while (!exitDone && SDL_WaitEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				exitSDLEventLoop(false);
				break;
			case SDL_MOUSEMOTION:
			{
				// ignore old mouse motion events in the event queue
				SDL_Event new_event;

				if (SDL_PeepEvents(&new_event, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_MOUSEMOTION)) > 0) {
					while (SDL_PeepEvents(&new_event, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_MOUSEMOTION)) > 0);
					processSDLEvents(new_event);
				} else {
					processSDLEvents(event);
				}
				break;
			}

			case SDL_USEREVENT:
				processSDLUserEvents((const SDL_UserEvent&) event);
				break;

			default:
				processSDLEvents(event);
				break;
		}
	}*/

	timerMutex->lock();
	ticking = false;
	timerMutex->unlock();

	//SDL_SetTimer(0, NULL);

	timerMutex->lock();
	globalMutex->lock();

	delete myTracker;
	myTracker = NULL;

	delete myTrackerScreen;
	myTrackerScreen = NULL;

	delete myDisplayDevice;

	globalMutex->unlock();
	timerMutex->unlock();

	//SDL_Quit();

	delete globalMutex;
	delete timerMutex;

	return 0;
}
