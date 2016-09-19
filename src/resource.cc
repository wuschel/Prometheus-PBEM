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
  * MODUL:               resource.C / RESOURCE.CPP
  * AUTOR/DATUM:         Mathias Kettner, 1. Mai 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
// 
//	Definiert die Funktionen der Klassse RESOURCE_VEKTOR.
// 
// **************************************************************************

#include <ctype.h>

#include "resource.h"
#include "prom.h"
#include "alg.h"
#include "enzyklop.h"
#include "laengen.h"
#include "kompatib.h"
#include "log.h"

RESOURCE_VEKTOR::RESOURCE_VEKTOR(long wert, char symbol)
{
  init();
  char string[13];
  sprintf(string, "%ld%c", wert, toupper(symbol));
  setzen_auf(string);
}

void RESOURCE_VEKTOR::init()
{
  anzahl = enzyklopaedie()->anzahl_verschiedener_resourcen;
  lager = new long[anzahl];
}

void RESOURCE_VEKTOR::setzen_auf(const RESOURCE_VEKTOR& a)
{
  anzahl = a.anzahl;
  for (int i=0; i<anzahl; i++) lager[i] = a.lager[i];
}

void RESOURCE_VEKTOR::setzen_auf(char *string)
{
  // Erstmal alles loeschen...
  for (int i=0; i<anzahl; i++) lager[i]=0;

  // Jetzt muss ich den String auswerten, falls einer vorhanden ist..
  if (string)
  {
//    char *resourcestring = string; // Merken fuer eine Fehlerausgabe

    // Es kommt immer eine Zahl gefolgt von einem Buchstaben, z.B. 20E-1F33N
    // Ich lege einen Puffer mit etwa 15 Zeichen Platz an. Er muesste
    // fuer eine Ganzzahl reichen....
    char zahlstring[64]; // Hier baue ich stets die aktuelle Zahl auf
    char *zahlzeiger;    // Zeigt auf die aktuelle Stelle in zahlstring[]


    // Jetzt beginne ich eine Schleife, die alle Eintraege im String durch-
    // geht.
    while (*string) {
      zahlzeiger = zahlstring;

      if (*string=='-') *zahlzeiger++ = *string++; // Vorzeichen

      while (isdigit(*string) || tolower(*string)=='o') {
         if (tolower(*string)=='o') {
           *zahlzeiger++ = '0';
           string++;
         }
	 else *zahlzeiger++ = *string++;
      }
      *zahlzeiger=0; // Zahlenstring beenden

      // Falls der String nun schon auf 0 zeigt, ist ein Fehler aufgetreten!

      if (!*string) {
	break;
      }

      // Jetzt muss ich anhand des Buchstaben, der nun kommt, feststellen,
      // um welche Resourceklasse es sich handelt und zur richten Klasse
      // den Wert im Zahlstring addieren

      RESOURCE_ENZ *res=(RESOURCE_ENZ *)enzyklopaedie()->resourcenliste.first();
      for (short klasse=0; !res->is_tail();
	     res=(RESOURCE_ENZ *)res->next(), klasse++)
      {
	if (toupper(res->symbol) == toupper(*string)) {
	   lager[klasse] += atol(zahlstring);
	   break;
	}
      }
      string++; // Symbol ueberlesen
    } // while (*string)

  } // if string

  // So. Das wars...

}


void RESOURCE_VEKTOR::einheitsvektor_aus(char *symbole)
{
  while (*symbole) operator [] (*symbole++) = 1;
}


long RESOURCE_VEKTOR::betrag()
{
  long betrag = 0;
  for (int i=0; i<anzahl; i++) betrag += ABS(lager[i]);
  return betrag;
}


long RESOURCE_VEKTOR::hemmingnorm() // Anzahl Elemente != 0
{
  long norm = 0;
  for (int i=0; i<anzahl; i++) norm += (lager[i] != 0);
  return norm;
}


