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
  * MODUL:               prom.C  /  PROM.CPP
  * AUTOR/DATUM:         Mathias Kettner, 4. Mai 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Enthaelt Funktionen ausserhalb von Klassen, die von allgemeinem
//      Interesse fuer alle Objekte sind, die im Zusammenhang mit des
//      speziellen Objektmodells Prometheus stehen.
//
// **************************************************************************

#include <string.h>

#include "prom.h"
#include "alg.h"
#include "resource.h"
#include "laengen.h"
#include "einheit.h"
#include "kompatib.h"
#include "drucker.h"
#include "layout.h"
#include "enzyklop.h"
#include "listmac.h"
#include "uhr.h"
#include "log.h"

extern DOUBLIST globale_objekt_menge;
extern UHR *g_uhr; // von main.cpp

/**---------------------------------------------------------------------------
  * enzyklopaedie()
  * 
  * Kleine Hilfsfunktion, die in der Objektmenge nach dem Objekt mit
  * dem Namen "Enzyklopaedie" sucht und einen Zeiger darauf zurueck-
  * gibt. Wenn die Enzyklopaedie nicht gefunden wurde, dann wird eine
  * Fehlermeldung ausgegeben. Nach Konvention muss der Aufrufer fuer
  * diesen Fall keine Abfrage vorsehen. Diese Funktion gibt eine Feh-
  * lermeldung auf dem Bildschirm aus, damit der Bediener oder Pro-
  * grammierer weiss, warum sein Programm ausgestiegen ist (was es
  * dann naemlich mit hoher Wahrscheinlichkeit ohnehin tut).
  *
  * @return
  * ENZYKLOPAEDIE *: Zeiger auf das Objekt oder NULL.
  ---------------------------------------------------------------------------*/
ENZYKLOPAEDIE *enzyklopaedie()
{
  ENZYKLOPAEDIE *enz =
	 (ENZYKLOPAEDIE *)objekt_mit_namen("Enzyklopaedie");
  if (!enz) {
      log('I', "enzyklopaedie(): Die Enzyklopaedie ist nicht vorhanden");
  }
  return enz;
}


/**---------------------------------------------------------------------------
  * lexikon_eintrag()
  * 
  * Sucht in der Enzyklopaedie im Kapitel LEXIKON: nach einem bestimmten
  * Begriff und holt dort einen bestimmten Eintrag. Funktioniert nur,
  * wenn ein Objekt mit dem Namen Enzyklopaedie existiert, das auch
  * eine Enzyklopaedie ist.
  *
  * @param
  * char *name:   Name des Stichwortes
  * short nummer: Laufende Nummer des Eintrag (Bei jedem Stichwort
  * beginnt die Zaehlung von 1).
  *
  * @return
  * char * Zeiger auf statischen Bereich, in dem der Antwortstring steht.
  ---------------------------------------------------------------------------*/
char *lexikon_eintrag(char *name, short nummer)
{
  ENZYKLOPAEDIE *enz = enzyklopaedie();
  if (enz) return enz->lexikon_eintrag(name, nummer);
  else return NULL;
}

long lexikon_long(char *k, short s)
{ 
  return myatol(lexikon_eintrag(k,s));
};


/**---------------------------------------------------------------------------
  * konjugation()
  * 
  * Bildet eine Nominalphrase unter Zuhilfenahme des Lexikons und der
  * deutschen Grammatik. Es kann unter verschiedenen Phrasen ausge-
  * waehlt werden.
  *
  * @param
  * short phrase:   Eine Kombination von Konstanten wie in prom.h
  * definiert, z.B. NOMINATIV|SINGULAR
  *
  * @return
  * char *antwort: static und in Kleinschrift.
  ---------------------------------------------------------------------------*/
