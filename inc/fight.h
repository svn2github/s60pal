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

#ifndef FIGHT_H
#define FIGHT_H

#ifdef __cplusplus
extern "C"
{
#endif

INT
PAL_BattleSelectAutoTarget(
   VOID
);

SHORT
PAL_CalcBaseDamage(
   WORD        wAttackStrength,
   WORD        wDefense
);

SHORT
PAL_CalcMagicDamage(
   WORD             wMagicStrength,
   WORD             wDefense,
   const WORD       rgwElementalResistance[NUM_MAGIC_ELEMENTAL],
   WORD             wMagicID
);

SHORT
PAL_CalcPhysicalAttackDamage(
   WORD           wAttackStrength,
   WORD           wDefense,
   WORD           wAttackResistance
);

VOID
PAL_UpdateTimeChargingUnit(
   VOID
);

FLOAT
PAL_GetTimeChargingSpeed(
   WORD           wDexterity
);

VOID
PAL_BattleUpdateFighters(
   VOID
);

VOID
PAL_BattleStartFrame(
   VOID
);

VOID
PAL_BattleCommitAction(
   VOID
);

VOID
PAL_BattlePlayerPerformAction(
   WORD         wPlayerIndex
);

#ifdef __cplusplus
}
#endif
#endif
