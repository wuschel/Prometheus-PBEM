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
  * MODUL:               weltbaut.C  /  WELTBAUT.CPP
  * AUTOR/DATUM:         Mathias Kettner, 19. August 1993
  * KOMPATIBILITAET:     C++
  ----------------------------------------------------------------------------
  *
  *     Enthaelt alle Funktionen der Klasse WELTBAUT.
  *
  ---------------------------------------------------------------------------*/

#include <math.h>

#include "alg.h"
#include "listmac.h"
#include "prom.h"
#include "welt.h"
#include "dauer.h"
#include "layout.h"
#include "laengen.h"
#include "drucker.h"
#include "weltbaut.h"
#include "enzyklop.h"
#include "kompatib.h"
#include "einheit.h"
#include "stadt.h"
#include "log.h"

extern short laser; // Wird zum Debuggen benoetigt.
extern DOUBLIST globale_objekt_menge; 
extern ENZYKLOPAEDIE *g_enzyklopaedie;

/**---------------------------------------------------------------------------
  * WELTBAUT::WELTBAUT()           // constructor
  * 
  * Konstruktor der Weltbauten. Die Daten werden aus der Enzyklopaedie
  * entnommen.
  *
  * @param
  * char *name und char *attr: Wie auch bei den anderen Objekten ueblich.
  * Wichtiges Attribut ist das Attribut VERSORGER, welches ein Objekt
  * definiert, das als Versorger dienen soll (eine Stadt im Normalfall).
  * Der Versorger wird aber nicht im Konstruktor ermittelt, sondern
  * erst bei naechster_zug(), da beim Laden des spezifizierte Objekt
  * vielleicht nicht existiert.
  ---------------------------------------------------------------------------*/
WELTBAUT::WELTBAUT(char *name, char *attr) : OBJEKT(name, attr)
{
  // Meine Gattung sollte schon im Attribut stehen...
  char *gattung = attribut("GATTUNG");

  // Jetzt muss ich in der Enz nachschauen, um auch meine Startattribute
  // und Einfluesse zu erfahren.

  if (!enzyklopaedie()) return; // Dann ist eh' alles am dampfen...

  VORHABEN_ENZ *eintrag = enzyklopaedie()->vorhaben(gattung);
  if (!eintrag) {
      log('K', "Can't find entry for %s in game configuration file", gattung);
      return; // Dann mach ich eben nichts, ausser zu existieren...
  }

  // Mit welchen Resourcen ich versorgt werden soll, steht auch hier.
  attribut_setzen("UNTERHALT", eintrag->unterhalt.to_string());

  // Wenn geladen wurde, dann muss ich nicht viel Konstrukten...
  if (attribut_gesetzt("++LADEN++")) return;

  // Die Attribute der Enz fuege ich den meinen noch hinzu.
  attribute.aus_string_einlesen(eintrag->start_attribute.to_string());

  // Und jetzt uebe ich noch einen Einfluss aus, falls einer definiert ist.
  // Welcher Einfluss dies ist, steht in der Enzyklopaedie unter einfluss_art
  // , einfluss_name, einfluss_attr, einfluss_besitzer, einfluss_parameter

  beeinflussen(eintrag->einfluss_art, eintrag->einfluss_spez,
	       eintrag->einfluss_parameter);

  // Wenn es waehrend einer Runde gebaut wird, dann wird ja naechster_zug()
  // nicht ausgefuehrt.

  naechster_zug(0);
}


/**---------------------------------------------------------------------------
  * WELTBAUT::speichern()
  * 
  * Speichert die Objektspezifischen Daten in eine offenen Asciidatei.
  *
  * @param
  * FILE *file: Zeiger auf offenes File.
  *
  * @return
  * short 1 bei einem Fehler, short 0 sonst.
  ---------------------------------------------------------------------------*/
short WELTBAUT::speichern(FILE *file)
{
  fprintf(file, "%s\n", resourcenspeicher.to_string());
  return ferror(file) != 0;
}


/**---------------------------------------------------------------------------
  * WELTBAUT::laden()
  * 
  * Laedt die Objektspezifischen Daten aus einem offenen Asciifile.
  *
  * @param
  * FILE *file: Zeiger auf offenes File.
  *
  * @return
  * short 1 bei einem Fehler, short 0 sonst.
  ---------------------------------------------------------------------------*/
short WELTBAUT::laden(FILE *file)
{
  char resstring[MAX_LAENGE_RESOURCEZEILE+2];
  fscanf(file, "%s", resstring);
  resourcenspeicher.setzen_auf(resstring);
  return ferror(file) != 0;
}


/**---------------------------------------------------------------------------
  * WELTBAUT::welt()
  * 
  * Liefert einen Zeiger auf die Welt, in der die Weltbaut steht.
  *
  * @return
  * WELT *  Welt, oder NULL im Fehlerfalle (mit Ausgabe).
  ---------------------------------------------------------------------------*/
WELT *WELTBAUT::welt()
{
  if (strcmp(ort()->attribut("TYP"), "WELT")) { // Huch?
      log('I', "WELTBAUT::welt(): Weltbaut %s befindet sich nicht "
	  "in einer Welt!", name);
      return NULL;
  }

  else return (WELT *)ort();
}


/**---------------------------------------------------------------------------
  * WELTBAUT::naechster_zug()
  * 
  * Eine Weltbaut hat beim naechsten Zug nicht viel zu tun.
  ---------------------------------------------------------------------------*/
void WELTBAUT::naechster_zug(long)
{
    gefoerdert.setzen_auf(0);   // Wegen der Statistik ueber die Runde
    transfer.setzen_auf(0); // Wegen der Statistik ueber die Runde
    alte_resourcen.setzen_auf(resourcenspeicher); // Merken fuer Ausdruck
}


/**---------------------------------------------------------------------------
  * WELTBAUT::naechste_phase()
  * 
  * Fuer die Weltbaut ist vor allem Runde interressant, in der sie
  * sich versorgt. Welche Runde dies ist, steht in dauer.h unter
  * dem Namen RUNDE_VERSORGUNG.
  ---------------------------------------------------------------------------*/
void WELTBAUT::naechste_phase(long runde)
{
    log('6', "    -> %s", name);

    // Statusausgabe waerend des Zuges.
  io_printxy(39,1,name);
  io_printxy(44,1,"->           ");

  if (runde == RUNDE_VERSORGUNG_WELTBAUTEN) unterhalt_einholen();
  if (runde == RUNDE_FOERDERUNG) resourcen_foerdern(); // Fuer Bergwerk..
}


