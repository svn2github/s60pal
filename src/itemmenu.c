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

static int     g_iNumInventory = 0;
static WORD    g_wItemFlags = 0;

WORD
PAL_ItemSelectMenuUpdate(
   VOID
)
/*++
  Purpose:

    Initialize the item selection menu.

  Parameters:

    None.

  Return value:

    The object ID of the selected item. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
   int                i, j, k;
   WORD               wObject;
   BYTE               bColor;
   static BYTE        bufImage[2048];
   static WORD        wPrevImageIndex = 0xFFFF;
   LoginInfo("PAL_ItemSelectMenuUpdate\n" );
   if (gpGlobals->iCurInvMenuItem < 0)
   {
      gpGlobals->iCurInvMenuItem = 0;
   }
   else if (gpGlobals->iCurInvMenuItem >= g_iNumInventory)
   {
      gpGlobals->iCurInvMenuItem = g_iNumInventory - 1;
   }

   //
   // Redraw the box
   //
   PAL_CreateBox(PAL_XY(2, 0), 6, 17, 1, FALSE);

   //
   // Draw the texts in the current page
   //
   i = gpGlobals->iCurInvMenuItem / 3 * 3 - 3 * 4;
   if (i < 0)
   {
      i = 0;
   }

   for (j = 0; j < 7; j++)
   {
      for (k = 0; k < 3; k++)
      {
         wObject = gpGlobals->rgInventory[i].wItem;
         bColor = MENUITEM_COLOR;

         if (i >= MAX_INVENTORY || wObject == 0)
         {
            //
            // End of the list reached
            //
            j = 7;
            break;
         }

         if (i == gpGlobals->iCurInvMenuItem)
         {
            if (!(gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) ||
               gpGlobals->rgInventory[i].nAmount <= gpGlobals->rgInventory[i].nAmountInUse)
            {
               //
               // This item is not selectable
               //
               bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
            }
            else
            {
               //
               // This item is selectable
               //
               bColor = MENUITEM_COLOR_SELECTED;
            }
         }
         else if (!(gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) ||
            gpGlobals->rgInventory[i].nAmount <= gpGlobals->rgInventory[i].nAmountInUse)
         {
            //
            // This item is not selectable
            //
            bColor = MENUITEM_COLOR_INACTIVE;
         }

         //
         // Draw the text
         //
         PAL_DrawText(PAL_GetWord(wObject), PAL_XY(15 + k * 100, 12 + j * 18),
            bColor, TRUE, FALSE);

         //
         // Draw the cursor on the current selected item
         //
         if (i == gpGlobals->iCurInvMenuItem)
         {
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
               gpScreen, PAL_XY(40 + k * 100, 22 + j * 18));
         }

         //
         // Draw the amount of this item
         //
		 if (gpGlobals->rgInventory[i].nAmount - gpGlobals->rgInventory[i].nAmountInUse > 1)
		 {
            PAL_DrawNumber(gpGlobals->rgInventory[i].nAmount - gpGlobals->rgInventory[i].nAmountInUse,
               2, PAL_XY(96 + k * 100, 17 + j * 18), kNumColorCyan, kNumAlignRight);
		 }

         i++;
      }
   }

   //
   // Draw the picture of current selected item
   //
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
      PAL_XY(5, 140));

   wObject = gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem;

   if (gpGlobals->g.rgObject[wObject].item.wBitmap != wPrevImageIndex)
   {
      if (PAL_MKFReadChunk(bufImage, 2048,
         gpGlobals->g.rgObject[wObject].item.wBitmap, gpGlobals->f.fpBALL) > 0)
      {
         wPrevImageIndex = gpGlobals->g.rgObject[wObject].item.wBitmap;
      }
      else
      {
         wPrevImageIndex = 0xFFFF;
      }
   }

   if (wPrevImageIndex != 0xFFFF)
   {
      PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(12, 148));
   }

   //
   // Process input
   //
   if (g_InputState.dwKeyPress & kKeyUp)
   {
      gpGlobals->iCurInvMenuItem -= 3;
   }
   else if (g_InputState.dwKeyPress & kKeyDown)
   {
      gpGlobals->iCurInvMenuItem += 3;
   }
   else if (g_InputState.dwKeyPress & kKeyLeft)
   {
      gpGlobals->iCurInvMenuItem--;
   }
   else if (g_InputState.dwKeyPress & kKeyRight)
   {
      gpGlobals->iCurInvMenuItem++;
   }
   else if (g_InputState.dwKeyPress & kKeyPgUp)
   {
      gpGlobals->iCurInvMenuItem -= 3 * 7;
   }
   else if (g_InputState.dwKeyPress & kKeyPgDn)
   {
      gpGlobals->iCurInvMenuItem += 3 * 7;
   }
   else if (g_InputState.dwKeyPress & kKeyMenu)
   {
      return 0;
   }
   else if (g_InputState.dwKeyPress & kKeySearch)
   {
      if ((gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) &&
         gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmount >
         gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmountInUse)
      {
         j = (gpGlobals->iCurInvMenuItem < 3 * 4) ? (gpGlobals->iCurInvMenuItem / 3) : 4;
         k = gpGlobals->iCurInvMenuItem % 3;

         PAL_DrawText(PAL_GetWord(wObject), PAL_XY(15 + k * 100, 12 + j * 18),
            MENUITEM_COLOR_CONFIRMED, FALSE, FALSE);

         return wObject;
      }
   }

   return 0xFFFF;
}

VOID
PAL_ItemSelectMenuInit(
   WORD                      wItemFlags
)
/*++
  Purpose:

    Initialize the item selection menu.

  Parameters:

    [IN]  wItemFlags - flags for usable item.

  Return value:

    None.

--*/
{
	LoginInfo("PAL_ItemSelectMenuInit\n" );
	g_wItemFlags = wItemFlags;

   //
   // Compress the inventory
   //
   PAL_CompressInventory();

   //
   // Count the total number of items in inventory
   //
   g_iNumInventory = 0;
   while (g_iNumInventory < MAX_INVENTORY &&
      gpGlobals->rgInventory[g_iNumInventory].wItem != 0)
   {
      g_iNumInventory++;
   }
}

