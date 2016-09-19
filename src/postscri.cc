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
  * MODUL:               postscri.C / POSTSCRI.CPP
  * AUTOR/DATUM:         Mathias Kettner, 28. Maerz 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Definiert diejenigen Funktionen von LAYOUT und deren verbuendeten 
//      Klassen, die direkt den Drucker ansteuern. Dabei werden Postscript
//      Programme erzeugt. Mit anderen Worten ist dies hier der Postscript-
//      Druckertreiber fuer Prometheus. postscri.o und pcl5.o schliessen ein-
//      ander gegenseitig aus. Beim Linken muss man sich also fuer einen
//      bestimmten Drucker entscheiden.
//
// *************************************************************************

#include <string.h>

#include "layout.h"
#include "drucker.h"
#include "alg.h"
#include "kompatib.h"
#include "attribut.h"


/**---------------------------------------------------------------------------
  * Die Konfigurationsangabe ist eine kleine Funktion, die einen Text
  * zurueckgibt, der dann auf dem Titelbildschirm ausgegeben wird.
  ---------------------------------------------------------------------------*/

char *version_druckertreiber()
{
  return "Adobe Postscript Level II";
}

// fuer BITMATRIX::ausdrucken()

ATTRIBUT_LISTE bitmatrix_cache;

long aktuelle_seite = 0; // Dient zur Seitenzaehlung fuer %%
void fusszeile(); // Druckt Seitennummern.
bool duplex_druck = false;

float aktuelle_fontgroesse = -1;
char *aktueller_font = NULL; // So'ne Art Fontcache

float aktuelle_position = 0.0;
float aktueller_zeilenabstand = 0.5; // cm 
const float seitenhoehe = 27.00; // cm DIN-A4 mit Rand

// Umrechnungsfunktionen. Das Koordinatensystem von Postscript ist standard-
// gemaess auf 1/72.72 Zoll geeicht. Ich eiche es auf 300dpi, damit meine
// Koordinatenangaben ohne Nachkommastellen stehen koennen.

char *boundingbox = "%%BoundingBox: 49 0 2393 3329\n";
float units(float cm) { return 300 * cm / 2.54; };
float cm(float units) { return 2.54 * units / 300; };
float yctos(float cm) { return units(1.25 + seitenhoehe - cm); };
float xctos(float cm) { return units(cm + 0.455); };
float ctos(float cm)  { return units(cm); };

void seiteabschliessen() { 
  drucken("showpage\nrestore\n"); 
  bitmatrix_cache.clear(); // Alles verlorengegangen, da restore
}

void seitebeginnen() 
{ 
  aktuelle_seite++; 
  drucken("%%%%Page: %ld %ld\nsave\n", aktuelle_seite, aktuelle_seite);
  myfree(aktueller_font);
  aktueller_font = NULL;
  aktuelle_fontgroesse = -1;
  
  // Und nun eine kleine Fusszeile drucken...
  fusszeile();
}

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void put_units(float units) {
  static char string[32];
  sprintf(string,"%ld ",long(units+0.5));
  drucken(string);
}

void put_koords(float x, float y) {
  put_units(xctos(x));
  put_units(yctos(y));
}

void put_koords_descaled(float x, float y, float scale) {
  put_units(xctos(x)/scale);
  put_units(yctos(y)/scale);
}
  
void put_koords(OFFSET &off) { put_koords(off.x, off.y); };
void moveto(OFFSET &off) { put_koords(off); drucken("m "); };

