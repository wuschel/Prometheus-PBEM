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
  * MODUL:               bemaske.C / BEMASKE.CPP
  * AUTOR/DATUM:         Mathias Kettner, 1. Juli 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Implementiert die Funktionen der Klasse BEMASKE. Bemaske heisst
//      Befehlseingabemaske. Sie ist die Bedienungsoberflaeche fuer das
//      Eintippen der Befehlskarte eines Spielers.
//
// *************************************************************************

#include <ctype.h>

#include "staat.h"
#include "bemaske.h"
#include "kompatib.h"
#include "enzyklop.h"
#include "mystring.h"
#include "alg.h"
#include "prom.h"
#include "listmac.h"
#include "miniedit.h"
#include "uhr.h"

extern UHR *g_uhr; // von main.cpp


/**---------------------------------------------------------------------------
  * KONSTANTE:
  ---------------------------------------------------------------------------*/
#define Y_MEMORY_OBEN 4   // Ausmasse des Bereichs, in dem eingetippte...
#define Y_MEMORY_UNTEN 21 // ... Befehlszeilen angezeigt werden
#define Y_MEMORY_MITTE 12


/**---------------------------------------------------------------------------
  * GLOBALE VARIABLE:
  * char *befehlstabelle[]
  * Alle gueltigen Befehle sind hier gespeichert, damit die Maske kon-
  * trollieren kann, ob ein gueltiger Befehl eingetippt wurde. Werde
  * neue Befehle implementiert, so muss diese Tabelle entsprechend
  * erweitert werden.
  ---------------------------------------------------------------------------*/
char *befehlstabelle[] = {

/* Spezial */
    L("AA","CC"),

/* STAAT */
    "*L",
    L("AB","AP"),
    L("KR","IM"),
    L("DM","ID"),
    L("PU","PU"),
    L("TK","IT"),
    L("DS","CD"),
    L("MT","ME"),
    L("AD","DA"),
    L("AM","XC"),
    
/* STADT */
    L("BA","BU"),
    L("AB","AP"),
    L("FB","SP"),
    L("NB","UP"),
    L("NR","RF"),
    L("AR","RB"),
    "*B",
    "*E",
    "*R",
    L("AL","RC"),
    L("NA","CN"),
    L("US","AN"),
    L("BE","RO"),
    L("GV","DC"),
    L("SV","PD"),
    L("AV","UA"),
    
/* EINHEIT */  
    L("RE","MV"),
    L("SG","FC"),
    L("EI","DI"),
    L("AU","DO"),
    L("RV","DM"),
    L("MO","MO"),
    L("ST","CR"),
    L("BA","BS"),
    L("EO","CQ"),
    L("US","AN"),
    L("EN","OC"),
    L("AN","AT"),
    L("FE","FI"),
    L("EM","EM"),
    L("DE","DE"),
    L("XX","DU"),
    L("WR","DF"),
    L("AF","AF"),
    L("TR","DS"),
    L("BL","LO"),
    L("EL","UL"),
    L("AC","CS"),
    L("SW","SS"),
    L("GZ","GT"),
    L("PV","SH"),
    L("EX","EX"),
    L("AS","SE"),
    L("NA","CN"),
    L("BS","SM"),
    L("BO","BO"),
    L("BG","EU"),
    L("WA","WA"),
    L("KP","CP"),
    L("WI","RP"),
    L("UN","ES"),
    L("MB","UM"),
    L("AV","UA"),
    L("VW","RZ"),
    
/* WELTBAUT */ 
    L("US","AN"), 
    L("FE","FI"),
    L("AE","SO"),
    L("XX","RS"),

    NULL /* Markiert das Ende der Tabelle */
};


extern DOUBLIST globale_objekt_menge;

/**---------------------------------------------------------------------------
  * BEMASKE::BEMASKE()            // construktor
  * 
  * Der Aufruf der Befehlsmaske geschieht ueber den Konstruktor! Wird
  * eine neue Variable vom Typ BEMASKE geschaffen, so wird direkt
  * im Konstruktor die ganze Sitzung gefahren. Am Ende des Konstruktors
  * ist die Struktur im Prinzip wertlos und kann sogleich wieder ver-
  * nichtet werden.
  *
  * @param
  * st:     Staat, fuer den Befehle eingegeben werden sollen
  *
  ---------------------------------------------------------------------------*/
BEMASKE::BEMASKE(STAAT *st)
{
  fileviewer = NULL;
  staat = st;
  if (!st) return;

  besitzliste_berechnen();
  maske_aufbauen();
  
  eingabephase();
}


