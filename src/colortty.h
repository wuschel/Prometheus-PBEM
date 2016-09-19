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
  * MODUL:               colortty.h
  * AUTOR/DATUM:         Mathias Kettner, 24. November 1995
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Enthaelt die Funktionen io_fgcolour() und io_bgcolour() zum
//	zetzen der Schriftfarbe fuer die Bildschirmausgabe. Enthaelt
//	Code und darf daher nur einmal included werden.
//
// *************************************************************************

void io_fgcolour(int colour)
{
  char *cstring;
  switch (colour) {
    case IO_BLACK:   cstring = "\033[30m"; break;
    case IO_RED:     cstring = "\033[31m"; break;
    case IO_GREEN:   cstring = "\033[32m"; break;
    case IO_YELLOW:  cstring = "\033[33m"; break;
    case IO_BLUE:    cstring = "\033[34m"; break;
    case IO_MAGENTA: cstring = "\033[35m"; break;
    case IO_CYAN:    cstring = "\033[36m"; break;

    case IO_NORMALCOLOR:
    case IO_WHITE:   cstring = "\033[37m"; break;
    default: return;
  }
//  io_print(cstring);
}

void io_bgcolour(int colour)
{
  char *cstring ;
  switch (colour) {
    case IO_BLACK:   cstring = "\033[40m"; break;
    case IO_RED:     cstring = "\033[41m"; break;
    case IO_GREEN:   cstring = "\033[42m"; break;
    case IO_YELLOW:  cstring = "\033[43m"; break;
    case IO_BLUE:    cstring = "\033[44m"; break;
    case IO_MAGENTA: cstring = "\033[45m"; break;
    case IO_CYAN:    cstring = "\033[46m"; break;

    case IO_NORMALCOLOR:
    case IO_WHITE:   cstring = "\033[47m"; break;
    default: return;
  }
//  io_print(cstring);
}

