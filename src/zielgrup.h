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
  * MODUL:               zielgrup.h / ZIELGRUP.H
  * AUTOR/DATUM:         Mathias Kettner, 27. Mai 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Definiert die Klasse ZIELGRUPPE, deren Instanzen im Prinzip
//	Beschreibung von Teilmengen aus der globalen Objektmenge in
//	Abhaengigkeit eines weiteren Objektes sind.
//
// *************************************************************************
#ifndef __zielgrup_h
#define __zielgrup_h

class 	OBJEKT;
#include "attribut.h"

/**---------------------------------------------------------------------------
  * KLASSE:		ZIELGRUPPE
  * ABGELEITET VON:	(Basisklasse)
  * 
  * Bei fest gegebenen Referenzobjekt beschreiben die Daten einer
  * Zielgruppe eine Teilmenge der globalen Objektmenge. Dabei ge-
  * schieht die Auswahl ueber eine UND-Verknuepfung von vier Prae-
  * dikaten:  Name, Attribute, Besitzer, Ort.
  *
  * Name: 	So muss das beeinflusste Objekt heissen. Kein Name bedeutet
  * beliebiger Name
  * Attribute: Alle diese Attribute muss das Objekt besitzen und bei
  * angabe eines Wertes auch in diesem uebereinstimmen.
  * Besitzer: Das Objekt muss (transitiv) von diesem Objekt besessen
  * werden.
  * Ort:	Angabe des Ortes, an oder in dem sich das Objekt befinden
  * muss. Die Angabe eines Objektnamen fordert das Enthaltensein
  * des Objektes in diesem Ort. Die Angabe von <abstand>:<name>
  * fordert, dass das Zielobjekt sich im gleichen Ort befindet,
  * wie das Objekt <name> und von ihm einen Abstand kleiner
  * oder gleich <abstand> hat.
  *
  ---------------------------------------------------------------------------*/
class ZIELGRUPPE
{
  char *name;                   // den das Objekt haben muss oder NULL
  ATTRIBUT_LISTE attribute; // die das Objekt haben muss
  char *besitzer;               // Dem das Objekt gehoeren muss
  char *ort;			// An dem sich das Objekt befinden muss
public:
  ZIELGRUPPE(char *);	   // Aus String einlesen
  ZIELGRUPPE(FILE *);          // Aus Datei laden
  virtual ~ZIELGRUPPE();
  short zielt_von_auf(OBJEKT *, OBJEKT *);
  short speichern(FILE *);
private:
  void aus_string_initialisieren(char *spez);
};


#endif // __zielgrup_h
