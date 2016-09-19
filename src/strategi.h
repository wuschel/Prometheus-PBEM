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
// MODUL:               strategi.h  /  STRATEGI.H
// AUTOR/DATUM:         Mathias Kettner, 3. August 1993
// KOMPATIBILITAET:     C++
// **************************************************************************
//
//	Definiert Konstante und Klassen fuer das System der Strategien und
//	Alarms.
//
// **************************************************************************

#ifndef __strategi_h
#define __strategi_h

#define S_BEFEHLSGEBUNDEN     0  // Reagiert ueberhaupt nicht auf Alarms
#define S_UNBEDINGTER_ANGRIFF 1
#define S_FEIGE               2
#define S_ZIELSTREBIG         3  // Achtung: Teil der Impleme. in EINHEIT::entfernungsfunktion()
#define S_DEFENSIV            4
#define S_BODENSCHATZSUCHE    5   // Macht BS, falls Bodenschatz vermutet.
#define S_PIRATENSCHIFF       6
#define S_FEINDFLUG           7
#define S_LUFTABWEHR          8
#define S_BATTERIE            9
#define S_STRASSENBAU        10
#define S_BOMBER             11

#define ANZAHL_STRATEGIEN 12 // Muss eins hoeher sein als die letzte Strat.

// Verschiedene Alarmtypen. Der Alarm Feind_greift_an wurde wieder ge-
// strichen, da meist eine sichere Flucht vor dem Angreifer ermoeglicht.
// Es gibt kein Ereignis, das mehrere Alarms ausloest. Im Ueberschneidungs-
// fall wird der Alarm mit der kleineren Nummer ausgeloest.

#define A_FEINDKONTAKT 		 0 
	// Schiff oder Bodeneinheit ist benachbart, %s=Richtung

#define A_FEIND_IM_WEG 		 1
	// RE-Befehl wurde durch Einheit verhindert, %s=Richtung

#define A_FEIND_IN_SICHT 	 2
	// Schiff oder Bodeneinheit in Sichtweite, %s=Richtung

#define A_FEIND_GREIFT_AN 	 3 
	// Ein Feind fuehrt gerade den AN-Befehl auf mein Feld aus. %s=Richtung

#define A_FREMDE_STADT_IN_SICHT  4 
	// Gegnerische Stadt befindet sich in Sichtweite, %s=Richtung

#define A_FREMDE_STADT 		 5 
	// Ich befinde mich genau auf einer gegnerischen Stadt.

#define A_BODENSCHATZ_VERMUTET 	 6 
	// Auf diesem Feld noch nicht ueber Bodenschatz entschieden, aus-
	// serdem koennte die Einheit dies mit BS jetzt tun.

#define A_FEIND_HIER 		 7
	// Gegnerisches Schiff oder Bodeneinheit auf gleichem Feld.

#define A_FLUGZEUG_IN_SICHT 	 8
	// Gegnerisches Flugzeug in Sichtweite, %s=Richtung

#define A_FLUGZEUG_KONTAKT	 9
	// Gegnersches Flugzeug auf benachbartem Feld, %s=Richtung

#define A_FLUGZEUG_HIER		10
	// Gegnerisches Flugzeug auf gleichem Feld.

#define A_KAPERBARE_EINHEIT 	11
	// Kaperbare gegnerische Einheit benachbart.
	
#define A_KEINE_STRASSE 	12

#define ANZAHL_ALARMS 13      // Muss angepasst werden, wenn neue Alarms def.


// Hier kommen noch Flags fuer die strategie_flags[]- Tabelle
#define SB_HEERESLEITUNG	 1
#define SB_NICHT_FLUG		 2
#define SB_NICHT_ZIVIL		 4
#define SB_NUR_FLUG		 8
#define SB_FERNKAMPF		16

struct ALARM : DOUBLIST_NODE
{
  short typ;
  char *parameter;

  ALARM(short t, char *p=NULL) { typ = t; parameter = mystrdup(p); };
  ~ALARM() { myfree(parameter); };
  short prioritaet(short str); // Ermittelt diese in Abh. von der Strategie
};


#endif // __strategi_h