char *konjugation_deutsch(char *name, short phrase)
{
  // Fuer die Bestimmung des Artikels habe ich zwei statische Tabellen...

  // Artikel des Singular

  static char *artikel_tabelle[3][2][4] = {
   { {"der ","des ","dem ","den "}, {"ein ", "eines ","einem ","einen "} },
   { {"die ","der ","der ","die "}, {"eine ","einer ","einer ","eine "} },
   { {"das ","des ","dem ","das "}, {"ein ", "eines ","einem ","ein "} }
  };

  // Artikel des Plural. Hier gibt es keinen unbestimmten Artikel.

  static char *plural_artikel[4] = { "die ", "der ", "den ", "die " };

  // In dieser kleinen Tabelle steht zu jeder Phrase der richtige Fall.
  // 0 bedeutet Plural.

  short fall = phrase & (NOMINATIV | GENITIV | DATIV | AKKUSATIV);

  // Der erste Eintrag im Lexikon bestimmt das Geschlecht, die Anzahl
  // und ueber unbestimmter/bestimmter Artikel (das Rad / eine Kornkammer)

  char *spez = lexikon_eintrag(name, 1);
  
  // Wenn ich keine Spezifikation in der Enzyklopaedie finden kann, dann
  // ist das zwar ein Fehler, aber kein schlimmer, und ich gebe den Namen
  // zurueck, damit es nicht so ganz falsch ist.
  
  if (!spez) return name;

  short geschlecht;
  if       (spez[0] == 'm') geschlecht = 0;
  else  if (spez[0] == 'w') geschlecht = 1;
  else                      geschlecht = 2;

  short plural; // Bedeutet, dass das Wort in seiner Grundform Plural ist.
  if (spez[1] == 's') plural = 0;
  else                plural = 1;

  short unbestimmt;
  if (spez[2] == 'u') unbestimmt = 1;
  else                unbestimmt = 0;

  // Jetzt bereite ich den Artikel vor.
  char *artikel = "";
  if (plural || phrase&PLURAL) {
    if (!unbestimmt) artikel = plural_artikel[fall-1];
    else artikel="";
  }

  else artikel = artikel_tabelle[geschlecht][unbestimmt][fall-1];

  static char antwort[3][100]; // Zur Vorbereitung der Antwort
  static int selektor = 0;

  selektor = (selektor+1) % 3; // Immer ein Ausgabefeld weiterschalten.

  switch (phrase) {
   case NOMINATIV | SINGULAR:
     sprintf(antwort[selektor], "%s%s", artikel, lexikon_eintrag(name, 2));
     return antwort[selektor];

   case GENITIV | SINGULAR:
     sprintf(antwort[selektor], "%s%s", artikel, lexikon_eintrag(name, 3));
     return antwort[selektor];

   case DATIV | SINGULAR:
     sprintf(antwort[selektor], "%s%s", artikel, lexikon_eintrag(name, 4));
     return antwort[selektor];

   case AKKUSATIV | SINGULAR:
     sprintf(antwort[selektor], "%s%s", artikel, lexikon_eintrag(name, 5));
     return antwort[selektor];

   case NOMINATIV | PLURAL:
   case GENITIV | PLURAL:
   case AKKUSATIV | PLURAL:
     sprintf(antwort[selektor], "%s%s", artikel, lexikon_eintrag(name, 6));
     return antwort[selektor];

   case DATIV | PLURAL:
     sprintf(antwort[selektor], "%s%s", artikel, lexikon_eintrag(name, 7));
     return antwort[selektor];

   case GENITIV_PHRASE:
     if (plural) sprintf(antwort[selektor],"von %s", lexikon_eintrag(name, 7));
     else sprintf(antwort[selektor], "%s%s", artikel, lexikon_eintrag(name, 3));
     return antwort[selektor];
  }

  return name; // Fehlerhafter Aufruf.
}

// Aussehen des Lexikons fuer die Englische Version:
//
// Wheel\the\\ 
// Skyscraper\a\Skyscrapers\ 
// Artillery\an\Artilleries\ 
// Settlers\\Settler_Units\ 

char *konjugation_english(char *name, short phrase)
{
    // Der erste Eintrag im Lexikon legt den Artikel fest. Dadurch ergibt
    // sich bestimmt, unbestimmt oder Pluralwort (siehe Beispiele 10 Zeilen
    // hoeher). Soll die Phrase ohnehin im Plural gebildet werden entfaellt
    // der Artikel.

    const char *artikel = (phrase & PLURAL) ? "" : lexikon_eintrag(name, 1);
  
    // Wenn ich keine Spezifikation in der Enzyklopaedie finden kann, dann
    // ist das zwar ein Fehler, aber kein schlimmer, und ich gebe den Namen
    // zurueck, damit es nicht so ganz falsch ist.
  
    if (!artikel) return name;

    static char antwort[3][100]; // Zur Vorbereitung der Antwort
    static int selektor = 0;

    selektor = (selektor+1) % 3; // Immer ein Ausgabefeld weiterschalten. HACK ;-)

    sprintf(antwort[selektor], "%s%s%s",
	    artikel,
	    mystrlen(artikel) > 0 ? " " : "",
	    phrase & PLURAL ? lexikon_eintrag(name, 2) : name);
    return antwort[selektor];
}