WORD
PAL_ItemSelectMenu(
   LPITEMCHANGED_CALLBACK    lpfnMenuItemChanged,
   WORD                      wItemFlags
)
/*++
  Purpose:

    Show the item selection menu.

  Parameters:

    [IN]  lpfnMenuItemChanged - Callback function which is called when user
                                changed the current menu item.

    [IN]  wItemFlags - flags for usable item.

  Return value:

    The object ID of the selected item. 0 if cancelled.

--*/
{
   int        iPrevIndex;
   WORD       w;
   DWORD      dwTime;
   LoginInfo("PAL_ItemSelectMenu\n" );
   PAL_ItemSelectMenuInit(wItemFlags);
   iPrevIndex = gpGlobals->iCurInvMenuItem;

   PAL_ClearKeyState();

   if (lpfnMenuItemChanged != NULL)
   {
      (*lpfnMenuItemChanged)(gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem);
   }

   dwTime = SDL_GetTicks();

   while (TRUE)
   {
      w = PAL_ItemSelectMenuUpdate();
      VIDEO_UpdateScreen(NULL);

      PAL_ClearKeyState();

      while (SDL_GetTicks() < dwTime)
      {
         UTIL_Delay(50);
         if (g_InputState.dwKeyPress != 0)
         {
			 break;
         }
      }

      dwTime = SDL_GetTicks() + FRAME_TIME;

      if (w != 0xFFFF)
      {
         return w;
      }

      if (iPrevIndex != gpGlobals->iCurInvMenuItem)
      {
         if (gpGlobals->iCurInvMenuItem >= 0 && gpGlobals->iCurInvMenuItem < MAX_INVENTORY)
         {
            if (lpfnMenuItemChanged != NULL)
            {
               (*lpfnMenuItemChanged)(gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem);
            }
         }

         iPrevIndex = gpGlobals->iCurInvMenuItem;
      }
   }

   return 0; // should not really reach here
}
