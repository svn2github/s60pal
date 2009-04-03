//
// Copyright (c) 2009, Wei Mingzhi <whistler@openoffice.org>.
// All rights reserved.
//
// Based on PALx Project by palxex.
// Copyright (c) 2006-2008, Pal Lockheart <palxex@gmail.com>.
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

BOOL            g_fScriptSuccess = TRUE;
static int      g_iCurEquipPart = -1;

//#define SCRIPT_TRACE 1

static BOOL
PAL_NPCWalkTo(
   WORD           wEventObjectID,
   INT            x,
   INT            y,
   INT            h,
   INT            iSpeed
)
/*++
  Purpose:

    Make the specified event object walk to the map position specified by (x, y, h)
    at the speed of iSpeed.

  Parameters:

    [IN]  wEventObjectID - the event object to move.

    [IN]  x - Column number of the tile.

    [IN]  y - Line number in the map.

    [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
              (See map.h for details.)

    [IN]  iSpeed - the speed to move.

  Return value:

    TRUE if the event object has successfully moved to the specified position,
    FALSE if still need more moving.

--*/
{
   LPEVENTOBJECT    pEvtObj;
   int              xOffset, yOffset;

   pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);

   xOffset = (x * 32 + h * 16) - pEvtObj->x;
   yOffset = (y * 16 + h * 8) - pEvtObj->y;

   if (yOffset < 0)
   {
      pEvtObj->wDirection = ((xOffset < 0) ? kDirWest : kDirNorth);
   }
   else
   {
      pEvtObj->wDirection = ((xOffset < 0) ? kDirSouth: kDirEast);
   }

   if (abs(xOffset) < iSpeed * 2 || abs(yOffset) < iSpeed * 2)
   {
      pEvtObj->x = x * 32 + h * 16;
      pEvtObj->y = y * 16 + h * 8;
   }
   else
   {
      PAL_NPCWalkOneStep(wEventObjectID, iSpeed);
   }

   if (pEvtObj->x == x * 32 + h * 16 && pEvtObj->y == y * 16 + h * 8)
   {
      pEvtObj->wCurrentFrameNum = 0;
      return TRUE;
   }

   return FALSE;
}

static VOID
PAL_PartyWalkTo(
   INT            x,
   INT            y,
   INT            h,
   INT            iSpeed
)
/*++
  Purpose:

    Make the party walk to the map position specified by (x, y, h)
    at the speed of iSpeed.

  Parameters:

    [IN]  x - Column number of the tile.

    [IN]  y - Line number in the map.

    [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
              (See map.h for details.)

    [IN]  iSpeed - the speed to move.

  Return value:

    None.

--*/
{
   int           xOffset, yOffset, i, dx, dy;
   DWORD         t;

   xOffset = x * 32 + h * 16 - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset);
   yOffset = y * 16 + h * 8 - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset);

   t = 0;

   while (xOffset != 0 || yOffset != 0)
   {
      PAL_ProcessEvent();
      while (SDL_GetTicks() <= t)
      {
         PAL_ProcessEvent();
         SDL_Delay(1);
      }

      t = SDL_GetTicks() + FRAME_TIME;

      //
      // Store trail
      //
      for (i = 3; i >= 0; i--)
      {
         gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
      }
      gpGlobals->rgTrail[0].wDirection = gpGlobals->wPartyDirection;
      gpGlobals->rgTrail[0].x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
      gpGlobals->rgTrail[0].y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

      if (yOffset < 0)
      {
         gpGlobals->wPartyDirection = ((xOffset < 0) ? kDirWest : kDirNorth);
      }
      else
      {
         gpGlobals->wPartyDirection = ((xOffset < 0) ? kDirSouth: kDirEast);
      }

      dx = PAL_X(gpGlobals->viewport);
      dy = PAL_Y(gpGlobals->viewport);

      if (abs(xOffset) <= iSpeed * 2)
      {
         dx += xOffset;
      }
      else
      {
         dx += iSpeed * (xOffset < 0 ? -2 : 2);
      }

      if (abs(yOffset) <= iSpeed)
      {
         dy += yOffset;
      }
      else
      {
         dy += iSpeed * (yOffset < 0 ? -1 : 1);
      }

      //
      // Move the viewport
      //
      gpGlobals->viewport = PAL_XY(dx, dy);

      PAL_UpdatePartyGestures(TRUE);
      PAL_GameUpdate(FALSE);
      PAL_MakeScene();
      VIDEO_UpdateScreen(NULL);

      xOffset = x * 32 + h * 16 - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset);
      yOffset = y * 16 + h * 8 - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset);
   }

   PAL_UpdatePartyGestures(FALSE);
}

static VOID
PAL_PartyRideEventObject(
   WORD           wEventObjectID,
   INT            x,
   INT            y,
   INT            h,
   INT            iSpeed
)
/*++
  Purpose:

    Move the party to the specified position, riding the specified event object.

  Parameters:

    [IN]  wEventObjectID - the event object to be ridden.

    [IN]  x - Column number of the tile.

    [IN]  y - Line number in the map.

    [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
              (See map.h for details.)

    [IN]  iSpeed - the speed to move.

  Return value:

    TRUE if the party and event object has successfully moved to the specified
    position, FALSE if still need more moving.

--*/
{
   int              xOffset, yOffset, dx, dy, i;
   DWORD            t;
   LPEVENTOBJECT    p;

   p = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);

   xOffset = x * 32 + h * 16 - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset);
   yOffset = y * 16 + h * 8 - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset);

   t = 0;

   while (xOffset != 0 || yOffset != 0)
   {
      PAL_ProcessEvent();
      while (SDL_GetTicks() <= t)
      {
         PAL_ProcessEvent();
         SDL_Delay(1);
      }

      t = SDL_GetTicks() + FRAME_TIME;

      if (yOffset < 0)
      {
         gpGlobals->wPartyDirection = ((xOffset < 0) ? kDirWest : kDirNorth);
      }
      else
      {
         gpGlobals->wPartyDirection = ((xOffset < 0) ? kDirSouth: kDirEast);
      }

      if (abs(xOffset) > iSpeed * 2)
      {
         dx = iSpeed * (xOffset < 0 ? -2 : 2);
      }
      else
      {
         dx = xOffset;
      }

      if (abs(yOffset) > iSpeed)
      {
         dy = iSpeed * (yOffset < 0 ? -1 : 1);
      }
      else
      {
         dy = yOffset;
      }

      //
      // Store trail
      //
      for (i = 3; i >= 0; i--)
      {
         gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
      }

      gpGlobals->rgTrail[0].wDirection = gpGlobals->wPartyDirection;
      gpGlobals->rgTrail[0].x = PAL_X(gpGlobals->viewport) + dx + PAL_X(gpGlobals->partyoffset);
      gpGlobals->rgTrail[0].y = PAL_Y(gpGlobals->viewport) + dy + PAL_Y(gpGlobals->partyoffset);

      //
      // Move the viewport
      //
      gpGlobals->viewport =
         PAL_XY(PAL_X(gpGlobals->viewport) + dx, PAL_Y(gpGlobals->viewport) + dy);

      p->x += dx;
      p->y += dy;

      PAL_GameUpdate(FALSE);
      PAL_MakeScene();
      VIDEO_UpdateScreen(NULL);

      xOffset = x * 32 + h * 16 - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset);
      yOffset = y * 16 + h * 8 - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset);
   }
}

