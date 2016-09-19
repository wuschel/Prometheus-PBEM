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
/**---------------------------------------------------------------------------
  * MODUL:               drucker.h / DRUCKER.H
  * AUTOR/DATUM:         Mathias Kettner, 28. Maerz 1994
  * KOMPATIBILITAET:     C++
  -----------------------------------------------------------------------------
  *
  *	Deklaration  der Druckerfunktionen der Klasse dr_...(), die unab-
  *	haenging von den Druckfunktionen von LAYOUT sind, sich aber
  *	mit ihnen ergaenzen. Sie opererieren zum Teil auf gemeinsamen 
  *	Variablen innerhalb des Druckertreibermoduls (z.B. pcl5.o oder
  *	postscri.o). Einige der Funktionen, die unabhaengig von der Drucker-
  *	anpassung sind und eher allgemeinere Hilfsfunktionen darstellen
  *	(etwa Blocksatz von Texten fuer Zeichensaetze mit festem Zeichen-
  *	abstand) sind in drucker.cpp definiert.
  *
  ---------------------------------------------------------------------------*/

#ifndef __drucker_h
#define __drucker_h

#include <stdio.h>

char *dr_infodatei_formatieren(char *filename, long spalten);
void dr_umlaute_ersetzen(char *puffer);
void dr_strip_lf_cr(char *puffer);
char *dr_umlaut_sequenz(char); // Muss vom Druckermodul bereitgestellt werden
void dr_anfang(bool duplex); // Neuer Ausdruck soll begonnen werden. Drucker init.
float dr_abschnitt(float cm); // Prueft, ob Platz, macht sonst neue Seite.
void dr_auswurf();  // Beendet aktuelle Seite und druckt aus.
void dr_neueseite(); // Erzwingt Seitenumbruch
void dr_zeilenabstand(float); // Zeilenabstand in cm einstellen
long dr_anzahl_zeilen(char *); // Zaehlt LFs

#endif // __drucker_h