/**---------------------------------------------------------------------------
  * BEMASKE::besitzliste_berechnen()
  * 
  * Wird vom Konstruktor aufgerufen und macht eine Liste all derer
  * Objekte, die der Staat (transitiv und reflexiv) besitzt. Der
  * Staat selbst ist also auch in der Liste.
  ---------------------------------------------------------------------------*/
void BEMASKE::besitzliste_berechnen()
{
  OBJEKT_LIST_NODE *objnode;
  objnode = (OBJEKT_LIST_NODE *)globale_objekt_menge.first();
  while (!objnode->is_tail()) {
    if (staat->besitzt(objnode->objekt) || objnode->objekt==staat) {
      OBJEKT_LIST_NODE *neu = new OBJEKT_LIST_NODE;
      neu->objekt = objnode->objekt;
      besitzliste.add_tail(neu);
    }
    objnode = (OBJEKT_LIST_NODE *)objnode->next();
  }
}


/**---------------------------------------------------------------------------
  * BEMASKE::maske_aufbauen()
  * 
  * Baut die Bildschirmmaske auf und zeigt auch gleich einige Daten
  * ueber den Spieler an.
  ---------------------------------------------------------------------------*/
void BEMASKE::maske_aufbauen()
{
  io_cls();
  char string[200];
  sprintf(string,
L("Befehle eingeben fuer %s %s (%s)","Enter commands for %s %s (%s)"),
      staat->attribut("VORNAME"),staat->attribut("NACHNAME"),staat->name);
  io_centerline(0,string);
  io_line(1);

  io_line(18);
  io_printxy(1,19,L("  Ctrl-O:  Befehle von Diskette      |   Ctrl-E:  Vorerst fertig mit Eintippen","  Ctrl-O:  Read Commands from disk   |   Ctrl-E:  Ready with typing commands"));
  io_printxy(1,20,L("Mit 'Pfeil-hoch' Befehle am Anfang   |   Ctrl-L:  Fehlerhafte Zeile loeschen  ","  Hit 'Up'  to see commands at top   |   Ctrl-L:  Delete incorect line"));
  io_printxy(1,21,L(" der Liste anschauen, wenn noetig.   |   Ctrl-F:  Persoenliche Mitteilung     ","  of the list whenever necessary.    |   Ctrl-F:  Personal Message"));
  io_line(22);
}


/**---------------------------------------------------------------------------
  * BEMASKE::eingabephase()
  * 
  * In dieser Funktion befindet sich die zentrale Schleife. Alle
  * gedrueckten Tasten werden in einer Art endlicher Automat ver-
  * arbeitet. Einzig CTRL-Tasten gelten als Steuertasten und
  * werden nicht in der Transitionsfunktion verarbeitet. Die Pfeiltasten
  * werden an den Fileviewer weitergeleitet.
  ---------------------------------------------------------------------------*/
void BEMASKE::eingabephase()
{
  // Hier wird es etwas komplizierter. Die Eingabe erfolgt interaktiv und
  // wird staendig kontrolliert. Im Prinzip ist das Ganze hier so eine
  // art endlicher Automat.

  zustand = OBJEKTNAME_NOCH_NICHT_FERTIG;
  ef_loeschen();

  memory_laden();

  // Jetzt gehe ich in die Hauptschleife
  for (;;)
  {
    // Zuerst hole ich mir eine Taste

    int taste = io_getch();

    // Die Taste kann entweder eine Steuertaste sein,
    // oder eine Taste, die zum String gehoert, der eingegeben werden
    // soll. Jenachdem springe ich entweder die Transitionsfunktion
    // an oder die Funktion, die die Steuertasten behandelt.

    if (taste==9 || taste==13 || io_isbackspace(taste)
	 || taste >= 32 && taste <= 126)
	transitionsfunktion(taste);

    // Die Steuertastenfunktion liefert 1, wenn die Engabe abgeschlossen
    // werden soll.

    else if (steuertaste_verarbeiten(taste)) break;
    
    else io_beep(); // Fehlerpiepen.
  }
}