/**---------------------------------------------------------------------------
  * WELTBAUT::unterhalt_einholen()
  * 
  * Besorgt sich vom Versorger Resourcen fuer den Unterhalt. Wenn
  * der Unterhalt nicht klappt, wird das Attribut Unversorgt gesetzt.
  * Wenn der Unterhalt steht, wird dieses Attribut geloescht.
  ---------------------------------------------------------------------------*/
void WELTBAUT::unterhalt_einholen()
{
  OBJEKT *vers = versorger();
  if (vers && !vers->kommando("RESOURCEN_ANFORDERN",(void *)attribut("UNTERHALT")))
  {
      if (attribut_gesetzt("Unversorgt"))
      {
	  report(L("Die Versorgung von %s steht wieder. Wir nehmen unsere Arbeit wieder auf.","The supply of %s is re-established. We can continue operation."),
		 vers->a_name());
	  attribut_loeschen("Unversorgt");
      }
  }
  else { // Unversorgt
      if (!attribut_gesetzt("Unversorgt")) // letzte Runde noch versorgt
      {
	  attribut_setzen("Unversorgt");
	  if (vers) report(L("Die Stadt %s kann uns nicht mehr versorgen. ","The town %s cannot supply us anymore! "), vers->a_name());
	  else report(L("Wir haben keine Stadt mehr, die uns versorgt. ","There is no more town to supply us! "));
	  if (attribut_gesetzt("FOERDERQUOTE"))
	      report(L("Wir werden n~achste Runde keine Rohstoffe f~ordern!\n","We will not haul up any resources next turn! \n"));
	  else report(L("Wir m~ussen den Betrieb einstellen.\n","We have to cease operation!\n"));
      }
      else { // weiterhin unversorgt
	  report(L("Wir werden nach wie vor nicht versorgt.\n","There is still no supply.\n"));
      }
  }
}



/**---------------------------------------------------------------------------
  * WELTBAUT::resourcen_foerdern()
  * 
  * Wird nur verwendet, wenn es sich um ein Bergwerk oder einer
  * Bohrinsel oder sowas aehnliches handelt. Foerdert von der
  * Welt soviel Resourcen, wie die Speicherkapazitaet, die Foer-
  * derquote und die Bodenschatzvorkommen erlauben.
  ---------------------------------------------------------------------------*/
void WELTBAUT::resourcen_foerdern()
{
  // Natuerlich nur, wenn der Unterhalt steht.
  if (attribut_gesetzt("Unversorgt")) return;

  if (!attribut_gesetzt("FOERDERQUOTE")) return; // Bin garkein Bergwerk

  if (!welt()) return; // Huch.

  RESOURCE_VEKTOR& vorkommen = welt()->bodenschatz_bei(adresse);


  // Ich habe nun einen direkten Zugang zu den Ressorcen. Ich ziehe
  // mir einmal alles raus, und dann verringerere ich das Vorkommen
  // um eins. Das Vorkommen ist naemlich neuerdings die Foerderquote.
  
  RESOURCE_VEKTOR rausholbar(vorkommen);
  
  if (vorkommen.betrag() > 2) {
    RESOURCE_VEKTOR abziehen, null_vektor;
    abziehen.einheitsvektor_aus(attribut("RESOURCEN"));
    vorkommen.subtrahiere(abziehen);
    vorkommen.negieren();
    vorkommen.begrenzen_auf(null_vektor);
    vorkommen.negieren(); // War alles dazu da um eins abzuziehen
  }
  
  // Ich darf natuerlich nicht alle Resourcen foerdern, sondern nur
  // diejenigen in meine Attribut RESOURCEN. Z.B Uranbergwerk nicht X!
  
  RESOURCE_VEKTOR foerderarten;
  foerderarten.einheitsvektor_aus(attribut("RESOURCEN"));
  foerderarten.multipliziere_mit(100000L); // Mehr werde ich sicher nicht foerdern.
  rausholbar.begrenzen_auf(foerderarten);

  resourcenspeicher.addiere(rausholbar); // Speicher erhoehen
  gefoerdert.setzen_auf(rausholbar);     // Merken fuer den Bericht
}


/**---------------------------------------------------------------------------
  * WELTBAUT::abschlussbericht()
  * 
  * Abschlussbericht der Weltbaut. Orientiert sich auch an der globalen
  * Variablen laser.
  ---------------------------------------------------------------------------*/
void WELTBAUT::abschlussbericht()
{
    if (laser) {
	abschlussbericht_layouten();
    }
    else {
	drucken(L("%s %s. Versorgt von %s, ","%s %s. Supplied by  %s, "), attribut("GATTUNG"), name, attribut("VERSORGER"));
	drucken(L("Anfang = %s, ","Starting at  %s, "),alte_resourcen.to_string());
	drucken(L("Foerderung = %s ","Haulage = %s "), gefoerdert.to_string());
	drucken(L("Transfer = %s\n","Transfer = %s\n"), transfer.to_string());
	drucken(L("Speicher = %s, ","Store = %s, "),resourcenspeicher.to_string());
	drucken(L("Attribute: %s\n","Attribute: %s\n"), attribute.to_string());
	drucken("------------------------------------------------------------------------\n");
	
	reportliste_drucken();
    }
    
    reportliste_loeschen(); // Achtung! Muss nach HTML-OUT kommen. Sonst fehlen dort die Reports.
}

/**---------------------------------------------------------------------------
  * WELTBAUT::abschlussbericht_html()
  * 
  * Schreibt den Bericht der Weltbaut in ein offenes HTML-File
  ---------------------------------------------------------------------------*/
