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
  * MODUL:               objekt.C / OBJEKT.CPP
  * AUTOR/DATUM:         Mathias Kettner, 25. Juli 1993
  * KOMPATIBILITAET:     C++
  -----------------------------------------------------------------------------
  *
  *      Funktionen der Klasse OBJEKT, die den Kern des Objektorientier-
  *      ten Spielsystems darstellt.
  *
  ---------------------------------------------------------------------------*/

#include <string.h>
#include <ctype.h>

#include "mystring.h"
#include "landscha.h"
#include "einfluss.h"
#include "mystring.h"
#include "laengen.h"
#include "alg.h"
#include "kompatib.h"
#include "listmac.h"
#include "log.h"

extern bool programm_terminieren; // aus main.C

/**---------------------------------------------------------------------------
  * GLOBALE VARIABLE:
  * globale_objekt_menge
  * Liste von Zeigern auf die Objekte. Jedes Objekt ist in dieser 
  * Liste genau einmal repraesentiert. Der Objekt Konstruktor ueber-
  * nimmt das Einbinden, der Destrutkor das Entfernen. Die Eintraege
  * sind vom Typ OBJEKT_LIST_NODE.
  *
  * globale_einfluss_menge
  * Liste, die alle Einfluesse enthaelt, die irgendwelche Objekte
  * auf irgendwelche anderen Objekte ausueben. Die Eintraege sind
  * vom Typ EINFLUSS.
  ---------------------------------------------------------------------------*/

DOUBLIST globale_objekt_menge;
EINFLUSS_LISTE globale_einfluss_menge;

/**---------------------------------------------------------------------------
  * OBJEKT::OBJEKT()              // constructor
  * 
  * Dies ist der einzige Konstruktor von OBJEKT. Er initialisiert
  * alle Eintraege und bindet das Objekt ueber einen zusaetzlichen Kno-
  * ten in der globalen_objekt_menge ein.
  *
  * @param
  * gewuenschter_name:  Eindeutiger(!) Objektname aus Buchstaben, Ziffern
  * und Unterstrichen. Es erfolgt eine automatische
  * Umwandelung in Grossbuchstaben.
  * attr:               String von Attributen, die das Objekt bei der
  * Erschaffung sogleich erhaelt. Syntax siehe
  * ATTRIBUT_LISTE.
  ---------------------------------------------------------------------------*/
OBJEKT::OBJEKT(char *gewuenschter_name, char *attr)
{
  // Hier sollte ich als allererstes kontrollieren, ob es nicht schon
  // ein Objekt mit dem gewuenschten Namen gibt.
  
  long namecount = 1;
  char *alter_name = gewuenschter_name;
  while (objekt_mit_namen(gewuenschter_name)) {
    // Name existiert schon, ich erzeuge einen neuen Namen...
    static char tempname[MAX_LAENGE_NAME];
    sprintf(tempname,"%s%ld",alter_name, namecount++);
    gewuenschter_name = tempname;
  }

  name = mystrdup(gewuenschter_name);

  // Und nun nach Grossbuchstaben konvertieren
  char *w = name;
  while (*w) *w++ = toupper(*w);
  
  vernichtenswert = 0;
  phasenzaehler = 0;
  besitzer = NULL;
  ort_objekt = NULL;
  temp_ort_name = NULL;
  tagebuch_eintraege = NULL;
  namenszaehler=1;

  // Aus dem Parameter attr lese ich Startattribute ein

  attribute.aus_string_einlesen(attr);

  // Hier erzeuge ich einen neuen globalen Node und fuege ihn in die
  // globale Objektliste ein. global_node ist eine Variable der
  // Objektstruktur, die ich mir deshalb merken muss, damit sie der
  // Destruktor auch wieder aus der globalen Liste entfernen kann.

  global_node = new OBJEKT_LIST_NODE;
  global_node->objekt = this;
  globale_objekt_menge.insert(global_node);

}

/**---------------------------------------------------------------------------
  * OBJEKT::~OBJEKT()              // destructor
  * 
  * Der Destruktor von OBJEKT ist relativ komplex und mehrfach
  * ueberarbeitet (vor irgendwelchen Aenderungen bitte sehr gut
  * nachdenken!). Laut Konvention werden mit einem Objekt auch die
  * Objekte vernichtet, die es besitzt und diejenigen, die sich
  * in ihm befinden (ort()). Sowohl die Besitztrelation als auch
  * die Ortsrelation sind Baumartig. Trotzdem kann der summierte
  * Graf aus beiden Kreise enthalten! Diese Tatsache zusammen mit
  * der REKURSIVITAET dieses Destruktors zu verschiedenen Schwierig-
  * keiten (Siehe inline-Doku).
  ---------------------------------------------------------------------------*/
OBJEKT::~OBJEKT()
{
  // Durch die Rekursivitaet des Destruktors kann es theoretisch vorkommen,
  // dass der Destruktor fuer das gleiche Objekt mehrmals (verschachtelt)
  // aufgerufen wird. Um dies zu verhindern, entferne ich mich zuerst aus
  // der globalen Objektliste und anschiessend entferne ich mich noch aus
  // der Liste meines Besitzers, falls ich einen habe!

  delete global_node;
  if (besitzer) remove();

  // Und jetzt inserte ich mich noch in eine Liste, die eigens fuer den
  // Destruktor geschaffen ist.

  static DOUBLIST destruktor_liste;
  destruktor_liste.insert(this);

  // Laut der Spezifikation meines Objektsystems werden mit einem Objekt
  // stets alle Objekte vernichtet, die es besitzt. Sollte ein spezielles
  // Objekt fuer sein Besitztum an anderes "Testament" vorgesehen haben,
  // so muss dies im Destruktor dieser abgeleiteten Klasse geschehen, der
  // Destruktor der abgeleiteten Klasse wird naemlich VOR diesem hier aus-
  // gefuehrt. Das Vernichten der Besitzobjekte geschieht automatisch
  // durch das Vernichten der Liste besitztum, die ja direkt in der Objekt-
  // struktur vorhanden ist. Will ein Objekt bei seiner Vernichtung seine
  // Objekte noch einem anderen Objekt geben, so muss es dabei beruecksich-
  // tigen, dass dieses andere Objekt unter Umstaenden auch gerade vernich-
  // tet wird!

  // Laut Spezifikation muessen weiterhin alle Objekte vernichtet werden,
  // die sich in mir befinden, d.h. als Ort MICH haben!

  OBJEKT_LIST_NODE *node;
  node = (OBJEKT_LIST_NODE *)globale_objekt_menge.first();
  while (!node->is_tail())
  {
    if (node->objekt->ort_objekt == this) {
      delete node->objekt;
      // Wegen der Rekursivitaet dieses Destruktors kann ich nicht sicher
      // sein, dass aus der globalen Liste nicht eine Reihe von weiteren Ob-
      // jekten vernichtet wurden. Deshalb muss ich hier wieder von vorne
      // anfangen!

      node = (OBJEKT_LIST_NODE *)globale_objekt_menge.first();
    }
    else node = (OBJEKT_LIST_NODE *)node->next();
  }

  // Alle Einfluesse, die von mir ausgehen, muss ich loeschen!

  globale_einfluss_menge.einfluesse_loeschen(this);

  // Und noch ein paar dynamisch angelegte Strings...

  myfree(name);
  myfree(temp_ort_name);
  tagebuch_loeschen();
}

