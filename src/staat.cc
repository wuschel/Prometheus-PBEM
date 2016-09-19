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
  * MODUL:               staat.C / STAAT.CPP
  * AUTOR/DATUM:         Mathias Kettner, 14. Mai 1993
  * KOMPATIBILITAET:     C++
  -----------------------------------------------------------------------------
  *
  *
  *      Enthaelt Funktionen zum Objekttyp STAAT.
  *
  *
  ---------------------------------------------------------------------------*/

#include <string.h>

#include "staat.h"
#include "stadt.h"
#include "enzyklop.h"
#include "uhr.h"
#include "einheit.h"
#include "alg.h"
#include "prom.h"
#include "laengen.h"
#include "drucker.h"
#include "einfluss.h"
#include "kompatib.h"
#include "layout.h"
#include "listmac.h"
#include "statisti.h"
#include "dauer.h"
#include "version.h"
#include "log.h"

extern DOUBLIST globale_objekt_menge;
extern EINFLUSS_LISTE globale_einfluss_menge;

extern short laser; // Ausdruck auf dem Laser oder lieber nicht?
extern bool printout_duplex; // main.cc

extern ENZYKLOPAEDIE *g_enzyklopaedie;  // von main.cpp
extern UHR           *g_uhr;		// von main.cpp

/**---------------------------------------------------------------------------
  * STAAT::STAAT()                 // constructor
  ---------------------------------------------------------------------------*/
STAAT::STAAT(char *name, char *attr) : OBJEKT(name, attr)
{
  neu_moeglich = NULL; // Liste der ab dieser Runde neu moeglichen Entwicklungen
  moegliche_entwicklungen = NULL; // Wird Zu Beginn jeder Runde berechnet

  // Wenn nur geladen wurde, dann bin ich schon fertig.

  if (attribut_gesetzt("++LADEN++")) return;

  // Die nachfolgenden Initialisierungen sind nur beim ersten Erschaffen
  // noetig.

  gesamtwertung_alt = gesamtwertung = 0; // Fuer die Punktewertung
  anzahl_angriffe = 0; // Fuer die Statistik

  attribut_setzen("STAATSFORM",L("Despotismus","Despotism")); // Damit faengt jeder an.
  spielerinformation("despotis.inf");   // Info in den Ergebnisausdruck

  // Jeder Staat unterhaelt bei der Welt ein subjektives Abbild der Land-
  // schaft. Dieses wird unser Staat nun anfordern. Die weitere Verwaltung
  // uebernimmt dann die Welt. An dieser Stelle muss die Welt deshalb schon
  // existieren.

  if (!welt()) {
      log('I', "STAAT::STAAT: Dem Staat %s fehlt die Welt", name);
      return;
  }
  else welt()->landschaftsabbild_anlegen(this);

  // Von der Welt hole ich mir ein huebsches Fleckchen, wo ich starten kann.
  // Das geht mit einem info. Die Welt hat nur ein
  // begrenztes Kontingent von Startpunkten, die beim Genesis II - Algorith-
  // mus verteilt werden.

  ADR start;
  start.x = myatol(attribut("START_X"));
  start.y = myatol(attribut("START_Y"));

  // Der Mittelpunkt des Reiches ist gleich dem Startpunkt. Der Mittel-
  // punkt ist entscheidend fuer die Angabe der relativen Koordinaten.
  // Jeder Spieler startet naemlich bei den subjektiven Koordinaten (0,0).
  // Welcher Adresse dass in Wirklichkeit entspricht, steht in der
  // Variablen mittelpunkt.

  mittelpunkt = start;

  // Jetzt berechne ich die Starteinfluesse, die sich aus den Entwick-
  // lungen und der Staatsform ergeben.
  
  kommando_einfluesse_berechnen();

  // So. Jetzt gruende ich die Hauptstadt! Dazu stelle ich die Attribute
  // zusammen

  ATTRIBUT_LISTE attrib;
  attrib.setzen("GATTUNG",L("Hauptstadt","Capital City"));
  attrib.setzen("GRAFIK","stadtx");

  // Sowohl beim Staat, als auch bei den Staedte existiert eine direkte
  // Referenz auf die Welt durch deren Namen, naemlich im Attribut
  // WELTNAME.

  attrib.setzen("WELTNAME",attribut("WELTNAME"));

  // Auch muss die Stadt bei der Erschaffung ihre eigene Startposition
  // bereits kennen. Sie muss naemlich bei der Landschaft ihr Feld
  // als bewirtschaftet anmelden.

  attrib.setzen("STARTX",myltoa(start.x));
  attrib.setzen("STARTY",myltoa(start.y));

  // Die Hauptstadt beginnt mit 30000 Einwohnern, wohingegen jede andere Stadt
  // mit weniger Einwohnern beginnt.

  attrib.setzen("EINWOHNERZAHL","30000");

  // Nun braucht die Hauptstadt auch eine Abkuerzung, die mit HS anfaengt.

  char *n = eindeutige_objekt_abkuerzung(L("HS","CP"),3);
  if (!n) {
      log('I', "Name der Hauptstadt '%s' ist nicht  eindeutig abkuerzbar",
	  attribut("HAUPTSTADT"));
    zur_vernichtung_vormerken();
    return;
  }
  
  //  attrib.setzen("NAME",attribut("HAUPTSTADT"));
  attribut_setzen("HAUPTS_ABK",n); // Damit kann ich meine HS bestimmen

  // Und nun kann ich die Stadt endlich schaffen.

  OBJEKT *stadt = objekt_schaffen(n, "STADT", attrib.to_string());
  if (!stadt) {
    attribut_setzen("FEHLER",L("Ich kann die Hauptstadt nicht gruenden!","I cannot found the Capital City!"));
    zur_vernichtung_vormerken();
    return;
  }

  // Hier muss ich noch die Starteinheiten schaffen. Neuerdings sind die
  // Starteinheiten aber nicht mehr variabel, sondern im Programm fest-
  // gelegt. 

  const short anzahl = 6;
  static char *gattungen[] = { L("GATTUNG=Siedler","GATTUNG=Settlers"),
			       L("GATTUNG=Bautrupp","GATTUNG=Construction_Worker_Gang"),
			       L("GATTUNG=Bautrupp","GATTUNG=Construction_Worker_Gang"),
			       L("GATTUNG=Wilde","GATTUNG=Natives"),
			       L("GATTUNG=Wilde","GATTUNG=Natives"),
			       L("GATTUNG=Landwehr","GATTUNG=National_Guards")};

  for (int k=0; k<anzahl; k++) {
    spieler_bekommt_info_ueber(gattungen[k]+8);
    OBJEKT *einheit 
	  = objekt_schaffen(eindeutiger_name(), "EINHEIT", gattungen[k]);

    // Jetzt muss die Einheit noch an den richtigen Ort gebracht werden und
    // der Stadt uebereignet werden

    einheit->ort_wechseln(welt(), start);
    geben(stadt, einheit);

    // Hier kommt noch ein kleiner Bonus ins Spiel: Die Starteinheiten
    // kosten keinen Unterhalt! Sonst hat es der Spieler schon sehr
    // schwer am Anfang, wenn er mit einer Stadt fuer vier Einheiten
    // aufkommen muss!

    einheit->attribut_loeschen("VERSORGUNG");

  }

  // Der Default-Anzeigemodus fuer die Einheiten wird noch gesetzt.
  attribut_setzen("AMODUS","IGS");

}


/**---------------------------------------------------------------------------
  * STAAT::~STAAT()                // destructor
  ---------------------------------------------------------------------------*/
STAAT::~STAAT()
{
  // Ich muss alle Landschaftsabbilder vernichten und meinen Startpunkt
  // zurueckgeben.

  OBJEKT_LIST_NODE *objnode;
  objnode = (OBJEKT_LIST_NODE *)globale_objekt_menge.first();

  char *weltname = attribut("WELTNAME");
  if (weltname) {
      WELT *welt = (WELT *)objekt_mit_namen(attribut("WELTNAME"));
      if (welt) { // sonst ist es eigentlich ein Fehler.
	  welt->landschaftsabbild_vernichten(this);
	  if (!mittelpunkt.ist_ausserhalb()) 
	      welt->kommando("STARTPUNKT_ZURUECK", &mittelpunkt);
      }
  }

  if (neu_moeglich) delete neu_moeglich;
  if (moegliche_entwicklungen) delete moegliche_entwicklungen;
}


/**---------------------------------------------------------------------------
  * STAAT::speichern()                        // virtuell
  ---------------------------------------------------------------------------*/
short STAAT::speichern(FILE *file)
{
  if (bekannte_infos.in_file_ausgeben(file)) return 1;
  if (neue_infos.in_file_ausgeben(file)) return 1;
  if (entwicklungen.in_file_ausgeben(file)) return 1;
  if (diplomatie.in_file_ausgeben(file)) return 1;
  fprintf(file, "%s %ld %ld %ld\n",mittelpunkt.to_string(), gesamtwertung,
		gesamtwertung_alt, anzahl_angriffe);
  return (ferror(file) != 0);
}


/**---------------------------------------------------------------------------
  * STAAT::laden()                            // virtuell
  ---------------------------------------------------------------------------*/
short STAAT::laden(FILE *file)
{
  char *puffer = new char[MAX_LAENGE_ATTRIBUTSZEILE];

  fscanf(file,"%s",puffer);
  bekannte_infos.aus_string_einlesen(puffer); // Schon bekommene Infos

  fscanf(file,"%s",puffer);
  neue_infos.aus_string_einlesen(puffer); // Neue bekommene, auszudruckende

  fscanf(file,"%s",puffer);
  entwicklungen.aus_string_einlesen(puffer); // Errungene technische Entwickl.

  fscanf(file,"%s",puffer);
  diplomatie.aus_string_einlesen(puffer); // Diplomatische Stati.

  mittelpunkt = ADR(file);
  fscanf(file,"%ld%ld%ld",&gesamtwertung, &gesamtwertung_alt, &anzahl_angriffe);

  delete puffer;
  return (ferror(file) != 0);
}


/**---------------------------------------------------------------------------
  * STAAT::naechste_phase()                   // virtuell
  ---------------------------------------------------------------------------*/
void STAAT::naechste_phase(long runde)
{
    // LOG Eintrag, falls erwuenscht.
    log('5', name);

    // Statusausgabe waerend des Zuges.
  io_deleteline(1);
  io_printxy(26,1,myltoa(runde));
  io_printxy(30,1,name);
  io_printxy(35,1,"->");

  // Diese Routine wird waehrend eines ZUGS(!) sehr oft aufgerufen (wahr-
  // scheinlich zwischen 20 und 100 mal), so dass hier nicht Dinge berech-
  // net werden duerfen, die nur einmal im Zug noetig sind.

  // Und nun verteile ich an mich, falls ich ein schwacher Spieler bin, 
  // noch einen Bonus. Allerdings nur in Phase fuenf der Runden 3 bis 30.
  
  if (runde == RUNDE_VERMISCHTES)
  {
    // Almosen fuer schlechtere Spieler...
    boni_verteilen();
  }
  bonus_verteilen(); // Eventuell Bonus moeglich. Macht momentan noch nichts.

}


/**---------------------------------------------------------------------------
  * STAAT::naechster_zug()                // virtuell
  ---------------------------------------------------------------------------*/
void STAAT::naechster_zug(long)
{
  // Wenn der Staat in der letzten Runde vernichtet wurde, dann loesche ich
  // erst hier dass Objekt. Der Staat hat also seinen letzten Ausdruck noch
  // bekommen.
  
  if (zivilisation_vernichtet()) {
    zur_vernichtung_vormerken();
    return;
  }

  // In der ersten Runde setze ich den Namen der Hauptstadt auf den
  // richtigen Wert.
  
  if (!hauptstadt()->attribut_gesetzt("NAME") ||
       hauptstadt()->attribut("NAME")[0] == 0 )
    hauptstadt()->attribut_setzen("NAME",attribut("HAUPTSTADT"));

  // Bei der ersten Auswertung setze ich die Startentwicklungen des
  // Staates. Dies darf nicht schon im Konstruktor stattfinden, da
  // sonst eine Aenderung dieser nicht propagiert wird. Neuerdings
  // geschehen naemlich die Einstellungen der Startwerte eines
  // Staates durch naechtraegliche Aederungen, nachdem jeder Staat
  // mit Defaultwerten neu konstruiert wird.
  
  // Es macht uebrigens auch nichts, wenn ich das jeden Zug wieder neu
  // mache.
  
  entwicklungen.setzen(attribut("ENTWICKLUNG1"));
  entwicklungen.setzen(attribut("ENTWICKLUNG2"));
  entwicklungen.setzen(attribut("ENTWICKLUNG3"));
    
  // Und Infos darueber. Denn sonst bekommt er sie nicht mehr, da Infos
  // nur ueber Dinge ausgespuckt werden, die man bauen kann, und Ent-
  // wicklungen kann man nur einmal bauen.

  spieler_bekommt_info_ueber(attribut("ENTWICKLUNG1"));
  spieler_bekommt_info_ueber(attribut("ENTWICKLUNG2"));
  spieler_bekommt_info_ueber(attribut("ENTWICKLUNG3"));

  // Hier muss ich bei meinem Landschaftsabbild, dass ich in der Welt
  // habe, die Funktion aufrufen, die die Objekte loescht.
  if (welt()) welt()->landschaftsabbild_objekte_loeschen(this);

  // Jetzt berechne ich noch, welche Entwicklungen der Staat angehen koennte,
  // da er die Voraussetzungen schon hat. Dadurch kann ich am Ende des Zuges
  // feststellen, ob neue Moeglichkeiten offen stehen, und diese dem Spieler
  // auf dem Ausdruck mitteilen.

  if (moegliche_entwicklungen) delete moegliche_entwicklungen;
  moegliche_entwicklungen = liste_jeder_moeglichen("ERFINDUNG");

  // Die Staat- und Stadtinfos werden jede Runde neu erzeugt und deshalb hier
  // geloescht.
  
  staat_und_stadt_infos.clear();

  // Und nun hole ich aus einer Asciidatei die Befehle, die der Spieler dem
  // den Staat und seinen Objekten erteilt hat.
  
  befehle_einlasten();
  
  // Nun werden noch die Einfluesse frisch berechnet.

  kommando_einfluesse_berechnen();

  // Und jetzt kann der Staat noch seine Befehle ausfuehren, damit
  // die DS-Befehle vor allen anderen Objekten drankommen. Dies muss
  // so sein, damit das US auch schon in der ersten Runde richtig klappt.

  aktion(-1); // siehe objekt.cpp --> OBJEKT::aktion(long)

}


/**---------------------------------------------------------------------------
  * STAAT::befehle_einlasten()
  * 
  * Oeffnet eine Asciidatei, in der die Befehle fuer den Staat und
  * seine Objekte stehen, und erteilt den Objekten der Reihe nach
  * die Befehle. Dabei werden die Befehle AA, AA1, AD bereits
  * ausgefuehrt!
  ---------------------------------------------------------------------------*/
void STAAT::befehle_einlasten()
{
  char *dateiname = g_uhr->befehlsdateiname(this);
  if (!dateiname) return; // Huch?
  
  FILE *file = fopen(dateiname,"r");
  if (!file) return; // Keine Befehle diese Runde.
  
  // Nun muss ich der Reihe nach eine Zeile lesen und diese verarbeiten.
  // Die Datei enthaelt keinerlei Kommentare oder Leerzeilen.

  char zeile[200];
  while (fgets(zeile, 199, file))
  {
    // In der Zeile stehen durch Spaces getrennt Objektname und Befehle

    char *p = zeile;

    while (*p != ' ' && *p) p++;
    if (*p != ' ') continue; // Fehlerhafte Befehlszeile
    
    *p=0; // Space durch Null ersetzen

    char *objname = zeile;
    OBJEKT *objekt = objekt_mit_namen(objname);
    if (!objekt) continue; // Schande!
    
    // Nur Befehle fuer eigene Objekte zulassen.
    if (objekt != this && !besitzt(objekt)) continue;

    // Jetzt der Reihe nach die einzelnen Befehle auslesen. Ein Befehl ist
    // mindestens da!

    char weitermachen = ' ';
    while (weitermachen) {
      // Assertion: p steht auf einer 0, der String geht aber noch weiter
      p++;
      char *befehl = p;
      while (*p && *p!=' ' && *p!=10 && *p!=13) p++;
      // Assertion: p steht auf 0 oder auf Space, CR oder LF

      weitermachen = *p == ' '; // Nur bei einem Space folgen noch Befehle
      *p=0; // Falls p auf Space steht noetig.

      // Handelt es sich um 'AA'?
      if (!*befehl || isspace(*befehl)) continue;
      else if (!mystrcmp_no_case(befehl, L("AA","CC"))) objekt->alle_befehle_abbrechen();
      else if (!mystrcmp_no_case(befehl, L("AA1","CC1"))) objekt->alle_befehle_abbrechen(1);
      else if (!mystrncmp_no_case(befehl, L("AD","DA"), 2))
      {
	// Der Rest der Zeile soll der Inhalt der Abkuerzung sein.
	if (weitermachen) abkuerzung_definieren(befehl[2], p+1);
	else abkuerzung_definieren(befehl[2], NULL);
	break;
      }
	
      else objekt->befehl_erteilen(befehl);

      // Assertion: wenn weitermachen != 0, dann geht String nach p weiter
    }
  }
  
  // Ok. Das war alle.
  fclose(file);
}
    

/**---------------------------------------------------------------------------
  * STAAT::boni_verteilen()
  * 
  * Ermittelt anhand der Einwohnerzahl, wie stark oder schwach der
  * Staat im internationalen Vergleich ist. Dementsprechend bekommt
  * er dann einen Bonus oder nicht.
  ---------------------------------------------------------------------------*/
void STAAT::boni_verteilen()
{
  // Liste aller Staaten holen und besten sowie schlechtesten Staat
  // ermitteln (Dessen Einwohnerzahl).
  
  long bester = 0, eigene = myatol(info_einwohnerzahl());
  long anzahl = 0;
  
  FOR_EACH_OBJEKT_IN (&globale_objekt_menge)
  DO (
    if (!mystrcmp(objekt->a_typ(), "STAAT"))
    {
      anzahl ++;
      long ez = myatol(objekt->info("EINWOHNERZAHL"));
      if (ez > bester) bester = ez;
    }
  )
  
  // So. Nun schaue ich mal, wie ich so stehe.

  long bonus = myatol(attribut("BONUS"));
  bonus += 14 - (eigene*15)/bester;
  if (bonus < 0) bonus = 0;
  attribut_setzen("BONUS",myltoa(bonus));
}


/**---------------------------------------------------------------------------
  * STAAT::bonus_verteilen()
  * 
  * Wenn noch Bonuspunkte da sind, wird eventuell ein Bonus verteilt.
  ---------------------------------------------------------------------------*/
void STAAT::bonus_verteilen()
{
}


/**---------------------------------------------------------------------------
  * STAAT::zug_abschliessen()
  * 
  * Macht abschliessende Sachen, die auch zur Vorbereitung fuer den
  * Ergebnisausdruck wichtig sind. Der Ergebnisausdruck selbst wird
  * jedoch hier NICHT angefertigt. Das veranlasst die Uhr. Sie ruft
  * dazu die STAAT - Funktion abschlussbericht() auf.
  *
  * Bricht alle Befehle ab.
  ---------------------------------------------------------------------------*/
void STAAT::zug_abschliessen(long)
{
  alle_befehle_abbrechen(); // Der Staat stapelt keine Befehle.

  // Aus den gemerkten moeglichen_entwicklungen und einer Neuberechnung
  // der Liste kann ich feststellen, welche der Entwicklungen erst ab
  // nun moeglich sind. Aber: Im allerersten Zug, da gebe ich alle moeglichen
  // Entwicklungen ueberhaupt aus, da zwar waerend des Zuges keine hinzuge-
  // kommen sind, aber doch seit Beginn des Spieles.

  if (neu_moeglich) delete neu_moeglich;
  neu_moeglich = liste_jeder_moeglichen("ERFINDUNG");

  // Von der Uhr hole ich mir die Nummer der aktuellen Spielrunde. Wenn
  // diese 1 ist, dann gebe ich alle Entwicklungen aus.

  long zug = myatol(objekt_mit_namen("Uhr")->info("ZUGNUMMER"));
  if (zug != 1) neu_moeglich -> subtrahiere(moegliche_entwicklungen);

}


/**---------------------------------------------------------------------------
  * STAAT::hauptstadt()
  * 
  * Liefert einen Zeiger auf die Hauptstadt oder NULL, wenn der Staat
  * keine Hauptstadt mehr hat. Fragt dazu die Gattung der Staedte im
  * Besitz ab.
  ---------------------------------------------------------------------------*/
STADT *STAAT::hauptstadt()
{
  OBJEKT *hs = objekt_mit_namen(attribut("HAUPTS_ABK"));
  if (hs) return (STADT *)hs;
  
  FOR_EACH_OBJEKT_IN (alle_objekte_im_direkten_besitz(L("TYP=STADT,GATTUNG=Hauptstadt","TYP=STADT,GATTUNG=Capital City")))
  DO_AND_DELETE
  (
    hs = objekt;
  )
  return (STADT *)hs;
}

/**---------------------------------------------------------------------------
  * STAAT::alle_staedte()
  * 
  ---------------------------------------------------------------------------*/
DOUBLIST *STAAT::alle_staedte(bool auch_tote)
{
  DOUBLIST *antwortliste = new DOUBLIST;
  OBJEKT *objekt;
  SCAN(&besitztum, objekt)
  {
    if (objekt->typ_ist("STADT") && (auch_tote || !objekt->zur_vernichtung_vorgemerkt())) {
      antwortliste->add_tail(new OBJEKT_LIST_NODE(objekt));
    }
    NEXT(objekt);
  }
  return antwortliste;
}


/**---------------------------------------------------------------------------
  * STAAT::alle_weltbauten()
  * 
  ---------------------------------------------------------------------------*/
DOUBLIST *STAAT::alle_weltbauten()
{
  DOUBLIST *antwortliste = new DOUBLIST;
  OBJEKT *objekt;
  SCAN(&besitztum, objekt)
  {
    if (objekt->typ_ist("WELTBAUT") && !objekt->zur_vernichtung_vorgemerkt()) {
      antwortliste->add_tail(new OBJEKT_LIST_NODE(objekt));
    }
    NEXT(objekt);
  }
  return antwortliste;
}


/**---------------------------------------------------------------------------
  * STAAT::alle_einheiten()
  * 
  * Ermittelt eine Liste aller Einheiten. Wenn das Flag auch_tote gesetzt
  * ist, werden auch vernichtete Einheiten ermittelt. 
  ---------------------------------------------------------------------------*/
DOUBLIST *STAAT::alle_einheiten(bool auch_tote)
{
  DOUBLIST *antwortliste = new DOUBLIST;

  FOR_EACH_OBJEKT_IN(alle_staedte(auch_tote))
  DO_AND_DELETE({
    DOUBLIST *einheitenliste = ((STADT *)objekt)->alle_einheiten(auch_tote);
    antwortliste->merge(einheitenliste);
    delete einheitenliste;
  }) ;

  // Auch dem Staat selbst koennen Einheiten unterstehen, allerdings
  // nur vernichtete! Z.B. wenn die Stadt atomgebombt wurde.
  OBJEKT *objekt;
  SCAN(&besitztum, objekt)
  {
    if (objekt->typ_ist("EINHEIT")
	&& (auch_tote || !objekt->zur_vernichtung_vorgemerkt())) {
	antwortliste->add_tail(new OBJEKT_LIST_NODE(objekt));
    }
    NEXT(objekt);
  }
      
  return antwortliste;
}


/**---------------------------------------------------------------------------
  * STAAT::alle_einheiten_sortiert()
  * 
  * Ermittelt eine sortierte Liste  aller nicht vernichteten Einheiten.
  * Das Flag auch_tote gibt an, ob auch die Vernichteten einheiten
  * migenommen werden sollen.
  ---------------------------------------------------------------------------*/
DOUBLIST *STAAT::alle_einheiten_sortiert(bool auch_tote)
{
    DOUBLIST *antwortliste = alle_einheiten(auch_tote);
    antwortliste->sort(EINHEIT::sortfunktion_einheiten);
    return antwortliste;
}
     

/**---------------------------------------------------------------------------
  * STAAT::zivilisation_vernichtet()
  * 
  * Stellt fest, ob der Staat vernichtet ist. Das ist er wenn er
  * keine Staedte oder auch keine Einwohner mehr hat.
  *
  * @return
  * short 0, wenn nein, short 1, wenn schon vernichtet.
  ---------------------------------------------------------------------------*/
short STAAT::zivilisation_vernichtet()
{
  return (!myatol(info_einwohnerzahl()) || !myatol(info_staedtezahl()));
}


/**---------------------------------------------------------------------------
  * STAAT::dipl_status_gegenueber()
  * 
  * Ermittelt den diplomatischen Status, den ich gegenueber einem anderen
  * Staat habe.
  * @param
  * OBJEKT *obj:  Ein Objekt des fraglichen Staates oder er selbst.
  * @return
  * short Summe aus: 0 fuer gegnerisch, 1 fuer neutral, 2 fuer freundlich,
  * 3 fuer "bin es selbst!"
  ---------------------------------------------------------------------------*/
short STAAT::dipl_status_gegenueber(OBJEKT *obj)
{
  // Zuerst ermittle ich den Staat des Objektes.
  while (obj && !obj->typ_ist("STAAT")) obj=obj->besitzer;
  if (!obj) return 1; // Bei Fehler als neutral melden.
  if (obj == this) return 3; // Bin ich selbst.
  
  char *s = diplomatie.abfragen(obj->name);
  
  // Wenn der Status noch nicht festliegt, dann setze ich ihn nun auf den
  // Defaultstatus.
  if (!s) {
    if (char *ds = attribut("DEFAULT_STATUS")) diplomatie.setzen(obj->name,ds);
    else diplomatie.setzen(obj->name,L("VN","PN"));
    return dipl_status_gegenueber(obj);
  }
  
  else if (s[1] == L('G','H')) return 0; // Gegner
  else if (s[1] == L('F','A')) return 2; // Freund
  else return 1; // Ansonsten Neutral.
}


