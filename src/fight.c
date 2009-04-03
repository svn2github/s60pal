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
#include <math.h>

static BOOL
PAL_IsPlayerDying(
   WORD        wPlayerRole
)
/*++
  Purpose:

    Check if the player is dying.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    TRUE if the player is dying, FALSE if not.

--*/
{
   return (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] < 100 &&
      gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] < gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] / 5);
}

INT
PAL_BattleSelectAutoTarget(
   VOID
)
/*++
  Purpose:

    Pick an enemy target automatically.

  Parameters:

    None.

  Return value:

    The index of enemy. -1 if failed.

--*/
{
   int          i;

   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      if (g_Battle.rgEnemy[i].wObjectID != 0 &&
         g_Battle.rgEnemy[i].e.wHealth > 0)
      {
         return i;
      }
   }

   return -1;
}

SHORT
PAL_CalcBaseDamage(
   WORD        wAttackStrength,
   WORD        wDefense
)
/*++
  Purpose:

    Calculate the base damage value of attacking.

  Parameters:

    [IN]  wAttackStrength - attack strength of attacker.

    [IN]  wDefense - defense value of inflictor.

  Return value:

    The base damage value of the attacking.

--*/
{
   SHORT            sDamage;

   //
   // Formula courtesy of palxex and shenyanduxing
   //
   if (wAttackStrength > wDefense)
   {
      sDamage = (SHORT)(wAttackStrength * 2 - wDefense * 1.6 + 0.5);
   }
   else if (wAttackStrength > wDefense * 0.6)
   {
      sDamage = (SHORT)(wAttackStrength - wDefense * 0.6 + 0.5);
   }
   else
   {
      sDamage = 0;
   }

   return sDamage;
}

SHORT
PAL_CalcMagicDamage(
   WORD             wMagicStrength,
   WORD             wDefense,
   const WORD       rgwElementalResistance[NUM_MAGIC_ELEMENTAL],
   WORD             wMagicID
)
/*++
   Purpose:

     Calculate the damage of magic.

   Parameters:

     [IN]  wMagicStrength - magic strength of attacker.

     [IN]  wDefense - defense value of inflictor.

     [IN]  rgwAttribResistance - inflictor's resistance to the attributed magics.

     [IN]  wMagicID - object ID of the magic.

   Return value:

     The damage value of the magic attack.

--*/
{
   SHORT           sDamage;
   WORD            wElem;

   wMagicID = gpGlobals->g.rgObject[wMagicID].magic.wMagicNumber;

   //
   // Formula courtesy of palxex and shenyanduxing
   //
   wMagicStrength *= RandomLong(9, 11);
   wMagicStrength /= 10;

   sDamage = PAL_CalcBaseDamage(wMagicStrength, wDefense) / 2;
   sDamage += gpGlobals->g.lprgMagic[wMagicID].wBaseDamage;

   if (gpGlobals->g.lprgMagic[wMagicID].wElemental != 0)
   {
      wElem = gpGlobals->g.lprgMagic[wMagicID].wElemental - 1;

      sDamage *= 10 - rgwElementalResistance[wElem];
      sDamage /= 5;

      if (wElem < NUM_MAGIC_ELEMENTAL)
      {
         sDamage *= 10 + gpGlobals->g.lprgBattleField[gpGlobals->wNumBattleField].rgsMagicEffect[wElem];
         sDamage /= 10;
      }
   }

   return sDamage;
}

SHORT
PAL_CalcPhysicalAttackDamage(
   WORD           wAttackStrength,
   WORD           wDefense,
   WORD           wAttackResistance
)
/*++
  Purpose:

    Calculate the damage value of physical attacking.

  Parameters:

    [IN]  wAttackStrength - attack strength of attacker.

    [IN]  wDefense - defense value of inflictor.

    [IN]  wAttackResistance - inflictor's resistance to phycal attack.

  Return value:

    The damage value of the physical attacking.

--*/
{
   SHORT             sDamage;

   sDamage = PAL_CalcBaseDamage(wAttackStrength, wDefense);
   sDamage /= wAttackResistance;

   sDamage = (SHORT)(sDamage * RandomFloat(1, 1.125));

   return sDamage;
}

