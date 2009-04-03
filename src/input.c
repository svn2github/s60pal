//
// Copyright (c) 2009, Wei Mingzhi <whistler@openoffice.org>.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "main.h"
extern SDL_Surface *gpScreen;
PALINPUTSTATE g_InputState;
static SDL_Joystick *g_pJoy = NULL;

static VOID PAL_KeyboardEventFilter(const SDL_Event *lpEvent)
/*++
 Purpose:

 Handle keyboard events.

 Parameters:

 [IN]  lpEvent - pointer to the event.

 Return value:

 None.

 --*/
	{

	switch (lpEvent->type)
		{
		case SDL_KEYDOWN:
			//
			// Pressed a key
			//
			if (lpEvent->key.keysym.mod & KMOD_ALT)
				{
				if (lpEvent->key.keysym.sym == SDLK_RETURN)
					{
					//
					// Pressed Alt+Enter (toggle fullscreen)...
					//
					VIDEO_ToggleFullscreen();
					return;
					}
				else if (lpEvent->key.keysym.sym == SDLK_F4)
					{
					//
					// Pressed Alt+F4 (Exit program)...
					//
					PAL_Shutdown();
					exit(0);
					}
				}

			switch (lpEvent->key.keysym.sym)
				{

				case SDLK_0:
					VIDEO_ToggleFullscreen();
					break;
				case SDLK_1:
					ChangeVolume(0);
					break;
				case SDLK_3:
					ChangeVolume(1);
					break;
				case SDLK_UP:
				case SDLK_KP8:
					g_InputState.prevdir = g_InputState.dir;
					g_InputState.dir = kDirNorth;
					g_InputState.dwKeyPress |= kKeyUp;
					break;

				case SDLK_DOWN:
				case SDLK_KP2:
					g_InputState.prevdir = g_InputState.dir;
					g_InputState.dir = kDirSouth;
					g_InputState.dwKeyPress |= kKeyDown;
					break;

				case SDLK_LEFT:
				case SDLK_KP4:
					g_InputState.prevdir = g_InputState.dir;
					g_InputState.dir = kDirWest;
					g_InputState.dwKeyPress |= kKeyLeft;
					break;

				case SDLK_RIGHT:
				case SDLK_KP6:
					g_InputState.prevdir = g_InputState.dir;
					g_InputState.dir = kDirEast;
					g_InputState.dwKeyPress |= kKeyRight;
					break;

				case SDLK_ESCAPE:
				case SDLK_INSERT:
				case SDLK_LALT:
				case SDLK_RALT:
				case SDLK_KP0:
					g_InputState.dwKeyPress |= kKeyMenu;
					break;

				case SDLK_RETURN:
				case SDLK_SPACE:
				case SDLK_KP_ENTER:
					g_InputState.dwKeyPress |= kKeySearch;
					break;

				case SDLK_PAGEUP:
				case SDLK_KP9:
					g_InputState.dwKeyPress |= kKeyPgUp;
					break;

				case SDLK_PAGEDOWN:
				case SDLK_KP3:
					g_InputState.dwKeyPress |= kKeyPgDn;
					break;

				case SDLK_7:
				case SDLK_r:
					g_InputState.dwKeyPress |= kKeyRepeat;
					break;

				case SDLK_2:
				case SDLK_a:
					g_InputState.dwKeyPress |= kKeyAuto;
					break;

				case SDLK_d:
					g_InputState.dwKeyPress |= kKeyDefend;
					break;

				case SDLK_e:
					g_InputState.dwKeyPress |= kKeyUseItem;
					break;

				case SDLK_w:
					g_InputState.dwKeyPress |= kKeyThrowItem;
					break;

				case SDLK_q:
					g_InputState.dwKeyPress |= kKeyFlee;
					break;

				case SDLK_s:
					g_InputState.dwKeyPress |= kKeyStatus;
					break;

				case SDLK_f:
					g_InputState.dwKeyPress |= kKeyForce;
					break;

				case SDLK_p:
				case SDLK_HASH:
					VIDEO_SaveScreenshot();
					break;
				}
			break;

		case SDL_KEYUP:
			//
			// Released a key
			//
			switch (lpEvent->key.keysym.sym)
				{
				case SDLK_UP:
				case SDLK_KP8:
					if (g_InputState.dir == kDirNorth)
						{
						g_InputState.dir = g_InputState.prevdir;
						}
					g_InputState.prevdir = kDirUnknown;
					break;

				case SDLK_DOWN:
				case SDLK_KP2:
					if (g_InputState.dir == kDirSouth)
						{
						g_InputState.dir = g_InputState.prevdir;
						}
					g_InputState.prevdir = kDirUnknown;
					break;

				case SDLK_LEFT:
				case SDLK_KP4:
					if (g_InputState.dir == kDirWest)
						{
						g_InputState.dir = g_InputState.prevdir;
						}
					g_InputState.prevdir = kDirUnknown;
					break;

				case SDLK_RIGHT:
				case SDLK_KP6:
					if (g_InputState.dir == kDirEast)
						{
						g_InputState.dir = g_InputState.prevdir;
						}
					g_InputState.prevdir = kDirUnknown;
					break;
				}
			break;
		}
	}

