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


#ifndef __adrindex_h
#define __adrindex_h

#include <stdlib.h>
#include <math.h>

#include "alg.h"
#include "adr_ric.h"
#include "objekt.h"

#define ADRINDEX_ENTRY_NIL NULL

class ADRINDEX_ENTRY {
  ADRINDEX_ENTRY *next;
  OBJEKT *objekt;
  friend class ADRINDEX;
  ADRINDEX_ENTRY(OBJEKT *o=NULL, ADRINDEX_ENTRY *n=NULL) { objekt=o; next=n; };
  void deleteChain() { if (next) { next->deleteChain(); delete next; } };
};

class ADRINDEX 
{
  int welt_breite, welt_hoehe; 
  ADRINDEX_ENTRY ***table;
public:
  ADRINDEX(int, int);
  ~ADRINDEX();
  void enter_objekt(OBJEKT *objekt, ADR& adr);
  void remove_objekt(OBJEKT *objekt);
  short objekt_bei_adresse(ADR& adr) { return table[adr.x][adr.y] != 0; }
  DOUBLIST *alle_objekte_bei_adresse(ADR&, char *bed=NULL);
  DOUBLIST *alle_objekte_im_umkreis_von(ADR&, float, char *bed=NULL);
  static short radius_kleiner_gleich(int x, int y, float radius) 
  { return MAX(ABS(x),ABS(y)) + MIN(ABS(x),ABS(y)) * 0.5 <= radius; }
   
private:
  void objekte_in_liste_aufnehmen(DOUBLIST *, int, int, char *);
  DOUBLIST *langsam_alle_objekte_im_umkreis_von(int, int, float, char *bed=NULL);
};

#endif // __adrindex_h

