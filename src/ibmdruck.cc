/*--------------------------------------------------------------------------*\ 
*                                                                            *
|     ____                           _   _                                   |
|    |  _ \ _ __ ___  _ __ ___   ___| |_| |__   ___ _   _ ___                |
|    | |_) | '__/ _ \| '_ ` _ \ / _ \ __| '_ \ / _ \ | | / __|               |
|    |  __/| | | (_) | | | | | |  __/ |_| | | |  __/ |_| \__ \               |
|    |_|   |_|  \___/|_| |_| |_|\___|\__|_| |_|\___|\__,_|___/               |
|                                                                            |
|    A strategical game to be played via mail or email                       |
|    Copyright 1993-2003 Mathias Kettner (prometheus@mathias-kettner.de)     |
|                                                                            |
|    ====================================================================    |
|                                                                            |
|    This program is free software; you can redistribute it and/or modify    |
|    it under the terms of the GNU General Public License as published by    |
|    the Free Software Foundation; either version 2 of the License, or       |
|    (at your option) any later version.                                     |
|                                                                            |
|    This program is distributed in the hope that it will be useful,         |
|    but WITHOUT ANY WARRANTY; without even the implied warranty of          |
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           |
|    GNU General Public License for more details.                            |
|                                                                            |
|    You should have received a copy of the GNU General Public License       |
|    along with this program; if not, write to the Free Software             |
|    Foundation, Inc., 59 Temple Place, Suite 330, Boston,                   |
|    MA  02111-1307  USA                                                     |
*                                                                            *
\*--------------------------------------------------------------------------*/


/**---------------------------------------------------------------------------
  * MODUL:               IMBDRUCK.CPP
  * AUTOR/DATUM:         Mathias Kettner, 28. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Enthaelt Funktionen, die die spezifischen Besonderheiten des erwei-
//	terten IBM-Zeichensatzes beruecksichtigen.
//
// *************************************************************************

#include "drucker.h"
#include "version.h"

/**---------------------------------------------------------------------------
  * Die Konfigurationsangabe ist eine kleine Funktion, die einen Text
  * zurueckgibt, der dann auf dem Titelbildschirm ausgegeben wird.
  ---------------------------------------------------------------------------*/
char *version_zeichensatz()
{
  return "PC-8";
}


char *dr_umlaut_sequenz(char zeichen)
{
  switch(zeichen)
  {
    case 'a': return "\204";
    case 'A': return "\216";
    case 'o': return "\224";
    case 'O': return "\231";
    case 'u': return "\201";
    case 'U': return "\232";
    case 's': return "\341";
    default: return "";
  }
}