/**---------------------------------------------------------------------------
  * STAAT::endgueltig_freundlich_gegenueber()
  * 
  * Stellt fest, ob wir den Staat, der ein bestimmtes Objekt besitzt
  * oder ist, als endgueltig freundlich eingestuft haben.
  * @param
  * OBJEKT *obj     Objekt des anderen Staates.
  * @return
  * short 0, falls nicht endgueltig freundlich
  ---------------------------------------------------------------------------*/
short STAAT::endgueltig_freundlich_gegenueber(OBJEKT *obj)
{
  dipl_status_gegenueber(obj); // Setzt u.U Defaultwert.
  
  // Nun ermittle ich den Staat des Objektes.
  while (obj && !obj->typ_ist("STAAT")) obj=obj->besitzer;
  if (!obj) return 0; // Bei Fehler als nicht EF melden.
  
  char *s = diplomatie.abfragen(obj->name);
  return (!mystrcmp(s, L("EF","FA")));  
}


/**---------------------------------------------------------------------------
  * STAAT::mir_ist_endgueltig_freundlich_gegenueber()
  * 
  * Stellt fest, ob der Staat, der ein bestimmtes Objekt besitzt
  * oder ist, mich als endgueltig freundlich eingestuft hat.
  * @param
  * OBJEKT *obj     Objekt des anderen Staates.
  * @return
  * short 0, falls nicht endgueltig freundlich
  ---------------------------------------------------------------------------*/
short STAAT::mir_ist_endgueltig_freundlich_gegenueber(OBJEKT *obj)
{
  while (obj && !obj->typ_ist("STAAT")) obj=obj->besitzer;
  if (!obj) return 0; // Bei Fehler als nicht EF melden.

  return ((STAAT *)obj)->endgueltig_freundlich_gegenueber(this);
}  


/**---------------------------------------------------------------------------
  * STAAT::befehl_auswerten()             // virtuell
  ---------------------------------------------------------------------------*/
short STAAT::befehl_auswerten(char *befehl, long)
{
  if      (!mystrncmp(L("KR","IM"),befehl,2)) return befehl_staatsform_aendern(befehl);
  else if (!mystrncmp(L("DM","ID"),befehl,2)) return befehl_staatsform_aendern(befehl);
  else if (!mystrncmp(L("PU","PU"),befehl,2)) return befehl_staatsform_aendern(befehl);
  else if (!mystrncmp(L("TK","IT"),befehl,2)) return befehl_staatsform_aendern(befehl);
  else if (!mystrncmp(L("DS","CD"),befehl,2)) return befehl_diplomatischer_status(befehl);
  else if (!mystrncmp(L("AM","XC"),befehl,2)) return befehl_administration(befehl);
  else if (!mystrncmp(L("MT","ME"),befehl,2)) return befehl_mitteilung(befehl);
  else if (!mystrncmp(L("BA","BU"),befehl,2)) return befehl_bauen_abbrechen(befehl);
  else if (!mystrncmp(L("AB","AP"),befehl,2)) return befehl_bauen_abbrechen(befehl);
  else if (!mystrcmp("*L",befehl))            return befehl_landschaftsabbild();
  else {
    report(L("Der Befehl %s kann nicht dem Staat erteilt werden!\n","Command %s cannot be given to your empire!\n"), befehl);
    return 1;
  }
}


/**---------------------------------------------------------------------------
  * STAAT::befehl_bauen_abbrechen()
  * 
  * Leitet ein BA oder einen AB-Befehl an alle Staedte wieder. Der
  * Neue Befehl hat allerdings als Zweites Zeichen ein Minus '-',
  * damit die Stadt weiss, dass sie keine Fehlermeldung ausgeben soll,
  * wenn etwas nicht klappt (z.B. Projektliste voll oder Projekt garnicht
  * eingelastet). Sonst gibt es oft einen Wust von ueberfluessigen Fehler-
  * meldungen.
  ---------------------------------------------------------------------------*/
short STAAT::befehl_bauen_abbrechen(char *befehl)
{
  char neuer_befehl[100];
  strcpy(neuer_befehl, befehl);
  neuer_befehl[1] = '-'; // Befehl markieren.
  
  FOR_EACH_OBJEKT_IN(alle_staedte())
  DO_AND_DELETE ( objekt->befehl_erteilen(neuer_befehl); )
  return 1;
}


/**---------------------------------------------------------------------------
  * STAAT::befehl_mitteilung()
  * 
  ---------------------------------------------------------------------------*/
short STAAT::befehl_mitteilung(char *befehl)
{
  const int anzahl_form = 10;
  static char *formulierung[anzahl_form] =
  {
    L("Gesandte aus dem Staat %s berichten: \"%s\"\n","Messengers from the empire %s report: \"%s\"\n"),
    L("In meinem pers~onlichen Briefkasten fand diesen Morgen folgende Nachricht aus %s: \"%s\"\n","This morning I received a letter from  %s: \"%s\"\n"),
    L("Reisende aus %s berichten: \"%s\"\n","Travelers from %s report: \"%s\"\n"),
    L("Wir bekommen eine Botschaft aus %s: \"%s\"\n","We received a message from %s: \"%s\"\n"),
    L("Botschafter aus %s ~uberbringen folgende Nachricht: \"%s\"\n","Messengers from %s ~report the following: \"%s\"\n"),
    L("Meine Informanten aus %s berichten mir am Nachmittag: \"%s\"\n","Our spies in %s report: \"%s\"\n"),
    L("Von Fremden aus %s kam mir zu Ohren: \"%s\"\n","Some strangers from %s told me: \"%s\"\n"),
    L("Angeblich soll in %s jemand gesagt haben: \"%s\"\n","Somebody in %s said: \"%s\"\n"),
    L("In %s brodelt die Ger~uchtek~uche: \"%s\"\n","Rumours from %s: \"%s\"\n"),
    L("Soeben erreicht mich eine Depesche aus %s: \"%s\"\n","I just got a telegram from %s: \"%s\"\n")
  };
  
  if (!befehl[2]) return 1;
  char text[512];
  sprintf(text, formulierung[io_random(anzahl_form)],
	a_name(), befehl+2);

  // Und nun ersetze ich noch alle Unterstriche durch Spaces, damit der
  // automatische Zeilenumbruch funktioniert.
  
  char *scan = text;
  while (*scan) {
    if (*scan == '_') *scan=' ';
    scan++;
  }

  OBJEKT_LIST_NODE *staatnode;
  SCAN(&globale_objekt_menge, staatnode)
  {
    if (staatnode->objekt != this) {
      if (!mystrcmp("STAAT",staatnode->objekt->a_typ()))
	      staatnode->objekt->report(text);
    }
    NEXT(staatnode);
  }
  
  return 1;
}



/**---------------------------------------------------------------------------
  * STAAT::befehl_staatsform_aendern()
  * 
  * Veranlasst den Staat, seine Staatsform zu aendern. Das kann durch
  * einen der Befehle KR, DM, PU, TK geschehen.
  ---------------------------------------------------------------------------*/
short STAAT::befehl_staatsform_aendern(char *befehl)
{
  // Die fuenf verschiedenen Faelle muessen relativ unterschiedlich
  // gehandhabt werden.

  char *staatsform = attribut("STAATSFORM"); // Abkuerzung vorbereiten

  if  (!strncmp(befehl, L("KR","IM"), 2)) { // Kroenung

    if (!mystrcmp(staatsform,L("Monarchie","Monarchy"))) { // Wuerde nichts aendern.
      report(L("Da ich bereits gekr~ont bin und unsere Staatsform die ","The command IM is pretty senseless, because I already wear the crown and because our form of government "));
      report(L("Monarchie ist, ist der Befehl 'KR' hinf~allig.\n","is still Monarchy.\n"));
      return 1;
    }

    // Die Monarchie kann ich nur einfuehren, wenn vorher Despotismus
    // war. Ansonsten wuerde das einen Rueckfall ins Mittelalter bedeuten.
    if (mystrcmp(staatsform, L("Despotismus","Despotism"))) {
      report(L("Es ist v~ollig unm~oglich, von einer modernen Staats","It is quite impossible to return from such a modern form of government like "));
      report(L("form wie %s wieder zur Monarchie zur~uckzukehren. "," %s back to good old Monarchy... "),
	konjugation(staatsform, DATIV | SINGULAR ));
      report(L("Dieses Zeitalter ist in unserem Staat eng~ultig abgeschlossen.\n","This aera has been finished long ago.\n"));
      return 1;
    }

    if (!entwicklungen.gesetzt(L("Monarchie","Monarchy"))) { // Entwicklung vorhanden?
      report(L("Mein Versuch, die Monarchie einzuf~uhren und meine Kr~onung ","My attempt to introduce Monarchy and  my coronation fails. "));
      report(L("zu veranlassen, scheitert, da im Staat noch keine genauen ","Obviously, nobody in my very own Empire knows the "));
      report(L("Vorstellungen davon existieren, wie eine Monarchie im ein","secrets of royality. "));
      report(L("zelnen verwirklicht werden soll. Mein pers~onlicher Berater ","My personal advisor recommends: "));
      report(L("empfiehlt mir, zuerst wissenschaftliche Untersuchungen zu ","Research, and thou shalt know more! "));
      report(L("befehlen.\n","\n"));
      return 1;
    }

    // Um eine Monarchie einzufuehren, brauche ich in der Hauptstadt
    // einen Palast.
    OBJEKT *hs = objekt_mit_namen(attribut("HAUPTS_ABK"));
    if (!hs) return 1;
    
    short erfolg = 0;
    FOR_EACH_OBJEKT_IN (hs->alle_objekte_im_direkten_besitz(L("GATTUNG=Palast","GATTUNG=Palace")))
    DO_AND_DELETE   (  erfolg = 1;  )
    if (!erfolg) {
      report(L("Um mich kr~onen zu lassen und die Monarchie einzuf~uhren, ","To wear a glittering crown and to introduce monarchy "));
      report(L("ben~otige ich unbedingt einen Palast in unserer Hauptstadt ","we ought to build a palace in our Capital City. "));
      report("%s.\n", attribut("HAUPTSTADT"));
      return 1;
    }

    // So, dass soll fuers erste an Voraussetzungen genuegen. Jetzt lasse
    // ich den Despoten kroenen...

    long titelnummer = myatol(befehl+2);
    char *titel=L("K~onig","King");  // Bei 0 und 1: Default
    switch (titelnummer) {
      case 2: titel = L("Zar","Zar"); break;
      case 3: titel = L("Gro~smogul","Grand  Mogul"); break;
      case 4: titel = L("Scheich","Sheik"); break;
      case 5: titel = L("Khan","Khan"); break;
      case 6: titel = L("Sultan","Sultan"); break;
      case 7: titel = L("Maharadscha","Maharadscha"); break;
      case 8: titel = L("Shogun","Shogun"); break;
      case 9: titel = L("Daimo","Daimo"); break;
      case 10: titel = L("Pharao","Pharao"); break;
      case 11: titel = L("Schahinschah","Schahinschah"); break;
      case 12: titel = L("C~asar","Cesar"); break;
      case 13: titel = L("King","Prince"); break;

      case 21: titel = L("K~onigin","Princess"); break;
      case 22: titel = L("Zarin","Zarin"); break;
      case 27: titel = L("Maharani","Maharani"); break;
      case 30: titel = L("Pharaonin","Pharaoness"); break;
      case 33: titel = L("Queen","Queen"); break;
    }      

    report(L("Endlich ist es soweit, ein historischer Augenblick ist gekommen. ","A historical moment has come! "));
    report(L("In meinem Palast in %s wird mir ","In my splendid palace in %s "), attribut("HAUPTSTADT"));
    report(L("die Krone ~uberreicht. Ab heute stehe ich als %s ","I am coronated. From now on, I am ruling a new kingdom as %s "), titel);
    report(L("%s an der Spitze des neuen K~onigreiches ","%s of "), attribut("HERRSCHERNAME"));
    report("%s.\n", a_name());
   
    attribut_setzen("TITEL",titel);
    attribut_setzen("STAATSFORM",L("Monarchie","Monarchy"));
    kommando_einfluesse_berechnen();
    return 1;

  } // Befehl KR

  else if (!strncmp(befehl, L("DM","ID"), 2)) { // Einf~uhrung der Demokratie

    if (!mystrcmp(staatsform,L("Demokratie","Democracy"))) { // Wuerde nichts aendern.
      report(L("Die Demokratie mu~s zum Gl~uck nicht mehr eingef~uhrt werden,","The democracy needs not to be introduced again, "));
      report(L(" sie schon seit geraumer Zeit etabliert ist.\n"," because it is fortunately already established!\n"));
      return 1;
    }

    if (!entwicklungen.gesetzt(L("Demokratie","Democracy"))) { // Entwicklung vorhanden?
      report(L("Um die Demokratie einzuf~uhren, m~ussen erst ihre grund","We have to discover the basics of a democratical society first "));
      report(L("legenden Konzepte erforscht werden.\n","before introducing Democracy.\n"));
      return 1;
    }

    // Um eine Demokratie einzufuehren, brauche ich in ein Parlament.
    // Selbiges muss im uebrigen nicht unbedingt in der Hauptstadt stehen.
    short erfolg = 0;
    FOR_EACH_OBJEKT_IN (alle_objekte_im_besitz(L("GATTUNG=Parlament","GATTUNG=Parliament")))
    DO_AND_DELETE (if (objekt) erfolg=1; )
    if (!erfolg) {
      report(L("Ohne ein Parlamentsgeb~aude ist es unm~oglich, die Demokratie","Without a parliament building we cannot introduce"));
      report(L(" einzuf~uhren!\n"," Democracy!\n"));
      return 1;
    }

    // Das Reich darf keine Arbeitslager und auch kein Hauptquartier haben.
    // Alle diese verbotenen Objekte sind mit DM_Verboten gekennzeichnet.
    
    if (alle_objekte_im_besitz("DM_Verboten") -> count_and_del() )
    {
      report(L("Bei der Einf~uhrung der Demokratie ist es untragbar, da~s ","You cannot introduce Democracy while there are still "));
      report(L("im Staat noch Arbeitslager oder Hauptquartiere existieren. ","works camps and/or headquarters in your towns! "));
      report(L("Diese m~ussen erst abgerissen werden. \n","Raze´em first! \n"));
      return 1;
    }

    // So. Nun kommt es noch darauf an, welche Staatsform wir gerade haben.
    if (!mystrcmp(staatsform, L("Technokratie","Technocracy"))) {
      report(L("Um von unserer fortschritlichen Technokratie wieder zur","It´s much too late to return from our progressive Technocracy back to"));
      report(L(" gew~ohnlichen Demokratie zur~uckzukehren, sind unsere Strukturen "," a lowly Democracry."));
      report(L("schon viel zu festgefahren. Der technologische Fortschrittsglaube"," Technology, progress, our brave new world"));
      report(L(" l~a~st sich eben nicht mehr bremsen.\n"," shall last forever!\n"));
      return 1;
    }

    // Neuen Titel aussuchen
    long titelnummer = myatol(befehl+2);
    char *titel=L("Pr~asident","President");  // Bei 0 und 1: Default
    switch (titelnummer) {
      case 1: titel = L("Pr~asident","President"); break;
      case 2: titel = L("Reichspr~asident","President of the Reich"); break;
      case 3: titel = L("Staatspr~asident","President of State"); break;
      case 4: titel = L("Bundespr~asident","Imperial President"); break;
      case 5: titel = L("Staatsratsvorsitzender","Chairman"); break;
      case 6: titel = L("Kanzler","Chancellor"); break;
      case 7: titel = L("Reichskanzler","Chancellor of the Reich"); break;
      case 8: titel = L("Bundeskanzler","Imperial Chancellor"); break;
      case 9: titel = L("Premierminister","Prime Minister"); break;

      case 21: titel = L("Pr~asidentin","The female President"); break;
      case 22: titel = L("Reichspr~asidentin","The female President of the Reich"); break;
      case 23: titel = L("Staatspr~asidentin","The female Imperial President"); break;
      case 24: titel = L("Bundespr~asidentin","The female President of State"); break;
      case 25: titel = L("Staatsratsvorsitzende","The Chairwoman"); break;
      case 26: titel = L("Kanzlerin","The female Chancellor"); break;
      case 27: titel = L("Reichskanzlerin","The female Chancellor of the Reich"); break;
      case 28: titel = L("Bundeskanzlerin","The female Imperial Chancellor"); break;
      case 29: titel = L("Premierministerin","The female Prime Minister"); break;
    }

    // So. Jetzt fuehre ich ein.

    report(L("Mit der Einf~uhrung der Demokratie bricht in unserem Staat","With the introduction of the Democracy our Empire"));
    report(L(" ein neues Zeitalter an. Dank der gro~sen Beliebtheit meiner "," enters a new political age. Thanks to the great public acceptance of the new "));
    report(L("Regierung verf~ugt unsere Partei eine Komfortable Mehrheit im ","government our party has a comfortable majority "));
    report(L("neuen Parlament.\n","in the new parliament.\n"));
    attribut_setzen("TITEL",titel);
    attribut_setzen("STAATSFORM",L("Demokratie","Democracy"));
    kommando_einfluesse_berechnen();
    return 1;
  }

  else if (!strncmp(befehl, L("TK","IT"), 2)) {

    if (!mystrcmp(staatsform,L("Technokratie","Technocracy"))) { // Wuerde nichts aendern.
      report(L("Die Einf~uhrung der Technokratie ist ~uberfl~ussig, da ","Introducing Technocracy is superflous because this IS "));
      report(L("dies bereits unsere Staatsform ist.\n","already our form of government.\n"));
      return 1;
    }

    if (!entwicklungen.gesetzt(L("Technokratie","Technocracy"))) { // Entwicklung vorhanden?
      report(L("Eine Einf~uhrung der Technokratie ist sinnlos, solange uns","We would really like to introduce Technocracy but we"));
      report(L(" eine gewisse technisch-#wissenschaftliche Perfektion noch"," lack the necessary technical and scientific"));
      report(L(" fehlt.\n"," basics.\n"));
      return 1;
    }

    if (mystrcmp(staatsform, L("Demokratie","Democracy"))) {
      report(L("Bevor wir eine so fortschrittliche Staatsform wie die ","Before establishing an advanced government like the Technocracy "));
      report(L("Technokratie bekommen, sollten erst einmal demokratische ","we should be in a democratical state first."));
      report(L("Verh~altnisse eingef~uhrt werden. Die "," "));
      report(L("Demokratie ist gewisserma~sen eine notwendige Vorstufe.\n","Democracy should be seen as a necessary step in front.\n"));
      return 1;
    }

    // So. Alles klar.
    report(L("Die technische Perfektion in unserem Staat erlaubt es uns, ","The technical and scientific progress in our Empire grants us "));
    report(L("die Demokratie weiter zu entwickeln und zu automatisieren, und","to enhance and improve democracy until we finally"));
    report(L(" somit die Technokratie zu erreichen. Damit haben wir wohl die"," have entered the state of Technocracy. We have now achieved the"));
    report(L(" modernste Staatsform erreicht.\n"," most progressive form of government. Congratulations !\n"));
//    attribut_setzen("TITEL",L("Pr~asident","President"));
    attribut_setzen("STAATSFORM",L("Technokratie","Technocracy"));
    kommando_einfluesse_berechnen();
    return 1;
  }

  else { // if (befehl[0] == 'P')
    if (!mystrcmp(staatsform,L("Milit~ardiktatur","Military_Dictatorship"))) {
      report(L("Da ich bereits an der Spitze einer gefestigten Diktatur stehe,","Because I´m already the leader of an established dictatorship"));
      report(L(" ist der Befehl 'PU' ~uberfl~ussig. Ich w~u~ste nicht, wie ich"," the command  'PU' seems foolish. How can I putsch"));
      report(L(" gegen mich selbst putschen soll.\n"," myself ?\n"));
      return 1;
    }

    else if (!mystrcmp(staatsform,L("Despotismus","Despotism"))) {
      report(L("In einem totalit~aren Staat wie dem Despotismus ist kein ","No putsch possible in a totalitarian government"));
      report(L("Milit~arputsch m~oglich. Der Befehl 'PU' klappt deshalb nicht. ","like Despotism. Command 'PU' aborted. "));
      report(L("Man sollte vorher die Monarchie eingef~uhrt haben.\n","You should introduce Monarchy beforehand.\n"));
      return 1;
    }

    short erfolg = 0;
    FOR_EACH_OBJEKT_IN (alle_objekte_im_besitz(L("GATTUNG=Hauptquartier","GATTUNG=Headquarters")))
    DO_AND_DELETE (if (objekt) erfolg=1; )
    if (!erfolg) {
      report(L("Der Befehl 'PU' funktioniert noch nicht, da man zum Milit~arputsch","Command 'PU' failed!  You have to build headquarters"));
      report(L(" ein Hauptquartier ben~otigt.\n"," first.\n"));
      return 1;
    }

    report(L("V~ollig ~uberraschend f~ur die restliche Regierung und die ","Nasty surprise for the government and all the population! "));
    report(L("Bev~olkerung gelingt es mir, mithilfe eines elit~aren Kreises ","With a little help by my militaristic friends and some "));
    report(L("aus der Armee durch einen Putsch alle Macht in meinen H~anden ","guerillas I now do wield all the political power. "));
    report(L("zu b~undeln. Ich stehe nun unangefochten an der ","Now I´m atop of a new miltary government "));
    report(L("Spitze des Staates und habe volle Befehlsgewalt. ","and possess supreme command. "));

    short par_zerst = 0;
    FOR_EACH_OBJEKT_IN (alle_objekte_im_besitz(L("GATTUNG=Parlament","GATTUNG=Parliament")))
    DO_AND_DELETE (
      objekt->zur_vernichtung_vormerken();
      par_zerst ++;
    )

    if (par_zerst > 1)
      report(L("Bei den K~ampfen wurden alle Parlamentsgeb~aude zerst~ort.\n","All parliaments have been destroyed during the riots..\n"));
    else if (par_zerst)
      report(L("Bei den K~ampfen wurde das Parlamentsgeb~aude zerst~ort.\n","The parliament building has been destroyed during the fights.\n"));

    // Neuen Titel aussuchen
    long titelnummer = myatol(befehl+2);
    char *titel=L("F~uhrer","F~uhrer");  // Bei 0 und 1: Default
    switch (titelnummer) {
      case 1: titel = L("F~uhrer","F~uhrer"); break;
      case 2: titel = L("Feldherr","Feldherr"); break;
      case 3: titel = L("General","General"); break;
      case 4: titel = L("Staatsmarschall","Marshall"); break;
      case 5: titel = L("Diktator","Dictator"); break;

      case 21: titel = L("F~uhrerin","F~uhrerin"); break;
      case 22: titel = L("Feldherrin","Feldherrin"); break;
      case 23: titel = L("Gener~alin","The female General"); break;
      case 24: titel = L("Staatsmarschall","Marshall of the Reich"); break;
      case 25: titel = L("Diktatorin","The female Dictator"); break;
    }

    attribut_setzen("STAATSFORM",L("Milit~ardiktatur","Military_Dictatorship"));
    attribut_setzen("TITEL",titel);
    kommando_einfluesse_berechnen();
    return 1;
  }
}


/**---------------------------------------------------------------------------
  * STAAT::befehl_diplomatischer_status()
  * 
  * Befehl DS, mit dem der Spieler den Diplomatischen Status zu einem
  * anderen Staat festlegen kann.
  * @param
  * char *befehl:  "DS%3s,%c%c",<name>,{V|E}, {F|N|G}
  * oder:   DSVN DSVF DSEN DSEF DSEG: Defaultwert einstellen.
  ---------------------------------------------------------------------------*/
short STAAT::befehl_diplomatischer_status(char *befehl)
{
  if (strlen(befehl) == 4) {
    if ( mystrcmp(befehl+2, L("EG","FH"))
	 && mystrcmp(befehl+2, L("VN","PN"))
	 && mystrcmp(befehl+2, L("EN","FN"))
	 && mystrcmp(befehl+2, L("VF","PA"))
	 && mystrcmp(befehl+2, L("EF","FA")) )
    {
      report(L("(* Befehl %s: Als Werte sind nur EF, VF, EN, VN und EG erlaubt. *)\n","(* Command %s: Parameters  FH, PN, FN, PA and FA only. *)\n"),
       befehl);
      return 1;
    }
    
    attribut_setzen("DEFAULT_STATUS",befehl+2);
    report(L("Wir entschlie~sen uns, unbekannten V~olkern gegen~uber ","We will decide on  diplomatic status "));
    report(L("stets vorerst die Haltung '%s' einzunehmen.\n"," '%s' whenever encountering unknown units.\n"),befehl+2);
    return 1;
  }

  if (strlen(befehl) != 8) {
    report(L("(* Falsche Syntax beim Befehl '%s',","(* Syntax error. Command '%s' invalid."), befehl);
    report(L("richtig w~are z.B. 'DSSTA,VF'. *)\n"," Syntax e.g.. 'CDSTA,PA'. *)\n"));
    return 1;
  }

  // Zeichen 2 bis 4 sind die Abkuerzung fuer den Staat
  char staatname[12];
  sprintf(staatname,"STAAT%c%c%c",befehl[2],befehl[3],befehl[4]);

  if (!mystrcmp_no_case(name, staatname+5)) {
    report(L("(* Befehl '%s': Wie sollen wir einen diplomatischen Status","(* Command '%s': Hmmmm.. change that diplomatic status?"),befehl);
    report(L(" uns selbst gegen~uber haben? *)\n"," Freaky idea.  *)\n"));
    return 1;
  }

  // Kennt der Spieler den Staat schon?

  if (!bekannte_infos.gesetzt(staatname)) {
    report(L("(* Befehl DS: Bis jetzt (Anfang der Runde) hatten wir noch keinen Kontakt zu einem ","(* Command CD: Until now (beginning of turn) we did not haye any contact to an "));
    report(L("Staat mit der Abk~urzung '%s'. *)\n","empire '%s'. *)\n"), staatname+5);
    return 1;
  }

  // Mal die Daten des Staates holen..
  OBJEKT *staat = objekt_mit_namen(staatname+5);
  if (!staat) {
    report(L("(* DS: Ein Staat mit der Abk~urzung '%s' existiert nicht mehr. *)\n","(* CD: An empire with the abbreviation  '%s' does not exist anymore. *)\n")
	    ,staatname+5);
    return 1;
  }

  // Jetzt hole ich mir die weiteren Parameter und teste auf Gueltigkeit.
  char p1 = toupper(befehl[6]), p2=toupper(befehl[7]);
  if ((p1!=L('V','P')
       && p1!=L('E','F'))
      || (p2!=L('G','H')
	  && p2!=L('N','N')
	  && p2!=L('F','A')))
  {
    report(L("(* %s: Es sind nur die Zust~ande VF VN EF EN und EG m~oglich. *)\n","(* %s: Status can only be FH PN FN PA or FA  *)\n")
	     ,befehl);
    return 1;
  }

  // Gut. Jetzt kann ich es gelten lassen.
  report(L("Wir ~andern unseren diplomatischen Status zu %s auf","We change our diplomatic status for %s to"), staat->a_name());
  report(" \"%s\".\n", dipl_status_name(p1,p2));
  char s[3];
  s[0]=p1;
  s[1]=p2;
  s[2]=0;
  diplomatie.setzen(staatname+5,s);

  return 1;
}