void WELTBAUT::abschlussbericht_html(HTML html)
{
    short anzahl_resourcen = mystrlen(attribut("RESOURCEN"));
    bool  miniausdruck = (anzahl_resourcen == 0);

    html.anchor(name);
    html.smalleimage(attribut("GRAFIK"))                                    // Grafik Einrichtung
	.fieldimage(welt()->gelaendeform_attribut(adresse, "GRAFIK"));      // Grafik Gelaende
    
    if (zur_vernichtung_vorgemerkt()) html.iconimage("totnkopf");

    html.courier().bold().text(" ").text(name).bold_off().courier_off();    // Abkuerzung
    html.text(", ");

    g_enzyklopaedie->vorhaben_mit_link(html, a_gattung());                  // Versorger

    if (versorger()) {
	html.text(L(" versorgt von "," supplied by "));
	html.href_objekt(versorger()).text(versorger()->a_name()).end_href();
    }
    else html.text(L(" ohne Versorger"," without provider"));
    RIC *ric = (RIC *)staat()->info("RELATIVE_ADRESSE",&adresse);
    html.text(L(" ist bei ",", at ")).bold()
	.text(ric->x).text(",").text(ric->y).bold_off()
	.text(". ");
    html.text(L("Unterhalt: ","Upkeep: "))
	.text(attribut("UNTERHALT"));
    if (attribut_gesetzt("Unversorgt")) html.text(", ").bold(L("unversorgt","not supplied"));
    
    if (attribut_gesetzt("Geladen")) {
	if (myatol(attribut("RANGE")) > 1000)
	    html.text(L(", Atomrakete geladen (Reichweite ist unbegrenzt)",", Nuclear missile loaded (Unlimited range)"));
	else html.text(L(", Atomrakete geladen (Reichweite ist ",", Nuclear missile loaded (Range: "))
	    .text(attribut("RANGE"))
	    .text(L(" Felder)"," squares)"));
    }
    
    if (!miniausdruck) {
	if (attribut_gesetzt("FOERDERQUOTE")) // Bei Bergwerken angeben.
	{ 
	    RESOURCE_VEKTOR& vorkommen = welt()->bodenschatz_bei(adresse);
	    char kennbuchstabe = attribut("RESOURCEN")[0];
	    long menge = vorkommen[kennbuchstabe];
	    if (menge) {
		html.text(L(", F~orderquote: ",", Haulage Quota: "))
		    .text(menge)
		    .text(kennbuchstabe);
	    }
	}
	
	if (attribut_gesetzt("KeineAusgabe")) html.text(L(", Auslieferung eingestellt",", Delivery OFF"));
	else                                  html.text(L(", Auslieferung aktiv",", Delivery ON"));
    
	// Jetzt die Guetertabelle.

	HTML *subhtml = new HTML(html);
	subhtml->set_cell_spacing(0)
	    .set_table_color("#ffffff")
	    .set_table_width("100%")
	    .table()
	    .set_row_color("#e0e0e0")
	    .next_row()
	    .next_cell(L("G~utertabelle","Commodities Chart"));
	
	const short anzahl_sparten = 5;
	char *sparten[anzahl_sparten] =
	{L(" Anfang"," Starting"),
	 L(" F~orderung"," Haulage"),
	 L("Verarbeitung","Processing"),
	 L(" Transfer"," Transfer"),
	 L(" Speicher"," Store")};
	
	subhtml->bold();
	for (int i=0; i<anzahl_sparten; i++) subhtml->next_cell(sparten[i]);
	subhtml->bold_off().unset_row_color();
	
	// Hier bereite ich die Vektoren vor.
	RESOURCE_VEKTOR resvekt[anzahl_sparten];
	resvekt[0].setzen_auf(alte_resourcen);
	resvekt[1].setzen_auf(gefoerdert);
	resvekt[2].setzen_auf(0); // Kommt spaeter noch.
	resvekt[3].setzen_auf(transfer);
	resvekt[4].setzen_auf(resourcenspeicher);
	
	// Und nun die Resourcen...
	for (int res=0; res<anzahl_resourcen; res++)
	{
	    char buchstabe = attribut("RESOURCEN")[res];
	    subhtml->next_row();
	    RESOURCE_ENZ *resenz = g_enzyklopaedie->resource(buchstabe);
	    if (!resenz) continue; // Huch?
	    subhtml->set_cell_alignment(AL_LEFT).next_cell(resenz->name);
	    for (int i=0; i<anzahl_sparten; i++) 
	    {
	      long wert = resvekt[i][buchstabe];
	      if (wert) subhtml->set_cell_alignment(AL_RIGHT).next_cell(wert);
	      else subhtml->empty_cell();
	    }
	}
	subhtml->end_table();
	delete subhtml;
    } // !miniausdruck
    
    html.linebreak();
    REPORT *report;
    SCAN(&reportliste, report) {
	html.text(report->text);
	NEXT(report);
    }
}


/**---------------------------------------------------------------------------
  * WELTBAUT::abschlussbericht_layouten()
  * 
  * Gibt den Abschlussbericht in Laserdruckerformat aus.
  ---------------------------------------------------------------------------*/
