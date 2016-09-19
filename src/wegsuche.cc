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
  * MODUL:               wegsuche.C  /  WEGSUCHE.CPP
  * AUTOR/DATUM:         Mathias Kettner, 5. August 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Diejenigen Funktionen von WELT, die den Wegfindungs-Algorithmus
//	realisieren, der hauptsaechlich von den Einheiten fuer den SC-Befehl
//	gebraucht wird.
//
// **************************************************************************

#include "welt.h"
#include "kompatib.h"
#include "wegsuche.h"
#include "log.h"

/**---------------------------------------------------------------------------
  * WELT::schnellster_weg()
  * 
  * Das Grundproblem, das mich zu diesem Algorithmus motivierte, ist
  * die moeglichst autonome und flexible Steuerung der Objekte im Spiel.
  * Im Verlauf des Spiels werden die Reiche groesser und komplexer wer-
  * den. Immer mehr Einheiten und Staedte muessen gesteuert werden. Dies
  * soll aber mit eine begrenzten Anzahl von Befehlen trotzdem effektiv
  * geschehen. Zum zweiten ist eine Einheit eine relativ lange Zeit
  * auf sich selbst gestellt waehrend eines Zuges.
  *
  * Dieser Algorithmus soll den schnellsten Weg finden, auf dem eine
  * Einheit von A nach B kommt. Das dies leider nicht immer der kuerze-
  * ste ist, liegt an unterschiedlichen Bewegungsgeschwindigkeiten auf
  * unterschiedlichen Routen. Vor allem Strassen und Eisenbahn verzerren
  * die Sachlage.
  *
  * Der Spieler hat Zugriff auf diesen Algorithmus mit dem Befehl GZ adr,
  * bei dem er nur den Zielort angeben muss, und die Einheit zieht selb-
  * staendig so schnell wie moeglich dort hin. Auch andere Befehle grei-
  * fen aber auf die automatische Wegfindung zurueck.
  *
  * @param
  * start, ziel: Von wo nach wo ueberhaupt ein Weg gesucht wird.
  * entfernungsfunktion: Zeiger auf eine Funktion, die mithilfe
  * von zwei ADR und einem dritten Parameter
  * vom Typ void * ermittelt, wie weit zwei
  * Felder voneinander entfernt sind.
  * void *entfernungsdata: Wird der Entfernungsfunktion mitgegeben.
  * long *laenge:	   Ausgabeparameter, in den die Laenge des
  * errechneten Weges uebertragen wird, sofern der
  * Zeiger nicht auf NULL zeigt. Wird -1, wenn es
  * keinen Weg gibt.
  *
  * @return
  * DOUBLIST * Zeiger auf eine Liste mit Nodes vom Typ
  * ADR_LIST_NODE. Sie beschreiben die Felder
  * auf den schnellsten Weg in der Reihenfolge vom
  * Start zum Ziel. Die Liste muss nach Gebrauch
  * freigegeben werden. Das Startfeld selbst befindet
  * sich nicht in der Liste.
  *
  * NULL,	   wenn nicht genug Arbeitsspeicher vorhanden war,
  * wenn das Ziel garnicht erreicht werden kann, oder
  * wenn der Weg zum Ziel eine gewisse (eher utopische)
  * Laenge ueberschreitet.
  ---------------------------------------------------------------------------*/