/**---------------------------------------------------------------------------
  * STAAT::befehl_administration()
  * 
  * Mit dem AM-Befehl kann der Spieler besimmte Parameter steuern, wie er
  * seinen Printout bekommen will.
  ---------------------------------------------------------------------------*/
short STAAT::befehl_administration(char *befehl)
{
    switch (befehl[2]) {
    case L('W','W'): // Weltkarte: Breite x Hoehe eines Stueckes im HTML-Printout
	return befehl_administration_w(befehl+3);
    case L('G','P'): // Gesamtkarte als PS-File im HTML-Printout ein/aus.
	return befehl_administration_g();
    default:
	report(L("*** %s: Nach dem Befehl AM muss einer der Buchstaben W oder G kommen.\n ","*** %s: Command XC: You must give letter W or P.\n "), befehl);
	return 1;
    }
}

/**---------------------------------------------------------------------------
  * STAAT::befehl_administration_w()
  * 
  * AMW: Groesser der Kartenstueck im HTML-Printout.
  ---------------------------------------------------------------------------*/
short STAAT::befehl_administration_w(char *parameter)
{
    if (strlen(parameter) == strspn(parameter, "0123456789,"))
    {
	int breite, hoehe;
	int anzahl = sscanf(parameter, "%d,%d", &breite, &hoehe);
	if (anzahl != 2) report(L("*** AMW%s: Du mu~st hinter AMW Breite und H~ohe in Feldern angeben, durch","*** XCW%s: You need to give Width and Height in squares separated by a")
				L(" ein Komma getrennt, z.B. AMW15,10 oder AMW20,18. ***\n"," colon, eg. XCW15,10 or XCW20,18. ***\n"), parameter);
	else {
	    if (breite < 20) {
		report(L("*** AMW%s: Die Kartenausschnitte kannst Du nicht weniger als 20 Felder breit machen.","*** XCW%s: The clippings must have a minimum width of 20 squares. ")
		       L(" Ich nehme 20 Felder Breite. ***"," I shall use Width 20. ***"), parameter);
		breite = 20;
	    }
	    if (hoehe < 10) {
		report(L("*** AMW%s: Die Kartenausschnitte kannst Du nicht weniger als 10 Felder hoch machen.","*** XCW%s: The clippings must have a minimum height of 10 squares.")
		       L(" Ich nehme 10 Felder H~ohe. ***"," I shall use Height 10. ***"), parameter);
		hoehe = 10;
	    }
	    attribut_setzen("KARTENBREITE", myltoa(breite));
	    attribut_setzen("KARTENHOEHE", myltoa(hoehe));
	    report(L("Die Teilst~ucke der HTML-Weltkarte werden ab jetzt maximal %s breit"," The HTML-WorldMap will now be Width  %s "), myltoa(breite));
	    report(L(" und %s Felder hoch sein. "," and Height %s. "), myltoa(hoehe));
	}
    }
    return 1; // Befehl zuende.
}


/**---------------------------------------------------------------------------
  * STAAT::befehl_administration_g()
  * 
  * AMG: PostScript Gesamtkarte ein-/ausschalten.
  ---------------------------------------------------------------------------*/
short STAAT::befehl_administration_g()
{
    if (attribut_gesetzt("PSGesamtkarte")) {
	attribut_loeschen("PSGesamtkarte");
	report(L("Ab jetzt bekommst du keine Gesamtkarte als PostScript Datei mehr. ","From now on you will not get your map as a PostScript file. "));
    }
    else {
	attribut_setzen("PSGesamtkarte");
	report(L("Ab jetzt bekommst du immer eine Gesamtkarte als PostScript Datei. "," From now on you will  get your map as a PostScript file. "));
    }
    return 1; // Befehl zuende.
}


/**---------------------------------------------------------------------------
  * STAAT::dipl_status_name()
  * 
  * Liefert die Klartextbezeichung fuer einen bestimmten Diplomatischen
  * Status.
  *
  * @param
  * char p1: 'V' oder 'E'
  * char p2: 'F', 'N' oder 'G'
  ---------------------------------------------------------------------------*/
char *STAAT::dipl_status_name(char p1, char p2)
{
  static char erg[50];
  if (toupper(p1) == L('V','P'))
      strcpy(erg,L("vorl~aufig ","preliminarily "));
  else strcpy(erg, L("endg~ultig ","finally "));

  switch (toupper(p2)) {
    case L('F','A'):
	strcat(erg,L("freundlich","allied"));
	break;
    case L('N','N'):
	strcat(erg,L("neutral","neutral"));
	break;
    default: strcat(erg, L("gegnerisch","hostile")); break;
  }
  return erg;
}


/**---------------------------------------------------------------------------
  * STAAT::befehl_landschaftsabbild()
  * 
  * Fakebefehl, der das komplette Landschaftsabbild des Staates ak-
  * tualisiert, so dass der Spieler die ganze Welt auf einmal sieht,
  * und zwar mit allen Objekten (diese allerdings zum Zeitpunkt des
  * Zuganfanges).
  ---------------------------------------------------------------------------*/
short STAAT::befehl_landschaftsabbild()
{
  welt()->landschaftsabbild_komplett(this);
  return 1;
}


/**---------------------------------------------------------------------------
  * STAAT::kennungattr()
  * 
  * Erwartet einen Buchstaben und gibt den Attributsnamen zurueck,
  * unter dem die Abkuerzung zu dem Buchstaben definiert ist. Der
  * Buchstabe sollte von A - Z sein und waehlt eine Abkuerzung
  * fuer die Befehle AD und AV aus.
  ---------------------------------------------------------------------------*/
char *STAAT::kennungattr(char kennung)
{
  static char antwort[10];
  sprintf(antwort,"ABK_%c",normkennung(kennung));
  return antwort;
}
  
/**---------------------------------------------------------------------------
  * STAAT::abkuerzung_definieren()
  * 
  * Definiert eine Abkuerzung.
  ---------------------------------------------------------------------------*/
void STAAT::abkuerzung_definieren(char kennung, char *text)
{
  char *klasse = kennungattr(kennung);
  if (text) attribut_setzen(klasse, av_ad_aa_entfernen(text));
  else attribut_loeschen(klasse);
}

/**---------------------------------------------------------------------------
  * av_ad_aa_entfernen()
  * 
  * Entfernt aus dem Befehlsstring Befehle, die innerhalb eines AD-
  * Befehls nicht zulaessig sind.
  ---------------------------------------------------------------------------*/
char *STAAT::av_ad_aa_entfernen(char *text)
{
  static char antwort[1000];

  char *dup = mystrdup(text);
  antwort[0] = 0;
  char *befehl = strtok(dup, " \t\n\012\015");
  while (befehl) {
    if (mystrncmp_no_case(befehl,L("AA","CC"),2)
       && mystrncmp_no_case(befehl,L("AV","UA"),2)
       && mystrncmp_no_case(befehl,L("AD","DA"),2))
    {
      strcat(antwort,befehl),
      strcat(antwort," ");
    }
    befehl = strtok(NULL, " \t\n\012\015");
  }
  if (antwort[0]) antwort[strlen(antwort)-1] = 0;
  myfree(dup);
  return antwort;
}


/**---------------------------------------------------------------------------
  * STAAT::abkuerzung_expandieren()
  * 
  * Ereilt einem Objekt alle Befehle, die eine bestimmte Abkuerzung
  * festlegt. Wird vom Befehl AV aufgerufen.
  ---------------------------------------------------------------------------*/
void STAAT::abkuerzung_expandieren(OBJEKT *objekt, char kennung)
{
  char *befehlszeile = abkuerzung_abfragen(kennung);
  if (!befehlszeile) return;
  
  // Und nun Befehl fuer Befehl holen und einlasten. Das Problem ist,
  // dass ich sie in umgekehrter Reihenfolge vorschieben muss...
  
  char *dup = mystrdup(befehlszeile);
  char *befehl[100]; // Maximal 100 Befehle
  short anzahl_befehle = 0;
  
  befehl[anzahl_befehle] = strtok(dup, " \t\n\012\015");
  while (befehl[anzahl_befehle]) {
    anzahl_befehle++;
    befehl[anzahl_befehle] = strtok(NULL, " \t\n\012\015");
  }
    
  // Und nun alle rueckwaerts wieder vorschieben...
  
  while(anzahl_befehle) {
    anzahl_befehle--;
    if (befehl[anzahl_befehle][0] != 0)
      objekt->befehl_vorschieben(befehl[anzahl_befehle]);
  }
  myfree(dup);
}


/**---------------------------------------------------------------------------
  * STAAT::info()                         // virtuell
  ---------------------------------------------------------------------------*/
char *STAAT::info(char *info, void *par1, void *, void*)
{

  if      (!mystrcmp("HAT_ENTWICKLUNG",info)) return info_voraussetzungen_erfuellt(par1);
  else if (!mystrcmp("EINWOHNERZAHL",info)) return info_einwohnerzahl();
  else if (!mystrcmp("BEVOELKERUNGSWACHSTUM",info)) return info_bevoelkerungswachstum();
  else if (!mystrcmp("STAEDTEZAHL",info)) return info_staedtezahl();
  else if (!mystrcmp("STAND_DER_FORSCHUNG",info)) return info_stand_der_forschung();
  else if (!mystrcmp("BEKANNTE_WELT",info)) return info_bekannte_welt();
  else if (!mystrcmp("OFFENSIVKRAFT",info)) return info_offensivkraft();
  else if (!mystrcmp("ROHSTOFFPRODUKTION",info)) return info_rohstoffproduktion();
  else if (!mystrcmp("BAU_UND_ENTWICKLUNG",info)) return info_bau_und_entwicklung();
  else if (!mystrcmp("FRIEDLICHKEIT",info)) return myltoa(anzahl_angriffe);
  else if (!mystrcmp("RELATIVE_ADRESSE",info)) return info_relative_adresse(par1);
  else if (!mystrcmp("ABSOLUTE_ADRESSE",info)) return info_absolute_adresse(par1);
  else if (!mystrcmp("EINDEUTIGER_NAME",info)) return eindeutiger_name();
  else if (!mystrcmp("UNTERSUCHE",info)) return info_untersuche(par1);
  else if (!mystrcmp("PUNKTE",info)) return info_punkte();
  else {
      log('I', "STAAT::info(\"%s\") unbekanntes Info!", info);
      return NULL;
  }
}


/**---------------------------------------------------------------------------
  * STAAT::info_punkte()
  * 
  * Wird von der Uhr angefordert und teilt ihr mit, ueber wieviele
  * zahlende Punkte der Staat verfuegt.
  ---------------------------------------------------------------------------*/
char *STAAT::info_punkte()
{
  return myltoa(myatol(attribut("PUNKTE")));
  //  + einfluss_aufsummieren("BONUSPUNKTE"));  
}

/**---------------------------------------------------------------------------
  * STAAT::info_relative_adresse()
  * 
  * Berechnet aus einer absoluten Adresse, die Koordinatenangaben, die
  * der Spieler erhaelt. Dessen Koordinaten werden naemlich immer
  * vom Mittelpunkt des Reiches (STAAT::mittelpunkt) aus angegeben.
  * Deshalb sind auch negative Werte moeglich. Da die Klasse RIC
  * aber eben genauso einen Wert enthalten kann, wird sie als
  * Rueckgabewert verwendet.
  *
  * Als Zweite Funktion kann aber auch der Absolute Mittelpunkt
  * des Reiches ermittelt werden, wenn keine weiteren Parameter
  * angegeben werden.
  *
  * @param
  * (ADR *)     Absolute Adresse.
  *
  * @return
  * (char *)(RIC *) Zeiger auf statische RIC-Struktur mit
  * dem berechneten Vektor, bei Fall1
  * (char *)(ADR *) Zeiger auf absoluten Mittelpunkt des Reiches
  * in Fall 2.
  ---------------------------------------------------------------------------*/
char *STAAT::info_relative_adresse(void *par1)
{
  if (!par1) return (char *)&mittelpunkt;

  static RIC ric("");
  if (!welt()) return (char *)&ric; // Fatal!

  ADR absolut = *(ADR *)par1;
  ric = welt()->richtung_von_nach(mittelpunkt, absolut);
  return (char *)&ric;
}


/**---------------------------------------------------------------------------
  * STAAT::info_absolute_adresse()
  * 
  * Berechnet aus einer fuer den Spieler subjektiven Adressangabe die
  * objektive (absolute) Adresse.
  *
  * @param
  * char *rel:      Relative (subjektive) Adresse in Stringform
  *
  * @return
  * char *:         Absolute Adresse als statischer String
  ---------------------------------------------------------------------------*/
char *STAAT::info_absolute_adresse(void *par1)
{
  ADR absolut((char *)par1); // Wird hoffentlich nicht gleich umgewrappt.
  absolut.addiere(mittelpunkt);
  welt()->wrap(&absolut);
  return absolut.to_string();
}


/**---------------------------------------------------------------------------
  * STAAT::info_untersuche()
  * 
  * Wird von der Hauptstadt fuer eine Einheit angefordert, die UN auf sie
  * macht. Der Parameter gibt an, welche Art von Informationen
  * geholt werden soll. Es ist ein Zeiger auf eine long-Zahl.
  ---------------------------------------------------------------------------*/
char *STAAT::info_untersuche(void *)
{
  return ""; // Stillgelegt.
}

/**---------------------------------------------------------------------------
  * STAAT::info_bevoelkerungswachstum()
  * 
  ---------------------------------------------------------------------------*/
char *STAAT::info_bevoelkerungswachstum()
{
   long alte_ez = myatol(attribut("EINWOHNERZAHL"));
   long ez = myatol(info_einwohnerzahl());
   attribut_setzen("EINWOHNERZAHL", myltoa(ez));
   if (!alte_ez) return "0";
   else return myftoa( (100*float(ez-alte_ez)) / alte_ez );
}


/**---------------------------------------------------------------------------
  * STAAT::info_einwohnerzahl()
  * 
  * Ermittelt die Anzahl der Einwohner im gesamten Staat, wobei die
  * Einwohner, die in den Einheiten stecken, mitgezaehlt werden.
  ---------------------------------------------------------------------------*/