/**---------------------------------------------------------------------------
  * OBJEKT::geben()
  * 
  * Entfernt ein Objekt von seinem Besitzer und uebergibt es einem
  * neuen Besitzer. Diese Routine sollte auf jeden Fall von dem
  * Objekt aus aufgerufen werden, der das Objekt bestitzt.
  *
  * Anmerkung: Der Empfaenger kann auch das Universum (NULL) sein!
  * In diesem Fall befindet sich das uebergebene Objekt nach diesem
  * Aufruf im Besitz keines anderen Objektes (aehnlich wie die Uhr).
  * Es ist dann nur noch ueber die globale_objekt_menge erreichbar.
  *
  * @param
  * empfaenger:     Dieses Objekt soll der neue Besitzer sein.
  * objekt:         Dieses Objekt soll gegeben werden.
  * this            Sollte der alte Besitzer des Objektes sein.
  ---------------------------------------------------------------------------*/
void OBJEKT::geben(OBJEKT *empfaenger, OBJEKT *objekt)
{
  if (!objekt) {
      log('I', "OBJEKT::geben(): objekt = NULL!");
      return;
  }
  else if (!this) {
      log('I', "OBJEKT::geben(): this = NULL!");
      return;
  }
  objekt->remove();
  empfaenger->in_besitz_nehmen(objekt);
}

/**---------------------------------------------------------------------------
  * OBJEKT::in_besitz_nehmen()
  * 
  * Ein Objekt, das bisher noch keinem anderen Objekt gehoert (!)
  * bekommt nun einen Besitzer. Der Besitzer kann dabei auch dass
  * Universum sein (NULL). In diesem Fall aendert sich naetuerlich
  * garnichts, da ein Objekt, das keinem gehoert nach Definition
  * ja dem Universum gehoert. Dieser Fall ist nur vollstaendigkeits-
  * halber aufgenommen.
  *
  * @param
  * objekt:         Objekt, das einen neuen Besitzer bekommen soll.
  * this:           Das Objekt, das es in Besitz nimmt.
  ---------------------------------------------------------------------------*/
void OBJEKT::in_besitz_nehmen(OBJEKT *objekt)
{
    if (!objekt) {
      log('I', "OBJEKT::in_besitz_nehmen(): objekt = NULL!");
      return;
  }

  // Um die Besitzverhaeltnisse eindeutig festzulegen, muss ich ZWEI ver-
  // knuepfungen herstellen, da sowohl der Besitzer seinen Besitz als auch
  // jedes Objekt seinen Besitzer kennen soll.

  if (this) besitztum.insert(objekt);
  objekt->besitzer = this;
}

/**---------------------------------------------------------------------------
  * OBJEKT::objekt_mit_namen()
  * 
  * Mit dieser Funktion kann der Zeiger auf ein Objekt bestimmt werden,
  * von dem man nur den Namen kennt. Dazu durchsucht die Funktion
  * die globale Objektliste. Bei der Schreibung des Namens gibt es
  * bestimmte geschuetzte Namen, die eine besondere Bedeutung haben:
  *
  * !t      steht fuer das aufrufende Objekt selbst.
  * !p      steht fuer den Besitzer des aufrufenden Objektes.
  * !u	steht fuer den Besitzer des Besitzers des aufrufenden Objektes.
  * !o      steht fuer den Ort des aufrufenden Objektes
  *
  * @param
  * name:           Name des gesuchten Objektes.
  * this:           Objekt, von dem aus gesucht werden soll. Der
  * this-Pointer ist nur relevant, wenn der Name
  * relativ ist (z.B. !t oder !p).
  *
  * @return
  * Zeiger auf das gesuchter Objekt oder NULL, wenn es ein solches
  * nicht gibt.
  ---------------------------------------------------------------------------*/
OBJEKT *OBJEKT::objekt_mit_namen(char *gesuchter_name)
{
  if (gesuchter_name[0] == '!') // Sondername, relativ.
  {
    switch (gesuchter_name[1]) {
      case 't':  return this;
      case 'p':  return besitzer;
      case 'u':  return besitzer->besitzer;
      case 'o':  return ort();
    }
    return NULL; // Ungueltig, gesuchtes Objekt existiert daher nicht.
  }
  return ::objekt_mit_namen(gesuchter_name);
}


/**---------------------------------------------------------------------------
  * OBJEKT::besitzt()
  * 
  * Stellt fest, ob sich ein bestimmtes Objekt im Besitz des aufrufenden
  * befindet. Dabei gilt ein Objekt auch im Besitz, wenn dies transitiv
  * ueber ein oder mehrere Unterobjekte der Fall ist, die eine geschlos-
  * sene Besitzkette zu dem Aufrufenden besitzen, d.h. wenn er das Ob-
  * jekt  zwar nicht selbst besitzt, es aber von einem seiner Objekte
  * besessen wird.
  *
  * @param
  * objekt:         Objekt, dessen Besitzverhaeltnisse geklaert werden
  * sollen.
  * this:           Vermeindtlicher Besitzer des Objekts.
  *
  * @return
  * 1, wenn es im Besitz ist, 0 sonst.
  ---------------------------------------------------------------------------*/
short OBJEKT::besitzt(OBJEKT *obj)
{
  return (obj->besitzer && 
   (this == obj->besitzer || besitzt(obj->besitzer)));
}