static VOID PAL_MouseEventFilter(const SDL_Event *lpEvent)
/*++
 Purpose:

 Handle mouse events.

 Parameters:

 [IN]  lpEvent - pointer to the event.

 Return value:

 None.

 --*/
	{
	
	const SDL_VideoInfo *vi;
	
	int gridWidth;
	int gridHeight; 
	int state = 0;
	int thumbx;
	int thumby;
	INT gridIndex;
	static INT lastReleaseButtonTime, lastPressButtonTime;
	static INT lastPressx = 0;
	static INT lastPressy = 0;
	static INT lastReleasex = 0;
	static INT lastReleasey = 0;
	if(lpEvent->type!= SDL_MOUSEBUTTONDOWN && lpEvent->type != SDL_MOUSEBUTTONUP)
			return;
	vi = SDL_GetVideoInfo();
	gridWidth = vi->current_w / 3;
	gridHeight = vi->current_h / 3;
	thumbx = ceil(lpEvent->button.x / gridWidth);
	thumby = floor(lpEvent->button.y / gridHeight);
	gridIndex = thumbx + thumby * 3; 
	switch (lpEvent->type)
		{

		case SDL_MOUSEBUTTONDOWN:
			lastPressButtonTime = SDL_GetTicks();
			lastPressx = lpEvent->button.x;
			lastPressy = lpEvent->button.y;
			switch (gridIndex)
				{
				case 2:
					g_InputState.prevdir = g_InputState.dir;
					g_InputState.dir = kDirNorth;
					break;
				case 6:
					g_InputState.prevdir = g_InputState.dir;
					g_InputState.dir = kDirSouth;
					break;
				case 0:
					g_InputState.prevdir = g_InputState.dir;
					g_InputState.dir = kDirWest;
					break;
				case 8:
					g_InputState.prevdir = g_InputState.dir;
					g_InputState.dir = kDirEast;
					break;
				case 1:
					g_InputState.dwKeyPress |= kKeyUp;
					break;
				case 7:
					g_InputState.dwKeyPress |= kKeyDown;
					break;
				case 3:
					g_InputState.dwKeyPress |= kKeyLeft;
					break;
				case 5:
					g_InputState.dwKeyPress |= kKeyRight;
					break;
				}

			break;
		case SDL_MOUSEBUTTONUP:
			//
			// Pressed the joystick button
			//
			lastReleaseButtonTime = SDL_GetTicks();
			lastReleasex = lpEvent->button.x;
			lastReleasey = lpEvent->button.y;
			switch (gridIndex)
				{
				case 2:
				case 6:
				case 0:
				case 8:
//					if (g_InputState.dir == kDirNorth)
//						{
//						g_InputState.dir = g_InputState.prevdir;
//						}
					g_InputState.dir  = kDirUnknown;
					g_InputState.prevdir = kDirUnknown;
					break;

				}
			if (gridIndex == 4 && abs(lastPressx - lastReleasex) < 5 && abs(
					lastPressy - lastReleasey) < 5)
				{
				if (lastReleaseButtonTime - lastPressButtonTime < 1000)
					{
					g_InputState.dwKeyPress |= kKeySearch;
					}
				else
					{
					g_InputState.dwKeyPress |= kKeyMenu;
					}
				}

			break;
		}

	}