void WELTBAUT::abschlussbericht_layouten()
{
  char string[100]; // Fuer alle moeglichen Texte

  const float rechterrand = 19.80;
  const float yraster = 0.4;
  const float xraster = 1.5; // Etwas breiter, als in der Stadt
  const float namelinks = 15.02;
  const float abkunten = 0.4;

#define KLEINE_SCHRIFT_PROP "Times",7
#define KLEINE_ZAHLEN "Courier",7.1
#define NAME_SCHRIFT "Times",10
#define STATUS_SCHRIFT "Times",9
#define ABK_SCHRIFT "Courier",11.5

  // Bevor es losgehen kann, muss ich erstmal die Hoehe des Kaestchens
  // bestimmen. An meinem Attribut RESOURCEN kann ich sehen, welche Ressource-
  // zeilen ich ueberhaupt brauche (Hemmingnorm). Zeilen, die Grundsaetz-
  // lich leer sind, lasse ich naemlich gleich weg.

  // Bei Einrichtungen ohne Gueter drucke ich nur ein ganz schmales Layout,
  // da ich hier nur wenige Angaben benoetige.

  float hoehe;
  short miniausdruck;
    
  short anzahl_resourcen = mystrlen(attribut("RESOURCEN"));

  if (!anzahl_resourcen) {
    hoehe = abkunten;
    miniausdruck = 1;
  }
  
  else {
    hoehe = MAX(1.4 , (anzahl_resourcen+1) * yraster);
    miniausdruck = 0;
  }

  // Und nun kann ich ein nettes Layout anlegen...
  dr_abschnitt(0.2); // Etwas Platz zum vorherigen lassen.
  LAYOUT ly(dr_abschnitt(hoehe));

  // Alles Umrahmen...
  ly.rechteck(0,0,rechterrand,hoehe);

  // Grafik..
  char grafikname[100];
  sprintf(grafikname, "e%s.gra", attribut("GRAFIK"));
  ly.bitmatrix(0, 0, grafikname);

  // Als erstes kommt der rechte Bereich mit der Gattung und der Abkuerzung
  // und den Koordinaten

  ly.linie(namelinks, 0, namelinks, hoehe);
  ly.linie(namelinks+0.05, 0, namelinks+0.05, hoehe);
  ly.text(namelinks + 0.3, 0.3, attribut("GATTUNG"), NAME_SCHRIFT);

  float abklinks = rechterrand - 0.25*strlen(name) - 0.15;
  if (!miniausdruck) ly.linie(abklinks,abkunten,rechterrand,abkunten);
  ly.linie(abklinks,0,abklinks,abkunten);
  ly.text(abklinks+0.1,abkunten-0.1,name,ABK_SCHRIFT);

  // Der Versorger soll auch drin stehen.

  if (versorger()) sprintf(string,L("Versorger: %s","Provider: %s"),versorger()->a_name());
  else sprintf(string,L("Kein Versorger!","No Provider!"));
  if (miniausdruck)
    ly.text(0.5, 0.3, string, STATUS_SCHRIFT);
  else
    ly.text(namelinks + 0.3, 0.85, string, STATUS_SCHRIFT);

  // Und dann auch noch der Auslieferstatus.
  if (!miniausdruck)  {
    char *status;
    if (attribut_gesetzt("KeineAusgabe")) status = L("Auslieferung eingestellt.","Delivery OFF");
    else status = L("Auslieferung aktiv.","Delivery ON");
    ly.text(namelinks + 0.3, 1.183, status, STATUS_SCHRIFT);
  }

  // Die (relative) Adresse der Weltbaut traege ich rechts unten ins Eck ein.
  RIC *ric = (RIC *)staat()->info("RELATIVE_ADRESSE",&adresse);
  sprintf(string,"%ld,%ld",ric->x, ric->y);
  if (miniausdruck) 
    ly.text(abklinks-strlen(string)*0.15-0.1, hoehe-0.1, string, KLEINE_ZAHLEN);
  else 
    ly.text(rechterrand-strlen(string)*0.15-0.1, hoehe-0.1, string, KLEINE_ZAHLEN);

  // Jetzt kommt die Resourcentabelle. Zuerst die Uberschriften.
  // (Ich drucke die ganze Tabelle nur aus, wenn es ueberhaupt Resourcen
  // gibt, die ich ausdrucken soll).

  const short anzahl_sparten = 5;

  if (anzahl_resourcen)
  {
    char *sparten[5] = {L(" Anfang"," Starting at"),
			L(" F~orderung"," Haulage"),
			L("Verarbeitung"," Processing"),
			L(" Transfer"," Transfer"),
			L(" Speicher"," Store")};
    for (int i=0; i<anzahl_sparten; i++) {
      ly.linie(1.9 + xraster*(i+1), 0, 1.9 + xraster * (i+1), hoehe);
      ly.text(1.9 + xraster*i + 0.1, yraster-0.1, sparten[i], KLEINE_SCHRIFT_PROP);
    }

    if (!enzyklopaedie()) return; // Huch?

    // Hier bereite ich die Vektoren vor.
    RESOURCE_VEKTOR resvekt[anzahl_sparten];
    resvekt[0].setzen_auf(alte_resourcen);
    resvekt[1].setzen_auf(gefoerdert);
    resvekt[2].setzen_auf(0); // Kommt spaeter noch.
    resvekt[3].setzen_auf(transfer);
    resvekt[4].setzen_auf(resourcenspeicher);

    // Und nun die Resourcen...
    for (int res=0; res<anzahl_resourcen; res++) {
      ly.linie(0, yraster*(res+1), 1.9 + xraster*anzahl_sparten, yraster*(res+1));
      char search[2];
      search[1] = 0;
      search[0] = attribut("RESOURCEN")[res];
      RESOURCE_ENZ *resenz = enzyklopaedie()->resource(search);
      if (!resenz) continue; // Huch?

      ly.text(0.1, yraster*(res+2) - 0.1, resenz->name, KLEINE_SCHRIFT_PROP);
      for (int i=0; i<anzahl_sparten; i++) {
        long wert = resvekt[i][search[0]];
        if (wert) {
	  sprintf(string, "%9ld", wert);
	  ly.text(2.0+xraster*i, yraster*(res+2) - 0.1, string, KLEINE_ZAHLEN);
	}
      }
    }

    // Jetzt fehlen noch ein paar Trennstriche.
    ly.linie(1.85, 0, 1.85, hoehe);
    ly.linie(1.9, 0, 1.9, hoehe);
    ly.linie(1.9+anzahl_sparten*xraster+0.05, 0, 1.9+anzahl_sparten*xraster+0.05,hoehe);

    // Nun male ich noch eine nette kleine Grafik. Eine Scheibe, die
    // den Lagerinhalt repraesentiert.

    const float torte1x = 11.6;
    const float tortey = 0.7;
    const float max_radius = 0.65;

    float tabelle[1] , radius;

    ly.text( torte1x-max_radius-1.1, 0.8, L("Speicher","Store"), KLEINE_SCHRIFT_PROP);
    radius = MAX(0.05,MIN(sqrt(float(resourcenspeicher.betrag())) * 0.015 ,
                           max_radius));
    tabelle[0] = 1.0; // Dummywert. Ich will nur einen Kreis.
    ly.torte(torte1x, tortey, radius, tabelle, 1);
  }

  ly.linie(12.3, 0, 12.3, hoehe);
  
  // Angaben im Zweiten Block von rechts.
  
//  float unterhalt_oben = 0.3 + (!miniausdruck) * 0.03333;
  ly.text(12.4, 0.3, L("Unterhalt:","Upkeep:"), KLEINE_SCHRIFT_PROP);
  ly.text(13.6, 0.3, attribut("UNTERHALT"), KLEINE_SCHRIFT_PROP);

  if (miniausdruck) {
    if (attribut_gesetzt("Unversorgt"))
      ly.text(6, 0.3, L("Unversorgt!","No Supply!"), NAME_SCHRIFT);
    else ly.text(6, 0.3, L("Versorgung gew~ahrleistet","Supply established") , NAME_SCHRIFT);
  }
  else // (!miniausdruck)
  {
    ly.linie(12.3, 0.4667, namelinks, 0.4667);
    ly.linie(12.3, 0.9333, namelinks, 0.9333);

    if (attribut_gesetzt("Unversorgt"))
      ly.text(12.4, 0.8, L("Unversorgt!","No Supply!"), KLEINE_SCHRIFT_PROP);
    else
      ly.text(12.4, 0.8, L("Versorgung gew~ahrleistet","Supply established"), KLEINE_SCHRIFT_PROP);

    if (attribut_gesetzt("FOERDERQUOTE")) // Bei Bergwerken angeben.
    { 
      RESOURCE_VEKTOR& vorkommen = welt()->bodenschatz_bei(adresse);
      char kennbuchstabe = attribut("RESOURCEN")[0];
      long menge = vorkommen[kennbuchstabe];
      if (menge)
      {
        char string[50];
        sprintf(string, L("F~orderquote: %ld%c","Haulage Quota: %ld%c"), menge, kennbuchstabe);
        ly.text(12.4, 1.26667, string,KLEINE_SCHRIFT_PROP);
      }
    }
  }


  ly.ausdrucken();

  // Und nun kommen noch Reports, falls welche da sind (z.B. Sternwarte)
  reports_layouten_und_drucken(this);
}


/**---------------------------------------------------------------------------
  * WELTBAUT::kommando()
  * 
  * Kommandofunktion wie in objekt.h deklariert. Schnittstelle wie
  * ueblich.
  ---------------------------------------------------------------------------*/