/**---------------------------------------------------------------------------
  * OBJEKT::alle_objekte_im_besitz()
  * 
  * Liefert eine Liste aller Objekte, die das implizite Objekt be-
  * sitzt. Es findet eine transitive Erweiterung der Relation statt!
  * Das Objekt selbst ist nicht in der Liste (Relation nicht reflexiv!)
  * Zur Vernichtung vorgemerkte Objekte werden nicht aufgenommen!
  *
  * Diese Funktion ist extrem ineffizient und soll daher nur an
  * stellen aufgerufen werden, die alle Jubeljahre mal drankommen.
  * Z.B. Staatsform aendern, Spielsieg, usw...
  *
  * @param
  * Hier kann eine Bedingung im Sinne von bedingung_erfuellt() an-
  * gegeben werden, die die gesuchten Objekte zusaetzlich noch er-
  * fuellen muessen, wenn sie in die Liste aufgenommen werden sollen.
  *
  * @return
  * DOUBLIST * dynamisch angelegte Liste der gesuchten Objekte
  * (OBJEKT_LIST_NODE), die nach Gebrauch freigegeben werden muss.
  ---------------------------------------------------------------------------*/
DOUBLIST *OBJEKT::alle_objekte_im_besitz(char *bedingung)
{
  DOUBLIST *liste = new DOUBLIST;
  OBJEKT_LIST_NODE *objnode;
  objnode = (OBJEKT_LIST_NODE *)globale_objekt_menge.first();
  while (!objnode->is_tail()) {
    if (!bedingung || objnode->objekt->bedingung_erfuellt(bedingung)) {
      if (besitzt(objnode->objekt)) { // Obj muss auch im Besitz sein.
	if (!objnode->objekt->zur_vernichtung_vorgemerkt()) {
	  OBJEKT_LIST_NODE *neu = new OBJEKT_LIST_NODE;
	  neu->objekt = objnode->objekt;
	  liste->insert(neu);
	}
      }
    }
    objnode = (OBJEKT_LIST_NODE *)objnode->next();
  }
  return liste;
}


/**---------------------------------------------------------------------------
  * OBJEKT::alle_objekte_im_direkten_besitz()
  * 
  * Liefert alle Objekte in der besitztumsliste, die einer Bediungung
  * genuegen.
  ---------------------------------------------------------------------------*/
DOUBLIST *OBJEKT::alle_objekte_im_direkten_besitz(char *bedingung)
{
  DOUBLIST *antwortliste = new DOUBLIST;
  OBJEKT *objekt;
  SCAN(&besitztum, objekt)
  {
    if ((!bedingung || objekt->bedingung_erfuellt(bedingung))
        && !objekt->zur_vernichtung_vorgemerkt()) {
      antwortliste->add_tail(new OBJEKT_LIST_NODE(objekt));
    }
    NEXT(objekt);
  }
  return antwortliste;
}


/**---------------------------------------------------------------------------
  * OBJEKT::besitztum_permutieren()
  * 
  * Permutiert alle Objekte im Besitz, die einer Bedingung genuegen,
  * in deren Reihenfolge dort zufaellig. Wird z.B. benoetigt, damit
  * jeden Zug die Spieler in einer anderen Reihenfolge drankommen.
  * @param
  * char *bed:      Bedingung als Attributstring
  ---------------------------------------------------------------------------*/
void OBJEKT::direktes_besitztum_permutieren(char *bed)
{
  // Als erstes hole ich mir die Liste aller zu permutierenden Objekte.
  DOUBLIST *liste = alle_objekte_im_direkten_besitz(bed);

  // Die Nodes der erhaltenen Objektliste zeigen auf die tatsaechlichen
  // Nodes der Objekte in der Besitzliste diese Objektes. Ich kann also
  // direkt mit diesen Nodes jonglieren und diese entfernen und neu ein-
  // fuegen.

  // Der Algorithmus geht folgendermassen: Ich suche immer ein Objekt aus
  // der Liste aus, entferne es aus dem Besitztum und fuege es dort am
  // Anfang wieder ein. Dann entferne ich seine indirekte Node aus meiner
  // Liste. Das mache ich solange, bis die Liste leer ist und alle Objekte
  // am Anfang im Besitz stehen.

  while (!liste->is_empty()) {
    long anzahl = liste->number_of_elements();
    long auswahl = io_random(anzahl);
    OBJEKT_LIST_NODE *kandidat = (OBJEKT_LIST_NODE *)liste->first();
    while (auswahl) {
       kandidat = (OBJEKT_LIST_NODE *)kandidat->next();
       auswahl--;
    }
    
    // Jetzt habe ich das richtige Objekt und entferne es und fuege es
    // wieder ein.
    
    kandidat->objekt->remove();
    besitztum.insert(kandidat->objekt);
    delete kandidat; // Loescht nur die indirekte List-Node!
  }
  delete liste;
}


/**---------------------------------------------------------------------------
  * OBJEKT::alle_enthaltenen_objekte()
  * 
  * Liefert eine Liste alle Objekte, die im impliziten Objekt direkt
  * enthalten sind. Eine transitive Erweitertung der Relation findet
  * NICHT statt (momentan).
  * Zur Vernichtung vorgemerkte Objekte werden nicht aufgenommen!
  * @param
  * Hier kann eine Bedingung im Sinne von bedingung_erfuellt() an-
  * gegeben werden, die die gesuchten Objekte zusaetzlich noch er-
  * fuellen muessen, wenn sie in die Liste aufgenommen werden sollen.
  * @return
  * DOUBLIST * dynamisch angelegte Liste der gesuchten Objekte
  * (OBJEKT_LIST_NODE), die nach Gebrauch freigegeben werden muss.
  ---------------------------------------------------------------------------*/
DOUBLIST *OBJEKT::alle_enthaltenen_objekte(char *bedingung)
{
  DOUBLIST *liste = new DOUBLIST;

  FOR_EACH_OBJEKT_IN (&globale_objekt_menge)
  DO (
    if (objekt->ort() == this) {
      if (!bedingung || objekt->bedingung_erfuellt(bedingung)) {
	if (!objekt->zur_vernichtung_vorgemerkt()) {
	  OBJEKT_LIST_NODE *neu = new OBJEKT_LIST_NODE;
	  neu->objekt = objekt;
	  liste->insert(neu);
	}
      }
    }
  )
  return liste;
}


/**---------------------------------------------------------------------------
  * FUNTKTION:           OBJEKT_LIST_NODE::matches()
  * 
  * Wird von der find()-Funktion der Basisklasse benoetigt und
  * soll feststellen, ob das implizite Objekt den Vergleichsdaten
  * entspricht, die uebergeben werden.
  *
  * @param
  * vergleichsdaten:        (void *)Zeiger auf einen Objektnamen
  *
  * @return
  * 1, falls es das gesuchte Objekt ist, 0 sonst.
  ---------------------------------------------------------------------------*/
