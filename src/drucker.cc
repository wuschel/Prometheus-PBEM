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
  * MODUL:		drucker.C  /  DRUCKER.CPP
  * AUTOR/DATUM:		Mathias Kettner, 2. Juli 1993
  * KOMPATIBILITAET:	C++
  ---------------------------------------------------------------------------*/
//
//	Definiert Funktionen fuer die Schnittstelle zum Drucker. Diese
//	Funktionen sind die unterste Ebene an Druckerfunktionen, die
//	noch druckerunspezifisch implementiert werden koennen. Die
//	spezifischen Funktionen muessen fuer jeden Drucker gesondert
//	implementiert werden. Spezifische und unspezifischen Funktionen
//	sind in drucker.h/DRUCKER.H deklariert.
//
// **************************************************************************

#include <string.h>
#include <ctype.h>

#include "drucker.h"
#include "mystring.h"
#include "alg.h"

/**---------------------------------------------------------------------------
  * dr_infodatei_formatieren()
  * 
  * Laedt eine Textdatei mit Formatzeichen und erzeugt einen blockgesetz-
  * ten Ausgabestring, bei dem auch die Umlaute durch die richtigen Zei-
  * chen ersetzt wurden. Zeilenumbrueche werden durch \n erzeugt.
  *
  * Zum Format der Quelldatei: Alle Zeichen werden 1:1 uebertragen, bis
  * auf folgende Ausnahmen:
  *
  * 1. Die Tilde leitet eine Zeichenersetzung ein. Momentan sind de-
  * finiert: ~a,~A,~o,~O,~u,~U: Umlaute, ~s: Scharfes S.
  *
  * 2. Mit dem Zeichen '#' koennen Trennstellen vorgeschlagen werden.
  * Dies sollte bei langen Wort verwendet werden, damit im Ausdruck
  * kein zu grossen Luecken entstehen.
  *
  * 3. Folgt auf das '#' und Zeilenende oder ein Leerzeichen, so wird
  * an dieser Stelle im Ausdruck ein explizites Zeilenende erzwungen.
  * Auf diese Art koennen Absaetze gebildet werden.
  *
  * 4. Unterstriche werden als Space ausgegeben.
  *
  * @param
  * char *filename:		Dateiname
  * long spalten:           Breite, auf die formatiert werden soll.
  *
  * @return
  * Zeiger auf einen einzigen grossen String, der das Ergebnis enthaelt.
  * NULL, wenn ein Fehler auftrat. Der String muss nach Gebrauch mittels
  * delete freigegeben werden.
  ---------------------------------------------------------------------------*/