static WORD
PAL_InterpretInstruction(
   WORD           wScriptEntry,
   WORD           wEventObjectID
)
/*++
  Purpose:

    Interpret and execute one instruction in the script.

  Parameters:

    [IN]  wScriptEntry - The script entry to execute.

    [IN]  wEventObjectID - The event object ID which invoked the script.

  Return value:

    The address of the next script instruction to execute.

--*/
{
   LPEVENTOBJECT          pEvtObj, pCurrent;
   LPSCRIPTENTRY          pScript;
   int                    iPlayerRole, i, j, x, y;
   WORD                   w, wCurEventObjectID;

   pScript = &(gpGlobals->g.lprgScriptEntry[wScriptEntry]);

   if (wEventObjectID != 0)
   {
      pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);
   }
   else
   {
      pEvtObj = NULL;
   }

   if (pScript->rgwOperand[0] == 0 || pScript->rgwOperand[0] == 0xFFFF)
   {
      pCurrent = pEvtObj;
      wCurEventObjectID = wEventObjectID;
   }
   else
   {
      pCurrent = &(gpGlobals->g.lprgEventObject[pScript->rgwOperand[0] - 1]);
      wCurEventObjectID = pScript->rgwOperand[0];
   }

   if (pScript->rgwOperand[0] < MAX_PLAYABLE_PLAYER_ROLES)
   {
      iPlayerRole = gpGlobals->rgParty[pScript->rgwOperand[0]].wPlayerRole;
   }
   else
   {
      iPlayerRole = gpGlobals->rgParty[0].wPlayerRole;
   }

   switch (pScript->wOperation)
   {
   case 0x000B:
   case 0x000C:
   case 0x000D:
   case 0x000E:
      //
      // walk one step
      //
      pEvtObj->wDirection = pScript->wOperation - 0x000B;
      PAL_NPCWalkOneStep(wEventObjectID, 2);
      break;

   case 0x000F:
      //
      // Set the direction and/or gesture for event object
      //
      if (pScript->rgwOperand[0] != 0xFFFF)
      {
         pEvtObj->wDirection = pScript->rgwOperand[0];
      }
      if (pScript->rgwOperand[1] != 0xFFFF)
      {
         pEvtObj->wCurrentFrameNum = pScript->rgwOperand[1];
      }
      break;

   case 0x0010:
      //
      // Walk straight to the specified position
      //
      if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
         pScript->rgwOperand[2], 3))
      {
         wScriptEntry--;
      }
      break;

   case 0x0011:
      //
      // Walk straight to the specified position, at a lower speed
      //
      if ((wEventObjectID & 1) ^ (gpGlobals->dwFrameNum & 1))
      {
         if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
            pScript->rgwOperand[2], 2))
         {
            wScriptEntry--;
         }
      }
      else
      {
         wScriptEntry--;
      }
      break;

   case 0x0012:
      //
      // Set the position of the event object, relative to the party
      //
      pCurrent->x =
         pScript->rgwOperand[1] + PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
      pCurrent->y =
         pScript->rgwOperand[2] + PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);
      break;

   case 0x0013:
      //
      // Set the position of the event object
      //
      pCurrent->x = pScript->rgwOperand[1];
      pCurrent->y = pScript->rgwOperand[2];
      break;

   case 0x0014:
      //
      // Set the gesture of the event object
      //
      pEvtObj->wCurrentFrameNum = pScript->rgwOperand[0];
      pEvtObj->wDirection = kDirSouth;
      break;

   case 0x0015:
      //
      // Set the direction and gesture for a party member
      //
      gpGlobals->wPartyDirection = pScript->rgwOperand[0];
      gpGlobals->rgParty[pScript->rgwOperand[2]].wFrame =
         gpGlobals->wPartyDirection * 3 + pScript->rgwOperand[1];
      break;

   case 0x0016:
      //
      // Set the direction and gesture for an event object
      //
      if (pScript->rgwOperand[0] != 0)
      {
         pCurrent->wDirection = pScript->rgwOperand[1];
         pCurrent->wCurrentFrameNum = pScript->rgwOperand[2];
      }
      break;

   case 0x0017:
      //
      // set the player's extra attribute
      //
      {
         WORD *p;

         i = pScript->rgwOperand[0] - 0xB;

         p = (WORD *)(&gpGlobals->rgEquipmentEffect[i]); // HACKHACK

         p[pScript->rgwOperand[1] * MAX_PLAYER_ROLES + wEventObjectID] =
            (SHORT)pScript->rgwOperand[2];
      }
      break;

   case 0x0018:
      //
      // Equip the selected item
      //
      i = pScript->rgwOperand[0] - 0x0B;
      g_iCurEquipPart = i;

      //
      // The wEventObjectID parameter here should indicate the player role
      //
      PAL_RemoveEquipmentEffect(wEventObjectID, i);

      if (gpGlobals->g.PlayerRoles.rgwEquipment[i][wEventObjectID] != pScript->rgwOperand[1])
      {
         w = gpGlobals->g.PlayerRoles.rgwEquipment[i][wEventObjectID];
         gpGlobals->g.PlayerRoles.rgwEquipment[i][wEventObjectID] = pScript->rgwOperand[1];

         PAL_AddItemToInventory(pScript->rgwOperand[1], -1);

         if (w != 0)
         {
            PAL_AddItemToInventory(w, 1);
         }

         gpGlobals->wLastUnequippedItem = w;
      }
      break;

   case 0x0019:
      //
      // Increase/decrease the player's attribute
      //
      {
         WORD *p = (WORD *)(&gpGlobals->g.PlayerRoles); // HACKHACK

         if (pScript->rgwOperand[2] == 0)
         {
            iPlayerRole = wEventObjectID;
         }
         else
         {
            iPlayerRole = pScript->rgwOperand[2] - 1;
         }

         p[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] +=
            (SHORT)pScript->rgwOperand[1];
      }
      break;

   case 0x001A:
      //
      // Set player's attribute
      //
      if (pScript->rgwOperand[2] & 0x8000)
      {
         //
         // Set temporarily in battle (TODO)
         //
      }
      else
      {
         //
         // Set permanently
         //
         WORD *p = (WORD *)(&gpGlobals->g.PlayerRoles); // HACKHACK

         if (g_iCurEquipPart != -1)
         {
            //
            // In the progress of equipping items
            //
            p = (WORD *)&(gpGlobals->rgEquipmentEffect[g_iCurEquipPart]);
         }

         if (pScript->rgwOperand[2] == 0)
         {
            //
            // Apply to the current player. The wEventObjectID should
            // indicate the player role.
            //
            iPlayerRole = wEventObjectID;
         }
         else
         {
            iPlayerRole = pScript->rgwOperand[2] - 1;
         }

         p[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] =
            (SHORT)pScript->rgwOperand[1];
      }
      break;

   case 0x001B:
      //
      // Increase/decrease player's HP
      //
      if (pScript->rgwOperand[0])
      {
         //
         // Apply to everyone
         //
         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            w = gpGlobals->rgParty[i].wPlayerRole;
            PAL_IncreaseHPMP(w, (SHORT)(pScript->rgwOperand[1]), 0);
         }
      }
      else
      {
         //
         // Apply to one player. The wEventObjectID parameter should indicate the player role.
         //
         if (!PAL_IncreaseHPMP(wEventObjectID, (SHORT)(pScript->rgwOperand[1]), 0))
         {
            g_fScriptSuccess = FALSE;
         }
      }
      break;

   case 0x001C:
      //
      // Increase/decrease player's MP
      //
      if (pScript->rgwOperand[0])
      {
         //
         // Apply to everyone
         //
         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            w = gpGlobals->rgParty[i].wPlayerRole;
            PAL_IncreaseHPMP(w, 0, (SHORT)(pScript->rgwOperand[1]));
         }
      }
      else
      {
         //
         // Apply to one player. The wEventObjectID parameter should indicate the player role.
         //
         if (!PAL_IncreaseHPMP(wEventObjectID, 0, (SHORT)(pScript->rgwOperand[1])))
         {
            g_fScriptSuccess = FALSE;
         }
      }
      break;

   case 0x001D:
      //
      // Increase/decrease player's HP and MP
      //
      if (pScript->rgwOperand[0])
      {
         //
         // Apply to everyone
         //
         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            w = gpGlobals->rgParty[i].wPlayerRole;
            PAL_IncreaseHPMP(w,
               (SHORT)(pScript->rgwOperand[1]), (SHORT)(pScript->rgwOperand[1]));
         }
      }
      else
      {
         //
         // Apply to one player. The wEventObjectID parameter should indicate the player role.
         //
         if (!PAL_IncreaseHPMP(wEventObjectID,
            (SHORT)(pScript->rgwOperand[1]), (SHORT)(pScript->rgwOperand[1])))
         {
            g_fScriptSuccess = FALSE;
         }
      }
      break;

   case 0x001E:
      //
      // Increase or decrease cash by the specified amount
      //
      if ((SHORT)(pScript->rgwOperand[0]) < 0 &&
         gpGlobals->dwCash < (WORD)(-(SHORT)(pScript->rgwOperand[0])))
      {
         //
         // not enough cash
         //
         wScriptEntry = pScript->rgwOperand[1] - 1;
      }
      else
      {
         gpGlobals->dwCash += (SHORT)(pScript->rgwOperand[0]);
      }
      break;

   case 0x001F:
      //
      // Add item to inventory
      //
      PAL_AddItemToInventory(pScript->rgwOperand[0], (SHORT)(pScript->rgwOperand[1]));
      break;

   case 0x0020:
      //
      // Remove item from inventory
      //
      if (!PAL_AddItemToInventory(pScript->rgwOperand[0],
         -((pScript->rgwOperand[1] == 0) ? 1 : pScript->rgwOperand[1])))
      {
         //
         // Try removing equipped item
         //
         x = pScript->rgwOperand[1];
         if (x == 0)
         {
            x = 1;
         }

         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            w = gpGlobals->rgParty[i].wPlayerRole;

            for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
            {
               if (gpGlobals->g.PlayerRoles.rgwEquipment[j][w] == pScript->rgwOperand[0])
               {
                  PAL_RemoveEquipmentEffect(w, j);
                  gpGlobals->g.PlayerRoles.rgwEquipment[j][w] = 0;

                  if (--x == 0)
                  {
                     i = 9999;
                     break;
                  }
               }
            }
         }

         if (x > 0 && pScript->rgwOperand[2] != 0)
         {
            wScriptEntry = pScript->rgwOperand[2] - 1;
         }
      }
      break;

   case 0x0021:
      //
      // Inflict damage to the enemy
      //
      if (pScript->rgwOperand[0])
      {
         //
         // Inflict damage to all enemies
         //
         for (i = 0;i <= g_Battle.wMaxEnemyIndex; i++)
         {
            if (g_Battle.rgEnemy[i].wObjectID != 0)
            {
               g_Battle.rgEnemy[i].e.wHealth -= pScript->rgwOperand[1];
            }
         }
      }
      else
      {
         //
         // Inflict damage to one enemy
         //
         g_Battle.rgEnemy[wEventObjectID].e.wHealth -= pScript->rgwOperand[1];
      }
      break;

   case 0x0022:
      //
      // Revive player
      //
      if (pScript->rgwOperand[0])
      {
         //
         // Apply to everyone
         //
         g_fScriptSuccess = FALSE;

         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            w = gpGlobals->rgParty[i].wPlayerRole;
            if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
            {
               gpGlobals->g.PlayerRoles.rgwHP[w] =
                  gpGlobals->g.PlayerRoles.rgwMaxHP[w] * pScript->rgwOperand[1] / 10;

               PAL_CurePoisonByLevel(w, 3);
               for (x = 0; x < kStatusAll; x++)
               {
                  PAL_RemovePlayerStatus(w, x);
               }

               g_fScriptSuccess = TRUE;
            }
         }
      }
      else
      {
         //
         // Apply to one player
         //
         if (gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] == 0)
         {
            gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] =
               gpGlobals->g.PlayerRoles.rgwMaxHP[wEventObjectID] * pScript->rgwOperand[1] / 10;

            PAL_CurePoisonByLevel(wEventObjectID, 3);
            for (x = 0; x < kStatusAll; x++)
            {
               PAL_RemovePlayerStatus(wEventObjectID, x);
            }
         }
         else
         {
            g_fScriptSuccess = FALSE;
         }
      }
      break;

   case 0x0023:
      //
      // Remove equipment from the specified player
      //
      if (pScript->rgwOperand[1] == 0)
      {
         //
         // Remove all equipments
         //
         for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
         {
            w = gpGlobals->g.PlayerRoles.rgwEquipment[i][iPlayerRole];
            if (w != 0)
            {
               PAL_AddItemToInventory(w, 1);
               gpGlobals->g.PlayerRoles.rgwEquipment[i][iPlayerRole] = 0;
            }
            PAL_RemoveEquipmentEffect(iPlayerRole, i);
         }
      }
      else
      {
         w = gpGlobals->g.PlayerRoles.rgwEquipment[pScript->rgwOperand[1] - 1][iPlayerRole];
         if (w != 0)
         {
            PAL_RemoveEquipmentEffect(iPlayerRole, pScript->rgwOperand[1] - 1);
            PAL_AddItemToInventory(w, 1);
            gpGlobals->g.PlayerRoles.rgwEquipment[pScript->rgwOperand[1] - 1][iPlayerRole] = 0;
         }
      }
      break;

   case 0x0024:
      //
      // Set the autoscript entry address for an event object
      //
      if (pScript->rgwOperand[0] != 0)
      {
         pCurrent->wAutoScript = pScript->rgwOperand[1];
      }
      break;

   case 0x0025:
      //
      // Set the trigger script entry address for an event object
      //
      if (pScript->rgwOperand[0] != 0)
      {
         pCurrent->wTriggerScript = pScript->rgwOperand[1];
      }
      break;

   case 0x0026:
      //
      // Show the buy item menu
      //
      PAL_MakeScene();
      VIDEO_UpdateScreen(NULL);
      PAL_BuyMenu(pScript->rgwOperand[0]);
      break;

   case 0x0027:
      //
      // Show the sell item menu
      //
      PAL_MakeScene();
      VIDEO_UpdateScreen(NULL);
      PAL_SellMenu();
      break;

   case 0x0028:
      break;

   case 0x0029:
      //
      // Apply poison to player
      //
      if (pScript->rgwOperand[0])
      {
         //
         // Apply to everyone
         //
         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            w = gpGlobals->rgParty[i].wPlayerRole;
            if (RandomLong(1, 100) > PAL_GetPlayerPoisonResistance(w))
            {
               PAL_AddPoisonForPlayer(w, pScript->rgwOperand[1]);
            }
         }
      }
      else
      {
         //
         // Apply to one player
         //
         if (RandomLong(1, 100) > PAL_GetPlayerPoisonResistance(wEventObjectID))
         {
            PAL_AddPoisonForPlayer(wEventObjectID, pScript->rgwOperand[1]);
         }
      }
      break;

   case 0x002A:
      break;

   case 0x002B:
      //
      // Cure poison by object ID
      //
      if (pScript->rgwOperand[0])
      {
         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            w = gpGlobals->rgParty[i].wPlayerRole;
            PAL_CurePoisonByKind(w, pScript->rgwOperand[1]);
         }
      }
      else
      {
         PAL_CurePoisonByKind(wEventObjectID, pScript->rgwOperand[1]);
      }
      break;

   case 0x002C:
      //
      // Cure poisons by level
      //
      if (pScript->rgwOperand[0])
      {
         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            w = gpGlobals->rgParty[i].wPlayerRole;
            PAL_CurePoisonByLevel(w, pScript->rgwOperand[1]);
         }
      }
      else
      {
         PAL_CurePoisonByLevel(wEventObjectID, pScript->rgwOperand[1]);
      }
      break;

   case 0x002D:
      //
      // Set the status for player
      //
      PAL_SetPlayerStatus(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1]);
      break;

   case 0x002E:
      break;

   case 0x002F:
      //
      // Remove player's status
      //
      PAL_RemovePlayerStatus(wEventObjectID, pScript->rgwOperand[0]);
      break;

   case 0x0030:
      //
      // Increase player's stat temporarily by percent
      //
      {
         WORD *p = (WORD *)(&gpGlobals->rgEquipmentEffect[kBodyPartExtra]); // HACKHACK
         WORD *p1 = (WORD *)(&gpGlobals->g.PlayerRoles);

         if (pScript->rgwOperand[2] == 0)
         {
            iPlayerRole = wEventObjectID;
         }
         else
         {
            iPlayerRole = pScript->rgwOperand[2] - 1;
         }

         p[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] =
            p1[pScript->rgwOperand[0] * MAX_PLAYER_ROLES + iPlayerRole] *
               (SHORT)pScript->rgwOperand[1] / 100;
      }
      break;

   case 0x0031:
      //
      // Change battle sprite temporarily for player
      //
      gpGlobals->rgEquipmentEffect[kBodyPartExtra].rgwSpriteNumInBattle[wEventObjectID] =
         pScript->rgwOperand[0];
      break;

   case 0x0033:
      break;

   case 0x0034:
      break;

   case 0x0035:
      //
      // Shake the screen
      //
      i = pScript->rgwOperand[1];
      if (i == 0)
      {
         i = 4;
      }
      VIDEO_ShakeScreen(pScript->rgwOperand[0], i);
      if (!pScript->rgwOperand[0])
      {
         VIDEO_UpdateScreen(NULL);
      }
      break;

   case 0x0036:
      //
      // Set the current playing RNG animation
      //
      gpGlobals->iCurPlayingRNG = pScript->rgwOperand[0];
      break;

   case 0x0037:
      //
      // Play RNG animation
      //
      PAL_RNGPlay(gpGlobals->iCurPlayingRNG,
         pScript->rgwOperand[0],
         pScript->rgwOperand[1] > 0 ? pScript->rgwOperand[1] : 999,
         pScript->rgwOperand[2] > 0 ? pScript->rgwOperand[2] : 16);
      break;

   case 0x0038:
      //
      // Teleport the party out of the scene
      //
      if (!gpGlobals->fInBattle &&
         gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wScriptOnTeleport != 0)
      {
         gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wScriptOnTeleport =
            PAL_RunTriggerScript(gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wScriptOnTeleport, 0xFFFF);
      }
      else
      {
         //
         // failed
         //
         g_fScriptSuccess = FALSE;
         wScriptEntry = pScript->rgwOperand[0] - 1;
      }
      break;

   case 0x0039:
      break;

   case 0x003A:
      break;

   case 0x003F:
      //
      // Ride the event object to the specified position, at a low speed
      //
      PAL_PartyRideEventObject(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
         pScript->rgwOperand[2], 2);
      break;

   case 0x0040:
      //
      // set the trigger method for a event object
      //
      if (pScript->rgwOperand[0] != 0)
      {
         pCurrent->wTriggerMode = pScript->rgwOperand[1];
      }
      break;

   case 0x0041:
      //
      // Mark the script as failed
      //
      g_fScriptSuccess = FALSE;
      break;

   case 0x0042:
      break;

   case 0x0043:
      //
      // Set background music
      //
      gpGlobals->wNumMusic = pScript->rgwOperand[0];
      RIX_Play(pScript->rgwOperand[0], TRUE, pScript->rgwOperand[1]);
      break;

   case 0x0044:
      //
      // Ride the event object to the specified position, at the normal speed
      //
      PAL_PartyRideEventObject(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
         pScript->rgwOperand[2], 4);
      break;

   case 0x0045:
      //
      // Set battle music
      //
      gpGlobals->wNumBattleMusic = pScript->rgwOperand[0];
      break;

   case 0x0046:
      //
      // Set the party position on the map
      //
      {
         int xOffset, yOffset, x, y;

         xOffset =
            ((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirSouth)
               ? 16 : -16);
         yOffset =
            ((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirNorth)
               ? 8 : -8);

         x = pScript->rgwOperand[0] * 32 + pScript->rgwOperand[2] * 16;
         y = pScript->rgwOperand[1] * 16 + pScript->rgwOperand[2] * 8;

         x -= PAL_X(gpGlobals->partyoffset);
         y -= PAL_Y(gpGlobals->partyoffset);

         gpGlobals->viewport = PAL_XY(x, y);

         x = PAL_X(gpGlobals->partyoffset);
         y = PAL_Y(gpGlobals->partyoffset);

         for (i = 0; i < MAX_PLAYABLE_PLAYER_ROLES; i++)
         {
            gpGlobals->rgParty[i].x = x;
            gpGlobals->rgParty[i].y = y;
            gpGlobals->rgTrail[i].x = x + PAL_X(gpGlobals->viewport);
            gpGlobals->rgTrail[i].y = y + PAL_Y(gpGlobals->viewport);
            gpGlobals->rgTrail[i].wDirection = gpGlobals->wPartyDirection;

            x += xOffset;
            y += yOffset;
         }
      }
      break;

   case 0x0047:
      //
      // Play sound effect
      //
      SOUND_Play(pScript->rgwOperand[0]);
      break;

   case 0x0049:
      //
      // Set the state of event object
      //
      pCurrent->sState = pScript->rgwOperand[1];
      break;

   case 0x004A:
      //
      // Set the current battlefield
      //
      gpGlobals->wNumBattleField = pScript->rgwOperand[0];
      break;

   case 0x004B:
      //
      // Nullify the event object for a short while
      //
      pEvtObj->sVanishTime = -15;
      break;

   case 0x004C:
      //
      // chase the player
      //
      i = pScript->rgwOperand[0]; // max. distance
      j = pScript->rgwOperand[1]; // speed

      if (i == 0)
      {
         i = 8;
      }

      if (j == 0)
      {
         j = 4;
      }

      if (gpGlobals->wChaseRange != 0)
      {
         x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset) - pEvtObj->x;
         y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset) - pEvtObj->y;

         //
         // Is the party near to the event object?
         //
         if (abs(x) + abs(y) * 2 < i * 32 * gpGlobals->wChaseRange)
         {
            if (x < 0)
            {
               if (y < 0)
               {
                  pEvtObj->wDirection = kDirWest;
               }
               else
               {
                  pEvtObj->wDirection = kDirSouth;
               }
            }
            else
            {
               if (y < 0)
               {
                  pEvtObj->wDirection = kDirNorth;
               }
               else
               {
                  pEvtObj->wDirection = kDirEast;
               }
            }

            if (x != 0)
            {
               x = pEvtObj->x + x / abs(x) * j * 2;
            }
            else
            {
               x = pEvtObj->x;
            }

            if (y != 0)
            {
               y = pEvtObj->y + y / abs(y) * j;
            }
            else
            {
               y = pEvtObj->y;
            }

            if (pScript->rgwOperand[2] ||
               !PAL_CheckObstacle(PAL_XY(x, y), TRUE, wEventObjectID))
            {
               pEvtObj->x = x;
               pEvtObj->y = y;
            }
         }
      }

      PAL_NPCWalkOneStep(wEventObjectID, 0);
      break;

   case 0x004D:
      //
      // wait for any key
      //
      PAL_WaitForKey();
      break;

   case 0x004E:
      //
      // Load the last saved game
      //
      PAL_FadeOut(1);
      PAL_InitGameData(gpGlobals->bCurrentSaveSlot);
      return 0; // don't go further

   case 0x004F:
      //
      // Fade the screen to red color (game over)
      //
      PAL_FadeToRed();
      break;

   case 0x0050:
      //
      // screen fade out
      //
      VIDEO_UpdateScreen(NULL);
      PAL_FadeOut(pScript->rgwOperand[0] ? pScript->rgwOperand[0] : 1);
      gpGlobals->fNeedToFadeIn = TRUE;
      break;

   case 0x0051:
      //
      // screen fade in
      //
      VIDEO_UpdateScreen(NULL);
      PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette,
         ((SHORT)(pScript->rgwOperand[0]) > 0) ? pScript->rgwOperand[0] : 1);
      gpGlobals->fNeedToFadeIn = FALSE;
      break;

   case 0x0052:
      //
      // hide the event object for a while, default 800 frames
      //
      pEvtObj->sState *= -1;
      pEvtObj->sVanishTime = (pScript->rgwOperand[0] ? pScript->rgwOperand[0] : 800);
      break;

   case 0x0053:
      //
      // use the day palette
      //
      gpGlobals->fNightPalette = FALSE;
      break;

   case 0x0054:
      //
      // use the night palette
      //
      gpGlobals->fNightPalette = TRUE;
      break;

   case 0x0055:
      //
      // Add magic to a player
      //
      i = pScript->rgwOperand[1];
      if (i == 0)
      {
         i = wEventObjectID;
      }
      else
      {
         i--;
      }
      PAL_AddMagic(i, pScript->rgwOperand[0]);
      break;

   case 0x0056:
      //
      // Remove magic from a player
      //
      i = pScript->rgwOperand[1];
      if (i == 0)
      {
         i = wEventObjectID;
      }
      else
      {
         i--;
      }
      PAL_RemoveMagic(i, pScript->rgwOperand[0]);
      break;

   case 0x0057:
      break;

   case 0x0058:
      //
      // Jump if there is less than the specified number of the specified items
      // in the inventory
      //
      if (PAL_GetItemAmount(pScript->rgwOperand[0]) < (SHORT)(pScript->rgwOperand[1]))
      {
         wScriptEntry = pScript->rgwOperand[2] - 1;
      }
      break;

   case 0x0059:
      //
      // Change to the specified scene
      //
      if (pScript->rgwOperand[0] > 0 && pScript->rgwOperand[0] <= MAX_SCENES &&
         gpGlobals->wNumScene != pScript->rgwOperand[0])
      {
         //
         // Set data to load the scene in the next frame
         //
         gpGlobals->wNumScene = pScript->rgwOperand[0];
         PAL_SetLoadFlags(kLoadScene);
         gpGlobals->fEnteringScene = TRUE;
         gpGlobals->wLayer = 0;
      }
      break;

   case 0x005A:
      //
      // Halve the player's HP
      // The wEventObjectID parameter here should indicate the player role
      //
      gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] /= 2;
      break;

   case 0x005B:
      break;

   case 0x005C:
      break;

   case 0x005D:
      //
      // Jump if player doesn't have the specified poison
      //
      if (!PAL_IsPlayerPoisonedByKind(wEventObjectID, pScript->rgwOperand[0]))
      {
         wScriptEntry = pScript->rgwOperand[1] - 1;
      }
      break;

   case 0x005E:
      break;

   case 0x005F:
      //
      // Kill the player immediately
      // The wEventObjectID parameter here should indicate the player role
      //
      gpGlobals->g.PlayerRoles.rgwHP[wEventObjectID] = 0;
      break;

   case 0x0060:
      break;

   case 0x0061:
      //
      // Jump if player is not poisoned
      //
      if (!PAL_IsPlayerPoisonedByLevel(wEventObjectID, 1))
      {
         wScriptEntry = pScript->rgwOperand[0] - 1;
      }
      break;

   case 0x0062:
      //
      // Pause enemy chasing for a while
      //
      gpGlobals->wChasespeedChangeCycles = pScript->rgwOperand[0];
      gpGlobals->wChaseRange = 0;
      break;

   case 0x0063:
      //
      // Speed up enemy chasing for a while
      //
      gpGlobals->wChasespeedChangeCycles = pScript->rgwOperand[0];
      gpGlobals->wChaseRange = 3;
      break;

   case 0x0064:
      break;

   case 0x0065:
      //
      // Set the player's sprite
      //
      gpGlobals->g.PlayerRoles.rgwSpriteNum[pScript->rgwOperand[0]] = pScript->rgwOperand[1];
      if (!gpGlobals->fInBattle && pScript->rgwOperand[2])
      {
         PAL_SetLoadFlags(kLoadPlayerSprite);
         PAL_LoadResources();
      }
      break;

   case 0x0066:
      break;

   case 0x0067:
      break;

   case 0x0068:
      break;

   case 0x0069:
      //
      // Enemy escape in battle
      //
      PAL_BattleEnemyEscape();
      break;

   case 0x006A:
      break;

   case 0x006B:
      break;

   case 0x006C:
      //
      // Walk the NPC in one step
      //
      pCurrent->x += (SHORT)(pScript->rgwOperand[1]);
      pCurrent->y += (SHORT)(pScript->rgwOperand[2]);
      PAL_NPCWalkOneStep(wCurEventObjectID, 0);
      break;

   case 0x006D:
      //
      // Set the enter script and teleport script for a scene
      //
      if (pScript->rgwOperand[0])
      {
         if (pScript->rgwOperand[1])
         {
            gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wScriptOnEnter =
               pScript->rgwOperand[1];
         }

         if (pScript->rgwOperand[2])
         {
            gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wScriptOnTeleport =
               pScript->rgwOperand[2];
         }

         if (pScript->rgwOperand[1] == 0 && pScript->rgwOperand[2] == 0)
         {
            gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wScriptOnEnter = 0;
            gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wScriptOnTeleport = 0;
         }
      }
      break;

   case 0x006E:
      //
      // Move the player to the specified position in one step
      //
      for (i = 3; i >= 0; i--)
      {
         gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
      }
      gpGlobals->rgTrail[0].wDirection = gpGlobals->wPartyDirection;
      gpGlobals->rgTrail[0].x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
      gpGlobals->rgTrail[0].y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

      gpGlobals->viewport = PAL_XY(
         PAL_X(gpGlobals->viewport) + (SHORT)(pScript->rgwOperand[0]),
         PAL_Y(gpGlobals->viewport) + (SHORT)(pScript->rgwOperand[1]));

      gpGlobals->wLayer = pScript->rgwOperand[2] * 8;

      if (pScript->rgwOperand[0] != 0 || pScript->rgwOperand[1] != 0)
      {
         PAL_UpdatePartyGestures(TRUE);
      }
      break;

   case 0x006F:
      //
      // Sync the state of current event object with another event object
      //
      if (pCurrent->sState == (SHORT)(pScript->rgwOperand[1]))
      {
         pEvtObj->sState = (SHORT)(pScript->rgwOperand[1]);
      }
      break;

   case 0x0070:
      //
      // Walk the party to the specified position
      //
      PAL_PartyWalkTo(pScript->rgwOperand[0], pScript->rgwOperand[1], pScript->rgwOperand[2], 2);
      break;

   case 0x0071:
      //
      // Wave the screen
      //
      gpGlobals->wScreenWave = pScript->rgwOperand[0];
      gpGlobals->sWaveProgression = (SHORT)(pScript->rgwOperand[1]);
      break;

   case 0x0073:
      //
      // Fade the screen to scene
      //
      VIDEO_BackupScreen();
      PAL_MakeScene();
      VIDEO_FadeScreen(pScript->rgwOperand[0]);
      break;

   case 0x0074:
      //
      // Jump if not all players are full HP
      //
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         w = gpGlobals->rgParty[i].wPlayerRole;
         if (gpGlobals->g.PlayerRoles.rgwHP[w] < gpGlobals->g.PlayerRoles.rgwMaxHP[w])
         {
            wScriptEntry = pScript->rgwOperand[0] - 1;
            break;
         }
      }
      break;

   case 0x0075:
      //
      // Set the player party
      //
      gpGlobals->wMaxPartyMemberIndex = 0;
      for (i = 0; i < 3; i++)
      {
         if (pScript->rgwOperand[i] != 0)
         {
            gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex++].wPlayerRole =
               pScript->rgwOperand[i] - 1;
         }
      }
      gpGlobals->wMaxPartyMemberIndex--;

      //
      // Reload the player sprites
      //
      PAL_SetLoadFlags(kLoadPlayerSprite);
      PAL_LoadResources();

      PAL_UpdateEquipments();
      break;

   case 0x0076:
      //
      // Show FBP picture
      //
      PAL_EndingSetEffectSprite(0);
      PAL_ShowFBP(pScript->rgwOperand[0], pScript->rgwOperand[1]);
      break;

   case 0x0077:
      //
      // Stop current playing music
      //
      RIX_Play(0, FALSE,
         (pScript->rgwOperand[0] == 0) ? 2.0f : (FLOAT)(pScript->rgwOperand[0]) * 2);
      gpGlobals->wNumMusic = 0;
      break;

   case 0x0078:
      break;

   case 0x0079:
      //
      // Jump if the specified player is in the party
      //
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         if (gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[i].wPlayerRole] ==
            pScript->rgwOperand[0])
         {
            wScriptEntry = pScript->rgwOperand[1] - 1;
            break;
         }
      }
      break;

   case 0x007A:
      //
      // Walk the party to the specified position, at a higher speed
      //
      PAL_PartyWalkTo(pScript->rgwOperand[0], pScript->rgwOperand[1], pScript->rgwOperand[2], 4);
      break;

   case 0x007B:
      //
      // Walk the party to the specified position, at the highest speed
      //
      PAL_PartyWalkTo(pScript->rgwOperand[0], pScript->rgwOperand[1], pScript->rgwOperand[2], 8);
      break;

   case 0x007C:
      //
      // Walk straight to the specified position
      //
      if ((wEventObjectID & 1) ^ (gpGlobals->dwFrameNum & 1))
      {
         if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
            pScript->rgwOperand[2], 4))
         {
            wScriptEntry--;
         }
      }
      else
      {
         wScriptEntry--;
      }
      break;

   case 0x007D:
      //
      // Move the event object
      //
      pCurrent->x += (SHORT)(pScript->rgwOperand[1]);
      pCurrent->y += (SHORT)(pScript->rgwOperand[2]);
      break;

   case 0x007E:
      //
      // Set the layer of event object
      //
      pCurrent->sLayer = (SHORT)(pScript->rgwOperand[1]);
      break;

   case 0x007F:
      //
      // Move the viewport
      //
      if (pScript->rgwOperand[0] == 0 && pScript->rgwOperand[1] == 0)
      {
         //
         // Move the viewport back to normal state
         //
         x = gpGlobals->rgParty[0].x - 160;
         y = gpGlobals->rgParty[0].y - 112;

         gpGlobals->viewport =
            PAL_XY(PAL_X(gpGlobals->viewport) + x, PAL_Y(gpGlobals->viewport) + y);
         gpGlobals->partyoffset = PAL_XY(160, 112);

         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            gpGlobals->rgParty[i].x -= x;
            gpGlobals->rgParty[i].y -= y;
         }

         if (pScript->rgwOperand[2] != 0xFFFF)
         {
            PAL_MakeScene();
            VIDEO_UpdateScreen(NULL);
         }
      }
      else
      {
         DWORD time;

         i = 0;

         x = (SHORT)(pScript->rgwOperand[0]);
         y = (SHORT)(pScript->rgwOperand[1]);

         time = SDL_GetTicks() + FRAME_TIME;

         do
         {
            if (pScript->rgwOperand[2] == 0xFFFF)
            {
               x = PAL_X(gpGlobals->viewport);
               y = PAL_Y(gpGlobals->viewport);

               gpGlobals->viewport =
                  PAL_XY(pScript->rgwOperand[0] * 32 - 160, pScript->rgwOperand[1] * 16 - 112);

               x -= PAL_X(gpGlobals->viewport);
               y -= PAL_Y(gpGlobals->viewport);

               for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
               {
                  gpGlobals->rgParty[j].x += x;
                  gpGlobals->rgParty[j].y += y;
               }
            }
            else
            {
               gpGlobals->viewport =
                  PAL_XY(PAL_X(gpGlobals->viewport) + x, PAL_Y(gpGlobals->viewport) + y);
               gpGlobals->partyoffset =
                  PAL_XY(PAL_X(gpGlobals->partyoffset) - x, PAL_Y(gpGlobals->partyoffset) - y);

               for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
               {
                  gpGlobals->rgParty[j].x -= x;
                  gpGlobals->rgParty[j].y -= y;
               }
            }

            if (pScript->rgwOperand[2] != 0xFFFF)
            {
               PAL_GameUpdate(FALSE);
            }

            PAL_MakeScene();
            VIDEO_UpdateScreen(NULL);

            //
            // Delay for one frame
            //
            PAL_ProcessEvent();
            while (SDL_GetTicks() < time)
            {
               PAL_ProcessEvent();
               SDL_Delay(1);
            }
            time = SDL_GetTicks() + FRAME_TIME;
         } while (++i < (SHORT)(pScript->rgwOperand[2]));
      }
      break;

   case 0x0080:
      //
      // Toggle day/night palette
      //
      gpGlobals->fNightPalette = !(gpGlobals->fNightPalette);
      PAL_PaletteFade(gpGlobals->wNumPalette, gpGlobals->fNightPalette,
         !(pScript->rgwOperand[0]));
      break;

   case 0x0081:
      //
      // Jump if the player is not facing the specified event object
      //
      {
         if (pScript->rgwOperand[0] <= gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex ||
            pScript->rgwOperand[0] > gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex)
         {
            //
            // The event object is not in the current scene
            //
            wScriptEntry = pScript->rgwOperand[2] - 1;
            g_fScriptSuccess = FALSE;
            break;
         }

         x = pCurrent->x;
         y = pCurrent->y;

         x +=
            ((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirSouth)
               ? 16 : -16);
         y +=
            ((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirNorth)
               ? 8 : -8);

         x -= PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
         y -= PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

         if (abs(x) + abs(y * 2) < pScript->rgwOperand[1] * 32 + 16)
         {
            if (pScript->rgwOperand[1] > 0)
            {
               //
               // Change the trigger mode so that the object can be triggered in next frame
               //
               pCurrent->wTriggerMode = kTriggerTouchNormal + pScript->rgwOperand[1];
            }
         }
         else
         {
            wScriptEntry = pScript->rgwOperand[2] - 1;
            g_fScriptSuccess = FALSE;
         }
      }
      break;

   case 0x0082:
      //
      // Walk straight to the specified position, at a high speed
      //
      if (!PAL_NPCWalkTo(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
         pScript->rgwOperand[2], 8))
      {
         wScriptEntry--;
      }
      break;

   case 0x0083:
      //
      // Jump if event object is not in the specified zone of the current event object
      //
      if (pScript->rgwOperand[0] <= gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex ||
         pScript->rgwOperand[0] > gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex)
      {
         //
         // The event object is not in the current scene
         //
         wScriptEntry = pScript->rgwOperand[2] - 1;
         g_fScriptSuccess = FALSE;
         break;
      }

      x = pEvtObj->x - pCurrent->x;
      y = pEvtObj->y - pCurrent->y;

      if (abs(x) + abs(y * 2) >= pScript->rgwOperand[1] * 32 + 16)
      {
         wScriptEntry = pScript->rgwOperand[2] - 1;
         g_fScriptSuccess = FALSE;
      }
      break;

   case 0x0084:
      //
      // Place the item which player used as an event object to the scene
      //
      if (pScript->rgwOperand[0] <= gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex ||
         pScript->rgwOperand[0] > gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex)
      {
         //
         // The event object is not in the current scene
         //
         wScriptEntry = pScript->rgwOperand[2] - 1;
         g_fScriptSuccess = FALSE;
         break;
      }

      x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
      y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

      x +=
         ((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirSouth)
            ? -16 : 16);
      y +=
         ((gpGlobals->wPartyDirection == kDirWest || gpGlobals->wPartyDirection == kDirNorth)
            ? -8 : 8);

      if (PAL_CheckObstacle(PAL_XY(x, y), FALSE, 0))
      {
         wScriptEntry = pScript->rgwOperand[2] - 1;
         g_fScriptSuccess = FALSE;
      }
      else
      {
         pCurrent->x = x;
         pCurrent->y = y;
         pCurrent->sState = (SHORT)(pScript->rgwOperand[1]);
      }
      break;

   case 0x0085:
      //
      // Delay for a period
      //
      UTIL_Delay(pScript->rgwOperand[0] * 80);
      break;

   case 0x0086:
      //
      // Jump if the specified item is not equipped
      //
      y = FALSE;
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         w = gpGlobals->rgParty[i].wPlayerRole;
         for (x = 0; x < MAX_PLAYER_EQUIPMENTS; x++)
         {
            if (gpGlobals->g.PlayerRoles.rgwEquipment[x][w] == pScript->rgwOperand[0])
            {
               y = TRUE;
               i = 999;
               break;
            }
         }
      }
      if (!y)
      {
         wScriptEntry = pScript->rgwOperand[2] - 1;
      }
      break;

   case 0x0087:
      //
      // Animate the event object
      //
      PAL_NPCWalkOneStep(wCurEventObjectID, 0);
      break;

   case 0x0088:
      //
      // Set the base damage of magic according to amount of money
      //
      i = ((gpGlobals->dwCash > 5000) ? 5000 : gpGlobals->dwCash);
      gpGlobals->dwCash -= i;
      j = gpGlobals->g.rgObject[pScript->rgwOperand[0]].magic.wMagicNumber;
      gpGlobals->g.lprgMagic[j].wBaseDamage = i * 2 / 5;
      break;

   case 0x0089:
      //
      // Set the battle result
      //
      g_Battle.BattleResult = pScript->rgwOperand[0];
      break;

   case 0x008A:
      //
      // Enable Auto-Battle
      //
      gpGlobals->fAutoBattle = TRUE;
      break;

   case 0x008B:
      //
      // change the current palette
      //
      gpGlobals->wNumPalette = pScript->rgwOperand[0];
      if (!gpGlobals->fNeedToFadeIn)
      {
         PAL_SetPalette(gpGlobals->wNumPalette, FALSE);
      }
      break;

   case 0x008C:
      //
      // Fade from/to color
      //
      PAL_ColorFade(pScript->rgwOperand[1], (BYTE)(pScript->rgwOperand[0]),
         pScript->rgwOperand[2]);
      gpGlobals->fNeedToFadeIn = FALSE;
      break;

   case 0x008D:
      //
      // Increase player's level
      //
      PAL_PlayerLevelUp(wEventObjectID, pScript->rgwOperand[0]);
      break;

   case 0x008F:
      //
      // Halve the cash amount
      //
      gpGlobals->dwCash /= 2;
      break;

   case 0x0090:
      //
      // Set the object script
      //
      gpGlobals->g.rgObject[pScript->rgwOperand[0]].rgwData[2 + pScript->rgwOperand[2]] =
         pScript->rgwOperand[1];
      break;

   case 0x0091:
      //
      // Jump if the enemy is not alone
      //
      if (gpGlobals->fInBattle)
      {
         for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
         {
            if (i != wEventObjectID &&
               g_Battle.rgEnemy[i].wObjectID == g_Battle.rgEnemy[wEventObjectID].wObjectID)
            {
               wScriptEntry = pScript->rgwOperand[0] - 1;
               break;
            }
         }
      }
      break;

   case 0x0092:
      break;

   case 0x0093:
      //
      // Fade the screen. Update scene in the process.
      //
      PAL_SceneFade(gpGlobals->wNumPalette, gpGlobals->fNightPalette,
         (SHORT)(pScript->rgwOperand[0]));
      gpGlobals->fNeedToFadeIn = ((SHORT)(pScript->rgwOperand[0]) < 0);
      break;

   case 0x0094:
      //
      // Jump if the state of event object is the specified one
      //
      if (pCurrent->sState == (SHORT)(pScript->rgwOperand[1]))
      {
         wScriptEntry = pScript->rgwOperand[2] - 1;
      }
      break;

   case 0x0095:
      //
      // Jump if the current scene is the specified one
      //
      if (gpGlobals->wNumScene == pScript->rgwOperand[0])
      {
         wScriptEntry = pScript->rgwOperand[1] - 1;
      }
      break;

   case 0x0096:
      //
      // Show the ending animation
      //
      PAL_EndingAnimation();
      break;

   case 0x0097:
      //
      // Ride the event object to the specified position, at a higher speed
      //
      PAL_PartyRideEventObject(wEventObjectID, pScript->rgwOperand[0], pScript->rgwOperand[1],
         pScript->rgwOperand[2], 8);
      break;

   case 0x0098:
      //
      // Set follower of the party
      //
      if (pScript->rgwOperand[0] > 0)
      {
         gpGlobals->nFollower = 1;
         gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].wPlayerRole = pScript->rgwOperand[0];

         PAL_SetLoadFlags(kLoadPlayerSprite);
         PAL_LoadResources();

         //
         // Update the position and gesture for the follower
         //
         gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].x =
            gpGlobals->rgTrail[3].x - PAL_X(gpGlobals->viewport);
         gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].y =
            gpGlobals->rgTrail[3].y - PAL_Y(gpGlobals->viewport);
         gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].wFrame =
            gpGlobals->rgTrail[3].wDirection * 3;
      }
      else
      {
         gpGlobals->nFollower = 0;
      }
      break;

   case 0x0099:
      //
      // Change the map for the specified scene
      //
      if (pScript->rgwOperand[0] == 0xFFFF)
      {
         gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wMapNum = pScript->rgwOperand[1];
         PAL_SetLoadFlags(kLoadScene);
         PAL_LoadResources();
      }
      else
      {
         gpGlobals->g.rgScene[pScript->rgwOperand[0] - 1].wMapNum = pScript->rgwOperand[1];
      }
      break;

   case 0x009A:
      //
      // Set the state for multiple event objects
      //
      for (i = pScript->rgwOperand[0]; i <= pScript->rgwOperand[1]; i++)
      {
         gpGlobals->g.lprgEventObject[i - 1].sState = pScript->rgwOperand[2];
      }
      break;

   case 0x009B:
      //
      // Fade to the current scene
      // FIXME: This is obviously wrong
      //
      VIDEO_BackupScreen();
      PAL_MakeScene();
      VIDEO_FadeScreen(2);
      break;

   case 0x009C:
      break;

   case 0x009E:
      break;

   case 0x009F:
      break;

   case 0x00A0:
      //
      // Quit game
      //
      PAL_Shutdown();
      exit(0);
      break;

   case 0x00A1:
      //
      // Set the positions of all party members to the same as the first one
      //
      for (i = 0; i < MAX_PLAYABLE_PLAYER_ROLES; i++)
      {
         gpGlobals->rgTrail[i].wDirection = gpGlobals->wPartyDirection;
         gpGlobals->rgTrail[i].x = gpGlobals->rgParty[0].x + PAL_X(gpGlobals->viewport);
         gpGlobals->rgTrail[i].y = gpGlobals->rgParty[0].y + PAL_Y(gpGlobals->viewport);
      }
      for (i = 1; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         gpGlobals->rgParty[i].x = gpGlobals->rgParty[0].x;
         gpGlobals->rgParty[i].y = gpGlobals->rgParty[0].y - 1;
      }
      PAL_UpdatePartyGestures(FALSE);
      break;

   case 0x00A2:
      //
      // Jump to one of the following instructions randomly
      //
      wScriptEntry += RandomLong(0, pScript->rgwOperand[0] - 1);
      break;

   case 0x00A3:
      //
      // Play CD music. Just use the RIX music here.
      //
      RIX_Play(pScript->rgwOperand[1], TRUE, 0);
      break;

   case 0x00A4:
      //
      // Scroll FBP to the screen
      //
      if (pScript->rgwOperand[0] == 68)
      {
         //
         // HACKHACK: to make the ending picture show correctly
         //
         PAL_ShowFBP(69, 0);
         PAL_ScrollFBP(pScript->rgwOperand[0], pScript->rgwOperand[2], TRUE);
      }
      else
      {
         PAL_ScrollFBP(pScript->rgwOperand[0], pScript->rgwOperand[2], pScript->rgwOperand[1]);
      }
      break;

   case 0x00A5:
      //
      // Show FBP picture with sprite effects
      //
      if (pScript->rgwOperand[1] != 0xFFFF)
      {
         PAL_EndingSetEffectSprite(pScript->rgwOperand[1]);
      }
      PAL_ShowFBP(pScript->rgwOperand[0], pScript->rgwOperand[2]);
      break;

   case 0x00A6:
      //
      // backup screen
      //
      VIDEO_BackupScreen();
      break;
   case 0xffff:
	   //lost event
	   break;
   default:
      TerminateOnError("SCRIPT: Invalid Instruction at %4x: (%4x - %4x, %4x, %4x)",
         wScriptEntry, pScript->wOperation, pScript->rgwOperand[0],
         pScript->rgwOperand[1], pScript->rgwOperand[2]);
      break;
   }

   return wScriptEntry + 1;
}