static SHORT
PAL_GetEnemyDexterity(
   WORD          wEnemyIndex
)
/*++
  Purpose:

    Get the dexterity value of the enemy.

  Parameters:

    [IN]  wEnemyIndex - the index of the enemy.

  Return value:

    The dexterity value of the enemy.

--*/
{
   SHORT      s;

   assert(g_Battle.rgEnemy[wEnemyIndex].wObjectID != 0);

   s = (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 3;
   s += (SHORT)g_Battle.rgEnemy[wEnemyIndex].e.wDexterity;

   if (g_Battle.rgEnemy[wEnemyIndex].rgStatus[kStatusHaste] != 0)
   {
      s *= 6;
      s /= 5;
   }
   else if (g_Battle.rgEnemy[wEnemyIndex].rgStatus[kStatusSlow] != 0)
   {
      s *= 2;
      s /= 3;
   }

   return s;
}

VOID
PAL_UpdateTimeChargingUnit(
   VOID
)
/*++
  Purpose:

    Update the base time unit of time-charging.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   g_Battle.flTimeChargingUnit = (FLOAT)(pow(PAL_GetPlayerDexterity(0) + 5, 0.3) / 3);
   g_Battle.flTimeChargingUnit /= PAL_GetPlayerDexterity(0);
}

FLOAT
PAL_GetTimeChargingSpeed(
   WORD           wDexterity
)
/*++
  Purpose:

    Calculate the time charging speed.

  Parameters:

    [IN]  wDexterity - the dexterity value of player or enemy.

  Return value:

    The time-charging speed of the player or enemy.

--*/
{
   if (g_Battle.UI.state == kBattleUISelectMove && g_Battle.UI.MenuState != kBattleMenuMain)
   {
      //
      // Pause the time when there are submenus
      //
      return 0;
   }

   if (gpGlobals->fAutoBattle)
   {
   	  //
   	  // The battle should be faster when using Auto Battle
   	  //
   	  wDexterity *= 3;
   }

   return g_Battle.flTimeChargingUnit * wDexterity;
}

static WORD
PAL_GetPlayerActualDexterity(
   WORD            wPlayerRole
)
/*++
  Purpose:

    Get player's actual dexterity value in battle.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The player's actual dexterity value.

--*/
{
   WORD wDexterity = PAL_GetPlayerDexterity(wPlayerRole);

   if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusHaste] != 0)
   {
      wDexterity *= 6;
      wDexterity /= 5;
   }
   else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSlow] != 0)
   {
      wDexterity *= 2;
      wDexterity /= 3;
   }

   if (PAL_IsPlayerDying(wPlayerRole))
   {
      //
      // player who is low of HP should be slower
      //
      wDexterity *= 2;
      wDexterity /= 3;
   }

   return wDexterity;
}

static VOID
PAL_BattleDelay(
   WORD       wDuration,
   WORD       wObjectID
)
/*++
  Purpose:

    Delay a while during battle.

  Parameters:

    [IN]  wDuration - Number of frames of the delay.

    [IN]  wObjectID - The object ID to be displayed during the delay. 

  Return value:

    None.

--*/
{
   int    i, j;
   DWORD  dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

   for (i = 0; i < wDuration; i++)
   {
      //
      // Update the gesture of enemies.
      //
      for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
      {
         if (g_Battle.rgEnemy[j].wObjectID == 0)
         {
            continue;
         }

         if (--g_Battle.rgEnemy[j].e.wIdleAnimSpeed == 0)
         {
            g_Battle.rgEnemy[j].wCurrentFrame++;
            g_Battle.rgEnemy[j].e.wIdleAnimSpeed =
               gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[g_Battle.rgEnemy[j].wObjectID].enemy.wEnemyID].wIdleAnimSpeed;
         }

         if (g_Battle.rgEnemy[j].wCurrentFrame >= g_Battle.rgEnemy[j].e.wIdleFrames)
         {
            g_Battle.rgEnemy[j].wCurrentFrame = 0;
         }
      }

      //
      // Clear the input state of previous frame.
      //
      PAL_ClearKeyState();

      //
      // Wait for the time of one frame. Accept input here.
      //
      PAL_ProcessEvent();
      while (SDL_GetTicks() <= dwTime)
      {
         PAL_ProcessEvent();
         SDL_Delay(1);
      }

      //
      // Set the time of the next frame.
      //
      dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

      PAL_BattleMakeScene();
      SDL_BlitSurface(g_Battle.lpSceneBuf, NULL, gpScreen, NULL);
      PAL_BattleUIUpdate();

      if (wObjectID != 0)
      {
         PAL_DrawText(PAL_GetWord(wObjectID), PAL_XY(210, 50),
            15, TRUE, FALSE);
      }

      VIDEO_UpdateScreen(NULL);
   }
}

