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
  * MODUL:               gelaende.C / GELAENDE.CPP
  * AUTOR/DATUM:         Mathias Kettner, 1. Mai 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Enthaelt die Funktionen zur Klasse GELAENDE_FORM_TABELLE.
//
// **************************************************************************

#include <ctype.h>
#include <string.h>

#include "objekt.h"
#include "gelaende.h"
#include "laengen.h"
#include "kompatib.h"
#include "alg.h"
#include "uhr.h"
#include "log.h"

extern UHR *g_uhr; // von main.cpp


/**---------------------------------------------------------------------------
  * GELAENDE_FORM_TABELLE::GELAENDE_FORM_TABELLE // constr.
  * 
  * Konstruktor, der die Daten aus einem Asciifile liest. In der ersten
  * Zeile dieser Datei muss die Anzahl der Gelaendeformen stehen, in
  * jeder weiteren Zeile steht eine Attributsliste fuer eine dieser
  * Gelaendeformen. Momentan sind maximal 256 verschiedene Gelaendeformen
  * erlaubt.
  ---------------------------------------------------------------------------*/
GELAENDE_FORM_TABELLE::GELAENDE_FORM_TABELLE()
{
    i_am_ok = false;

    anzahl = 0;
    FILE *file;
  
    // An dieser Stelle muss ich unbedingt davon ausgehen, dass die Uhr exis-
    // tiert. Von ihr hole ich mir naemlich den Namen der Gelaendeformdatei,
    // wenn es geht.

    char *filename = g_uhr->gelaende_dateiname();
    
    file = fopen(filename, "r");
    if (!file) {
	log('E', "Can't open file defining area types '%s'", filename);
	return;
    }
    log('2', "Using area type definition file '%s'", filename);
    
    // Als erstes muss die Anzahl der verschiedenen Gelaendeattribute in der
    // Datei stehen
    
    fscanf(file, "%ld", &anzahl);
    if (anzahl <= 0 || anzahl > 256) {
	log('K', "Invalid number %ld of area types in file '%s'",
	    anzahl, filename);
	anzahl = 0;
	return;
    }
    
    // Jetzt lege ich Speicher fuer das Array der Gelaendeformzeiger an...
    tabelle = new GELAENDE_FORM *[anzahl];
    
    // .. und initialisiere deren Eintraege
    for (int f=0; f<anzahl; f++) tabelle[f] = new GELAENDE_FORM;
    
    // Puffer anlegen
    char *puffer = new char [MAX_LAENGE_ATTRIBUTSZEILE+2];
    
    // Und jetzt kommen die einzelnen Gelaendeformen...
    
    short status = 0;
    for (int i=0; i<anzahl; i++)
    {
	fgets(puffer,MAX_LAENGE_ATTRIBUTSZEILE,file);
	if (ferror(file)) {
	    status = 1;
	    break;
	}
	
	// Jetzt ueberlese ich alle Whitespaces und LFs und schaue, ob dann
	// ueberhaupt noch etwas uebrigbleibt. Wenn die Zeile mit einem '#'
	// anfaengt, dann betrachte ich sie als Kommentarzeile und hole mir
	// ebensfalls die naechte.
	
	char *p = puffer;
	while (isspace(*p) || *p==10 || *p==13) p++;
	if (!*p || *p=='#') {
	    i--; // Ich muss wieder um eins zurueckzaehlen!
	    continue; // Es kommt garnichts mehr ==> Neue Zeile holen!
	}
	else tabelle[i]->attribute.aus_string_einlesen(p);
	
	// Und hier noch eine Integritaetsabfrage. Ein Attribut, dass alle
	// Gelaendeformen unbedingt haben muessen, ist ABK.
	
	if (!tabelle[i]->attribute.gesetzt("ABK")) {
	    log('K', "Invalid area type file '%s'. Area type %d misses attribute ABK",
		filename, i);
	    status=1;
	    break;
	}
	
	
    }
    
    delete puffer;
    fclose(file);
    
    if (status) log('K', "Error parsing area type file '%s'", filename);
    else i_am_ok = true;
}


/**---------------------------------------------------------------------------
  * GELAENDE_FORM_TABELLE::~GELAENDE_FORM_TABELLE // destr.
  * 
  * Gibt im wesentlichen die Eintraege der Tabelle der Gelaendeformen
  * frei.
  ---------------------------------------------------------------------------*/
GELAENDE_FORM_TABELLE::~GELAENDE_FORM_TABELLE()
{
  if (anzahl && tabelle) {
    while (anzahl) {
      anzahl--;
      delete tabelle[anzahl];
    }
    delete tabelle;
  }
}


/**---------------------------------------------------------------------------
  * GELAENDE_FORM_TABELLE::attribut_fuer_form()
  * 
  * Sucht ein bestimmtes Attribut einer bestimmten Gelaendeform, die
  * ueber ihre Nummer (Position im Initialisierungsfile) spezifiziert
  * wird.
  *
  * PARMETER:
  * form:           Nummer der Form
  * klasse:         Attributsklasse, deren Wert ermittelt werden soll
  *
  * @return
  * Stringzeiger auf den Wert, der in Besitzt der Funktion bleibt.
  ---------------------------------------------------------------------------*/
char *GELAENDE_FORM_TABELLE::attribut_fuer_form(short form, char *klasse)
{
  if (form<0 || form>=anzahl)
  {
      log('K', "Reference to non-existing area type %ld. Check area type file", form);
      return NULL;
  }
  else return tabelle[form]->attribute.abfragen(klasse);
}

/**---------------------------------------------------------------------------
  * GELAENDE_FORM_TABELLE::attribut_gesetzt_fuer_form()
  * 
  * Ermittelt, ob ein bestimmtes Attribut einer bestimmten Gelaendeform
  * gesetzt ist.
  *
  * @param
  * form:           Laufende Nummer der Form in der Initialisierungsdatei
  * klasse:         Klasse des zu untersuchenden Attributs
  *
  * @return
  * 1, falls das Attribut gesetzt (vorhanden) ist, sonst 0.
  ---------------------------------------------------------------------------*/
short GELAENDE_FORM_TABELLE::attribut_gesetzt_fuer_form(short form,
								char *klasse)
{
  if (form<0 || form>=anzahl)
  {
      log('K', "Reference to non-existing area type %ld. Check area type file", form);
      return 0;
  }
  else return tabelle[form]->attribute.gesetzt(klasse);
}



short GELAENDE_FORM_TABELLE::form_mit_namen(char *name)
{
  for (short form=0; form<anzahl; form++) {
    if (!mystrcmp(name, tabelle[form]->attribute.abfragen("REP"))) return form;
  }

  return 0;
}

short GELAENDE_FORM_TABELLE::form_mit_abkuerzung(char *abk)
{
  for (short form=0; form<anzahl; form++) {
    char *testabk = tabelle[form]->attribute.abfragen("ABK");
    if (testabk && !mystrcmp(abk, testabk)) return form;
  }

  return 0;
}