short WELTBAUT::kommando(const char *kommando, const void *par1, const void *par2, const void *)
{
  if (!mystrcmp("RESOURCEN_LIEFERUNG",kommando))
	return kommando_resourcen_bekommen(par1);

  else if (!mystrcmp("RESOURCEN_ANFORDERN",kommando))
	return kommando_resourcen_anfordern(par1);

  else if (!mystrcmp("BESCHUSS", kommando))
	return kommando_beschossen_werden(par1, par2);

  else if (!mystrcmp("EINGENOMMEN",kommando))
	return kommando_eingenommen(par1, par2);

  else if (!mystrcmp("METEORIT",kommando))
	return kommando_meteorit(par1, par2);

  else if (!mystrcmp("ATOM_DETECT",kommando))
	return kommando_atom_detect(par1, par2);

  else if (!mystrcmp("RAKETE_STATIONIEREN",kommando))
	return kommando_rakete_stationieren(par1);

  else return 1;
}


/**---------------------------------------------------------------------------
  * WELTBAUT::kommando_atom_detect()
  * 
  * (ADR *)par1:	Einschlagsort der Atomrakete
  * (OBJEKT *)par2:	Raketenbasis des Abschusses
  ---------------------------------------------------------------------------*/
short WELTBAUT::kommando_atom_detect(const void *par1, const void *)
{
  if (mystrcmp(a_gattung(), L("Sternwarte","Observatory")) || attribut_gesetzt("Unversorgt"))
    return 1;

  ADR zielfeld = *(ADR *)par1;
  RIC richtung = welt()->richtung_von_nach(adresse, zielfeld);
  float entfernung = welt()->entfernung_zwischen(adresse, zielfeld);
    
  char string[512];
  sprintf(string, L("Schwere Atomare Detonation grob aus Richtung %s in einer Entfernung von %.1f festgestellt.\n","Devastating nuclear detonation reported ! Estimated  direction:  %s. Estimated distance:  %.1f .\n"),
    richtung.grob_to_string(), entfernung);
  report(string);
  return 0;
}

  
/**---------------------------------------------------------------------------
  * WELTBAUT::kommando_rakete_stationieren()
  * 
  * Eine Rakete wird in einem Raketensilo stationiert.
  * @param
  * (OBJEKT *)par1:    Kernwaffeneinheit
  * @return
  * 1, wenn kein Platz mehr war, sonst 0.
  ---------------------------------------------------------------------------*/
short WELTBAUT::kommando_rakete_stationieren(const void *par1)
{
    OBJEKT *rakete = (OBJEKT *)par1;
  
    if (attribut_gesetzt("Geladen")) {
	report(L("Die %s ","The %s "),rakete->a_gattung());
	report(L("%s konnte nicht geladen werden, da das Silo schon geladen ist.","%s could not be loaded, because the Rocket Launcher already is."),
	       rakete->name);
	return 1;
    }

    if (attribut_gesetzt("Unversorgt")) {
	report(L("Die %s ","The %s "),rakete->a_gattung());
	report(L("%s konnte nicht geladen werden, da das Silo unversorgt ist.","%s could not be loaded, because the Launcher is not supplied."),
	       rakete->name);
	return 1;
    };


    report(L("Wir laden die %s","We loaded the %s"), rakete->a_gattung());
    report(L(" %s. Reichweite ist "," %s. Range: "), rakete->name);
    long range = myatol(rakete->attribut("RANGE"));
    if (range < 999) report(L("%s Felder.\n","%s Squares.\n"), myltoa(range));
    else report(L("unbegrenzt.\n","unlimited.\n"));
    
    attribut_setzen("Geladen");
    attribut_setzen("RANGE", myltoa(range));
    return 0;
}



/**---------------------------------------------------------------------------
  * WELTBAUT::kommando_meteorit()
  * 
  * Gilt nur fuer Sternwarten. Gibt reports ueber gesichtete Meteoriten
  * aus. Unversorgte Sternwarten sehen nichts.
  * @param
  * (ADR *)par1:	Einschlagort.
  * (long *)par2:		Groesse in X.
  ---------------------------------------------------------------------------*/
short WELTBAUT::kommando_meteorit(const void *par1, const void *par2)
{
  if (mystrcmp(a_gattung(), L("Sternwarte","Observatory")) || attribut_gesetzt("Unversorgt"))
    return 1;
  
  long groesse = *(long *)par2;
  
  char *klasse;
  if (groesse <= 10) klasse = L("I (1-10 kg)","I (1-10 lbs)");
  else if (groesse <= 20) klasse = L("II (11-20 kg)","II (11-20 lbs)");
  else if (groesse <= 35) klasse = L("III (21-35 kg)","III (21-35 lbs)");
  else if (groesse <= 60) klasse = L("IV (36-60 kg)","IV (36-60 lbs)");
  else                    klasse = L("V (61-100 kg)","V (61-100 lbs)");

  char string[256];
  RIC *ric = (RIC *)staat()->info("RELATIVE_ADRESSE", (void *)par1);
  sprintf(string,L("Meteoriteneinschlag Klasse %s bei %ld,%ld gesichtet.#\n","Meteorite Impact Level %s sighted at %ld,%ld .#\n"),
    klasse, ric->x, ric->y);
  report(string);
  return 0;
}
  

/**---------------------------------------------------------------------------
  * WELTBAUT::kommando_resourcen_bekommen()
  * 
  * Die Weltbaut bekommt von irgendwem einfach mal Resourcen. Was sie
  * damit macht, ist ihr Problem. In meinem Fall versucht sie, soviel
  * wie moeglich davon zu speichern. Der Rest verfaellt leider.
  *
  * @param
  * (char *)   Resourcestring, der ueber die Lieferung Auskunft gibt.
  *
  * @return
  * short 0.
  ---------------------------------------------------------------------------*/
short WELTBAUT::kommando_resourcen_bekommen(const void *par1)
{
  RESOURCE_VEKTOR bekommen((char *)par1);
  resourcenspeicher.addiere(bekommen);
  return 0;
}


/**---------------------------------------------------------------------------
  * WELTBAUT::kommando_resourcen_anforden()
  * 
  * Diese Art der Resourcenanforderung funktioniert im Gegensatz zu
  * den infos im alles oder nichts Prinzip. Nur wenn alle geforderten
  * Resourcen da waren, wird die Aktion ausgefuert.
  * @param
  * (char *)par1: Resourcestring der Forderung
  * @return
  * short 0 bei Erfolg, sonst 1.
  ---------------------------------------------------------------------------*/
