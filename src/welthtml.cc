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


// Erzeugung der Weltkarte als HTML-Files

#include <stdio.h>

#include "welt.h"
#include "laengen.h"
#include "staat.h"
#include "kompatib.h"
#include "listmac.h"
#include "alg.h"
#include "prom.h"
#include "html.h"
#include "log.h"
#include "maxmin.h"

const char *schichtname[2] = { "welt", "infr" };

void link_auf_schicht(HTML *html, short schicht, short teil_x, short teil_y)
{
    char htmlname[128];
    sprintf(htmlname, "%s%02d%02d",schichtname[schicht], teil_x, teil_y);
    html->href(htmlname, "top");
}

    
void WELT::weltkarte_html(STAAT *staat)
{

  LANDSCHAFTSABBILD *abbild = landschaftsabbild(staat);
  if (!abbild) {
      log('I', "WELT::weltkarte_html(): Landschaftsabbild von Staat "
	  "%s nicht vorhanden", staat->name);
      return;
  }
  
  // Es soll ein moeglichst kleiner Rechteckiger Bereich gefunden werden,
  // der die ganze bekannte Welt beinhaltet. Das macht folgender Aufruf.
  // in mitte_x soll die X-Koordinate von der Spalte stehen, die in der
  // Mitte sein soll (Startpunkt des Spielers).

  ADR mitte = staat->koordinaten_ursprung();

  long xstart, xende, ystart, yende; // Transientparameter
  abbild->benutzten_bereich_ermitteln(xstart, ystart, xende, yende, mitte.x);

  const short UEBERLAPPUNG = 3;

  // Der Spieler kann mit dem Befehl AMW einstellen, wie gross sein Bildschirm
  // ist.

  int MAX_FELDER_BREITE = myatol(staat->attribut("KARTENBREITE")) - 2*UEBERLAPPUNG;
  if (MAX_FELDER_BREITE <= 0) MAX_FELDER_BREITE = 18;

  int MAX_FELDER_HOEHE = myatol(staat->attribut("KARTENHOEHE")) - 2*UEBERLAPPUNG;
  if (MAX_FELDER_HOEHE <= 0) MAX_FELDER_HOEHE = 9;

  int teilhoehe = yende-ystart+1;
  int ganze_yportionen = teilhoehe / MAX_FELDER_HOEHE;
  int resthoehe = teilhoehe % MAX_FELDER_HOEHE;
  int ydruckende = yende;
  short teil_y = 0;
  short von_y = ganze_yportionen + (resthoehe ? 1 : 0);
  

  int teilbreite; // Auszudruckende Breite.
  if (xende >= xstart) teilbreite = xende-xstart+1;
  else teilbreite = breite - xstart + xende + 1;

  while (ganze_yportionen || resthoehe)
  {
    teil_y++;
  
    int xdrucklinks = xstart;
    int ganze_xportionen = teilbreite / MAX_FELDER_BREITE;
    int restbreite = teilbreite % MAX_FELDER_BREITE;
    short teil_x = 0;
    short von_x = ganze_xportionen + (restbreite ? 1 : 0);

    while (ganze_xportionen || restbreite)
    {
      teil_x++;
      int xdruckrechts;
      if (ganze_xportionen) { // Dann ganze Breite ausnuetzen
         xdruckrechts = (xdrucklinks + MAX_FELDER_BREITE - 1) % breite;
         xdrucklinks %= breite;
         ganze_xportionen--;
      }
      else { // Restlichen Bereich
        xdruckrechts = (xdrucklinks + restbreite - 1) % breite;
        restbreite = 0;
      }

      if (ganze_yportionen) {
	teilmatrix_drucken_html(abbild, staat, 
			 xdrucklinks,  ydruckende-MAX_FELDER_HOEHE+1,
			 xdruckrechts, ydruckende,
			 teil_x, von_x, teil_y, von_y, UEBERLAPPUNG);
      }
      else {
	teilmatrix_drucken_html(abbild, staat, 
			 xdrucklinks, ystart, 
			 xdruckrechts, ydruckende,
			 teil_x, von_x, teil_y, von_y, UEBERLAPPUNG);
	resthoehe = 0;
      }
      xdrucklinks += MAX_FELDER_BREITE;
    }   // x-Schleife

    ydruckende -= MAX_FELDER_HOEHE;
    if (ganze_yportionen) ganze_yportionen--;

  } // y-Schleife
}

