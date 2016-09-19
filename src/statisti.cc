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
  * MODUL:               statisti.C / STATISTI.CPP
  * AUTOR/DATUM:         Mathias Kettner, 22. September 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Enthaelt Funktionen von UHR, die der Uebersicht halber
//	ausgelagert wurden. Diese Funktionen beschaeftigen sich mit
//	der Erstellung von Statistiken und Punktewertungen fuer die
//	Staaten.
//
//	Enthaelt ausserdem die Funktionen der Klassen HITLISTE und
//	HITLISTEN_EINTRAG, soweit es sich nicht um inline-Funktionen
//	handelt.
//
// **************************************************************************

#include "uhr.h"
#include "listmac.h"
#include "layout.h"
#include "kompatib.h"
#include "html.h"
#include "log.h"

/**---------------------------------------------------------------------------
  * UHR::statistiken_berechnen()
  * 
  *
  ---------------------------------------------------------------------------*/
void UHR::statistiken_berechnen()
{
  // Ich baue eine Liste von Hitlisten auf. Die Liste liegt in meiner Varia-
  // blen hitlisten. Zuerst loesche ich die alten Listen.

  hitlisten.clear();

  // Und nun berechne ich der Reihe nach alle Hitlisten...

  hitliste_erstellen(L("Einwohnerzahl","Number of Citizens"),
		     "EINWOHNERZAHL", "%8ld",           HL_LONG | HL_ABSTEIGEND | HL_DOPPELT);
  hitliste_erstellen(L("Forschung","Developments "),
		     "STAND_DER_FORSCHUNG", "%7ld",     HL_LONG | HL_ABSTEIGEND | HL_DOPPELT);
  hitliste_erstellen(L("Offensivkraft","Military Power"),
		     "OFFENSIVKRAFT", "%7ld",           HL_LONG | HL_ABSTEIGEND | HL_DOPPELT);
  hitliste_erstellen(L("St~adtezahl","Number of Towns"),
		     "STAEDTEZAHL", "%7ld",             HL_LONG | HL_ABSTEIGEND);
  hitliste_erstellen(L("Welterforschung","Exploration"),
		     "BEKANNTE_WELT", "%7ld",           HL_LONG | HL_ABSTEIGEND);
  hitliste_erstellen(L("Rohstoffproduktion","Raw Materials"),
		     "ROHSTOFFPRODUKTION", "%7ld",      HL_LONG | HL_ABSTEIGEND);
  hitliste_erstellen(L("Bau und Entwicklung","Construction and Devel."),
		     "BAU_UND_ENTWICKLUNG", "%7ld",     HL_LONG | HL_ABSTEIGEND);
  hitliste_erstellen(L("Bev~olkerungswachstum","Population Growth"),
		     "BEVOELKERUNGSWACHSTUM", "%5.2f%%",HL_FLOAT| HL_ABSTEIGEND);
}


/**---------------------------------------------------------------------------
  * UHR::hitliste_erstellen()
  * 
  * Erstellt eine neue Hitliste zu einem bestimmten Thema und fuegt diese
  * zur Menge der Hitlisten hinzu.
  *
  * @param
  * char *name:	Name der Hitliste.
  *
  * char *infoname:	Info, mit dem beim Objekt STAAT die Information
  * ueber das Abschneiden eines Staates erfragt werden
  * kann.
  *
  * char *format:	Formatstring, der zur Ausgabe der Werte der Ein-
  * traege beim Ausdrucken der Liste verwendet werden
  * soll. Z.B. "%.2f" fuer Prozentwerte oder "%ld"
  * fuer normale Ganzzahlen. Der Formatstring muss
  * statisch sein (es gibt wohl eh' keinen Grund, ihn
  * anders zu definieren).
  *
  * short flags:	Angaben, in welcher Reihenfolge sortiert werden soll,
  * und welche Art von Daten zu erwarten sind (long/float).
  * z.B. HL_LONG | HL_AUFSTEIGEND.
  ---------------------------------------------------------------------------*/
