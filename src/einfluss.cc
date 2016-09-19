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
  * MODUL:               einfluss.C / EINFLUSS.CPP
  * AUTOR/DATUM:         Mathias Kettner, 20. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Enthaelt alle Funktionen zum Typ EINFLUSS.
//
// **************************************************************************

#include <string.h>

#include "objekt.h"
#include "einfluss.h"
#include "laengen.h"
#include "kompatib.h"
#include "alg.h"
#include "log.h"

extern EINFLUSS_LISTE globale_einfluss_menge;

/**---------------------------------------------------------------------------
  * EINFLUSS::EINFLUSS(..1..)      // constructor
  * 
  * Konstruktor der Klasse der Einfluesse. Durch den Konstruktor wird
  * der Einfluss gleich voll spezifiziert und in die globale Einfluss-
  * liste eingetragen. Es ist also nicht moeglich, einen Einfluss zu
  * schaffen, der 'leer im Raum schwebt'. Einfluesse koennen im ue-
  * brigen nur von Objekten ausgeuebt werden.
  * 
  * @param
  * obj:            Objekt, dass den Einfluss ausloest.
  * art:            Art des Einflusses. Die Art ist eine Zeichenkette,
  * die nur aus Buchstaben, Ziffern und Unterstrichen
  * bestehen darf.
  * ziel:           Zielspezifikation laut ZIELGRUPPE
  * par:            Parameter des Einflusses. Jeder Einfluss kann
  * durch einen Parameterstring genauer spezifiziert
  * werden.
  ---------------------------------------------------------------------------*/
EINFLUSS::EINFLUSS(OBJEKT *obj,  char *art, char *ziel, char *par)
{
  aktiv = 1;
  beeinflusser = obj;
  art_des_einflusses = mystrdup(art);
  parameter = mystrdup(par);
  zielgruppe = new ZIELGRUPPE(ziel);
  globale_einfluss_menge.insert(this); // global Merken
}


/**---------------------------------------------------------------------------
  * EINFLUSS::EINFLUSS(..2..)      // construktor
  * 
  * Zweite Version des Konstruktors, der nur von der Funktion
  * EINFLUSS_LISTE::laden() aufgerufen wird und die Zielgruppe nicht
  * in Form von drei Einzelstrings angegeben wird, sondern in Form eines
  * Zeigers auf eine fertig eingerichtete Zielgruppe.
  *
  * @param
  * obj:            Beeinflussendes Objekt
  * art:            Art des Einflusses
  * par:            Parameter des Einflusses oder NULL
  * ziel:           Zeiger auf die Zielgruppe des Einflusses
  * a:              Aktivierung des Einflusses (0 oder 1)
  ---------------------------------------------------------------------------*/
EINFLUSS::EINFLUSS(OBJEKT *obj, char *art, char *par,
					     ZIELGRUPPE *ziel, short a)
{
  aktiv = a;
  beeinflusser = obj;
  art_des_einflusses = mystrdup(art);
  parameter = mystrdup(par);
  zielgruppe = ziel;
  globale_einfluss_menge.insert(this);
}


/**---------------------------------------------------------------------------
  * EINFLUSS::~EINFLUSS()           // destructor
  * 
  * Zerstoert einen Einfluss, hebt ihn korrekt auf. Das Entfernen aus
  * der Liste geschieht durch den Destruktor der Basisklasse.
  ---------------------------------------------------------------------------*/
EINFLUSS::~EINFLUSS()
{
  delete zielgruppe;
  myfree(art_des_einflusses);
  myfree(parameter);
}


/**---------------------------------------------------------------------------
  * EINFLUSS_LISTE::finde_einfluss()
  * 
  * Durchsucht die Einflussliste nach einem Einfluss einer bestimmten
  * Art, der auf ein bestimmtes Zielobjekt wirken soll.
  *
  * @param
  * ziel:           Objekt, auf das der Einfluss ausgeuebt wird.
  * art:            Art des Einflusses.
  *
  * @return
  * Zeiger auf die Einflussstruktur, wenn ein solcher Einfluss existiert,
  * NULL, wenn nicht.
  ---------------------------------------------------------------------------*/
EINFLUSS *EINFLUSS_LISTE::finde_einfluss(OBJEKT *ziel, char *art)
{
  EINFLUSS_FIND_SPEC_STR finddata;
  finddata.obj = ziel;
  finddata.art = art;
  return (EINFLUSS *)find((void *)&finddata);
}


