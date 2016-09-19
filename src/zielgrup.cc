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
  * MODUL:               zielgrup.C / ZIELGRUP.CPP
  * AUTOR/DATUM:         Mathias Kettner, 27. Mai 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Enthaelt alle Funktionen der Klasse ZIELGRUPPE, mit der eine
//      Teilmenge der globalen_objekt_menge in Abhaengingkeit von einem
//      Ausgangsobjekt spezifiziert werden kann. Genauer gesagt realisiert
//      jede Instanz der Klasse eine "ist Element von"-Relation.
//
// *************************************************************************

#include <string.h>
#include <ctype.h>

#include "zielgrup.h"
#include "laengen.h"
#include "objekt.h"
#include "landscha.h"
#include "kompatib.h"
#include "log.h"

/**---------------------------------------------------------------------------
  * ZIELGRUPPE::ZIELGRUPPE()      // constructor
  * 
  * Konstruktor der Zielgruppe. Hier kann eine neue Zielgruppe (d.h.
  * Zusammenfassung von Merkmalen, die eine bestimmte Teilmenge aus
  * den Objekten auswaehlt) definiert werden, die dann verwendet
  * werden kann, um Einfluesse zu spezifizieren.
  *
  * @param
  * char *spez:	Spezifikation der Zielgruppe. Sie hat folgendes
  * Format:
  * Name,Attribute,Besitzer,Ort
  *
  ---------------------------------------------------------------------------*/
ZIELGRUPPE::ZIELGRUPPE(char *spez)
{
  aus_string_initialisieren(spez);
}

void ZIELGRUPPE::aus_string_initialisieren(char *spez)
{
  char *temp = mystrdup(spez);

  // Jetzt muss ich immer bis zum Komma auslesen und mit dem Teilstring
  // dann etwas machen
  char *anfang = temp;
  for (int teil=1; teil<=4; teil++)
  {
    char *scan = anfang;
    while (*scan && *scan != ',') scan++;
    char komma = *scan;
    *scan = 0;
    switch (teil) {
      case 1: 	if (anfang[0]) name = mystrdup(anfang);
		else name = NULL;
		break;

      case 2: 	attribute.aus_string_einlesen(anfang);
		break;

      case 3: 	if (anfang[0]) besitzer = mystrdup(anfang);
		else besitzer = NULL;
		break;

      case 4: 	if (anfang[0]) ort = mystrdup(anfang);
		else ort = NULL;
		break;
    }

    anfang = scan+1; // Anfang fuer den naechsten Teilstring vorbereiten

    if ((teil < 4 && !komma) || (teil==4 && komma)) {
	log('K', "Invalid specification in game configuration: %s",
	    spez);
	return;
    }
  }
  myfree(temp);
}


/**---------------------------------------------------------------------------
  * ZIELGRUPPE::ZIELGRUPPE(..2..) // construktor
  * 
  * Zweite Form des Konstruktors. Er bezieht die Daten aus einer Datei,
  * Die mit der Funktion ZIELGRUPPE::speichern() beschrieben wurde.
  *
  * @param
  * file:           Filepointer (fopen())
  ---------------------------------------------------------------------------*/
ZIELGRUPPE::ZIELGRUPPE(FILE *file)
{
  char *puffer = new char[MAX_LAENGE_ATTRIBUTSZEILE+3*MAX_LAENGE_NAME+2];

  fscanf(file, "%s", puffer);
  // Nun noch ein ^M am Zeilenende wegmachen, wenn von DOS noch eins da
  // ist...
  if (puffer[strlen(puffer) - 1] == 13) puffer[strlen(puffer)-1] = 0;

  aus_string_initialisieren(puffer);

  delete puffer;
}

/**---------------------------------------------------------------------------
  * ZIELGRUPPE::~ZIELGRUPPE()      // destruktor
  * 
  * Destruktor der Zielgruppenklasse.
  ---------------------------------------------------------------------------*/
ZIELGRUPPE::~ZIELGRUPPE()
{
  myfree(name);
  myfree(besitzer);
  myfree(ort);
  // Da die Attribute direkt vorhanden sind (und nicht nur als Zeiger), wird
  // ihr Destruktor automatisch aufgerufen.
}