void UHR::hitliste_erstellen(char *name, char *infoname, char *format,
				 short flags)
{
  HITLISTE *hitliste = new HITLISTE(name, format, flags);

  // Und nun von jedem Staat das Info anfordern und einen neuen Eintrag
  // anlegen und diesen einsortieren.

  FOR_EACH_OBJEKT_IN (alle_staaten()) DO_AND_DELETE
  (
    if (flags & HL_LONG) {
      long wert = myatol(objekt->info(infoname));
      hitliste->eintraege.insert((DOUBLIST_NODE *)
				new HITLISTEN_EINTRAG(objekt, wert));
    }
    else if (flags & HL_FLOAT) {
      float wert = myatof(objekt->info(infoname));
      hitliste->eintraege.insert((DOUBLIST_NODE *)
				new HITLISTEN_EINTRAG(objekt, wert));
    }
    else break; /* Huch? */
  )

  // Nun muss ich noch sortieren. Die Sortierfunktion stelle ich anschlies-
  // send bereit.

  hitliste->eintraege.sort(UHR::sortfunction_hitliste, (void *)&flags);

  // Und nun noch die Hitliste in meine Liste der Hitlisten aufnehmen..
  hitlisten.insert((DOUBLIST_NODE *)hitliste);

  // Das war's auch schon wieder...
}


/**---------------------------------------------------------------------------
  * UHR::sortfunction_hitliste()
  * 
  * Vergleichsfunktion fuer Hitlisten. Sie beruecksichtigt die Flags
  * und vergleicht dann entweder die long oder die floatwerte.
  *
  * @param
  * DOUBLIST_NODE *s1, *s2: Zeiger auf die HITLISTEs.
  * void *data:			Zeiger auf die flags (short).
  *
  * @return
  * short 1, wenn getauscht werden muss, sonst short 0.
  ---------------------------------------------------------------------------*/
short UHR::sortfunction_hitliste(DOUBLIST_NODE *s1,
				DOUBLIST_NODE *s2, void *data)
{
  short flags = *(short *)data;

  HITLISTEN_EINTRAG *l1, *l2;
  l1 = (HITLISTEN_EINTRAG *)s1;
  l2 = (HITLISTEN_EINTRAG *)s2;

  if ((flags&HL_LONG) && (flags&HL_AUFSTEIGEND)) return l1->longwert > l2->longwert;
  else if ((flags&HL_LONG) && (flags&HL_ABSTEIGEND)) return  l1->longwert < l2->longwert;
  else if ((flags&HL_FLOAT) && (flags&HL_AUFSTEIGEND)) return l1->floatwert > l2->floatwert;
  else if ((flags&HL_FLOAT) && (flags&HL_ABSTEIGEND)) return l1->floatwert < l2->floatwert;

  else return 0; // Huch?
}


/**---------------------------------------------------------------------------
  * HITLISTE::punkte_auf_platz()
  * 
  * Ermittelt, wieviele Punkte ein Spieler bei einer Auswertung kommt,
  * wenn er auf einem bestimmten Platz in dieser Liste ist.
  *
  * @param
  * short platz:	Position in der Liste (von 1 bis ...)
  *
  * @return
  * long: Punkte, die er bekommt (von 0 bis ...)
  ---------------------------------------------------------------------------*/