static VOID PAL_JoystickEventFilter(const SDL_Event *lpEvent)
/*++
 Purpose:

 Handle joystick events.

 Parameters:

 [IN]  lpEvent - pointer to the event.

 Return value:

 None.

 --*/
	{
	switch (lpEvent->type)
		{
		case SDL_JOYAXISMOTION:
			//
			// Moved an axis on joystick
			//
			switch (lpEvent->jaxis.axis)
				{
				case 0:
					//
					// X axis
					//
					if (lpEvent->jaxis.value > 20000)
						{
						if (g_InputState.dir != kDirEast)
							{
							g_InputState.dwKeyPress |= kKeyRight;
							}
						g_InputState.prevdir = g_InputState.dir;
						g_InputState.dir = kDirEast;
						}
					else if (lpEvent->jaxis.value < -20000)
						{
						if (g_InputState.dir != kDirWest)
							{
							g_InputState.dwKeyPress |= kKeyLeft;
							}
						g_InputState.prevdir = g_InputState.dir;
						g_InputState.dir = kDirWest;
						}
					else
						{
						if (g_InputState.prevdir != kDirEast
								&& g_InputState.prevdir != kDirWest)
							{
							g_InputState.dir = g_InputState.prevdir;
							}
						g_InputState.prevdir = kDirUnknown;
						}
					break;

				case 1:
					//
					// Y axis
					//
					if (lpEvent->jaxis.value > 20000)
						{
						if (g_InputState.dir != kDirSouth)
							{
							g_InputState.dwKeyPress |= kKeyDown;
							}
						g_InputState.prevdir = g_InputState.dir;
						g_InputState.dir = kDirSouth;
						}
					else if (lpEvent->jaxis.value < -20000)
						{
						if (g_InputState.dir != kDirNorth)
							{
							g_InputState.dwKeyPress |= kKeyUp;
							}
						g_InputState.prevdir = g_InputState.dir;
						g_InputState.dir = kDirNorth;
						}
					else
						{
						if (g_InputState.prevdir != kDirNorth
								&& g_InputState.prevdir != kDirSouth)
							{
							g_InputState.dir = g_InputState.prevdir;
							}
						g_InputState.prevdir = kDirUnknown;
						}
					break;
				}
			break;

		case SDL_JOYBUTTONDOWN:
			//
			// Pressed the joystick button
			//
			switch (lpEvent->jbutton.button & 1)
				{
				case 0:
					g_InputState.dwKeyPress |= kKeySearch;
					break;

				case 1:
					g_InputState.dwKeyPress |= kKeyMenu;
					break;
				}
			break;
		}
	}

static int SDLCALL
PAL_EventFilter(const SDL_Event *lpEvent)
/*++
 Purpose:

 SDL event filter function. A filter to process all events.

 Parameters:

 [IN]  lpEvent - pointer to the event.

 Return value:

 1 = the event will be added to the internal queue.
 0 = the event will be dropped from the queue.

 --*/
	{
	switch (lpEvent->type)
		{
		case SDL_VIDEORESIZE:
			//
			// resized the window
			//
			VIDEO_Resize(lpEvent->resize.w, lpEvent->resize.h);
			break;

		case SDL_QUIT:
			//
			// clicked on the close button of the window. Quit immediately.
			//
			PAL_Shutdown();
			exit(0);
		}

	PAL_KeyboardEventFilter(lpEvent);
	PAL_JoystickEventFilter(lpEvent);
	PAL_MouseEventFilter(lpEvent);
	//
	// All events are handled here; don't put anything to the internal queue
	//
	return 0;
	}

VOID PAL_ClearKeyState(VOID
)
/*++
 Purpose:

 Clear the record of pressed keys.

 Parameters:

 None.

 Return value:

 None.

 --*/
	{
	g_InputState.dwKeyPress = 0;
	}

VOID PAL_InitInput(VOID
)
/*++
 Purpose:

 Initialize the input subsystem.

 Parameters:

 None.

 Return value:

 None.

 --*/
	{
	memset(&g_InputState, 0, sizeof(g_InputState));
	g_InputState.dir = kDirUnknown;
	g_InputState.prevdir = kDirUnknown;
	SDL_SetEventFilter(PAL_EventFilter);

	//
	// Check for joystick
	//
	/*if (SDL_NumJoysticks() > 0)
	 {
	 g_pJoy = SDL_JoystickOpen(0);
	 if (g_pJoy != NULL)
	 {
	 SDL_JoystickEventState(SDL_ENABLE);
	 }
	 }*/
	}

VOID PAL_ShutdownInput(VOID
)
/*++
 Purpose:

 Shutdown the input subsystem.

 Parameters:

 None.

 Return value:

 None.

 --*/
	{
	/*if (SDL_JoystickOpened(0))
	 {
	 assert(g_pJoy != NULL);
	 SDL_JoystickClose(g_pJoy);
	 g_pJoy = NULL;
	 }*/
	}

VOID PAL_ProcessEvent(VOID
)
/*++
 Purpose:

 Process all events.

 Parameters:

 None.

 Return value:

 None.

 --*/
	{
	while (SDL_PollEvent(NULL))
		;
	}