DOUBLIST *WELT::schnellster_weg(ADR& start, ADR& ziel,
 EFKT entfernungsfunktion, void *entfernungsdata, long *laenge)
{
  // Im ersten Schritt lege ich eine Hilfmatrix an, die ich zum numerieren
  // der Felder brauche.

  short **hilfsmatrix = hilfsmatrix_anlegen();

  // Im zweiten Schritt numeriere ich die Matrix. In jedes Feld, dass auf
  // dem Weg liegen koennte, wird die Entfernung vom Ausgangspunkt und
  // Bewegungsdauer eingetragen (also nicht Luftlinie, sondern die Anzahl
  // der Runden, die tatsaechlich benoetigt wird, um es zu erreichen).

  DOUBLIST *wegliste = NULL; // Fuer die Antwort.

  long rwert = hilfsmatrix_numerieren(start, ziel, hilfsmatrix,
	entfernungsfunktion, entfernungsdata);
  if (-1 != rwert)  // Hat bei != -1 geklappt.
  {
    // Im dritten Schritt kann ich vom Ziel aus den schnellsten Weg zurueckrech-
    // nen. Hier bekomme ich auch schon die Liste mit dem Weg.

    wegliste = weg_zurueckrechnen(hilfsmatrix, ziel);
  }

  // Nun raeume ich noch meine Arbeitsdaten auf und gebe das Ergebnis zurueck.

  hilfsmatrix_vernichten(hilfsmatrix);
  if (laenge) *laenge = rwert;
  return wegliste;
}

/**---------------------------------------------------------------------------
  * WELT::hilfsmatrix_anlegen()
  * 
  * Legt Speicher fuer eine Matrix in der Groesse der Welt an.
  * Belegt alle Felder mit dem Wert WS_OHNE_NUMMER.
  ---------------------------------------------------------------------------*/
short **WELT::hilfsmatrix_anlegen()
{
  short **hm = new short *[breite];
  if (!hm) return NULL;

  for (int i=0; i<breite; i++)
  {
    hm[i] = new short[hoehe];
    if (!hm[i]) return NULL; // Etwas Speicher geht so verloren, aber das ist
			     // ohnehin schon gleichgueltig.
    else  { // Jetzt muss die Spalte noch initialisiert werden.
      for (int j=0; j<hoehe; j++) hm[i][j] = WS_OHNE_NUMMER;
    }
  }

  return hm;
}


/**---------------------------------------------------------------------------
  * WELT::hilfsmatrix_vernichten(short **hm)
  * 
  * Gibt dem Hilfsspeicher wieder frei.
  ---------------------------------------------------------------------------*/
void WELT::hilfsmatrix_vernichten(short **hm)
{
  if (hm) {
    for (int i=0; i<breite; i++) if (hm[i]) {
       delete hm[i];
       hm[i]=NULL;
    };
    delete hm;
  }
}


/**---------------------------------------------------------------------------
  * WELT::hilfsmatrix_numerieren()
  * 
  * Numeriert die Hilfsmatrix nach dem Algorithmus von Dejkstra.
  * @return
  * -1, wenn es keinen Weg gibt,
  * die Laenge des kuerzesten Weges sonst.
  ---------------------------------------------------------------------------*/