void schriftart_einstellen(char *zeichensatz, float pt)
{
  if (pt == aktuelle_fontgroesse && !mystrcmp(zeichensatz, aktueller_font)) 
    return; // Font aendert sich nicht.
  
  drucken("%s ",myftoa(pt*4.1254));
  
  // Hier habe ich das Problem, dass die Auswahl des Zeichensatzes im
  // Programm ueber Namen geschieht, ersteres aber keine Information ueber
  // die tatsaechlich angebotenen Zeichensaetze hat. Deshalb gehe stelle ich
  // genau drei Zeichensaetze zur Verfuegung, die immer vorhanden sein muessen:
  // Courier, Times und eine serifenlose proportionalschrift, die bei HP
  // Univers und bei Postscript Helvetica heisst. Im Zweifelsfall nehme
  // ich halt irgendeine Schrift, damit wenigstens etwas kommt.

  if (!mystrcmp(zeichensatz, "Helvetica") || !mystrcmp(zeichensatz, "Univers"))
    drucken("sh\n");
  
  else if (!mystrcmp(zeichensatz, "Times"))
    drucken("st\n");
  
  else if (!mystrcmp(zeichensatz, "Courier")
	|| !mystrcmp(zeichensatz,"Letter Gothic")) {
    if (pt >= 9) drucken("scb\n");
    else drucken("sc\n");
  }
  
  else drucken("sh\n");
  
  aktuelle_fontgroesse = pt;
  myfree(aktueller_font);
  aktueller_font = mystrdup(zeichensatz); // merken.
}


void dr_zeilenabstand(float cm)
{
  // Den Zeilenabstand merke ich mir als internes Register in cm.
  // Zum Drucker schicke ich an dieser Stelle noch nichts.
  aktueller_zeilenabstand = cm;
}


/**---------------------------------------------------------------------------
  * SCHRIFTZUG::ausdrucken()
  * 
  * Beim Ausdrucken eines Schriftzuges muss ich, im Gegensatz zu PCL5,
  * die Zeilenumbrueche selbst berechnen. D.h. ich muss bei jedem LF
  * (CR ignoriere ich einfach) wieder an den Anfang der Zeile, aber
  * den Zeilenabstand hinzuzaehlen. Letzteren habe ich in eine globalen
  * Variable gespeichert.
  ---------------------------------------------------------------------------*/
void SCHRIFTZUG::ausdrucken(float links, float oben)
{
  schriftart_einstellen(zeichensatz, pt);

  links += offset.x;
  oben += offset.y;
  char *textzeiger = text; // Arbeitszeiger.

  while (*textzeiger) {
    put_koords(links, oben);
    drucken("(");
    while (*textzeiger && *textzeiger!='\n') { // Zeichen ausgeben
      if (*textzeiger == '(') drucken("\\050");
      else if (*textzeiger == ')') drucken("\\051");
      else drucken(*textzeiger);
      textzeiger++;
    }
    drucken(") t\n");
    
    // Wenn der String tatsaechlich zuende war, dann breche ich ab.
    // Ich zaehle dabei gleich noch eins weiter, wall ich im Falle
    // eines Nicht-Endes das LF unbedingt ueberlesen muss. Wenn doch 
    // das Ende war, dann stoert es ja auch nicht.

    if (!*textzeiger++) break; // String ist zuende.
    
    // Hier muss ich in die naechste Zeile rutschen. Wie weit diese von
    // der alten entfernt ist, besagt eine globale Variable, deren ein-
    // ziger Sinn genau eben gerade und nur diese einzige Zeile ist, die
    // nun folgt.
    
    oben+=aktueller_zeilenabstand;
  }
}


void LINIE::ausdrucken(float links, float oben)
{
  OFFSET anfang(links+offset.x, oben+offset.y);
  OFFSET ende(anfang);
  ende.addiere(endpunkt);
  put_koords(anfang);
  put_koords(ende);
  drucken("l\n"); // Makro l im Header als Linienbefehl
}


/**---------------------------------------------------------------------------
  * TORTE::ausdrucken()
  * 
  * Druckroutine fuer die Tortengrafiken
  ---------------------------------------------------------------------------*/
void TORTE::ausdrucken(float links, float oben)
{
  OFFSET mitte(links+offset.x, oben+offset.y);

  float summe = 0.0; // Zum normieren muss ich erst die Summe berechnen
  short i;
  for (i=0; i<anzahl; i++) summe += eintraege[i];

  // Jetzt geht's los. Ich beginne eine Schleife. Aber vorher fahre ich
  // mit dem Stift in die Mitte.

  float summe_bis_jetzt = 0.0;
  for (i=0; i<anzahl; i++) {
    float startwinkel = -(summe_bis_jetzt / summe) * 360;
    float endwinkel = startwinkel -(eintraege[i]/summe) * 360;

    drucken("gsave newpath ");
    moveto(mitte);
    put_koords(mitte);
//     const float pi = 3.14592654;
    put_units(units(radius));
    drucken("%s %s arcn closepath %s setgray gsave fill grestore stroke grestore\n" ,myftoa
	  (startwinkel), myftoa(endwinkel),
	   myftoa(1 - float(i+.5)/float(anzahl)));
    summe_bis_jetzt += eintraege[i];
  }
}