short RESOURCE_VEKTOR::ist_negativ()
{
  for (int i=0; i<anzahl; i++) if (lager[i] < 0) return 1;
  return 0;
}

void RESOURCE_VEKTOR::addiere(const RESOURCE_VEKTOR& a)
{
  for (int i=0; i<anzahl; i++) lager[i] += a.lager[i];
}

void RESOURCE_VEKTOR::subtrahiere(const RESOURCE_VEKTOR& a)
{
  for (int i=0; i<anzahl; i++) lager[i] -= a.lager[i];
}

void RESOURCE_VEKTOR::multipliziere_mit(long faktor)
{
  for (int i=0; i<anzahl; i++) lager[i] *= faktor;
}

void RESOURCE_VEKTOR::multipliziere_mit_float(float faktor) // rundet nach oben auf!
{
    for (int i=0; i<anzahl; i++) 
	lager[i] = (int)ceil( ((float)lager[i] * faktor) - 0.0001 );
}

void RESOURCE_VEKTOR::teile_durch(long divisor)
{
  for (int i=0; i<anzahl; i++) lager[i] /= divisor;
}


void RESOURCE_VEKTOR::negieren()
{
  for (int i=0; i<anzahl; i++) lager[i] = -lager[i];
}


void RESOURCE_VEKTOR::kompensieren_aus(RESOURCE_VEKTOR& quelle)
{
  for (int i=0; i<anzahl; i++) {
    if (lager[i] > quelle.lager[i]) {
      lager[i] -= quelle.lager[i];
      quelle.lager[i] = 0;
    }
    else {
      quelle.lager[i] -= lager[i];
      lager[i] = 0;
    }
  }
}


void RESOURCE_VEKTOR::begrenzen_auf(RESOURCE_VEKTOR& g)
{
  for (int i=0; i<anzahl; i++)
      if (lager[i] > g.lager[i]) lager[i] = g.lager[i];
}

void RESOURCE_VEKTOR::begrenzen_auf(long max) // Auf einen Betrag begrenzen
{
  if (max >= betrag()) return; // Passt rein.
  long diff = betrag()-max;
  while (diff) {
    for (int i=0; i<anzahl && diff; i++) {
      if (lager[i]) {
	lager[i]--;
	diff--;
      }
    }
  }
}


int RESOURCE_VEKTOR::passt_in(RESOURCE_VEKTOR& v)
{
  for (int i=0; i<anzahl; i++) if (lager[i] > v.lager[i]) return 0;
  return 1;
}

int RESOURCE_VEKTOR::ist_null()
{
  return !betrag();
}


char *RESOURCE_VEKTOR::to_string()
{
  static char string[MAX_LAENGE_RESOURCEZEILE + 1];
  string[0] = 0;

  RESOURCE_ENZ *res =
     (RESOURCE_ENZ *)enzyklopaedie()->resourcenliste.first();
  for (long klasse=0; !res->is_tail();
	res=(RESOURCE_ENZ *)res->next(), klasse++)
  {
    if (lager[klasse]) { // String verlaengern
       char add[16];
       sprintf(add, "%ld%c", lager[klasse], res->symbol);
       strcat(string, add);
    }
  }

  // Falls der String leer ist, ersetze ich ihn durch den String ".",
  // der den leeren Resourcestring darstellt.

  if (!string[0]) return ".";
  else return string; // Bleibt Eigentum von dieser Funktion!!!
}

long& RESOURCE_VEKTOR::operator [] (char symbol)
{
  RESOURCE_ENZ *res =
     (RESOURCE_ENZ *)enzyklopaedie()->resourcenliste.first();
  for (long klasse=0; !res->is_tail();
	res=(RESOURCE_ENZ *)res->next(), klasse++)
  {
    if (res->symbol == symbol) return lager[klasse];
  }

  log('K', "Unknown resource symbol '%c'. Check game configuration file",
      symbol);
  return lager[0];
}
