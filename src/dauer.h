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
  * MODUL:		dauer.h  /  DAUER.H
  * AUTOR/DATUM:		Mathias Kettner, 21. Juli 1993
  * KOMPATIBILIATAET:	C++
  ---------------------------------------------------------------------------*/
//
//	Beinhaltet Konstantendefinitionen zuer Dauer verschiedener Aktionen,
//	die von Einheiten, Stadten usw. ausgefuerht werden koennen.
//
// **************************************************************************

#ifndef __dauer_h
#define __dauer_h

#define RUNDEN_PRO_ZUG    100

#define EIN_ZUG 	  RUNDEN_PRO_ZUG
#define HALBER_ZUG	  ((EIN_ZUG)/2)
#define EINEINHALB_ZUEGE  ((EIN_ZUG*3)/2)
#define ZWEI_ZUEGE        ((EIN_ZUG*2))
#define ZWEIEINHALB_ZUEGE ((EIN_ZUG*5)/2)

// EINHEIT

#define DA_WALDRODUNG 		20
#define DA_AUFFORSTUNG 		ZWEI_ZUEGE
#define DA_TROCKENLEGUNG	EIN_ZUG
#define DA_VERWUESTEN		10
#define DA_ACKERLAND 		80
#define DA_BRANDSCHATZUNG 	20
#define DA_EINGRABUNG 		50
#define DA_AUSGRABUNG 		10
#define DA_RESERVE 		5
#define DA_MOBILISIERUNG 	95
#define DA_ANGRIFF 		20
#define DA_EMBARKATION 		10
#define DA_DISEMBARKATION 	10
#define DA_EROBERN 		0
#define DA_BELADEN 		5
#define DA_ENTLADEN 		5
#define DA_STRASSENBAU 		20
#define DA_BODENSCHATZSUCHE 	5
#define DA_STADTGRUENDUNG	25
#define DA_EINMISCHEN           HALBER_ZUG
#define DA_KAPERUNG             20
#define DA_UNTERSUCHE		10
#define DA_METEORIT_BERGEN	20

// Jetzt kommen noch einige interessante Definitionen.
// Der Ablauf des Zuges bei einer Stadt ist in verschiedene Abschnitte
// unterteilt. In welcher Runde was stattfindet, wird hier definiert.

#define RUNDE_GUETERVERKEHR 5 // Befehl SV
#define RUNDE_VERMISCHTES 5 // z.B. im Staat Boni verteilen.
#define RUNDE_ARBEITSKRAFT 10 // In der Stadt wird die Arbeitskraft berechnet
#define RUNDE_FOERDERUNG 20 // Resourcenfoerderung bei den Weltbauten.
#define RUNDE_FELDER_BEWIRTSCHAFTEN 30 // Bewirtschaftung der Felder berechnen
#define RUNDE_RESOURCEN_UMWANDELN 40 // Fabriken u.s.w berechnen
#define RUNDE_VERSORGUNG_EINHEITEN 50 // Besitztum der Stadt und Weltbauten vers. sich.
#define RUNDE_VERSORGUNG_STADTAUSBAU 60
#define RUNDE_VERSORGUNG_WELTBAUTEN 70 // Besitztum der Stadt und Weltbauten vers. sich.
#define RUNDE_EINWOHNER_ERNAEHREN 80
#define RUNDE_WEITERBAUEN 90

#endif // __dauer_h