char *konjugation(char *name, short phrase)
{
    return L(konjugation_deutsch(name, phrase),
	     konjugation_english(name, phrase));
}

    
    


/**---------------------------------------------------------------------------
  * woertliche_objekt_aufzaehlung()
  * 
  * Diese Funktion erzeugt einen dynamischen String. Er enthaelt eine
  * Aufzaehlung einer Liste von Objekten, oder genauer: deren Gattungen.
  * Dabei wird die Liste zuerst nach dem Typ der Objekte und dann
  * nach den Gattungen alphabetisch sortiert und anschliessend werden
  * gleichnamige Objekte zusammengefasst. Eine Ausgabe koennte z.B.
  * lauten: "zwei Kasernen, eine Schule und eine Miliz".
  *
  * @param
  * DOUBLIST *objliste:     Liste der Objekte. Diese Liste wird
  * innerhalb dieser Funktion aus Effi-
  * zienzgruenden geleert(!), jedoch
  * nicht geloescht!
  *
  * short fall:                     NOMINATIV, GENITIV, DATIV, AKKUSATIV
  *
  * @return
  * char *: Dynamischer (!) String mit der Antwort. Ist die Liste leer,
  * so wird "" zurueckgegeben.
  ---------------------------------------------------------------------------*/
char *woertliche_objekt_aufzaehlung(DOUBLIST *objliste, short fall)
{
  if (objliste->is_empty()) return mystrdup("");

  // Zuerst sortiere ich die Liste mit meiner eigens dafuer angelegt Sort-
  // funktion:

  objliste->sort(sortfunction_woa);

  // Und nun alle der Reihe nach ausgeben. Ich baue das ganze in einem dy-
  // namische String mittels der Funktion myaddstr() auf.

  char *antwort = mystrdup(NULL);

  short erster_posten = 1;
  while (!objliste->is_empty()) {
    OBJEKT_LIST_NODE *objnode = (OBJEKT_LIST_NODE *)objliste->first();
    char *gattung = objnode->objekt->attribut("GATTUNG");
   
    // Objekte mit dem Attribut "BENENNUNG" haben ihre eigenen Ausgabe, und
    // wollen voneinander auch bei gleicher Gattung unterschieden werden.
    // (Sehenswuerdigkeiten)

    char *benennung = objnode->objekt->attribut("BENENNUNG");
    delete objnode;

    // Nun schaue ich, wieviele mit gleicher Gattung noch folgen...
    long anzahl = 1;
    if (!benennung) {
      while (!objliste->is_empty()) {
        OBJEKT_LIST_NODE *objnode = (OBJEKT_LIST_NODE *)objliste->first();
        if (!strcmp(gattung, objnode->objekt->attribut("GATTUNG"))) {
  	  anzahl ++;
	  delete objnode;
        }
        else break;
      }
    }

    // Ein 'und' steht zwischen dem letzten und dem vorletzten Eintrag,
    // jedoch nur, wenn die Liste nicht aus nur einem einzigen Eintrag
    // besteht. In den restlichen Faellen werden die Eintraege durch
    // Kommata getrennt.

    if (!erster_posten) { // Vor dem ersten Eintrag nicht ausgeben.
      if (!objliste->is_empty()) mystradd(antwort,", ");
      else mystradd(antwort, L(" und "," and "));
    }
    erster_posten = 0;

    if (benennung) mystradd(antwort, benennung);
    else {
      // Jetzt gebe ich je nach Anzahl oder Benennung aus.
      if (anzahl == 1) mystradd(antwort, konjugation(gattung, fall|SINGULAR));
      else {
        mystradd(antwort, nice_ltoa(anzahl)); // Anzahl ausgeschrieben
        mystradd(antwort, " ");
        mystradd(antwort, konjugation(gattung, fall|PLURAL));
      }
    }
  } // while (!is_empty())

  return antwort;
}


/**---------------------------------------------------------------------------
  * sortfunction_woa()
  * 
  * Sortfunktion fuer die obige Funktion woertliche_objekt_aufzaehlung()
  * im Sinne von DOUBLIST::sort()
  *
  * PARAMETER, @return Signatur wie bei DOUBLIST deklariert.
  ---------------------------------------------------------------------------*/