short OBJEKT_LIST_NODE::matches(void *vergleichsdaten)
{
  char *gesuchter_name = (char *)vergleichsdaten;
  return (!mystrcmp_no_case(gesuchter_name, objekt->name));
}

/**---------------------------------------------------------------------------
  * OBJEKT::befehl_erteilen()
  * 
  * Mit dieser Funktion kann einem Objekt von aussen ein Befehl erteilt
  * werden. Der Befehl wird aber nicht sofort ausgefuert, sondern in
  * der Befehlsliste des Objektes gespeichert (FIFO). Die Befehle ge-
  * langen sukzessive zur Auswertung, wenn die aktion()-Funktion des
  * Objektes aufgerufen wird. Wieviele Befehle in einem Aufruf von
  * aktion() ausgefuert werden, bzw. wieviele Aufrufe von aktion()
  * fuer einen Befehl benoetigt werden, muss jedes Objekt selbst ent-
  * scheiden. Dafuer bekommt es jede Phase die Variable phasenzaehler.
  *
  * @param
  * wortlaut:       Befehl als Asciizeichenfolge. Die genaue Syntax
  * legt jedes Objekt selbst fest.
  ---------------------------------------------------------------------------*/
void OBJEKT::befehl_erteilen(char *wortlaut)
{
    BEFEHL *befehl = new BEFEHL(wortlaut);
    befehlsliste.add_tail(befehl); // Bei den Befehlen herrscht FIFO vor.
}


/**---------------------------------------------------------------------------
  * OBJEKT::alle_befehle_abbrechen()
  * 
  * Loescht die ganze Befehlsliste eines Objektes. Darf auch aus der
  * befehl_auswerten()-Funktion eines Objektes heraus aufgerufen werden.
  * Der Phasenzaehler wird auf 0 gesetzt.
  * Ausserdem wird das Objekt ueber das Kommando BEFEHLE_ABGEBROCHEN
  * darueber informiert, damit es eventuell interne Daten zur Befehls-
  * abbarbeitung initialisieren kann.
  * @param
  * short modus: 	0, wenn alle Befehle abgebrochen werden sollen.
  *                     1, wenn der erste Befehl und der Phasenzaehler bleiben soll.
  ---------------------------------------------------------------------------*/
void OBJEKT::alle_befehle_abbrechen(short modus)
{
  if (modus == 1) {
    while (befehlsliste.number_of_elements() > 1) 
      delete befehlsliste.last();
  }
  else {
    befehlsliste.clear();
    phasenzaehler = 0;
  }
  kommando("BEFEHLE_ABGEBROCHEN");
}


/**---------------------------------------------------------------------------
  * OBJEKT::befehl_vorschieben()
  * 
  * Lastet einen neuen Befehl ein, aber am Anfang der Liste. Dieser
  * Befehl wird also noch vor allen anderen ausgefuehrt, die bis jetzt
  * in der Liste stehen. Im Gegensatz zu befehl_nachschieben() darf
  * diese Funktion auch bei leerer Befehlliste und auch von ausserhalb
  * der befehl_auswerten()-Funktion eines Objekte aufgerufen werden.
  * Wird sie von befehl_auswerten() aufgerufen, so rueckt der aktuelle
  * Befehl an zweite Stelle. Der phasenzaehler bleibt unveraendert, so
  * dass die bis jetzt gewartete Phasenzahl fuer den neuen Befehl gleich
  * verwendet werden kann. Ist dies nicht erwuenscht, so kann der
  * phasenzaehler von Hand auf 1 (nicht 0!) gesetzt werden.
  *
  * @param
  * char *wortlaut: Einzuschiebender Befehl.
  ---------------------------------------------------------------------------*/
void OBJEKT::befehl_vorschieben(char *wortlaut)
{
  BEFEHL *befehl = new BEFEHL(wortlaut);
  if (befehl) befehlsliste.add_head(befehl);
}


/**---------------------------------------------------------------------------
  * OBJEKT::befehl_nachschieben()
  * 
  * Darf nur aus der befehl_auswerten()-Funktion eines Objekte heraus
  * verwendet werden! Stellt einen neuen Befehl an die zweite Stelle
  * der Liste, so dass er als naechstes nach dem aktuellen Befehl an
  * der Reihe ist, ganz gleich, welche Befehle folgen. Der aktuelle
  * Befehl bleibt an erster Stelle.
  *
  * @param
  * char *wortlaut: Einzuschiebender Befehl.
  ---------------------------------------------------------------------------*/
void OBJEKT::befehl_nachschieben(char *wortlaut)
{
  if (befehlsliste.is_empty()) return; // Darf nicht sein.
  BEFEHL *befehl = new BEFEHL(wortlaut);
  if (befehl) befehl->insert_after(befehlsliste.first());
}


/**---------------------------------------------------------------------------
  * OBJEKT::befehl_umwandeln()
  * 
  * Ist dafuer gedacht, von innerhalb einer befehl_auswerten() Funktion
  * aufgerufen zu werden, um den momentan bearbeiteten Befehl zu aendern.
  * So kann erreicht werden, dass der Befehl an eine andere auswerten-
  * Funktion sozusagen 'uebergeben' wird.
  *
  * Einschraenkung: Da diese Funktion immer den ersten Befehl in der
  * Liste umwandelt, ist es wichtig, dass die aufrufende Befehlsfunktion
  * noch keine anderen Befehl vorgeschoben hat! Sonst wird naemlich
  * einer der vorgeschobenen Befehle geaendert, statt dem aktuellen.
  *
  * @param
  * char *wortlaut: So soll der Befehl ab jetzt lauten.
  ---------------------------------------------------------------------------*/
void OBJEKT::befehl_umwandeln(char *wortlaut)
{
  if (befehlsliste.is_empty()) return; // Darf nicht sein.
  BEFEHL *befehl = (BEFEHL *)befehlsliste.first();
  myfree(befehl->befehlstext);
  befehl->befehlstext = mystrdup(wortlaut);
}


/**---------------------------------------------------------------------------
  * OBJEKT::befehl_durch_naechsten_ersetzen()
  * 
  * Der aktuelle Befehl wird einfach vergessen und es wird sofort
  * zum naechsten Befehl weitergeschaltet. Und zwar innerhalb
  * einer befehl_auswerten() Funktion. Insbesondere ist das eine
  * Moeglichkeit, den phasenzaehler nicht wieder auf 0 zu setzen
  * und trotzdem zum naechsten Befehl weiterzugehen.
  ---------------------------------------------------------------------------*/