/**---------------------------------------------------------------------------
  * BEMASKE::transitionsfunktion()
  * 
  * Realisiert die Uebergangsfunktion des Automaten. Der Automat kennt
  * vier verschiedene Zustaende:
  *
  * OBJEKTNAME_NOCH_NICHT_FERTIG:
  * In diesem Zustand wird jede gedrueckte Taste zum Namen des Obje-
  * ktes hinzugefuegt, fuer das Befehle eingegeben werden sollen.
  * Es wird stets kontrolliert, ob es ein Objekt mit so einem Namen
  * geben kann und nur moegliche Zeichen zugelassen. Sobald der Name
  * eindeutig ist, wird er automatisch vervollstaendigt und es
  * wird in den naechsten Zustand uebergegangen
  *
  * FERTIG_FUER_NAECHSTEN_BEFEHL:
  * Es wird der erste Buchstabe eines Befehls erwartet und anschlies-
  * send in den naechsten Zustand uebergegangen.
  *
  * ERWARTE_ZWEITEN_BUCHSTABEN:
  * Es wird der zweite Buchstabe erwartet. Es wird ueberprueft, ob
  * der Befehl insgesamt existieren wuerde und nur dann zum naechs-
  * ten Zustand uebergegangen.
  *
  * ERWARTE_PARAMETER:
  * Hier werden alle Zeichen ohne weitere Kontrolle angehaengt. Mit
  * der TAB-Taste versucht der Rechner, einen angefangenen Namen
  * aus der Enzyklopaedie zu vervollstaendigen, und macht dies, wenn
  * es geht, bis vor die erste Stelle, an der es nicht mehr ein-
  * deutig ist, welcher von mehreren Namen gemeint ist.
  *
  * @param
  * int taste      Gedrueckte Taste. Mit Backspace wird grundsaetzlich
  * ein Zeichen geloescht. Dabei wird eventuell in
  * einen frueheren Zustand verzweigt.
  ---------------------------------------------------------------------------*/
void BEMASKE::transitionsfunktion(int taste)
{
  // Je nach Zustand, in dem ich mich befinde, leite ich weiter zu
  // einer Unterfunktion...

  switch (zustand) {
    case OBJEKTNAME_NOCH_NICHT_FERTIG:  trans_onnf(taste); break;
    case FERTIG_FUER_NAECHSTEN_BEFEHL:  trans_ffnb(taste); break;
    case ERWARTE_ZWEITEN_BUCHSTABEN:    trans_ezb(taste);  break;
    case ERWARTE_PARAMETER:             trans_ep(taste);   break;
    case QUOTE_MODUS:                   trans_qm(taste);   break;
  }
}


/**---------------------------------------------------------------------------
  * BEMASKE::trans_onnf()
  * 
  * Transitionsfunktion fuer den Zustand OBJEKTNAME_NOCH_NICHT_FERTIG.
  *
  * @param
  * int taste.
  ---------------------------------------------------------------------------*/
void BEMASKE::trans_onnf(int taste)
{
  // Als allererstes in der Zeile gibt des Eingeber den Namen des
  // Objektes ein, das als naechstes Befehle erhalten soll. Die
  // Funktionalitaet soll so sein:

  // Bei jedem Zeichen, das eingegeben wird, wird sofort die Liste all derer
  // Objekte durchsucht, die dem Staat gehoeren. Dann gibt es mehrere Faelle:
  // 1. Der Name kann nicht richtig sein => Taste ignorieren + Piep
  // 2. Das Zeichen kann richtig sein => Zeichen anhaengen und weitermachen.

  // Wird die Backspacetaste gedrueckt, so wird ein Zeichen geloescht,
  // falls eines vorhanden war.

  // Wird die Spacetaste gedrueckt, so gilt die Namenseingabe als abgeschlossen
  // Dann muss der Name kontrolliert werden => annehmen bzw. ignorieren.
  // Statt der Spacetaste kann auch der Doppelpunkt gedrueckt werden, da
  // dieser dann auch auf dem Bildschirm erscheint.

  if (taste < 255 && isalnum(taste)) {
    ef_anfuegen(toupper(taste));
    switch (objektname_vervollstaendigen())
    {
//  case 0: // OK, Naechster Zustand
//      zustand = FERTIG_FUER_NAECHSTEN_BEFEHL;
//      ef_anfuegen(":");
//      return;

      case 1: // Name noch nicht eindeutig
	return;

      case 2: // Name kann garnicht so lauten!
	ef_backspace();
	io_beep();
	return;
    }
  }

  else if (io_isbackspace(taste)) {
    ef_backspace();
    return;
  }

  // So. Nun wird die Spacetaste behandelt.

  else if (taste == ' ')
  {
    // Die Spacetaste funktioniert genau dann, wenn der Name existiert.
    if (!objekt_in_besitz_mit_namen(eingabefeld)) {
      io_beep();
      return; // Falsch
    }
    else {
      zustand = FERTIG_FUER_NAECHSTEN_BEFEHL;
      ef_anfuegen(" ");
      return;
    }
  }
}