void WELT::teilmatrix_drucken_html(LANDSCHAFTSABBILD *abbild, 
       STAAT* staat, int xstart, int ystart, int xende, int yende,
       short teil_x, short von_x, short teil_y, short von_y, short ueberlappung)
{
    // teilbreite und teilhoehe geben die Breite und Hoehe des hier auszu-
    // druckenden Bereiches an (in Feldern).
    
    long teilbreite;
    if (xende >= xstart) teilbreite = xende-xstart+1;
    else teilbreite = breite - xstart + xende + 1;
//    long teilhoehe = yende-ystart+1;

    // Ich mache eine Ueberlappung der Bereiche um einige Felder.
    // Wenn die teilbreite zusammen mit der Ueberlappung groesser waere
    // als die Gesamtbreite, dann muss ich die Ueberlappung kleiner machen.
    
    if (teilbreite + 2*ueberlappung > breite)
    {
	xstart = (xstart - (breite-teilbreite) / 2 + breite) % breite;
	xende  = (xende + breite - teilbreite - (breite-teilbreite) / 2) % breite;
	teilbreite = breite;
    }
    else {
	xstart = (xstart - ueberlappung + breite) % breite;
	xende  = (xende + ueberlappung) % breite;
	teilbreite += 2 * ueberlappung;
    }
    ystart = max(0, ystart - ueberlappung);
    yende  = min(hoehe-1, yende + ueberlappung);
    

    // Die Weltkarte wird in zwei Schichten erzeugt. Die obere Schicht enthaelt
    // Gelaendeformen und Einheiten. Die untere Schicht Strassen, Bodenschatzsuche,
    // Gelaendeformen nur durch Farben repraesentiert.

    HTML *html[2];
    #define BEIDE(s) for (int s = 0; s<2; s++)
    BEIDE(s) html[s] = html_anlegen(staat, s, teil_x, von_x, teil_y, von_y);

    // Ein Anker direkt auf die Tabelle. Fuer Leute mit kleinem Bildschirm (640x480)

    BEIDE(s) html[s]->anchor("top");
    
    // Und nun geht's los!
   
    BEIDE(s) html[s]->table();

    // Nun gebe ich den Bereich an, in dem sich die relativen X-Koordinaten
    // bewegen sollen, die der Spieler ausgedruckt bekommt.
    
    ADR mitte = staat->koordinaten_ursprung();
    
//    long xbera = (xstart > mitte.x) ? xstart-breite-mitte.x : xstart - mitte.x;
//    long xbere = breite/2; // Weiter darf es nicht steigen.
//    long ybera = yende-mitte.y; // Subjektive Y-Koordinaten, Anfang
//    long yr = -1; // Absteigene Skalierung bei Y.
    
    // Die beiden Schleifen laufen auf objektiven Koordinaten.

    for (long zeile = yende+1; zeile >= ystart-1; zeile--) 
    {
	BEIDE(s) html[s]->next_row();

	short kopf_oder_fuss = (zeile == yende+1 || zeile==ystart-1);
	if (!kopf_oder_fuss)
	{
	    BEIDE(s) {
		html[s]->set_cell_alignment(AL_CENTER).next_cell(" ")
		.text(zeile - mitte.y).text(" ").unset_cell_alignment();
	    }
	}
	else BEIDE(s) html[s]->next_cell();
	
	for (long spalte = xstart; spalte < xstart+teilbreite; spalte++)
	{
	    if (kopf_oder_fuss) 
	    {
		int xkoord = spalte - mitte.x;
		if (xkoord > breite/2) xkoord -= breite;
		else if (xkoord <= -(breite/2)) xkoord += breite;
		if (xkoord % 2 == 0) {
		    BEIDE(s) 
			html[s]->set_cell_alignment(AL_CENTER).next_cell(xkoord)
			.unset_cell_alignment();
		}
		else BEIDE(s) html[s]->next_cell();
		continue;
	    }
	    
	    // Beim Zugriff auf die Matrizen muss ich die Spalte
	    // stets modulo breite nehmen, da der Bereich unter Umstaenden falsch
	    // umgebrochen ist.
	    
	    long x = (spalte+breite) % breite;
	    long y = zeile; // Y ist real und unveraendert.
	    ADR adr(x, y); // Vorbereiten

	    // Endlich habe ich die Adresse des zu zeichnenden Feldes. Jetzt
	    // muss ich mir nur noch um das Feld selbst kuemmern.

	    // Gelaendeform und Feldattribute ermitteln.
	    
	    unsigned short feldwert = abbild->matrix[x][y];
	    short gform = feldwert & 0xff;

	    // Falls in der bit_matrix ein Eintrag ist, so wird nicht
	    // die Gelaendeform sondern eine Markierung dargestellt.
	    // Die hoechste Prioritaet haben die immobilen Objekte.
	    
	    unsigned char immobilmaske = abbild->bit_matrix[x][y] & LA_IMMOBILMASKE;
	    unsigned char mobilmaske   = abbild->bit_matrix[x][y] & LA_MOBILMASKE;
	    
	    // Staedte und Weltbauten vorbereiten.

	    OBJEKT *stadt_oder_weltbaut = NULL;
	    if (immobilmaske == LA_STADT) { // Gilt auch fuer Weltbaut
		
		// Je nach Gattung der Stadt wird ein anderes Symbol dargestellt.
		// Damit ich ermittlen kann, welche Stadt dort steht, schaue ich
		// nochmal in der Objektemenge nach. Dadurch ergibt sich aller-
		// dings folgendes Phaenomen: Ueber die Groesse bleibt ein Spieler
		// auf dem laufenden. Das Weltbild ist an dieser Stelle also 
		// Objektiv. Sollte die Stadt inzwischen aufgeloest worden
		// sein, so ergiebt sich ein besonderes Problem.
		
		FOR_EACH_OBJEKT_IN (alle_objekte_bei_adresse(adr))
		    DO_AND_DELETE ({
			if (objekt->typ_ist("STADT") || objekt->typ_ist("WELTBAUT"))
			stadt_oder_weltbaut = objekt;
			});
	    }
	    bool stadt_hier    = stadt_oder_weltbaut && stadt_oder_weltbaut->typ_ist("STADT");
	    bool weltbaut_hier = stadt_oder_weltbaut && stadt_oder_weltbaut->typ_ist("WELTBAUT");
	    bool eigene_stadt_oder_weltbaut =
		stadt_oder_weltbaut && staat->besitzt(stadt_oder_weltbaut);

	    // Einheiten vorbereiten.

	    OBJEKT *einheit = NULL;
	    if (!stadt_oder_weltbaut && (mobilmaske & LA_EINHEIT))
	    {
		// In der mobilmaske ist nur vermerkt, dass ueberhaupt eine Einheit
		// an dieser Stelle gesehen wird. Die genauen Daten ueber alle Ein-
		// heiten hole ich mir aber von der realen Matrix der Welt.
		
		DOUBLIST *einheitenliste = alle_objekte_bei_adresse(adr, "TYP=EINHEIT");
		if (!einheitenliste->is_empty())
		    einheit = ((OBJEKT_LIST_NODE *)einheitenliste->first())->objekt;
		delete einheitenliste;
	    }
	    bool eigene_einheit = einheit && staat->besitzt(einheit);
		
	    // Bevor ich fuer beide Schichten die neue Zelle aufmache, muss ich
	    // fuer Schicht 2 evtl. die Hintergrundfarbe auf den richtigen Wert
	    // setzen.
	    
	    if (gform) { // Bekannt.
		const char *farbe = (stadt_hier ? "#ffffff"
			       : (weltbaut_hier ? "#c0c0c0" : 
				  gelaendeform(gform)->attribute.abfragen("FARBE")));
		html[1]->set_cell_color(farbe);
	    }

	    // Jetzt fuer beide Schichten eine neue Zelle
	    BEIDE(s) html[s]->next_cell();

	    html[1]->unset_cell_color();

	    // Nun kommen die Links auf die Nachbarseiten. Alle Felder
	    // am Rand der Karte beinhalten einen Link. Ausgeschlossen
	    // davon ist die Schicht 0 bei Feldern mit eigenen Einheiten oder Staedten.

	    bool href_offen[2] = { false, false };
	    short link_x = 0;
	    short link_y = 0;
	    if      (zeile == yende && teil_y > 1)      link_y = -1;
	    else if (zeile == ystart && teil_y < von_y) link_y = +1;
	    if      (spalte == xstart)                  link_x = -1;
	    else if (spalte == xstart + teilbreite - 1) link_x = +1;
	    if (link_x != 0 || link_y != 0)
	    {
		short ziel_x = ((teil_x + link_x - 1 + von_x) % von_x) + 1;
		short ziel_y = teil_y + link_y;
		html[1]->set_image_border(0);
		link_auf_schicht(html[1], 1, ziel_x, ziel_y);
		href_offen[1] = true;
		if (!eigene_stadt_oder_weltbaut && !eigene_einheit)
		{
		    html[0]->set_image_border(0);
		    link_auf_schicht(html[0], 0, ziel_x, ziel_y);
		    href_offen[0] = true;
		}
	    }

	    // Schicht 1: Strassen und Bodenschatzsuche.
	    
	    if (gform)
	    {
		// Jetzt kommt der Strassenwahnsinn.
		const int strmaske = (0x8000 >> FELD_ATTRIBUT_STRASSE);
		const int bsmaske = (0x8000 >> FELD_ATTRIBUT_BODEN_UNTERSUCHT);
		unsigned short **mat = abbild->matrix; // Kleine Abkuerzung.
		unsigned char filewert = 0; // Es gibt 512 Dateien! Sie sind in Hex nummeriert.
		if (mat[x][y] & strmaske)
		{
		    if (y>0 && (mat[x]                         [y-1] & strmaske)) filewert += 0x01;
		    if (y<hoehe-1 && (mat[x]                   [y+1] & strmaske)) filewert += 0x02;
		    if ((mat[(x-1+breite)%breite]              [y]   & strmaske)) filewert += 0x04;
		    if ((mat[(x+1)%breite]                     [y]   & strmaske)) filewert += 0x08;
		    if (y>0 && (mat[(x+1)%breite]              [y-1] & strmaske)) filewert += 0x10;
		    if (y<hoehe-1 && (mat[(x+1)%breite]        [y+1] & strmaske)) filewert += 0x20;
		    if (y<hoehe-1 && (mat[(x-1+breite)%breite] [y+1] & strmaske)) filewert += 0x40;
		    if (y>0 && (mat[(x-1+breite)%breite]       [y-1] & strmaske)) filewert += 0x80;
		    
		    html[1]->streetimage(filewert, (mat[x][y] & bsmaske) != 0);
		}

		else if (mat[x][y] & bsmaske) html[1]->streetimage("bsc");
		else                          html[1]->streetimage("nix");

	    }
	    else                              html[1]->fieldimage("unknown");

		
	    // Schicht 0: Staedte und Weltbauten.

	    if (stadt_oder_weltbaut)
	    {
		static char filename[MAX_LAENGE_DATEINAME];
		sprintf(filename, "%c%s", eigene_stadt_oder_weltbaut ? 'e' : 'g',
			stadt_oder_weltbaut->attribut("GRAFIK"));
		if (eigene_stadt_oder_weltbaut)
		{
		    html[0]->set_image_border(0);
		    html[0]->href_objekt(stadt_oder_weltbaut);
		    html[0]->smallimage(filename, stadt_oder_weltbaut->name);
		    html[0]->end_href().unset_image_border();
		}
		else html[0]->smallimage(filename);
		continue;
	    } //  Stadt oder Weltbaut
	    
	    // Schicht 0: Einheiten

	    else if (einheit)
	    {
		char filename[MAX_LAENGE_DATEINAME];
		sprintf(filename,"%c%s", eigene_einheit ? 'e' : 'g', einheit->attribut("GRAFIK"));
		if (eigene_einheit) {
		    html[0]->set_image_border(0).href_objekt(einheit);
		    html[0]->smallimage(filename, einheit->name);
		    html[0]->end_href().unset_image_border();
		}
		else html[0]->smallimage(filename);
	    }

	    // Schicht 0: Gelaendeformen. 

	    else if (gform) html[0]->fieldimage(gelaendeform(gform)->attribute.abfragen("GRAFIK"));

	    // Schicht 0: Unbekannt.

	    else       html[0]->fieldimage("unknown"); // leere Zelle

	    BEIDE(s) if (href_offen[s]) html[s]->end_href().unset_image_border();

	} // for (Jedes Feld in der Zeile)
	
	if (!kopf_oder_fuss)
	{
	    BEIDE(s) html[s]->set_cell_alignment(AL_CENTER).next_cell(" ")
		.text(zeile - mitte.y).text(" ").unset_cell_alignment();
	}
	
    } // for (Jede Zeile)
    BEIDE(s) {
	html[s]->end_table();
	delete html[s];
    }
}