/**---------------------------------------------------------------------------
  * EINFLUSS::matches()
  * 
  * Stellt fest, ob ein Einfluss bestimmten Bedingungen genuegt. Die
  * Funktion wird von der Listenklasse aufgerufen, die damit fest-
  * stellt, ob es sich um den (einen) gesuchten Einfluss handelt.
  *
  * @param
  * data:           (void *)Zeiger auf eine Einflussspezifikation.
  *
  * @return
  * 1, falls es sich um den gesuchten Einfluss handelt, 0 sonst.
  ---------------------------------------------------------------------------*/
short EINFLUSS::matches(void *data)
{
  if (!aktiv) return 0; // Nur aktive Einfluesse gelten.
  EINFLUSS_FIND_SPEC_STR *finddata = (EINFLUSS_FIND_SPEC_STR *)data;
  if (strcmp(finddata->art, art_des_einflusses)) return 0;
  return zielgruppe->zielt_von_auf(beeinflusser, finddata->obj);
}


/**---------------------------------------------------------------------------
  * EINFLUSS_LISTE::zusammenfassen()
  * 
  * Mit der Funktion finde_einfluss() wird stets implizit angenommen,
  * dass sich mehrere Einflusse der gleichen Art auf das selbe Objekt
  * sich genauso auswirken wie ein einziger, da bei Finden des ersten
  * Einflusses der gesuchten Art die Suche abgebrochen wird.
  * Sollen sich jedoch mehrere Einfluesse der gleichen Art ueberlagern,
  * so kann dazu diese Funktion verwendet werden. Die Art der Ueber-
  * lagerung wird (wohlgemerkt!) nicht beim Setzen, sondern beim Ab-
  * fragen der Einflusse angegeben. Gearbeitet wird grundsaetzlich
  * mit Einfluessen, die sich nach long konvertieren lassen. Fliess-
  * kommazahlen werden bei der Rechnung in Ganzzahlen umgewandelt
  * (Nachkommateil geht verloren).
  *
  * @param
  * ziel:           Objekt, das die Einflusse auf sich pruefen will
  * art:            Art der gesuchten Einfluesse
  * modus:          Gibt die Art des Ueberlagerung an:
  *
  * EINFLUSS_SUMME:         Summiert alle Werte
  * EINFLUSS_PRODUKT:       Bildet das Produkt ueber alle Werte
  * EINFLUSS_MAXIMUM:       Ermittelt den maximalen Wert
  * EINFLUSS_MINIMUM:       Ermittelt den minimalen Wert
  *
  * @return
  * Je nach modus berechneter Wert. Existiert kein Einfluss der gesuchten
  * art ist das Ergebnis 0 (Summe, Maximum), 2^31-1 (Minimum),
  * bzw. 1 (Produkt).
  ---------------------------------------------------------------------------*/
long EINFLUSS_LISTE::zusammenfassen(OBJEKT *ziel, char *art, short modus)
{
  long ergebnis = 0;
  if (modus == EINFLUSS_PRODUKT) ergebnis = 1; // Sonst kommt immer 0 'raus!
  else if (modus == EINFLUSS_MINIMUM) ergebnis = 256*256*128-1;

  // Dazu durchsuche ich die ganze Liste.
  EINFLUSS *suche = (EINFLUSS *)first();
  while (!suche->is_tail())
  {
    if (suche->aktiv && !strcmp(suche->art_des_einflusses, art)) {
      if (suche->zielgruppe->zielt_von_auf(suche->beeinflusser, ziel))
      {
	switch (modus) {
	  case EINFLUSS_SUMME:
		ergebnis += atol(suche->parameter);
		break;

	  case EINFLUSS_PRODUKT:
		ergebnis *= atol(suche->parameter);
		break;

	  case EINFLUSS_MAXIMUM:
		ergebnis = MAX(atol(suche->parameter), ergebnis);
		break;

	  case EINFLUSS_MINIMUM:
		ergebnis = MIN(atol(suche->parameter), ergebnis);
	} // switch...
      } // ziel auf...
    } // art stimmt

    suche = (EINFLUSS *)suche->next();
  } // while()...

  return ergebnis;
}