/**---------------------------------------------------------------------------
  * BEMASKE::objekt_in_besitz_mit_namen()
  * 
  * Testet, ob sich im Besitzt des Staates ein bestimmtes Objekt be-
  * findet anhand des Namens des Objektes.
  *
  * @param
  * char *name: Name des Objektes.
  *
  * @return
  * 1, falls es existiert und der Staat es besitzt oder selbst ist.
  * 0, sonst.
  ---------------------------------------------------------------------------*/
short BEMASKE::objekt_in_besitz_mit_namen(char *name)
{
  OBJEKT *obj = objekt_mit_namen(name);
  if (!obj) return 0;
  return staat->besitzt(obj) || staat==obj;
}


/**---------------------------------------------------------------------------
  * BEMASKE::objektname_vervollstaendigen()
  * 
  * Wird diese Funktion aufgerufen, so stehen im Eingabefeld einige
  * Zeichen. Diese Funktion versucht, diese auf eindeutige Art
  * zu dem Namen eines Objektes zu vervollstaendigen.
  *
  * AENDERUNG: Es wird nicht mehr versucht, zu vervollstaendigen,
  * sondern nur noch getestet, ob der Name sein kann. Sonst wird
  * der Eintipper noch wahnsinnig...
  *
  * @return
  * ( 0: Erste Buchstaben haben Namen eindeutig bestimmt. )
  * 1: Name ist noch nicht eideutig bestimmbar
  * 2: Anfang muss schon falsch sein, kein Name beginnt mit ihnen
  ---------------------------------------------------------------------------*/
short BEMASKE::objektname_vervollstaendigen()
{
  // Der Name, der im Eingabefeld steht, muss untersucht werden.
  // Alle in Frage kommenden Objekte stehen in der Liste besitz.

  short anzahl_matches = 0;
  // char *objname = NULL;

  OBJEKT_LIST_NODE *objnode = (OBJEKT_LIST_NODE *)besitzliste.first();
  while (!objnode->is_tail()) {
    if (!mystrncmp_no_case(objnode->objekt->name,eingabefeld,strlen(eingabefeld))) { // passt
      // objname = objnode->objekt->name;
      anzahl_matches++;
    }
    objnode = (OBJEKT_LIST_NODE *)objnode->next();
  }

  // Wenn anzahl_matches noch 0 ist, dann kann der Name garnicht so anfangen
  if (!anzahl_matches) return 2;

  // Ansonsten kann ich gruenes Licht geben...
  return 1;

  /*
    ef_anfuegen(objname+strlen(eingabefeld));
    return 0;
  */

}


/**---------------------------------------------------------------------------
  * BEMASKE::trans_ffnb(int taste)
  * 
  * Transitionsfunktion vom Zustand FERTIG_FUER_NAECHSTEN_BEFEHL.
  *
  * @param
  * int taste.
  ---------------------------------------------------------------------------*/
void BEMASKE::trans_ffnb(int taste)
{
  // So. Jetzt gibt es verschiedene Moeglichkeiten. Mit Backspace
  // komme ich zurueck
  if (io_isbackspace(taste))
  {
    // Links von mir ist mit Sicherheit ein Space. Allerdings weiss ich
    // nicht, ob links von diesem Space ein anderer Befehl oder der Objekt-
    // name kommt. Ich muss also ganz nach links gehen und das explizit
    // nachkontrollieren. Wenn links von mir keine weiteres Space kommt,
    // als das unmittelbar links von mir, dann muss es sich um den Objekt-
    // namen handeln.

    char *z = eingabefeld;
    short space_kam = 0;
    while (z < eingabezeiger - 1) {
      space_kam |= (*z == ' ');
      z++;
    }
      
    if (!space_kam)  zustand = OBJEKTNAME_NOCH_NICHT_FERTIG;

    else { 
      zustand = ERWARTE_PARAMETER;
      // Jetzt wird es leider noch ein bisschen komplizierter, da ich
      // momentan nicht weiss, wo der Parameterbereich des letzten Be-
      // fehles beginnt. Dazu suche ich nach links bis zum naechsten
      // Space und gehe dann wieder drei Zeichen nach
      // rechts.
      parameterzeiger = eingabezeiger-2;
      while (*parameterzeiger!=' ') parameterzeiger--;
      parameterzeiger += 3;
    }
    ef_backspace();

    return;
  }

  // Jetzt MUSS ein Kleinbuchstabe kommen, da jeder Befehl mit einem
  // Buchstaben beginnt, oder ein '*', mit dem die Spezialbefehle beginnen.
  // Grossbuchstaben erzeuge ich automatisch. Der Tipper soll aber die
  // Taste ohne SHIFT druecken, damit es schneller geht.

  if (!islower(taste) && taste !='*') {
    io_beep();
    return;
  }

  // Jetzt kontrolliere ich aber noch, ob es ueberhaupt einen Befehl gibt,
  // der mit dem Buchstaben beginnt.

  if (!befehl_beginnt_mit(toupper(taste))) {
    io_beep();
    return;
  }

  // So. Ich haenge den Buchstaben an und gehe in den naechsten Zustand
  // ueber...

  ef_anfuegen(toupper(taste));
  zustand = ERWARTE_ZWEITEN_BUCHSTABEN;
}