void OBJEKT::befehl_durch_naechsten_ersetzen()
{
  if (!befehlsliste.is_empty()) delete befehlsliste.first();
}

/**---------------------------------------------------------------------------
  * OBJEKT::aktion()
  * 
  * Sehr wichtige, zentrale Funktion! Bei jedem Aufruf dieser Funktion
  * wird beim Objekt
  * 1. Die Funktion naechste_phase() aufgerufen, die jedes tatsaechliche
  * Objekt zur Verfuegung stellen kann (virtuell ueberladen).
  * 2. Die Funktion befehl_auswerten() wird solange aufgerufen, bis sie
  * 0 zurueckgibt. Gibt sie einen positiven Wert zurueck, so wird der
  * erste Befehl aus der Liste geloescht und mit dem naechsten fort-
  * gefahren. Bei einem negativen Wert wird der Befehl nicht geloescht
  * und mit dem ersten Befehl der Liste fortgefahren. Der Phasenzaehler
  * bleibt dann unveraendert.
  * 3. Fuer jedes Objekt im Besitz wird die Funktion aktion() aufgerufen.
  *
  * Ruft man diese Funktion also fuer das Wurzelobjekt des gesamten Ob-
  * jektbaums auf, so machen alle Objekte genau einen Aufruf der Funk-
  * tion aktion().
  *
  * Zum Phasenzaehler ist anzumerken, dass er von dieser Aktion nur
  * stetig hochgezaelt wird, von der befehl_auswerten()-Funktion je-
  * doch nach Abarbeitung eines Befehls wieder zurueckgesetzt werden
  * sollte. Dazu hilft die Funktion OBJEKT::befehl_dauert_noch().
  *
  * @param
  * phase:  Ein Zaehler, der rekursiv unveraendert weitergegeben wird.
  * Ein negativer Wert bedeutet, dass der rekursive Aufruf
  * von aktion fuer die Objekte im Besitz unterbunden wird.
  ---------------------------------------------------------------------------*/
void OBJEKT::aktion(long phase)
{
  const short RETRY_MAX=5000; // Maximal 5000 Befehle pro Phase, sonst Alarm.

  // Ein Objekt, dass zur Vernichtung vorgemerkt ist, kann keinerlei
  // Befehle mehr ausfuehren oder andere Dinge machen.

  if (zur_vernichtung_vorgemerkt()) return;

  // Achtung! Laut Konvention darf sich das Objekt nicht selbst vernichten.
  // Um dies zu erreichen, muss es den indirekten Weg ueber die Funktion
  // zur_vernichtung_vormerken() gehen.


  // Bevor irgendwelche Befehle abgearbeitet werden, wird die
  // Funktion naechste_phase() aufgerufen, die die laufenden
  // Berechnungen des Objektes vornimmt.

  naechste_phase(phase);

  // Nun die Befehle abarbeiten.

  short retrycounter = 0;
  bool phasenzaehler_schon_erhoeht = false;
  while (retrycounter++ < RETRY_MAX && !befehlsliste.is_empty() && !zur_vernichtung_vorgemerkt())
  {
    BEFEHL *befehl = (BEFEHL *)befehlsliste.first();
    short    rwert = befehl_auswerten(befehl->befehlstext, phase);

    if (rwert > 0) delete befehl; // Befehl abgearbeitet

    // Befehle die 0 Phasen dauern, koennen schon vor der ersten
    // Phase ausgefuehrt werden. die Funktion aktion wird aber immer
    // NACH jeder Phase aufgerufen und eine 0. Phase gibt es nicht.
    // Deshalb muss ich in Phase 1 den Befehlen, die 0 Phasen brauchen,
    // eine Chance geben, ausgefuehrt zu werden, ohne dass dadurch
    // eine Phase verloren geht.

    if (phasenzaehler == 0 && rwert != 0) continue;
    
    if (rwert > 0) phasenzaehler = 0; // Befehl fertig -> Neuer Befehl faengt wieder bei 0 an.
    else if (rwert == 0) // Befehl noch nicht fertig. Befehl dauert noch.
    {
	if (!phasenzaehler_schon_erhoeht) {
	    phasenzaehler++;
	    phasenzaehler_schon_erhoeht = true;
	    continue; // Erneuter Versuch mit der zusaetlichen Phase.
	}
	else break; // Befehl dauert wirklich noch
    }
    // negative Werte zeigen an, dass es mit gleichem Phasenzaehler weitergehen soll.
    // Befehle, die andere Befehl vorschieben (z.B. AV) brauchen das.
  }

  if (retrycounter >= RETRY_MAX) { // Fehler!
      log('W', "Objekt '%s': To much commands lasting 0 phases. Last command was '%s'."
	  " Unit will be stopped. Evaluation will be continued.", this->name,
	  ((BEFEHL *)befehlsliste.first())->befehlstext);
      alle_befehle_abbrechen();
  }
  
  if (zur_vernichtung_vorgemerkt()) return;

  // Damit sich bei leerer Befehlsliste keine Phasen anhaeufen, setze
  // ich den Phasenzaehler dann auf 0.

  if (befehlsliste.is_empty()) phasenzaehler = 0;

  // Jetzt fuehren alle Objekte in der Besitzliste ihre Aktion durch, wenn
  // 'phase' nicht negativ ist.

  if (phase >= 0) {
    OBJEKT *obj = (OBJEKT *)besitztum.first();
    while (!obj->is_tail()) {
      OBJEKT *next = (OBJEKT *)obj->next(); // Koennte sich aendern!
      obj->aktion(phase);
      obj = next;
    }
  }
}


/**---------------------------------------------------------------------------
  * OBJEKT::befehl_dauert_noch()
  * 
  * Macht nur innerhalb von befehl_auswerten()-Funktionen Sinn. Eine
  * solche Funktion kann damit feststellen, ob schon genuegend Phasen
  * vergangen sind, damit der Befehl ausgefuert werden kann. Eine
  * Beispielstelle koennte so aussehen:
  *
  * ... (Fehlerabfragen zur Syntax des Befehls, etc.) ...
  * if (befehl_dauert_noch(DA_STRASSENBAU)) return 0;
  * ... (Aktion einleiten, Befehl ausfuehren) ...
  *
  * Die Funktion kontrolliert, ob der Phasenzaehler den benoetigten
  * Wert erreicht hat. Der Phasenzaehler bleibt unveraendert.
  *
  * @param
  * long dauer: Anzahl an Phasen, die der Befehl zur Ausfuehrung benoetigt.
  *
  * @return
  * short 0, wenn der phasenzaehler den noetigen Wert erreicht hatte,
  * short 1, wenn nicht.
  ---------------------------------------------------------------------------*/