static VOID
PAL_BattleBackupStat(
   VOID
)
/*++
  Purpose:

    Backup HP and MP values of all players and enemies.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i;
   WORD wPlayerRole;

   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      if (g_Battle.rgEnemy[i].wObjectID == 0)
      {
         continue;
      }
      g_Battle.rgEnemy[i].wPrevHP = g_Battle.rgEnemy[i].e.wHealth;
   }

   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

      g_Battle.rgPlayer[i].wPrevHP =
         gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole];
      g_Battle.rgPlayer[i].wPrevMP =
         gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole];
   }
}

static BOOL
PAL_BattleDisplayStatChange(
   VOID
)
/*++
  Purpose:

    Display the HP and MP changes of all players and enemies.

  Parameters:

    None.

  Return value:

    TRUE if there are any number displayed, FALSE if not.

--*/
{
   int i, x, y;
   SHORT sDamage;
   WORD wPlayerRole;
   BOOL f = FALSE;

   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      if (g_Battle.rgEnemy[i].wObjectID == 0)
      {
         continue;
      }

      if (g_Battle.rgEnemy[i].wPrevHP != g_Battle.rgEnemy[i].e.wHealth)
      {
         //
         // Show the number of damage
         //
         sDamage = g_Battle.rgEnemy[i].e.wHealth - g_Battle.rgEnemy[i].wPrevHP;
 
         x = PAL_X(g_Battle.rgEnemy[i].pos);
         y = PAL_Y(g_Battle.rgEnemy[i].pos) - 70;

         x += PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame)) / 2;
         y += PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgEnemy[i].lpSprite, g_Battle.rgEnemy[i].wCurrentFrame)) / 2;

         if (y < 10)
         {
            y = 10;
         }

         if (sDamage < 0)
         {
            PAL_BattleUIShowNum((WORD)(-sDamage), PAL_XY(x, y), kNumColorBlue);
         }
         else
         {
            PAL_BattleUIShowNum((WORD)(sDamage), PAL_XY(x, y), kNumColorYellow);
         }

         f = TRUE;
      }
   }

   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

      if (g_Battle.rgPlayer[i].wPrevHP != gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole])
      {
         sDamage =
            gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] - g_Battle.rgPlayer[i].wPrevHP;

         x = PAL_X(g_Battle.rgPlayer[i].pos);
         y = PAL_Y(g_Battle.rgPlayer[i].pos) - 70;

         x += PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame)) / 2;
         y += PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame));

         if (y < 10)
         {
            y = 10;
         }

         if (sDamage < 0)
         {
            PAL_BattleUIShowNum((WORD)(-sDamage), PAL_XY(x, y), kNumColorBlue);
         }
         else
         {
            PAL_BattleUIShowNum((WORD)(sDamage), PAL_XY(x, y), kNumColorYellow);
         }

         f = TRUE;
      }

      if (g_Battle.rgPlayer[i].wPrevMP != gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole])
      {
         sDamage =
            gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] - g_Battle.rgPlayer[i].wPrevMP;

         x = PAL_X(g_Battle.rgPlayer[i].pos);
         y = PAL_Y(g_Battle.rgPlayer[i].pos) - 62;

         x += PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame)) / 2;
         y += PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgPlayer[i].lpSprite, g_Battle.rgPlayer[i].wCurrentFrame));

         if (y < 10)
         {
            y = 10;
         }

         if (sDamage < 0)
         {
            PAL_BattleUIShowNum((WORD)(-sDamage), PAL_XY(x, y), kNumColorBlue);
         }
         else
         {
            PAL_BattleUIShowNum((WORD)(sDamage), PAL_XY(x, y), kNumColorCyan);
         }

         f = TRUE;
      }
   }

   return f;
}

static VOID
PAL_BattlePostActionCheck(
   BOOL      fCheckPlayers
)
/*++
  Purpose:

    Essential checks after an action is executed.

  Parameters:

    [IN]  fCheckPlayers - TRUE if check for players, FALSE if not.

  Return value:

    None.

--*/
{
   int      i;
   BOOL     fFade = FALSE;

   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      if (g_Battle.rgEnemy[i].wObjectID == 0)
      {
         continue;
      }

      if (g_Battle.rgEnemy[i].e.wHealth == 0)
      {
         //
         // This enemy is KO'ed
         //
         SOUND_Play(g_Battle.rgEnemy[i].e.wDeathSound);
         g_Battle.rgEnemy[i].wObjectID = 0;
         fFade = TRUE;
      }
   }

   if (fCheckPlayers)
   {
   }

   if (fFade)
   {
      PAL_BattleBackupScene();
      PAL_BattleMakeScene();
      PAL_BattleFadeScene();
   }
}