short WELTBAUT::kommando_resourcen_anfordern(const void *par1)
{
  RESOURCE_VEKTOR forderung((char *)par1);
  if (forderung.passt_in(resourcenspeicher)) {
    resourcenspeicher.subtrahiere(forderung);
    return 0;
  }
  else return 0; // Nicht mehr genug da.
}

/**---------------------------------------------------------------------------
  * WELTBAUT::kommando_beschossen_werden()
  * 
  * Teilt der Weltbaut mit, dass sie bombardiert wird. Hier wird ent-
  * schieden, was mit ihr passiert.
  * Der Staat wird ausserdem gebeten, den DS zu ueberpruefen.
  *
  * @param
  * (char *)        Feuerkraft, 9999 = Atombombe
  * (OBJEKT *)  Bombardeur
  *
  * @return
  * 0, falls die Weltbaut vernichtet wurde
  * 1, sonst.
  ---------------------------------------------------------------------------*/
short WELTBAUT::kommando_beschossen_werden(const void *par1, const void *par2)
{
  long feuerkraft = myatol((char *)par1);
  OBJEKT *bombardeur = (OBJEKT *)par2;
  short atom = (feuerkraft == 9999);

  // Es koennte sein, dass wird den STAAT von par2 von nun ab als Gegner
  // ansehen muessen.
  staat()->kommando("AGGRESSION",par2);

  if (!atom) {

    // in meinem Attribut STABILITAET steht mein Verteididungswert drinnen.
    // Ich wuerfle gegen die Feuerkraft.

    long verteidigung = myatol(attribut("STABILITAET"));

    if (io_random(feuerkraft + verteidigung) < feuerkraft) { // Getroffen
      staat()->kommando("WELTBAUT_BESCHOSSEN", this, bombardeur,
			NULL); // NULL heisst vernichtet.
      zur_vernichtung_vormerken();
      return 0;
    }

    else {
      staat()->kommando("WELTBAUT_BESCHOSSEN", this, bombardeur,
			      this); // NULL hiesse vernichtet.
      return 1;
    }
  }
  
  else { // Atomar!
    staat()->kommando("WELTBAUT_ATOMGEBOMBT", this, bombardeur);
    zur_vernichtung_vormerken();
    return 0;
  }
}


/**---------------------------------------------------------------------------
  * WELTBAUT::kommando_eingenommen()
  * 
  * Wird vom Feind aufgerufen, wenn die Weltbaut von ihm eingenommen
  * wird. Erzeugt den Report beim Staat. Als neuer Versorger wird
  * die Stadt der gegnerischen Einheit gewaehlt.
  *
  * @param
  * (OBJEKT *)  Angreifende Einheit
  * (OBJEKT *)  Gegnerischer Staat
  ---------------------------------------------------------------------------*/
short WELTBAUT::kommando_eingenommen(const void *par1, const void *par2)
{
   // Zuerst hole ich mir die Daten

  OBJEKT *angreifer = (OBJEKT *)par1;
  OBJEKT *gegnerstaat = (OBJEKT *)par2;
  OBJEKT *gegnerstadt = angreifer->besitzer;

  // Meinen Staat und Gegnerstaat informieren.

  gegnerstaat->kommando("STADT_EROBERT", (void *)this);
  besitzer->kommando("STADT_VERLOREN", (void *)this, (void *)angreifer);

  // Besitzer und Versorger wechseln

  geben(gegnerstaat, this);
  attribut_setzen("VERSORGER",gegnerstadt->name);

  return 0;
}


/**---------------------------------------------------------------------------
  * WELTBAUT::info()
  * 
  * Virtuell ueberladene Infofunktion von OBJEKT.
  *
  * PARAMTER:
  * char *info:     Die ersten beiden Buchstaben geben das Info an,
  * der Rest sind Parameter.
  *
  * @return
  * char *: Je nach Info.
  ---------------------------------------------------------------------------*/
char *WELTBAUT::info(char *info, void *par1, void *par2, void *par3)
{
  if (!strcmp("RESSOURCEN_AUSLIEFERN",info)) return info_resourcen_anfordern(par1);
  else if (!strcmp("RESSOURCEN_ANFORDERN",info)) return info_resourcen_anfordern_2(par1, par2, par3);
  else if (!strcmp("ROHSTOFFPRODUKTION",info)) return info_rohstoffproduktion();
  else return NULL; // Unbekanntes Info.
}


/**---------------------------------------------------------------------------
  * WELTBAUT::info_resourcen_anfordern()
  * 
  * Realisiert das Info, mit der von der Weltbaut Resourcen an-
  * gefordert werden koennen. Sie rueckt soviel, wie verlangt oder wie
  * moeglich raus. Wenn das Attribut KeineAusgabe gesetzt ist, dann
  * ist die Auslierung eingestellt, und der Anforderer bekommt nichts.
  *
  * @param
  * (char *)resstring: Gewuenschte Resourcen. Wenn der NULL oder "" ist,
  * dann rueckt die Weltbaut ihr ganzes Lager raus.
  *
  * @return
  * char *: Resourcestring der tatsaechlich bekommenen Resourcen
  * (von RESOURCE_VEKTOR::to_string())
  ---------------------------------------------------------------------------*/
char *WELTBAUT::info_resourcen_anfordern(void *par1)
{
  if (attribut_gesetzt("KeineAusgabe")) return NULL;

  RESOURCE_VEKTOR erwuenscht;
  if (par1)  {
    erwuenscht.setzen_auf((char *)par1);
    erwuenscht.begrenzen_auf(resourcenspeicher); // Mehr als alles gibt's numal nicht.
  }
  else erwuenscht.setzen_auf(resourcenspeicher); // Dann will er alles

  resourcenspeicher.subtrahiere(erwuenscht);
  transfer.subtrahiere(erwuenscht); // Wegen der Statistik
  return erwuenscht.to_string();
}


/**---------------------------------------------------------------------------
  * WELTBAUT::info_resourcen_anfordern_2()
  * 
  * Arbeitet aehnlich, wie das info resourcen_anfordern(), aber
  * mit anderen Parametern. Es ist eine negative Anforderung auch
  * moeglich. Diese entspricht einem Ressourcenangebot.
  *
  * @param
  * (char *)par1:           Ressourcestring
  * (OBJEKT *)par2:     Partner
  * (long *)par3:           Maximalbetrag
  * @return
  * char *: Resourcestring (::to_string()) von den bekommenen Resourcen.
  ---------------------------------------------------------------------------*/