short OBJEKT::befehl_dauert_noch(long dauer)
{
  if (phasenzaehler < dauer) return 1;

  phasenzaehler -= dauer;
  return 0;
}


/**---------------------------------------------------------------------------
  * OBJEKT::zug()
  * 
  * Aehnlich der Funktion aktion() wird die Funktion zug() stets rekur-
  * siv fuer alle Unterobjekte (=Besitz) aufgerufen. Fuer jedes Objekt
  * wird die Funktion naechster_zug() aufgerufen, die in den abgelei-
  * teten Klassen ueberladen werden kann. Die Verwendung der zug()-
  * und naechster_zug()-Funktionen ist im Prinzip beliebig. Beachte:
  * Die Funktion naechster_zug() wird VOR den rekursiven Aufrufen 
  * ausgefuehrt!
  *
  * @param
  * zugnummer:      long-Zahl, die zum Zaehlen verwendet werden kann
  * und an alle rekursiven Aufrufe weitergeleitet wird.
  ---------------------------------------------------------------------------*/
void OBJEKT::zug(long zugnummer)
{
  // Ein Objekt, das bereits zur Vernichtung vorgemerkt ist, darf diese
  // Funktion nicht mehr ausfuehren
  if (zur_vernichtung_vorgemerkt()) return;

  naechster_zug(zugnummer);
  if (programm_terminieren) return; // wegen UHR::naechster_zug()

  // Jetzt noch fuer jedes Objekt im Besitz...

  OBJEKT *obj = (OBJEKT *)besitztum.first();
  while (!obj->is_tail()) {
    obj->zug(zugnummer);
    obj = (OBJEKT *)obj->next();
  }
}

/**---------------------------------------------------------------------------
  * OBJEKT::zug_ende()       
  * 
  * Diese Funktion ist fast identisch mit der Funktion zug(), nur dass
  * als Wirkfunktion die Funktion zug_abschliessen() ueberladen werden
  * muss. Auch hier ist die Verwendung im Prinzip frei. Unterschied
  * zu zug(): Die Funktion zug_abschliessen() wird erst NACH den rekur-
  * siven Unteraufrufen ausgefuehrt!
  *
  * @param   
  * zugnummer:      long-Zahl, die weitergereich wird und zur freien
  * Verwendung ist.
  ---------------------------------------------------------------------------*/
void OBJEKT::zug_ende(long zugnummer)
{
  // Darf nicht von vernichteten Objekten ausgefuehrt werden.
  if (zur_vernichtung_vorgemerkt()) return;

  // Zuerst fuer jedes Objekt im Besitz...

  OBJEKT *obj = (OBJEKT *)besitztum.first();
  while (!obj->is_tail()) {
    obj->zug_ende(zugnummer);
    obj = (OBJEKT *)obj->next();
  }

  // Und jetzt die tatsaechliche Funktion
  zug_abschliessen(zugnummer);
}


/**---------------------------------------------------------------------------
  * OBJEKT::einfluss_vorhanden()
  * 
  * Stellt fest, ob auf das Objekt ein Einfluss einer bestimmten
  * Art aufgefuert wird.
  *
  * @param
  * art             Art des Einflusses als String.
  *
  * @return
  * 1, falls ein solcher Einfluss vorhanden ist, sonst 0.
  ---------------------------------------------------------------------------*/
short OBJEKT::einfluss_vorhanden(char *art)
{
  return NULL != globale_einfluss_menge.finde_einfluss(this, art);
}

/**---------------------------------------------------------------------------
  * OBJEKT::einfluss_der_art()
  * 
  * Ermittelt den Parameter, den ein Einfluss einer bestimmten Art auf
  * das Objekt ausuebt.
  *
  * @param
  * art:            Art des Einflusses, nach dem gesucht werden soll.
  *
  * @return
  * Parameter in Stringform oder NULL, wenn gar kein Einfluss der
  * benannten Art vorliegt.
  ---------------------------------------------------------------------------*/
char *OBJEKT::einfluss_der_art(char *art)
{
  EINFLUSS *einfl = globale_einfluss_menge.finde_einfluss(this, art);
  if (einfl) return einfl->parameter;
  return NULL;
}

/**---------------------------------------------------------------------------
  * OBJEKT::beeinflussen()
  * 
  * Zentrale Funktion im Mechanismus der Beeinflussungen. Mit ihr kann
  * ein Objekt Einfluss auf ein anderes Objekt, bzw. auf eine Klasse
  * von Objekten ausueben. Die ausgeuebten Einflusse gelten sofort
  * nach dem Aufruf und koennen entweder explizit vom Objekt selbst
  * entfernt werden oder gehen automatisch verloren, wenn das Objekt
  * zerstoert wird.
  *
  * @param
  * art:            Art des Einflusses, der ausgeuebt werden soll.
  *
  * (Die folgenden drei Angaben spezifizieren die Zielgruppe:)
  * name:           Name des zu beeinflussenden Objektes oder
  * NULL, wenn eine Objektklasse statt einem expliziten
  * Objekt beeinflusst werden soll.
  * attribute:      Liste von Attributen, die ein beeinflusstes Objekt
  * haben muss. Die Syntax ist art1=wert1,...,artn=wertn
  * besitzer:       Dieses Objekt muss Besitzer jedes beeinflussten
  * Objektes sein. Beachten Sie, dass hier auch die
  * relativen Namen !t und !p verwendet werden koennen.
  * parameter:      Parameter, mit dem der Einfluss bewertet werden kann.
  ---------------------------------------------------------------------------*/
void OBJEKT::beeinflussen(char *art, char *ziel, char *parameter)
{
  // Falls keine Einflussart angegeben ist, mache ich einfach garnichts.

  if (!art || !*art) return;

  // Eingebunden in die globale Einflussmenge wird der Einfluss durch
  // seinen Kostruktor. Ich muss also nur einen neuen Einfluss erzeugen

  new EINFLUSS(this, art, ziel, parameter);
}


/**---------------------------------------------------------------------------
  * OBJEKT::unbekannter_befehl()
  * 
  * Kleine Servicefunktion, die von der Funktion befehl_auswerten() der
  * ueberladenen Klassen angesprungen werden kann, wenn der Befehl un-
  * bekannt ist.
  *
  * @param
  * befehl:         Fehlerhafter Befehl
  *
  * @return
  * Ist immer 1, bedeutet "Befehl fertig" bei der Rueckgabe von befehl_
  * auswerten().
  ---------------------------------------------------------------------------*/