char *STAAT::info_einwohnerzahl()
{
  // Die Einwohnerzahl setzt sich aus den Einwohner in der Staedten
  // und den Einwohnern zusammen, die sich in Einheiten befinden.

  long ez = 0;

  FOR_EACH_OBJEKT_IN (alle_staedte()) DO_AND_DELETE
  (
    ez += myatol(objekt->info("EINWOHNERZAHL"));
  )

  FOR_EACH_OBJEKT_IN (alle_einheiten()) DO_AND_DELETE
  (
    ez += myatol(objekt->attribut("EINWOHNER"));
  )

  return myltoa(ez);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
char *STAAT::info_staedtezahl()
{
  DOUBLIST *staedte = alle_staedte();
  char *antwort = myltoa(staedte->number_of_elements());
  delete staedte;
  return antwort;
}


/**---------------------------------------------------------------------------
  * STAAT::info_stand_der_forschung()
  * 
  * Diese Info liefert eine Punktewertung fuer den Stand der Forschung.
  * Momentan zaehle ich einfach alle Entwicklungen.
  ---------------------------------------------------------------------------*/
char *STAAT::info_stand_der_forschung()
{
  long anzahl = entwicklungen.number_of_elements();
  return myltoa(anzahl);
}


/**---------------------------------------------------------------------------
  * STAAT::info_bekannte_welt()
  * 
  * Gibt an, wieviele Felder der Weltkarte der Spieler schon kennt.
  * Ist daher maximal breite * hoehe von 2D_MATRIX_ALTAS der Welt.
  ---------------------------------------------------------------------------*/
char *STAAT::info_bekannte_welt()
{
  if (!welt()) return "0"; // Huch?

  return myltoa(welt()->landschaftsabbild_auszaehlen(this));
}


/**---------------------------------------------------------------------------
  * STAAT::info_offensivkraft()
  * 
  * Untersucht alle Einheiten des Staates und berechnet einen Wert
  * fuer die gesamte Offensivkraft, indem von jeder Einheit das Info
  * OK addiert wird.
  ---------------------------------------------------------------------------*/
char *STAAT::info_offensivkraft()
{
  long offensivkraft = 0;
  FOR_EACH_OBJEKT_IN (alle_einheiten()) DO_AND_DELETE
  (
    offensivkraft += myatol(objekt->info("OFFENSIVKRAFT"));
  )

  return myltoa(offensivkraft);
}


/**---------------------------------------------------------------------------
  * STAAT::info_rohstoffproduktion()
  * 
  * Anzahl der in einer Runde im ganzen Staat gefoerderten Rohstoffe.
  * (H,S,M,F,K)
  ---------------------------------------------------------------------------*/
char *STAAT::info_rohstoffproduktion()
{
  long rsp = 0;
  FOR_EACH_OBJEKT_IN (alle_staedte()) DO_AND_DELETE
  (
    rsp += myatol(objekt->info("ROHSTOFFPRODUKTION"));
  )

  FOR_EACH_OBJEKT_IN (alle_weltbauten()) DO_AND_DELETE
  (
    rsp += myatol(objekt->info("ROHSTOFFPRODUKTION"));
  )
  
  return myltoa(rsp);
}


/**---------------------------------------------------------------------------
  * STAAT::info_bau_und_entwicklung()
  * 
  * Summierte Bau/Entwicklungsspalten im ganzen Reich.
  ---------------------------------------------------------------------------*/
char *STAAT::info_bau_und_entwicklung()
{
  long summe = 0;
  
  FOR_EACH_OBJEKT_IN(alle_staedte())
  DO_AND_DELETE
  ( summe += myatol(objekt->info("BAU_UND_ENTWICKLUNG")) )
  
  return myltoa(summe);
}


/**---------------------------------------------------------------------------
  * STAAT::info_voraussetzungen_erfuellt()
  * 
  * Testet, ob der Staat eine Reihe von Entwicklungen besitzt, die
  * als Attributstring gegebensein muessen.
  *
  * @param
  * (char *)par1:     Benoetigte Entwicklungen als Attribute. Z.B.
  * "Medizin,Automobilbau"
  *
  * @return
  * "Ja.", falls er sie erfuellt,  NULL, falls nicht.
  ---------------------------------------------------------------------------*/
char *STAAT::info_voraussetzungen_erfuellt(void *par1)
{
  ATTRIBUT_LISTE attr((char *)par1);
  if (attr.passen_in(&entwicklungen)) return "Ja.";
  else return NULL;
}


/**---------------------------------------------------------------------------
  * STAAT::kommando()                     // virtuell
  ---------------------------------------------------------------------------*/
short STAAT::kommando(const char *kom, const void *par1, const void *par2, const void *par3)
{
  if      (!mystrcmp("EINFLUSS",kom)) return kommando_einfluesse_berechnen();
  else if (!mystrcmp("ENTWICKLUNG",kom)) return kommando_neue_entwicklung((void *)par1);
  else if (!mystrcmp("STADT_EROBERT",kom)) return kommando_stadt_einnehmen((void *)par1);
  else if (!mystrcmp("STADT_VERLOREN",kom)) return kommando_stadt_verlieren((void *)par1, (void *)par2);
  else if (!mystrcmp("STADT_ATOMGEBOMBT",kom)) return kommando_stadt_atomgebombt((void *)par1, (void *)par2);
  else if (!mystrcmp("WELTBAUT_ATOMGEBOMBT",kom)) return kommando_weltbaut_atomgebombt((void *)par1, (void *)par2);
  else if (!mystrcmp("WELTBAUT_BESCHOSSEN",kom)) return kommando_weltbaut_beschossen((void *)par1, (void *)par2, (void *)par3);
  else if (!mystrcmp("PUNKTEAUSWERTUNG",kom)) return kommando_punkteauswertung((void *)par1);
  else if (!mystrcmp("GESAMTWERTUNG_PLATZ",kom)) return kommando_gesamtwertung_platz((void *)par1);
  else if (!mystrcmp("ANGRIFF_ZAEHLEN",kom)) return kommando_angriff_zaehlen();
  else if (!mystrcmp("DIPL_STATUS",kom)) return dipl_status_gegenueber((OBJEKT *)(void *)par1);
  else if (!mystrcmp("BONUSPUNKTE",kom)) return kommando_bonuspunkte((void *)par1);
  else if (!mystrcmp("AGGRESSION",kom)) return kommando_aggression((void *)par1);
  else if (!mystrcmp("AUSSTIEG",kom)) return kommando_ausstieg();
  else if (!mystrcmp("INFOS_NEU",kom)) return kommando_infos_neu();
  else return 1;
}


/**---------------------------------------------------------------------------
  * STAAT::kommando_ausstieg()
  * 
  * Teilt dem Staat mit, dass er nun keinen Spieler mehr hat.
  ---------------------------------------------------------------------------*/
short STAAT::kommando_ausstieg()
{
  attribut_setzen("Ausgestiegen");
  alle_befehle_abbrechen();
  FOR_EACH_OBJEKT_IN (alle_objekte_im_besitz())
  DO_AND_DELETE (objekt->kommando("AUSSTIEG"));
  
  attribut_setzen("HERRSCHERNAME",L("Niemand","Nobody"));
  attribut_setzen("TITEL","");
  return 0;
}
  

/**---------------------------------------------------------------------------
  * STAAT::kommando_infos_neu()
  * 
  * Alle noetigen Infos werden neu gedruckt, damit ein anderer Spieler
  * uebernehmen kann.
  ---------------------------------------------------------------------------*/
short STAAT::kommando_infos_neu()
{
  attribut_loeschen("G_Infos");
  attribut_setzen("Entwicklungen_neu"); // Liste aller Entwicklungen
  bekannte_infos.clear();
  spielerinformation("despotis.inf");   // Info in den Ergebnisausdruck
  ATTRIBUT *attr;
  SCAN(&entwicklungen,attr)
  {
    spieler_bekommt_info_ueber(attr->klasse);
    NEXT(attr);
  }
  return 0;
}


/**---------------------------------------------------------------------------
  * STAAT::kommando_aggression()
  * 
  * Teilt dem Staat mit, dass ihm gegenueber von einem anderen Staat
  * ein aggressiver Akt vollzogen wurde. Es wird der diplomatische
  * Status geprueft und, falls er nicht endgueltig war, auf EG gestellt.
  * @param
  * (OBJEKT *)par1: Fremder Staat, oder ein Objekt von ihm.
  ---------------------------------------------------------------------------*/
short STAAT::kommando_aggression(void *par1)
{
  OBJEKT *sta = (OBJEKT *)par1;
  while (sta && mystrcmp(sta->a_typ(),"STAAT")) sta = sta->besitzer;
  if (!sta) return 1; // Huch? Falsches Objekt
  if (sta == this) return 1; // Das bin ich ja selbst!
  
  // Nun muss ich den Status pruefen. Wenn er nicht vorhanden, oder aber
  // vorlaeufig ist, dann wird er gegnerisch.
  
  char *ds = diplomatie.abfragen(sta->name);
  if (!ds || ds[0]==L('V','P')) { 
    diplomatie.setzen(sta->name,L("EG","FH"));
    report(L("Aufgrund aggressiver Handlungen gegen uns ~andern wir unseren","Due to the agressive actions performed we against us change diplomatic status for"));
    report(L(" diplomatischen Status zu %s auf EG (eng~ultig gegnerisch)!\n"," %s to FH (finally hostile)!\n"),
      sta->a_name());
  }
  return 0;
}

  
/**---------------------------------------------------------------------------
  * STAAT::kommando_bonuspunkte(void *par1)
  * 
  * Fordert eine Zahl von Bonuspunkten an, falls moeglich. Diese Bonus-
  * punkte heissen ungluecklichweise so, haben aber mit der Punkteaus-
  * wertung nicht das geringste zu tun. Sie sind verantwortlich fuer
  * die Unterstuetzung schwacher Spieler durch 15% mehr Arbeitskraft 
  * etc.
  * @return
  * 1, falls nicht genug da.
  * 0, falls Forderung erfuellt und Bonuspunkte verringert.
  ---------------------------------------------------------------------------*/
short STAAT::kommando_bonuspunkte(void *par1)
{
  long gewuenscht = myatol((char *)par1);
  long bonus = myatol(attribut("BONUS"));
  if (bonus >= gewuenscht) {
    bonus -= gewuenscht;
    attribut_setzen("BONUS",myltoa(bonus));
    return 0;
  }
  else return 1;
}


/**---------------------------------------------------------------------------
  * STAAT::kommando_stadt_einnehmen()
  * 
  * Wird von einer gegnerischen Stadt aufgerufen, wenn sie gerade von
  * mir eingenommen wurde. Sie hat schon alles noetige veranlasst, ich
  * muss nur noch einen schoenen Report ausgeben. Diese Funktion wird
  * gleichermassen fuer Weltbauten verwendet.  Beim Aufruf der Funktion
  * muss die Stadt/Weltbaut
  * noch dem urspruenglichen Besitzer gehoeren.
  *
  * Ach ja. Und dann bekommt der Staat noch alle Infos ueber dortige
  * Stadtausbauten, falls er sie nicht schon hat.
  *
  * @param
  * (OBJEKT *)          Eingenommene Stadt oder Weltbaut
  *
  * @return
  * short 0.
  ---------------------------------------------------------------------------*/
short STAAT::kommando_stadt_einnehmen(void *par1)
{
  OBJEKT *opfer = (OBJEKT *)par1;

  if (opfer->typ_ist("STADT")) {
    report(L("Unsere Truppen nehmen die Stadt %s ","Our troops conquer the town %s "), opfer->a_name());
    report(L("des Staates %s ein.\n","of Empire %s.\n"), opfer->besitzer->a_name());

    // Und nun bekommt der Spieler noch die Infos ueber die Stadtausbauten,
    // damit er weiss, was er so erbeutet hat, wenn er noch sehr rueck-
    // staendig ist.

    FOR_EACH_OBJEKT_IN (((STADT *)opfer)->alle_stadtausbauten())
    DO_AND_DELETE (
      spieler_bekommt_info_ueber(objekt->a_gattung());
    )
  }
  else {
    report(L("Unsere Truppen erobern %s.\n","Our troops conquer %s.\n"),
	     konjugation(opfer->a_gattung(), AKKUSATIV | SINGULAR));
  }
  return 0;
}


/**---------------------------------------------------------------------------
  * STAAT::kommando_stadt_atomgebombt()
  * 
  * Der Staat hat eine Stadt weniger.
  ---------------------------------------------------------------------------*/
short STAAT::kommando_stadt_atomgebombt(void *par1, void *par2)
{
  OBJEKT *stadt = (OBJEKT *)par1;
  OBJEKT *silo = (OBJEKT *)par2;
  report(L("%s wird von einer Atomrakete aus ","%s is devastated by a nuclear missile launched by "), stadt->a_name());
  report(L("%s in Schutt und Asche gelegt!\n","%s !\n"), silo->besitzer->a_name());
  if (stadt == hauptstadt()) neue_hauptstadt_waehlen();
  return 0;
}


/**---------------------------------------------------------------------------
  * STAAT::kommando_weltbaut_atomgebombt()
  * 
  * Der Staat hat eine Einrichtung weniger
  ---------------------------------------------------------------------------*/
short STAAT::kommando_weltbaut_atomgebombt(void *par1, void *par2)
{
  OBJEKT *weltbaut = (OBJEKT *)par1;
  OBJEKT *silo = (OBJEKT *)par2;
  report(L("Eine Atomrakete aus %s ","A nuclear missile from  %s "), silo->besitzer->a_name());
  report(L("vernichtet %s ","destroyed %s "), konjugation(weltbaut->a_gattung(),
	 AKKUSATIV | SINGULAR));
  report("(%s).\n", weltbaut->name);
  return 0;
}
  

/**---------------------------------------------------------------------------
  * STAAT::kommando_stadt_verlieren()
  * 
  * Soll dem Staat signalisieren, dass er soeben eine Stadt oder eine
  * Weltbaut verloren hat.  Wenn es sich
  * bei der eingenommenen Stadt um die Hauptstadt gehandelt hat,
  * dann wird eine neue Stadt zur Hauptstadt gemacht.
  * Wenn diese Funktion aufgerufen wird, dann muss dem Staat die
  * Stadt noch gehoeren.
  * Ausserdem ueberprueft der Staat noch den DS.
  *
  * @param
  * (OBJEKT *)          Eingenommene Stadt oder Weltbaut
  * (OBJEKT *)          Angreifer
  *
  * @return
  * short 0.
  ---------------------------------------------------------------------------*/
short STAAT::kommando_stadt_verlieren(void *par1, void *par2)
{
  OBJEKT *opfer = (OBJEKT *)par1;
  OBJEKT *angreifer = (OBJEKT *)par2;
  short opfer_ist_stadt = opfer->typ_ist("STADT");

  // Zuerst mache ich einen report, damit der Spieler weiss, warum er auf ein-
  // mal eine Stadt/Weltbaut weniger hat.

  char string[160];
  if (opfer_ist_stadt)
  {
    sprintf(string, L("Unsere Stadt %s wird von %s des Staates %s eingenommen!\n","Our town %s was conquered by %s of %s !\n"),
	 opfer->a_name(), konjugation(angreifer->a_gattung(),
	 DATIV | SINGULAR), angreifer->besitzer->besitzer->a_name());
    report (string);

    // War's die Hauptstadt?

    if (!mystrcmp(opfer->a_gattung(), L("Hauptstadt","Capital City"))) // Ja.
       neue_hauptstadt_waehlen();
  }

  else  { // Weltbaut
    sprintf(string, L("Der Gegner %s nimmt mit %s %s von uns ein.\n","Our opponent %s conquers with %s our %s.\n"),
       angreifer->besitzer->besitzer->a_name(),
       konjugation(angreifer->a_gattung(), DATIV | SINGULAR),
       konjugation(opfer->a_gattung(), AKKUSATIV | SINGULAR));
    report (string);
  }

  // Und nun den Status ueberpruefen.
  kommando_aggression(angreifer);

  return 0;
}


/**---------------------------------------------------------------------------
  * STAAT::neue_hauptstadt_waehlen()
  * 
  * Waehlt eine andere Stadt als Hauptsstadt.
  ---------------------------------------------------------------------------*/
short STAAT::neue_hauptstadt_waehlen()
{
    STADT *neue_hauptstadt = groesste_stadt_ausser(hauptstadt());
    if (neue_hauptstadt)
    {
	report(L("Wir machen %s zu unserer neuen Hauptstadt.\n"," %s is now our new Capital.\n"), neue_hauptstadt->a_name());
	attribut_setzen("HAUPTSTADT",neue_hauptstadt->a_name());
	attribut_setzen("HAUPTS_ABK",neue_hauptstadt->name);
	neue_hauptstadt->attribut_setzen("GATTUNG",L("Hauptstadt","Capital City"));
	return 1;
    }
    else return 0;
}


/**---------------------------------------------------------------------------
  * Sucht die Groesste Stadt im Reich. Wahlweise kann dabei eine Stadt
  * ausgeschlossen werden.
  ---------------------------------------------------------------------------*/
STADT *STAAT::groesste_stadt_ausser(STADT *ausser)
{
    // Ich nehme die erste in der Liste, da die Staedte
    // ohnehin der Groesse nach geordnet sind.
    
    STADT *antwort;
    DOUBLIST *staedte = alle_staedte();

    OBJEKT_LIST_NODE *onode = (OBJEKT_LIST_NODE *)staedte->first();
    if (onode->is_tail()) antwort = 0; // Garkeine Stadt!!
    else if (ausser && onode->objekt == ausser) {
	onode = (OBJEKT_LIST_NODE *)onode->next();
	if (onode->is_tail()) antwort =  0; // ausser war einzige Stadt
	else antwort = (STADT *)onode->objekt;
    }
    else antwort = (STADT *)onode->objekt;
    delete staedte;
    return antwort;
}


/**---------------------------------------------------------------------------
  * STAAT::kommando_weltbaut_beschossen()
  * 
  * Soll dem Staat signalisieren, dass er soeben eine Weltbaut
  * beschossen wurde.
  * @param
  * (OBJEKT *)          Weltbaut
  * (OBJEKT *)          Angreifer
  * void *par3:             NULL, wenn vernichtet, sonst <> 0.
  *
  * @return
  * short 0.
  ---------------------------------------------------------------------------*/
short STAAT::kommando_weltbaut_beschossen(void *par1, void *par2, void *par3)
{
  OBJEKT *opfer = (OBJEKT *)par1;
  OBJEKT *angreifer = (OBJEKT *)par2;  // Ist ne Einheit
  short vernichtet = (par3 == NULL);

  // Zuerst mache ich einen report, damit der Spieler weiss, warum er auf ein-
  // mal eine Stadt/Weltbaut weniger hat.

  char string[160];
  if (vernichtet) {
    sprintf(string, L("Der Gegner %s vernichtet mit %s %s von uns.\n","The opponent %s destroys with %s our %s.\n"),
       angreifer->besitzer->besitzer->a_name(),
       konjugation(angreifer->a_gattung(), DATIV | SINGULAR),
       konjugation(opfer->a_gattung(), AKKUSATIV | SINGULAR));
    report(string);
  }

  else {
    sprintf(string, L("%s bombardiert %s von uns.\n","%s bombards our %s.\n"),
       angreifer->besitzer->besitzer->a_name(),
       konjugation(opfer->a_gattung(), AKKUSATIV | SINGULAR));
    report(string);
  }

  // Sonst muss ich nichts machen.
  return 0;
}




/**---------------------------------------------------------------------------
  * STAAT::kommando_einfluesse_berechnen()
  ---------------------------------------------------------------------------*/
short STAAT::kommando_einfluesse_berechnen()
{
  // Erstmal loesche ich alle Einfluesse, die von mir ausgehen...
  globale_einfluss_menge.einfluesse_loeschen(this);

  // Nun gehe ich die technischen Entwicklungen durch und setzte fuer
  // jede Entwicklung die Einfluesse

  ATTRIBUT *attr = (ATTRIBUT *)entwicklungen.first();
  while (!attr->is_tail())
  {
    VORHABEN_ENZ *vorhaben;
    if (!(vorhaben = enzyklopaedie()->vorhaben(attr->klasse)))
    {
	log('K', "Missing project '%s' in game configuration file",
	    attr->klasse);
    }
    else {
      beeinflussen(vorhaben->einfluss_art, vorhaben->einfluss_spez,
		   vorhaben->einfluss_parameter);
    }

    attr = (ATTRIBUT *)attr->next();
  }

  // Und jetzt kommen noch die Einfluesse der Staatsform.
  STAATSFORM_ENZ *staatsform
		= enzyklopaedie()->staatsform(attribut("STAATSFORM"));
  if (!staatsform) {
      log('K', "Missing entry '%s' under section 'STAATSFORMEN:' in"
	  "game configuration file", attribut("STAATSFORM"));
  }
  else {
    EINFLUSS_ENZ *einfl = (EINFLUSS_ENZ *)staatsform->einfluesse.first();
    while (!einfl->is_tail()) {
      beeinflussen(einfl->art, einfl->ziel, einfl->par);
      einfl = (EINFLUSS_ENZ *)einfl->next();
    }
  }

  return 0;
}


/**---------------------------------------------------------------------------
  * STAAT::kommando_neue_entwicklung()
  * 
  * An dieser und nur an dieser Stelle bekommt der Stadt im Laufe des
  * Spieles neue Entwicklungen. Er gibt dann einen Report aus und fuegt
  * die neue Entwicklung in die Liste der bestehenden.
  *
  * @param
  * (char *)   Name der neuen Entwicklung, wie sie in der Enzyklopaedie
  * steht.
  *
  * @return
  * 1, wenn sie zum zweiten mal erfunden wurde.
  * 0, sonst.
  ---------------------------------------------------------------------------*/
short STAAT::kommando_neue_entwicklung(void *par1)
{
  char *name = (char *)par1;
  if (entwicklungen.gesetzt(name)) { // Habe ich schon.
    report(L("Unsere Forscher erfinden %s abermals.\n","Our scientists discover %s once again.\n"),
	    konjugation(name,AKKUSATIV|SINGULAR));
    return 1;
  }

  else {
    report(L("Unsere Wissenschaftler entdecken das Geheimnis %s.\n","Our mad scientists discovered the secret of  the %s.\n"),
    konjugation(name, GENITIV_PHRASE));
    entwicklungen.setzen(name);

    // Und jetzt muss ich die Einfluesse neu berechnen
    kommando_einfluesse_berechnen();
    return 0;
  }
}


/**---------------------------------------------------------------------------
  * STAAT::kommando_punkteauswertung
  * 
  * Mit diesem Kommando fordert die Uhr am Ende eines Zuges den Staat
  * auf, eine Punkteauswertung anhand der inzwischen vorliegenden
  * Hitlisten vorzunehmen. Dies kann nicht in zug_abschliessen() des
  * Staates geschehen, da zu diesem Zeitpunkt die Hitlisten noch
  * nicht vorliegen. In der Funktion STAAT::abschlussbericht() sollte
  * es aber auch nicht geschehen, da diese Funktion moeglichst wenig
  * Seiteneffekte haben sollte und ausserdem von jedem Staat schon
  * feststehen muss, wieviele Punkte er hat.
  *
  * @param
  * (DOUBLIST *) Zeiger auf die Liste der Hitlisten
  *
  * @return
  * short 0
  ---------------------------------------------------------------------------*/
short STAAT::kommando_punkteauswertung(void *par1)
{
  DOUBLIST *hitlisten = (DOUBLIST *)par1;

  // Meine alte Punktezahl merke ich mir.
  punkte_letzte_runde = myatol(attribut("PUNKTE"));
  
  long punkte = 0;

  // Jetzt gehe ich alle Eintraege durch und berechne fuer jeden die Punkte
  HITLISTE *hitliste = (HITLISTE *)hitlisten->first();
  while (!hitliste->is_tail())
  {
    punkte += hitliste->punkte_fuer_staat(this);
    hitliste = (HITLISTE *)hitliste->next();
  }

  // das Attribut PUNKTE zeigt an, wieviele Punkte der Staat diese
  // Runde bekommt. Es wird jede Runde wieder ueberschrieben.

  attribut_setzen("PUNKTE",myltoa(punkte));
  return 0;
}


/**---------------------------------------------------------------------------
  * STAAT::kommando_gesamtwertung_platz()
  * 
  * Die Uhr teilt dem Staat ueber dieses Kommando seinen neuen Platz
  * in der Gesamtwertung mit. Der Staat merkt sich zudem seinen alten
  * Platz in der Gesamtwertung fuer den Ergebnisausdruck.
  *
  * @param
  * (long *)par1:   Zeiger auf neuen Platz
  ---------------------------------------------------------------------------*/
short STAAT::kommando_gesamtwertung_platz(void *par1)
{
  gesamtwertung_alt = gesamtwertung;
  gesamtwertung = *(long *)par1;
  return 0;
}


/**---------------------------------------------------------------------------
  * STAAT::kommando_angriff_zaehlen()
  * 
  * Teilt dem Staat mit, dass eine seiner Einheiten einen Angriff
  * durchgefuehrt hat.
  ---------------------------------------------------------------------------*/
short STAAT::kommando_angriff_zaehlen()
{
  anzahl_angriffe++;
  return 0;
}


/**---------------------------------------------------------------------------
  * STAAT::formatieren_und_ausdrucken()
  * 
  * Formatiert eine Asciidatei blockbuendig auf eine bestimmte Breite,
  * und druckt sie aus, je nach 'laser' fuer den Laserdrucker formatiert
  * oder als purer Asciitext.
  * @param
  * char *filename:         Name des Quellfiles
  * long breite:            Zeichen pro Zeile, darf 116 oder 93 sein.
  * char *gn:               Name einer Grafikdatei, falls eine Grafik
  * mitgerduckt werden soll. Sollte eine 48X48
  * Grafik sein, da sonst falsch formatiert wird.
  ---------------------------------------------------------------------------*/
void STAAT::formatieren_und_ausdrucken(char *filename,long breite,char *gn)
{
  if (!laser) breite = 80; // Damit man was lesen kann..
  
  char *formatiert = dr_infodatei_formatieren(filename, breite);
  if (!formatiert)
  {
      log('K', "Missing file '%s' for printout of country '%s'", filename, name);
      return;
  }
  
  if (!laser) drucken(formatiert);

  else {
    long anzahl_zeilen = dr_anzahl_zeilen(formatiert);

    float zhoehe = (breite==116 ? 0.35 : 0.45);
    float add = (breite==116 ? 0.05 : 0.2);
    float oben = (breite==116 ? 0.275 : 0.4);
    float pt = (breite==116 ? 8.063 : 10.066);

    float hoehe = anzahl_zeilen * zhoehe + add;
    
    if (gn && hoehe < 2.0) hoehe = 2.0; // Mindestens so hoch wie die Grafik.

    if (gn) {
      LAYOUT ly(dr_abschnitt(hoehe));
      dr_zeilenabstand(zhoehe);
      ly.text(gn ? 1.8 : 0, oben, formatiert, "Courier", pt);
      ly.bitmatrix(0, 0.2, gn, 4); // Vierfache Vergroesserung
      ly.ausdrucken();
    }
    else { // Zeilenweise ausdrucken, wegen den langen Einheitenberichten.
      dr_abschnitt(add); // Platz lassen
      char *zeilenzeiger = formatiert;
      for (int i=0; i<anzahl_zeilen; i++)
      {
        LAYOUT ly(dr_abschnitt(zhoehe));
        // Jetzt muss ich das Zeilenende finden.
        char *scan = zeilenzeiger;
        while (*scan && *scan != '\n') scan++;
        *scan = 0;
        ly.text(0, oben, zeilenzeiger, "Courier", pt);
        ly.ausdrucken();
        zeilenzeiger = scan+1;
      }
    }
  }
  myfree(formatiert);
}


/**---------------------------------------------------------------------------
  * STAAT::sektionsueberschrift()
  * 
  * Druckt die Ueberschriften zwischen den einzelnen Sektionen.
  * @param
  * char *text:     Text der Sektionsueberschrift
  * float breite... Breite der Ueberschrift in cm bei einer Schrift von 1 pt.
  ---------------------------------------------------------------------------*/
void STAAT::sektionsueberschrift(char *text, float breite_pro_pt)
{
  if (laser) {
      log('p', "Creating for player '%s' printout section '%s'..", name, text);
      LAYOUT balken(dr_abschnitt(0.9));
      char *bname;
      if (zivilisation_vernichtet())                     bname = "totenkop.gra";
      else if (entwicklungen.gesetzt(L("Nahverkehr","Public Transport")))      bname = "eisenb.gra";
      else if (entwicklungen.gesetzt(L("Computer","Computer")))        bname = "platine.gra";
      else if (entwicklungen.gesetzt(L("Astrophysik","Astrophysics")))     bname = "sterne.gra";
      else if (entwicklungen.gesetzt(L("Demokratie","Democracy")))      bname = "mauer.gra";
      else if (entwicklungen.gesetzt(L("Dampfmaschine","Steam Engine")))   bname = "rohre.gra";
      else if (entwicklungen.gesetzt(L("Manufakturwesen","Manufacture"))) bname = "kelche.gra";
      else if (entwicklungen.gesetzt(L("Bildungswesen","Education")))   bname = "buecher.gra";
      else                                               bname = "ranken.gra";
    
      // Nun brauche ich die Breite des Textes in cm. Univers hat eine
      // durchschnittliche Zeichendicke von 0.0161 cm pro pt. Ich verwende
      // momentan 17pt Schrift.
      
      float breite = breite_pro_pt * 17;
      float slinks = balken.breite/2 - breite/2;
      float srechts = balken.breite/2 + breite/2;
      int anzahl = (int)(slinks / 1.355);
      for (int i=0; i<=anzahl; i++) // Um eins ueberlappen.
      {
	  balken.bitmatrix(slinks - float(i+1)*1.354667 - 0.2, 0.1, bname);
	  balken.bitmatrix(srechts + float(i)*1.354667 + 0.2, 0.1, bname);
      }
      balken.rechteck(slinks-0.2, 0.1, srechts+0.2, 0.8);
      balken.rechteck_ausgefuellt(slinks-0.2, 0.1, srechts+0.2, 0.8, 10);
      balken.rechteck(slinks-0.2, 0.1, srechts+0.2, 0.8);
      balken.text(slinks, 0.66, text, "Univers", 17);
      balken.ausdrucken();
  }
  else
   drucken("\n            *********************  %s  *********************\n\n",text);
    
}
  

/**---------------------------------------------------------------------------
  * STAAT::trennlinie()
  * 
  * Macht auf dem Drucker eine duenne Trennlinie und erzeugt dabei auch
  * eine vertikalen Abstand. Wenn die Variable laser auf 0 ist, dann wird
  * garnichts gemacht.
  * @param
  * float abstand: Die Trennlinie wird bei abstand/2 gezeichnet.
  ---------------------------------------------------------------------------*/
void STAAT::trennlinie(float abstand)
{
  if (laser) {
    LAYOUT trenn(dr_abschnitt(abstand));
    trenn.trennlinie(abstand / 2);
    trenn.ausdrucken();
  }
  else drucken("--------------------------------------------------------------------------------\n");
}


/**---------------------------------------------------------------------------
  * STAAT::abschlussbericht()
  * 
  * Erzeugt den kompletten Abschlussbericht fuer einen Staat. Dieser
  * wird nicht sofort ausgedruckt, sondern zunaechst in einer Datei
  * mit der Endung .ead abgelegt. Der Name dieser Datei setzt sich
  * aus der Abkuerzung fuer den Staat (this->name) und der Zugnummer
  * zusammen (%03d). Der Staat Deutschland hat im dritten Zug also
  * wahrscheinlich die Datei deu003.ead.
  *
  * In Abhaengigkeit von der globalen Variablen laser wird der Ausdruck
  * entweder fuer den Laserdrucker formatiert (laser == 1), oder es
  * wird eine Asciidatei erzeugt, die inhaltlich in etwa die gleichen
  * Daten enthaelt und am Bildschirm kontrolliert werden kann.
  *
  * Im Falle der Ausgabe auf dem Laserdrucker wird zusaetzlich eine
  * Druckdatei mit einem Befehlsbogen erzeugt, auf dem alle fuer den
  * Spieler spezifischen Daten schon eingetragen sind. Diese Datei
  * hat die Endung .bfb.
  *
  ---------------------------------------------------------------------------*/
void STAAT::abschlussbericht()
{
  if (ausgestiegen()) return; // Bekommt keinen Ausdruck mehr.

  if (attribut_gesetzt("Html")) {
      HTML::unset_default_background_image();
  }

  // Zum Debuggen noch wichtig:
  io_printxy(0,0,L("Einen kleinen Moment. Das Drucken kann etwas dauern...","One moment please. Printing..."));

  // Statt den Ergebnisausdruck direkt zum Drucker zu schicken,
  // leite ich den Ausdruck in eine Datei um. Wie diese Datei heisst,
  // legt ein Namensschema fest, dass in der Kommandozeile beim Start
  // von Prometheus mit "-d" definiert werden kann.

  char *filename;
  if (laser) filename = g_uhr->printout_dateiname(this, "a");
  else       filename = g_uhr->asciiout_dateiname(this);

  verzeichnis_gewaehrleisten(filename);

  if (drucker_init(filename)) { // Hat nicht geklappt!
      log('W', "Couldn't open printout file '%s' for country '%s'",
	  filename, name);
      return;
   }

  // Der Ergebnisausdruck fuer die Spieler ist in mehrere Sektionen
  // eingeteilt. Bevor es losgeht, muss ich aber die Seitenumbruch-
  // verwaltung starten.

  if (laser) dr_anfang(printout_duplex);

  long zugnummer = myatol(g_uhr->info("ZUGNUMMER"));

  // Nun kommen die Einzelnen Sektionen

  io_deleteline(0);
  io_printxy(0,0,L("Drucke Staat, Staedte, Infos...","Printing Empire, Towns, ..."));

  ausdruck_sektion_titel();      // Ueberschrift
  ausdruck_sektion_spieler();    // Zugnummer, Daten, Adressen...

  ausdruck_sektion_spielende();  // Kommt nur einmal.
  ausdruck_sektion_staat();      // 1. Einige Attribute, Reports
  ausdruck_sektion_staedte();    // 2. Daten und reports
  ausdruck_sektion_weltbauten(); // 3. Weltbauten (Bergwerke...)
  ausdruck_sektion_einheiten();  // 4. Tagebuecher und Reports von den Eh.
  ausdruck_sektion_listen();     // 5  Listen der moeglichen Vorhaben.
  ausdruck_sektion_infos();      // 6. Neue Erkenntnisse
  ausdruck_sektion_gelaendeformen(); // 7. Infos ueber alle Gelaendeformen
  ausdruck_sektion_mitspieler(); // 8. Neu gefundene Mitspieler
  ausdruck_sektion_weltkarte();  // 9. Bekannter Teil der Weltkarte
  ausdruck_sektion_punktewertung(zugnummer);     // 10. Hitlisten und Punkte
  ausdruck_sektion_abkuerzungen();  // Definitionen mit ADA-ADZ
  ausdruck_sektion_befehle(zugnummer);           // 11. Eingetippte Befehle.
  ausdruck_allgemeine_mitteilungen(zugnummer);   // 12. Rundmeldungen
  ausdruck_persoenliche_mitteilungen(zugnummer); // 13. Anmerkungen

  // Jetzt sage ich der Seitenumbruchverwaltung, dass der Druck abgeschlossen
  // ist, und sie die letzte Seite auch noch auswerfen kann.  Anschliessend
  // schliesse ich auch das Druckfile wieder...

  if (laser) dr_auswurf();
  drucker_close();

  // HTML-Seiten erstellen

  if (attribut_gesetzt("Html"))
  {
      ausdruck_hauptindex_html();
      ausdruck_index_html();
      ausdruck_projekte_html();
      ausdruck_diplomatie_html();
      ausdruck_befehle_html();
      ausdruck_abkuerzungen_html();
      ausdruck_mitteilungen_html(zugnummer);
      ausdruck_ps_gesamtkarte(zugnummer);
  }

  reportliste_loeschen(); // Damit die Reports nicht naechste Runde nochmal kommen.
  io_deleteline(0);

  // Und nun kommt der Befehlsbogen. Aber nur, wenn der Spieler nicht gerade
  // 'rausgeflog oder das Spiel zuende ist...
  
  if (laser && !zivilisation_vernichtet() && !g_uhr->spiel_ist_zuende())
      befehlsbogen_drucken(zugnummer);

  // Ich darf den Staat nicht vernichten, sonst wird auch der Spieler aus
  // Liste in der Uhr geloscht, und der Spieler bekommt seinen Ausdruck
  // garnicht mehr! Deshalb kommentiere ich die folgende Zeile momentan aus:
  // if (zivilisation_vernichtet()) zur_vernichtung_vormerken();
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_titel()
  * 
  * Druckt den Titel der Auswertung mit dem Schriftzug "Prometheus".
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_titel()
{
  if (!laser) return;

  // Zuerst muss ich Platz auf der Seite reservieren
  LAYOUT ly(dr_abschnitt(3));

  //  ly.text(3.7,2,"Prometheus","Univers",70);
  ly.bitmatrix(2.28, .2, "prometl.gra",2);
  ly.bitmatrix(9.9, .2, "prometr.gra",2);
  // ly.trennlinie(3.4);

  // ly.text(17.5,2.3,"Classic","Univers",20);
  ly.text(15.8, 3.3, version_programm, "Times",7);
  if (NUR_TESTVERSION) ly.text(4.5, 3.3, L("T E S T V E R S I O N","T E S T V E R S I O N"),"Courier",25);
  ly.ausdrucken();
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_spieler()
  * 
  * Hier kommen Spielorganisatorische Daten wie Adresse der Spielers
  * fuer das Sichtfenster im Brief, sowie der naechste Zugabgabetermin.
  * @param
  * short befbog: Ist 1, wenn es sich um den Befehlsbogen handelt, sonst
  * 0 (default).
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_spieler(short befbog)
{
  // Nun kommen im wesentlichen Spielerspezifische Daten und spezifische
  // Daten ueber den Staat

  long zugnummer = myatol(g_uhr->info("ZUGNUMMER"));

  if (laser) {

    // Die Anschrift des Spielers drucke ich in ein nettes Kaestchen,
    // dass dann gleich der Brieftraeger lesen kann, wenn das Kouvert
    // ein Fenster hat.

    LAYOUT ly(dr_abschnitt(4.6));
    // ly.rechteck(1.6, 1.1, 10.5, 4.5);
    char string[160];

    if (!befbog)
    {
      sprintf(string,"%s %s",attribut("VORNAME"), attribut("NACHNAME"));
      ly.text(2.1, 1.6, string, "Univers",16);
      ly.text(2.1, 2.2, attribut("STRASSE"), "Univers",16);
      ly.text(2.1, 3.3, attribut("WOHNORT"), "Univers",18);
    }
    
    // Beim Befehlsbogen kommt die Anschrift des Organisators aus
    // dem Lexikon. Der Eintrag lautet BB_ORGANISATOR
    
    else {
      sprintf(string,"%s %s",lexikon_eintrag("BB_ORGANISATOR",1)
	   ,lexikon_eintrag("BB_ORGANISATOR",2));
      ly.text(2.1, 1.6, string, "Univers", 16);
      ly.text(2.1, 2.2, lexikon_eintrag("BB_ORGANISATOR",3), "Univers", 16);
      ly.text(2.1, 3.3, lexikon_eintrag("BB_ORGANISATOR",4), "Univers", 16);

      // Ausserdem den Namen des Spielers
      sprintf(string, "%s %s", attribut("VORNAME"), attribut("NACHNAME"));
      ly.text(11.0, 3.9, string, "Univers", 12);
    }  

    // Jetzt kommen noch Angaben wie Zugnummer, Kontonummer, heutiges
    // Datum..

    sprintf(string, L("Kundennummer: %s","Customer number: %s"), attribut("KONTONR"));
    ly.text(14.25, 3.3, string,"Univers",14);

    // Zugnummer in Kaestchen schreiben. Im Falle des Befehlsbogens addiere
    // ich eins zur Zugnummer, da es ja der Bogen fuer den naechsten Zug ist.
    // Sonst kommt der Eintipper noch durcheinander!
    
    const float zugl = 11.0, zugo = 1.2;
    ly.rechteck(zugl,zugo,zugl+2,zugo+1.3);
    ly.linie(zugl,zugo+0.4,zugl+2,zugo+0.4);
    ly.text(zugl+0.60,zugo+0.3,L("Runde","Turn"),"Times",8);
    sprintf(string,"%2ld", zugnummer + (befbog != 0));
    ly.text(zugl + 0.85 -(strlen(string)*0.22), zugo+1.13, string, "Univers",25);

    // Name der Partie in Kaestchen schreiben
    const float kontol = 13.1, kontoo = 1.2;
    ly.rechteck(kontol,kontoo,kontol+2.5,kontoo+1.3);
    ly.linie(kontol,kontoo+0.4,kontol+2.5,kontoo+0.4);
    ly.text(kontol+1.0,kontoo+0.3,L("Partie","Game"),"Times",8);
    ly.text(kontol+0.17,kontoo+1.15,g_uhr->info("SESSIONNAME"),
     "Univers",25);

    // Und jetzt kommt noch ein Kaestchen fuer den naechsten Annahmetermin.
    // der ist immerhin ziemlich wichtig
    const float abl = 15.7, abo = 1.2;
    ly.rechteck(abl,abo,19.8,abo+1.3);
    ly.linie(abl,abo+0.4,19.8,abo+0.4);
    if (befbog) ly.text(abl+1.1,abo+0.3,L("Spieltermin","Deadline"),"Times",8);
    else ly.text(abl+1.2,abo+0.3,L("Spieltermin","Deadline"),"Times",8);
    ly.text(abl+0.05,abo+1.13,g_uhr->info("NAECHSTER_ZUG"),"Univers",25);

    ly.text(zugl, 3.3, name, "Courier",14);

    ly.ausdrucken();
  }
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_spielende()
  * 
  * Prueft, ob das Spiel zuende ist und gibt ggfls. eine Lister der
  * Gewinner aus. Informiert ausserdem die Uhr, dass nun Ende ist.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_spielende()
{
    short alle_freundlich = g_uhr->alle_sind_freundlich();
    short haben_ursprung =  g_uhr->einer_hat_ursprung_des_lebens();

    if (!alle_freundlich && !haben_ursprung) return; // Kein Spielende

    sektionsueberschrift(L("Spielende","End of  Game"), L(.15, .1975));

    // Fuer den Textteil lege ich eine temp-datei an.

    verzeichnis_gewaehrleisten(tempfile_name());
    FILE *file = fopen(tempfile_name(),"w");
    if (!file) {
	log('W', "Couldn't create tempfile '%s' for country '%s'",
	    tempfile_name(), name);
	return; // Kein Ausdruck moeglich.
    }
    
    // Es gibt momentan vier Moeglichkeiten fuer eine Spielende
    
    // FALL1: Ich habe den Ursprung entdeckt.
    if (habe_ursprung_des_lebens()) // Auch wenn ich vielleicht schon vernichtet bin!
    {
	fprintf(file, L("Der Ursprung des Lebens ist gefunden. Nach Jahrtausenden ","The origin of life has been discovered. After milleniums of "));
	fprintf(file, L("von sinnierenden Generationen ist es unserem Staat ","meticulous research our Empire was "));
	if (haben_ursprung == 1) fprintf(file, L("als erstem ","the first one "));
	else fprintf(file, L("und anderen ","( among other empires)"));
	fprintf(file, L("gelungen, das letzte gro~se Geheimnis der Menschheit zu l~uften. ","to solve the very last enigma of human kind. "));
	if (haben_ursprung > 1) fprintf(file, L("Die Siegerstaaten sind %s. ","The victorious Empires are %s. "),
					alle_sieger_bzw_verlierer(1));
	char *mit_dabei = alle_sieger_bzw_verlierer(2);
	if (strlen(mit_dabei) > 0)
	    fprintf(file, L("Au~serdem waren bis zum Ende mit dabei: %s.\n","Still been fighting to their last breath: %s.\n"), mit_dabei);
	else fprintf(file, L("Die anderen Staaten hatten schon vorher aufgegeben.\n","All other Empire were cowards and wimps and surrendered.\n"));
	
	gewinnerdatei_anlegen(2);
    }

    // FALL2: Andere haben den Ursprung entdeckt.
    else if (haben_ursprung) {
	fprintf(file, L("Nach Jahrtausenden von Zivilisationsgeschichte ist es der ","The last enigma of civilisation has been solved ! "));
	fprintf(file, L("Menschheit gelungen, das letzte gro~se Geheimnis nach dem "," The secret of the Origin of Life ! "));
	fprintf(file, L("Ursprung des Lebens zu l~uften. ","Congratulations! "));
	if (haben_ursprung == 1) {
	    fprintf(file, L("Dies ist das Verdienst des Staates %s. ","This is the merit of %s. "), 
		    alle_sieger_bzw_verlierer(1));
	}
	else {
	    fprintf(file, L("Dies ist das Verdienst der Staaten %s. ","This is the merit of %s. "),
		    alle_sieger_bzw_verlierer(1));
	}
	gewinnerdatei_anlegen(2);
    }

    // FALL3: Sieg durch einziges Ueberlegen
    else if (bin_als_einziger_uebrig())
    {
	fprintf(file, L("... Und dann kehrte endlich Ordnung ein auf der Erde. ","... And then , natural order and balance descended to earth. "));
	fprintf(file, L("%s ~ubernahm die alleinige Vorherrschaft ","%s ~ is the one and only leader "),
		attribut(L("NAME","NAME")));
	fprintf(file, L("und sorgte f~ur Ruhe und Ordnung. Endlich war dem ","and brought peace and prosperity to all the nations. "));
	fprintf(file, L("Unwesen der barbarischen V~olker ein Ende gesetzt und ","The great perennial Empire shall come ! "));
	fprintf(file, L("das gro~se tausendj~ahrige Reich nahm seinen Anfang. "," "));
	fprintf(file, L("Ruhm und Ehre sei %s %s.\n","Honored and hallowed be %s %s.\n"), attribut("TITEL"),
		attribut("HERRSCHERNAME"));
	gewinnerdatei_anlegen(1);
    }
  
    // FALL4: Gruppensieg mit mir
    else if (!zivilisation_vernichtet() && alle_freundlich)
    {
	fprintf(file, L("... Und dann kehrte endlich Frieden auf der Welt ein: ","... And then ... eternal peace: "));
	fprintf(file, L("Die Friedenskonferenz von %s. ","The Peace Negotiations of %s. "), irgendeine_stadt()->a_name());
	
	fprintf(file, L("Die Siegerm~achte %s ","The victorious Empires %s "),
		alle_sieger_bzw_verlierer(0));
	fprintf(file, L("unterzeichneten den letzten gro~sen Friedensvertrag. ","signed the final contract. "));
	gewinnerdatei_anlegen(1);
    }

    // FALL5: Einzelsieg ohne mich
    else {
	fprintf(file, L("... Und dann kehrte endlich Frieden auf der Welt ein: ","... And then...  eternal peace: "));
	fprintf(file, L("Die Friedenskonferenz von %s. ","The Peace Negotiations of %s. "), irgendeine_stadt()->a_name());
	fprintf(file, L("Die Siegerstaaten sind: %s","Victorious Empires are: %s"),
		alle_sieger_bzw_verlierer(0));
	gewinnerdatei_anlegen(1);
    }

    fclose(file);
    formatieren_und_ausdrucken(tempfile_name(),93);

    // Gut. Und nun noch ein bisschen Layout, falls Laserdrucker.
  
    if (laser) {
	LAYOUT ly(dr_abschnitt(2));
	if (habe_gewonnen()) ly.text(L(5.9, 7.45), 1.1, L("Du hast gewonnen!","You WON!"),
				     "Times", 30);
	else ly.text(L(4.3, 4.8),1.1,L("Damit ist das Spiel zuende.","The game is over now."),
		     "Times",30);
	ly.ausdrucken();
    }

    // HTML printout
    if (attribut_gesetzt("Html"))
    {
	HTML html(this, "spielend", L("Das Ende des Spiels","End of Game"));
	html.buttonpanel().ueberschrifts_balken(L("Das Ende des Spiels","End of Game"));
	html.text_from_file(tempfile_name());
	if (habe_gewonnen()) {
	    html.paragraph()
		.paragraph()
		.font(2, "#c00000")
		.center()
		.text(L("Du hast gewonnen!","Yes! You WON!!"))
		.end_center()
		.end_font();
	}
    }
    
    
    g_uhr->attribut_setzen("SPIELENDE");

}


/**---------------------------------------------------------------------------
  * STAAT::gewinnerdatei_anlegen()
  * 
  * Erzeugt ein ascii-File, in dem die Liste aller Gewinner steht.
  * Mit Name und Kontonummer.
  * @param
  * short Gewinnmodus:  Wird nicht mehr benutzt.
  ---------------------------------------------------------------------------*/
void STAAT::gewinnerdatei_anlegen(short)
{
  char filename[MAX_LAENGE_DATEINAME];
  sprintf(filename,"%s_gew.txt", g_uhr->info("SESSIONNAME"));
  FILE *file = fopen(filename,"w");
  if (!file)
  {
      log('W', "Could'nt create file '%s' containing the names of the winners",
	  filename);
      return;
  }

  DOUBLIST *staaten = ((UHR *)besitzer)->alle_staaten(true); // Auch mit zivilisation_vernichtet()
  filtere_objekt_liste(staaten, NULL, "Ausgestiegen");
  
  FOR_EACH_OBJEKT_IN (staaten)
  DO_AND_DELETE
  ({
     STAAT *staat = (STAAT *)(objekt);
     if (staat->habe_gewonnen())
     {
       fprintf(file,"%s %s %s\n", staat->attribut("NACHNAME"),
	staat->attribut("VORNAME"),
	staat->attribut("KONTONR"));
     }
  })
  
  fclose(file);
  return;
}  


/**---------------------------------------------------------------------------
  * STAAT::habe_ursprung_des_lebens()
  * 
  * Testet, ob ich schon den Ursprung des Lebens erforscht habe.
  ---------------------------------------------------------------------------*/
short STAAT::habe_ursprung_des_lebens()
{
  return einfluss_vorhanden("URSPRUNG");
}


/**---------------------------------------------------------------------------
  * STAAT::alle_sieger_bzw_verlierer()
  * 
  * Gibt einer List, die eine Auswahl aus den Staaten ist.
  * @param
  * short modus:  0: Alle Staaten, die nicht Ausgestiegen sind.
  * short modus:  1: ... die darueberhinaus den Ursprung des Lebens
  * schon entdeckt haben.
  * short modus:  2: ... die darueberhinaus den Ursprung des Lebens
  * noch nicht entdeckt haben.
  * short modus:  3: Testet, ob nur noch ein Staat dabei ist. Gibt
  * "ja" zurueck, falls ja, sonst NULL.
  * @return
  * Statischer String mit einer Aufzaehlung der Staaten.
  ---------------------------------------------------------------------------*/
char *STAAT::alle_sieger_bzw_verlierer(short modus)
{
    DOUBLIST *erg = ((UHR *)besitzer)->alle_staaten(modus == 1); // Bei modus==1 darf ziv. vern. sein
    filtere_objekt_liste(erg, NULL, "Ausgestiegen");
  
  if (modus == 1 || modus == 2)
  {
    OBJEKT_LIST_NODE *node;
    SCAN(erg, node)
    {
       STAAT *staat = (STAAT *)(node->objekt);
       if ((modus != 1 && staat->zivilisation_vernichtet())
	   || (modus == 1 && !staat->habe_ursprung_des_lebens())
	   || (modus == 2 &&  staat->habe_ursprung_des_lebens()))
       {
	   delete node;
	   FIRST(erg, node);
       }
       else NEXT(node);
    }
  }

  short anzahl = erg->number_of_elements();

  if (modus == 3) return (anzahl == 1 ? (char *)"ja" : (char *)NULL);
  
  static char string[5000];
  string[0] = 0;
  
  while (anzahl) {
    OBJEKT *staat = ((OBJEKT_LIST_NODE *)erg->first())->objekt;
    strcat(string, staat->a_name());
    if (anzahl > 2) strcat(string, ", ");
    else if (anzahl == 2) strcat(string, L(" und "," and "));
    anzahl --;
    delete erg->first();
  }
  delete erg;
  return string;
}


/**---------------------------------------------------------------------------
  * STAAT::irgendeine_stadt()
  * 
  * Gibt irgendeine Stadt auf der ganzen Welt zurueck. Aber am Ende einer
  * bestimmten Runde jedesmal die gleiche.
  ---------------------------------------------------------------------------*/
OBJEKT *STAAT::irgendeine_stadt()
{
  DOUBLIST *staedte = besitzer->alle_objekte_im_besitz("TYP=STADT");
  OBJEKT *stadt = ((OBJEKT_LIST_NODE *)(staedte->first())) -> objekt;
  delete staedte;
  return stadt;
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_staat()
  * 
  * In dieser Sektion kommen alle Daten des Objektes STAAT in seiner
  * Funktion im Spiel, also nicht das organisatorische. Dazu gehoert
  * im wesentlichen ein Bericht, der auch die Reports enthaelt.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_staat()
{

  sektionsueberschrift(L("Staat","Empire"), L(.0794, 0.1094));

  if (laser) {
    LAYOUT ly(dr_abschnitt(0.7));
    char string[200];
    sprintf(string,"%s %s, %s (%s)",attribut("TITEL"),attribut("HERRSCHERNAME"),
      a_name(), name);
    ly.text(0,0.5,string,"Courier",12);
    ly.ausdrucken();
  }

  // Den ganzen Text, der nun kommt, schreibe ich in eine temporaere Datei.
  // Am Ende lasse ich sie formatieren und ausgeben.

  FILE *file = fopen(tempfile_name(),"w");
  if (!file) {
      log('W', "Couldn't create tempfile '%s' for country '%s'",
	  tempfile_name(), name);
    return; // Kein Ausdruck moeglich.
  }

  long zugnummer = atol(g_uhr->info("ZUGNUMMER"));

  // Nun kommt eine kleine Einleitung.
  fprintf(file, L("Die Historiker berichten ~uber die Ereignisse im alten %s","Historians tell us about the events in ancient %s"),a_name());

  long erstes_jahr = jahr_in_zug(zugnummer);
  long letztes_jahr = jahr_in_zug(zugnummer+1);
  char erstes[30], letztes[30];
  if (erstes_jahr < 0) sprintf(erstes,L("%ld vor Christus","%ld BC"), -erstes_jahr);
  else sprintf(erstes,L("%ld AD","%ld AD"),erstes_jahr);
  if (letztes_jahr < 0) sprintf(letztes,L("%ld vor Christus","%ld BC"), -letztes_jahr);
  else sprintf(letztes,L("%ld AD","%ld AD"),letztes_jahr);

  fprintf(file, L(" in den Jahren %s bis %s. "," from the year  %s to %s. "), erstes, letztes);
  fprintf(file, L("Aus dem ewigen Tagebuch des Herrschers %s %s wird zitiert:#\n","Quoting from the diary of the leader %s %s:#\n"),
	attribut("TITEL"), attribut("HERRSCHERNAME"));

  // Und nun zieht der Herrscher noch ein Resume. Wenn der Staat vernichtet
  // ist, dann sieht dass sehr speziell aus.

  if (zivilisation_vernichtet()) {
    report(L("Dies sind wohl die letzten Zeilen meines Tagebuchs.\n","This will be my very last entry.\n"));
    report(L("Das Reich ist zerfallen. ~Uber die Gr~unde und Ursachen","My empire has fallen. About the reasons and causes "));
    report(L(" m~ogen sich die Historiker noch Jahrhunderte sp~ater ","historians will speculate forever, but maybe will never come "));
    report(L("streiten. Fest steht f~ur mich, da~s meine Fehler alleine","to a conclusion. I am convinced that I have been a good and wise leader"));
    report(L(" wohl kaum ausgereicht haben, um so ein Ende herbeizuf~uhren.",".... But I lost the favor of the Gods."));
    report(L(" Aber sei es wie es sei, solange es unsere Erde gibt, werden "," Alas, but as long the earth still turns, ancient empires "));
    report(L("alte Reiche untergehen und neue entstehen...#\n#\n","will fall and new ones will rise...#\n#\n"));

  }

  reportliste_in_file(file);

  // Jetzt kommen die ganzen Reports, die auch im richtigen Stil gehalten sein
  // sollten.

  if (!zivilisation_vernichtet()) { // Staat nicht vernichtet.
    long ez = 1000 * ((myatol(info("EINWOHNERZAHL"))+500)/1000);
    fprintf(file, L("Die Einwohnerzahl im ganzen Reich wird nun ","Estimated entire population is "));
    fprintf(file, L("auf %s gesch~atzt, die vor allem in "," %s. Most of them "), myltoa(ez));

    long anzstaedte = myatol(info("STAEDTEZAHL"));
    if (anzstaedte == 1) fprintf(file, L("der einzigen Stadt leben. ","do live in our only town. "));
    else fprintf(file, L("den %s St~adten des Reiches leben. ","are scattered among our %s towns. "), nice_ltoa(anzstaedte));

    // Hier kommt eine Liste aller Entwicklungen, die der Staat hat.
    // Aber neuerdings nur in der allerersten Runde.

    if (zugnummer == 1 || attribut_gesetzt("Entwicklungen_neu"))
    {
      ATTRIBUT *eattr = (ATTRIBUT *)entwicklungen.first();
      if (entwicklungen.number_of_elements() == 1)
	fprintf(file, L("Wir kennen das Geheimnis ","We discovered the secret of "));
      else fprintf(file, L("Wir kennen die Geheimnisse ","We discovered all the secrets of "));
      while (!eattr->is_tail()) {
	fprintf(file, "%s", konjugation(eattr->klasse, GENITIV_PHRASE));
	if (!eattr->is_last()) {
	  if (eattr->next()->is_last()) fprintf(file, L(" und "," and "));
	  else fprintf(file, ", ");
	}
	eattr = (ATTRIBUT *)eattr->next();
      }
      fprintf(file, ".\n");
      attribut_loeschen("Entwicklungen_neu");
    }

    // Eventuell sind dem Staat neue Entwicklungen moeglich...

    if (!neu_moeglich->is_empty()) {
      fprintf(file, L("Am Vormittag teilt mir der Minister f~ur Wissenschaft und","This morning the Minister of Science and Research told "));
      fprintf(file, L(" Forschung mit, der Staat sei nun in der Lage, "," me that the Empire is now able to "));
      if (neu_moeglich->number_of_elements() == 1) {
	fprintf(file, L("die Erfindung %s in Angriff zu nehmen. ","develop %s. "),
	   konjugation(neu_moeglich->erstes_attribut(), GENITIV_PHRASE));
      }
      else {
	fprintf(file, L("verschiedene neue Gebiete zu erforschen. Er nennt mir ","research several new developments. "));
	fprintf(file, L("vor allem die Gebiete ","Those are "));

	// Die Liste darf ich hier ruhig abbauen.
	while (!neu_moeglich->is_empty()) {
	  fprintf(file, "%s", neu_moeglich->erstes_attribut());
	  switch (neu_moeglich->number_of_elements()) {
	    case 1: fprintf(file, ". "); break;
	    case 2: fprintf(file, L(" und "," and ")); break;
	    default: fprintf(file, ", "); break;
	  }
	  neu_moeglich->loeschen(neu_moeglich->erstes_attribut());
	} // Schleife durch alle neu moeglichen Entwicklugen
      } // Mehr als eine neue Entwicklung
    } // Neue Entwicklungen moeglich

    fprintf(file, L("Die Staatsform ist bis auf weiteres %s.","The Form of Government is still  %s."), attribut("STAATSFORM"));

  } // (Staat ist nicht vernichtet)

  // So. Jetzt muss der ganze Schmarrn noch zum Drucker geschickt werden...
  fclose(file);
  formatieren_und_ausdrucken(tempfile_name(),93);

  if (attribut_gesetzt("Html")) ausdruck_staat_html(tempfile_name());
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_staedte()
  * 
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_staedte()
{
  sektionsueberschrift(L("St~adte","Towns"),L(.0971, 0.1042));

  // Ich muss die Liste der Staedte selbst aufbauen, da unter anderem
  // zur Vernichtung vorgemerkte Staedte noch einen Bericht ausgeben
  // sollen. Ausserdem mus ich die Liste noch sortieren.

  DOUBLIST objliste;
  
  // Bevor es losgeht, sortiere ich die Staedte aber noch nach ihrer
  // Groesse

  besitztum.sort(STADT::sortfunction_stadt);

  // Wegen der Erstellung der HTML-Seiten braucht jede Stadt einen
  // Zeiger auf die letzte und auf die naechste Stadt.

  OBJEKT *stadt, *letztestadt=NULL;
  SCAN(&besitztum, stadt)
  {
    if (stadt->typ_ist("STADT")) {
      stadt->abschlussbericht();
      if (attribut_gesetzt("Html")) 
      {
        OBJEKT *naechstestadt = (OBJEKT *)stadt->next();
        while (!naechstestadt->is_tail() && !naechstestadt->typ_ist("STADT"))
          naechstestadt = (OBJEKT *)naechstestadt;
        if (naechstestadt->is_tail()) naechstestadt=NULL;
        ((STADT *)stadt)->abschlussbericht_html(letztestadt, naechstestadt);
        letztestadt = stadt;
      }
    }
    NEXT(stadt);
  }
}

/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_weltbauten()
  * 
  * Ruft die Abschlussberichte der Objekte vom Typ WELTBAUT auf,
  * die dem Staat gehoeren.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_weltbauten()
{
  DOUBLIST *objliste = alle_weltbauten();

  if (attribut_gesetzt("Html")) {
    HTML html(this, "einricht",L("Einrichtungen","Structures"));
    html.buttonpanel()
	.ueberschrifts_balken(L("Einrichtungen","Structures"));
    
    short nicht_erstes = 0;
    FOR_EACH_OBJEKT_IN (objliste)
    DO ({
      if (nicht_erstes++) html.horizontal_rule();
      ((WELTBAUT *)objekt)->abschlussbericht_html(html); 
    });
    if (!nicht_erstes) {
	html.text(L("Du besitzt keine Einrichtungen. Deine Bautrupps k~onnten f~ur dich ","You do not have any structures. Your construction units")
		  L(" welche bauen. Schau doch mal in der Spielanleitung beim Befehl "," could build some for you. Just consult the instructions for the command "))
	    .href_manual(L("BA","BA"))
	    .text(L("BA","BU"))
	    .end_href()
	    .text(L(" f~ur die Einheiten nach."," (for your units) "));
    }
  }
  
  if (!objliste->is_empty())
  {
      sektionsueberschrift(L("Einrichtungen","Structures"),L(.2088, .1624));
      FOR_EACH_OBJEKT_IN (objliste) DO (objekt->abschlussbericht(); ) ;
  }

  delete objliste;

}
          

/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_einheiten()
  * 
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_einheiten()
{
  sektionsueberschrift(L("Einheiten","Units"),L(.1412, .08825));
  
  if (attribut_gesetzt("Html"))
  {
    HTML html(this, "einheit", L("Einheiten","Units"));
    html.buttonpanel().ueberschrifts_balken(L("Einheiten","Units"));

    short erstes = 0; // Vor dem ersten Eintrag kein Trennstrich
    FOR_EACH_OBJEKT_IN (alle_einheiten_sortiert(true)) // true: Auch tote Einheiten
	DO_AND_DELETE (
	    if (erstes++) html.horizontal_rule();
	    ((EINHEIT *)objekt)->abschlussbericht_html(html);
	    ) ;
  }

  short erstes = 0; // Vor dem ersten Eintrag kein Trennstrich
  FOR_EACH_OBJEKT_IN (alle_einheiten_sortiert(true)) // true: Auch tote Einheiten
  DO_AND_DELETE (
    if (erstes++) trennlinie();
    objekt->abschlussbericht();
  )
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_stadtinfos()
  * 
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_stadtinfos()
{
  sektionsueberschrift(L("St~adteinformationen","City informations"));
   
  // Ich scanne die gesamte Attributliste staat_und_stadt_infos durch.
  
  short erstes=0; // Vor dem ersten kommt keine Trennlinie
  ATTRIBUT *attr;
  SCAN (&staat_und_stadt_infos, attr)
  {
    if (erstes++) trennlinie();
    OBJEKT *stadt = objekt_mit_namen(attr->klasse);
    if (stadt->typ_ist("STADT")) ausdruck_stadtinfo(stadt, attr->wert);
    NEXT(attr);
  }
}
    

/**---------------------------------------------------------------------------
  * STAAT::ausdruck_stadtinfo()
  * 
  * Druckt das Stadtinfo fuer eine bestimmte Stadt aus.
  * @param
  * OBJEKT *stadt:      Stadt
  * char *modus:            Legt die Informationsfuelle fest.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_stadtinfo(OBJEKT *stadt, char *modus)
{
  // Ich fprintf-e in ein Tempfile, dass ich am Ende formatiere.
  FILE *file = fopen(tempfile_name(),"w");
  if (!file) return;
  
  stadt->kommando("STADTINFO", modus, file);
  
  fclose(file);
  
  // Und nun das ganze formatiert zum Drucker bringen.
  formatieren_und_ausdrucken(tempfile_name(), 116);

 }

  
  
/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_listen()
  * 
  * Teilt dem Spieler mit, welche Vorhaben sein Staat in der Lage ist,
  * anzugehen (Einheiten, Stadtausbauten, Weltbauten, Erfindungen).
  * Gleichzeitig wird fuer jedes Vorhaben in den Listen auch das Info
  * ausgespuckt, falls der Spieler es nicht schon hat.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_listen()
{
  sektionsueberschrift(L("M~ogliche Projekte","Possible projects"),L(.2735, .2655));

  ATTRIBUT_LISTE *liste;

  // Moegliche Entwicklungen
  liste = liste_jeder_moeglichen("ERFINDUNG",2);
  projekttabelle_drucken(liste);
  delete liste;

  // Stadtausbauten
  liste = liste_jeder_moeglichen("STADTAUSBAU",2);
  projekttabelle_drucken(liste);
  delete liste;

  // Einheiten
  liste = liste_jeder_moeglichen("EINHEIT",2);
  projekttabelle_drucken(liste);
  delete liste;

  // Die Einrichtungen setze ich etwas ab, damit man weiss, dass sie nicht
  // von den Staedten, sondern von den Bautrupps gebaut werden.

  dr_abschnitt(0.2); // Kleinen Zwischenraum lassen.

  liste = liste_jeder_moeglichen("WELTBAUT",2);
  projekttabelle_drucken(liste);
  delete liste;

}

/**---------------------------------------------------------------------------
  * STAAT::projekttabelle_drucken()
  * 
  ---------------------------------------------------------------------------*/
void STAAT::projekttabelle_drucken(ATTRIBUT_LISTE *liste)
{
  #define T "Times",7

  if (!laser) return;
  short zeilen = (liste->number_of_elements() + 2) / 3;
  float hoehe = zeilen * 0.26 + 0.5;
  LAYOUT ly(dr_abschnitt(hoehe+0.2));
  ly.linie(0,0,19.5,0);
  ly.linie(0,hoehe,19.5,hoehe);
  ly.linie(0,0,0,hoehe);
  for (int spalte=0; spalte<3; spalte++)
  {
    float links = spalte * 6.5;
    ly.linie(links + 6.5, 0, links + 6.5, hoehe);
    ly.text(links+0.2, 0.25, L("Abk.","Abbr."), T);
    ly.text(links+0.9, 0.25, L("Projekt","Project"), T);
    ly.text(links+3.3, 0.25, L("Kosten","Cost"), T);
    ly.text(links+4.8, 0.25, L("Einwoh.","Inhab."), T);
    ly.text(links+5.6, 0.25, L("Unterh.","Upkeep."), T);
  }
  
  char abk[5], vname[80], baukosten[50], einwohner[20], unterhalt[50];
  short nummer = 0;
  while (!liste->is_empty()) {
    char *attr = liste->erstes_attribut();
    sscanf(attr, "%s %s %s %s %s", abk, vname, baukosten, 
	einwohner, unterhalt);

    float oben = (nummer % zeilen) * 0.26 + 0.65;
    float links = nummer / zeilen * 6.5 + 0.1;
    ly.text(links+0.1, oben, abk, T);
    ly.text(links+0.8, oben, vname, T);
    ly.text(links+3.2, oben, baukosten, T);
    ly.text(links+4.7, oben, einwohner, T);
    ly.text(links+5.5, oben, unterhalt, T);
    nummer++;
    liste->loeschen(attr);
  }
    
  ly.ausdrucken();
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_infos()
  * 
  * Druckt alle Infotexte, die der Spieler in der Runde neu erhalten hat.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_infos()
{
  sektionsueberschrift(L("Infos","Infos"),L(.0728, .0728));

  // Sind ueberhaupt neue Infos bekannt geworden?
  if (neue_infos.is_empty()) {
    if (laser) {
      LAYOUT ly(dr_abschnitt(0.5));
      ly.text(0,0.4,L("Keine neuen Infos.","No new infos."),"Courier",10);
      ly.ausdrucken();
    }
    else drucken(L("Keine neuen Infos.\n","No new infos.\n"));
    return;
  }

  while (!neue_infos.is_empty()) {
    char *infoname = neue_infos.erstes_attribut();
    info_drucken(infoname);
    bekannte_infos.setzen(infoname);
    neue_infos.loeschen(infoname);
    if (!neue_infos.is_empty()) trennlinie(0.3);
  }
}


/**---------------------------------------------------------------------------
  * STAAT::info_drucken()
  * 
  * Druckt das Info mit einem bestimmten Namen. Haengt vorne an den Info-
  * namen den Pfadnamen an, wie er in der Uhr festgelegt ist.
  *
  * Wenn HTML gesetzt ist, dann wird ausserdem ein HTML-File.
  *
  * @param
  * char *infoname:  (relativer) Dateiname des Infos.
  ---------------------------------------------------------------------------*/
void STAAT::info_drucken(char *infoname)
{
  HTML *html = NULL;

  // Bei manchen Infos soll noch eine Grafik mitgedruckt werden.
  // Ich Frage die Enzyklopaedie, ob dies der Fall ist.
  
  char *grafikname = g_enzyklopaedie->info_grafik(infoname);

  // Dateiname ergibt sich aus der Option --infos und dem Variablensymbol
  // in infoname

  char *infopfad = g_uhr->infodateiname(infoname);

  if (attribut_gesetzt("Html"))
  {
      // Die erste Zeile der Infodatei enthaelt den Namen des Projektes.
      // Den behandle ich gesondert.
      FILE *file = fopen(infopfad, "r");
      if (!file) {
	  log('W', "Can't open infofile %s. HTML-printout of player %s incomplete",
	      infopfad, name);
      }
      else {
	  char titel[512];
	  fgets(titel, 511, file);
	  while (titel[0] && (titel[strlen(titel)-1] == '#'
	      || titel[strlen(titel)-1] == '\r'
	      || titel[strlen(titel)-1] == '\n')) titel[strlen(titel)-1] = 0;
	  
	  char *htmlname = mystrdup(infoname);
	  htmlname[strlen(htmlname)-4]=0; // .inf wegmachen
	  html = new HTML(this, htmlname, titel, true); // true = Unterverzeichnis infos/
	  myfree(htmlname);
	  
	  char *gifname = NULL;
	  if (grafikname)
	  {
	      gifname = mystrdup(grafikname);
	      gifname[strlen(grafikname+4)] = 0; // .gra wegmachen
	  }
	  
	  html->buttonpanel(false, 0, 0, "../index");
	  html->ueberschrifts_balken(titel).paragraph();
	  if (grafikname) 
	  {
	      html->set_table_border(0).set_cell_valignment(VAL_TOP)
		  .table().next_row().next_cell().fullimage(gifname).next_cell()
		  .text(file).end_table();
	      myfree(gifname);
	  }
	  else html->text(file);
	  fclose(file);
      }
  }
  
  formatieren_und_ausdrucken(infopfad, grafikname ? 84 : 93, grafikname);
 

  // So. Nun kommt der zweite Abschnitt des Infotextes. Dieser umfasst
  // eine automatisch generierte Zusatzinformation der Enzyklopaedie.

  char *zusatzinfo = g_enzyklopaedie->infotext_zusatzinfo(infoname);
  if (zusatzinfo) {
    // Ich muss ihn nun formatieren. Dies mache ich in einer etwas kleineren
    // Schrift, so wie die von den Einheitentagebuechern.

    FILE *tempfile = fopen(tempfile_name(), "w");
    fprintf(tempfile, zusatzinfo);
    myfree(zusatzinfo);
    fclose(tempfile);
    formatieren_und_ausdrucken(tempfile_name(), 116);
    if (html) html->paragraph().italic().text_from_file(tempfile_name());
  }
  if (html) delete html;
}   


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_gelaendeformen()
{
  if (attribut_gesetzt("G_Infos")) return; // Schon erhalten.
  attribut_setzen("G_Infos");

  sektionsueberschrift(L("Gel~andeformen-Infos","Territory infos"),L(.3253, .2196));
 
  if (attribut_gesetzt("Html"))
  {
      HTML html(this, "gelaende", L("Gel~andeformen","Territories"), true); // true: Unterverzeichnis infos/
      html.buttonpanel(false, 0, 0, "../index")
	  .ueberschrifts_balken(L("Gel~andeformen","Territories"));
      welt()->gelaendeform_infos_html(html);
  }

  if (laser) welt()->gelaendeform_infos();
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_mitspieler()
  * 
  * In dieser Sektion erhaelt der Spieler Adressen von anderen Mit-
  * spielern.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_mitspieler()
{

  short erster_ausdruck = 1; // Wegen der Ueberschrift.

  // Zuerst uebertrage ich alle neuen Mitspieler in die Infoliste
  ATTRIBUT *attribut;
  SCAN(&neue_mitspieler, attribut)
  {
    bekannte_infos.setzen(attribut->klasse);
    NEXT(attribut);
  }
    
  // Und nun gehe ich die Liste aller bekannten Infos durch und suche
  // nach STAATstx - Eintraegen.

  SCAN(&bekannte_infos, attribut)
  {
    char *infoname = attribut->klasse;
    if (!strncmp(infoname, "STAAT", 5)) { // Handelt sich um Staat
      STAAT *staat = (STAAT *)objekt_mit_namen(infoname+5);

      if (staat) // Vielleicht aus dem Spiel genommen?
      {
	if (erster_ausdruck) {
	  sektionsueberschrift(L("Mitspieleradressen","Known Adresses"),L(.2882, .2613));
	  erster_ausdruck = 0;
	}
	
	LAYOUT ly(laser ? dr_abschnitt(0.92) : 0.0);

	char string[500], *s;
	dipl_status_gegenueber(staat); // setzt eventuell Defaultstatus
	char *status = diplomatie.abfragen(staat->name);
	if (!mystrcmp(status, L("EG","FH")))
	    s = L("Einstufung: GEGNER!","Status: HOSTILE!!");
	else if (!mystrcmp(status, L("VN","PN")))
	    s = L("Einstufung: VORL~AUFIG NEUTRAL","Status: PRELIMINARY NEUTRAL");
	else if (!mystrcmp(status, L("EN","FN")))
	    s = L("Einstufung: ENDG~ULTIG NEUTRAL","Status: FINALLY NEUTRAL");
	else if (!mystrcmp(status, L("VF","PA")))
	    s = L("Einstufung: VORL~AUFIG FREUNDLICH","Status: PRELIMINARY ALLIED");
	else if (!mystrcmp(status, L("EF","FA")))
	    s = L("Einstufung: ENG~ULTIG FREUNDLICH!","Status: FINALLY ALLIED");
	else s = L("Einstufung: UNBEKANNT!", "Status: UNKNOWN!");

	const char *neu = neue_mitspieler.gesetzt(infoname) ? L("NEU: ","NEW: ") : "";

	sprintf(string,L("%s%s %s herrscht in %s (Abk~urzung %s). %s","%s%s %s rules  %s (Abbreviation %s). %s"),
	  neu,
	  staat->attribut("TITEL"), staat->attribut("HERRSCHERNAME"),
	  staat->a_name(), staat->name,s);
	  
	if (laser) ly.text(0,0.36,string,"Courier",8);
	else drucken("%s\n", string);
	  
	// Falls der Spieler nicht ausgestiegen ist, kommt noch seine
	// Adresse.

	if (!staat->attribut_gesetzt("Ausgestiegen"))
	{
	  sprintf(string,L("Spieler: %s %s, %s in %s. Tel.: %s","Player: %s %s, %s in %s. Tel.: %s"),
	      staat->attribut("VORNAME"),staat->attribut("NACHNAME"),
	      staat->attribut("STRASSE"),staat->attribut("WOHNORT"),
	      staat->attribut("TELEFON"));
	  if (laser) ly.text(1,0.73,string,"Courier",8);
	  else drucken("%s\n", string);
	}
	if (laser) ly.ausdrucken();

      } // if (staat)

    } // if (info ist das von einem anderen STAAT, strncmp...

  NEXT(attribut);
  }  // SCAN-Schleife

//  neue_mitspieler.clear(); // Wichtig, wenn ohne Programmende zwei Runden
// auskommentiert wegen HTML   // gespielt werden (kommt nur beim Testen vor).
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_weltkarte()
  * 
  * Druckt die Sektion der Weltkarte. In der Weltkarte werden alle
  * dem Spieler bekannten Felder ausgegeben. Die Weltkarte wird,
  * wenn dies noetig ist, auf mehrere Seiten verteilt. Dies ueber-
  * nimmt jedoch alles die WELT.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_weltkarte()
{
  io_deleteline(0);
  io_printxy(0,0,L("Drucke Weltkarte....","Printing World Map...."));
  sektionsueberschrift(L("Weltkarte","World Map"),L(.1441, .1666));

  // Der Balken wird von der Welt selbst ausgedruckt, damit er jeden
  // Fall auf der gleichen Seite ist, wie die Karte...

  if (!welt()) {
      log('I', "STAAT::ausdruck_sektion_weltkarte():"
	  "Welt \"%s\" nicht gefunden", attribut("WELTNAME"));
    return;
  }

  welt()->landschaftsabbild_ausdrucken(this);
  
  if (attribut_gesetzt("Html")) {
      welt()->weltkarte_html(this);
// Don't create GIF anymore!
//      char *dateiname = HTML::printout_filename(this, "welt.gif");
//      welt()->weltkarte_gif(this, dateiname);
  }
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_ps_gesamtkarte(int)
  *
  * Erzeugt in PS File mit der Weltkarte, damit die HTML Spieler ihre Welt
  * schoen ausdrucken koennen.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_ps_gesamtkarte(int/* zugnummer*/)
{
    if (attribut_gesetzt("PSGesamtkarte") && attribut_gesetzt("Html"))
    {
	log('p', "Creating complete map as PS file for player %s", name);
	char *dateiname = HTML::printout_filename(this, "welt.ps");
	if (drucker_init(dateiname)) { // Hat nicht geklappt!
	    log('W', "Couldn't open file '%s' for output", dateiname);
	    return;
	}
	dr_anfang(false); // PS-Header usw. schreiben. Hier KEIN Duplexdruck

	welt()->landschaftsabbild_ausdrucken(this);
	dr_auswurf();
	drucker_close();
   }
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_punktewertung()
  * 
  * Holt von der Uhr die Liste der Statistiken und gibt dem Spieler
  * die eigene Position in diesen Tabellen wieder.
  *
  * Diese Funktion berechnet verteilt ausserdem die Punkte fuer die
  * zentrale Punktewertung.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_punktewertung(long zugnummer)
{
  // Punkteauswertung nur in bestimmten Runden
  if (!auswertung_in_zug(zugnummer)) return;

  // Die Ueberschrift kommt auf jeden Fall.
  sektionsueberschrift(L("Punktewertung","Rankings"),L(.2276, .1535));

  // Jetzt generiere ich den allgemeinen Text. Der wird naemlich bei
  // laser und ascii gleichermassen ausgegeben.

  FILE *file = fopen(tempfile_name(),"w");
  if (!file) { 
      log('W', "Coudln't create tempfile '%s' for country '%s'",
	  tempfile_name(), name);
      return; // Kein Ausdruck moeglich.
  }

  long punkte = myatol(attribut("PUNKTE"));

  // Bonuspunkte 
  // long bonuspunkte = einfluss_aufsummieren("BONUSPUNKTE");

  fprintf(file, L("Deine Gesamtpunktzahl aus allen Listen liegt bei %ld. ","Your total combined rating is  %ld. "),punkte);
  if (zugnummer >= 3) {
    if (punkte == punkte_letzte_runde)
      fprintf(file, L("Das ist genausoviel wie in der letzten Runde. ","Your rating is still as high as last turn. "));
    else {
      fprintf(file, L("Im Vergleich zur vorigen Runde entspricht das ","In comparison to the last turn  your rating "));
      if (punkte > punkte_letzte_runde) 
	fprintf(file, L("einer Steigerung um %ld Punkte. ","increased by %ld points. "), punkte - punkte_letzte_runde);
      else
	fprintf(file, L("einer Abnahme um %ld Punkte. ","dropped by %ld points. "), punkte_letzte_runde - punkte);
    }
  }
    
  if (gesamtwertung_alt == 0) {
    fprintf(file, L("In der Gesamtwertung steigst du auf Platz %ld ein. "," You start at rank %ld in our Hall of Fame. ")
    , gesamtwertung);
  }
  else if (gesamtwertung == gesamtwertung_alt) {
    fprintf(file, L("In der Gesamtwertung belegst du nach wie vor den %ld. Platz. ","You are still ranked at %ld in the Hall of Fame. ") ,
      gesamtwertung);
  }
  else if (gesamtwertung < gesamtwertung_alt-1) {
    fprintf(file, L("Damit r~uckst du in der Gesamtwertung um %s Positionen ","Thus you jump by %s ranks "),
     nice_ltoa(gesamtwertung_alt - gesamtwertung));
    fprintf(file, L("auf und belegst nun Platz %ld. ","to advance to rank %ld. "), gesamtwertung);
  }
  else if (gesamtwertung == gesamtwertung_alt-1) {
    fprintf(file, L("In der Gesamtwertung kannst du dich um eine Position nach"," You moved up one rank!"));
    fprintf(file, L(" vorne schieben und bist nun "," Your wonderful and new postition: "));
    if (gesamtwertung != 1) fprintf(file, L("auf Position %ld. ","%ld. "), gesamtwertung);
    else fprintf(file, L("an erster Stelle! ","Top Notch! "));
  }
  else if (gesamtwertung == gesamtwertung_alt+1) {
    fprintf(file, L("In der Gesamtwertung f~allst du leider um eine Position","Unfortunately, you drop down one position."));
    fprintf(file, L(" zur~uck. Du bist jetzt auf Rang %ld. "," Current rank:  %ld. "), gesamtwertung);
  }
  else {
    fprintf(file, L("Leider f~allst Du in der Gesamtwertung um %s ","Sadly enough, you crash down %s "),
       nice_ltoa(gesamtwertung - gesamtwertung_alt));
    fprintf(file, L(" Positionen zur~uck und belegst nun Platz %ld.\n"," positions to settle at rank %ld.\n"),
      gesamtwertung);
  }

  // Jetzt noch eine Angabe, wer der beste ist...
  if (gesamtwertung != 1) {
    fprintf(file, L("Der am besten plazierte Staat ist momentan %s. ","The most prosperous and top-ranked Empire (ta-da!) is  %s. "),
     ((OBJEKT *)(g_uhr->info("BESTER_STAAT")))->a_name());
  }

  fclose(file);

  // Zunaechst brauche ich von der Uhr einen Zeiger auf die Tabellen.

  DOUBLIST *hitlisten = (DOUBLIST *)g_uhr->info("HITLISTEN");
  if (!hitlisten) return; // Ebenfalls Huch!

  if (!laser) return; // keine Asciiausgabe

  static const int anzahl = 8;
  static char *namen[anzahl] = {L("Einwohnerzahl","Number of Citizens"),
				L("Offensivkraft","Military Power"),
				L("Forschung","Developments "),
				L("St~adtezahl","Number of Towns"),
				L("Welterforschung","Exploration"),
				L("Rohstoffproduktion","Raw Materials"),
				L("Bau und Entwicklung","Construction and Devel."),
				L("Bev~olkerungswachstum","Population Growth")};
  static short plaetze[anzahl]={6, 6, 6, 9, 9, 9, 9, 9};

  // HTML

  if (attribut_gesetzt("Html"))
  {
      static char *farbe[anzahl] = { "#f0f0f0", "#f0f0f0", "#f0f0f0",
				     "#d0d0d0", "#d0d0d0", "#d0d0d0", "#d0d0d0", "#d0d0d0" };

      HTML html(this, "punkte",L("Punktewertung","Rankings"));
      html.buttonpanel()
	  .ueberschrifts_balken(L("Punktewertung","Rankings"));

      html.center();

      
      for (int n=0; n<anzahl; n++)
      {
	  if (n==0) html.heading(2, L("Hauptlisten","Main Lists"));
	  else if (n==3) html.heading(2, L("Nebenlisten","Secondary Lists"));

	  if (n==0 || n==3 || n==6) {
	      html.set_cell_spacing(0)
		  .set_table_border(0)
		  .unset_table_color()
		  .set_cell_valignment(VAL_TOP)
		  .table()
		  .next_row();
	  }

	  // Erst muss ich mir die Liste mit dem richtigen Namen besorgen.
	  HITLISTE *hitliste = (HITLISTE *)hitlisten->find(namen[n]);
	  if (!hitliste) continue; // Huch?
	  
	  html.next_cell();
	  hitliste->layout_erstellen_html(html, this, plaetze[n], farbe[n]);
	  if (n==2 || n==5 || n==7) html.end_table();
      }
      html.end_center()
	  .paragraph()
	  .text_from_file(tempfile_name());
  }

      // Ab hier gilt alles nur noch fuer den Laserdrucker
  
      // Jetzt lasse ich mir die Listen auf ein handliches Format layouten.
      // Dabei kann ich Breite und Hoehe angeben, sowie die Anzahl der Ein-
      // traege, die ausgegeben werden sollen.
  
      // Dazu habe ich hier ein paar kleine statische Tabellen.

  static float links[anzahl]={0, 6.66, 13.32, 0, 4, 8, 12, 16};
  static float oben[anzahl]={0, 0, 0, 5.8, 5.8, 5.8, 5.8, 5.8};
  static float breite[anzahl]={6.46, 6.46, 6.46, 3.80, 3.80, 3.80, 3.80, 3.8};
  static float hoehe[anzahl]={5.6, 5.6, 5.6, 4.5, 4.5, 4.5, 4.5, 4.5};
  static float titel[anzahl]={
      L(1.6, 0.9), L(1.7, 1.15), L(1.9, 1.525), L(1.2, 0.625),
      L(0.7, 1.075), L(0.6, 0.875), L(0.385, 0.235), L(0.25, 0.575)};
      
  // In Abhaengigkeit davon lege ich Platz fuer das Layout an..
  float basisoben = dr_abschnitt(10.4);

  // Hier kommt die Schleife fuer alle Hitlisten
  for (int n=0; n<anzahl; n++) {
    // Erst muss ich mir die Liste mit dem richtigen Namen besorgen.
    HITLISTE *hitliste = (HITLISTE *)hitlisten->find(namen[n]);
    if (!hitliste) continue; // Huch?

    // Jetzt mache ich mir ein nettes kleines Layoutchen davon...
    LAYOUT *ly = hitliste->layout_erstellen
		(breite[n], hoehe[n], this, plaetze[n], titel[n]);
    if (!ly) continue; // Huch?

    // Nun muss es nur noch an die richtige Stelle
    ly->ausdrucken(links[n], oben[n] + basisoben);
    delete ly;
  }

  // Jetzt kommt noch der Text.
  formatieren_und_ausdrucken(tempfile_name(),93);

  // Uff... Das waere geschafft.
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_abkuerzungen()
  * 
  * Falls der Spieler mit AD Abkuerzungen definiert hat, so werden
  * sie hier angezeigt.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_abkuerzungen()
{
  short titel_schon_gedruckt = 0;
  FILE *file = 0; // Sonst gibt's eine Warnung
  
  for (char kennung = 'A'; kennung <= 'Z'; kennung++)
  {
    char *abktext = abkuerzung_abfragen(kennung);
    if (abktext) {
      if (!titel_schon_gedruckt) {
	titel_schon_gedruckt = 1;
	sektionsueberschrift(L("Definierte Abk~urzungen","Defined Abbreviations"),
			     L(.36,.3385));
	file = fopen(tempfile_name(), "w");
	if (!file) return;
      }
      fprintf(file, L("AV%c = %s#\n","UA%c = %s#\n"), kennung, abktext);
    }
  }

  if (titel_schon_gedruckt) {
    fclose(file);
    formatieren_und_ausdrucken(tempfile_name(),93);
  }
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_sektion_befehle()
  * 
  * Der Spieler erhaelt ein Kleingedrucktes, in dem alle eingetippten
  * Befehle stehen. Unuebersichtlich, aber vollstaendig. Zur Kontrolle
  * reicht es aus, im Zweifelsfall.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_sektion_befehle(long zugnummer)
{
  if (zugnummer == 1) return; // Beim Aufstellungszug eh keine Befehle
  
  sektionsueberschrift(L("Befehle","Commands"),L(.12355, .1747));
  
  char *dateiname = g_uhr->befehlsdateiname(this);
  if (file_exists(dateiname)) 
    formatieren_und_ausdrucken(dateiname, 116);
  else {
    if (laser) {
      LAYOUT ly(dr_abschnitt(1.0));
      ly.text(0,0.6,L("Diese Runde wurden keine Befehle aufgenommen.","No commands received this turn."), "Courier",8.063);
      ly.ausdrucken();
    }
    else drucken(L("Diese Runde wurden keine Befehle aufgenommen.\n","No commands received this turn.\n"));
  }
}



/**---------------------------------------------------------------------------
  * STAAT::ausdruck_allgemeine_mitteilungen()
  * 
  * Druckt die allgemeine Mitteilung an alle Spieler, falls eine
  * existiert (Das heisst eine Datei mit Namen alg%03ld.txt mit
  * der richtigen Zugnummer).
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_allgemeine_mitteilungen(long)
{
  char *filename = g_uhr->allgemeine_mitteilungdateiname();
  if (file_exists(filename) && laser) {
    sektionsueberschrift(L("Allgemeine Mitteilungen","Public Messages"),L(.3707, .2698));
    formatieren_und_ausdrucken(filename, 93);
  }
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_persoenliche_mitteilungen()
  * 
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_persoenliche_mitteilungen(long)
{
  char *filename = g_uhr->persoenliche_mitteilungdateiname(this);
  if (file_exists(filename) && laser) {
    sektionsueberschrift(L("Pers~onliche Mitteilungen","Personal Messages"),L(.3765, .2837));
    formatieren_und_ausdrucken(filename, 93);
  }
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_hauptindex_html()
  * 
  * Erzeugt das HAUPT-HTML-File fuer alle Printouts einer Partie.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_hauptindex_html()
{
    char titel[512];
    sprintf(titel, L("Prometheus - Partie %s - %s","Prometheus - Game %s - %s"), g_uhr->info("SESSIONNAME"), a_name());

    HTML html(g_uhr->htmldateiname(this, "index.htm"), titel);
    html.set_image_border(0);
    html.center()
	.imagewo("i/promklei")
	.end_center();
    
    html.horizontal_rule();
 
    long runde = myatol(g_uhr->info("ZUGNUMMER"));
    char index[512];
    sprintf(index, "%s_%03ld/index", g_uhr->info("SESSIONNAME"), runde);
    
    html.set_font_size(3)
	.table()
	.next_row()
	.next_cell()
	.imagewo("i/anleitun")
	.next_cell()
	.href("a/index")
	.text(L("Spielregeln","Rulebook"))
	.end_href()
	.next_row()
	.next_cell()
	.imagewo("i/welt")
	.next_cell()
	.href(index)
	.text(L("Runde ","Turn "))
	.text(runde)
	.end_table();

    html.ueberschrifts_balken(L("Fr~uhere Auswertungen","Previous evaluations"));

    html.set_font_size(1)
	.set_cell_spacing(0)
	.set_table_width("100%")
	.table();
	
    int umbruch = 0;
    const int pro_zeile = 6;
    
    for (long r=runde-1; r>=1; r--, umbruch++)
    {
	if (umbruch % pro_zeile == 0) html.next_row();

 	sprintf(index, "%s_%03ld/index", g_uhr->info("SESSIONNAME"), r);
	html.next_cell()
	    .href(index)
	    .text(L("Runde ","Turn "))
	    .text(r)
	    .end_href();
    }
    html.end_table();
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_index_html()
  * 
  * Erzeugt das Rundenspezifische index.htm.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_index_html()
{
  long zugnummer = myatol(g_uhr->info("ZUGNUMMER"));
  char titel[512];
  sprintf(titel, L("Prometheus - Partie %s - Runde %ld - %s","Prometheus - Game %s - Turn %ld - %s"),
	  g_uhr->info("SESSIONNAME"), zugnummer, a_name());
  HTML html(this, "index",titel);
  html.set_image_border(0);
  html.center()
      .iconimage("promklei")
      .end_center()
      .right()
      .text(version_programm)
      .end_right();

  if (NUR_TESTVERSION) {
      html.horizontal_rule();
      html.ueberschrifts_balken(L("T E S T V E R S I O N","T E S T V E R S I O N"));
  }

  html.horizontal_rule();
  html.set_table_color("#f0f0f0").set_cell_padding(0)
      .set_table_width("100%").table().next_row().next_cell();
  
  HTML *tabhtml = new HTML(html);
  tabhtml->set_cell_spacing(2).set_table_width("100%").table()
      .set_font_size(2)
      .next_row()
      .next_cell(L("Partie:","Game:"))
      .next_cell(g_uhr->info("SESSIONNAME"))
      .set_cell_alignment(AL_RIGHT)
      .next_cell()
      .text(L("Spieltermin:  ","Deadline:  ")).text(g_uhr->info("NAECHSTER_ZUG"))
      .unset_cell_alignment()
      .next_row()
      .next_cell(L("Runde:","Turn:"))
      .next_cell(g_uhr->info("ZUGNUMMER"))
      .next_row()
      .next_cell(L("Staat:","Empire:")).next_cell(name)
      .set_cell_alignment(AL_RIGHT)
      .next_cell()
      .text(attribut("TITEL")).text(" ").text(attribut("HERRSCHERNAME")).text(L(" aus "," from "))
      .text(a_name())
      .unset_cell_alignment()
      .end_table();
  delete tabhtml;
  html.end_table();

  html.horizontal_rule();
  html.center();
  html.href("../index").text(L("Hauptindex","Main Index"))          .end_href().text(" ").space(2)
      .href("staat")   .text(L("Staat","Empire"))               .end_href().text(" ").space(2)
      .href("einricht").text(L("Einrichtungen","Structures"))       .end_href().text(" ").space(2)
      .href("einheit") .text(L("Einheiten","Units"))           .end_href().text(" ").space(2)
      .href("welt0101").text(L("Weltkarte","World Map"))           .end_href().text(" ").space(2)
      .href("diplomat").text(L("Diplomatie","Diplomacy"))          .end_href().text(" ").space(2);
  if (auswertung_in_zug(zugnummer))
      html.href("punkte")  .text(L("Punktewertung","RANKINGS"))   .end_href().text(" ").space(2);
  html.href("mitteil") .text(L("Mitteilungen","Messages"))        .end_href().text(" ").space(2)
      .href("projekte").text(L("M~ogliche Projekte","Possible projects"))  .end_href().text(" ").space(2)
      .href_info("gelaende").text(L("Gel~andeformen","Territories")) .end_href().text(" ").space(2)
      .href("abkuerz") .text(L("Abk~urzungen","Abbreviations"))        .end_href().text(" ").space(2)
      .href("befehle") .text(L("Befehle dieser Runde","Commands this turn")).end_href().text(" ").space(2);
  if (!g_uhr->spiel_ist_zuende()) html.href("befbogen").text(L("Befehlsbogen","Turnsheet"))
				      .end_href().text(" ").space(2);
  html.href_manual("index") .text(L("Spielanleitung","Rulebook"))      .end_href()
      .end_center()
      .horizontal_rule();

  if (g_uhr->spiel_ist_zuende())
  {
      html.font_size(3)
	  .center()
	  .href("spielend")
	  .text(L("Das Ende des Spiels","End of Game"))
	  .end_href()
	  .end_center()
	  .end_font()
	  .horizontal_rule();
  }
  
  if (zugnummer == 1)
  {
      html.font_size(3)
	  .center()
	  .href_manual(L("kpDieErs","kpFirstt"))
	  .text(L("Hilfe!!","Help!!"))
	  .end_href()
	  .end_center()
	  .end_font()
	  .horizontal_rule();
  }
  
  html.set_table_color("#c0c0c0")
      .set_cell_spacing(0)
      .set_cell_padding(8)
      .table();

  HTML *subhtml = new HTML(html);
  
  subhtml->ueberschrifts_balken(L("St~adte","Towns"));
  subhtml->set_image_border(0).set_cell_alignment(AL_CENTER).set_font_size(-2).set_cell_width(36);

  subhtml->table();
  short spalte = 0;

  // Die Staedte sollen in der richtigen Reihenfolge kommen.
  besitztum.sort(STADT::sortfunction_stadt);

  FOR_EACH_OBJEKT_IN(alle_staedte(true)) // true: Auch tote Staedte liefern.
  DO_AND_DELETE({
    if (spalte++ % 14 == 0) subhtml->next_row();
    subhtml->next_cell();
    subhtml->href_objekt(objekt).smalleimage(objekt->attribut("GRAFIK")).linebreak();
    subhtml->text(objekt->name).end_href();
  })
  subhtml->end_table();
    
  subhtml->ueberschrifts_balken(L("Einrichtungen","Structures"));
  subhtml->table();
  spalte=0;
  FOR_EACH_OBJEKT_IN(alle_weltbauten())
  DO_AND_DELETE({
    if (spalte++ % 14 == 0) subhtml->next_row();
    subhtml->next_cell();
    subhtml->href_objekt(objekt).smalleimage(objekt->attribut("GRAFIK")).linebreak();
    subhtml->text(objekt->name).end_href();
  })
  subhtml->end_table();

  subhtml->ueberschrifts_balken(L("Einheiten","Units"));
  subhtml->table();
  spalte=0;
  FOR_EACH_OBJEKT_IN(alle_einheiten_sortiert(true)) // true: Auch tote Einheiten
  DO_AND_DELETE (
      if (objekt->typ_ist("EINHEIT") && besitzt(objekt))
      {
	  if (spalte++ % 14 == 0) subhtml->next_row();
	  subhtml->next_cell();
	  subhtml->href_objekt(objekt)
	      .smalleimage(objekt->attribut("GRAFIK")).linebreak();
	  subhtml->text(objekt->name).end_href();
      }) ;
  subhtml->end_table();
  delete subhtml;
  html.end_table();
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_staat_html()
  * 
  * Das HTML-File mit den Berichten vom Staat.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_staat_html(char *reports_filename)
{
    HTML html(this, "staat", L("Staat","Empire"));
    html.buttonpanel().ueberschrifts_balken(L("Staat","Empire"));
    
    html.bold();
    html.text(attribut("TITEL"))
	.text(" ").text(attribut("HERRSCHERNAME"))
	.text(", ").text(a_name()).text(" (").text(name).text(")").bold_off()
	.paragraph();
    
    html.text_from_file(reports_filename);

    // Liste aller Entwicklungen mit Links.
    html.paragraph().paragraph();
    html.bold(L("Bisherige Forschungsergebnisse:","Previous developments:"));
    html.paragraph();

    ATTRIBUT *attr = (ATTRIBUT *)entwicklungen.first();
    while (!attr->is_tail())
    {
	g_enzyklopaedie->vorhaben_mit_link(html, attr->klasse);
	if (!attr->is_last()) html.text(", ");
	attr = (ATTRIBUT *)attr->next();
    }
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_projekte_html()
  * 
  * Erzeugt das File, in dem die Liste der moeglichen Projekte
  * ist.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_projekte_html()
{
   #define KOPFZEILENFARBE "#FFFF80"

   HTML html(this, "projekte",L("Liste der m~oglichen Projekte","List of  Projects"));
   html.buttonpanel().ueberschrifts_balken(L("M~ogliche Projekte","Possible Projects"));

   html.set_table_border(1).set_table_color("#f0f0f0").set_cell_spacing(0);

   // Entwicklungen
   
   html.heading(3,L("Entwicklungen","Development:"));
   ATTRIBUT_LISTE *liste = liste_jeder_moeglichen("ERFINDUNG",0);
   html.set_table_border(1).table();
   html.italic().set_row_color(KOPFZEILENFARBE).next_row()
       .next_cell(L("Abk.","Abbr."))
       .next_cell(L("Entwicklung","Development"))
       .next_cell(L("Kosten","Cost"))
       .italic_off().unset_row_color();
//   short spaltenzaehler = 0;
   while (!liste->is_empty())
   {
     html.next_row();
     char *projektname = liste->erstes_attribut();
     VORHABEN_ENZ *vorhaben = g_enzyklopaedie->vorhaben(projektname, this);
     html.next_cell().bold(vorhaben->abk()).next_cell();
     g_enzyklopaedie->vorhaben_mit_link(html, projektname);


     char *baukosten = mystrdup(g_uhr->aktuelle_forschungskosten(
				    vorhaben->name, 
				    vorhaben->benoetigte_resourcen).to_string());

     html.set_cell_alignment(AL_RIGHT)
         .next_cell(baukosten)
         .set_cell_alignment(AL_LEFT);
     myfree(baukosten);
     liste->loeschen(projektname);
   }
   delete liste;
   html.end_table();

   // Stadtausbauten

   html.heading(3, L("Stadtausbauten","Buildings"));
   liste = liste_jeder_moeglichen("STADTAUSBAU",0);
   html.table();
   html.italic().set_row_color(KOPFZEILENFARBE).next_row()
       .next_cell(L("Abk.","Abbr."))
       .next_cell(L("Stadtausbaut","Buildings"))
       .next_cell(L("Kosten","Cost"))
       .next_cell(L("Einwohner","Inhabitants"))
       .next_cell(L("Unterhalt","Upkeep"))
       .italic_off().unset_row_color();
   while (!liste->is_empty())
   {
     char *projektname = liste->erstes_attribut();
     VORHABEN_ENZ *vorhaben = g_enzyklopaedie->vorhaben(projektname, this);
     html.next_row().next_cell().bold(vorhaben->abk()).next_cell();
     g_enzyklopaedie->vorhaben_mit_link(html, projektname);
     html.set_cell_alignment(AL_RIGHT)
         .next_cell(vorhaben->benoetigte_resourcen.to_string())
         .next_cell(myatol(vorhaben->start_attribute.abfragen("EINWOHNER")));
     html.next_cell(vorhaben->unterhalt.to_string())
         .set_cell_alignment(AL_LEFT);
     liste->loeschen(projektname);
   }
   delete liste;
   html.end_table();
   
   // Einheiten
   
   html.heading(3, L("Einheiten","Units"));
   liste = liste_jeder_moeglichen("EINHEIT",0);
   html.table();
   html.italic().set_row_color(KOPFZEILENFARBE).next_row()
       .next_cell(L("Abk.","Abbr."))
       .next_cell(L("Einheit","Unit"))
       .next_cell(L("Kosten","Cost"))
       .next_cell(L("Einwohner","Inhabitants"))
       .next_cell(L("Unterhalt","Upkeep"))
       .next_cell(L("A","O"))
       .next_cell(L("V","D"))
       .next_cell(L("B","M"))
       .next_cell(L("S","S"))
       .next_cell(L("FK","FP"))
       .next_cell(L("SW","FR"))
       .next_cell(L("Kapazit~at","Capacity"))
       .next_cell(L("G~uter","Commodities"))
       .next_cell(L("Konvoi","Convoy"))
       .italic_off().unset_row_color();
   while (!liste->is_empty())
   {
     char *projektname = liste->erstes_attribut();
     VORHABEN_ENZ *vorhaben = g_enzyklopaedie->vorhaben(projektname, this);
     EINHEIT_ENZ *einheit = g_enzyklopaedie->einheit(projektname);
     html.next_row().next_cell().bold(vorhaben->abk()).next_cell();
     g_enzyklopaedie->vorhaben_mit_link(html, projektname);
     html.set_cell_alignment(AL_RIGHT)
         .next_cell(vorhaben->benoetigte_resourcen.to_string())
         .next_cell(myatol(vorhaben->start_attribute.abfragen("EINWOHNER")));
     html.next_cell(vorhaben->unterhalt.to_string())
         .next_cell(einheit->angriffskraft)
         .next_cell(einheit->verteidigungskraft)
         .next_cell(einheit->bewegungspunkte_pro_feld)
         .next_cell(myftoa(einheit->sichtweite, 1)) // eine Nachkommastelle
         .next_cell(vorhaben->start_attribute.abfragen("FEUERKRAFT"))
         .next_cell(vorhaben->start_attribute.abfragen("SCHUSSWEITE"))
         .next_cell(vorhaben->start_attribute.abfragen("KAPAZ"))
         .next_cell(vorhaben->start_attribute.abfragen("LAGER"))
         .next_cell(vorhaben->start_attribute.abfragen("CONVOY"))
         .set_cell_alignment(AL_LEFT);
     liste->loeschen(projektname);
   }
   delete liste;
   html.end_table().paragraph();
   html.bold(L("A","O"))
       .text(L(" = Kampfkraft, "," = Offense , "))
       .bold(L("V","D"))
       .text(L(" = Verteidigungsbonus, "," = Defense Bonus, "))
       .bold(L("B","M"))
       .text(L(" = Bewegungsdauer in Phasen pro Feld, "," = movement cost (phases per move) , "))
       .bold(L("S","S"))
       .text(L(" = Sichtweite in Feldern, "," = Sight Range "))
       .bold(L("FK","FP"))
       .text(L(" = Feuerkraft, "," = Firepower, "))
       .bold(L("SW","FR"))
       .text(L(" = Schussweite, "," = Firing Range, "))
       .bold(L("Kapazit~at","Capacity"))
       .text(L(" und "," and "))
       .bold(L("G~uter","Commodities"))
       .text(L(" gibt an, wieviel und welche "," indicates number and species of")
	     L("G~uter die Einheit transportieren kann, "," commodities unit can transport, "))
       .bold(L("Konvoi","Convoy"))
       .text(L(" gibt an, wieviele andere Einheiten diese Einheit transportieren kann."," indicates the number of units given unit can carry."));

   // Einrichtungen
   
   html.heading(3,L("Einrichtungen","Structures"));
   liste = liste_jeder_moeglichen("WELTBAUT",0);
   html.set_table_border(1).table();
   html.italic().set_row_color(KOPFZEILENFARBE).next_row()
       .next_cell(L("Abk.","Abbr."))
       .next_cell(L("Einrichtung","Structure"))
       .next_cell(L("Unterhalt","Upkeep"))
       .italic_off().unset_row_color();
   while (!liste->is_empty())
   {
     html.next_row();
     char *projektname = liste->erstes_attribut();
     VORHABEN_ENZ *vorhaben = g_enzyklopaedie->vorhaben(projektname, this);
     html.next_cell().bold(vorhaben->abk()).next_cell();
     g_enzyklopaedie->vorhaben_mit_link(html, projektname);
     html.set_cell_alignment(AL_RIGHT)
         .next_cell(vorhaben->unterhalt.to_string())
         .set_cell_alignment(AL_LEFT);
     liste->loeschen(projektname);
   }
   delete liste;
   html.end_table();
}
   

/**---------------------------------------------------------------------------
  * STAAT::ausdruck_diplomatie_html()
  * 
  * Mitspieleradressen und Diplomatische Stati in eine HTML-File.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_diplomatie_html()
{
  HTML html(this, "diplomat",L("Diplomatie","Diplomacy"));
  html.buttonpanel().ueberschrifts_balken(L("Diplomatie","Diplomacy"));

  html.set_table_border(0).set_cell_valignment(VAL_TOP).set_cell_spacing(10);

  short mindestens_einen_getroffen = 0;

  ATTRIBUT *attribut;
  SCAN(&bekannte_infos, attribut)
  {
    char *infoname = attribut->klasse;
    if (!strncmp(infoname, "STAAT", 5)) { // Handelt sich um Staat
      STAAT *staat = (STAAT *)objekt_mit_namen(infoname+5);

      if (staat) // Vielleicht aus dem Spiel genommen?
      {
        if (!mindestens_einen_getroffen++) html.table();

        html.next_row().next_cell().bold(staat->name).next_cell();
        
//	char string[500], *s;
	char *s;
	dipl_status_gegenueber(staat); // setzt eventuell Defaultstatus
	char *status = diplomatie.abfragen(staat->name);
	if (!mystrcmp(status, L("EG","FH")))
	    s = L("GEGNER!","HOSTILE!!");
	else if (!mystrcmp(status, L("VN","PN")))
	    s = L("VORL~AUFIG NEUTRAL","PRELIMINARY NEUTRAL");
	else if (!mystrcmp(status, L("EN","FN")))
	    s = L("ENDG~ULTIG NEUTRAL","FINALLY NEUTRAL");
	else if (!mystrcmp(status, L("VF","PA")))
	    s = L("VORL~AUFIG FREUNDLICH","PRELIMINARY ALLIED");
	else if (!mystrcmp(status, L("EF","FA")))
	    s = L("ENG~ULTIG FREUNDLICH!","FINALLY ALLIED");
	else s = L("UNBEKANNT!","UNKNOWN");

	if (neue_mitspieler.gesetzt(infoname)) html.bold(L("NEU: ","New: "));
        html.text(staat->attribut("TITEL")).text(" ")
            .text(staat->attribut("HERRSCHERNAME")).text(L(" herrscht in "," rules "))
            .bold(staat->a_name()).text(". ").linebreak();

	if (!staat->attribut_gesetzt("Ausgestiegen"))
	{
	  html.text(L("Spieler: ","Player: ")).text(staat->attribut("VORNAME"))
	      .text(" ").text(staat->attribut("NACHNAME")).text(", ")
	      .text(staat->attribut("STRASSE")).text(L(" in "," in "))
	      .text(staat->attribut("WOHNORT"))
	      .text(".")
	      .linebreak();
	  char *telefon = staat->attribut("TELEFON");
	  if (telefon && telefon[0]) html.text(L("Telefon: ","Phone: ")).bold(telefon);
	  else html.text(L("(Telefon unbekannt)","(Number not known)"));

	  html.linebreak();
	  char *email = staat->attribut("EMAIL");
	  if (email && email[0]) html.text(L("email: ","email: ")).email_ref(email).text(email).end_href();
	  else html.text(L("(email unbekannt)","(email unknown)"));
	}
	
	html.next_cell(s);
	
      } // if (staat)

    } // if (info ist das von einem anderen STAAT, strncmp...

  NEXT(attribut);
  }  // SCAN-Schleife
  html.end_table();
  
  if (mindestens_einen_getroffen) html.end_table();
  else html.bold(L("Noch","Still"))
	   .text(L(" ... bist du keinem begegnet."," ... you had no strange encounters."));

}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_befehle_html()
  * 
  * Eine Uebersicht ueber alle eingetippten Befehle.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_befehle_html()
{
  HTML html(this, "befehle",L("Befehle dieser Runde","Commands this turn"));
  html.buttonpanel().ueberschrifts_balken(L("Befehle dieser Runde","Commands this turn"));
    
  char *dateiname = g_uhr->befehlsdateiname(this);
  FILE *file = fopen(dateiname, "r");
  if (!file) {
    html.text(L("Diese Runde wurde keine Befehle aufgenommen!","No commands received this turn!"));
    return;
  }
  
  html.set_table_border(0).table();
  char linebuffer[MAX_LAENGE_BEFEHLSZEILE];
  while (fgets(linebuffer, MAX_LAENGE_BEFEHLSZEILE, file)) {
    html.next_row();
    char *scan = linebuffer;
    while (*scan && *scan!=' ') scan++;
    if (*scan == ' ') {
      *scan = 0;
      // linebuffer zeigt nun auf die Abkuerzung des Objektes,
      // scan+1 zeigt auf die Befehle.
      // Die Abkuerzung wird ein Link auf das Objekt.
      OBJEKT *ob = objekt_mit_namen(linebuffer);
      if (ob) {
	  html.next_cell()
	      .href_objekt(ob)
	      .text(linebuffer)
	      .end_href();
      }
      else html.next_cell().bold(linebuffer);
      html.next_cell(scan+1);
    }
    else html.next_cell().next_cell(scan); // Huch?
  }
  html.end_table();
  fclose(file);
}

  
/**---------------------------------------------------------------------------
  * STAAT::ausdruck_abkuerzungen_html()
  * 
  * Falls der Spieler mit AD Abkuerzungen definiert hat, so werden
  * sie hier angezeigt. Und zwar in Form eines HTML-Files.
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_abkuerzungen_html()
{
    HTML html(this, "abkuerz",L("Definierte Abk~urzungen","Defined abbreviations"));
    html.buttonpanel().ueberschrifts_balken(L("Definierte Abk~urzungen","Definied abbreviations"));
    
    bool war_eine_definiert = false;
    html.set_cell_spacing(3)
	.table();

    for (char kennung = 'A'; kennung <= 'Z'; kennung++)
    {
	char *abktext = abkuerzung_abfragen(kennung);
	if (abktext) {
	    war_eine_definiert = true;
	    html.next_row();
	    html.next_cell()
		.bold()
		.text(L("AV","UA"))
		.text(kennung)
		.bold_off()
		.next_cell(abktext);
	}
    }
    html.end_table();
    if (!war_eine_definiert) {
	html.text(L("Du hast keine Abk~urzungen definiert. Sieh Dir doch einmal ","You have not defined any abbreviations. Have a look at ")
		  L("die Beschreibung der Befehle ","the command description "))
	    .href_manual(L("STAAT_AD","STAAT_AD"))
	    .text(L("AD","DA")).end_href()
	    .text(L(" und "," and "))
	    .href_manual(L("STADT_AV","STADT_AV"))
	    .text(L("AV (St~adte)","UA (Towns)"))
	    .end_href()
	    .text(L(" bzw. "," or "))
	    .href_manual(L("AV","UA"))
	    .text(L("AV (Einheiten)","UA (Units)")).end_href().
	    text(L(" an.","."));
    }
}


/**---------------------------------------------------------------------------
  * STAAT::ausdruck_mitteilungen_html(long)
  * 
  ---------------------------------------------------------------------------*/
void STAAT::ausdruck_mitteilungen_html(long)
{
    HTML html(this, "mitteil", L("Mitteilungen der Spielleitung","Messages from the Game Lords"));
    html.buttonpanel().ueberschrifts_balken(L("Mitteilungen der Spielleitung","Messages from the Game Lords"));

    html.heading(3, L("Allgemeine Mitteilungen","General Messages"));
    char *filename = g_uhr->allgemeine_mitteilungdateiname();
    if (file_exists(filename)) html.text_from_file(filename);
    else html.text(L("(keine Mitteilungen)","(no messages)"));
    
    html.heading(3, L("Mitteilungen pers~onlich an dich","Messages adressed to you personally "));
    filename = g_uhr->persoenliche_mitteilungdateiname(this);
    if (file_exists(filename)) html.text_from_file(filename);
    else html.text(L("(keine Mitteilungen)","(no messages)"));
}


/**---------------------------------------------------------------------------
  * STAAT::liste_jeder_moeglichen()
  * 
  * Scant die Vorhabenliste der Enzyklopaedie durch und prueft, ob der
  * Staat schon in der Lage ist, die Entwicklung in Angriff zu nehmen,
  * bzw. die Einheit oder das Bauwerk zu bauen. Bei den Vorhaben vom
  * Typ ERFINDUNG wird zusaetzlich geprueft, ob der Staat diese Er-
  * findung nicht schon gemacht hat.
  *
  * @param
  * char *typ: "ERFINDUNG", "EINHEIT", "STADTAUSBAU", "WELTBAUT",
  * oder NULL, wenn der Typ egal sein soll
  *
  * short info:           Wenn info > 0, dann bekommt der Spieler
  * auch gleich automatisch das Info ueber
  * das entsprechende Vorhaben.
  * Wenn info > 1, dann wird hinter jedem
  * Eintrag der Attributsliste in Klammern
  * die Abkuerzung fuer das Vorhaben einge-
  * tragen. Ausserdem wird dann eine tabel-
  * larische Ausgabe erzeugt.
  * @return
  * ATTRIBUT_LISTE *: Die Antwort kommt als Attributliste!
  ---------------------------------------------------------------------------*/
ATTRIBUT_LISTE *STAAT::liste_jeder_moeglichen(char *typ, short info)
{
  ATTRIBUT_LISTE *attr = new ATTRIBUT_LISTE;
  if (!enzyklopaedie()) return attr;
  
  bool ist_forschung = !mystrcmp(typ, "ERFINDUNG");

  VORHABEN_ENZ *vorhaben; // Zum Abscannen aller Vorhaben der Enz.
  vorhaben = (VORHABEN_ENZ *)enzyklopaedie()->vorhabenliste.first();
  while (!vorhaben->is_tail()) {
    if (!typ || !mystrcmp(vorhaben->start_attribute.abfragen("TYP"),typ)) {
      if (vorhaben->voraussetzungen_im_staat.passen_in(&entwicklungen)) {
	if (!entwicklungen.gesetzt(vorhaben->name)) // Sonst hat er's schon!
	{
	  if (info) spieler_bekommt_info_ueber(vorhaben->name);
	  if (info > 1) {
	    char string[250];
	    
	    long einwohner = myatol(vorhaben->start_attribute.abfragen("EINWOHNER"));
	    char *abk = vorhaben->start_attribute.abfragen("ABK");
	    
	    // An dieser Stelle kommt die Regel mit den verringerten Forschungspreisen
	    // ins Spiel. Die Baukosten einer Forschung reduzieren sich um den
	    // Anteil der Spieler, welche die Forschung schon haben.
	    
	    
	    char *baukosten;
	    if (ist_forschung) {
		RESOURCE_VEKTOR baukosten_neu = g_uhr->aktuelle_forschungskosten(
					 vorhaben->name, 
					 vorhaben->benoetigte_resourcen);
		baukosten = mystrdup(baukosten_neu.to_string());
		// Merke die Kosten beim Staat. Wichtig ist, dass die Reduktion der
		// Forschungskosten immer nur am Rundenende stattfindet. Das Ergebnis
		// muss ich irgendwo speichern. Am einfachsten geht dies beim Staat
		// im Attribut fuer die jeweilige Forschung.
		if (!attribut_gesetzt(vorhaben->name))
		    attribut_setzen(vorhaben->name, baukosten);
		else // nur setzen, falls neuer Preis kleiner
		{
		    RESOURCE_VEKTOR kosten_bisher(attribut(vorhaben->name));
		    if (baukosten_neu.passt_in(kosten_bisher))
		    {
			char *alt = mystrdup(kosten_bisher.to_string());
			attribut_setzen(vorhaben->name, baukosten);
			char reporttext[1024];
			snprintf(reporttext, 1024, 
				 L("Die Kosten f~ur %s verringern sich von %s auf %s.\n",
				   "The cost for %s is reduced from %s to %s.\n"), 
				 konjugation(vorhaben->name, AKKUSATIV | SINGULAR),
				 alt, baukosten);
			myfree(alt);
		    }
		}
	    }

	    else 
		baukosten = mystrdup(vorhaben->benoetigte_resourcen.to_string());


	    char *unterhalt = vorhaben->unterhalt.to_string();
	    if (mystrlen(unterhalt) < 2) unterhalt = "";

	    sprintf(string, "%s %s %s %ld %s",
	      abk, vorhaben->name, baukosten, einwohner,
	      unterhalt[0] ? unterhalt : " - ");

	    myfree(baukosten);

	    attr->setzen(string);
	  }
	  else attr->setzen(vorhaben->name);
	}
      }
    }
    vorhaben = (VORHABEN_ENZ *)vorhaben->next();
  }
  return attr;
}

/**---------------------------------------------------------------------------
  * STAAT::spieler_bekommt_info_ueber()
  * 
  * Sorgt dafuer, dass der Spieler das Info zu einem bestimmten Vorhaben
  * ("Vorhaben" im Sinne der Enzyklopaedie, also z.B. Einheiten, Entwik-
  * klungen...) bekommt, falls er es nicht schon zuvor bekommmen hatte.
  *
  * @param
  * char *name:     Name des Vorhabens, z.B. "Siedler"
  ---------------------------------------------------------------------------*/
void STAAT::spieler_bekommt_info_ueber(char *name)
{
  if (!enzyklopaedie()) return; // Huch?
  VORHABEN_ENZ *vorenz = enzyklopaedie()->vorhaben(name);
  if (!vorenz) return; // Huch?
  spielerinformation(vorenz->info_datei_filename);
}


/**---------------------------------------------------------------------------
  * STAAT::spielerinformation()
  * 
  * Sorgt dafuer, dass der Spieler im naechsten Ausdruck ein bestimmtes
  * Info bekommt, falls er es nicht schon einmal bekommen hat.
  *
  * @param
  * char *info:     Name der Infodatei, z.B. "siedler.inf".
  ---------------------------------------------------------------------------*/
void STAAT::spielerinformation(char *info)
{
  if (!bekannte_infos.gesetzt(info) && !neue_infos.gesetzt(info))
	neue_infos.setzen(info);
}


/**---------------------------------------------------------------------------
  * STAAT::mitspielerinformation()
  * 
  * Sorgt dafuer, dass der Spieler im naechsten Ausdruck den Namen eines
  * anderen Staates und die Adresse dessen Spielers bekommt, falls er
  * sie nicht schon kennt.
  *
  * @param
  * OBJEKT *staat:      Zeiger auf den fremden Staat, oder auf
  * ein Objekt in dessen Besitz.
  ---------------------------------------------------------------------------*/
void STAAT::mitspielerinformation(OBJEKT *staat)
{
  // Zuerst versuche ich, festzustellen, welchem Staat das Objekt gehoert,
  // falls es nicht selbst schon ein Staat ist.

  while (staat && !staat->typ_ist("STAAT")) staat = staat->besitzer;
  if (staat == this) return; // Das bin ich ja selbst!

  else if (staat) { // Der Staat konnte tatsaechlich ermittelt werden.
    // Der Infoname lautet auf STAAT<name>
    char infoname[20];
    sprintf(infoname,"STAAT%s",staat->name);
    if (!neue_mitspieler.gesetzt(infoname) && !bekannte_infos.gesetzt(infoname))
	neue_mitspieler.setzen(infoname);
  }
}


/**---------------------------------------------------------------------------
  * STAAT::befehlsbogen_drucken()
  * 
  * Erzeugt eine Druckdatei mit einem Befehlsbogen.
  ---------------------------------------------------------------------------*/
void STAAT::befehlsbogen_drucken(long)
{
    if (attribut_gesetzt("Html")) befehlsbogen_html();
    
    // Druckausgabe in Datei umleiten. Der Name ist aehnlich wie der vom
  // Ergebnisausdruck.
  
  char *filename = g_uhr->printout_dateiname(this, "b");
  
  if (drucker_init(filename)) { // Hat nicht geklappt!
      log('W', "Couldn't create file '%s' for command sheet of country '%s'",
	  filename, name);
    return;
  }
  
  dr_anfang(printout_duplex);
 

  // Und nun erzeuge ich den Befehlsbogen. Der Titel ist fast gleich wie
  // beim Ergebnisausdruck. Deshalb verwende ich die gleiche Funktion.
  
  ausdruck_sektion_titel();
  ausdruck_sektion_spieler(1);
  
  // Teil mit den Kaestchenreihen. Ich nehme fuer die Anzahl momentan
  // noch eine Konstante. Spaeter kann es variabel gestaltet werden.
  
  const short anzahl_zeilen = 23;
  const short halbierte_zeilen = 5;
  const float KBREITE = 0.4; // cm
  const float KHOEHE = 0.7;
  const float KABSTAND = 0.15;
  const float ABKLINKS = 1;
  const float BEFLINKS = 3.9;
  const float ABKHALBLINKS = 10.3;
  const float BEFHALBLINKS = 13.1;
  const float KANZAHL = 38;
  const float KANZAHL_HALB = 15;
  
  // Links kommt ein Kaestchen fuer die Abkuerzung, das ich nun vorbereite
  LAYOUT abkly;
  float re = KBREITE * 6;
  abkly.rechteck_ausgefuellt(-0.05,-0.05,re+0.05,KHOEHE+0.05,100);
  abkly.rechteck_ausgefuellt(0,0,re,KHOEHE,5);
  int j;
  for (j=1; j<6; j++)  abkly.linie(j*KBREITE, 0, j*KBREITE, KHOEHE);

  // Und nun bereite ich noch die rechte Kaestchenreihe vor, in die
  // die Befehle eingetragen werden.
  
  LAYOUT befly;
  befly.rechteck(0,0,KANZAHL*KBREITE,KHOEHE);
  for (j=1; j<KANZAHL; j++) befly.linie(j*KBREITE, 0, j*KBREITE, KHOEHE);

  // Und das halbierte Befehlslayout auch noch vorbereiten.
  LAYOUT befhalbly;
  befhalbly.rechteck(0,0,KANZAHL_HALB*KBREITE,KHOEHE);
  for (j=1; j<KANZAHL_HALB; j++) befhalbly.linie(j*KBREITE, 0, j*KBREITE, KHOEHE);

  // Jetzt kann ich alle Zeilen drucken.

  float oben = dr_abschnitt(anzahl_zeilen * KABSTAND + 1.0);
  
  for (short i=0; i<anzahl_zeilen; i++)
  {
    abkly.ausdrucken(ABKLINKS, oben + (KABSTAND+KHOEHE) * i);
    if (i < anzahl_zeilen - halbierte_zeilen) 
      befly.ausdrucken(BEFLINKS, oben + (KABSTAND+KHOEHE) * i);
    else {
      befhalbly.ausdrucken(BEFLINKS, oben + (KABSTAND+KHOEHE) * i);
      abkly.ausdrucken(ABKHALBLINKS, oben + (KABSTAND+KHOEHE) * i);
      befhalbly.ausdrucken(BEFHALBLINKS, oben + (KABSTAND+KHOEHE) * i);
    }
  }

  dr_auswurf();
  drucker_close();
}


/**---------------------------------------------------------------------------
  * STAAT::befehlsbogen_html()
  * 
  * Erzeugt eine HTML-Datei mit einem Form zum Abschicken der Befehle. Erzeugt
  * auch eine Asciidatei, mir der man die Befehle abschicken kann.
  ---------------------------------------------------------------------------*/
void STAAT::befehlsbogen_html()
{
    char *asciifilename = HTML::printout_filename(this, "befbogen", ".txt", false);
    FILE *ascii = fopen(asciifilename, "w");
    if (!ascii) {
	log('W', "Couldn't open file %s for player %s. Player won't receive ascii command sheet",
	    asciifilename, name);
    }

    char *partie = g_uhr->info("SESSIONNAME");
    long  runde  = myatol(g_uhr->info("ZUGNUMMER"));
    char *email  = lexikon_eintrag("BB_ORGANISATOR",5);
    if (mystrlen(email) <= 0) {
	log('K', "Missing email address of in lexical section, entry "
	    "BB_ORGANISATOR of configuration file");
	return;
    }
    
    HTML html(this, "befbogen",L("Befehlsbogen","Turnsheet"));
    html.buttonpanel()
	.ueberschrifts_balken(L("Befehlsbogen","Turnsheet"));

    html.center()
	.font_size(2)
	.text(L("Partie ","Game "))
	.text(partie)
	.text(L(" - Runde "," - Turn "))
	.text(runde + 1)
	.text(" - ")
	.text(name)
	.end_font();

    html.paragraph()
	.text(L("Datum der n~achsten Auswertung: ","Next evaluation date: "))
	.text(g_uhr->info("NAECHSTER_ZUG"))
	.end_center()
	.paragraph();
    html.text(L("Wenn du willst, kannst du deine Befehle auch mit einem normalen "," Here you can send your commands via ")
	      L("Emailprogramm verschicken. Trage Deine Befehle unten ein und klicke auf 'Befehle abschicken' oder f~ulle ","Email. Just enter your commands below and hit 'Email Commands' or insert your commands into "))
	.href_raw("befbogen.txt")
	.text(L("diese Textdatei","this textfile"))
	.end_href()
	.text(L(" mit deinen Befehle aus und schicke sie an "," and email the file to "))
	.email_ref(email)
	.text(email)
	.end_href()
	.text(L(". Als Betreff verwende ",". As subject please use "))
	.bold()
	.text(partie)
	.text(" ")
	.text(runde + 1)
	.text(" ")
	.text(name)
	.bold_off()
	.text(".")
	.paragraph();

    // Fuer jeden Spieler wird eine Zufallsnummer generiert, die
    // einen geheimen Schluessel darstellt.

    char schluessel[128];
    sprintf(schluessel, "%04ld%04ld%04ld-%04ld%04ld%04ld-%03ld",
	    io_random(10000), io_random(10000), io_random(10000),
	    io_random(10000), io_random(10000), io_random(10000),
	    io_random(1000));
    attribut_setzen("EMAILKEY", schluessel);
    log('3', "Player %s has for turn %ld email key %s", name, runde + 1, schluessel);
    html.form(email);
    html.textarea(L("Befehle","Commands"), 20, 78);

/*    
      Kunde=
      Emailschluessel=868046066437-896637486431-354
      Spiel=w105
      Spieltyp=PROMETHEUS
      Runde=2
      Position=STA
      Absender=
      Befehle=
      BEFEHLSANFANG
*/

    char header[8192];
    sprintf(header, L("Kunde:%s%s\n"
		      "Emailschluessel:%s%s\n"
		      "Spiel:%s%s\n"
		      "Spieltyp:PROMETHEUS%s\n"
		      "Runde:%ld%s\n"
		      "Position:%s%s\n"
		      "Absender:%s%s\n"
		      "Befehle%s\n%s\n"
		      "BEFEHLSANFANG%s\n%s\n"
		      ,
		      "Customer:%s%s\n"
		      "Emailkey:%s%s\n"
		      "Game:%s%s\n"
		      "Gametype:PROMETHEUS%s\n"
		      "Turn:%ld%s\n"
		      "Position:%s%s\n"
		      "Address:%s%s\n"
		      "Commands:%s\n%s\nSTARTOFCOMMANDS%s\n%s\n"),
	    attribut("KONTONR"), CR(), 
	    schluessel, CR(),
	    partie, CR(), 
	    CR(), runde+1, CR(), 
	    name, CR(),
	    attribut_gesetzt("EMAIL") ? attribut("EMAIL") : "", CR(),
	    CR(), CR(), CR(), CR());
    
    html.text(header);

    if (ascii)
	fprintf(ascii, "%s", header);
    
    // Staat
    html.text(name).text(" \n");
    if (ascii) fprintf(ascii, "%s %s\n", name, CR());
    
    // Staedte
    DOUBLIST *staedte = alle_staedte();
    staedte->sort(STADT::sortfunction_stadt_nodelist);
    FOR_EACH_OBJEKT_IN (staedte)
	DO_AND_DELETE ({
	    html.text(objekt->name).text(" \n");
	    if (ascii) fprintf(ascii, "%s %s\n", objekt->name, CR());
	});

    // Weltbauten
    FOR_EACH_OBJEKT_IN (alle_weltbauten())
	DO_AND_DELETE ({
	    html.text(objekt->name).text(" \n");
	    if (ascii) fprintf(ascii, "%s %s\n", objekt->name, CR());
	});
    
    // Einheiten
    FOR_EACH_OBJEKT_IN (alle_einheiten_sortiert())
	DO_AND_DELETE ({
	    html.text(objekt->name).text(" \n");
	    if (ascii) fprintf(ascii, "%s %s\n", objekt->name, CR());
	});
    
    html.text(L("\nBEFEHLSENDE\n\n", "\nENDOFCOMMANDS\n\n"));
    if (ascii) {
	fprintf(ascii, L("%s\nBEFEHLSENDE%s\n%s\n","%s\nENDOFCOMMANDS%s\n%s\n"),
		CR(), CR(), CR());
	fclose(ascii);
    }

    html.end_textarea()
	.paragraph();
    char string[512];
    sprintf(string, L("Befehle abschicken an %s","Email commands to %s"), email);
    html.submit_button(string)
	.text(" ")
	.reset_button(L("ALLE BEFEHLE WIEDER L~OSCHEN!","CLEAR ALL COMMANDS!"))
	.end_form();

    // Liste aller Befehle mit Links auf die Anleitung.
    char *befehlsliste =
	#include "befliste.h"
	;
    html.include(befehlsliste);
}


/**---------------------------------------------------------------------------
  * Uebergibt alle Einrichtungen, die von einer Stadt versorgt wurden,
  * der groessten anderen Stadt im Reich. Wenn es keine mehr gibt,
  * bekommt ein gegnerischer Staat die Einrichtungen.
  *
  * Wird aufgerufen, wenn einer eine Stadt atombombt.
  ---------------------------------------------------------------------------*/
void STAAT::einrichtungen_uebergeben(STADT *von, STAAT *wem)
{
    STADT *zielstadt = groesste_stadt_ausser(von);
    if (zielstadt) einrichtungen_uebergeben(von, zielstadt);
    else // Ich fliege diese Runde raus.
    {
	report(L("Unsere Einrichtungen werden von %s ~ubernommen. ","All our structures are taken over by %s . "),
	       wem->a_name());
	wem->report(L("Wir ~ubernehmen alle Einrichtungen von %s. ","We take over all structures from %s. "),
		    a_name());
	einrichtungen_uebergeben(von, wem->groesste_stadt());
    }
}


/**---------------------------------------------------------------------------
  * Uebergibt alle Einrichtungen, die von einer Stadt versorgt wurden,
  * der groessten anderen Stadt im Reich. Wenn es keine mehr gibt,
  * bekommt sie die Stadt einer gegnerischen Einheit.
  *
  * Wird aufgerufen, wenn einer eine Stadt von einem anderen
  * Staat einnimmt.
  ---------------------------------------------------------------------------*/
void STAAT::einrichtungen_uebergeben(STADT *von, EINHEIT *wem)
{
    STADT *zielstadt = groesste_stadt_ausser(von);
    if (zielstadt) einrichtungen_uebergeben(von, zielstadt);
    else // Ich fliege diese Runde raus.
    {
	report(L("Unsere Einrichtungen werden von %s ~ubernommen. ","Our structures are taken over by %s. "),
	       wem->staat()->a_name());
	wem->staat()->report(L("Wir ~ubernehmen alle Einrichtungen von %s. ","We take over all structures by %s. "),
		    a_name());
	einrichtungen_uebergeben(von, (STADT *)(wem->besitzer));
    }
}


/**---------------------------------------------------------------------------
  * Uebergibt alle Einrichtungen einer anderen Stadt des
  * Reiches. Wird vom Befehl AL und von den beiden obigen
  * Funktionen aufgerufen.
  *
  * @param
  * von:  STADT, deren Einrichtungen uebergeben werden.
  * nach: STADT, die sie empfaengt. Kann einem anderen Staat gehoeren.
  ---------------------------------------------------------------------------*/
void STAAT::einrichtungen_uebergeben(STADT *von, STADT *nach)
{
    FOR_EACH_OBJEKT_IN (alle_weltbauten())
	DO_AND_DELETE({
	    if (((WELTBAUT *)objekt)->versorger() == von)
		nach->einrichtung_uebernehmen(von, (WELTBAUT *)objekt);
	});
}

/**---------------------------------------------------------------------------
  * STAAT:ortsangabe_auswerten()
  *
  * Der Spieler hat vier Moeglichkeiten, einen Fleck auf der Welt zu bestimmen.
  *
  * 0. Leerstring
  * 1. Relativ:             NW, NOO, NNOO, ...
  * 2. Relativ mit Zahlen:  23N18O 4S4W ...
  * 3. Absolut:             -18,4  0,-7 ...
  * 4. Objektreferenz:      HSA, ERO23, ...
  *
  * Zwischen der ersten und der vierten Variante kann es Uneindeutigkeiten geben.
  * Im Zweifelsfall wird die erste angenommen.
  *
  * Ein Leerstring ist ein Spezialfall der ersten Variante.
  *
  * Diese Funktion wandelt eine solche Angabe in Welt-absolte Koordinaten um.
  * Rueckgabe: NULL, wenn alles OK ist, sonst einen Fehlertext.
  ---------------------------------------------------------------------------*/
char *STAAT::ortsangabe_auswerten(char *angabe, ADR& quelle, ADR& ziel)
{
    unsigned laenge = mystrlen(angabe);
    if (!laenge) ziel = quelle; // 0.
    else if (laenge == strspn(angabe, L("nswoSWNO0","nsweSWNE"))) { // 1. NW, NO, SSW,...
	RIC richtung(angabe);
	ziel = welt()->adresse_in_richtung(quelle, richtung);
    }
    else if (laenge == strspn(angabe, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")) { // 4. und 2.
	OBJEKT *obj = objekt_mit_namen(angabe);
	if (obj) { // 4.
	    STAAT *staat = obj->staat();
	    if (!staat) return L("Ung~utltige Ortsangabe. ","Invalid destination. ");
	    if (staat->dipl_status_gegenueber(this) < 2)
		return L("Du kannst nur eigene Objekte und die von freundlichen ","As a destination you can only give own objects or ")
		    L("Staaten als Zielangabe verwenden. ","objects controlled by allies. ");
	    if (obj->ort() != welt())
	    {
		if (obj->ort()->typ_ist("EINHEIT"))
		    return L("Einheiten im Konvoi kannst du nicht als Ziel angeben. ","Units in convoy cannot be a destination. ");
		else return L("Es gibt keine Einheit, Stadt oder Einrichtung mit dieser ","There are no units, towns or structures with such an ")
			 L("Abk~urzung. ","abbreviation. ");
	    }
	    ziel = obj->adresse;
	}
	else if (laenge == strspn(angabe, L("nswoNSWO0123456789","nsweNSWE0123456789")))
	{
	    RIC richtung(angabe);
	    ziel = welt()->adresse_in_richtung(quelle, richtung);
	}
    }
    else if (laenge == strspn(angabe, "oO0123456789-+,"))  // Kandidat fuer absolute Koordinaten
    {
	char *konvert = mystrdup(angabe);
	for (char *p=konvert; *p; p++ ) if (*p == 'o' || *p =='O') *p='0';
	ziel = info_absolute_adresse(konvert);
	myfree(konvert);
	if (welt()->adresse_ist_ausserhalb(ziel))
	    return L("Ung~ultige Ortsangabe. ","Invalid destination. ");
    }
    else return L("Ung~ultige Ortsangabe. ","Invalid destination. ");

    if (welt()->adresse_ist_ausserhalb(ziel)) return L("Ung~ultige Ortsangabe. ","Invalid destination. ");
    else return 0;
}

/**---------------------------------------------------------------------------
  * STAAT::habe_gewonnen()
  ---------------------------------------------------------------------------*/
short STAAT::habe_gewonnen()
{
    return habe_ursprung_des_lebens() ||
	(!g_uhr->einer_hat_ursprung_des_lebens()
	 && !zivilisation_vernichtet()
	 && g_uhr->alle_sind_freundlich());
}


/**---------------------------------------------------------------------------
  * STAAT::bin_als_einziger_uebrig()
  *
  * Liefert true, wenn ich der einzige Staat bin, der noch im Spiel ist.
  * Ich darf aber nicht zivilisation_vernichtet() haben.
  ---------------------------------------------------------------------------*/
bool STAAT::bin_als_einziger_uebrig()
{
    return !zivilisation_vernichtet()
	&& NULL != alle_sieger_bzw_verlierer(3);
}