char *WELTBAUT::info_resourcen_anfordern_2(void *par1, void *, void *par3)
{
  RESOURCE_VEKTOR erwuenscht((char *)par1);
  long max_betrag = *(long *)par3;

  if (erwuenscht.ist_negativ()) { // Ich bekomme etwas
    erwuenscht.negieren(); // Ist nun positiv
    transfer.addiere(erwuenscht); // Habe etwas bekommen, daher positiv
    resourcenspeicher.addiere(erwuenscht);
    return erwuenscht.to_string(); // Ruckinfo: Soviel habe ich aufgenommen.
  }
  
  else { // Ich soll herausruecken
    erwuenscht.begrenzen_auf(resourcenspeicher);
    erwuenscht.begrenzen_auf(max_betrag);
    transfer.subtrahiere(erwuenscht);
    resourcenspeicher.subtrahiere(erwuenscht);
    return erwuenscht.to_string();
  }
}

/**---------------------------------------------------------------------------
  * WELTBAUT::info_rohstoffproduktion()
  * 
  ---------------------------------------------------------------------------*/
char *WELTBAUT::info_rohstoffproduktion()
{
  return myltoa(gefoerdert[L('K','N')]
		+ gefoerdert[L('F','C')]
		+ gefoerdert[L('M','R')]);
}

/**---------------------------------------------------------------------------
  * WELTBAUT::befehl_auswerten()
  * 
  * Befehlauswertenfunktion gemaess dem Objektmodell.
  *
  * @param
  * char *befehl: Klartext des Befehls mitsamt Parametern
  *
  * @return
  * Nach ueblichen Konventionen.
  ---------------------------------------------------------------------------*/
short WELTBAUT::befehl_auswerten(char *befehl, long)
{
  if (!strncmp(L("US","AN"),befehl,2)) return befehl_ueberstellen(befehl);
  else if (!strncmp(L("AE","DO"),befehl,2)) return befehl_auslieferung_einstellen();
  else if (!strncmp(L("FE","LN"),befehl,2)) return befehl_rakete_abschiessen(befehl);
  else if (!strncmp(L("XX","RS"),befehl,2)) return befehl_aufloesen();

  else {
    report(L("Der Befehl %s ist f~ur Einrichtungen in der","The Command %s is not valid for "), befehl);
    report(L(" Landschaft nicht verwendbar.\n"," structures..\n"));
    return 1;
  }
}


/**---------------------------------------------------------------------------
  * WELTBAUT::befehl_aufloesen()
  * 
  * Der Befehl XX reisst ein Bergwerk o.a ab.
  ---------------------------------------------------------------------------*/
short WELTBAUT::befehl_aufloesen()
{
  besitzer->report(L("Wir l~osen die Einrichtung %s ","We razed the structure %s "), name);
  besitzer->report(L("(%s) auf.\n","(%s) .\n"), a_gattung());
  zur_vernichtung_vormerken();
  return 1;
}

  
/**---------------------------------------------------------------------------
  * WELTBAUT::befehl_rakete_abschiessen()
  * 
  * Nur fuer Raketensilos. Dient zum Abschiessen von Atomraketen.
  * PARAETER:
  * "FE17,-23"
  ---------------------------------------------------------------------------*/
short WELTBAUT::befehl_rakete_abschiessen(char *befehl)
{
  if (!attribut_gesetzt("Silo")) {
    report(L("%s: Diesen Befehl k~onnen nur Raketensilos ausf~uhren.\n","%s: Only Rocket Lauchers can execute that command.\n"),
    befehl);
    return 1;
  }

  if (attribut_gesetzt("Unversorgt")) {
    report(L("%s: Solange das Raketensilo unversorgt ist, kann keine Rakete","%s: No missile can be launched as long the Rocket Launcher "),
      befehl);
    report(L(" abgefeuert werden.\n"," is not supplied.\n"));
    return 1;
  }
  
  if (!attribut_gesetzt("Geladen")) {
    report(L("%s: Es ist keine Rakete geladen.\n","%s: No missile loaded.\n"), befehl);
    return 1;
  }
  
  // Zielfeld bestimmen.
  ADR ziel;
  char *fehler = staat()->ortsangabe_auswerten(befehl+2, adresse, ziel);
  if (fehler) {
    report("%s: ", befehl);
    report("%s", fehler);
    return 1;
  }
  
  // Wenn der Spieler nach ausserhalb schiesst, ist es sein eigenes
  // Problem. Er darf dadurch nicht herausfinden koennen, wie gross
  // die Welt ist. Aber die Reichweite der Rakete muss ich schon 
  // abfragen.
  
  float range = myatof(attribut("RANGE"));
  float abstand = welt()->entfernung_zwischen(adresse, ziel);
  if (abstand > range) {
    report(L("%s: Das Ziel liegt au~serhalb der Reichweite der geladenen","%s: Target out of missile range!"), befehl);
    report(L(" Rakete. Die Reichweite betr~agt nur %s Felder.\n"," Range is only %s Squares.\n"), attribut("RANGE"));
    return 1;
  }
  
  report(L("Wir feuern eine Atomrakete auf die Koordinaten %s.","We launched a missile targeting coordinates %s."), befehl+2);
  attribut_loeschen("Geladen");
  
  // Ist auf dem Feld eine Stadt mit Abwehrraketen? Wenn ja, dann muessen
  // diese erst Gelegenheit zur Abwehr bekommen.

  bool abgewehrt = false;
  if (!welt()->adresse_ist_ausserhalb(ziel))
  {
    FOR_EACH_OBJEKT_IN (welt()->alle_objekte_bei_adresse(ziel, "TYP=STADT"))
    DO_AND_DELETE ({
      if (objekt->kommando("ABWEHRTEST",this)) abgewehrt = true;
    });
  }    
  if (abgewehrt) 
    report(L("Doch leider wird sie von einer verdammten Abwehrrakete unsch~adlich gemacht!\n","Unfortunately, it was crushed by a  mean-spirited  Defensive Missile!\n"));
 
  else { // Mobile Abwehrraketen
      if (welt()->abwehr_durch_mobile_raketen(adresse, ziel, 9999, staat()))
      {
	  report(L("Aber eine verfluchte mobile Abwehrrakete macht sie unsch~adlich!\n","but it  is destroyed by a cursed  mobile Defensive Missile!\n"));
	  abgewehrt = true;
      }
  }

  // Und nun bekommen alle Dinge auf diesem Feld eine schoene Mitteilung.
  // Die Feuerkraft 9999 steht symbolisch fuer "undendlich" und mus von
  // den Zielen abgefragt werden.

  if (!abgewehrt && !welt()->adresse_ist_ausserhalb(ziel))
  {
      // Zuerst wird das Feld noch zu atomarer Wueste. Die Strassen
      // verschwinden auch. Aber nur, wenn es sich nicht um ein
      // Meeresfeld handelt. Also eines mit ART=See.
      
      // Ich mache dies, bevor die Stadt den Beschuss mitbekommt,
      // weil die Stadt das Landschaftsabbild des Staates aktualisiert
      // und dabei den Krater in die Karte des Spielers uebertraegt.
      
      if (mystrcmp(welt()->gelaendeform_attribut(ziel, "ART"), "See"))
      {
	  welt()->loesche_feld_attribut(ziel, FELD_ATTRIBUT_STRASSE);
	  welt()->gelaendeform_aendern(ziel, 
				       welt()->gelaendeform_mit_namen(L("Atomare_W~uste","Nuclear_Wasteland")));
      }
      
      
      // Die Staaten und die Sternwarten werden informiert.
      restliche_welt_informieren(ziel);
      
      FOR_EACH_OBJEKT_IN (welt()->alle_objekte_bei_adresse(ziel))
	  DO_AND_DELETE ({
	      objekt->kommando("BESCHUSS", "9999", this);
	  });
  }
  return 1;

}


