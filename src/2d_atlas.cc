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
  * MODUL:               2d_atlas.C  /  2D_ATLAS.CPP
  * AUTOR/DATUM:         Mathias Kettner, 25. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
// 
//      Funktionen der Klasse 2D_MATRIX_ATLAS und der abgeleiteten
//      Klassen RECHTECK_ATLAS, ..., die davon abgeleitet sind
//
// **************************************************************************
	     
#include "alg.h"
#include "landscha.h"
#include "kompatib.h"
#include "listmac.h"

#include "log.h"

extern DOUBLIST globale_objekt_menge;

/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::ZWEID_MATRIX_ATLAS()
  * 
  * Der Konstruktor des zweidimensionalen Atlas ist derjenige unter
  * den hierarchisch angeordneten Landschaftsobjekten, welche den
  * Speicher fuer die eigentliche Matrix anlegt.
  ---------------------------------------------------------------------------*/
ZWEID_MATRIX_ATLAS::ZWEID_MATRIX_ATLAS(char *name, char *attr)
	 : MATRIX_ATLAS(name, attr)
{
  diagonal = attribut_gesetzt("Diagonal");
  breite = myatol(attribut("BREITE"));
  hoehe = myatol(attribut("HOEHE"));

  if (breite <= 0 || hoehe <= 0) {
    breite = 30;
    hoehe = 20;
    attribut_setzen("BREITE","30");
    attribut_setzen("HOEHE","20");
  }

  // Ich lege die Speicherstruktur spaltenweise an. Dadurch greift man auf
  // jeden Eintrag mit [x][y] zu.

  matrix = new unsigned short * [breite];

  for (long spalte = 0; spalte < breite; spalte++)
  {
    matrix[spalte] = new unsigned short [hoehe];
  }

  // Jetzt den Adressindex anlegen
  adrindex = new ADRINDEX(breite, hoehe);

  if (!attribut_gesetzt("++LADEN++"))  matrix_leeren();
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::~ZWEID_MATRIX_ATLAS()
  * 
  * Der Destruktor gibt den Speicher fuer die eigentliche Matrix frei.
  ---------------------------------------------------------------------------*/
ZWEID_MATRIX_ATLAS::~ZWEID_MATRIX_ATLAS()
{
  // Freigeben in umgekehrter Reihenfolge!
  if (matrix) {
    for (long spalte = 0; spalte < breite; spalte++) delete matrix[spalte];
    delete matrix;
  }
  delete adrindex;
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::speichern()
  * 
  * Speichert die Daten der 2D-Matrix ab. Dies besteht aus dem Speichern
  * der Matrix selbst und dem Weitergeben des Speichernaufrufes an die 
  * Basisklasse.
  ---------------------------------------------------------------------------*/
short ZWEID_MATRIX_ATLAS::speichern(FILE *file)
{
  if (!matrix) return 1;
  // Alles, was ich tun muss, ist das Abspeichern der Matrix. Ich speichere
  // sie so ab, dass sie moeglichst menschenlesbar ist, d.h. Zeile fuer
  // Zeile, von oben nach unten. Dies ist mir sogar einigen Aufwand wert.
  
  typeof(matrix[0][0]) *puffer = new typeof(matrix[0][0]) [breite];

  for (long zeile=hoehe-1; zeile>=0; zeile--) {
    for (long spalte=0; spalte<breite; spalte++)
        puffer[spalte] = matrix[spalte][zeile];
    fputhex(file, puffer, breite);
    if (ferror(file)) {
      delete puffer;
      return 1;
    }
  }

  delete puffer;

  // Jetzt soll noch meine Basisklasse ihre Sachen abspeichern
  return MATRIX_ATLAS::speichern(file);
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::laden()
  * 
  * Laedt meine Daten aus einem Spielstandfile. Im wesentlichen ist das
  * die Gelaendematrix. Am Ende lasse ich auch noch meine Basisklasse
  * ihr Zeugs laden.
  ---------------------------------------------------------------------------*/
short ZWEID_MATRIX_ATLAS::laden(FILE *file)
{
  // Ich habe die Daten etwas umstaendlich gespeichert (s.o.). Deshalb 
  // die Doppelschleife.
 
  typeof(matrix[0][0]) puffer;

  if (!matrix) return 1;
  for (long zeile=hoehe-1; zeile>=0; zeile--) {
    for (long spalte=0; spalte<breite; spalte++) {
       fgethex(file, &puffer, 1);
       matrix[spalte][zeile] = puffer;
    }
    if (ferror(file)) return 1;
  }

  // Und die Basisklasse hat auch noch was zu laden...

  return MATRIX_ATLAS::laden(file);
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::matrix_leeren()
  * 
  * Fuellt die Matrix mit lauter Nullen. Wird vom Konstruktor aufgerufen,
  * aber nur, wenn die Welt neu geschaffen wird. Nicht wenn nur geladen
  * wird.
  ---------------------------------------------------------------------------*/
void ZWEID_MATRIX_ATLAS::matrix_leeren()
{
  if (!matrix) return;
  for (long x=0; x<breite; x++)
    for (long y=0; y<hoehe; y++) matrix[x][y] = 0;
}


/**---------------------------------------------------------------------------
  * adress_fehler()
  * 
  * Lokale Hilfsfunktion, die eine Fehlermeldung fuer den Entwickler
  * ausgibt, wenn versucht wurde, eine Position ausserhalb der Matrix
  * mit feld() oder setze_feld() anzusprechen.
  ---------------------------------------------------------------------------*/
void adress_fehler(ADR& adr)
{
    log('I', "Fehlerhafte Koordinaten (%ld,%ld) in ZWEID_MATRIX_ATLAS",	adr.x, adr.y);
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::feld()
  * 
  * Liefert den Wert eines Matrixeintrages, falls die Adresse in Ordnung
  * ist.
  ---------------------------------------------------------------------------*/
unsigned short ZWEID_MATRIX_ATLAS::feld(ADR& adr)
{
  if (!matrix) return 0;
  if (adresse_ist_ausserhalb(adr)) { adress_fehler(adr); return 0; }
  else return matrix[adr.x][adr.y];
}

void ZWEID_MATRIX_ATLAS::setze_feld(ADR& adr, unsigned short feld)
{
  if (!matrix) return;
  if (adresse_ist_ausserhalb(adr)) { adress_fehler(adr); return; }
  else matrix[adr.x][adr.y] = feld;
}

short ZWEID_MATRIX_ATLAS::adresse_ist_ausserhalb(ADR& adr)
{
  return adr.x<0 || adr.y<0 || adr.x>=breite || adr.y>=hoehe;
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::alle_adressen_im_umkreis_von()
  * 
  * Legt eine Liste aller Adressen an, die von einem Mittelpunkt nicht
  * weiter enfernt sind als ein bestimmter Radius.
  ---------------------------------------------------------------------------*/
DOUBLIST *ZWEID_MATRIX_ATLAS::alle_adressen_im_umkreis_von
		(ADR& adr, float radius)
{
  DOUBLIST *feldliste = new DOUBLIST; // Ergebnisliste

  // Ich mache hier eine Fallunterscheidung, die ausschliesslich der
  // Effizienzsteigerung dieser durch die Parameteruebergabe ohnehin schon
  // recht exzessiven Funktion dient. Dazu fange ich einige kleine Radien-
  // bereiche gesondert ab. Zur Sicherheit vor Fehlern durch Rechenungenauig-
  // keiten setze ich die Grenze der Bereich immer etwas zu klein an. Wenn
  // naemlich als Radius Wurzel 2 angegeben wird, so ist mit Sicherheit davon
  // auszugehen, dass der Abstand Wurzel 2 eingeschlossen werden soll, auch
  // wenn der Wert leicht abgerundet ankommen sollte!
  
  if (radius < 0) return feldliste; // Ist eigentlich Fehler.

  short effekt = 0; // Anzahl der Felder aus der Tabelle
  
  if      (radius < 1.0)   effekt = 1;  // nur Mittelfeld
  else if (radius < 1.499) effekt = 5;  // + seitlich benachbarte Felder
  else if (radius < 1.999) effekt = 9;  // + diagonal benachbarte Felder
  else if (radius < 2.499) effekt = 13; // + zwei Felder seitlich
  else if (radius < 2.999) effekt = 21; // + Felder mit Roesselsprung
  
  // Wenn effekt gesetzt ist, dann hole ich die Felder aus einer Tabelle.
  // Ansonsten berechnet ich sie explizit.

  if (effekt) {
    static short
      x_o[21] = {0,0,1,0,-1,-1,-1,1,1,0,2,0,-2,-2,-2,-1,1,2,2,1,-1},
      y_o[21] = {0,1,0,-1,0,-1,1,1,-1,2,0,-2,0,-1,1,2,2,1,-1,-2,-2};
    
    for (int i=0; i<effekt; i++)
    {
      ADR neu(adr.x + x_o[i], adr.y + y_o[i]);
      wrap(&neu); // Eventuell Ueberlauf im ueber X=0.
      if (adresse_ist_ausserhalb(neu)) continue; // liegt ausserhalb
      ADR_LIST_NODE *neunode = new ADR_LIST_NODE(neu);
      feldliste->insert(neunode);
    }
    return feldliste;
  }

  // Hier kommt der explizite Fall, in dem ich einen anderen Trich anwende,
  // der mich davor bewart, alle moeglichen Adressen auszuprobieren.

  // Als erstes bestimme ich ein Quadrat, das so gross ist, dass sich
  // auf jeden Fall alle gesuchten Felder darin befinden. Die Kantenlaenge
  // ist dabei 2*radius+1, da von Mitte zu Mitte gerechnet wird und
  // deshalb auf beiden Seiten noch ein Feld hinzukommt.

  long links = adr.x - (long(radius)+1);
  long rechts = adr.x + (long(radius)+1);
  long oben = adr.y - (long(radius)+1);
  long unten = adr.y + (long(radius)+1);

  // Jetzt durchlaufe ich zwei geschachtelte
  // Schleifen, um das ganze Quadrat abzuscannen.


  for (long x=links; x<=rechts; x++)
  {
    for (long y=oben; y<=unten; y++)
    {
      ADR testadr(x,y);
      wrap(&testadr);
      // Jetzt bestimme ich den Abstand zum Mittelfeld und schaue, ob
      // er klein genug ist.
      if (entfernung_zwischen(adr, testadr) <= radius) // Ja.
      {
	// Ich kontrolliere aber noch, ob sich das Feld nicht ausser-
	// halb befindet. Dann gilt es naemlich nicht.
	if (!adresse_ist_ausserhalb(testadr)) // OK.
	{
	  ADR_LIST_NODE *node = new ADR_LIST_NODE;
	  node->adresse = testadr;
	  feldliste->insert(node);
	}
      } // Entfernung war klein genug
    } // y-Schleife
  } // x-Schleife

  return feldliste;
}


ADR& ZWEID_MATRIX_ATLAS::adresse_in_richtung(ADR& adr, RIC& ric)
{
  static ADR erg; // Achtung!
  erg.x = adr.x + ric.x;
  erg.y = adr.y + ric.y;
  wrap(&erg);
  return erg;
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::matrix_anzeigen()
  * 
  * Ausgabe zu Debug-Zwecken.
  ---------------------------------------------------------------------------*/
void ZWEID_MATRIX_ATLAS::matrix_anzeigen()
{
//   for (long y=hoehe-1; y>=0; y--) {
//     for (long x=0; x<breite; x++)
//     {
//       ADR adr(x,y);
//       if (y<23) io_printxy(x*2, 23-y, gelaendeform_attribut(adr, "ABK"));
//     }
//     cout << endl;
//   }
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::leeres_feld_aussenrum()
  * 
  * Waehlt zufaellig ein zu einem Feld benachbartes anderes Feld aus.
  * Dazu bestehen immer entweder 4 oder drei Moeglichkeiten, solange
  * die Matrix mindestens 3X2 Felder gross ist, was natuerlich bei
  * jeder Welt um Zehnerpotenzen uebertroffen wird.
  ---------------------------------------------------------------------------*/
short ZWEID_MATRIX_ATLAS::leeres_feld_aussenrum(ADR& pos, ADR& feld)
{
  // return 1..8: 's waren 1 bis 8 frei.   return 0: Alles schon voll

  // Es kommen vier Felder in Frage. Aus Effizienzgruenden verwende ich
  // hier nicht die Funktion alle_adressen_im_umkreis...

  const short anzahl = 4; // Wenn 8, dann diagonal auch moeglich!
  short xoffsets[] = { 0, 1,  0, -1, 1,  1, -1, -1 };
  short yoffsets[] = { 1, 0, -1,  0, 1, -1, -1,  1 };

  ADR nb[anzahl]; // nb steht fuer nicht-belegt
  short davon_frei = 0;
  for (int i=0; i<anzahl; i++) {
    ADR test(pos.x + xoffsets[i], pos.y + yoffsets[i]);
    wrap(&test);
    if (adresse_ist_ausserhalb(test)) continue;

    if (!gelaendeform_in_feld(test)) { // Ja.
      nb[davon_frei] = test;
      davon_frei++;
    }
  }

  // Jetzt weiss ich auch schon, wieviele Felder frei sind.

  if (!davon_frei) {
    feld = ADR::ausserhalb();
    return 0; // Keines frei.
  }

  // Ansonsten suche ich eines zufaellig aus und gebe dies zurueck.
  short auswahl = io_random(davon_frei);
  feld = nb[auswahl];
  return davon_frei;
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::irgendein_feld_aussenrum()
  * 
  * Geht von der Adresse aus um ein Feld in irgendeine von vier Himmels-
  * richtungen. An den Polen sind nur drei Richtungen moeglich.
  ---------------------------------------------------------------------------*/
void ZWEID_MATRIX_ATLAS::irgendein_feld_aussenrum(ADR& pos, ADR& feld)
{
  // ich darf nicht auf pos und feld gleichzeitig operieren, da diese unter
  // Umstaenden identisch sein koennen!

  ADR start = pos;

  if (adresse_ist_ausserhalb(pos)) { adress_fehler(pos); return; }
  
  // Es kommen vier Felder in Frage. Aus Effizienzgruenden verwende ich
  // hier nicht die Funktion alle_adressen_im_umkreis...

  static short xoffsets[] = { 0, 1,  0, -1};
  static short yoffsets[] = { 1, 0, -1,  0};

  // Ich waehle irgendeines aus. Wenn es zufaellig ausserhalb der Landschaft
  // war (sehr selten), dann versuche ich es einfach nochmal.
  
  do {
    short auswahl = io_random(4);
    feld.x = start.x + xoffsets[auswahl];
    feld.y = start.y + yoffsets[auswahl];
    wrap(&feld);
  } while (adresse_ist_ausserhalb(feld));
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::adrindex_neu_berechnen()
  * 
  * Traegt alle Objekte, die sich auf mir befinden, in das ADRINDEX
  * ein. Prueft nicht, ob der Index vorher leer ist.
  ---------------------------------------------------------------------------*/
void ZWEID_MATRIX_ATLAS::adrindex_berechnen()
{

  OBJEKT_LIST_NODE *objektnode;
  SCAN(&globale_objekt_menge, objektnode)
  {
    if (objektnode->objekt->ort() == this) {
      adrindex->enter_objekt(objektnode->objekt, objektnode->objekt->adresse);
    }
    NEXT(objektnode);
  }
}  