WORD
PAL_RunTriggerScript(
   WORD           wScriptEntry,
   WORD           wEventObjectID
)
/*++
  Purpose:

    Runs a trigger script.

  Parameters:

    [IN]  wScriptEntry - The script entry to execute.

    [IN]  wEventObjectID - The event object ID which invoked the script.

  Return value:

    The entry point of the script.

--*/
{
   static WORD       wLastEventObject = 0;

   WORD              wNextScriptEntry;
   BOOL              fEnded;
   LPSCRIPTENTRY     pScript;
   LPEVENTOBJECT     pEvtObj = NULL;
   int               i;
   FILE *fp;
   extern BOOL       g_fUpdatedInBattle; // HACKHACK

   wNextScriptEntry = wScriptEntry;
   fEnded = FALSE;
   g_fUpdatedInBattle = FALSE;

   if (wEventObjectID == 0xFFFF)
   {
      wEventObjectID = wLastEventObject;
   }

   wLastEventObject = wEventObjectID;

   if (wEventObjectID != 0)
   {
      pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);
   }

   g_fScriptSuccess = TRUE;

   while (wScriptEntry != 0 && !fEnded)
   {
      pScript = &(gpGlobals->g.lprgScriptEntry[wScriptEntry]);

	  #ifdef SCRIPT_TRACE
      {
		  fp = fopen("e:\\data\\pal\\log.txt", "a");
		  //FILE *fp = fopen("log.txt", "a");
         if (fp != NULL)
         {
            fprintf(fp, "%.4x: %.4x %.4x %.4x %.4x\n", wScriptEntry,
               pScript->wOperation, pScript->rgwOperand[0], pScript->rgwOperand[1],
               pScript->rgwOperand[2], pScript->rgwOperand[3]);
            fclose(fp);
         }
      }
#endif

      switch (pScript->wOperation)
      {
      case 0x0000:
         //
         // Stop running
         //
         fEnded = TRUE;
         break;

      case 0x0001:
         //
         // Stop running and replace the entry with the next line
         //
         fEnded = TRUE;
         wNextScriptEntry = wScriptEntry + 1;
         break;

      case 0x0002:
         //
         // Stop running and replace the entry with the specified one
         //
         if (pScript->rgwOperand[1] == 0 ||
            ++(pEvtObj->nScriptIdleFrame) < pScript->rgwOperand[1])
         {
            fEnded = TRUE;
            wNextScriptEntry = pScript->rgwOperand[0];
         }
         else
         {
            //
            // failed
            //
            pEvtObj->nScriptIdleFrame = 0;
            wScriptEntry++;
         }
         break;

      case 0x0003:
         //
         // unconditional jump
         //
         if (pScript->rgwOperand[1] == 0 ||
            ++(pEvtObj->nScriptIdleFrame) < pScript->rgwOperand[1])
         {
            wScriptEntry = pScript->rgwOperand[0];
         }
         else
         {
            //
            // failed
            //
            pEvtObj->nScriptIdleFrame = 0;
            wScriptEntry++;
         }
         break;

      case 0x0004:
         //
         // Call script
         //
         PAL_RunTriggerScript(pScript->rgwOperand[0],
            ((pScript->rgwOperand[1] == 0) ? wEventObjectID : pScript->rgwOperand[1]));
         wScriptEntry++;
         break;

      case 0x0005:
         //
         // Redraw screen
         //
         PAL_ClearDialog();

         if (PAL_DialogIsPlayingRNG())
         {
            VIDEO_RestoreScreen();
         }
         else if (gpGlobals->fInBattle)
         {
            PAL_BattleMakeScene();
            SDL_BlitSurface(g_Battle.lpSceneBuf, NULL, gpScreen, NULL);
            VIDEO_UpdateScreen(NULL);
         }
         else
         {
            if (pScript->rgwOperand[2])
            {
               PAL_UpdatePartyGestures(FALSE);
            }

            PAL_MakeScene();

            VIDEO_UpdateScreen(NULL);
            UTIL_Delay((pScript->rgwOperand[1] == 0) ? 60 : (pScript->rgwOperand[1] * 60));
         }

         wScriptEntry++;
         break;

      case 0x0006:
         //
         // Jump to the specified address by the specified rate
         //
         if (RandomLong(1, 100) >= pScript->rgwOperand[0])
         {
            wScriptEntry = pScript->rgwOperand[1];
            continue;
         }
         else
         {
            wScriptEntry++;
         }
         break;

      case 0x0007:
         //
         // Start battle
         //
         i = PAL_StartBattle(pScript->rgwOperand[0], !pScript->rgwOperand[2]);

         if (i == kBattleResultLost && pScript->rgwOperand[1] != 0)
         {
            wScriptEntry = pScript->rgwOperand[1];
         }
         else if (i == kBattleResultFleed && pScript->rgwOperand[2] != 0)
         {
            wScriptEntry = pScript->rgwOperand[2];
         }
         else
         {
            wScriptEntry++;
         }
         gpGlobals->fAutoBattle = FALSE;
         break;

      case 0x0008:
         //
         // Replace the entry with the next instruction
         //
         wScriptEntry++;
         wNextScriptEntry = wScriptEntry;
         break;

      case 0x0009:
         //
         // wait for the specified number of frames
         //
         {
            DWORD        time;

            PAL_ClearDialog();

            time = SDL_GetTicks() + FRAME_TIME;

            for (i = 0; i < (pScript->rgwOperand[0] ? pScript->rgwOperand[0] : 1); i++)
            {
               while (SDL_GetTicks() < time)
               {
                  UTIL_Delay(1);
               }

               time = SDL_GetTicks() + FRAME_TIME;

               if (pScript->rgwOperand[2])
               {
                  PAL_UpdatePartyGestures(FALSE);
               }

               PAL_GameUpdate(pScript->rgwOperand[1] ? TRUE : FALSE);
               PAL_MakeScene();
               VIDEO_UpdateScreen(NULL);
            }
         }
         wScriptEntry++;
         break;

      case 0x000A:
         //
         // Goto the specified address if player selected no
         //
         PAL_ClearDialog();

         if (!PAL_ConfirmMenu())
         {
            wScriptEntry = pScript->rgwOperand[0];
         }
         else
         {
            wScriptEntry++;
         }
         break;

      case 0x003B:
         //
         // Show dialog in the middle part of the screen
         //
         PAL_ClearDialog();
         PAL_StartDialog(kDialogCenter, (BYTE)pScript->rgwOperand[0], 0,
            pScript->rgwOperand[2] ? TRUE : FALSE);
         wScriptEntry++;
         break;

      case 0x003C:
         //
         // Show dialog in the upper part of the screen
         //
         PAL_ClearDialog();
         PAL_StartDialog(kDialogUpper, (BYTE)pScript->rgwOperand[1],
            pScript->rgwOperand[0], pScript->rgwOperand[2] ? TRUE : FALSE);
         wScriptEntry++;
         break;

      case 0x003D:
         //
         // Show dialog in the lower part of the screen
         //
         PAL_ClearDialog();
         PAL_StartDialog(kDialogLower, (BYTE)pScript->rgwOperand[1],
            pScript->rgwOperand[0], pScript->rgwOperand[2] ? TRUE : FALSE);
         wScriptEntry++;
         break;

      case 0x003E:
         //
         // Show text in a window at the center of the screen
         //
         PAL_ClearDialog();
         PAL_StartDialog(kDialogCenterWindow, (BYTE)pScript->rgwOperand[0], 0, FALSE);
         wScriptEntry++;
         break;

      case 0x008E:
         //
         // Restore the screen
         //
         PAL_ClearDialog();
         VIDEO_RestoreScreen();
         VIDEO_UpdateScreen(NULL);
         wScriptEntry++;
         break;

      case 0xFFFF:
         //
         // Print dialog text
         //
         PAL_ShowDialogText(PAL_GetMsg(pScript->rgwOperand[0]));
         wScriptEntry++;
         break;

      default:
         PAL_ClearDialog();
         wScriptEntry = PAL_InterpretInstruction(wScriptEntry, wEventObjectID);
         break;
      }
   }

   PAL_EndDialog();
   g_iCurEquipPart = -1;

   return wNextScriptEntry;
}

