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
// MODUL:               stratab.h  /  STRATAB.H
// AUTOR/DATUM:         Mathias Kettner, 3. August 1993
// KOMPATIBILITAET:     C++
// **************************************************************************
//
//	Enthaelt eine Tabelle (also tatsaechliche Daten!), die den ver-
//	schiedenen moeglichen built-in Strategien bei den verschiedenen
//	Alarms verschiedene Befehle zuweist.
//
// **************************************************************************

#ifndef __stratab_h
#define __stratab_h

// Hier ist die Tabelle der Befehle fuer alle Strategien. Die Konstanten
// sind in strategi.h definiert. 

// Alle Befehle werden in umgekehrter Reihenfolge ausgefuert. Dies liegt
// daran, dass die Aufgrund der Stapelcharakteristik der befehl_vorschieben()
// Funktion wesentlich einfacher zu programmieren war. Nur hier darf's eben
// nicht vergessen werden (c:

// Und hier noch die verschiedenen Alarmtypen...

// A_FEINDKONTAKT 	   A_FEIND_IM_WEG 		A_FEIND_IN_SICHT
// A_FEIND_GREIFT_AN 	   A_FREMDE_STADT_IN_SICHT 	A_FREMDE_STADT  

// A_BODENSCHATZ_VERMUTET  A_FEIND_HIER                 A_FLUGZEUG_IN_SICHT
// A_FLUGZEUG_KONTAKT	   A_FLUGZEUG_HIER              A_KAPERBARE_EINHEIT

// A_KEINE_STRASSE

char *strategie_tabelle[ANZAHL_STRATEGIEN][ANZAHL_ALARMS] =
{
 // SW0: S_BEFEHLSGEBUNDEN
 { "0", "0", "0", "0", "0", "0",
   "0", "0", "0", "0", "0", "0",
   "0" }
   
 // SW1: S_UNBEDINGTER_ANGRIFF, Bodeneinheit mit oder ohne Feuer
 ,{ L("6AN% FE%","6AT% FI%"),
    "0",
    L("5SC% FE%","5SC% FI%"),
    "0",
    L("4SC%","4SC%"),
    L("9EN","9OC"),
    "0",
    "0",
    L("7FE%","7FI%"),
    L("7FE%","7FI%"),
    L("7FE%","7FI%"),
    L("8AN% FE%","8AT% FI%"),
    "0" }
    
 // SW2: S_FEIGE, fuer alle milit~arischen Einheiten
 ,{  L("6FE%","6FI%"),
     "0",
     L("5SC%","5SC%"),
     L("7FE%","7FI%"),
     L("7SC% FE%","7SC% FI%"),
     L("9EN","9OC"),
    "0",
     "0",
     L("7FE%","7FI%"),
     L("8FE%","8FI%"),
     L("9FE","9FI"),
     L("9KP%","9CP%"),
    "0" }
    
 // SW3: S_ZIELSTREBIG, fuer Flugzeuge ueberfluessig
 ,{ "0",
    L("8AN%","8AT%"),     "0",        "0",        "0",        "0",
    "0",        "0",        "0",        "0",        "0",        "0",
    "0" }

 // SW4: S_DEFENSIV, nur fuer Defensiveinheiten, nicht fuer Flugzeuge
 ,{ L("6WA EI","6WA DI"),   "0",
    L("6WA EI","6WA DI"),
    L("6WA EI","6WA DI"),   "0",        "0",
    "0",
    L("6WA EI","6WA DI"),
    L("6WA EI","6WA DI"),
    L("6WA EI","6WA DI"),
    L("6WA EI","6WA DI"),   "0",
    "0" }

 // SW5: S_BODENSCHATZSUCHE, nur fuer Einheiten mit Graben oder Bohren
 ,{ "0",        "0",        "0",         "0",        "0",        "0",
    L("5BS","5SM"),       "0",        "0",        "0",        "0",        "0",
    "0" }

 // SW6: S_PIRATENSCHIFF, nur fuer militaerische Schiffe, mit oder ohne Feuer
 ,{ L("6KP%","6CP%"),     "0",
    L("5SC%","5SC%"),
    L("6FE%","6FI%"),     "0", 	"0",
    "0",
    L("6KP","6CP"),
    L("6FE%","6FI%"),
    L("7FE%","7FI%"),
    L("8FE%","8FI%"),
    L("8KP%","8CP%"),
    "0" }
 
 // SW7: S_FEINDFLUG, nur fuer Flugzeuge
 ,{ L("7AN%","7AT%"),     "0",
    L("4SC%","4SC%"),
    L("8AN%","8AT%"),
    L("6SC%","6SC%"),
    L("8BO","8BO"),
    "0",
    L("7AN BO","7AT BO"),
    L("4SC%","4SC%"),
    L("5AN%","5AT%"),
    L("6AN","6AT"),
    L("6AN%","6AT%"),
    "0" }

 // SW8: S_LUFTABWEHR, nur fuer Fernkampfeinheiten
 ,{ "0",        "0",        "0",
    L("8FE%","8FI%"),     "0",        "0",
    "0",        "0",
    L("4FE%","4FI%"),
    L("5FE%","5FI%"),
    L("6FE%","6FI%"),     "0",
    "0" }

 // SW9: S_BATTERIE, nur fuer Fernkampf, schiesst auf alles
 ,{ L("6FE%","6FI%"),     "0",
    L("5FE%","5FI%"),
    L("8FE%","8FI%"),     "0",        "0",
    "0",        "0",
    L("5FE%","5FI%"),
    L("6FE%","6FI%"),
    L("7FE%","7FI%"),
    L("7FE%","7FI%"),
    "0"}

 // SW10: S_STRASSENBAU nur fuer Bautrupps und Baukonvois. Baut ueberall Strasse.
 ,{ "0", "0", "0", "0", "0", "0",
    "0", "0", "0", "0", "0", "0",
    L("5ST","5CR") }

 // SW11: S_BOMBER: Versucht Einheiten und dann Staedte zu bombardieren
 ,{ L("5SC%","5SC%"), "0",
    L("4SC%","4SC%"), "0",
    L("3SC%","3SC%"),
    L("8BO","8BO"),
    "0", L("8BO","8BO"), "0", "0", "0",
    L("8BO","8BO"),
    "0" }
};

// Folgende Tabelle gibt Auskunft darueber, welche Bedingung noetig sind,
// damit eine Einheit eine Strategie waehlen darf.

long strategie_flags[ANZAHL_STRATEGIEN] =
{
   0		 						 // 0
  ,SB_NICHT_FLUG  | SB_NICHT_ZIVIL                               // 1    
  ,                 SB_NICHT_ZIVIL                               // 2
  ,SB_NICHT_FLUG  | SB_NICHT_ZIVIL | SB_HEERESLEITUNG            // 3
  ,SB_NICHT_FLUG  | SB_NICHT_ZIVIL | SB_HEERESLEITUNG            // 4
  ,SB_NICHT_FLUG                                                 // 5
  ,SB_NICHT_FLUG  | SB_NICHT_ZIVIL                               // 6
  ,SB_NUR_FLUG    | SB_NICHT_ZIVIL | SB_HEERESLEITUNG            // 7
  ,                 SB_FERNKAMPF   | SB_HEERESLEITUNG            // 8
  ,                 SB_FERNKAMPF   | SB_HEERESLEITUNG            // 9
  ,0								 // 10
  ,SB_NUR_FLUG    | SB_NICHT_ZIVIL | SB_HEERESLEITUNG            // 11
};
                                                                
#endif // __stratab_h                                              