HTML *WELT::html_anlegen(STAAT *staat, short schicht, int teil_x, int von_x,
			 int teil_y, int von_y)
{
    char *schichtbezeichnung[2] =
    { L("Gel~andeformen und Objekte","Territories and Objects"),
      L("Stra~sen und Bodensch~atze","Roads and Mineral Resources") };
    char htmlname[512], titel[512];
    sprintf(htmlname,"%s%02d%02d", schichtname[schicht], teil_x, teil_y);
    sprintf(titel, L("Weltkarte: %s Teil %d/%d","World Map: %s Clipping %d/%d"), schichtbezeichnung[schicht], teil_x, teil_y);
    HTML *html = new HTML(staat, htmlname,titel);
    html->buttonpanel(0, NULL, NULL);
    
    html->set_image_border(0);
    html->set_table_border(0).set_cell_spacing(0).set_cell_padding(0);
    html->table();
    for (int y=1; y<=von_y; y++)
    {
	html->next_row();
	for (int x=1; x<=von_x; x++) {
	    html->next_cell();
	    if (x == teil_x && y == teil_y) html->iconimage("welthier" );
	    else {
		sprintf(htmlname, "%s%02d%02d", schichtname[schicht], x, y);
		html->href(htmlname,"top").iconimage("weltdort").end_href();
	    }
	}
    }
    html->end_table();

    // Link auf die andere Schicht.
    link_auf_schicht(html, 1-schicht,teil_x,teil_y);
    html->text(schichtbezeichnung[1-schicht]).end_href();
	    
    html->set_table_border(0).set_cell_spacing(0).set_cell_padding(0);
    html->set_image_border(0);

    return html;
}