/**---------------------------------------------------------------------------
  * BEMASKE::trans_ezb()
  * 
  * Transitionsfunktion vom Zustand ERWARTE_ZWEITEN_BUCHSTABEN.
  *
  * @param
  * int taste
  ---------------------------------------------------------------------------*/
void BEMASKE::trans_ezb(int taste)
{
  if (io_isbackspace(taste)) { // Das ist einfach zu handeln...
    ef_backspace();
    zustand = FERTIG_FUER_NAECHSTEN_BEFEHL;
    return;
  }

  // Nur Buchstaben sind sinnvoll

  if (!islower(taste)) {
    io_beep();
    return;
  }

  // Hier kontrolliere ich, ob es einen Befehl gibt, der so lautet, wie
  // es der Eingeber will.
  char befehl[3];
  befehl[0] = *(eingabezeiger-1);
  befehl[1] = toupper(taste);
  befehl[2] = 0;

  if (!befehl_existiert(befehl)) {
    io_beep();
    return; // War nichts, gibt's nich'
  }

  ef_anfuegen(toupper(taste));

  if (!strcmp(befehl,L("MT","ME"))) zustand = QUOTE_MODUS;
  else zustand = ERWARTE_PARAMETER;
  parameterzeiger = eingabezeiger;
  return;
}


/**---------------------------------------------------------------------------
  * BEMASKE::befehl_beginnt_mit()
  * 
  * Testet, ob es einen Befehl gibt, der mit einem bestimmten Zeichen
  * beginnt.
  *
  * @param
  * char zeichen: Fragliches Zeichen
  *
  * @return
  * 1, falls ja, sonst 0.
  ---------------------------------------------------------------------------*/
short BEMASKE::befehl_beginnt_mit(char zeichen)
{
  for (int v=0; befehlstabelle[v]; v++)
  {
    if (befehlstabelle[v][0] == zeichen) return 1;
  }
  return 0;
}


/**---------------------------------------------------------------------------
  * BEMASKE::befehl_existiert()
  * 
  * Testet, ob es einen bestimmten Befehl gibt.
  *
  * @param
  * char *befehl:    Fraglicher Befehl.
  *
  * @return
  * 1, falls ja, sonst 0.
  ---------------------------------------------------------------------------*/
short BEMASKE::befehl_existiert(char *befehl)
{
  for (int v=0; befehlstabelle[v]; v++)
  {
    if (!strcmp(befehlstabelle[v],befehl)) return 1;
  }
  return 0;
}


/**---------------------------------------------------------------------------
  * BEMASKE::trans_ep()
  * 
  * Transitionsfunktion vom Zustand ERWARTE_PARAMETER.
  *
  * @param
  * int taste
  ---------------------------------------------------------------------------*/
void BEMASKE::trans_ep(int taste)
{
  // Mit Backspace kann man Zeichen wegmachen, wobei man unter Umstaenden
  // in einen vorhergehenden Zustand geraet. Drueckt man Tabulator, so
  // versucht der Computer, den Namen eines Vorhabens zu vervollstaendigen.
  // Druckt man den Schlusspunkt, so kommt stattdessen ein Doppelpunkt.
  // Aus dem Doppelkreuz mache ich einen Schraegstrich, damit man
  // diesen auf der deutschen Tastatur ohne Shift erreichen kann.

  if (taste == '.') taste = ':';
  if (taste == '#') taste = '/';

  if (io_isbackspace(taste)) {
    if (*parameterzeiger==0) { // Leer ==> zurueck
      zustand = ERWARTE_ZWEITEN_BUCHSTABEN;
      ef_backspace();
      return;
    }
    else ef_backspace();
    return;
  }

  else if (taste == ' ') { // Befehl abschliessen
    ef_anfuegen(' ');
    zustand = FERTIG_FUER_NAECHSTEN_BEFEHL;
    return;
  }

  else if (taste == 13 || taste == 10) { // Befehlszeile abschliessen
    memory_aufnehmen(eingabefeld);
    memory_speichern();
    memory_laden();
    ef_loeschen();
    zustand = OBJEKTNAME_NOCH_NICHT_FERTIG;
    return;
  }

  else { // Andere Zeichen werden einfach angehaengt, und zwar in Grossbuchst.
    if (!isprint(taste)) {
      io_beep();
      return; // Keine Steuerzeichen!
    }
    if (taste == '.') taste = ':';
    else if (taste == '#') taste = '/';
    ef_anfuegen(toupper(taste));
  }
}


