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


// -*- c++ -*-
// **************************************************************************
// MODUL:               statisti.h  /  STATISTI.H
// AUTOR/DATUM:         Mathias Kettner, 11. November 1993
// KOMPATIBILITAET:     C++
// **************************************************************************
//
//      Enthaelt die Deklaration der Klassen HITLISTE und
//	HITLISTEN_EINTRAG.
//
// **************************************************************************

#ifndef __statisti_h
#define __statisti_h

#include <string.h>

#include "alg.h"

class HTML;
class LAYOUT; // Damit ich nicht alles includen muss...

/**---------------------------------------------------------------------------
  * KLASSE:              HITLISTE
  * ABGELEITET VON:      DOUBLIST_NODE
  * 
  * Stellt eine Highscoreliste dar, die ein bestimmtes Thema hat.
  ---------------------------------------------------------------------------*/
class HITLISTE : public DOUBLIST_NODE
{
public:
    char *name;  	      // Dient zur Ident. und f.d. Ausdruck als Ueberschr.
    char *formatstring; // Fuer Ausgabe des Wertes! z.B. "%.2f" bei Prozentw.
    short flags; 	      // HL_LONG, HL_... geodert.

    DOUBLIST eintraege; // Hier stehen die Eintraege der Staaten
    
public:
    HITLISTE(char *n, char *f, short fl)
	{ name = mystrdup(n); formatstring = f; flags = fl; };
    ~HITLISTE() { myfree(name); };
    void Print() { drucken("%s:\n",name); eintraege.Print(); };
    long punkte_auf_platz(short);
    long punkte_auf_platz_pur(short);
    long punkte_fuer_staat(OBJEKT *);
    LAYOUT *layout_erstellen(float, float, OBJEKT *, short, float);
    void layout_erstellen_html(HTML, OBJEKT *, short, char *);
private:
    short matches(void *k) { return !strcmp((char *)k, name); };
};


class HITLISTEN_EINTRAG : public DOUBLIST_NODE
{
public:
  OBJEKT *staat;
  long longwert;
  float floatwert;

public:
  HITLISTEN_EINTRAG(OBJEKT *s, long w)
	{ staat=s; longwert=w; floatwert = 0; };
  HITLISTEN_EINTRAG(OBJEKT *s, float w)
	{ staat=s; floatwert=w; longwert = 0; };
  short gleicher_wert_wie(HITLISTEN_EINTRAG *c)
	{ return c && c->longwert==longwert && c->floatwert==floatwert; };

  void Print() { drucken("%s: %s %s\n",staat->attribut("NAME"),
		 myltoa(longwert), myftoa(floatwert)); };
};

// Konstante fuer die Flags und fuer die Funktion hitliste_erstellen()
#define HL_LONG 	1       // Eintraege vom Typ long
#define HL_FLOAT	2       // Eintraege vom Typ float
#define HL_AUFSTEIGEND	4	// Sortierung erfolgt aufsteigend
#define HL_ABSTEIGEND 	8	// Sortierung erfolgt absteigend
#define HL_DOPPELT	16	// Liste wird staerker gewichtet

#endif // __statisti_h