/**---------------------------------------------------------------------------
  * ZIELGRUPPE::zielt_von_auf()
  * 
  * Mit dieser Funktion kann man feststellen, ob diese Zielgruppe ein
  * bestimmtes Objekt beinhaltet. Dabei muss allerdings noch ein
  * Objekt angegeben werden, von dem aus die Zielgruppe berechnet
  * wird, da diese unter Umstaenden relative Bezuege enthaelt.
  * Sollte dieses Objekt (Einflussgeber) schon zur vernichtung vor-
  * gemerkt sein, so wird der Einfluss nicht mehr aktiv. Ich tue
  * dann so, als ab der Einfluss nicht mehr auf das Ziel zielt.
  *
  * @param
  * von_obj:        Objekt, von dem aus die Zielgruppe gesehen ist.
  * zu_obj:         Objekt, von dem geprueft werden soll, ob es
  * sich in dieser Zielgruppe befindet.
  *
  * @return
  * 1, wenn sich das Objekt in der Zielgruppe befindet.
  * 0, wenn nicht.
  ---------------------------------------------------------------------------*/
short ZIELGRUPPE::zielt_von_auf(OBJEKT *von_obj, OBJEKT *auf_obj)
{
  if (von_obj -> zur_vernichtung_vorgemerkt()) return 0; // Lebt garnicht mehr.
  if (name) if (von_obj->objekt_mit_namen(name) != auf_obj) return 0;

  if (ort) {

    // Hier sind zwei Darstellungen moeglich, die eine unterschiedliche
    // Bedeutung haben. Wenn die Darstellung mit einer Zahl anfaengt,
    // so bedeutet dies einen Abstand (siehe ZIELGRUP.H)

    if (isdigit(ort[0])) {
      char zahlstring[20], *scan, *writez;
      scan=ort;
      writez=zahlstring;
      while (*scan && *scan!=':') *writez++ = *scan++; // Zahl kopieren

      if (!*scan) { // Fehler
	  log('K', "Invalid specification of place in game configuration: %s"
	      ,ort);
	ort = NULL;
	return 0;
      }

      *writez=0; // String beenden.
      
      float max_abstand = myatof(zahlstring);

      // Mit mittelobjekt ist das Objekt gemeint, zu dem der Abstand
      // eingehalten werden muss, damit der Einfluss wirkt. Dieses
      // Objekt wird naemlich nach dem Doppelpunkt angegeben, nicht
      // etwa die Landschaft selbst.
      
      OBJEKT *mittelobjekt = von_obj->objekt_mit_namen(scan+1);

      // zielort ist der Ort, in dem sich das Mittelobjekt befindet.
      BASIS_LANDSCHAFT *zielort=(BASIS_LANDSCHAFT *)mittelobjekt->ort();

      // Falls sich das Auf-Objekt garnicht in der gleichen Landschaft befindet,
      // dann wirkt der Einfluss auf keinen Fall. Ebenso, wenn das Auf-Objekt
      // keine explizite Adresse hat.
      if (!zielort || auf_obj->ort() != zielort) return 0;
      if (auf_obj->adresse.ist_ausserhalb()) return 0;

      // Ich setze nun voraus, das Objekte mit einer gueltigen Adresse
      // sich in einem Objekt befinden, das von BASIS_LANDSCHAFT
      // abgeleitet ist. Dann berechne ich den Abstand vom Mittelobjekt.

      float abstand = ((BASIS_LANDSCHAFT *)zielort)
	  ->entfernung_zwischen(mittelobjekt->adresse, auf_obj->adresse);

      if (abstand > max_abstand) return 0; // Zu weit weg.

    } // Zahldarstellung

    else if (von_obj->objekt_mit_namen(ort) != auf_obj->ort()) return 0;
  }

  if (!attribute.passen_in(&auf_obj->attribute)) return 0;
  if (besitzer)
  {
    OBJEKT *bes = von_obj->objekt_mit_namen(besitzer);
    if (!bes) return 0; // Den Besitzer gibt es garnicht.
    if (!bes->besitzt(auf_obj)) return 0;
  }
  return 1;
}

/**---------------------------------------------------------------------------
  * ZIELGRUPPE::speichern()
  * 
  * Speichert die Daten, die eine Zielgruppe beschreiben in eine bereits
  * geoeffnete Datei. Der Eintrag besteht einem String
  *
  * @param
  * file:           Filepointer (fopen())
  *
  * @return
  * 1, im Fehlerfall, sonst 0.
  ---------------------------------------------------------------------------*/
short ZIELGRUPPE::speichern(FILE *file)
{
  char *attr = attribute.to_string();

  fprintf(file,"%s,%s,%s,%s\n",
      name ? name : "", attr, besitzer ? besitzer : "", ort ? ort : "");

  return (ferror(file) != 0);
}