short sortfunction_woa(DOUBLIST_NODE *a1,DOUBLIST_NODE *a2, void *)
{
  OBJEKT_LIST_NODE *node1, *node2;
  node1 = (OBJEKT_LIST_NODE *)a1;
  node2 = (OBJEKT_LIST_NODE *)a2;

  int compare = strcmp(node1->objekt->attribut("TYP")
	       ,node2->objekt->attribut("TYP"));
  if (compare) return compare;
  else return strcmp(node1->objekt->attribut("GATTUNG")
	       ,node2->objekt->attribut("GATTUNG"));
}


/**---------------------------------------------------------------------------
  * jahr_in_zug()
  * 
  * Ermittelt die Jahreszahl des ersten Jahres eines Zuges. Ein negati-
  * ver Wert bedeutet eine Jahreszahl vor Christus.
  *
  * @param
  * long zugnummer:         Zugnummer
  *
  * @return
  * long: Jahreszahl.
  ---------------------------------------------------------------------------*/
long jahr_in_zug(long zugnummer)
{
  static short jahr[] = 
   { -2000, -1800, -1600, -1400, -1200, -1000, -800, -600, -400, -200,
         0,   150,   300,   450,  600,  750,  900, 1000,  1100,  1200,
      1300,  1400,  1500,  1600, 1700, 1800, 1900, 1920,  1940,  1960,
      1980,  1985,  1990,  1992, 1994, 1996, 1998, 2000
  };
  
  if (zugnummer < 37) return jahr[zugnummer+1];
  else return zugnummer - 37 + 2001;
}


/**---------------------------------------------------------------------------
  * auswertung_in_zug()
  * 
  * Ermittelt, in welchen Zuegen eine Punkteauswertung stattfinden soll.
  *
  * @param
  * long zugnummer:         Zugnummer
  *
  * @return
  * 1 bei ja, 0 bei nein.
  ---------------------------------------------------------------------------*/
short auswertung_in_zug(long zugnummer)
{
  return (zugnummer >= 2); // Jede Runde, aber der zweiten.

  // Auswertung in Runden 5,10,15,20,21,22,23,24,...
  if (zugnummer < 5) return 0;
  else if (zugnummer <= 20) return (zugnummer % 5 == 0);
  else return 1;
}


/**---------------------------------------------------------------------------
  * punkte_fuer_gewinn()
  * 
  * Gibt aus, wieviel Punkte ein Spieler zum gewinnen braucht.
  ---------------------------------------------------------------------------*/
long punkte_fuer_gewinn()
{
  long anzahl_spieler = anzahl_spieler_angetreten();
  
  if (anzahl_spieler <= 10) return 3500;
  else if (anzahl_spieler <= 30) return 4000 - anzahl_spieler * 50;
  else if (anzahl_spieler <= 50) return 3250 - anzahl_spieler * 25;
  else return 2000;
}


/**---------------------------------------------------------------------------
  * anzahl_staaten()
  * 
  * Zaehlt die Anzahl, der Objekte vom Typ STAAT.
  ---------------------------------------------------------------------------*/
long anzahl_staaten()
{ 
  long anzahl=0;
  FOR_EACH_OBJEKT_IN (&globale_objekt_menge)
  DO ( anzahl += !strcmp(objekt->a_typ(), "STAAT"); )
  return anzahl;
}
    
  
/**---------------------------------------------------------------------------
  * anzahl_spieler_angetreten()
  * 
  * Zaehlt die Anzahl der Staaten im Spiel.
  ---------------------------------------------------------------------------*/
long anzahl_spieler_angetreten()
{
  return anzahl_staaten();
}


/**---------------------------------------------------------------------------
  * ein_zus_add_resource()
  * 
  * Dies ist eine sehr spezielle Funktion. Sie dient einzig dazu, der
  * Funktion EINFLUSS_LISTE::zusammenfassen() als Argument ueber-
  * geben zu werden. Informell stellt die Funktion die Addition zweier
  * als Strings gegebener Resourcevektoren dar. Letztendliches Ziel ist
  * die automatische Aufsummierung einer Klasse von Einfluessen, deren
  * Parameter Resourcevektoren sind.
  *
  * Ein Beispiel fuer einen Aufruf ist in der Funktion STADT::rest-
  * liche_resourcen_speichern(), wo die Speicherkapazitaet einer Stadt
  * durch einen bestimmten Einflusstyp vergroessert wird.
  *
  * Die Signatur der Funktion ist durch die Deklaration von EINFLUSS_
  * LISTE::zusammenfassen() festgelegt:
  *
  * @param
  * char *res1, *res2:      Zu addierende Resourcestrings
  *
  * @return
  * char *: Ergebnis als statischer String, der dieser Funktion ge-
  * hoert (und nicht irgendeiner ...to_string() Funktion.)
  ---------------------------------------------------------------------------*/