VOID
PAL_BattleUpdateFighters(
   VOID
)
/*++
  Purpose:

    Update players' and enemies' gestures and locations in battle.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int        i;
   WORD       wPlayerRole;

   //
   // Update the gesture for all players
   //
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

      g_Battle.rgPlayer[i].pos = g_Battle.rgPlayer[i].posOriginal;
      g_Battle.rgPlayer[i].iColorShift = 0;

      if (g_Battle.rgPlayer[i].state == kFighterAct &&
         g_Battle.rgPlayer[i].action.ActionType == kBattleActionMagic)
      {
         //
         // Player is using a magic
         //
         g_Battle.rgPlayer[i].wCurrentFrame = 5;
         continue;
      }

      if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
      {
         if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] == 0)
         {
            g_Battle.rgPlayer[i].wCurrentFrame = 2;
         }
         else
         {
            g_Battle.rgPlayer[i].wCurrentFrame = 0;
         }
      }
      else if (PAL_IsPlayerDying(wPlayerRole) ||
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] != 0)
      {
         g_Battle.rgPlayer[i].wCurrentFrame = 1;
      }
      else if (g_Battle.rgPlayer[i].fDefending)
      {
         g_Battle.rgPlayer[i].wCurrentFrame = 3;
      }
      else
      {
         g_Battle.rgPlayer[i].wCurrentFrame = 0;
      }
   }

   //
   // Update the gesture for all enemies
   //
   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      if (g_Battle.rgEnemy[i].wObjectID == 0)
      {
         continue;
      }

      g_Battle.rgEnemy[i].pos = g_Battle.rgEnemy[i].posOriginal;
      g_Battle.rgEnemy[i].iColorShift = 0;

      if (g_Battle.rgEnemy[i].rgStatus[kStatusSleep] > 0)
      {
         g_Battle.rgEnemy[i].wCurrentFrame = 0;
         continue;
      }

      if (--g_Battle.rgEnemy[i].e.wIdleAnimSpeed == 0)
      {
         g_Battle.rgEnemy[i].wCurrentFrame++;
         g_Battle.rgEnemy[i].e.wIdleAnimSpeed =
            gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[g_Battle.rgEnemy[i].wObjectID].enemy.wEnemyID].wIdleAnimSpeed;
      }

      if (g_Battle.rgEnemy[i].wCurrentFrame >= g_Battle.rgEnemy[i].e.wIdleFrames)
      {
         g_Battle.rgEnemy[i].wCurrentFrame = 0;
      }
   }
}

VOID
PAL_BattleStartFrame(
   VOID
)
/*++
  Purpose:

    Called once per video frame in battle.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int                      i;
   int                      iMax;
   BOOL                     fEnded;
   WORD                     wPlayerRole;
   WORD                     wDexterity;
   FLOAT                    flMax;

   PAL_BattleUpdateFighters();

   //
   // Update the scene
   //
   PAL_BattleMakeScene();
   SDL_BlitSurface(g_Battle.lpSceneBuf, NULL, gpScreen, NULL);

   //
   // Check if the battle is over
   //
   fEnded = TRUE;

   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      if (g_Battle.rgEnemy[i].wObjectID != 0)
      {
         fEnded = FALSE;
         break;
      }
   }

   if (fEnded)
   {
      //
      // All enemies are cleared. Won the battle.
      //
      g_Battle.BattleResult = kBattleResultWon;
      SOUND_Play(-1);
      return;
   }
   else
   {
      fEnded = TRUE;

      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;
         if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] != 0 ||
            gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] != 0)
         {
            fEnded = FALSE;
            break;
         }
      }

      if (fEnded)
      {
         //
         // All players are dead. Lost the battle.
         //
         g_Battle.BattleResult = kBattleResultLost;
         return;
      }
   }

   //
   // Run the logic for all enemies
   //
   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      if (g_Battle.rgEnemy[i].wObjectID == 0)
      {
         continue;
      }

      if (g_Battle.rgEnemy[i].fTurnStart)
      {
         g_Battle.rgEnemy[i].wScriptOnTurnStart =
            PAL_RunTriggerScript(g_Battle.rgEnemy[i].wScriptOnTurnStart, i);

         g_Battle.rgEnemy[i].fTurnStart = FALSE;
      }
   }

   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      if (g_Battle.rgEnemy[i].wObjectID == 0)
      {
         continue;
      }

      switch (g_Battle.rgEnemy[i].state)
      {
      case kFighterWait:
         flMax = PAL_GetTimeChargingSpeed(PAL_GetEnemyDexterity(i));

         if (flMax != 0)
         {
            g_Battle.rgEnemy[i].flTimeMeter += flMax;

            if (g_Battle.rgEnemy[i].flTimeMeter > 100 && flMax > 0)
            {
               g_Battle.rgEnemy[i].state = kFighterCom;
            }
         }
         break;

      case kFighterCom:
         g_Battle.rgEnemy[i].wScriptOnReady =
            PAL_RunTriggerScript(g_Battle.rgEnemy[i].wScriptOnReady, i);
         g_Battle.rgEnemy[i].state = kFighterAct;
         break;

      case kFighterAct:
////TEST///////////////////////////////////////////////////////////
SOUND_Play(g_Battle.rgEnemy[i].e.wAttackSound);
if (g_Battle.rgEnemy[i].fFirstMoveDone)
PAL_BattleUIShowText(va("enemy %d attack (2nd)",i), 500);
else PAL_BattleUIShowText(va("enemy %d attack",i), 500);
g_Battle.rgEnemy[i].flTimeMeter =0;
g_Battle.rgEnemy[i].state = kFighterWait;
////////////////////////////////////////////////////////////////////
         if (!g_Battle.rgEnemy[i].fFirstMoveDone)
         {
            if (g_Battle.rgEnemy[i].e.wDualMove >= 2 ||
               (g_Battle.rgEnemy[i].e.wDualMove != 0 && RandomLong(0, 1)))
            {
               g_Battle.rgEnemy[i].flTimeMeter = 100;
               g_Battle.rgEnemy[i].state = kFighterWait;
               g_Battle.rgEnemy[i].fFirstMoveDone = TRUE;
               break;
            }
         }

         g_Battle.rgEnemy[i].fFirstMoveDone = FALSE;
         g_Battle.rgEnemy[i].fTurnStart = TRUE;
         break;
      }
   }

   //
   // Update the battle UI
   //
   PAL_BattleUIUpdate();

   //
   // Run the logic for all players
   //
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

      //
      // Skip dead players
      //
      if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 &&
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] == 0)
      {
         g_Battle.rgPlayer[i].state = kFighterWait;
         g_Battle.rgPlayer[i].flTimeMeter = 0;
         continue;
      }

      switch (g_Battle.rgPlayer[i].state)
      {
      case kFighterWait:
         wDexterity = PAL_GetPlayerActualDexterity(wPlayerRole);
         g_Battle.rgPlayer[i].flTimeMeter +=
            PAL_GetTimeChargingSpeed(wDexterity) * g_Battle.rgPlayer[i].flTimeSpeedModifier;
         break;

      case kFighterCom:
         if (g_Battle.UI.state == kBattleUIWait)
         {
            PAL_BattleUIPlayerReady(i);
         }
         break;

      case kFighterAct:
         wDexterity = PAL_GetPlayerActualDexterity(wPlayerRole);
         g_Battle.rgPlayer[i].action.flRemainingTime -= PAL_GetTimeChargingSpeed(wDexterity);

         if (g_Battle.rgPlayer[i].action.flRemainingTime < 0)
         {
            //
            // Perform the action for this player.
            //
            PAL_BattlePlayerPerformAction(i);

            //
            // Reduce the time for other players when uses coopmagic
            //
            if (g_Battle.rgPlayer[i].action.ActionType == kBattleActionCoopMagic)
            {
               for (iMax = 0; iMax <= gpGlobals->wMaxPartyMemberIndex; iMax++)
               {
                  g_Battle.rgPlayer[iMax].flTimeMeter = 0;
                  g_Battle.rgPlayer[iMax].flTimeSpeedModifier = 2.0f;
               }
            }
            else
            {
               g_Battle.rgPlayer[i].flTimeMeter = 0;
            }

            //
            // Revert this player back to waiting state.
            //
            g_Battle.rgPlayer[i].state = kFighterWait;
            g_Battle.rgPlayer[i].flTimeSpeedModifier = 1.0f;
         }
         break;
      }
   }

   //
   // Start the UI for the fastest and ready player
   //
   flMax = 0;
   iMax = 0;

   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      if (g_Battle.rgPlayer[i].state == kFighterCom ||
         (g_Battle.rgPlayer[i].state == kFighterAct && g_Battle.rgPlayer[i].action.ActionType == kBattleActionCoopMagic))
      {
         flMax = 0;
         break;
      }
      else if (g_Battle.rgPlayer[i].state == kFighterWait)
      {
         if (g_Battle.rgPlayer[i].flTimeMeter > flMax)
         {
            iMax = i;
            flMax = g_Battle.rgPlayer[i].flTimeMeter;
         }
      }
   }

   if (flMax > 100.0f)
   {
      g_Battle.rgPlayer[iMax].state = kFighterCom;
      g_Battle.rgPlayer[iMax].fDefending = FALSE;
   }

////TEST/////////////////////////////////////////////////////////-START
if (g_InputState.dwKeyPress & kKeyFlee){
    for (i = 0; i < 5; i++){
        gpGlobals->g.PlayerRoles.rgwHP[i] = 0;
    }
   return;
}
////TEST/////////////////////////////////////////////////////////-END
}

VOID
PAL_BattleCommitAction(
   VOID
)
/*++
  Purpose:

    Commit the action which the player decided.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   WORD      w;

   g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType =
      g_Battle.UI.wActionType;
   g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget =
      (SHORT)g_Battle.UI.wSelectedIndex;
   g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID =
      g_Battle.UI.wObjectID;

   //
   // Check if the action is valid
   //
   switch (g_Battle.UI.wActionType)
   {
   case kBattleActionMagic:
      w = gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[g_Battle.UI.wObjectID].magic.wMagicNumber].wCostMP;
      if (gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole] < w)
      {
         w = gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[g_Battle.UI.wObjectID].magic.wMagicNumber].wType;
         if (w == kMagicTypeApplyToPlayer || w == kMagicTypeApplyToParty ||
            w == kMagicTypeTransform)
         {
            g_Battle.UI.wActionType = kBattleActionDefend;
         }
         else
         {
            g_Battle.UI.wActionType = kBattleActionAttack;
            if (g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget == -1)
            {
               g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget = 0;
            }
         }
      }
      break;

   case kBattleActionThrowItem:
   case kBattleActionUseItem:
      break;
   }

   //
   // Calculate the waiting time for the action
   //
   switch (g_Battle.UI.wActionType)
   {
   case kBattleActionMagic:
      {
         LPMAGIC      p;
         WORD         wCostMP;

         //
         // The base casting time of magic is set to the MP costed
         //
         p = &(gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[g_Battle.UI.wObjectID].magic.wMagicNumber]);
         wCostMP = p->wCostMP;

         if (wCostMP == 1)
         {
            if (p->wType == kMagicTypeSummon)
            {
               //
               // The Wine God is an ultimate move which should take long
               //
               wCostMP = 200;
            }
         }
         else if (p->wType == kMagicTypeApplyToPlayer || p->wType == kMagicTypeApplyToParty ||
            p->wType == kMagicTypeTransform)
         {
            //
            // Healing magics should take shorter
            //
            wCostMP /= 3;
         }

         g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.flRemainingTime = wCostMP;
      }
      break;

   case kBattleActionFlee:
      //
      // Fleeing should take a fairly long time
      //
      g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.flRemainingTime = RandomFloat(25, 75);
      break;

   case kBattleActionDefend:
      //
      // Defend takes no time
      //
      g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.flRemainingTime = 0;
      break;

   default:
      g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.flRemainingTime = 5;
      break;
   }

   g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].state = kFighterAct;
   g_Battle.UI.state = kBattleUIWait;
}

static VOID
PAL_BattleShowPlayerAttackAnim(
   WORD        wPlayerIndex,
   BOOL        fCritical
)
/*++
  Purpose:

    Show the physical attack effect for player.

  Parameters:

    [IN]  wPlayerIndex - the index of the player.

    [IN]  fCritical - TRUE if this is a critical hit.

  Return value:

    None.

--*/
{
   WORD wPlayerRole = gpGlobals->rgParty[wPlayerIndex].wPlayerRole;
   SHORT sTarget = g_Battle.rgPlayer[wPlayerIndex].action.sTarget;

   int index, i, j;
   int enemy_x = 0, enemy_y = 0, enemy_h = 0, x, y, dist = 0;
   int w, h;

   DWORD dwTime;

   if (sTarget != -1)
   {
      enemy_x = PAL_X(g_Battle.rgEnemy[sTarget].pos);
      enemy_y = PAL_Y(g_Battle.rgEnemy[sTarget].pos);

      enemy_x += PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgEnemy[sTarget].lpSprite, g_Battle.rgEnemy[sTarget].wCurrentFrame)) / 2;
      enemy_h = PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgEnemy[sTarget].lpSprite, g_Battle.rgEnemy[sTarget].wCurrentFrame));
      enemy_y += enemy_h;

      if (sTarget >= 3)
      {
         dist = (sTarget - wPlayerIndex) * 8;
      }
   }
   else
   {
      enemy_x = 150;
      enemy_y = 100;
   }

   index = gpGlobals->g.rgwBattleEffectIndex[gpGlobals->g.PlayerRoles.rgwSpriteNumInBattle[wPlayerRole]][1];
   index *= 3;

   //
   // Play the attack voice
   //
   if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
   {
      if (!fCritical)
      {
         SOUND_Play(gpGlobals->g.PlayerRoles.rgwAttackSound[wPlayerRole]);
      }
      else
      {
         SOUND_Play(gpGlobals->g.PlayerRoles.rgwCriticalSound[wPlayerRole]);
      }
   }

   //
   // Show the animation
   //
   x = enemy_x - dist + 64;
   y = enemy_y + dist + 20;

   g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 8;
   w = PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgPlayer[wPlayerIndex].lpSprite, 8));
   h = PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgPlayer[wPlayerIndex].lpSprite, 8));
   g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x - w / 2, y - h);

   PAL_BattleDelay(2, 0);

   x -= 10;
   y -= 2;
   g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x - w / 2, y - h);

   PAL_BattleDelay(1, 0);

   g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 9;
   x -= 16;
   y -= 4;

   SOUND_Play(gpGlobals->g.PlayerRoles.rgwWeaponSound[wPlayerRole]);

   x = enemy_x;
   y = enemy_y - enemy_h / 3 + 10;

   dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

   for (i = 0; i < 3; i++)
   {
      LPCBITMAPRLE b = PAL_SpriteGetFrame(g_Battle.lpEffectSprite, index++);

      //
      // Clear the input state of previous frame.
      //
      PAL_ClearKeyState();

      //
      // Wait for the time of one frame. Accept input here.
      //
      PAL_ProcessEvent();
      while (SDL_GetTicks() <= dwTime)
      {
         PAL_ProcessEvent();
         SDL_Delay(1);
      }

      //
      // Set the time of the next frame.
      //
      dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

      //
      // Update the gesture of enemies.
      //
      for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
      {
         if (g_Battle.rgEnemy[j].wObjectID == 0)
         {
            continue;
         }

         if (--g_Battle.rgEnemy[j].e.wIdleAnimSpeed == 0)
         {
            g_Battle.rgEnemy[j].wCurrentFrame++;
            g_Battle.rgEnemy[j].e.wIdleAnimSpeed =
               gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[g_Battle.rgEnemy[j].wObjectID].enemy.wEnemyID].wIdleAnimSpeed;
         }

         if (g_Battle.rgEnemy[j].wCurrentFrame >= g_Battle.rgEnemy[j].e.wIdleFrames)
         {
            g_Battle.rgEnemy[j].wCurrentFrame = 0;
         }
      }

      PAL_BattleMakeScene();
      SDL_BlitSurface(g_Battle.lpSceneBuf, NULL, gpScreen, NULL);

      PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));
      x -= 16;
      y += 16;

      PAL_BattleUIUpdate();

      if (i == 0)
      {
         if (sTarget == -1)
         {
            for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
            {
               g_Battle.rgEnemy[j].iColorShift = 6;
            }
         }
         else
         {
            g_Battle.rgEnemy[sTarget].iColorShift = 6;
         }

         //
         // Flash the screen if it's a critical hit
         //
         if (fCritical)
         {
            SDL_FillRect(gpScreen, NULL, 15);
         }
      }

      VIDEO_UpdateScreen(NULL);

      if (i == 1)
      {
         g_Battle.rgPlayer[wPlayerIndex].pos =
            PAL_XY(PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos) + 2,
                   PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos) + 1);
      }
   }

   dist = 8;

   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      g_Battle.rgEnemy[i].iColorShift = 0;
   }

   if (sTarget == -1)
   {
      for (i = 0; i < 3; i++)
      {
         for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
         {
            x = PAL_X(g_Battle.rgEnemy[j].pos);
            y = PAL_Y(g_Battle.rgEnemy[j].pos);

            x -= dist;
            y -= dist / 2;
            g_Battle.rgEnemy[j].pos = PAL_XY(x, y);
         }

         PAL_BattleDelay(1, 0);
         dist /= -2;
      }
   }
   else
   {
      x = PAL_X(g_Battle.rgEnemy[sTarget].pos);
      y = PAL_Y(g_Battle.rgEnemy[sTarget].pos);

      for (i = 0; i < 3; i++)
      {
         x -= dist;
         dist /= -2;
         y += dist;
         g_Battle.rgEnemy[sTarget].pos = PAL_XY(x, y);

         PAL_BattleDelay(1, 0);
      }
   }
}