/**---------------------------------------------------------------------------
  * restliche_welt_informieren()
  * 
  * Informiert Staaten und Sternwarten uebern einen atomaren Beschuss.
  ---------------------------------------------------------------------------*/
void WELTBAUT::restliche_welt_informieren(ADR& ziel)
{
  const int ANZAHL_TEXTE = 6;
  static char *text[ANZAHL_TEXTE] = { 
    L("Dem staatlichen Geheimdienst zufolge hat %s eine Atomrakete abgeschossen!\n","Message from your Secret Service: %s launched Nuclear Missile!\n"),
    L("Emp~orung unter der Bev~olkerung: %s feuert eine Atomrakete ab!\n","Rebellion amongst your citizens: %s launched  Nuclear Missile!\n"),
    L("Botschafter verschiedener Staaten melden den Abschu~s einer Atomrakete durch %s!\n","Emissaries of several states report the launching of Nuclear Missiles by  %s!\n"),
    L("%s feuert eine Atomrakete ab!\n","%s launches a Nuclear Missile!\n"),
    L("%s schie~st mit Atomraketen um sich!\n","%s uses lethal Nuclear weaponery!\n"),
    L("In %s wird eine Atomrakete gez~undet!\n","In %s a Nuclear Missile has been launched!\n")
  };

  char reportstring[512];
  sprintf(reportstring, text[io_random(ANZAHL_TEXTE)], staat()->a_name());

  FOR_EACH_OBJEKT_IN (&globale_objekt_menge)
  DO ({
    if (objekt->typ_ist("STAAT") && objekt != staat())
      objekt->report(reportstring);

    else if (!mystrcmp(objekt->a_gattung(),L("Sternwarte","Observatory")))
      objekt->kommando("ATOM_DETECT", &ziel, this);
  })
}


/**---------------------------------------------------------------------------
  * WELTBAUT::befehl_ueberstellen()
  * 
  * Mit dem Befehl US kann einer Weltbaut ein neuer Versorger zuge-
  * wiesen werden. Dies muss eine Stadt sein, und zwar eine vom ei-
  * genen Staat. Die Stadt muss vom Besitzer der Weltbaut (transitiv
  * aber nicht reflexiv!) besessen werden.
  *
  * @param
  * char *befehl: US%s mit der Objektabkuerzung des neuen Versorgers.
  *
  * @return
  * short 1.
  ---------------------------------------------------------------------------*/
short WELTBAUT::befehl_ueberstellen(char *befehl)
{
  if (!befehl[2]) {
    report(L("US: Der Befehl verlangt die Angabe der Abk~urzung einer Stadt.\n","AN: That command requires the abbreviation of a town.\n"));
    return 1;
  }

  OBJEKT *versorger = objekt_mit_namen(befehl+2);
  if (!versorger || mystrcmp(versorger->attribut("TYP"), "STADT"))
  {
    report(L("%s: Es gibt keine Stadt mit der Abk~urzung","%s: There is no town with that abbreviation."), befehl);
    report(" '%s'.\n", befehl+2);
    return 1;
  }

  OBJEKT *alter_staat = staat();

  // Falls die Einrichtung im gleichen Staat bleibt, ist alles ganz einfach.
  if (besitzer->besitzt(versorger))
  {
    attribut_setzen("VERSORGER", versorger->name);
    report(L("Ab jetzt versorgt uns %s. ","From now on, we are supplied by %s. "), versorger->a_name());
    return 1;
  }

  else if (!versorger->kommando("WELTBAUT_UEBERNEHMEN", this)) // 1 = Nein
  {
    // Mein Besitzer ist JETZT bereits der neue Staat.
    alter_staat->report(L("Die Einrichtung %s wird dem Staat ","Structure %s is handed over to the Empire of "), this->name);
    alter_staat->report(L("%s ~ubergeben.\n","%s .\n"), this->besitzer->a_name());
  }
  else {
    report("%s: ", befehl);
    report(L("%s will unsere Einrichtung nicht ~ubernehmen.\n","%s does not want to take possession of our structure.\n"), versorger->a_name());
  }

  return 1;
}


/**---------------------------------------------------------------------------
  * WELTBAUT::befehl_auslieferung_einstellen()
  * 
  * Toggelt zwischen den beiden Zustaenden Auslieferung aktiv und
  * eingestellt.
  * @return
  * short 1.
  ---------------------------------------------------------------------------*/
short WELTBAUT::befehl_auslieferung_einstellen()
{
  if (attribut_gesetzt("KeineAusgabe")) {
    report(L("Die Auslieferung wird wieder freigegeben...\n","Delivery is ON again.\n"));
    attribut_loeschen("KeineAusgabe");
  }
  else {
    report(L("Auslieferung von Ressourcen eingestellt...\n","Delivery of resources OFF.\n"));
    attribut_setzen("KeineAusgabe");
  }
  return 1;
}


/**---------------------------------------------------------------------------
  * Ermittelt die Stadt, welche die Weltbaut versorgt. Dies kann 0 sein,
  * in welchem Falle die Weltbaut unversorgt ist.
  ---------------------------------------------------------------------------*/
STADT *WELTBAUT::versorger()
{
    return (STADT *)(objekt_mit_namen(attribut("VERSORGER")));
}


/**---------------------------------------------------------------------------
  * WELTBAUT::absolute_adresse()
  * 
  * Berechnet aus Spielerkoordinaten die richtigen.
  ---------------------------------------------------------------------------*/
ADR& WELTBAUT::absolute_adresse(char *relativ)
{
  static ADR antwort;

  char *dup = mystrdup(relativ), *z;
  z = dup;
  while (*z) {
    if (tolower(*z) == 'o') *z = '0';
    z++;
  }

  antwort = ADR(staat()->info("ABSOLUTE_ADRESSE",dup));
  myfree(dup);
  return antwort;
}