char *ein_zus_add_resource(char *res1, char *res2)
{
  // Ich muss die beiden Resourceangaben addieren und das Ergebnis zurueck-
  // geben. In der Mitte muss ich die Strings in RESOURCE_VEKTOR um-
  // rechnen und das Ergebnis wieder zurueck.

  static char ergebnis[MAX_LAENGE_RESOURCEZEILE+1];
  RESOURCE_VEKTOR erg(res1);
  RESOURCE_VEKTOR summand(res2);
  erg.addiere(summand);
  strcpy(ergebnis, erg.to_string());
  return ergebnis;
}


/**---------------------------------------------------------------------------
  * nach_verteidigungskraft_sortieren()
  * 
  * Sortiert eine Objektliste, deren Elemente vom Typ EINHEIT sein
  * muessen nach deren Werten des Infos Verteidigungskraft.
  * Die Einheit mit der hoechsten Verteidigungskraft steht in der
  * Liste an erster Stelle.
  *
  * @param
  * DOUBLIST *objliste: Zeiger auf die zu sortierende Liste.
  ---------------------------------------------------------------------------*/
void nach_verteidigungskraft_sortieren(DOUBLIST *objliste)
{
  // Zum Vergleich nehme ich die info-Funktion. Als Algorithmus nehme
  // ich Bubblesort, da ohnehin die Anzahl der Einheit im Durchschnitt
  // kaum groesser als 2 sein duerfte!

  if (objliste->number_of_elements() <= 1) return;

  OBJEKT_LIST_NODE *objnode;
  short alles_sortiert;

  do {
    alles_sortiert = 1;
    objnode = (OBJEKT_LIST_NODE *)objliste->first();
    while (!objnode->is_last())  // Beim letzten brauche ich nichts mehr vergl.
    {
      if (atol(objnode->objekt->info("VERTEIDIGUNGSKRAFT")) <
	atol(((OBJEKT_LIST_NODE *)objnode->next())->
  	objekt->info("VERTEIDIGUNGSKRAFT")))
      {
	DOUBLIST_NODE *rem = objnode->next();
	rem->remove();
	rem->insert_before(objnode);
	alles_sortiert = 0;
      }
      else objnode = (OBJEKT_LIST_NODE *)objnode->next();

    } // while
  } while (!alles_sortiert);

  // So. Nun muesste die Liste eigentlich sortiert sein!
}


/**---------------------------------------------------------------------------
  * reports_layouten_und_drucken()
  * 
  * Schreibt alle reports eines Objektes in eine temporaere Datei.
  * Anschliessend wird der Text blockbuendig formatiert (auf 93 Zeichen
  * pro Zeile) und mit der Courierschrift auf
  * dem Drucker ausgegeben (mit den LAYOUT-Funktionen).
  *
  * @param
  * OBJEKT *objekt:     Objekt, dessen Reports gedruckt werden
  * sollen.
  ---------------------------------------------------------------------------*/
void reports_layouten_und_drucken(OBJEKT *objekt)
{
  if (objekt->reportliste_leer()) return;
  
  // Ich schreibe zuerst alle Reports in eine temporaere Datei, die
  // ich dann mit dem Befehl dr_infodatei_formatieren() formatiere und
  // ausgebe.

  verzeichnis_gewaehrleisten(tempfile_name());
  FILE *file = fopen(tempfile_name(),"w");
  if (!file) {
      log('W', "Can't open tempfile '%s'. Reports for object '%s' will be missing",
	  tempfile_name(), objekt->name);
      return;
  }
  objekt->reportliste_in_file(file);
  fclose(file);

  // Und nun muss ich formatieren und ausgeben.
  char *formatiert = dr_infodatei_formatieren(tempfile_name(),93);
  long anzahl_zeilen = dr_anzahl_zeilen(formatiert);
  LAYOUT ly(dr_abschnitt(anzahl_zeilen * 0.45 + 0.15));
  dr_zeilenabstand(0.45);
  ly.text(0, 0.4, formatiert, "Courier", 10.066);
  ly.ausdrucken();
  delete formatiert;
}


/**---------------------------------------------------------------------------
  * nach_verteidigungskraft_sortieren()
  * 
  ---------------------------------------------------------------------------*/