static VOID
PAL_BattleShowPlayerUseItemAnim(
   WORD         wPlayerIndex,
   WORD         wObjectID,
   SHORT        sTarget
)
/*++
  Purpose:

    Show the "use item" effect for player.

  Parameters:

    [IN]  wPlayerIndex - the index of the player.

    [IN]  wObjectID - the object ID of the item to be used.

    [IN]  sTarget - the target player of the action.

  Return value:

    None.

--*/
{
   int i, j;

   PAL_BattleDelay(4, 0);

   g_Battle.rgPlayer[wPlayerIndex].pos =
      PAL_XY(PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos) - 15,
             PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos) - 7);

   g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 5;

   SOUND_Play(28);

   for (i = 0; i <= 6; i++)
   {
      if (sTarget == -1)
      {
         for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
         {
            g_Battle.rgPlayer[j].iColorShift = i;
         }
      }
      else
      {
         g_Battle.rgPlayer[sTarget].iColorShift = i;
      }

      PAL_BattleDelay(1, wObjectID);
   }

   for (i = 5; i >= 0; i--)
   {
      if (sTarget == -1)
      {
         for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
         {
            g_Battle.rgPlayer[j].iColorShift = i;
         }
      }
      else
      {
         g_Battle.rgPlayer[sTarget].iColorShift = i;
      }

      PAL_BattleDelay(1, wObjectID);
   }
}