char *dr_infodatei_formatieren(char *filename, long spalten)
{
  // Im Text werden moegliche Trennstellen durch '#' gekennzeichnet. Umlaute
  // werden durch ~a ~o ~u ~A ~O ~U und ~s dargestellt.

  char *ausgabe = NULL; // Hier wird der Ausgabestring aufgebaut.

  // Jetzt hole ich mir die ganze Datei auf einen Schwung...

  char *lesepuffer = get_file(filename);
  if (!lesepuffer) return NULL;

  dr_umlaute_ersetzen(lesepuffer); // Steuerzeichen entfernen
  dr_strip_lf_cr(lesepuffer); // CRs entfernen und LFs durch Spaces ersetzen

  // Und dann brauche ich noch einen Zeiger innerhalb des Puffers.

  char *lesezeiger = lesepuffer;

  // Die Hauptschleife ist an den Ausgabezeilen orientiert.

  while (*lesezeiger) {

    // Erstmal ueberlese ich Spaces und Trennzeichen am Anfang...
    while (*lesezeiger==' ' || (*lesezeiger=='#' && (*lesezeiger+1)!=' '))
      lesezeiger++;

    // Vielleicht war's das auch schon mit dem String...
    if (!*lesezeiger) break;

    // Nun muss ich nach der richtigen Trennstelle suchen. Sie ist
    // nach dem Zeichen spalte oder weiter links. Ich muss also als
    // erstes dieses Zeichen finden. Dazu muss ich von links weg zaehlen
    // und die Trennsymbole '#' dabei ueberlesen. Hier geht auch die
    // Moeglichkeit eines expliziten Zeilenendes ein, dass durch "# "
    // oder "#\n" erzwungen werden kann.

    char *trennzeiger = lesezeiger;
    short i, spaces = 0; // Ich muss mir die Anzahl der Spaces merken
    for (i=0; i<spalten && *trennzeiger; trennzeiger++) {
	if (*trennzeiger != '#') i++;
	else if (*(trennzeiger+1) == ' ') break;
	if (*trennzeiger == ' ') spaces++;
    }

    // Wenn am Ende der Schleife die Variable i noch kleiner als spalten
    // ist, dann wurde die Schleife vorzeitig abgebrochen. In diesem
    // Fall findet kein Randausgleich statt, da das Abbrechen aufgrund
    // eines expliziten Zeilenendes geschah.

    // Jetzt steht der Trennzeiger genau auf der letzten moeglichen Spalte.
    // Nun muss ich soweit nach links gehen, bis dort ein Space oder links
    // davon ein '#' steht.

    short ausgleich=0;

    if (*trennzeiger && i==spalten) // Sonst ist ueberhaupt kein Ausgleich noetig!
    {
      char *alter_trennzeiger = trennzeiger; // Merken fuer spaeter
      while (trennzeiger>lesezeiger && *(trennzeiger-1)!='#' && *trennzeiger!=' ')
      {
	ausgleich++;
	trennzeiger--;
      }
      if (*trennzeiger == ' ') spaces--; // Das Trennspace zaehlt nicht.
      else if (*(trennzeiger-1) == '#') *(trennzeiger-1)='-';

      // Nun ist entweder wieder der Anfang der Zeile erreicht, oder eine
      // gute Trennstelle gefunden. Bei ersterem setze ich den Trennzeiger
      // einfach wieder auf seine alte Stelle.

      if (trennzeiger == lesezeiger) {
	ausgleich = 0;
	trennzeiger = alter_trennzeiger;
      }

    } // if (*trennzeiger)

    else ausgleich = 0;

    // So. Nun uebertrage ich den String in einenen Schreibpuffer.

    char *schreibpuffer = new char[256];
    char *schreibzeiger = schreibpuffer;
    short space_pro_space;
    short zusatzspaces;

    if (spaces == 0 || !ausgleich) {// Kein Ausgleich!
      space_pro_space = 0;
      zusatzspaces = 0;
    }

    else {
      space_pro_space = ausgleich/spaces;
      zusatzspaces = ausgleich%spaces;
    }

    while (lesezeiger<trennzeiger)
    {
      if (*lesezeiger == ' ') {

	// Kommt ein Space, so muss ich evtl. einen Ausgleich machen.
	// Die Anzahl der Spaces, die kommen muss, weiss ich genau.
	// Jetzt muss ich den Ausgleich moeglichst gleichmaessig auf die
	// Spaces verteilen. Dazu bekommt ersteinmal jedes Space die
	// gleiche Anzahl an Zusatzspaces. Der Rest wird dann von
	// links weg vergeben, an jedes Space noch eines. Dazu habe ich
	// mir zwei Variable vorberechnet.

	for (int j=0; j<space_pro_space; j++) *schreibzeiger++ = ' ';
	if (zusatzspaces) {
	  *schreibzeiger++ = ' ';
	  zusatzspaces--;
	}
      }

      // So. Das Space selbst wie alle anderen Zeichen werden nun ueber-
      // tragen (ausser dem Trennzeichen '#'.)

      if (*lesezeiger == '_') {
        *schreibzeiger++ = ' ';
        lesezeiger++;
      }
      else if (*lesezeiger != '#') *schreibzeiger++ = *lesezeiger++;
      else lesezeiger++;
    }

    // Die Zeile ist nun fertig aufgebaut. Ich haenge noch ein LF dran und
    // Fuege Sie zum Ergebnis.

    *schreibzeiger++ = '\n';
    *schreibzeiger++ = 0; // String muss noch terminiert werden!
    mystradd(ausgabe,schreibpuffer);
    delete schreibpuffer;

  } // while (*lesezeiger)

  // So. Das wars. Ich bin fertig.

  delete lesepuffer;
  return ausgabe;
}


/**---------------------------------------------------------------------------
  * dr_umlaute_ersetzen()
  * 
  * Entfernt alle Steuersequenzen, die durch eine Tilde eingeleitet
  * werden. Ruft dazu Funktionen aus dem druckerspezifischen Modul
  * auf.
  *
  * @param
  * char *puffer:	Datenbereich mit einem String, in dem ersetzt werden
  * soll.
  ---------------------------------------------------------------------------*/
void dr_umlaute_ersetzen(char *puffer)
{
  // Vereinfachend gehe ich davon aus, dass nie verlaengert wird, d.h.
  // dass ein Umlaut stets durch hoechstens zwei Zeichen erstetzt wird!

  char *quelle = puffer;
  char *ziel = puffer;

  while (*quelle) {
    if (*quelle != '~') *ziel++ = *quelle++; // Einfach uebertragen
    else {
      char *ersatz = dr_umlaut_sequenz(*(quelle+1));
      strncpy(ziel, ersatz, strlen(ersatz)); // strncpy, wegen dlem Stringende!
      ziel += strlen(ersatz);
      quelle += 2;
    }
  }
  *ziel = 0; // Ende muss natuerlich auch gesetzt werden!
}



/**---------------------------------------------------------------------------
  * dr_strip_lf_cr()
  * 
  * Ersetzt alle LF durch Space und alle CR durch garnichts.
  *
  * @param
  * char *puffer:	Datenbereich mit einem String, in dem ersetzt werden
  * soll.
  ---------------------------------------------------------------------------*/
void dr_strip_lf_cr(char *puffer)
{
  // Alle CR entfernen und LF durch Spaces ersetzen.

  char *quelle = puffer;
  char *ziel = puffer; // Klappt nur, da nie verlaengert wird!

  while (*quelle) {
    if (*quelle == '\n') *ziel++ = ' ';
    else if (*quelle != 13) *ziel++ = *quelle;
    quelle++;
  }
  *ziel = 0; // String wieder beenden.
}


long dr_anzahl_zeilen(char *text)
{
  long anz = 0, z=0;
  while (*text) {
    if (!isspace(*text) && !z) {
      anz++;
      z = 1;
    }
    else if (*text == '\n') z=0;
    text++;
  }
  return anz;
}