char *tempfile_name(const char *variabel)
{
  return g_uhr->tempfilename(variabel ? variabel : "prom");
}



WELT *erde()
{
  static WELT *erde = NULL;

  if (!erde) erde = (WELT *)objekt_mit_namen("ERDE");
  if (!erde) {
      log('I', "erde(): Objekt 'ERDE' nicht vorhabenden! Interner Programmfehler");
  }
  return erde;
}

short strasse_bei(ADR& adr)
{
  if (erde()->adresse_ist_ausserhalb(adr)) return 0;
  else return erde()->feld_attribut(adr, FELD_ATTRIBUT_STRASSE);
}

short nummer_schon_vergeben(long nummer)
{
  char *anummer = myltoa(nummer);
  FOR_EACH_OBJEKT_IN (&globale_objekt_menge)
  DO (
    if (!strcmp(objekt->a_typ(),"STAAT"))
    {
      if (!strcmp(objekt->attribut("SPIELERNUMMER"), anummer)) return 1;
    }
  )
  return 0;
}
  

long neue_spielernummer()
{
  long nummer = 1;
  while (nummer_schon_vergeben(nummer)) nummer++;
  return nummer;
} 


/**---------------------------------------------------------------------------
  * nice_gross_klein()
  * 
  * Wandelt einen Namen (Staedte oder Einheitenname) in eine schoene
  * Gross-Kleinschreibung um, egal wie er vorher geschrieben war.
  * Das Ergebnis ist ein static[] Feld und Eigentum dieser Funktion.
  ---------------------------------------------------------------------------*/
char *nice_gross_klein(char *name)
{
  static char antwort[256];
  
  char *read = name, *write = antwort;
  short gross = 1; // Erstes Zeichen gross. Dies ist ein Zaehler.

  while (*read) {
    if (*read == '~') *write++ = *read++; // Umlaut-Fluchtsymbol ueberlesen
    else if (isalpha(*read) && gross) { 
      *write++ = toupper(*read++);
      gross = 0;
    }
    else if (isalpha(*read)) {
      *write++ = tolower(*read++);
    }
    else { // Nach Sonderzeichen wieder Gross anfangen. z.B. "Donjon-Lothrain"
      gross = 1;
      *write++ = *read++; // Sonderzeichen selbst einfach uebernehmen.
    }
  }
  *write = 0;
  return antwort;
}


/**---------------------------------------------------------------------------
  * gewinnt_kampf_gegen()
  * 
  * Wer gewinnt, bei welcher Kampfkraft?
  *
  * RUECKGABE:
  * true, falls der erste gewinnt.
  ---------------------------------------------------------------------------*/
bool gewinnt_kampf_gegen(long kraft1, long kraft2)
{
    // Wenn bei Kraefte gleich 0 sind, dann liegt ein Programm
    // fehler oder ein Konfigurationsfehler vor.
    if (kraft1 + kraft2 == 0) {
	log('I', "gewinnt_kampf_gegen(%ld, %ld): Summe der Kraefte gleich 0!", kraft1, kraft2);
	return true;
    }

    if (hat_uebermacht(kraft1, kraft2)) return true;
    else if (hat_uebermacht(kraft2, kraft1)) return false;
    
    // Eintrag fehlt -> Wert 0 -> Best of 1.
    long best_of = myatol(lexikon_eintrag("kampfsystem", 2)) * 2 + 1;
    short erster_gewinnt = 0;
    for (int i=0; i<best_of; i++)
    {
	long wuerfel = io_random(kraft1 + kraft2);
	if (wuerfel < kraft1) erster_gewinnt ++;
    }
    return erster_gewinnt * 2 > best_of;
}


/**---------------------------------------------------------------------------
  * hat_uebermacht(long, long)
  * 
  * Liefert true, wenn die erste Kraft eine X-Fache Uebermacht ueber
  * die zweite hat. Einstellbar ist X in der Enzyklopaedie, Eintrag
  * kampfsystem, Wert 1.
  *
  * RUECKGABE:
  * true, falls der erste uebermacht ueber den zweiten hat.
  ---------------------------------------------------------------------------*/
bool hat_uebermacht(long kraft1, long kraft2)
{
    // Wenn einer clip_level-mal so hoch ist wieder andere, dann gewinnt
    //  er sofort.
    long clip_level = myatol(lexikon_eintrag("kampfsystem", 1));
    return clip_level > 0 && kraft1 >= clip_level * kraft2;
}