char *EINFLUSS_LISTE::zusammenfassen(OBJEKT *ziel, char *art,
   EIN_ZUS_FKT funktion)
{
  // Anstatt die Werte zu addieren oder aenliches, rufe ich eine Funktion
  // auf, die direkt auf den Parametern der Einfluesse operiert und vom
  // Aufrufer angegeben werden kann. Sie muss zweistellig, assozialtiv und
  // kommutativ sein. Als Startwert bekommt sie immer den NULL-String fuer
  // das erste Argument.

  char *zwischenergebnis = NULL;

  // Der Vorgang geht im wesentlichen wie bei der anderen Zusammenfassen-
  // Funktion..

  EINFLUSS *suche = (EINFLUSS *)first();
  while (!suche->is_tail())
  {
    if (suche->aktiv && !strcmp(suche->art_des_einflusses, art))
    {
      if (suche->zielgruppe->zielt_von_auf(suche->beeinflusser, ziel))
      {
	// Jetzt rufe ich die Custom-funktion auf.
	zwischenergebnis = funktion(zwischenergebnis, suche->parameter);
      }
    }
    suche = (EINFLUSS *)suche->next();
  }

  return zwischenergebnis;
}


/**---------------------------------------------------------------------------
  * EINFLUSS_LISTE::kommando_fuer_jeden_einfluss()
  * 
  * Ein Objekt kann mit dieser Funktion erreichen, dass fuer jeden
  * Einfluss einer bestimmten Art auf ihn ein bestimmtes Kommando
  * aufgerufen wird. Als Parameter des Kommandos bekommt er den
  * Einflussparameter.
  *
  * @param
  * OBJEKT *objekt: Objekt, auf den die Einfluesse wirken und
  * gleichzeitig das Objekt, dessen Kommando
  * ausgefuehrt wird.
  *
  * char *art:          Art der Einfluesse, nach denen gesucht wird.
  * char *kommando:     Kommando, das aufgerufen werden soll.
  *
  * @return
  * short: Summe der Ruckgabewerte aller aufgerufenen Kommandos.
  ---------------------------------------------------------------------------*/
short EINFLUSS_LISTE::kommando_fuer_jeden_einfluss(OBJEKT *objekt,
			char *art, char *kommando)
{
  short rwert = 0;

  EINFLUSS *einfluss;
  einfluss = (EINFLUSS *)first();
  while (!einfluss->is_tail())
  {
    if (einfluss->aktiv                             // Ist aktiv.
     && !strcmp(einfluss->art_des_einflusses, art) // Art stimmt
     && einfluss->zielgruppe->zielt_von_auf(einfluss->beeinflusser, objekt))
    {
	rwert += objekt->kommando(kommando, (void *)einfluss->parameter);
    }

    einfluss = (EINFLUSS *)einfluss->next();
  } // while (alle Einfluesse ueberhaupt)

  return rwert;
}


/**---------------------------------------------------------------------------
  * EINFLUSS_LISTE::einfluesse_loeschen()
  * 
  * Behandelt aus der Einflussliste eine Reihe von Einfluessen, die
  * bestimmten Kriterien genuegen. Die beiden Kriterien sind gegeben
  * durch Angabe des beeinflussenden Objektes und durch Angabe der
  * Art des Einflusses, die aber auch durch NULL ersetzt werden kann.
  * In diesem Fall werden alle Einfluesse des betreffenden Objektes
  * geloescht. Als Art der Behandlung kann aus "Loeschen", "Suspendieren"
  * und "Aktivieren" gewaehlt werden.
  *
  * @param
  * von:            Objekt, von dem die zu loeschenden Einfluesse
  * ausgehen.
  * art:            Art der zu loeschenden Beeinflussungen oder
  * NULL, wenn die Art egal ist.
  * modus:          0, wenn geloescht werden soll,
  * 1, wenn suspendiert werden soll,
  * 2, wenn aktiviert werden soll.
  ---------------------------------------------------------------------------*/
void EINFLUSS_LISTE::einfluesse_loeschen(OBJEKT *von, char *art,
							       short modus)
{
  EINFLUSS *akt, *zu_loeschen;
  akt = (EINFLUSS *)first();
  while (!akt->is_tail())
  {
    if (akt->beeinflusser == von) {
      if (!art || !strcmp(akt->art_des_einflusses,art)) {
	zu_loeschen = akt;
	akt = (EINFLUSS *)akt->next();

	if (!modus) delete zu_loeschen;    // Loeschen
	else zu_loeschen->aktiv = modus-1; // Suspendieren oder Aktivieren

	continue;
      }
    }
    akt = (EINFLUSS *)akt->next();
  }
}


