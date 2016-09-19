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
  * MODUL:		prom.h  /  prom.H
  * AUTOR/DATUM:         Mathias Kettner, 10. November 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Deklariert die Funktionen, die erstens nicht Mitglied einer Klasse
//	sind, und zweitens spezifisch fuer das Spiel Prometheus sind.
//
// **************************************************************************

#ifndef __prom_h
#define __prom_h

#include "doublist.h"

class ENZYKLOPAEDIE;
class OBJEKT;
class WELT;

ENZYKLOPAEDIE *enzyklopaedie();
char *lexikon_eintrag(char *, short);
long lexikon_long(char *k, short s=1);
long jahr_in_zug(long zugnummer);
long punkte_fuer_gewinn();
long anzahl_staaten();
long anzahl_spieler_angetreten();
short auswertung_in_zug(long zugnummer);
bool gewinnt_kampf_gegen(long kraft1, long kraft2);
bool hat_uebermacht(long kraft1, long kraft2);
char *ein_zus_add_resource(char *, char *);
void nach_verteidigungskraft_sortieren(DOUBLIST *);
void reports_layouten_und_drucken(OBJEKT *);
char *konjugation(char *, short);
char *woertliche_objekt_aufzaehlung(DOUBLIST *, short);
short sortfunction_woa(DOUBLIST_NODE *, DOUBLIST_NODE *, void *);
char *tempfile_name(const char *bestandteil = NULL); // Name fuer Tempdatei
char *nice_gross_klein(char *name);

// Hier kommen Funktionen, die rigoros am Objektsystem vorbeiarbeiten,
// die ich aber einfach brauche, um halbwegs vernuenftig programmieren
// zu koennen.

WELT *erde();
long neue_spielernummer();

// Konstante fuer die Funktion konjugation und woertliche...

#define NOMINATIV 	 1
#define GENITIV          2
#define DATIV            3
#define AKKUSATIV        4

#define SINGULAR	 8
#define PLURAL		16

#define GENITIV_PHRASE	(SINGULAR|GENITIV|32) // Eventuell mit 'von'+Dativ

#endif // __prom_h