long HITLISTE::punkte_auf_platz(short platz)
{
  // Grundproblem: Was mache ich, wenn zwei Staaten in einer Liste gleiche
  // Werte haben? Aus organisatorischen Gruenden will ich darauf verzichten,
  // dass ich eine Platznummer mehrmals vergebe. Deshalb loese ich das Pro-
  // blem ueber die Verteilung der Werte. Ich untersuche, wieviele Spieler
  // den gleichen Wert haben. Dann teilen sich all diese Spieler die Punkte
  // der Plaetze, die ihnen zustehen. Wenn z.B. zweit- und drittbester
  // Spieler in einer einfachen Liste gleiche Werte haben, so bekommt
  // jeder (15+10)/2 = 12.5 => 13 Punkte

  HITLISTEN_EINTRAG *eintrag = (HITLISTEN_EINTRAG *)eintraege.first(),
			*eintrag_alt = NULL;
  short platzierung = 0; // Dient als Zaehler fuer die aktuelle Position

  // Als "Cluster" bezeichne ich eine Folge von Eintraegen mit gleichen
  // Werten.

  short clustergroesse=0; // Soviele Eintraege haben gleichen Wert
  short clusterpunkte=0;  // Kumulierte Punkte fuer diesen Cluster
  short clusterhit=0; // true, wenn der Cluster den gesuchten Platz enthaelt

  while (!eintrag->is_tail()) {
    platzierung++;

    // Frage: Gehoert der naechste Eintrag noch zum aktuellen Cluster?
    // Genau dann, wenn ein Vergleich mit dem alten Eintrag eine Gleichheit
    // bezueglich des Wertes ergiebt.

    if (!eintrag->gleicher_wert_wie(eintrag_alt)) { // Nein: Neuer Cluster
      // Es muesste also ein neuer Cluster angelegt werden. Wenn der alte
      // Cluster aber bereits den gesuchten Platz enthaelt, dann kann ich die
      // Punkte ausrechnen und die ganze Funktion beenden.
      if (clusterhit) {
	return ((clusterpunkte*10/clustergroesse) +5 ) / 10; // 5/4-Rundung
      }

      clustergroesse = 0;
      clusterpunkte = 0;
      clusterhit = 0;
    }

    // Den aktuellen Eintrag rechne ich jetzt zum aktuellen Cluster hinzu.
    if (platzierung == platz) clusterhit = 1;
    clustergroesse ++;
    clusterpunkte += punkte_auf_platz_pur(platzierung);

    // Jetzt fuehre ich die Schleife fort
    eintrag_alt = eintrag;
    eintrag = (HITLISTEN_EINTRAG *)eintrag->next();
  }

  // Wenn die Schleife auf diese Art verlassen wird, dann ist es entweder
  // der Fall, dass der gesuchte Platz im letzten Cluster liegt (wobei
  // clusterhit dann auf 1 steht), oder die Liste enthaelt nicht genug
  // Eintraege, um den gesuchten Platz zu enthalten (Was einem Fehler
  // entspricht!)

  if (clusterhit) {
	return ((clusterpunkte*10/clustergroesse) +5 ) / 10; // 5/4-Rundung
  }

  log('I', "HITLISTE::punkte_auf_platz(%d): Fehler", platz);
  return 0;
}


/**---------------------------------------------------------------------------
  * HITLISTE::punkte_auf_platz_pur()
  * 
  * Hilfsfunktion von punkte_auf_platz(), die aus zwei Tabellen die
  * Punkte fuer einen bestimmten Platz der Hitliste ermittelt.
  *
  * @param
  * short platz:	Position in der Liste (von 1 bis ...)
  *
  * @return
  * long: Punkte, die fuer den Platz kontingentiert sind.
  ---------------------------------------------------------------------------*/