/**---------------------------------------------------------------------------
  * BEMASKE::trans_qm()
  * 
  * Modus, indem alle Tasten direkt uebernommen werden, und in dem
  * man auch Leerzeichen und Kleinbuchstaben tippen kann. Wird fuer
  * den Befehl MT (Mitteilung) benoetigt.
  * @param
  * int Gedrueckte Taste
  ---------------------------------------------------------------------------*/
void BEMASKE::trans_qm(int taste)
{
  // Mit Backspace kann man Zeichen wegmachen, wobei man unter Umstaenden
  // in einen vorhergehenden Zustand geraet. Drueckt man Tabulator, so
  // versucht der Computer, den Namen eines Vorhabens zu vervollstaendigen.
  // Druckt man den Schlusspunkt, so kommt stattdessen ein Doppelpunkt.

  if (taste == ' ') taste='_'; 

  if (io_isbackspace(taste)) {
    if (*parameterzeiger==0) { // Leer ==> zurueck
      zustand = ERWARTE_ZWEITEN_BUCHSTABEN;
      ef_backspace();
      return;
    }
    else ef_backspace();
    return;
  }

  else if (taste == 13 || taste == 10) { // Befehlszeile abschliessen
    memory_aufnehmen(eingabefeld);
    memory_speichern();
    memory_laden();
    ef_loeschen();
    zustand = OBJEKTNAME_NOCH_NICHT_FERTIG;
    return;
  }

  else { // Andere Zeichen werden einfach angehaengt, und zwar in Grossbuchst.
    if (!isprint(taste)) {
      io_beep();
      return; // Keine Steuerzeichen!
    }
    ef_anfuegen(taste);
  }
}


/**---------------------------------------------------------------------------
  * BEMASKE::steuertaste_verarbeiten()
  * 
  * Folgende vier Steuertasten sind momentan belegt:
  *
  * CTRL-E: Ende. Geht nur im Zustand EP oder bei leerer Zeile.
  * CTRL-L: Befehlszeile erfragen und loeschen
  * CTRL-F: Persoenliche Mitteilung an den Spieler editieren.
  * CTRL-O: Befehle von Diskette holen.
  * Cursor hoch und runter: Fileviewer verschieben.
  *
  * @param
  * int taste: Tastencode von io_getch();
  *
  * @return
  * 1, wenn die Maske beendet werden soll.
  * 0, wenn nicht.
  ---------------------------------------------------------------------------*/
short BEMASKE::steuertaste_verarbeiten(int taste)
{
  switch(taste)
  {
    case 5:  // CTRL-E
      if (zustand != ERWARTE_PARAMETER 
	  && eingabezeiger != eingabefeld) {
	io_beep();
	return 0;
      }
      return 1;

    case 12: // CTRL-L
      zeile_loeschen();
      ef_refresh();
      return 0;
   
    case 6: // CTRL-F
      persoenliche_mitteilung();
      maske_aufbauen();
      memory_laden();
      ef_refresh();
      return 0;
    
    case 15: // CTRL-O
      befehle_von_diskette();
      ef_refresh();
      return 0;
   }
      
   if (io_iscursorup(taste)) fileviewer->scroll(-1);
   else if (io_iscursordown(taste)) fileviewer->scroll(1);
   return 0;
}


/**---------------------------------------------------------------------------
  * BEMASKE::zeile_loeschen()
  * 
  * Fuehrt den Befehl Ctrl-L aus, mit dem fehlerhafte Zeilen geloescht
  * werden koennen. Der Benutzer gibt eine Abkuerzung an, und es werden
  * alle Zeilen der Abkuerzung geloescht.
  ---------------------------------------------------------------------------*/