short OBJEKT::unbekannter_befehl(char *befehl)
{
  report(L("Den Befehl %s gibt es nicht!\n","The command %s does NOT exist !\n"), befehl);
  return 1;
}


/**---------------------------------------------------------------------------
  * OBJEKT::tagebuch_fuehren()
  * 
  * Verlaengert den String tagebuch in der Objektstruktur um einen
  * gegebenen String.
  *
  * @param
  * eintrag:        Neuer Eintrag ins Tagebuch
  * trennung:       Zeichenkette, die davorgehaengt werden soll, wenn
  * das Tagebuch vorher nicht leer ist.
  ---------------------------------------------------------------------------*/
void OBJEKT::tagebuch_fuehren(char *eintrag, char *trennung)
{
  if (!tagebuch_eintraege) tagebuch_eintraege = mystrdup(eintrag);

  else {
    mystradd(tagebuch_eintraege, trennung);
    mystradd(tagebuch_eintraege, eintrag);
  }
}

/**---------------------------------------------------------------------------
  * OBJEKT::tagebuch_loeschen()
  * 
  * Loescht alle Eintraege im Tagebuch, welches somit wieder leer ist.
  ---------------------------------------------------------------------------*/
void OBJEKT::tagebuch_loeschen()
{
  myfree(tagebuch_eintraege);
  tagebuch_eintraege = NULL;
}

/**---------------------------------------------------------------------------
  * OBJEKT::report()
  * 
  * Ausser dem Tagebuch hat jedes Objekt noch einen Speicher fuer soge-
  * nannte Reports. Jeder Report besteht aus einem String. Diese
  * Funktion fuegt einen neuen Report an das Ende der bestehenden 
  * Reportliste ein. Die Reportliste kann dann z.B. am Ende eines 
  * Zuges komplett ausgegeben werden. Dadurch kann vermieden werden,
  * dass Meldungen von Objekten durcheinandergemischt ausgegeben wer-
  * den.
  *
  * @param
  * text:           Reportstring. Dieser String darf an einer Stelle
  * ein %s-Formatsymbol enthalten (auch %-10s u.s.w).
  * parameter:      Enthaelt der Reportstring ein %s-Symbol, so wird
  * dieser Parameter an der Stelle eingefuegt.
  ---------------------------------------------------------------------------*/
void OBJEKT::report(const char *text, const char *parameter)
{
  if (!text) return; // Kein Report erwuenscht.
  char *puffer = new char[strlen(text) + mystrlen(parameter) + 2];
  if (!puffer) return; // Kein Speicher mehr da. Schlecht.
  sprintf(puffer, text, parameter);

  REPORT *report = new REPORT(puffer);
  if (report) reportliste.add_tail(report);
  delete puffer;
}

void OBJEKT::report(char *f, long w)
{
  char string[16];
  sprintf(string,"%ld",w);
  report(f,string);
}


// Fragliche Funktion!
void OBJEKT::reportliste_drucken()
{
  REPORT *rep = (REPORT *)reportliste.first();
  while (!rep->is_tail())
  {
    drucken(rep->text);
    rep = (REPORT *)rep->next();
  }
}

void OBJEKT::reportliste_in_file(FILE *file)
{
  REPORT *rep = (REPORT *)reportliste.first();
  while (!rep->is_tail())
  {
    fputs(rep->text, file);
    rep = (REPORT *)rep->next();
  }
}


/**---------------------------------------------------------------------------
  * OBJEKT::bedingung_erfuellt()
  * 
  * Kleine Supportfunktion fuer abgeleitete Objekte. Der uebergebene
  * Attributstring wird darauf geprueft, ob er in die Attribute des
  * Objektes "passt" (siehe ATTRIBUT::passen_in()).
  *
  * @param
  * attribut_string: Liste von Attributen als String
  *
  * @return
  * 1, fall ja, sonst 0.
  ---------------------------------------------------------------------------*/
short OBJEKT::bedingung_erfuellt(char *attribut_string)
{
  ATTRIBUT_LISTE bedingungs_attribute;
  bedingungs_attribute.aus_string_einlesen(attribut_string);
  return bedingungs_attribute.passen_in(&attribute);
}


/**---------------------------------------------------------------------------
  * OBJEKT::ort()
  * 
  * Zum Feststellen, in welchem Ort sich ein Objekt befindet, ist es
  * nicht erlaubt, die Variable ort_objekt direkt abzufragen, da sie
  * eventuell durch eine noch aufzuloesende Namensreferenz ersetzt ist.
  * Die Funktion ort() ist die einzig konforme Methode. Uebrigens:
  * Die Adresse (genaue Position innerhalb eines Ortes) darf direkt
  * abgefragt werden (OBJEKT::adresse).
  *
  * @return
  * OBJEKT *    Objekt, in dem sich das implizite Objekt befindet.
  * UNIVERSUM (=NULL), wenn es sich in keinem anderen
  * Objekt befindet.
  ---------------------------------------------------------------------------*/
OBJEKT *OBJEKT::ort()
{
  // Moeglicherweise ist die Referenz auf den Ort noch als Name gegeben
  // (nach dem Laden). In so einem Fall muss sie erst aufgeloest werden.

  if (temp_ort_name) {
    ort_objekt = objekt_mit_namen(temp_ort_name); // Kann auch NULL sein!
    if (!ort_objekt)
      log('I', "OBJEKT::ort(), this->name=%s, Ort-Objekt noch nicht vorhanden",this->name);
    myfree(temp_ort_name);
    temp_ort_name = NULL;
  }
  return ort_objekt;
}

/**---------------------------------------------------------------------------
  * OBJEKT::bewegen()
  * 
  * Dies ist die einzige erlaubte Methode, um die Adresse eines Objektes
  * in einem Ort zu aendern. Eine selbstaendige Aenderung der Adresse
  * ist naemlich nicht gestattet, da der Ort in konformer Weise ueber
  * die Aenderung informiert werden muss und dieser sogar eine Bewegung
  * verbieten kann.
  *
  * @param
  * adr:            Gewuenschte neue Adresse des impliziten Objektes
  *
  * @return  
  * 0, wenn der Ort die Bewegung akzeptiert hat.
  * 1, wenn keine Bewegung zustandegekommen ist.
  ---------------------------------------------------------------------------*/
