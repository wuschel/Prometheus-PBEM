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
  * MODUL:               wegsuche.h  /  WEGSUCHE.H
  * AUTOR/DATUM:         Mathias Kettner, 5. August 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Deklarationen von Klassen und Konstanten fuer den Wegsuche-Algorith-
//	mus in wegsuche.C. Wird nur von dort eingebunden, ist allgemein
//	von wenig Interesse.
//
// **************************************************************************

#ifndef __wegsuche_h
#define __wegsuche_h


class WS_ADRESSLISTE : public DOUBLIST_NODE
{
  short nummer;
  DOUBLIST adressliste;
  WS_ADRESSLISTE(short n) { nummer=n; };
  short matches(void *k) { return *((short *)k) == nummer; };

  friend class WS_LISTEN_MENGE;
};


class WS_LISTEN_MENGE : public DOUBLIST
{
public:
  void feld_einfuegen(ADR& adr, short nummer);
  short naechstes_feld_holen(ADR& adr); // adr Dient als Ausgabepar!
  WS_LISTEN_MENGE(ADR& start); // Legt die erste Liste an.
private:
  WS_ADRESSLISTE *liste_mit_nummer(short); // Sucht bestimmte Liste.
  WS_ADRESSLISTE *neue_liste_anlegen(short); // Legt neue Liste fuer neue Nummer an.
};


// Konstante fuer die Nummerierung

#define WS_LISTE_LEER -1
#define WS_OHNE_NUMMER -2

// Typdefinition fuer die Entfernungsfunktion. Als Rueckgabe liefert eine
// Entfernungsfunktion die Anzahl der benoetigten Runden, falls das Reisen
// erlaubt ist und dauert, 0, wenn das Reisen spontan moeglich ist und
// -1, wenn eine Bewegung nicht moeglich ist. data zeigt typischerweise
// auf ein OBJEKT.

typedef short((*EFKT)(void *data, ADR& start, ADR& ziel));

#endif // __wegsuche_h