void put_hexstring(unsigned char *daten, long laenge)
{
  static char string[10];

  for (int i=0; i<laenge; i++) {
    unsigned char c=*daten++;
    sprintf(string,"%02X",c);
    drucken(string);
  }
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void BITMATRIX::ausdrucken(float links, float oben)
{
  if (!daten) return; // Matrix ungueltig
  OFFSET lo(links, oben);
  lo.addiere(offset);

  // Ich erziele nun mit relativ geringem Aufwand eine riesengrosse
  // Wirkung. Eine neue Grafik schicke ich nur beim ersten mal als
  // Daten zum Drucker. Dabei definiere ich sie im Drucker als Makro
  // (Bei Postscript kein Problem) und beziehe mich bei den spaeteren
  // malen darauf. Dazu brauche ich eine Liste von allen Grafiken,
  // die schon beim Drucker sind. Diese Liste ist eine Attributliste
  // und heisst bitmatrix_cache. Der Dateiname steht in dateiname.
  
  if (!bitmatrix_cache.gesetzt(dateiname))
  {
     // Jetzt muss ich das neue Makro definieren. Als Namen nehme ich
     // den Grafikdateinamen.
     
     drucken("/%s { ",dateiname);
     drucken("{ <"); // Zuerst kommen die Imagedaten als String
     put_hexstring(daten, (breite+7)/8 * hoehe);
     drucken("> } ");
     drucken("%ld %ld g } def\n", breite, hoehe); // Ausmasse
     
     // { /wiese2.gra { (234kjfhj\012fsadihjfsad...) } 48 48 g } def

     bitmatrix_cache.setzen(dateiname); // Merken, dass schonmal gedruckt.

  }
  
  // Und jetzt drucke ich unter Verwendung des Makros.

  // Zuerst muss ich an die Scalierung denken. Wenn sie ungleich 1 ist,
  // dann muss ich mit scale die Skalierung umdefinieren, aber gleich-
  // zeitig daran denken, dass die Koordinaten davon auch betroffen wer-
  // den. Deshalb muss ich diese durch die Scale teilen.
  
  if (scale != 1) { // gsave 5.00000 5.0000 scale 40.234 200.234 wiese2.gra grestore
    drucken("gsave %s ", myftoa(scale));
    drucken("%s scale ", myftoa(scale));
    put_koords_descaled(lo.x, lo.y, scale);
    drucken(dateiname);
    drucken(" grestore\n");
  }

  else {   // 234 1045 wiese2.gra
    put_koords(lo);
    drucken(dateiname);
    drucken("\n");
  }

}


void RECHTECK_AUSGEFUELLT::ausdrucken(float links, float oben)
{
  OFFSET lo (offset.x + links, offset.y + oben);
  OFFSET ru (rechts_unten.x + lo.x, rechts_unten.y + lo.y);
  put_koords(lo);
  put_koords(ru);
  drucken(" %s r\n",myftoa(1.0-(float)schattierung/100));
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void dr_anfang(bool duplex)
{
    duplex_druck = duplex;
    aktuelle_position = 0.0;
    drucken("%!PS-Adobe-2.0 ExitServer\n");
    drucken("%%Title: Mathias Kettner's Prometheus Prolog\n");
    drucken("%%Creator: Mathias Kettner's Prometheus\n");
    drucken("%%For: Player\n");
    drucken("%%Pages: (atend)\n");
    drucken("%%CreationDate: Tue Mar 29 1994\n");
    drucken("%%DocumentFonts: Courier Courier-Bold Helvetica Times-Roman\n");
    drucken(boundingbox);
// Wenn ich die naechste Zeile wegmache, dann funktioniert psselect... ?!
//  drucken("%%BeginDocument: Prometheus\n");
    drucken("%%EndComments\n");
    drucken("save .2424 .2424 scale\n");
    drucken("0 setlinewidth\n");
    drucken("/l { newpath moveto lineto stroke } def\n"); // Linie
    drucken("/t { 3 1 roll moveto show } def\n"); // Text
    drucken("/r { newpath gsave setgray 4 copy 3 1 roll exch 6 2 roll moveto\n");
    drucken("     lineto lineto lineto closepath fill grestore } def\n");
    drucken("/f { exch findfont exch scalefont setfont } def\n");
    drucken("/g { gsave 0 setgray 5 -2 roll translate true [1 0 0 -1 0 0] 5 -1 roll imagemask grestore } def\n");
    drucken("/m { moveto } def\n");
    drucken("/Umlaut /Courier findfont /Encoding get 256 array copy\n");
    drucken("dup 132 /adieresis put dup 142 /Adieresis put  dup 148 /odieresis put\n");
    drucken("dup 153 /Odieresis put dup 129 /udieresis put  dup 154 /Udieresis put\n");
    drucken("dup 225 /germandbls put def\n\n");
    drucken("/MakeUmlautFont\n  { findfont dup length dict /newdict exch def\n");
    drucken("    { 1 index /FID ne\n      { newdict 3 1 roll put }\n");
    drucken("      { pop pop }\n      ifelse\n");
    drucken("    } forall\n    newdict /Encoding Umlaut put\n");
    drucken("    newdict definefont pop\n  } def\n\n");
    drucken("/UCourier /Courier MakeUmlautFont\n");
    drucken("/UCourier-Bold /Courier-Bold MakeUmlautFont\n");
    drucken("/UHelvetica /Helvetica MakeUmlautFont\n");
    drucken("/UTimes-Roman /Times-Roman MakeUmlautFont\n");
    drucken("/sc { /UCourier findfont exch scalefont setfont } def\n");
    drucken("/scb { /UCourier-Bold findfont exch scalefont setfont } def \n");
    drucken("/sh { /UHelvetica findfont exch scalefont setfont } def\n");
    drucken("/st { /UTimes-Roman findfont exch scalefont setfont } def\n\n");
    drucken("%%EndProlog\n\n");
    
    bitmatrix_cache.clear(); // Alles verlorengegangen, da neue Druckdatei.
    
    aktuelle_seite=0; // Muss jedesmal zurueckgesetzt werden.
    seitebeginnen();
    
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void dr_auswurf()
{
  // Wenn ich noch mitten in einer Seite bin, dann muss ich diese ab-
  // schliessen.
  
  if (aktuelle_position) seiteabschliessen();

  // Im Duplexbetrieb muss ich bei ungerade Seitenzahl noch eine
  // Leerseite auswerfen.

  if (duplex_druck && aktuelle_seite % 2) {
      seitebeginnen();
      seiteabschliessen();
  }

  drucken("%%Trailer\n");
  drucken("restore\n");
  drucken("%%%%Pages: %ld\n",aktuelle_seite);
  aktuelle_position = 0.0;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
float dr_abschnitt(float hoehe)
{
  float rwert;

  if (aktuelle_position + hoehe < seitenhoehe) {
    rwert = aktuelle_position;
    aktuelle_position += hoehe;
  }
  else { // Kein Platz mehr->neue Seite.
    seiteabschliessen();
    seitebeginnen();
    rwert = 0.0;
    aktuelle_position = hoehe;
  }
  return rwert;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void dr_neueseite()
{
  if (aktuelle_position > 0.0) {
    seiteabschliessen();
    seitebeginnen();
    aktuelle_position = 0;
  }
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void io_printer_flush()
{
}

void fusszeile()
{
  schriftart_einstellen("Times",7);
  put_units(units(19.4));
  char string[20];
  sprintf(string,L("90 (Seite %ld) t\n","90 (Page %ld) t\n"),aktuelle_seite);
  drucken(string);
}