void BEMASKE::zeile_loeschen()
{
  io_deleteline(19,21);
  io_printxy(0,20,L("Ich loesche alle Zeilen mit der Abkuerzung ->      <-","I delete all lines containing the abbreviation ->      <-"));
  char abk[10];
  io_gotoxy(L(45, 49) ,20);
  io_readline(abk,6);
  
  if (*abk)
  {
    MEMORY *mem;
    SCAN(&memory, mem)
    {
      char *p1 = mem->zeile, *p2 = abk;
      while (toupper(*p1) == toupper(*p2)) { p1++; p2++; }
      if (*p1 == ' ' && *p2 == 0) {  // Dann war's gleich
	delete mem;
	FIRST(&memory, mem);
      }
      else NEXT(mem);
    }
    memory_speichern();
  }
  
  maske_aufbauen();
  memory_laden();
  
}
  

/**---------------------------------------------------------------------------
  * BEMASKE::ef_loeschen()
  * 
  * Loescht das Eingabefeld sowohl optisch als auch intern.
  ---------------------------------------------------------------------------*/
void BEMASKE::ef_loeschen()
{
  eingabefeld[0] = 0;
  eingabezeiger = &eingabefeld[0];
  io_gotoxy(X_EINGABEFELD, Y_EINGABEFELD);
  for (int i=0; i<EFLAENGE; i++) io_print(" ");
  io_gotoxy(X_EINGABEFELD, Y_EINGABEFELD);
}

/**---------------------------------------------------------------------------
  * BEMASKE::ef_anfuegen(..1..)
  * 
  * Fuegt optisch und intern ein Zeichen an den Eingabepuffer an.
  *
  * @param
  * char c: Zeichen zum Anhaengen
  ---------------------------------------------------------------------------*/
void BEMASKE::ef_anfuegen(char c)
{
  if (eingabezeiger >= eingabefeld+EFLAENGE) return; // schon voll
  *eingabezeiger++ = c;
  *eingabezeiger = 0;
  char ministring[2];
  ministring[0]=c;
  ministring[1]=0;
  io_printxy(eingabezeiger-eingabefeld+X_EINGABEFELD-1, Y_EINGABEFELD,ministring);
}

/**---------------------------------------------------------------------------
  * BEMASKE::ef_anfuegen(..2..)
  * 
  * Fuegt einen String an das Eingabefeld.
  *
  * @param
  * char *string: Anzufuegenden String.
  ---------------------------------------------------------------------------*/
void BEMASKE::ef_anfuegen(char *string)
{
  while (*string) ef_anfuegen(*string++);
}

/**---------------------------------------------------------------------------
  * BEMASKE::ef_backspace()
  * 
  * Loescht das letzte Zeichen vom Eingabepuffer intern und am
  * Bildschirm.
  ---------------------------------------------------------------------------*/
void BEMASKE::ef_backspace()
{
  if (eingabezeiger == eingabefeld) return; // Schon alles weg.
  io_printxy(eingabezeiger-eingabefeld+X_EINGABEFELD-1, Y_EINGABEFELD," ");
  eingabezeiger--;
  *eingabezeiger = 0;
  io_gotoxy(eingabezeiger-eingabefeld+X_EINGABEFELD, Y_EINGABEFELD);
}


/**---------------------------------------------------------------------------
  * BEMASKE::ef_refresh()
  * 
  * Zeichnet die Eingabezeile neu.
  ---------------------------------------------------------------------------*/
void BEMASKE::ef_refresh()
{
  io_printxy(0, Y_EINGABEFELD, eingabefeld);
}


/**---------------------------------------------------------------------------
  * BEMASKE::memory_laden()
  * 
  * Laedt die Befehlsdatei von Platte in die Memoryliste und zeigt
  * das File mit dem Fileviewer auf dem Bildschirm an.
  ---------------------------------------------------------------------------*/
void BEMASKE::memory_laden()
{
  memory.clear(); // Erstmal alle alten Eintraege loeschen....

  char *dateiname = g_uhr->befehlsdateiname(staat);
  if (!fileviewer) fileviewer = new FILEVIEWER(dateiname, 2,17);
  else fileviewer->reload();
  fileviewer->ende();

  // Folgende beiden Zeilen setzen den Cursor neu.
  ef_anfuegen(' ');
  ef_backspace();
  
  FILE *file = fopen(dateiname,"r");
  if (!file) return;
  
  while (!feof(file)) {
    char puffer[200];
    puffer[0]=0;
    fgets(puffer,199,file);
    
    // Nun muss ich noch abschliessende CR und LF entfernen. Fuck!
    char *ende = puffer + strlen(puffer) - 1;
    while ((*ende == 13 || *ende == 10) && ende>puffer) {
      *ende=0;
      ende--;
    }

    if (strlen(puffer) > 3) memory_aufnehmen(puffer);

  }
  fclose(file);
}
    

