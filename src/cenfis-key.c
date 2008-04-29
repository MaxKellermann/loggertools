/*
 * loggertools
 * Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "cenfis-crypto.h"

unsigned char cenfis_key_1[] =
    "_)[8\\}=.>,?/+-0\\&)!/\"$%)=?!~])!2[3)6~+?!/}&%,&!<8$m.>,?/+90\\&)!/\"$%)0="
    "$)!&}[]$?+~/\\}[&%%(0.>,?/+-0\\&)!/~$%)=+?\\/$}][?)+=!\\/5&%7>,?/+-0\\&)!/)"
    "_2}.>,?/+-0\\&)!/\"$%)=-?!\"$)5$)?=&?!/7._%0$<9>%}][#%&4.9.5?&56$&%]/%59?";

unsigned char cenfis_key_2[] =
    "hvnxeyvoltampermftspannungstrommagnetkondensatorspuletechnischdigitals"
    "nordensuedenwestenostenzenitkepplerephemeriedatenalmanacgpsdatumunduhr"
    "differentialgpscarrierphasetimetofirstfixcruisgroundspeedtrueairspeedx";

unsigned char cenfis_key_3[] =
    "IBINSKASERMANDLMITSCHWARZGUNGATEAUGENUNDDEMRUASIGEMGSICHTUNDWENNIDIEIN"
    "DIEKRALLENHANTDANNHILFTDIRKOANKAPULIERELUNDNICHTSGEWEICHTESWOHLAUFDERH"
    "UETTINGERALMDABINIDERHOAMUNDWENNDUDESLISCHTTRIFFTDIDERBLITZBEIMSCEISEN";