short OBJEKT::bewegen(ADR& adr)
{
    if (!this) {
      log('I', "OBJEKT::bewegen(): this = NULL");
      return 1;
  }
  // Wenn der ort() eines Objektes NULL ist, dann befindet es sich nicht
  // in einem anderen Objekt, sondern im Universum. Dann kann ich es
  // natuerlich auch nicht bewegen, da es im Universum keine Adresse gibt
  // (Regel 4).

  if (!ort()) return 1; // Das Objekt befindet sich nur im Universum

  // In allen anderen Faellen kann sich das Objekt bewegen, indem es einfach
  // seinen Ort darueber informiert und bei dessen Einverstaendnis seine
  // Adresse selbst aendert:

  if (!ort()->objekt_bewegen(this, adr)) // Ort ist einverstanden
  {
    adresse = adr;
    return 0; // Alles OK
  }
  else return 1; // Ort nicht einverstanden
}

/**---------------------------------------------------------------------------
  * OBJEKT::ort_wechseln()
  * 
  * Dies ist die einzige erlaubte Methode fuer ein Objekt, den Ort zu
  * wechseln. Ein Beschreiben der Variablen ort_objekt und temp_ort_name
  * ist grundsaetzlich verboten. Wenn der neue gewuenschte Ort das
  * Objekt nicht aufnehmen will, bleibt es auf jeden Fall an seinem
  * alten Ort (Garantie!).
  *
  * @param
  * neuer_ort:      Objekt, das als neuer Ort dienen soll
  * neu_adresse:    Gewuenschte Adresse, an der der neue Ort betreten 
  * werden soll.
  *
  * @return
  * 1, wenn das Objekt an seinem alten Platz geblieben ist.
  * 0, wenn der Ortswechsel erfolgreich vollzogen wurde.
  ---------------------------------------------------------------------------*/
short OBJEKT::ort_wechseln(OBJEKT *neuer_ort, ADR& neue_adresse)
{
  // Bevor ich irgendwohin gehen kann, muss ich erst den neuen Ort um
  // Erlaubnis fragen. Wenn die Antwort positiv ist (0), dann rechnet
  // dieser im uebrigen schon mit mir und hat mich schon in seiner Kartei
  // vermerkt. Ist der neue Ort NULL, dann heisst es, dass der Aufrufer
  // ins Universum wechseln will, und dem muss ich natuerlich nicht mit-
  // teilen, dass ich es betrete (das Universum ist ja kein Objekt, sonder
  // einfach NULL).

  if (!neuer_ort || !neuer_ort->objekt_aufnehmen(this, neue_adresse)) // Jepp.
  {
    // Bevor ich meine neue Adresse annehme, muss ich erst dem alten
    // Ort mitteilen, dass ich ihn verlassen habe (falls ich nicht im
    // Universum bin) .

    if (ort()) ort()->objekt_entlassen(this);

    // Nun kann ich meinen Ort und meine Adresse einfach einstellen.

    ort_objekt = neuer_ort;
    adresse = neue_adresse;
    return 0; // Alles OK.
  }

  // Der neue Ort will mich nicht, es bleibt alles beim alten!
  return 1;
}

/**---------------------------------------------------------------------------
  * OBJEKT::verlassen()
  * 
  * Diese Funktion ist ein Spezialfall von OBJEKT::ort_wechseln(),
  * der etwas haeufiger gebraucht wird. "Verlassen" eines Ortes bedeu-
  * tet, den gleichen Ort und die gleiche Adresse anzunehmen, wie der
  * eigene Ort. Befindet sich der eigene Ort im Universum, so gelangt
  * auch das implizite Objekt ins Universum. Befindet sich das Objekt
  * selbst schon im Universum, so kann es diese natuerlich nicht ver-
  * lassen.
  *
  * @return
  * 1, wenn der Ort nicht verlassen werden konnte.
  * 0, wenn alles geklappt hat.
  ---------------------------------------------------------------------------*/
short OBJEKT::verlassen()
{
  if (!ort()) return 1; // Das Universum kann niemand verlassen (c:
  else return ort_wechseln(ort()->ort(), ort()->adresse);
}


/**---------------------------------------------------------------------------
  * OBJEKT::eindeutiger_name
  * 
  * Generiert einen global eindeutigen Namen, der vom Objektnamen
  * abhaengt. Dazu wird der Objektname mit einer Zahl indiziert.
  * Jeder Index wird nur ein einziges Mal verwendet. Der Indexzaehler
  * wird beim Speichern des Objektes mitgespeichert. Sollte ein
  * Name trotz neuem Index schon vorhanden sein, so wird der naechste
  * Index probiert u.s.w. Es besteht also eine Garantie fuer einen
  * eindeutigen Namen.
  *
  * @return
  * static char[]   Zeiger auf ein statisches Array mit dem eindeutigen
  * Namen. Das Array bleibt im Besitz dieser Funktion. Der Name muss
  * kopiert werden (Dies uebernimmt z.B. der OBJEKT-Konstruktor).
  ---------------------------------------------------------------------------*/
char *OBJEKT::eindeutiger_name()
{
  static char antwort[MAX_LAENGE_NAME+15];

  // Da der Objektname eindeutig ist und hier nur indiziert wird, moechte
  // man denken, es koennte nicht passieren, dass ein Name doppelt vergeben
  // wird. Es ist aber moeglich, dass ein Objekt, dass andere Objekte mit
  // Namen erzeugt hat, diese an ein drittes Objekt uebergeben hat und
  // dann vernichtet wurde, sein Name also zur Vergabe wieder frei ist.
  // In diesem Fall kann ein neues Objekt geschaffen werden mit dem ehe-
  // maligen Namen des vernichteten, hat also den gleichen Namensbereich
  // bezueglich dieser Funktion. Deshalb die folgende Schleife!

  do {
    sprintf(antwort, "%s%ld",name, namenszaehler);
    namenszaehler++;
  } while (objekt_mit_namen(antwort)); // Irgendwann muss es klappen

  return antwort;
}


/**---------------------------------------------------------------------------
  * Ermittelt rekursiv den Staat eines Objektes. Fuer Objekte ohne Staat wird 0
  * zurueckgegeben. Keinen Staat haben die UHR, die WELT, die ENZYKLOPAEDIE.
  ---------------------------------------------------------------------------*/

STAAT* OBJEKT::staat()
{
    if (typ_ist("STAAT")) return (STAAT *)this;
    else if (besitzer) return besitzer->staat();
    else return 0;
}
