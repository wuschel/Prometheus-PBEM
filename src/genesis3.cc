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
  * MODUL:               genesis3.C / GENESIS3.CPP
  * AUTOR/DATUM:         Mathias Kettner, 5. April 1994
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Dies ist ein Welterzeugungsalgorithmus, der alternativ zum Genesis II
//      ist und auf einem anderen Prinzip basiert. Die Funktionalitaet ist
//      aber voellig identisch.
//
//      Die fuer den Algorithmus noetigen Zusatzdaten ueber die Gelaendeformen
//      werden in gelaende.dat konfiguriert.
//
// *************************************************************************

#include <ctype.h>

#include "maxmin.h"
#include "landscha.h"
#include "kompatib.h"
#include "listmac.h"
#include "alg.h"
#include "resource.h"
#include "layout.h"
#include "drucker.h"
#include "log.h"
#include "html.h"
#include "uhr.h"
#include "staat.h"

extern UHR *g_uhr;

/**---------------------------------------------------------------------------
  * GLOBALE VARIABLE
  * Da der G-III Algorithmus an einem Stueck komplett ablauft und Prometheus
  * natuerlich ohnehin nicht multithreadfaehig ist, kann ich mir hier
  * einige globale Variable erlauben, die ja immerhin lokal in diesem Modul
  * sind.
  ---------------------------------------------------------------------------*/

DOUBLIST keimliste;
long *formenspeicher;
GELAENDE_FORM_TABELLE *gelaendetabelle;
short *formguete;

/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::genesis_teil_3()
  * 
  * Erzeugt eine Welt nach bestimmten Vorgaben, die aus den Attributen,
  * des Objektes bezogen werden. Genauergesagt: Es werden die Gelaende-
  * formen und die Startpunkte fuer die Spieler festgelegt. Die Matrix
  * wird im Konstruktor von ZWEID_MATRIX_ATLAS angelegt.
  ---------------------------------------------------------------------------*/
