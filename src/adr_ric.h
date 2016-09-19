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
  * MODUL:               adr_ric.h  /  ADR_RIC.H
  * AUTOR/DATUM:         Mathias Kettner, 25. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
// 
//	Deklariert einige kleine Klassen, hauptsaechlich ADR, die fuer
//	die Ortsangabe und fuer Ortsbeziehungen innerhalb von Landschafts-
//	objekten noetig sind.
//
// **************************************************************************

#ifndef __adr_ric_h
#define __adr_ric_h

#include <stdio.h>

#include "doublist.h"

struct ADR
{
  long x,y;
public:
  ADR(ADR& a) { x=a.x; y=a.y; };
  ADR(char *string);
  ADR(FILE *file);
  ADR() { x=y=-1; };
  ADR(long xa, long ya) { x=xa; y=ya; };
  static ADR& ausserhalb();
  char *to_string();
  void addiere(ADR& a) { x+=a.x; y+=a.y; };
  void to_file(FILE *file) { fprintf(file,to_string()); };
  short ist_ausserhalb() { return x<0 || y<0; }; // Vorsicht!! Lieber bei
			// der Landschaft ermitteln, falls wichtig!
  int operator == (ADR& a) { return x==a.x && y==a.y; };
};

struct ADR_LIST_NODE : public DOUBLIST_NODE
{
  ADR adresse;
  ADR_LIST_NODE(ADR& adr) { adresse = adr; };
  ADR_LIST_NODE(long x, long y) { adresse.x = x; adresse.y = y; };
  ADR_LIST_NODE() {};
};


struct RIC
{
  long x,y;  // offsets
public:
  RIC(char *r);
  static RIC& null();
  char *to_string();
  char *grob_to_string();
  void drehen(float); // Aendert die Richtung um einen Winkel im Gradmass
  void drehen(); // Dreht um Plus 45 Grad.
};

#endif // __adr_ric_h