WORD
PAL_RunAutoScript(
   WORD           wScriptEntry,
   WORD           wEventObjectID
)
/*++
  Purpose:

    Runs the autoscript of the specified event object.

  Parameters:

    [IN]  wScriptEntry - The script entry to execute.

    [IN]  wEventObjectID - The event object ID which invoked the script.

  Return value:

    The address of the next script instruction to execute.

--*/
{
   LPSCRIPTENTRY          pScript;
   LPEVENTOBJECT          pEvtObj;

begin:
   pScript = &(gpGlobals->g.lprgScriptEntry[wScriptEntry]);
   pEvtObj = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);

   //
   // For autoscript, we should interpret one instruction per frame (except
   // jumping) and save the address of next instruction.
   //
   switch (pScript->wOperation)
   {
   case 0x0000:
      //
      // Stop running
      //
      break;

   case 0x0001:
      //
      // Stop running and replace the entry with the next line
      //
      wScriptEntry++;
      break;

   case 0x0002:
      //
      // Stop running and replace the entry with the specified one
      //
      if (pScript->rgwOperand[1] == 0 ||
         ++(pEvtObj->wScriptIdleFrameCountAuto) < pScript->rgwOperand[1])
      {
         wScriptEntry = pScript->rgwOperand[0];
      }
      else
      {
         pEvtObj->wScriptIdleFrameCountAuto = 0;
         wScriptEntry++;
      }
      break;

   case 0x0003:
      //
      // unconditional jump
      //
      if (pScript->rgwOperand[1] == 0 ||
         ++(pEvtObj->wScriptIdleFrameCountAuto) < pScript->rgwOperand[1])
      {
         wScriptEntry = pScript->rgwOperand[0];
         goto begin;
      }
      else
      {
         pEvtObj->wScriptIdleFrameCountAuto = 0;
         wScriptEntry++;
      }
      break;

   case 0x0004:
      //
      // Call subroutine
      //
      PAL_RunTriggerScript(pScript->rgwOperand[0],
         pScript->rgwOperand[1] ? pScript->rgwOperand[1] : wEventObjectID);
      wScriptEntry++;
      break;

   case 0x0006:
      //
      // jump to the specified address by the specified rate
      //
      if (RandomLong(1, 100) >= pScript->rgwOperand[0] && pScript->rgwOperand[1] != 0)
      {
         wScriptEntry = pScript->rgwOperand[1];
         goto begin;
      }
      else
      {
         wScriptEntry++;
      }
      break;

   case 0x0009:
      //
      // Wait for a certain number of frames
      //
      if (++(pEvtObj->wScriptIdleFrameCountAuto) >= pScript->rgwOperand[0])
      {
         //
         // waiting ended; go further
         //
         pEvtObj->wScriptIdleFrameCountAuto = 0;
         wScriptEntry++;
      }
      break;

   default:
      //
      // Other operations
      //
      wScriptEntry = PAL_InterpretInstruction(wScriptEntry, wEventObjectID);
      break;
   }

   return wScriptEntry;
}