static VOID
PAL_BattlePlayerValidateAction(
   WORD         wPlayerIndex
)
/*++
  Purpose:

    Validate player's action, fallback to other action when needed.

  Parameters:

    [IN]  wPlayerIndex - the index of the player.

  Return value:

    None.

--*/
{
   // TODO
   switch (g_Battle.rgPlayer[wPlayerIndex].action.ActionType)
   {
   }
}

VOID
PAL_BattlePlayerPerformAction(
   WORD         wPlayerIndex
)
/*++
  Purpose:

    Perform the selected action for a player.

  Parameters:

    [IN]  wPlayerIndex - the index of the player.

  Return value:

    None.

--*/
{
   // TODO
   SHORT    sDamage;
   WORD     wPlayerRole = gpGlobals->rgParty[wPlayerIndex].wPlayerRole;
   SHORT    sTarget = g_Battle.rgPlayer[wPlayerIndex].action.sTarget;
   int      x, y;
   int      i, t;
   WORD     str, def, res, wObject;
   BOOL     fCritical, fPoisoned;

   PAL_BattlePlayerValidateAction(wPlayerIndex);

   PAL_BattleBackupStat();

   switch (g_Battle.rgPlayer[wPlayerIndex].action.ActionType)
   {
   case kBattleActionAttack:
      if (sTarget != -1)
      {
         //
         // Attack one enemy
         //
         if (g_Battle.rgEnemy[sTarget].wObjectID == 0)
         {
            sTarget = PAL_BattleSelectAutoTarget();
            g_Battle.rgPlayer[wPlayerIndex].action.sTarget = sTarget;
         }

         for (t = 0; t < (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] ? 2 : 1); t++)
         {
            str = PAL_GetPlayerAttackStrength(wPlayerRole);
            def = g_Battle.rgEnemy[sTarget].e.wDefense;
            def += (g_Battle.rgEnemy[sTarget].e.wLevel + 6) * 4;
            res = g_Battle.rgEnemy[sTarget].e.wAttackResistance;
            fCritical = FALSE;

            sDamage = PAL_CalcPhysicalAttackDamage(str, def, res);

            if (RandomLong(0, 5) == 0)
            {
               //
               // Critical Hit
               //
               sDamage *= 3;
               fCritical = TRUE;
            }

            if (wPlayerRole == 0 && RandomLong(0, 11) == 0)
            {
               //
               // Bonus hit for Li Xiaoyao
               //
               sDamage *= 2;
               fCritical = TRUE;
            }

            if (sDamage <= 0)
            {
               sDamage = 1;
            }

            if (g_Battle.rgEnemy[sTarget].e.wHealth > (WORD)sDamage)
            {
               g_Battle.rgEnemy[sTarget].e.wHealth -= sDamage;
            }
            else
            {
               g_Battle.rgEnemy[sTarget].e.wHealth = 0;
            }

            if (t == 0)
            {
               g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 7;
               PAL_BattleDelay(4, 0);
            }

            PAL_BattleShowPlayerAttackAnim(wPlayerIndex, fCritical);

            //
            // Show the number of damage
            //
            x = PAL_X(g_Battle.rgEnemy[sTarget].pos);
            y = PAL_Y(g_Battle.rgEnemy[sTarget].pos) - 70;

            x += PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgEnemy[sTarget].lpSprite, g_Battle.rgEnemy[sTarget].wCurrentFrame)) / 2;
            y += PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgEnemy[sTarget].lpSprite, g_Battle.rgEnemy[sTarget].wCurrentFrame)) / 2;

            if (y < 10)
            {
               y = 10;
            }

            PAL_BattleUIShowNum((WORD)sDamage, PAL_XY(x, y), kNumColorBlue);
         }
      }
      else
      {
         //
         // Attack all enemies
         //
         for (t = 0; t < (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] ? 2 : 1); t++)
         {
            int division = 1;
            const int index[MAX_ENEMIES_IN_TEAM] = {2, 1, 0, 4, 3};

            fCritical = (RandomLong(0, 5) == 0);

            if (t == 0)
            {
               g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 7;
               PAL_BattleDelay(4, 0);
            }

            PAL_BattleShowPlayerAttackAnim(wPlayerIndex, fCritical);

            for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
            {
               if (g_Battle.rgEnemy[index[i]].wObjectID == 0 ||
                  index[i] > g_Battle.wMaxEnemyIndex)
               {
                  continue;
               }

               str = PAL_GetPlayerAttackStrength(wPlayerRole);
               def = g_Battle.rgEnemy[index[i]].e.wDefense;
               def += (g_Battle.rgEnemy[index[i]].e.wLevel + 6) * 4;
               res = g_Battle.rgEnemy[index[i]].e.wAttackResistance;

               sDamage = PAL_CalcPhysicalAttackDamage(str, def, res);
               if (fCritical)
               {
                  //
                  // Critical Hit
                  //
                  sDamage *= 3;
               }

               sDamage /= division;

               if (sDamage <= 0)
               {
                  sDamage = 1;
               }

               if (g_Battle.rgEnemy[index[i]].e.wHealth > (WORD)sDamage)
               {
                  g_Battle.rgEnemy[index[i]].e.wHealth -= sDamage;
               }
               else
               {
                  g_Battle.rgEnemy[index[i]].e.wHealth = 0;
               }

               //
               // Show the number of damage
               //
               x = PAL_X(g_Battle.rgEnemy[index[i]].pos);
               y = PAL_Y(g_Battle.rgEnemy[index[i]].pos) - 70;

               x += PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle.rgEnemy[index[i]].lpSprite, g_Battle.rgEnemy[index[i]].wCurrentFrame)) / 2;
               y += PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgEnemy[index[i]].lpSprite, g_Battle.rgEnemy[index[i]].wCurrentFrame)) / 2;

               if (y < 10)
               {
                  y = 10;
               }

               PAL_BattleUIShowNum((WORD)sDamage, PAL_XY(x, y), kNumColorBlue);

               division++;
               if (division > 3)
               {
                  division = 3;
               }
            }
         }
      }

      PAL_BattleUpdateFighters();
      PAL_BattleMakeScene();
      PAL_BattleDelay(5, 0);
      break;

   case kBattleActionAttackMate:
      break;

   case kBattleActionCoopMagic:
      break;

   case kBattleActionDefend:
      g_Battle.rgPlayer[wPlayerIndex].fDefending = TRUE;
      break;

   case kBattleActionFlee:
      break;

   case kBattleActionMagic:
      break;

   case kBattleActionThrowItem:
      break;

   case kBattleActionUseItem:
      wObject = g_Battle.rgPlayer[wPlayerIndex].action.wActionID;

      PAL_BattleShowPlayerUseItemAnim(wPlayerIndex, wObject, sTarget);

      //
      // Run the script
      //
      gpGlobals->g.rgObject[wObject].item.wScriptOnUse =
         PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].item.wScriptOnUse,
            (sTarget == -1) ? 0xFFFF : gpGlobals->rgParty[sTarget].wPlayerRole);

      //
      // Remove the item if the item is consuming and the script succeeded
      //
      if ((gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagConsuming) &&
         g_fScriptSuccess)
      {
         PAL_AddItemToInventory(wObject, -1);
      }

      PAL_BattleUpdateFighters();
      PAL_BattleDisplayStatChange();
      PAL_BattleDelay(8, 0);
      break;

   case kBattleActionPass:
      break;
   }

   PAL_BattlePostActionCheck(FALSE);

   //
   // Check for poisons
   //
   fPoisoned = FALSE;
   PAL_BattleBackupStat();

   for (i = 0; i < MAX_POISONS; i++)
   {
      wObject = gpGlobals->rgPoisonStatus[i][wPlayerIndex].wPoisonID;

      if (wObject != 0)
      {
         fPoisoned = TRUE;
         gpGlobals->rgPoisonStatus[i][wPlayerIndex].wPoisonScript =
            PAL_RunTriggerScript(gpGlobals->rgPoisonStatus[i][wPlayerIndex].wPoisonScript, wPlayerRole);
      }
   }

   if (fPoisoned)
   {
      PAL_BattleDelay(3, 0);
      PAL_BattleUpdateFighters();
      if (PAL_BattleDisplayStatChange())
      {
         PAL_BattleDelay(12, 0);
      }
   }

   //
   // Update statuses
   //
   for (i = 0; i < kStatusAll; i++)
   {
      if (gpGlobals->rgPlayerStatus[wPlayerRole][i] > 0)
      {
         gpGlobals->rgPlayerStatus[wPlayerRole][i]--;
      }
   }
}