long HITLISTE::punkte_auf_platz_pur(short platz)
{
  // Hier kommen zwei Tabellen fuer eine Unterschiedliche Bewertung der
  // einfachen und der staerker gewichteten Listen mit dem Flags
  // HL_DOPPELT. Tabelle 1 ist fuer die doppelten, Liste 2 fuer die einfa-
  // chen Listen

  static const short anzahl_1 = 20, anzahl_2 = 10;
  static short punkte_1_auf_platz[anzahl_1] =
  { 50, 40, 35, 30, 25, 20, 18, 16, 14, 12, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
  static short punkte_2_auf_platz[anzahl_2] =
  { 20, 15, 10, 8, 6, 5, 4, 3, 2, 1 };

  // Nun mal sehen, von welchem Typ ich bin...

  if (flags & HL_DOPPELT) {
    if (platz > anzahl_1) return 0;
    else return punkte_1_auf_platz[platz-1]; // -1, weil Tabelle bei 0 beginnt
  }
  else {
    if (platz > anzahl_2) return 0;
    else return punkte_2_auf_platz[platz-1];
  }
}


/**---------------------------------------------------------------------------
  * HITLISTE::punkte_fuer_staat()
  * 
  * Ermittelt, wieviele Punkte ein Staat fuer seine Leistung in dieser
  * Hitliste bekommt.
  *
  * @param
  * OBJEKT *staat:	Fuer diesen Staat
  *
  * @return
  * long Punkte, die er bekommt.
  ---------------------------------------------------------------------------*/
long HITLISTE::punkte_fuer_staat(OBJEKT *staat)
{
  // Zuerst muss ich die Position des Staates ermitteln. Dazu kann ich lei-
  // der nicht die find()-Funktion verwenden, da sie mir keinen Auf-
  // schluss ueber die Position gibt.

  HITLISTEN_EINTRAG *eintrag = (HITLISTEN_EINTRAG *)eintraege.first();
  short platzierung = 0;
  while (!eintrag->is_tail()) {
    platzierung++;
    if (eintrag->staat == staat) break;
    eintrag = (HITLISTEN_EINTRAG *)eintrag->next();
  }

  if (eintrag->is_tail()) return 0; // Ist gar nicht in der Liste!
  else return punkte_auf_platz(platzierung);
}


/**---------------------------------------------------------------------------
  * HITLISTE::layout_erstellen()
  * 
  * Erstellt ein druckfaehiges Layout von der Hitliste, wie es im
  * Ergebnisausdruck bei der Punkteauswertung erscheinen soll.
  * Dabei werden eine bestimmte Anzahl der vorderen Plaetze expli-
  * zit aufgelistet.
  *
  * @param
  * float breite, hoehe:	gewuenschte Ausmasse des Layouts in cm.
  * OBJEKT *staat:	Staat, fuer dessen Ausdruck layoutet wird.
  * Das ist wichtig, da die eigene Position in
  * der Liste hervorgehoben werden soll.
  * short plaetze:		Anzahl der Plaetze, die explizit ausgegeben
  * werden sollen.
  * float titel_links:	Abstand des Titels vom linken Rand in cm.
  * Damit kann der Titel (, der in Proportional-
  * schrift ausgegeben wird) von Hand zentriert
  * werden.
  *
  * @return
  * LAYOUT *: Zeiger auf fertiges Layout, dass dann gedruckt und
  * anschliessend deleted werden sollte.
  ---------------------------------------------------------------------------*/
LAYOUT *HITLISTE::layout_erstellen(float breite, float hoehe,
			  OBJEKT *staat, short plaetze, float titel_links)
{
  // Erstmal lege ich ein frisches Layout an.
  LAYOUT *ly = new LAYOUT;

  // Als erstes kommt ein Rahmen aussenrum.
  ly->rechteck(0, 0, breite, hoehe);

  // Nachdem die Groesse der Tabelle dynamisch sein soll, muss ich alle
  // meine Groessenangaben in Prozentwerten von breite und hoehe angeben.

  // Die Ueberschrift kommt in einen Kasten ganz oben. Die Groesse des
  // Kastens mache ich von der Breite abhaengig.

  float ueberschrift_unten = breite/9;
  ly->linie(0, ueberschrift_unten, breite, ueberschrift_unten);
  ly->text(titel_links, ueberschrift_unten - breite*.023, name,
				"Times", long(ueberschrift_unten * 25));

  // Als naechstes kommt die Legende mit Pl. Wert Staat Pkt.

  short legendenschriftgroesse = short(breite*2);
  float legende_unten = 1.8 * ueberschrift_unten;
  float platz_links = breite*.04, wert_links = breite*.10,
	staat_links = breite*.35, punkte_links = breite*.90;

  ly->text(platz_links-breite*0.02, legende_unten,
	   L("Pl.","P."), "Times", legendenschriftgroesse);
  ly->text(wert_links+breite*0.05, legende_unten,
	   L("Wert","Value"), "Times", legendenschriftgroesse);
  ly->text(staat_links, legende_unten,
	   L("Staat","Empire"), "Times", legendenschriftgroesse);
  ly->text(punkte_links-breite*0.03, legende_unten,
	   L("Pkt.","Pts."), "Times", legendenschriftgroesse);

  // Nun kommt der variable Teil. Ich drucke die ersten plaetze Plaetze der
  // Liste aus und muss aber noch Raum fuer einen weiteren Eintrag frei-
  // halten, in dem die Platzierung des Staates staat steht, wenn er nicht
  // unter den ersten Plaetzen rangiert. Den restlichen Raum teile ich
  // als durch plaetze+1, nachdem ich noch einen gewissen Randabstand nach
  // unten einberechnet habe.

  float restplatz = hoehe - legende_unten - 0.08 * breite;
  float zeilenabstand = restplatz/(plaetze+1);
  float tabelle_oben = legende_unten + zeilenabstand + 0.02 * breite;
  short tabellenschriftgroesse = legendenschriftgroesse;

  // Jetzt durchlaufe ich mit eine Schleife die Eintraege der Hitliste so-
  // lange, bis ich den Staat staat gefunden habe, mindestens aber, bis
  // plaetze Eintraege ausgedruckt sind. Gleichzeitig merke ich mir die
  // Platzierung des Staates.

  short platzierung = 0, aktueller_platz=1;
  short platznummer = 1; // Zum ausdrucken der Platznummer, wenn zwei Pl. gleich
  float eintrag_oben = tabelle_oben;
  HITLISTEN_EINTRAG *eintrag = (HITLISTEN_EINTRAG *)eintraege.first();
  while (!eintrag->is_tail() && (!platzierung || aktueller_platz<=plaetze))
  {
    // 'mal sehen, ob unser Staat an der Reihe ist...
    short bin_jetzt_dran = 0;
    if (eintrag->staat == staat) {
      bin_jetzt_dran = 1;
      platzierung = aktueller_platz; // Jepp.
    }

    // Ich gebe die Eintraege nur dann aus, wenn ich bei einem der ersten
    // Plaetze bin, oder wenn mein Staat gerade dran ist.

    if (aktueller_platz <= plaetze || bin_jetzt_dran) {

      // der graue Rahmen muss zuerst gedruckt werden.
      if (bin_jetzt_dran) {
	// An dieser Stelle will ich den eigenen Eintrag noch durch einen
	// Rahmen hervorheben.
	#define RMASSE breite*0.02, eintrag_oben-zeilenabstand*.7, \
		       breite*0.98, eintrag_oben+zeilenabstand*.24
	ly->rechteck_ausgefuellt(RMASSE,10);
	ly->rechteck(RMASSE);
      }

      ly->text(platz_links, eintrag_oben, myltoa(platznummer),
	"Times", tabellenschriftgroesse);

      // Der Wert wird durch einen String formatiert, der in der Hitliste
      // gespeichert ist.

      char wertstring[80];
      if (flags & HL_FLOAT)
	sprintf(wertstring, formatstring, eintrag->floatwert);
      else if (flags & HL_LONG)
	sprintf(wertstring, formatstring, eintrag->longwert);
      else wertstring[0]=0; // Huch?
      ly->text(wert_links, eintrag_oben, wertstring,"Times", tabellenschriftgroesse);

      // Der Name des Staates kommt als naechstes. Ihn muss ich auf eine
      // bestimmte Zeichenanzahl begrenzen.

      strncpy(wertstring,eintrag->staat->attribut("NAME"),15);
      if (strlen(wertstring) >= 14) { // Dann Abkuerzungspunkt.
	wertstring[14]='.';
	wertstring[15]=0;
      }

      ly->text(staat_links, eintrag_oben, wertstring, "Times", tabellenschriftgroesse);

      // Und zu guterletzt noch die Punktewertung

      sprintf(wertstring, "%2ld",punkte_auf_platz(aktueller_platz));
      ly->text(punkte_links, eintrag_oben, wertstring,"Times", tabellenschriftgroesse);



      // Nun noch so eine Art Zeilenvorschub...
      eintrag_oben += zeilenabstand;

    } // if (..Ausdrucken..)

    // Ich kann die Schleife jetzt beenden.
    aktueller_platz++;
    eintrag = (HITLISTEN_EINTRAG *)eintrag->next();
    if (!eintrag->is_tail() && punkte_auf_platz(aktueller_platz) !=
			       punkte_auf_platz(aktueller_platz-1))
    {
      platznummer = aktueller_platz;
    }
  }

  // So. Dann haett ich's
  return ly;
}


/**---------------------------------------------------------------------------
  * HITLISTE::layout_erstellen_html(HTML&, OBJEKT*, short, const char *)
  * 
  * Erstellt ein Layout der Hitliste in eine offenes HTML-Objekt.
  * Dabei werden eine bestimmte Anzahl der vorderen Plaetze expli-
  * zit aufgelistet.
  *
  * @param
  * OBJEKT *staat:	Staat, fuer dessen Ausdruck layoutet wird.
  * Das ist wichtig, da die eigene Position in
  * der Liste hervorgehoben werden soll.
  * short plaetze:		Anzahl der Plaetze, die explizit ausgegeben
  * werden sollen.
  ---------------------------------------------------------------------------*/
void HITLISTE::layout_erstellen_html(HTML html, OBJEKT *staat, short plaetze, char *farbe)
{
    // Ich mache eine Tabelle in der Tabelle. Die aeussere Tabelle hat eine
    // Zelle mit der Ueberschrift und eine mit dem Inhalt.

    html.set_table_border(1)
	.set_cell_spacing(0)
	.set_table_color(farbe)
	.set_cell_valignment(VAL_TOP)
	.table();
    html.next_row()
	.set_cell_alignment(AL_CENTER)
	.next_cell()
	.font_size(2)
	.text(name)
	.end_font();
    html.next_row()
	.next_cell();

    // Jetzt kommt die eigentlich Liste

    html.unset_table_border()
	.unset_table_color()
	.unset_cell_spacing()
	.table(); // TODO: Eigentlich keine verschachtelten Tabellen erlaubt!

    html.next_row() // Spaltenueberschriften.
	.next_cell(L("Pl.","P."))
	.next_cell(L("Wert","Value"))
	.next_cell(L("Staat","Empire"))
	.next_cell(L("Punkte","Points"));

    // Jetzt durchlaufe ich mit eine Schleife die Eintraege der Hitliste so-
    // lange, bis ich den Staat staat gefunden habe, mindestens aber, bis
    // plaetze Eintraege ausgedruckt sind. Gleichzeitig merke ich mir die
    // Platzierung des Staates.
    
    short platzierung = 0, aktueller_platz=1;
    short platznummer = 1; // Zum ausdrucken der Platznummer, wenn zwei Pl. gleich
    
    HITLISTEN_EINTRAG *eintrag = (HITLISTEN_EINTRAG *)eintraege.first();
    while (!eintrag->is_tail() && (!platzierung || aktueller_platz <= plaetze))
    {
	// 'mal sehen, ob unser Staat an der Reihe ist...
	short bin_jetzt_dran = 0;
	if (eintrag->staat == staat) {
	    bin_jetzt_dran = 1;
	    platzierung = aktueller_platz; // Jepp.
	}

	// Ich gebe die Eintraege nur dann aus, wenn ich bei einem der ersten
	// Plaetze bin, oder wenn mein Staat gerade dran ist.

	if (aktueller_platz <= plaetze || bin_jetzt_dran)
	{
	    if (bin_jetzt_dran) // Farbig hinterlegen.
		html.set_row_color("#ffff00");
	    else html.unset_row_color();
	    
	    html.next_row();
	    html.set_cell_alignment(AL_RIGHT)
		.next_cell(platznummer);
	    
	    // Der Wert wird durch einen String formatiert, der in der Hitliste
	    // gespeichert ist.
	    
	    char wertstring[80];
	    if (flags & HL_FLOAT)
		sprintf(wertstring, formatstring, eintrag->floatwert);
	    else if (flags & HL_LONG)
	    sprintf(wertstring, formatstring, eintrag->longwert);
	    else wertstring[0]=0; // Huch?
	    html.next_cell(wertstring);
	    
	    // Der Name des Staates kommt als naechstes. Im HTML-Ausdruck begrenze
	    // ich ihn nicht auf eine Maximallaenge.
	    
	    html.set_cell_alignment(AL_LEFT)
		.next_cell(eintrag->staat->a_name());
	    
	    // Und zu guterletzt noch die Punktewertung
	    html.set_cell_alignment(AL_RIGHT)
		.next_cell(punkte_auf_platz(aktueller_platz));
	}

	// Ich kann die Schleife jetzt beenden.
	aktueller_platz++;
	eintrag = (HITLISTEN_EINTRAG *)eintrag->next();
	if (!eintrag->is_tail() && punkte_auf_platz(aktueller_platz) !=
	    punkte_auf_platz(aktueller_platz-1))
	{
	    platznummer = aktueller_platz;
	}
    }
    
    // Innere Tabelle abschliessen.
    html.end_table();
    html.unset_row_color();

    // Aussere Tabelle abschliessen
    html.end_table();
}