/**---------------------------------------------------------------------------
  * BEMASKE::memory_aufnehmen()
  * 
  * Nimmt eine Befehlszeile in den Speicher auf
  *
  * @param
  * char *zeile: Aufzunehmende Zeile
  ---------------------------------------------------------------------------*/
void BEMASKE::memory_aufnehmen(char *zeile)
{
  MEMORY *neu = new MEMORY(zeile);
  memory.add_tail(neu);
}


/**---------------------------------------------------------------------------
  * BEMASKE::memory_speichern()
  * 
  * Speichert die ganze Befehlsliste in das Befehlsfile fuer diese
  * Runde, diese Session und diesen Staat.
  ---------------------------------------------------------------------------*/
void BEMASKE::memory_speichern()
{
  char *filename = g_uhr->befehlsdateiname(staat);
  verzeichnis_gewaehrleisten(filename);
  FILE *file = fopen(filename,"w");
  if (!file) {
    io_deleteline(6,16);
    io_centerline(6,L("Fehler! Ich kann die Datei","Error! Cannot open"));
    io_centerline(8,filename);
    io_centerline(10,L("fuer die Befehle nicht oeffnen!","turnsheet file!"));
    io_centerline(14,L("Die neu eingetippten Befehle gehen verloren!","The lines just typed in will get lost!"));
    io_centerline(16,L("Weiter mit 'Eingabe'","Hit 'Return' to continue"));
    io_getch();
    return;
  }
  
  MEMORY *mem;
  SCAN(&memory, mem)
  {
    fprintf(file, "%s\n",mem->zeile);
    NEXT(mem);
  }
  fclose(file);
}


/**---------------------------------------------------------------------------
  * BEMASKE::persoenliche_mitteilung()
  * 
  * Mit einem Minieditor kann der Benutzer eine kleine Mitteilung
  * fuer den Spieler schreiben, dessen Befehle er gerade eintippt.
  ---------------------------------------------------------------------------*/
void BEMASKE::persoenliche_mitteilung()
{
  io_cls();
  char string[90];
  sprintf(string,L("Persoenliche Mitteilung an %s %s (%s)","Personal Message to %s %s (%s)"),
    staat->attribut("VORNAME"), staat->attribut("NACHNAME"), staat->name);
  io_centerline(1, string);

  char *filename = g_uhr->persoenliche_mitteilungdateiname(staat);

  MINIEDITOR me(filename, 3,7,74,11);

  io_printxy(9,19,L("CTRL-E: Ende und abspeichern                 CTRL-A: Abbrechen","CTRL-E: Quit and Save                 CTRL-A: Cancel"));
  io_printxy(9,20,L("Umlaute koennen mit ~a ~o ~u ~A ~O ~U und ~s eingegeben werden.",""));

  me.edit();
}


/**---------------------------------------------------------------------------
  * BEMASKE::befehle_von_diskette()
  * 
  * Holt Befehle von einer Datei auf der Diskette.
  ---------------------------------------------------------------------------*/
void BEMASKE::befehle_von_diskette()
{
  io_mount_floppy();
  FILE *file = diskettendatei_oeffnen();
  if (!file) {
    io_umount_floppy();
    return;
  }

  while (!feof(file)) {
    char puffer[200];
    puffer[0]=0;
    fgets(puffer,199,file);
    
    // Nun muss ich noch abschliessende CR und LF entfernen. Fuck!
    char *ende = puffer + strlen(puffer) - 1;
    while ((*ende == 13 || *ende == 10) && ende>puffer) {
      *ende=0;
      ende--;
    }

    if (strlen(puffer) > 3) memory_aufnehmen(puffer);
  }
  
  fclose(file);
  io_umount_floppy();
  
  memory_speichern();
  maske_aufbauen();
  memory_laden();
  
}

/**---------------------------------------------------------------------------
  * BEMASKE::diskettendatei_oeffnen()
  * 
  * Oeffnet eine Datei /a/befehle bzw. A:\BEFEHLE
  ---------------------------------------------------------------------------*/
FILE *BEMASKE::diskettendatei_oeffnen()
{
  char filename[256];
  sprintf(filename,"%sbefehle",io_floppy_path());
  FILE *file = fopen(filename,"r");
  if (!file) {
    io_deleteline(24);
    char string[310];
    sprintf(string, L("Die Datei %s konnte nicht geoeffnet werden <Taste>.","Could not open File %s  <Press a key>."), filename);
    io_centerline(22, string);
    io_getch();
    io_line(22);
  }
  return file;
}