/**---------------------------------------------------------------------------
  * EINFLUSS_LISTE::speichern()
  * 
  * Speichert eine ganze Einflussliste in eine Datei ab (ASCII).
  *
  * @param
  * file:           Filepointer (fopen()) falls eine schon offene
  * Datei verwendet werden soll, sonst NULL.
  * name:           Name, falls eine neue Datei zum Speichern geoeffnet
  * werden soll.
  *
  * @return
  * 1, falls ein Fehler auftrat.
  * 0, wenn alles glatt lief.
  ---------------------------------------------------------------------------*/
short EINFLUSS_LISTE::speichern(FILE *file, char *filename)
{
  if (file) filename=NULL; // Daran erkenne ich, ob ich flose() machen muss!
  else {
     verzeichnis_gewaehrleisten(filename);
     file = fopen(filename, "w");
  }
  if (!file) return 1;

  short status = 0;

  EINFLUSS *einfl = (EINFLUSS *)first();

  while (!einfl->is_tail())
  {
     fprintf(file, "%ld %s %s ",long(einfl->aktiv), einfl->beeinflusser->name,
		       einfl->art_des_einflusses);
     fputstring(file, einfl->parameter, " ");
     einfl->zielgruppe->speichern(file);
     if (ferror(file)) {
       status = 1;
       break;
     }
     einfl = (EINFLUSS *)einfl->next();
  }

  if (filename) fclose(file);
  return status;
}


/**---------------------------------------------------------------------------
  * EINFLUSS_LISTE::laden()
  * 
  * Loescht zuerst alle Daten aus der Einflussliste und laedt dann
  * neue Einfluesse aus einer Datei. Jeder Einfluss besteht aus
  * sechs ASCII-Zeilen. Die einflussausloesenden Objekte sind mit
  * ihrem Namen referiert (Beim Abspeichern). Deshalb darf die
  * Beeinflussungsliste erst NACH dem Laden der Objekte geschehen,
  * wenn ein neues Objektmodell geladen werden soll!
  *
  * @param
  * file:           Filepointer (fopen()), falls aus einer schon
  * offenen Datei geladen werden soll, sonst NULL.
  * name:           Falls kein Filepointer angegeben wird, so muss
  * die Datei hier ueber einen Namen spezifiziert
  * werden.
  *
  * @return
  * 0, falls das Laden vollstaendig geklappt hat.
  * 1 im Fehlerfalle.
  ---------------------------------------------------------------------------*/
short EINFLUSS_LISTE::laden(FILE *file, char *filename)
{
  if (file) filename=NULL;
  else file = fopen(filename, "r");
  if (!file) return 1;

  short status = 0;
  clear(); // Alle bisherigen Einfluesse loeschen();

  char *art_puffer = new char[MAX_LAENGE_EINFLUSSART+2];
  char *par_puffer = new char[MAX_LAENGE_EINFLUSSPARAMETER+2];
  OBJEKT *beeinflusser;
  long aktiv_long;

  if (art_puffer && par_puffer) while (!feof(file))
  {
    fscanf(file, "%ld", &aktiv_long); // Aktivation

    // Jetzt erlaube ich mir, den par_puffer kurz fuer das Laden des Namens
    // des Beeinflussers zu entfremden.

    fscanf(file,"%s",par_puffer); // Name des Beeinflussers
    if (ferror(file)) { status=1; break; }
    if (feof(file)) break; // Irgendein LF zuviel...

    beeinflusser = objekt_mit_namen(par_puffer);

    // Wenn der Beeinflusser nicht gefunden werden kann, dann kann irgend-
    // etwas nicht stimmen. Ich breche daher das Laden ab.

    if (!beeinflusser) {
	log('W', "Corrupted game file. Missing object '%s'", par_puffer);
	status = 1;
	break;
    }

    fscanf(file,"%s",art_puffer); // Art des Einflusses
    if (ferror(file)) { status=1; break; }

    fscanf(file,"%s",par_puffer); // Einflussparameter
    if (ferror(file)) { status=1; break; }
    par_puffer = string_to_wert(par_puffer);   // Quotezeichen expandieren

    new EINFLUSS(beeinflusser, art_puffer, par_puffer
	,new ZIELGRUPPE(file), aktiv_long);
  }
  else status = 1;
  if (par_puffer) delete par_puffer;
  if (art_puffer) delete art_puffer;
  if (filename) fclose(file);
  return status;
}