void ZWEID_MATRIX_ATLAS::genesis_teil_3()
{
   if (!matrix) return;  // Fehler. Keine Landschaftsmatrix!

   // Globale Variable fuer die Keim-Klasse, da diese keinen Zugriff auf
   // die Landschaft hat.
   
   gelaendetabelle = &gelaende_formen;
   formenspeicher = new long[gelaende_formen.get_anzahl()];

   // Der Algorithmus gliedert sich in drei Schritte.
   
   g3_keime_setzen();                   // Berechnet auch Anteile
   g3_gelaende_formen();                // Keimwachstum
   g3_diagonalen_entfernen();           // Nachbearbeitung

   // Aufraeumen sollte ich auch noch.
   keimliste.clear();
   delete formenspeicher;

   io_deleteline(22); // Zaehler im unteren Bildschirmbereich loeschen.
   
   if (attribut_gesetzt("G2_DEBUG")) {
     io_cls();
     matrix_anzeigen();
     io_getch();
   }
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::g3_keime_setzen()
  * 
  ---------------------------------------------------------------------------*/
void ZWEID_MATRIX_ATLAS::g3_keime_setzen()
{
  // Die Keimdichte gibt die Anzahl der Felder an, auf die im Durchschnitt
  // ein Keim kommt. Je kleiner die Keimdichte, desto abwechslungsreicher
  // ist die Landschaft.

  long keimdichte;
  if (!(keimdichte = myatol(attribut("G3_KEIMDICHTE")))) keimdichte = 32;
  
  long anzahl_felder = breite*hoehe;
  long anzahl_keime = anzahl_felder / keimdichte;
  
  // Nun muss ich die Keime auf die Gelaendeformen verteilen. Zuerst aber be-
  // rechne ich, wieviele Felder von jeder Gelaendeform vorhanden sein sollen.
  
  // Jede Gelaendeform
  // hat eine Angabe, wie haeufig sie anzutreffen ist. Ich summiere all diese
  // Werte zuerst, da sie noch nicht normiert sind. In einem zweiten Durchgang
  // kann ich dann die Anteile berechnen. Die Gelaendeform 0 wird dabei
  // niemals beachtet oder mitgezaehlt.

  long anzahl_formen = gelaende_formen.get_anzahl();
  
  long summe=0;
  int form;
  for (form=1; form<anzahl_formen; form++)
    summe += myatol(gelaendeform_attribut(form, "ANTEIL"));

  long rest=0; // Divisionsrest merken. Damit es aufgeht.
  for (form=1; form<anzahl_formen; form++) {
    long anteil = myatol(gelaendeform_attribut(form, "ANTEIL"));
    long anzahl = (anzahl_felder*anteil + rest) / summe;
    rest = (anzahl_felder*anteil + rest) % summe;
    formenspeicher[form] = anzahl;
  }

  // Nun sind alle Felder verteilt. Jetzt muss ich noch die Keime verteilen.
  // Sie werden nach dem gleichen Prinzip verteilt. Es gibt aber ein eigenes
  // Attribut fuer die Keimhaefigkeit und das heisst KEIMANTEIL.

  // Ganz wichtig ist noch folgendes: Damit der Algorithmus nicht blockiert,
  // muss jede Gelaendeform mit ANTEIL!=0 unbedingt einen Keim haben, es sei
  // den, die kommt im Gelaendeprofil einer anderen Form vor.
  
  for (form=1, summe=0; form<anzahl_formen; form++)
    summe += myatol(gelaendeform_attribut(form, "KEIMANTEIL"));

  for (form=1, rest=0; form<anzahl_formen; form++)
  {
    long anteil = myatol(gelaendeform_attribut(form, "KEIMANTEIL"));
    if (anteil) {
      short keimzahl = (anzahl_keime*anteil + rest) / summe;
      rest = (anzahl_keime*anteil + rest) % summe;
      if (keimzahl <= 0) keimzahl = 1;
      for (;keimzahl;keimzahl--) g3_keim_setzen(form);
    }
  }
  
  // Das war bis jetzt alles.    
}      


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::g3_keim_setzen()
  * 
  * Definiert einen neuen Keim durch das einfuegen in die Liste der
  * Keime. In die Landschaft wird er dann auch gleich reingebonkt.
  * @param
  * short form:     Gelaendeform des Keimes
  ---------------------------------------------------------------------------*/
void ZWEID_MATRIX_ATLAS::g3_keim_setzen(short form)
{
    if (form==0) {
	log('I', "ZWEID_MATRIX_ATLAS::g3_keim_setzen(0)");
	return;
    }
  
  ADR pos;

  // Neue Keime versuche ich zunaechst auf ein freies Feld zu setzen. Nach
  // einer gewissen Anzahl von Versuchen kommt er aber einfach irgendwo hin.

  long retry = 25;
  
  do {
    if (retry) retry--;
    
    long x,y;
    x = io_random(breite);  // Longitude haengt nicht vom Klima ab.

    // Die X-Koordinate ist zufaellig. Die Y-Koordinate muss in der richtigen
    // Klimazone sein. Aufschluss ueber die Verteilung gibt das Attribut
    // KLIMAPROFIL. Z.B. KLIMAPROFIL=0013100 in einer Welt mit sieben Klimazonen.
 
    char *kp = gelaendeform_attribut(form, "KLIMAPROFIL");
    short summe = stringsumme(kp);
    if (!summe) y = io_random(hoehe); // Klima ist egal
    else {
      short auswahl = io_random(summe);
      char *scan = kp;
      short zone = 0;
      while (auswahl >= 0) {
	if (0 > (auswahl -= *scan-'0')) break;
	scan++;
	zone++;
      }

      // Nun habe ich die richtige Zone. Innerhalb dieser Zone kann ich einen
      // beliebigen Y-Wert auswuerfeln. Ich streue noch um +-1 und kann da-
      // dadurch gleichzeitig io_random(0) Fehler bei einer Zonendicke < 1.0
      // abfangen.
      
      y = (zone*hoehe)/strlen(kp) + io_random(hoehe/strlen(kp)+4) - 2;
      if (y<0) y=0;
      if (y>=hoehe) y=hoehe-1;

    }
    
    pos.x = x;
    pos.y = y;

  // Ich sollte eigentlich noch einen Mindestabstand einhalten...
  // ...
  // ...
  
  } while (retry && gelaendeform_in_feld(pos)); // Dann isses schon besetzt.

  KEIM *keim = new KEIM(form, pos);
  keimliste.add_tail(keim); 

  // Dass der neue Keim an das Ende der Liste kommt, ist sehr wichtig, da
  // auch im Laufe des Wachstumgs Keime geloscht und versetzt kommen. Neue
  // Keime muessen unbedingt an das Ende der Liste, damit ein Keim,  der
  // prinzipiell nicht wachsen kann, den Algorithmus nicht blockiert.
}  

    
/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::stringsumme()
  * 
  * Summe der Ziffern des Strings. Wird fuer die Nomierung von Klima-
  * profilen wie "0012521000" benoetigt.
  ---------------------------------------------------------------------------*/
long ZWEID_MATRIX_ATLAS::stringsumme(char *string)
{
  if (!string) return 0;
  long summe = 0;
  while (*string) {
    if (isdigit(*string)) summe += *string-'0';
    string++;
  }
  return summe;
}


/**---------------------------------------------------------------------------
  * g3_formenspeicherbetrag()
  * 
  * Summiert die Betraege alle Eintraege im Formenspeicher und gibt so-
  * mit Auskunft darueber, wieviele Felder (noch) falsch belegt sind.
  ---------------------------------------------------------------------------*/
long g3_formenspeicherbetrag()
{
  long rwert=0;
  for (int i=1; i<gelaendetabelle->get_anzahl(); i++)
     rwert += ABS(formenspeicher[i]);
  return rwert;
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::g3_gelaende_formen()
  * 
  * Hauptalgorithmus, welcher die Keime wuchern laesst. Dabei koennen die
  * Keime auch wandern und sogar versetzt werden. Gegen Ende des Wachs-
  * tums wird eine kleiner Unteralgorithmus aufgerufen, der alle bis 
  * dahin noch weissen Felder mit den Formen von Nachbarfeldern fuellt,
  * da sich der Algorithmus bei den letzten wenigen Prozent sehr schwer
  * tut.
  ---------------------------------------------------------------------------*/
void ZWEID_MATRIX_ATLAS::g3_gelaende_formen()
{
  // Zuerst zaehle ich alle noch leeren Felder, damit ich nachher immer
  // schnell weiss, ob ich schon fertig bin.
  
  long noch_leer = breite*hoehe;
  for (int i=0; i<breite; i++) {
    for (int j=0; j<hoehe; j++) {
      ADR pos(i,j);
      if (gelaendeform_in_feld(pos)) noch_leer--;
    }
  }

  // Bedeutung der folgenden Variablen:
  // last: Letzter Wert von noch_leer, um Veraenderungen zu bemerken.
  // retry: Retry-Counter, der eine Blockierung des Algorithmus erkennt.
  // last_betrag: Letzter Wert von betrag, um Veraenderungen zu erkennen.
  // kleinster_betrag: Bisher kleinster Wert von betrag, um Verbesserungen
  //    zu erkennen.
  
  long last=0, retry=0, last_betrag=0, kleinster_betrag=breite*hoehe;

  while (!keimliste.is_empty())
  {
    // Wenn der retry-Zaehler schon auf einen sehr hohen Wert gewachsen ist,
    // ohne dass etwas weitergeht, dann naehere ich mich der Endphase, und
    // ich fuelle den Rest, damit der Algorithmus nicht mit weissen Flaechen
    // terminiert! Wenn schon alle Felder belegt sind, dann natuerlich nicht.

    if (noch_leer>0 && retry>9900) {
      g3_rest_fuellen(noch_leer);
      noch_leer = 0;  // Da brauche ich garnicht lang zu zaehlen.
      retry = 0; // Nocheinmal komplett druebergehen.
    }
    
    // Der Betrag des formenspeicher-Vektors gibt an, wie gut das Ergebnis
    // bereits ist. Ich beende regulaer, wenn alle Felder belegt sind und
    // die Anzahl der Fehler unter ein ertraegliches Mass gesunken ist
    // (etwa 1% der gesamten Felder)

    long betrag = g3_formenspeicherbetrag();
    if (betrag <= breite*hoehe / 80 && noch_leer == 0) break;


    // Folgendes Ausgabe und Abbruchsanweiseung fuehre ich nur aus,
    // wenn sich ueberhaupt etwas getan hat.
	
    if (last != noch_leer || last_betrag != betrag) {
      
      // Der Retry-Counter wird nur zurueckgesetzt, wenn die Fehlerzahl
      // einen neuen kleinsten Wert angenommen hat.
    
      if (betrag < kleinster_betrag) {
	retry = 0;
	kleinster_betrag = betrag;
      }

      // Jetzt gebe ich noch ein paar laufende Zahlen aus, damit dem 
      // Zuseher nicht langweilig wird und er merkt, dass das Programm noch
      // nicht die Beine hochgenommen hat.

      char numstring[10];
      sprintf(numstring,"%7ld",noch_leer);
      io_printxy(15,22,numstring);
      sprintf(numstring,"%7ld",betrag);
      io_printxy(37,22,numstring);
    }

    else { // Es hat sich nichts getan.
    
      if (retry++ % 37 == 0) // Nur jedes 37.mal wird er ausgegeben.
      { 
	char numstring[10];
	sprintf(numstring,"%7ld\n",10000 - retry); // Zaehlt abwaerts
	io_printxy(62,22,numstring);
      }
      if (retry >= 10000) return; // Algorithmus blockiert.
    }
    
    // Hier merke ich mir die alten Werte
    
    last = noch_leer;
    last_betrag = betrag;

    // Nun kann es endlich mit der eigentlichen Arbeit losgehen.
    // Ich hole den ersten Keim aus der Liste und widme mich ihm.

    KEIM *keim;
    FIRST(&keimliste, keim);

    // Der Keim kommt nur dran, wenn noch genuegend Kontingent fuer diese
    // Gelaendeformart vorhanden ist. Ansonsten muss er ganz nach hinten.
    
    if (formenspeicher[keim->form] <= 0) {
      keim->remove();
      keimliste.add_tail(keim);
      continue;
    }
    
    // Der Keim muss ausserdem nach hinten, wenn er schon einmal versagt hat,
    // d.h. er konnte vergangenes mal nicht wachsen

    if (retry) {
      keim->remove();
      keimliste.add_tail(keim);  // Noch aendern
    }
    
    short urform = gelaendeform_in_feld(keim->adresse);
    
    // Wenn es eine Form mit dem Attribut "Einzeln" ist, dann setze ich einfach
    // und mache sonst nichts.

    if (gelaende_formen.attribut_gesetzt_fuer_form(keim->form, "Einzeln")) {
      if (urform) formenspeicher[urform]++;
      else noch_leer--;        
      gelaendeform_aendern(keim->adresse, keim->form);
      formenspeicher[keim->form]--;
    }
    
    // Sonst lasse ich den Keim normal wachsen. 
    else noch_leer -= g3_keim_waechst(keim);
 
  } // While noch_leer && noch Keim da.
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::g3_rest_fuellen()
  * 
  * Fuellt die letzten Luecken in der Matrix mit irgendwelchen nachbar-
  * werten.
  ---------------------------------------------------------------------------*/
void ZWEID_MATRIX_ATLAS::g3_rest_fuellen(long noch_leer)
{
  // Ich gehe nun alle Felder der Welt durch und fuelle weisse Felder
  // auf, indem ich eine Form aus irgendeinem seitlich benachbarten Feld
  // kopiere. Damit so nicht eine Gelaendeform ueber weiter Strecken "wandern"
  // kann, scanne ich die Felder nicht der Reihe noch, sondern in Zweierschritten
  // gewissermassen Schachbrettartig.

  long virtbreite = breite + (1+breite)%2;
  while (noch_leer) {
    for (int x=0; x<virtbreite; x++)
    {
      for (int y=0; y<hoehe; y++)
      {
	// Ich scanne die Welt diagonal in Zweierschritten, damit ich nicht 
	// lange Ketten von Leerfeldern mit der gleichen Form fuelle.

	ADR pos(((2*x % virtbreite)+y) % breite, y);
	if (!gelaendeform_in_feld(pos)) {
	  ADR feld;
	  irgendein_feld_aussenrum(pos, feld);
	  short form = gelaendeform_in_feld(feld);
	  if (form)
	  {
	    gelaendeform_aendern(pos, form);
	    formenspeicher[form]--;
	    noch_leer--;

	    char numstring[10];
	    sprintf(numstring,">%6ld",noch_leer);
	    io_printxy(15,22,numstring);
	    sprintf(numstring,">%6ld",g3_formenspeicherbetrag());
	    io_printxy(37,22,numstring);
	    
	    if (!noch_leer) return; // Muesste eigentlich nicht noetig sein.
	  }
	} // Feld war leer
      } // y
    } // x
  } // noch_leer
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::g3_keim_waechst()
  * 
  * Stellt den eigentlichen Keimwachstumsprozess dar.
  * @param
  * KEIM *keim: Enthaelt Adresse und Keimform des wachsenden
  * Keimes.
  * @return
  * short Anzahl der neu eingefaerbten Felder. Kann auch negativ sein,
  * was bedeutet, dass wieder Felder geloescht wurden. In den meisten
  * Faellen ist das Ergebnis 1 oder 0.
  ---------------------------------------------------------------------------*/
short ZWEID_MATRIX_ATLAS::g3_keim_waechst(KEIM *keim)
{
  short maxversuche = 50, rwert=0;
  ADR ziel = keim->adresse; // Ausgangspunkt
    
  while (maxversuche--)
  {
    // Ich hole mir ich beliebiges Nachbarfeld.
    irgendein_feld_aussenrum(ziel, ziel);

    short form = keim->naechste_form();   // Gelaendeprofil weiterschalten
   
    // Die Klimavertraeglichkeit muss ich pruefen. Bei Unvertraeglichkeit
    // muss ich abbrechen.

    float vertr = g3_klimazugehoerigkeit(ziel.y, form, 3);
    
    // vertr = 1: Durchschnitt. Wenn < 1, dann wuerfle ich drauf.
    
    if (vertr < 1 && io_random(1000) > vertr*1000) return rwert;

    // Ist die Form dort gleich der meinen?
    // Muss noch erweitert werden!
    
    short form_dort = gelaendeform_in_feld(ziel);
    if (form_dort == form) // Ja.
    {
      keim->adresse = ziel; // Keim wandert.
      continue;
    }
    
    // Nun muss ich schauen, ob die Verbindung zu den neuen Nachbarn ver-
    // traeglich waere.
      
    if (!g3_nachbarn_vertraeglich(form, ziel)) { // Nein.
	
      short urform;
      // Alle Felder im Umkreis von 1.5 werden wieder auf 0 gesetzt.
      FOR_EACH_ADR_IN (alle_adressen_im_umkreis_von(ziel, 1.5))
      DO_AND_DELETE
      (
	urform = gelaendeform_in_feld(adr);
	rwert =  ( (urform!=0) ? -1 : 0);
	formenspeicher[urform] ++;
	gelaendeform_aendern(adresse, 0);
      )
      return rwert;
	
    }
      
    // Die Nachbarn machen also kein Problem.
    short urform = gelaendeform_in_feld(ziel);
      
    if (urform) // schon besetzt. Wenn ich im Klimatest gewinne, dann
    {           // darf ich uebernehmen.
      if (!g3_klimatest(ziel.y, form, urform)) return 0;
      formenspeicher[urform] ++;
      rwert--;
    }          
      
    gelaendeform_aendern(ziel, form);
    if (io_random(3)) keim->adresse = ziel;  // Keim wandert enventuell
    formenspeicher[form]--;
    return rwert + 1;
    
  } // Weitermachen bei der Wanderung.
      
  // Es konnte nichts umgefaerbt werden. Der Keim ist hier wahrscheinlich
  // nutzlos. Deshalb wird der Keim an eine neue Position gebracht.
  
  g3_keim_setzen(keim->form); // Neuen Keim erzeugen.
  delete keim; // Wird aus globaler Liste entfernt
  
  return rwert; // Nichts eingefaerbt.
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::g3_klimatest()
  * 
  * Prueft, ob die Form 'neuform' besser zum Klima der Breite y passt.
  * Ausserdem gibt noch die Haufigkeitsverteilung Ausschlag. Anschlie-
  * ssend wird ein Zufallswurf auf beide Werte gemacht.
  ---------------------------------------------------------------------------*/
short ZWEID_MATRIX_ATLAS::g3_klimatest(long y, short neuform, short altform)
{
  // Ich muss von beiden Gelaendeformen feststellen, wie gut sie in das hiesige
  // Klima passen. Dazu hole ich mir deren Klimaprofile und berechne daraus die
  // Vergleichswerte. Dazu multipliziere ich noch die Anzahl der ausstehenden
  // Feldern fuer die Form, damit Formen, die noch oft drankommen sollen, be-
  // vorzugt werden.
  
  float vwert_neu = g3_klimazugehoerigkeit(y, neuform, 2) * formenspeicher[neuform];
  float vwert_alt = g3_klimazugehoerigkeit(y, altform, 2) * formenspeicher[altform];
  
  long summe = (long)(10 * vwert_neu + 10 * vwert_alt); // Mal 10 wegen kleinen Werten.
  long wurf = summe ? io_random(summe) : 0;
  return (wurf <= vwert_neu * 10);
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::g3_klimazugehoerigkeit()
  * 
  * Stellt fest, wie sehr ein bestimmter Breitengrad klimatisch zu einer
  * Gelaendeform passt. Dazu wird das KLIMAPROFIL zurate gezogen, dessen
  * Summe auf 1 normiert und der richtige Eintrag ermittelt. Das Ergebnis
  * ist eins, wenn eine fuer diese Form durchschnittliche Zugehoerigkeit
  * berechnet wird, 0, wenn sie hier garnicht vorkommt, < 1, wenn
  * sie hier selten auftritt und > 1, wenn sie gehaeuft auftritt.
  *
  * @param
  * long y:		Y-Koordinate, dessen Klima geprueft wird
  * short form:	Gelaendeform
  * long dy:	+/- Dieser Wert wird zu y addiert, damit nocheinmal
  * die Zugehoerigkeit ermittelt und halbiert addiert.
  * Dadurch werden die Profile etwas 'abgerundet'. Je
  * groesser dy ist, desto weiter entfernt von der
  * eigentlichen Zone darf die Form liegen.
  ---------------------------------------------------------------------------*/
float ZWEID_MATRIX_ATLAS::g3_klimazugehoerigkeit(long y, short form, long dy)
{
  float rwert;
  char *kp = gelaendeform_attribut(form, "KLIMAPROFIL");
  short laenge = mystrlen(kp);   // Fuer Zonennormierung.
  if (!laenge) return 0.0; // Fehler.
  
  // Der Durchschnittswert fuer diese Form ist stringsumme / laenge.
  // Ich betrachte den Wert an der fraglichen Y-Stelle im Klimaprofil
  // gemessen an diesem Durchschnitt. Durchschnittlich ist also 1.

  rwert = (float(kp[y*laenge / hoehe] - '0') * laenge)
  	  / stringsumme(kp);

  if (dy) {
    if (y > dy)
      rwert = (rwert*3 + g3_klimazugehoerigkeit(y-dy, form))/4.0;
    if (y < hoehe-dy)
      rwert = (rwert*4 + g3_klimazugehoerigkeit(y-dy, form))/5.0;
  }
  return rwert;
}
  

/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::g3_nachbarn_vertraeglich()
  * 
  ---------------------------------------------------------------------------*/
short ZWEID_MATRIX_ATLAS::g3_nachbarn_vertraeglich(short, ADR&)
{ 
  return 1;
}


/**---------------------------------------------------------------------------
  * KEIM::KEIM()
  * 
  ---------------------------------------------------------------------------*/
KEIM::KEIM(short f, ADR& a)
{
  form = f;
  adresse = a;
  profil_zeiger = gelaendetabelle->attribut_fuer_form(form, "GELAENDEPROFIL");
  profil_zaehler = 0;
}


/**---------------------------------------------------------------------------
  * KEIM::naechste_form()
  * 
  * Holt die naechste Form aus dem Gelaendeprofil der urspruenglichen
  * Form des Keimes, falls eines vorhanden. Falls nicht, oder dieses
  * einen Fehler aufweist, wird immer die urspruengliche Form zurueck-
  * gegeben. Ein Keim der als Urform nicht 0 hat, wie mit dieser Funktion
  * niemals eine ungueltige Gelaendeform oder 0 zurueckgeben.
  ---------------------------------------------------------------------------*/
short KEIM::naechste_form()
{
  // GELAENDEPROFIL=5Gb2Bg1Hu*Hu.Hu

  if (!profil_zeiger) return form; // Kein Gelaendeprofil vorhanden.
  
  char token = *profil_zeiger;
  if (isdigit(token)) {
    profil_zaehler++;
    if (profil_zaehler <= token-'0') return abk_form(profil_zeiger + 1);
    else {
      profil_weiterrutschen();
      return naechste_form();  // Rekursion. Jawohl!
    }
  }
  else switch(token) {
  case 0:   // String zuende.
    profil_weiterrutschen();
    if (!*profil_zeiger) return form; // Fehler
    else return naechste_form();
    
  case '*': // Beliebig oft-Token. Bei 50% wird weitergerutscht
    if (io_random(2)) {
       profil_weiterrutschen();
       return naechste_form();
     }
     else return abk_form(profil_zeiger + 1);
     
   case '.': // Unendlich oft-Token.
     return abk_form(profil_zeiger + 1);

   case '<': // Optionales Ende-Token.
     if (io_random(2)) profil_zuruecksetzen();
     else profil_zeiger++;
     return naechste_form();
     
   } // switch

  return form; // Fehlerhaftes Gelaendeprofil
}

 
/**---------------------------------------------------------------------------
  * KEIM::abk_form()
  * 
  ---------------------------------------------------------------------------*/
short KEIM::abk_form(char *s)
{
  char abk[3];
  abk[0]=s[0];
  abk[1]=s[1];
  abk[2]=0;
  short rwert = gelaendetabelle->form_mit_abkuerzung(abk);
  return (rwert ? rwert : form);
}


/**---------------------------------------------------------------------------
  * KEIM::profil_weiterrutschen()
  * 
  ---------------------------------------------------------------------------*/
void KEIM::profil_weiterrutschen()
{
  if (mystrlen(profil_zeiger) < 4) profil_zuruecksetzen();
  else {
    profil_zeiger+=3;
    profil_zaehler=0;
  }
}
 

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void KEIM::profil_zuruecksetzen()
{
  profil_zeiger = gelaendetabelle->attribut_fuer_form(form, "GELAENDEPROFIL");
  profil_zaehler = 0;
}
  

/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::g3_weltkarte_ausdrucken()
  * 
  * Druckt eine komplette Weltkarte mit richtigen Gelaendeformen,
  * damit man sieht, wo man die Startpunkte setzen kann.
  ---------------------------------------------------------------------------*/
void ZWEID_MATRIX_ATLAS::g3_weltkarte_ausdrucken(char *filename)
{
    verzeichnis_gewaehrleisten(filename);
    if (drucker_init(filename)) {
      log('W', filename ? "Can't open printerfile '%s'" : "Can't open printer", filename);
      return;
  }
  dr_anfang(false);

  #define LINKS 2 // cm linker Rand zum heften
  
  // Aufteilung auf mehrere Blaetter.
  int xblaetter = (breite + 39) / 40;
  int yblaetter = (hoehe + 54) / 55;
  
  for (int xblatt = 0; xblatt < xblaetter; xblatt++)
  {
    for (int yblatt = 0; yblatt < yblaetter; yblatt++)
    {
      if (xblatt || yblatt) dr_neueseite();  // Naechste Seite beginnen.
      
      LAYOUT titel(dr_abschnitt(1.8));
      char string[200];
      sprintf(string,L("Partie %s,  Weltkarte  %ld X %ld,  Seite %d von %d","Game %s,  World Map %ld X %ld,  Page %d of %d"),
        objekt_mit_namen("Uhr") -> info("SESSIONNAME"), breite, hoehe,
        xblatt * yblaetter + yblatt + 1, xblaetter*yblaetter);
      titel.text(LINKS, 0.3, string, "Times", 15);
      titel.ausdrucken();

      int xstart = xblatt * 40;
      int ystart = yblatt * 55;
      int xmax = MIN(breite - xstart, 40);
      int ymax = MIN(hoehe - ystart, 55);
      
      // Jetzt drucke ich ein Blatt aus.
      for (int x=0; x < xmax; x++)
      {
        LAYOUT ly(dr_abschnitt(0));
        
        if (x%5 == 0) 
        { 
          ly.text(x*0.4 + LINKS + 0.1, ymax*0.4 + 1, myltoa(x+xstart),"Times",9);
          ly.linie(x*0.4 + LINKS, 0.5, x*0.4 + LINKS, ymax*0.4+0.5);
        }          
          
        for (int y=0; y < ymax; y++)
        {
	  short ykoord = hoehe - (y + ystart) - 1; // Wahre Koordinaten
          if (x==0 && ykoord%5==0) // Koordinaten
          {
            ly.text(LINKS-0.6, 0.4*y + 0.8, myltoa(ykoord), "Times",9);
            ly.linie(LINKS, (y+1)*0.4 + 0.5, xmax*0.4 + LINKS, (y+1)*0.4 + 0.5);
          }
          
          ADR adr(x + xstart, ykoord);
          if (mystrcmp(gelaendeform_attribut(adr, "ART"), "See")) {
            char *gform = gelaendeform_attribut(adr, "GRAFIK");
            ly.bitmatrix(x*0.4 + LINKS , y*0.4 + 0.5, gform);
          }
        }

        ly.ausdrucken();
      }

    }
  }
  dr_auswurf();
  drucker_close();
}  


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::g3_weltkarte_ausdrucken_html()
  * 
  * Druckt eine komplette Weltkarte mit richtigen Gelaendeformen,
  * damit man sieht, wo man die Startpunkte setzen kann. Als HTML
  ---------------------------------------------------------------------------*/

void ZWEID_MATRIX_ATLAS::g3_weltkarte_ausdrucken_html()
{
    const int seitenbreite=12, seitenhoehe=8, ueberlappung = 4;
    int teilx=0, teily=0;
    
    for (int xanfang = 0; xanfang < breite; xanfang += seitenbreite)
    {
	teilx ++;
	teily = (hoehe+seitenhoehe-1) / seitenhoehe + 1;

	for (int yende = hoehe-1; yende >= 0; yende -= seitenhoehe)
	{
	    teily --;
	    char titel[512];
	    sprintf(titel, L("Weltkarte Partie %s, Gr~o~se %ld X %ld, Teil %d/%d","World Map Game %s, Size %ld X %ld, Clipping %d/%d"),
		    g_uhr->info("SESSIONNAME"), breite, hoehe, teilx, teily);

	    char variabel[8];
	    sprintf(variabel, "%02d%02d", teilx, teily);
	    char *filename = g_uhr->htmlmapfilename(variabel);
	    HTML html(filename, titel);

	    html.ueberschrifts_balken(titel);
	    
	    html.set_cell_spacing(1)
		.set_cell_padding(0)
		.set_table_border(0)
		.table();
	    
	    int y1 = min(hoehe-1, yende + ueberlappung),
		y0 = max(0, yende-seitenhoehe-ueberlappung+1);
	    for (int y=y1; y >= y0; y--)
	    {
		html.next_row();
		int x0 = max(xanfang - ueberlappung, 0),
		    x1 = min(xanfang + seitenbreite + ueberlappung , breite);

		for (int x=x0; x < x1; x++)
		{
		    html.next_cell();
		    char image[128], tooltip[128];
		    ADR adr(x,y);
		    OBJEKT *stadt = stadt_bei_adresse(adr);
		    if (stadt) {
			sprintf(image, "k/e%s%s", stadt->attribut("GRAFIK"), g_uhr->attribut("GFORMAT"));
			sprintf(tooltip, "%d,%d: %s (%s %s)", x, y, stadt->staat()->name,
				stadt->staat()->attribut("VORNAME"),
				stadt->staat()->attribut("NACHNAME"));
		    }
			
		    else {
			char *gform = gelaendeform_attribut(adr, "GRAFIK");
			sprintf(tooltip, "%d,%d", x,y);
			sprintf(image,"k/%s", gform);
			if (!strcmp(image + strlen(image) - 4, ".gra"))
			{
			    image[strlen(image)-4] = 0;
			    strcat(image, ".gif");
			}
		    }
		    html.image(image, tooltip);
		}
	    }
	    html.end_table();
	}
    }
}


/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::stadt_bei_adresse()
  *
  * Schaut, ob an einem Ort ein Stadt ist und gibt einen Zeiger auf sie,
  * wenn ja.
  ---------------------------------------------------------------------------*/
OBJEKT *ZWEID_MATRIX_ATLAS::stadt_bei_adresse(ADR& adr)
{
    DOUBLIST *staedte = alle_objekte_bei_adresse(adr, "TYP=STADT");
    OBJEKT *antwort;
    if (staedte->is_empty()) antwort = 0;
    else antwort = ((OBJEKT_LIST_NODE *)staedte->first())->objekt;
    delete staedte;
    return antwort;
}



/**---------------------------------------------------------------------------
  * ZWEID_MATRIX_ATLAS::g3_diagonalen_entfernen()
  * 
  ---------------------------------------------------------------------------*/
void ZWEID_MATRIX_ATLAS::g3_diagonalen_entfernen()
{
  // Ich muss nun die ganze Landschaft absuchen nach Felderkombinationen
  // wie diesen beiden hier: ~~ Wi    Tg ~~
  //                         Sp ~~    ~~ Bg
  // Es darf keine Land- und Wasserwege geben, die sich kreuzen.
  
  // Ich gehe Zeilenweise vor. Die letzte Zeile lasse ich aus, da ich 
  // auf Viererbloecken operiere.
  
  for (int zeile=0; zeile<hoehe-1; zeile++)
  {
    for (int spalte=0; spalte<=breite; spalte++)
    {
      // Jetzt muss ich die vier interessanten Felder berechnen.
      short modus = g3_ist_meer(spalte, zeile);
      if (modus == g3_ist_meer(spalte, zeile+1) ||
	  modus == g3_ist_meer(spalte+1, zeile)) continue;
      if (modus != g3_ist_meer(spalte+1, zeile+1)) continue;
      
      // Jetzt tausche ich zwei Felder. Und zwar die zeile+1 Felder.
      ADR pos1(spalte+1, zeile);   wrap(&pos1);
      ADR pos2(spalte+1, zeile+1); wrap(&pos2);
      short form = gelaendeform_in_feld(pos1);
      gelaendeform_aendern(pos1, gelaendeform_in_feld(pos2));
      gelaendeform_aendern(pos2, form);
    }
  }
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short ZWEID_MATRIX_ATLAS::g3_ist_meer(int x, int y)
{
  ADR pos(x,y);
  wrap(&pos);
  return !mystrcmp(gelaendeform_attribut(pos,"ART"),"See");
}
