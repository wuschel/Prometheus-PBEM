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
  * MODUL:               maske.h  /  MASKE.H
  * AUTOR/DATUM:         Mathias kettner, 10. April 1994
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Deklariert die Klasse MASKE und einige weitere Klassen in diesem
// 	Zusammenhang, die fuer das Aufbauen von Bildschirmmasken mit ver-
//	schiedenen Eingabefeldern geeignet sind.
//
// *************************************************************************
#ifndef __maske_h
#define __maske_h

#include <string.h>
#include <stdio.h>

#include "doublist.h"
#include "mystring.h"

#define EDIT_QUIT 0
#define EDIT_END 1
#define EDIT_UP 2
#define EDIT_DOWN 3

typedef void((*M_RFKT)(void));

class MASKE; // weil ich Zeiger darauf brauche...

class MASKEFELD : public DOUBLIST_NODE
{
protected:
  short xpos,ypos;    // Bildschirmposition
  short laenge; // Groesse des Eingabebereiches
  char *daten;  // Datenbereich
  char **zielbereich; // Das Ergebnis soll sofort dorthinfahren
public:
  MASKEFELD(MASKE *m, short x, short y, short l, char **z=NULL);
  virtual ~MASKEFELD() { myfree(daten); };
  void refresh();
  virtual short edit() { return EDIT_END; };
  virtual void wert_in_zielbereich() {}; // Kann ueberladen werden
  short edit_textfeld(short modus=0); // modus=1 fuer numerische Felder.

  friend class MASKE;
};

struct TEXTFELD : MASKEFELD
{
public:
  TEXTFELD(MASKE *m, short x, short y, short l,
				 char **z=NULL, char *startwert=NULL);
  short edit();
  char *wert() { return daten; };
};

struct NUMFELD : MASKEFELD
{
  long *ziel;
public:
  NUMFELD(MASKE *,short,short,short,long sw=0, long *ziel=NULL);

  short edit();
  long wert() { return atol(daten);  };
  char *charwert() { return daten; };
  void wert_in_zielbereich() { if (ziel) *ziel = wert(); };
};


class AUSWAHLFELD : public MASKEFELD
{
  char **auswahlliste;
  short aktwahl;
  short anzahl;
public:
  AUSWAHLFELD(MASKE *, short,short,short ,char **, char *,char **);
  short edit();
  char *wert() { return daten; };
private:
  void weiterflippen(short offset = 1);
};


class MASKE : public DOUBLIST
{
  MASKEFELD *aktuellesfeld;
public:
  M_RFKT refresh;

public:
  static void default_refresh() {};
  ~MASKE() { while (!is_empty()) first()->remove(); };
  MASKE(M_RFKT ref = NULL)
    { refresh = ref ? ref : MASKE::default_refresh;
      aktuellesfeld = (MASKEFELD *)first(); };
  short edit();
  void felder_refreshen();
  void werte_in_zielbereiche_kopieren();
};


#endif // __maske_h