long WELT::hilfsmatrix_numerieren(ADR& start, ADR& ziel,
	short **hilfsmatrix, EFKT entfernungsfunktion, void *entfernungsdata)
{
  // Dieser Teil des Algorithmus wird begleitet von einer Datenstruktur vom
  // Typ WS_LISTEN_MENGE, in der Listen von Listen von Adressen stehen.
  // Die Listen sind nach Feldnumerierung sortiert.

  WS_LISTEN_MENGE listenmenge(start);

  ADR adr; // Adresse des aktuellen Feldes
  short aktnummer; // Dessen Numerierung
  short kleinste_zielentfernung = -1; // Bis jetzt schnellster weg.
  hilfsmatrix[start.x][start.y] = 0; // Der Start wird sonst nicht numeriert.

  while (WS_LISTE_LEER != (aktnummer=listenmenge.naechstes_feld_holen(adr)))
  {
    // Und nun numeriere ich alle Felder, die benachbart sind.
    static short off_x[] = {1, 1, 1, 0, 0, -1, -1, -1};
    static short off_y[] = {1, 0,-1, 1,-1,  1,  0, -1};
    for (short i=0; i<8; i++)
    {
      ADR feld(adr.x + off_x[i],  adr.y + off_y[i]);
      wrap(&feld);
      if (feld.y >= hoehe || feld.y < 0) continue;

      // Hier kommt eine Optimierung: Wenn das Feld, von dem ich mir
      // ueberlege, wie ich es numerieren soll, schon eine Nummer hat,
      // und diese Nummer <= der aktuellen Nummer ist, dann brauche
      // ich die Entfernung garnicht berechnen. Es wuerde ohnehin
      // groesser werden.
      
      if (hilfsmatrix[feld.x][feld.y] != WS_OHNE_NUMMER &&
	  hilfsmatrix[feld.x][feld.y] <= aktnummer) continue;

      short entfernung = entfernungsfunktion(entfernungsdata, adr, feld);

      // Wenn die entfernung -1 ist, dann heisst das, dass das Feld garnicht
      // betreten werden kann. Ich kann mit diesem Feld gleich abbrechen.
      if (entfernung < 0) continue;

      short nummer = aktnummer + entfernung;

      // Sollte die nummer schon groesser oder gleich der bisher kuerzesten
      // Zielentfernung sein (falls Ziel gefunden), dann kann ich es gleich
      // lassen, da ich auf diesem Weg das Ziel mit Sicherheit nicht mehr
      // am schnellsten erreiche...

      if (nummer > kleinste_zielentfernung && kleinste_zielentfernung != -1)
	continue;

      // Jetzt schaue ich nach, ob das Feld schon numeriert ist. Wenn ja, dann
      // numeriere ich es trotzdem nochmal, wenn es dadurch eine kleinere
      // Nummer bekommt. Wenn nein, dann sowieso.

      if (hilfsmatrix[feld.x][feld.y] == WS_OHNE_NUMMER ||
	  hilfsmatrix[feld.x][feld.y] > nummer) // Dann numerieren.
      {
	hilfsmatrix[feld.x][feld.y] = nummer;
	listenmenge.feld_einfuegen(feld, nummer);

	// Nun schaue ich doch auch gleich, ob ich nicht gerade das Zielfeld
	// numeriert habe...

	if (feld == ziel) // In der Tat!
	  if (kleinste_zielentfernung == -1 || nummer < kleinste_zielentfernung)
	    kleinste_zielentfernung = nummer;
      }
    }

  } // for (alle 8 Nachbarfelder)

  // So. Weiter numerieren muss ich die Hilfsmatrix nicht. Wenn das Ziel
  // gefunden ist, dann muss die Variable kleinste_zielentfernung != -1
  // sein. Ansonsten gibt sie die Entfernung zum Ziel an. Genau diesen
  // Wert gebe ich zurueck.

  return (long)kleinste_zielentfernung;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
DOUBLIST *WELT::weg_zurueckrechnen(short **hilfsmatrix, ADR& ziel)
{
  // Wenn ich die Numerierung 0 gefunden habe, dann hoere ich auf.

  // Erstmal eine Liste anlegen.
  DOUBLIST *antwort = new DOUBLIST;
  ADR adr = ziel;
  ADR_LIST_NODE *zieladr = new ADR_LIST_NODE(ziel);
  antwort->add_head(zieladr);

  while (1) {
    DOUBLIST *adrliste = alle_adressen_im_umkreis_von(adr, 1.5);

    ADR schritt;
    short kleinste_nummer = -1;
    while (!adrliste->is_empty())
    {
      ADR test = ((ADR_LIST_NODE *)adrliste->first())->adresse;
      short testnummer = hilfsmatrix[test.x][test.y];
      if (testnummer != WS_OHNE_NUMMER && (kleinste_nummer == -1 ||
	 testnummer < kleinste_nummer))
      {
	kleinste_nummer = testnummer;
	schritt = test;
      }
      delete adrliste->first();
    }
    delete adrliste;
    if (kleinste_nummer == -1) { // Darf nie sein!
	log('I', "WELT::weg_zurueckrechnen(): Kein Anschlussfeld!");
	return NULL;
    }

    if (kleinste_nummer == 0) break; // Das war dann der Start.
    else antwort->add_head(new ADR_LIST_NODE(schritt)); // Der Start selbst soll nicht auftauchen
    adr = schritt; // Nun muss ich natuerlich ab hier weitermachen.
  }

  return antwort;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
WS_LISTEN_MENGE::WS_LISTEN_MENGE(ADR& start)
{
  // Ich lege eine Liste mit Nummer 0 und einem einzigen Eintrag start an.

  feld_einfuegen(start, 0);
}



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
WS_ADRESSLISTE *WS_LISTEN_MENGE::liste_mit_nummer(short nummer)
{
  return (WS_ADRESSLISTE *)find((void *)&nummer);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
WS_ADRESSLISTE *WS_LISTEN_MENGE::neue_liste_anlegen(short nummer)
{
  // Zur Vorsicht schaue ich mal, ob es nicht schon eine Liste mit der
  // Nummer gibt. Wenn ja, dann gebe ich eine Fehlermeldung aus, da dann
  // am Algorithmus irgendwas nicht stimmt.

    if (liste_mit_nummer(nummer)) { // Gibt's schon! Fehler.
	log('I', "WS_LISTEN_MENGE::neue_liste_anlegen(): "
	    "Liste fuer %d gibt's schon!" , nummer);
	return NULL;
    }

  // Jetzt erzeuge ich die neue Adressliste

  WS_ADRESSLISTE *al = new WS_ADRESSLISTE(nummer); // Beginn bei 0

  // Zum Einfuegen der Liste muss ich erst noch die richtige Stelle finden,
  // da die Listen der Nummer nach sortiert sind.

  WS_ADRESSLISTE *suche = (WS_ADRESSLISTE *)first();
  while (!suche->is_tail() && suche->nummer < nummer) {
      suche = (WS_ADRESSLISTE *)suche->next();
  }

  // Am Ende dieser Schleife hatten entweder alle Listen eine kleinere Nummer,
  // wobei suche dann auf den tail zeigt, oder es wurde abgebrochen, weil
  // die nummer kleiner war. In diesem Fall steht suche auf der ersten Liste
  // mit groesserer Nummer. In beiden Faellen ist der richtige Platz zum
  // Einfuegen VOR der Node, auf die suche am Ende zeigt.

  al->insert_before(suche);
  return al;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void WS_LISTEN_MENGE::feld_einfuegen(ADR& adr, short nummer)
{
  // Falls die Liste noch nicht existiert, so muss ich sie erst erzeugen.
  WS_ADRESSLISTE *al = liste_mit_nummer(nummer);
  if (!al) al = neue_liste_anlegen(nummer);
  if (!al) return;

  // Jetzt erzeuge ich eine neue Adressnode.

  ADR_LIST_NODE *adrnode = new ADR_LIST_NODE(adr);

  // Wo ich in der Liste al einfuege ist eigentlich recht wurst. Also
  // verwende ich insert()

  al->adressliste.insert(adrnode);
}


/**---------------------------------------------------------------------------
  * 
  * 
  * @return
  * short : Nummerierung des geholten Feldes, falls die Liste nicht leer
  * war.
  * WS_LISTE_LEER, fall die Liste leer war. adr bleibt dann unveraendert.
  ---------------------------------------------------------------------------*/
short WS_LISTEN_MENGE::naechstes_feld_holen(ADR& adr)
{
  // adr ist Rueckgabeparameter! Bei WS_LISTE_LEER als Rueckgabe ist
  // kein Feld mehr zur Verarbeitung.

  if (is_empty()) return WS_LISTE_LEER;
  WS_ADRESSLISTE *al = (WS_ADRESSLISTE *)first();

  ADR_LIST_NODE *adrnode = (ADR_LIST_NODE *)al->adressliste.first();
  if (!adrnode) { // List ist leer. Darf nie sein!
      log('I', "WS_LISTEN_MENGE::naechstes_feld_holen(): Leere Liste!");
      return WS_LISTE_LEER;
  }
  
  adr = adrnode->adresse;
  short nummer = al->nummer;
  delete adrnode;
  if (al->adressliste.is_empty()) delete al; // Wird nicht mehr gebraucht, wenn leer.

  return nummer;
}
