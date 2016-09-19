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
  * MODUL:               userint.C  / USERINT.CPP
  * AUTOR/DATUM:         Mathias Kettner, 3. Juni 1994
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Funktionen der Uhr, die fuer das neue Userinterface noetig sind.
//
// *************************************************************************

#include <ctype.h>

#include "uhr.h"
#include "kompatib.h"
#include "fileview.h"
#include "prom.h"
#include "staat.h"
#include "listmac.h"
#include "bemaske.h"
#include "maske.h"
#include "mystring.h"
#include "log.h"

extern short spielstand_modifiziert; // von main.cpp


#define UI_ANZAHLPUNKTE 11
char *ui_menupunkt[UI_ANZAHLPUNKTE] = 
{
  L("N ... Neuen Spieler aufnehmen","A ... Add new player"),
  L("A ... Spielerdaten aendern","C ... Change player attributes"),
  L("L ... Spieler wieder entfernen","R ... Remove player"),
  L("L ... Spieler ist ausgeschieden","R ... Player retreats"),
  L("T ... Tabelle aller Spieler anzeigen","T ... Table of players"),
  L("B ... Befehlsboegen eintippen","E ... Enter players' commands"),
  L("S ... Naechste Runde starten","S ... Start next turn"),
  L("S ... Aufstellungszug starten","S ... Start setup turn"),
  L("W ... Weltkarte drucken","W ... Generate world maps"),
  L("F ... Feld aendern","F ... Change square"),
  L("E ... Programm beenden","Q ... Quit program")
};

short ui_menupunkt_aktiv[UI_ANZAHLPUNKTE];
short ui_anzahl_aktiv = 0;
short ui_zeile = 0;
short ui_mp = 0;

#define UI_MENUOBEN 7
#define UI_MENULINKS 18
#define UI_MENUBREITE 44

short ui_ja_nein()
{
  while (1) {
    int taste = tolower(io_getch());
    if (taste == L('j','y')) return 1;
    else if (taste == L('n','n')) return 0;
  }
}


/**---------------------------------------------------------------------------
  * UHR::ui_menu_aufbauen()
  * 
  * Malt das Hauptmenu auf den Bildschirm und berechnet fuer jeden
  * Menupunkt, ob er zur Zeit auswaehlbar ist oder nicht. Entsprechend
  * werden die Flags im Feld ui_menupunkt_aktiv[] gesetzt oder nicht.
  * Es werden nur auswaehlbare Punkte angezeigt. Die Anzahler der aus-
  * waehlbaren Punkte wird in ui_anzahl_aktiv global gespeichert.
  * Zur Maske gehoert auch ein Vermerkt, wieviel Befehle diese Runde
  * schon eingetippt wurden.
  ---------------------------------------------------------------------------*/
void zeichne_menupunkt(short zeile, char *text, int aktiv=0)
{
  if (aktiv) io_setattr(IO_BOLD);
  io_printxy(UI_MENULINKS, zeile, "    ");
  io_print(text);
  for (int i=strlen(text); i<UI_MENUBREITE-4; i++) io_print(" ");
  if (aktiv) io_setattr(IO_NORMAL);
}
  
void UHR::ui_menu_aufbauen()
{
    bool spielende = spiel_ist_zuende();
    
    // Konstantes Layout
    io_cls();
    io_centerline(2,"P-R-O-M-E-T-H-E-U-S");
    io_centerline(17,L("Waehlen Sie mit Pfeiltasten/Eingabe oder durch Buchstabentasten","Choose with cursor keys/enter or by typing the letters"));
    
    // Status
    long anzahl_spieler;
    DOUBLIST *spielerliste = alle_staaten();
    anzahl_spieler = spielerliste->number_of_elements();
    delete spielerliste;
    
    io_line(20);
    if (ist_spielstand_modifiziert()) io_printxy(4,20,L("modifiziert","modified"));
    char string[120];
    
    sprintf(string,L("Partie \"%s\" in Runde %ld mit %ld Spielern","Game \"%s\" in turn %ld with %ld players")
	    ,info_sessionname(), spielrunde, anzahl_spieler);
    io_centerline(21,string);
    
    if (!spielende) 
    {
	sprintf(string,L("Allgemeine Mitteilungen in %s","Messages for all players in %s"), allgemeine_mitteilungdateiname());
	io_centerline(22,string);
	
	long befehle=0, anzsp=0;
	ui_befehle_zaehlen(befehle, anzsp);
	sprintf(string,L("%ld Befehle von %ld Spielern eingetippt","%ld commands of %ld players have been entered"), befehle, anzsp);
	io_centerline(23,string);
    }
    
    else {
	io_centerline(22,L("!!! DAS SPIEL IST ZUENDE !!!","!!! GAME OVER !!!"));
	sprintf(string,L("Liste der Gewinner in \"%s_gew.txt\"","List of winners is in \"%s_gew.txt\""),info("SESSIONNAME"));
	io_centerline(23,string);
    }
    
    // Ich muss bei jedem Menupunkt ueberlegen, ob er aktiv ist oder nicht.
    // Dazu muss ich wissen, ob ich in der ersten Runde des Spiels bin,
    // und, ob es schon Spieler gibt.
    
    short erste_runde = spielrunde == 1;
    short spieler_vorhanden = anzahl_spieler != 0;
    
    // Jetzt baue ich das Menu auf...
    
    ui_menupunkt_aktiv[0] = erste_runde; //  && !spielende;
    ui_menupunkt_aktiv[1] = spieler_vorhanden; // && !spielende;
    ui_menupunkt_aktiv[2] = spieler_vorhanden && erste_runde;
    ui_menupunkt_aktiv[3] = spieler_vorhanden && !erste_runde; // && !spielende;
    ui_menupunkt_aktiv[4] = spieler_vorhanden;
    ui_menupunkt_aktiv[5] = !erste_runde && spieler_vorhanden; // && !spielende;
    ui_menupunkt_aktiv[6] = !erste_runde && spieler_vorhanden; // && !spielende;
    ui_menupunkt_aktiv[7] = erste_runde && spieler_vorhanden; // && !spielende;
    ui_menupunkt_aktiv[8] = 1;
    ui_menupunkt_aktiv[9] = 1;
    ui_menupunkt_aktiv[10] = 1;
    
    short zeile = UI_MENUOBEN;
    ui_anzahl_aktiv = 0;
    for (int mp=0; mp < UI_ANZAHLPUNKTE; mp++) {
	if (ui_menupunkt_aktiv[mp]) {
	    zeichne_menupunkt(zeile, ui_menupunkt[mp]);
	    ui_anzahl_aktiv++;
	    zeile++;
	}
    }
}
   
   
/**---------------------------------------------------------------------------
  * UHR::ui_befehle_zaehlen()
  * 
  * Zaehlt, von wievielen Spielern schon Befehle eingetippt wurden,
  * und wieviele Befehle dies tatsaechlich sind.
  * @param
  * Referenzen auf Anzahl der Befehle und Anzahl der Spieler.
  ---------------------------------------------------------------------------*/
void UHR::ui_befehle_zaehlen(long& befehle, long& anzsp)
{
  befehle = anzsp = 0;
  
  // Ich laufe in einer Schleife durch alle Staaten
  FOR_EACH_OBJEKT_IN (alle_staaten())
  DO_AND_DELETE
  (
    ui_zaehle_befehle_von(objekt, befehle, anzsp);
  )
}


/**---------------------------------------------------------------------------
  * UHR::ui_zaehle_befehle_von()
  * 
  * Hilfsfunktion, welche die Befehl von einem Staat zaehlt.
  ---------------------------------------------------------------------------*/
void UHR::ui_zaehle_befehle_von(OBJEKT *staat, long& befehle, long& anzsp)
{
  // Datei oeffnen, bzw. schauen, ob sie ueberhaupt existiert.
  char *filename = befehlsdateiname((STAAT *)staat);
  FILE *file = fopen(filename,"r");
  if (!file) return; // Noch keine Befehle
  
  anzsp++;
  // Nun zaehle ich einfach die Spaces in der Datei, genau vor jedem
  // Befehl ein solches steht und sonst nirgends!
  
  while (!feof(file) && !ferror(file)) {
    char c = fgetc(file);
    if (c == ' ') befehle++;
  }
  fclose(file);
}

  
/**---------------------------------------------------------------------------
  * UHR::ui_auswahl()
  * 
  * Stellt das Hauptmenu dar und laesst den Benutzer einen Punkt aus-
  * waehlen. Dessen Nummer in der Tabelle ui_menupunkt[] wird zurueck-
  * gegeben. Wer mit der Nummer nicht gluecklich ist, kann mit
  * ui_menupunkt[ui_auswahl()][0] auch den Anfangsbuchstaben erfragen.
  ---------------------------------------------------------------------------*/
short UHR::ui_auswahl()
{
  ui_menu_aufbauen();
  
  // Die Auswahl erfolgt mit den Cursortasten und Return oder mit
  // den Anfangsbuchstaben. Entweder muss ich den aktuellen Punkt
  // neu setzen...

  if (ui_zeile == 0 || !ui_menupunkt_aktiv[ui_mp]) {
    ui_zeile = UI_MENUOBEN;
    ui_mp = 0;
    while (!ui_menupunkt_aktiv[ui_mp])  ui_mp++;
  }
  
  // ... oder ich behalte den Menupunkt vom letzten Mal bei, der
  // aber seine Position geaendert haben kann. Ich muss daher die 
  // Zeile neu berechnen.
  
  else {
    ui_zeile = UI_MENUOBEN;
    for (int i=0; i < ui_mp; i++) if (ui_menupunkt_aktiv[i]) ui_zeile++;
  }
    
  while (1) {

    // Nun den aktuellen Punkt hiliten.
    
    io_printxy(UI_MENULINKS - 4, ui_zeile,">>>");
    zeichne_menupunkt(ui_zeile, ui_menupunkt[ui_mp], 1);
    io_printxy(UI_MENULINKS + UI_MENUBREITE+1, ui_zeile,"<<<");

    int taste = io_getch();
    if (io_iscursorup(taste) || io_iscursordown(taste)) 
    {
      // Menupunkt wechseln..
      io_deleteline(ui_zeile);
      zeichne_menupunkt(ui_zeile, ui_menupunkt[ui_mp], 0);
      
      if (io_iscursorup(taste)) // Nach oben...
      {
	ui_zeile--;
	if (ui_zeile < UI_MENUOBEN) { // Wieder ganz nach unten.
	  ui_zeile = UI_MENUOBEN + ui_anzahl_aktiv - 1;
	  ui_mp = UI_ANZAHLPUNKTE-1;
	  while (!ui_menupunkt_aktiv[ui_mp]) ui_mp--;
	}
	else do { ui_mp--; } while (!ui_menupunkt_aktiv[ui_mp]);
      }
      
      else // nach unten...
      {
	ui_zeile++;
	if (ui_zeile >= UI_MENUOBEN + ui_anzahl_aktiv) { // Wieder nach oben
	  ui_zeile = UI_MENUOBEN;
	  ui_mp = 0;
	  while (!ui_menupunkt_aktiv[ui_mp]) ui_mp++;
	}
	else do { ui_mp++; } while (!ui_menupunkt_aktiv[ui_mp]);
      }
    } // Cursorbewegung
     
    else if (taste == 13) return ui_mp;
    
    else { // Anfangsbuchstaben...
      for (int auswahl=0; auswahl<UI_ANZAHLPUNKTE; auswahl++)
      {
	if (ui_menupunkt_aktiv[auswahl] && 
	    toupper(taste) == toupper(ui_menupunkt[auswahl][0])) return auswahl;   
      }
    }
    
    // Alle anderen Tasten werden einfach ignoriert.
  }
  
  // Hierhin kann es nie kommen.
}


/**---------------------------------------------------------------------------
  * UHR::ui_spielertabelle()
  * 
  * Laesst den User durch eine Tabelle aller Spieler browsen.
  ---------------------------------------------------------------------------*/
void UHR::ui_spielertabelle()
{
  io_cls();
  io_centerline(0,L("Tabelle aller Spieler","Table of players"));

  // Die Liste der Spieler wird zunaechst in eine temporaere Datei
  // geprintet. Diese kann dann am Bildschirm angezeigt oder gedruckt
  // werden.
  
  verzeichnis_gewaehrleisten(tempfile_name()); 
  FILE *out = fopen(tempfile_name(),"w");
  if (!out) {
    io_centerline(12,L("Probleme mit den Verzeichnissen - Aktion nicht durchfuehrbar","Can't open tempfile - action can't be performed"));
    io_centerline(14,L("Druecken Sie eine Taste, um fortzufahren","Press any key to continue"));
    io_getch();
    return;
  }
  
  io_printxy(0,2,L("Abk.  Nachname      Vorname    Strasse            Wohnort            Telefon\n","Abr.  Name          Surname    Street             City               Phone\n"));
  io_line(3);
  io_line(21);
  io_centerline(23,L("Auschnitt mit Pfeiltasten bewegen, wenn fertig 'Eingabe' druecken","Move visible area with cursor keys. Press <Enter> when finished."));

  FOR_EACH_OBJEKT_IN (alle_staaten())
  DO_AND_DELETE
  (  
    STAAT *staat = (STAAT *)objekt;
    fprintf(out,"%-3s   %-13s %-10s %-18s %-18s %-12s\n",staat->name,
      staat->attribut("NACHNAME"),staat->attribut("VORNAME"),
      staat->attribut("STRASSE"),staat->attribut("WOHNORT"),
      staat->attribut("TELEFON"));
  )
  
  fclose(out);

  // Und nun kommt der Fileviewer, mit dem ich das Ergebnis anschauen kann.
  FILEVIEWER view(tempfile_name(), 4, 20);
  view.interact(13); // 13: Ende durch Returntaste
}

  
/**---------------------------------------------------------------------------
  * UHR::ui_spieler_erfragen()
  * 
  * Fragt den User in der Mitte des Bildschirms nach der Abkuerzung
  * fuer einen Spieler. Ueberprueft das Vorhandensein eines Spielers
  * mit der Abkuerzung. Drueckt der User nur Return, so will er damit
  * die Aktion abbrechen.
  * @return  
  * OBJEKT *  Zeiger auf einen Staat, bzw. NULL, wenn der User
  * abgebrochen hat.
  ---------------------------------------------------------------------------*/
OBJEKT *UHR::ui_spieler_erfragen()
{
    io_centerline(11,L("Geben Sie die Abkuerzung fuer einen Spieler ein","Enter the abbreviation of a player"));
    io_centerline(15,L(" Druecken Sie 'Eingabe', um die Aktion abzubrechen!"," Press <Enter> to cancel"));
   char abk[8];
   
   OBJEKT *spieler = NULL;
   do {
     io_centerline(13,"->   <-");
     io_gotoxy(39,13);
     io_readline(abk, 3);
     if (!abk[0]) break;
     
     spieler = objekt_mit_namen(abk);
     if (!spieler || strcmp(spieler->a_typ(),"STAAT"))
     {
       io_centerline(15,L("Es gibt keinen solchen Spieler! 'Eingabe' um abzubrechen.","No such player. Press <Enter> to cancel."));
       spieler = NULL; // Wegen zweitem Fall
     }
     else if (spieler->attribut_gesetzt("Ausgestiegen"))
     {
       io_centerline(15,L("Der Spieler dieses Staates ist ausgestiegen! 'Eingabe' um abzubrechen","This player has already surrendered. Press <Enter> to cancel."));
       spieler = NULL;
     }
       
   } while (!spieler);

   io_deleteline(11,15);
   return spieler;
}
   
   
/**---------------------------------------------------------------------------
  * UHR::ui_spieler_entfernen()
  * 
  * Hiermit kann der User einen Spieler (= einen Staat) aus dem Spiel
  * entfernen, indem das Objekt geloescht wird. In der ersten Runde
  * bedeutet dies, dass er komplett wieder entfernt wird mit allen
  * seinen Einheiten, der Hauptstadt usw.
  *
  * In den spaeteren Runden jedoch bleibt der Staat erhalten, wird
  * aber von Ein- Ausgabe abgekoppelt. Seine Staedte verteidigen
  * sich autark usw.
  ---------------------------------------------------------------------------*/
void UHR::ui_spieler_entfernen()
{
  short erste_runde = spielrunde == 1;
  io_cls();
  if (erste_runde) io_centerline(2,L("Spieler wieder entfernen","Remove player"));
  else io_centerline(2,L("Spieler steigt aus dem laufenden Spiel aus","Player retreats from game"));
  

  STAAT *staat = (STAAT *)ui_spieler_erfragen();
  if (!staat) return;

  char string[160];
  sprintf(string, L("Soll der Spieler %s %s wirklich %s?","Are you sure that you want player %s %s to %s?"),
    staat->attribut("VORNAME"),staat->attribut("NACHNAME"),
    erste_runde ? L("geloescht werden","be deleted") :
	  L("aussteigen","retreat"));
  io_centerline(12,string);
  io_centerline(14,L("Antworten Sie mit 'ja', wenn Sie das wollen!","Please enter 'yes', if you want this."));
  io_centerline(16,"->    <-");
  io_gotoxy(38,16);
  io_readline(string,4);
  io_deleteline(12,16);
  if (!mystrcmp_no_case(L("ja","yes"),string)) {
    if (erste_runde) {
      delete staat;
      io_centerline(12,L("Der Spieler wurde geloescht!","Player was removed!"));
    }
    else {
      staat->kommando("AUSSTIEG");
      spielstand_ist_nun_modifiziert();
      io_centerline(12,L("Der Spieler ist ausgestiegen, der Staat spiel alleine weiter","The player retreated, but the Empire will continue to play."));
    }
  }
  
  else io_centerline(12,L("Vorgang abgebrochen - Spieler besteht weiterhin...","Canceled, player remains in game..."));
  
  io_centerline(23,L("Weiter mit 'Eingabe'","<Enter> to continue"));
  io_getch();
}

  
/**---------------------------------------------------------------------------
  * UHR::ui_weiter()
  * 
  * Startet die Auswertung. Entweder interaktiv oder automatisch (-y).
  ---------------------------------------------------------------------------*/
short UHR::ui_weiter()
{
    // Wenn Autostart gesetzt ist, frage ich WARTE_AUF_BEFEHLE ab. In diesem
    // Attribut ist die Anzahl der Befehlsboegen, die maximal fehlen darf,
    // damit die Auswertung gestartet wird.

    long anzbef, anzsp;
    ui_befehle_zaehlen(anzbef, anzsp);
    
    // Status
    long anzahl_spieler;
    DOUBLIST *spielerliste = alle_staaten(false, false); // ohne Verlierer und Ausgestiegene
    anzahl_spieler = spielerliste->number_of_elements();
    delete spielerliste;

    if (anzahl_spieler - anzsp > myatol(attribut("WARTE_AUF_BEFEHLE")))
    {
	log('1',"From %ld players I have only %ld turnsheets. I quit and wait for more", anzahl_spieler, 
	    anzsp);
	attribut_setzen("WARTE_AUF_BEFEHLE","warte");
	return 0;
    }

  io_cls();
  if (spielrunde == 1) io_centerline(2,L("Aufstellungszug starten","Evaluate startup turn"));
  else {
    char string[80];
    sprintf(string,L("Runde %ld starten","Evaluate turn %ld"),spielrunde);
    io_centerline(2,string);
  }
  
  char puffer1[9], puffer2[9], *datum1 = puffer1, *datum2 = puffer2;

  if (!attribut_gesetzt("Autostart"))
  {
    io_centerline(10,L("Zuerst muessen Sie die Termine eingeben:","Please enter dates:"));
    io_centerline(12,L("                Heutiges Datum: ->        <-","                   Todays date: ->        <-"));
    io_centerline(13,L("Termin fuer die Befehlsannahme: ->        <-","        Deadline for next turn: ->        <-"));
    io_centerline(19,L("Druecken Sie 'Eingabe' um abzubrechen. Die einzige Funktion der Termine ist","Press <Enter> to cancel. The dates are merely used to inform the players."));
    io_centerline(20,L("die Information der Spieler uebern den Einsendeschluss und ueber die Aktua-","For nicer printouts use  format MM/DD/JJ."));
    io_centerline(21,L("litaet des Ausdruckes. Damit der Ausdruck gut aussieht, geben Sie die Daten",""));
    io_centerline(22,L("am besten in der Form TT.MM.JJ ein.                                        ",""));

    io_gotoxy(52,12);
    io_readline(puffer1,8);
    if (puffer1[0]) {
      io_gotoxy(52,13);
      io_readline(puffer2,8);
    }
  }
  
  else {
    datum1 = attribut("DATUM1");
    datum2 = attribut("DATUM2");
  }
  
  if (attribut_gesetzt("Autostart") || (puffer1[0] && puffer2[0])) {
    strcpy(datum_dieser_auswertung, datum1);
    strcpy(datum_der_naechsten_auswertung, datum2);
    io_cls();
    io_centerline(12,L("Die Auswertung wird nun gestartet, haben Sie bitte viel Geduld.","Evaluation is being started. Please be patient."));
    io_centerline(14,L("Wenn der Zaehler bis 100 gezaehlt hat,  dann werden die","When the counter has reached 100, the printout files will"));
    io_centerline(15,L("Ausdrucke fuer die Spieler angefertigt.","be generated."));

    spielstand_speichern("s");
    spielstand_ist_nun_modifiziert(); // eigentlich: wird bald modifiziert
    return 1;
  }

  io_deleteline(10,23);
  io_centerline(12,L("Vorgang abgebrochen - Runde wird nicht gestartet...","Canceled evaluation."));
  io_centerline(23,L("Weiter mit 'Eingabe'","<Enter> to continue"));
  io_getch();
  return 0;
}


/**---------------------------------------------------------------------------
  * UHR::ui_ende()
  * 
  * Beendet das Programm nachdem alle Daten abgespeichert wurden.
  ---------------------------------------------------------------------------*/
short UHR::ui_ende()
{
    log('U', L("Program was quit without evaluation of next turn","Program was quit without evaluation of next turn"));
    io_cls();
    io_centerline(2,L("Programmende","Quit"));
    io_centerline(12, L("Alle Daten werden gespeichert. Einen Moment...","Saving, one moment please..."));
    spielstand_speichern("q");
    io_centerline(14, L("Speichern abgeschlossen.","Completed saving."));
    return 1;
}
  

/**---------------------------------------------------------------------------
  * UHR::ui_spieler_schaffen()
  * 
  * Erzeugt einen neuen Spieler mit leeren Daten. Die Daten koennen
  * mit ui_spieler_editieren() editiert werden.
  ---------------------------------------------------------------------------*/
OBJEKT *UHR::ui_neuer_spieler()
{
  io_cls();
  io_centerline(3, L("Neuen Spieler aufnehmen","Add new player"));
  io_printxy(10,5,L("Zuerst muessen Sie den Startpunkt auf der Welt angeben.","Please choose a start position."));

  WELT *erde = (WELT *)objekt_mit_namen("ERDE");
  if (!erde) {
    io_cls();
    io_centerline(12,L("FATAL ERROR: Die Erde fehlt!","FATAL ERROR: World missing!"));
    exit(5);
  }
    
  char antwort[6];
  // Als allererstes erfrage ich die Startposition  
  io_printxy(10,7,L("Startposition X-Koordinate:","Starting  Position X-coord:"));
  io_readline(antwort,5);
  if (!antwort[0]) return NULL;
  long x = myatol(antwort);
   
  io_printxy(10,8,L("Startposition Y-Koordinate:","Starting  Position Y-coord:"));
  io_readline(antwort,5);
  if (!antwort[0]) return NULL;
  long y = myatol(antwort);

  ADR start(x,y);
  if (erde->adresse_ist_ausserhalb(start)) {

    io_centerline(15, L("Der Startpunkt ist ausserhalb des gueltigen Bereiches:","This position is out of the valid range:"));
    io_centerline(17, start.to_string());
    io_centerline(19, L("Es wurde kein neuer Spieler eingerichtet.","No player was created."));
    io_getch();
    return NULL;
  }

  if (erde->anzahl_objekte_bei_adresse(start) > 0)
  {
    io_centerline(15, L("An dieser Position der Welt sitzt bereits ein anderer Spieler!","This position is already occupied by another player!"));
    io_centerline(19, L("Es wurde kein neuer Spieler eingerichtet.","No player was created."));
    io_getch();
    return NULL;
  }    

  ATTRIBUT_LISTE attr(L("ENTWICKLUNG1=Pflug,ENTWICKLUNG2=Holzverarbeitung,ENTWICKLUNG3=Stra~senbau,VORNAME=,NACHNAME=,WOHNORT=,STRASSE=,TELEFON=,KONTONR=,NAME=,TITEL=,HERRSCHERNAME=,HAUPTSTADT=,WELTNAME=ERDE","ENTWICKLUNG1=Pflug,ENTWICKLUNG2=Holzverarbeitung,ENTWICKLUNG3=Stra~senbau,VORNAME=,NACHNAME=,WOHNORT=,STRASSE=,TELEFON=,KONTONR=,NAME=,TITEL=,HERRSCHERNAME=,HAUPTSTADT=,WELTNAME=ERDE"));
  attr.setzen("SPIELERNUMMER",myltoa(neue_spielernummer()));
  attr.setzen("START_X",myltoa(x));
  attr.setzen("START_Y",myltoa(y));
  
  OBJEKT *staat = objekt_schaffen(
	 eindeutige_objekt_abkuerzung(L("st","em"),3), "STAAT", attr.to_string());
  if (!staat || staat->attribut("FEHLER")) {
    io_cls();
    io_centerline(12,L("Es konnte kein neuer Spieler eingerichtet werden!","Internal error: No player could be created!"));
    if (staat)  {
      io_centerline(14,staat->attribut("FEHLER"));
      delete staat;
      staat = NULL;
    }
    io_getch();
  }
  log('U', "New player position %s created", staat->name);
  spielstand_ist_nun_modifiziert();
  return staat;
}
  

/**---------------------------------------------------------------------------
  * UHR::ui_spieler_editieren()
  * 
  * Editiert die Daten eines bestehenden Spielers. 
  * @param
  * OBJEKT *staat:
  * Staat, dessen Daten editiert werden sollen. Bei NULL wird
  * garnichts gemacht, bei this wird erst ein Spieler erfragt.
  ---------------------------------------------------------------------------*/
void UHR::ui_spieler_editieren(OBJEKT *staat)
{
  io_cls();
  if (!staat) return;
  else if (staat == this) {
    ui_spieler_editieren(ui_spieler_erfragen());
    return;
  }
  
  // Nun baue ich ein Maskengeruest auf mit lauter schoenen Eingabefeldern,
  // die DIREKT auf den Attributen des Staates operieren!           

  MASKE maske(UHR::ui_se_maske_aufbauen);
  maske.refresh();
  io_printxy(24,1,L("Nr. ","Nr. "));
  io_print(staat->attribut("SPIELERNUMMER"));
  io_printxy(35,3, L("          Abkuerzung: ","        Abbreviation: "));
  io_print(staat->name);

  const short sp_x = 9, sp_y = 3, st_x = 56;

  // Zuerst kommen ein paar Tabellen fuer die Auswahlfelder

    static char *entwicklungen[] =  {
	L("Rad","Wheel"),
	L("Reiten","Riding"),
	L("Bronze","Bronze"),
	L("Pflug","Plow"),
	L("Mathematik","Mathematics"),
	L("Handel","Trade"),
	L("Schrift","Script"),
	L("Holzverarbeitung","Woodworking"),
	L("Milit~ardienst","Military_Service"),
	L("Heilkunde","Medical_Science"),
	L("Stra~senbau","Road_Construction"), NULL };

    static char *ja_nein[] = { L("ja","yes"),
			       L("nein","no"), 0 };

    char *html_ja_nein =
	(staat->attribut_gesetzt("Html"))
	? mystrdup(L("ja","yes"))
	: mystrdup(L("nein","no"));
    
    // Jetzt werden die Felder selbst angelegt

    TEXTFELD name   (&maske, sp_x, sp_y,   20, staat->attribut_referenz("NACHNAME"));
    TEXTFELD vorname(&maske, sp_x, sp_y+1, 15, staat->attribut_referenz("VORNAME"));
    TEXTFELD strasse(&maske, sp_x, sp_y+2, 25, staat->attribut_referenz("STRASSE"));
    TEXTFELD ort    (&maske, sp_x, sp_y+3, 25, staat->attribut_referenz("WOHNORT"));
    TEXTFELD telefon(&maske, sp_x, sp_y+4, 20, staat->attribut_referenz("TELEFON"));
    TEXTFELD kontonr(&maske, sp_x, sp_y+5,  8, staat->attribut_referenz("KONTONR"));
    TEXTFELD email  (&maske, sp_x, sp_y+6, 68, staat->attribut_referenz("EMAIL"));

    AUSWAHLFELD html (&maske, sp_x+10, sp_y+11, 4, &html_ja_nein, html_ja_nein, ja_nein);
    TEXTFELD p_symbol(&maske, sp_x+10, sp_y+12, 15, staat->attribut_referenz("P_SYMBOL"));
    TEXTFELD packer  (&maske, sp_x+10, sp_y+13, 15, staat->attribut_referenz("PACKER"));
    
    TEXTFELD reichname(&maske, st_x, sp_y+1, 24,  staat->attribut_referenz("NAME"));
    TEXTFELD titel(&maske, st_x, sp_y+2, 24,  staat->attribut_referenz("TITEL"));
    TEXTFELD herrscher(&maske, st_x, sp_y+3, 24, staat->attribut_referenz("HERRSCHERNAME"));
    TEXTFELD hauptstadt(&maske, st_x, sp_y+4, 24, staat->attribut_referenz("HAUPTSTADT"));

    AUSWAHLFELD entw1(&maske, 50,14,20,staat->attribut_referenz("ENTWICKLUNG1"),
	NULL, entwicklungen);
    AUSWAHLFELD entw2(&maske, 50,15,20,staat->attribut_referenz("ENTWICKLUNG2"),
	NULL, entwicklungen);
    AUSWAHLFELD entw3(&maske, 50,16,20,staat->attribut_referenz("ENTWICKLUNG3"),
	NULL, entwicklungen);

    // Die Maske kann nur mit OK verlassen werden, da ich die alten Werte
    // nicht speichere!
    while (!maske.edit());

    // Das Atribut HTML steht auf ja oder nein. Ich will, dass das Attribut
    // Html dann entweder gesetzt oder nicht ist.
    if (!mystrcmp(html_ja_nein, L("ja","yes"))) staat->attribut_setzen("Html");
    else if (!mystrcmp(html_ja_nein, L("nein","no"))) staat->attribut_loeschen("Html");
    else {
	log('I', "UHR::ui_spieler_editieren(): HTML ist weder 'ja' noch 'nein'!");
	staat->attribut_loeschen("Html");
    }
    myfree(html_ja_nein);
    
    // Und nun kann es sein, dass man will, dass der Spieler alle Infos
    // neu bekommt. Das aber nicht in Zug 1.
    ui_eventuell_infos_neu(staat);

    // Und vermerken, dass der Spielstand modifiziert wurde.
    spielstand_ist_nun_modifiziert();

    log('U', L("Data of player %s %s (%s) was changed manually","Data of player %s %s (%s) was changed manually"),
	staat->attribut("VORNAME"), 
	staat->attribut("NACHNAME"),
	staat->name);
}


/**---------------------------------------------------------------------------
  * UHR::ui_se_maske_aufbauen()
  * 
  * Printed die Maske fuers Spielereditieren auf den Bildschirm
  ---------------------------------------------------------------------------*/
void UHR::ui_se_maske_aufbauen()
{
  io_cls();
  io_printxy(4,1,L("Angaben zum Spieler                    Angaben zum Staat","Player  Attributes                   Empire Attributes"));

  io_printxy(1, 3, L("   Name:","   Name:"));
  io_printxy(1, 4, L("Vorname:","Surname:"));
  io_printxy(1, 5, L("Strasse:"," Street:"));
  io_printxy(1, 6, L("    Ort:","   City:"));
  io_printxy(1, 7, L("Telefon:","  Phone:"));
  io_printxy(1, 8, L("Kontonr:","Account:"));
  io_printxy(1, 9, L("  email:","  email:"));

  const short rechts = 35;
  io_printxy(rechts, 4, L("    Name des Staates:","     Name of  Empire:"));
  io_printxy(rechts, 5, L("Titel des Herrschers:","     Title of leader:"));
  io_printxy(rechts, 6, L(" Name des Herrschers:","      Name of leader:"));
  io_printxy(rechts, 7, L(" Name der Hauptstadt:","     Name of capital:"));

  short oben = 11;

  io_printxy(10, oben+1,L("Printout","Printout"));
  io_printxy(1, oben+3,L("   HTML-Printout:","   HTML-Printout:"));
  io_printxy(1, oben+4,L(" Zusatzpfad (%p):","Path symbol (%p):"));
  io_printxy(1, oben+5,L("     Packer (%z):","     Packer (%z):"));

  io_printxy(47,oben+1,L("Startpaket","Starters"));

  io_printxy(1,oben+7,L("CTRL-E: Editieren beenden","CTRL-E: Finish editing"));
  
  io_printxy(rechts, oben+3, L("1. Entwicklung:","1. Development:"));
  io_printxy(rechts, oben+4, L("2. Entwicklung:","2. Development:"));
  io_printxy(rechts, oben+5, L("3. Entwicklung:","3. Development:"));
  io_printxy(rechts-1, oben+7, L("Werte mit den Pfeiltasten <-> aendern","Modify values with cursor keys <- and ->"));
}


/**---------------------------------------------------------------------------
  * UHR::ui_eventuell_infos_neu()
  * 
  * Loescht auf Wunsch alle Hinweise auf Infos, die ein Spieler schon
  * kennt. Passiert aber nicht in Runde 1.
  ---------------------------------------------------------------------------*/
void UHR::ui_eventuell_infos_neu(OBJEKT *staat)
{
  if (spielrunde == 1) return;

  io_cls();
  io_printxy(8,5,L("Sie haben die Daten eines Spielers geaendert. Wenn die Position","You have modified  the player attributes. If his/her position"));
  io_printxy(8,6,L("von einem neuen Spieler uebernommen wird,  dann ist es sinnvoll,","is to be taken over by another player, it's important that"));
  io_printxy(8,7,L("wenn dieser alle Infos noch einmal bekommt.\n","he gets all infos again.\n"));
  io_printxy(8,9,L("Wollen Sie dies (j/n)? ","Do you want this (y/n)? "));
  char taste;
  while ((taste = tolower(io_getch()))
	 && taste!=L('j','y')
	 && taste!=L('n','n'));
  if (taste == L('n','n')) {
    io_print(L("nein","no"));
    io_printxy(18,15,L("Keine neuen Infos - alter Spieler setzt fort","No new infos - same player continues"));
    io_getch();
  }
  else {
    io_print(L("ja","yes"));
    staat->kommando("INFOS_NEU");
    io_printxy(4,15,L("Alle Infos werden neu gedruckt - ein neuer Spieler kann uebernehmen","All infos will be printed again. A new player can take over the position"));
    io_getch();
    log('U', L("Player %s %s (%s) gets all all infos new","Player %s %s (%s) gets all all infos new"), staat->attribut("VORNAME"),
	staat->attribut("NACHNAME"), staat->name);
  }
}


/**---------------------------------------------------------------------------
  * UHR::ui_erde_schaffen()
  * 
  * Wird aufgerufen, wenn beim Programmstart noch keine Welt mit
  * Namen 'ERDE' existiert. Der Benutzer muss dann erst eine Welt
  * schaffen.
  * @return
  * short 1, wenn eine Welt geschaffen wurde, 0
  * short 0, wenn nicht. Das Programm soll dann automatisch beendet werden.
  ---------------------------------------------------------------------------*/
short UHR::ui_erde_schaffen()
{
  OBJEKT *welt = NULL;

  io_cls();

  char string[150];
  sprintf(string,L("Sie haben das Programm fuer die Partie \"%s\" gestartet.","You started the program for the game \"%s\"."),
    info_sessionname());
  io_centerline(10,string);
  io_centerline(12,L("Da diese Partie ganz neu ist, muessen Sie erst eine Welt erzeugen.","As this is a new game, you first have to create a world to play on."));
  do {
    io_centerline(15,L("Wollen Sie weitermachen (j/n)? -> <-","Do you want to continue(y/n)?  -> <-"));
    io_gotoxy(55,15);
    io_readline(string,1);
  } while (toupper(string[0]) != L('J','Y')
	   && toupper(string[0]) != L('N','N'));
  if (toupper(string[0]) != L('J','Y')) {
    io_cls();
    return 0;
  }
  
  io_centerline(1,L("Bewegen Sie den Cursor mit den Pfeiltasten und editieren Sie","Move the cursor with the cursor keys and edit the values."));
  io_centerline(2,L("die Werte.  Wenn Sie fertig sind,  druecken Sie CTRL-E","When you are ready, press CTRL-E"));
  io_centerline(3,L("Wenn Sie doch keine Welt schaffen wollen, dann","If you want the cancel, press CTRL-A"));
  io_centerline(4,L("druecken Sie Ctrl-A.                                        ",""));

  char *attrstring = attribute_fuer_welterschaffung();
  if (!attrstring) {
    io_cls();
    io_centerline(12,L("Vorgang abgebrochen - Keine Welt geschaffen...","Canceled - no world created."));
    io_centerline(23,L("Weiter mit 'Eingabe'","<Enter> to continue"));
    io_getch();
    io_cls();
    return 0;
  }

  // Die Welt soll also erschaffen werden.

  ATTRIBUT_LISTE attr(attrstring);
  attr.loeschen("NAME");
  if (!(welt = objekt_schaffen("ERDE", "WELT", attr.to_string()))) {
    io_cls();
    io_centerline(12,L("Fehler beim Erschaffen der Welt. Wenden Sie sich an den Progammierer!","Internal error, please contact KL_815userint_ccK Spiele!"));
    io_centerline(23,L("Weiter mit 'Eingabe'","<Enter> to continue"));
    io_getch();
    io_cls();
    return 0;
  }

  io_cls();
  io_centerline(12,L("Die neue Welt ist fertig. Die Partie koennte nun gestartet werden.","The new world is completed. The game can begin."));
  io_centerline(14,L("Vorher haben Sie aber  die Moeglichkeit,  eine  Uebersicht von der","Now you have to possibility to get an overview map of the world."));
  io_centerline(15,L("Weltkarte auszudrucken und eventuell eine andere Welt zu schaffen,","You may decide to use another world if you don't like this one."));
  io_centerline(16,L("wenn Ihnen diese nicht gefaellt.                                  ",""));
  io_centerline(18,L("Wollen Sie also eine solche Uebersicht (j/n)?","Do you want such an overview? (y/n)?"));
  if (ui_ja_nein()) {
    io_cls();
    io_centerline(12,L("Einen Moment, ich drucke...","One moment, printing..."));
    welt->kommando("UEBERSICHTSKARTE",NULL);
    io_centerline(12,L("Gefaellt Ihnen die Welt (j/n)?","Do you want to keep this world (y/n)?"));
    if (!ui_ja_nein()) return ui_erde_schaffen();  // Rekursion!
  }

  spielstand_ist_nun_modifiziert();
  return 1;
}
  

/**---------------------------------------------------------------------------
  * UHR::ui_befehle_eingeben()
  * 
  * Startet die Befehlseingabe fuer einen bestimmten Staat.
  * Diese befindet sich in BEMASKE.CPP.
  ---------------------------------------------------------------------------*/
void UHR::ui_befehle_eingeben()
{
  while (1)
  {
    io_cls();
    io_centerline(2,L("Befehle fuer einen Spieler eingeben","Enter commands for a player"));
  
    OBJEKT *staat = ui_spieler_erfragen();
    if (!staat) return;
  
    BEMASKE maske((STAAT *)staat);

    // Das Eingeben von Befehlen modifiziert den Spielstand nicht
  } 
}

/**---------------------------------------------------------------------------
  * UHR::ui_weltkarte_ausdrucken()
  * 
  * Ausdruck der ganze Weltkarte der Partie.
  ---------------------------------------------------------------------------*/
void UHR::ui_weltkarte_ausdrucken()
{
  io_cls();
  io_centerline(2,L("Weltkarten erzeugen","Create world maps"));

  io_centerline(10, L("Die Übersichtskarten werden neu erzeugt. Moment bitte...","The overview maps are being created. Please wait..."));
  uebersichtskarten_erzeugen();
}
 

/**---------------------------------------------------------------------------
  * UHR::ui_feld_aendern()
  * 
  * Der Benutzer kann ein Feld der Welt auf eine andere Gelaendeform
  * setzen. Zwar etwas umstaendlich, aber reicht aus fuer kleine
  * Korrekturen.
  ---------------------------------------------------------------------------*/
void UHR::ui_feld_aendern()
{
  io_cls();
  io_centerline(2,L("Feld der Weltkarte aendern","Change a field of the world"));

  ZWEID_MATRIX_ATLAS *weltmatrix = ((ZWEID_MATRIX_ATLAS *)erde());
  int anzahl_gelaendeformen = weltmatrix->anzahl_gelaendeformen();
  for (int i=1; i<anzahl_gelaendeformen; i++)
  {
      int spalte = (i-1)%3;
      int zeile = (i-1)/3;
      io_printxy(spalte*25 + 3, zeile + 4, myltoa(i));
      io_print(": ");
      io_printxy(spalte*25 + 7, zeile + 4, weltmatrix->gelaendeform_attribut(i, "Rep"));
  }

  char antwort[16];
  io_printxy(10,18,L("X-Koordinate:","   X-coords:"));
  io_readline(antwort,5);
  if (!antwort[0]) return;
  long x = myatol(antwort);
   
  io_printxy(10,19,L("Y-Koordinate:","   Y-coords:"));
  io_readline(antwort,5);
  if (!antwort[0]) return;
  long y = myatol(antwort);

  ADR adr(x,y);
  if (weltmatrix->adresse_ist_ausserhalb(adr)) {
      io_centerline(22, L("Liegt ausserhalb der Welt!","This lies somewhere in the universe!"));
      io_getch();
      ui_feld_aendern();
      return;
  }

  io_printxy(10,20,L("  Formnummer:","      Number:"));
  io_readline(antwort, 3);
  long form = myatol(antwort);
  if (form < 1 || form >= anzahl_gelaendeformen) {
      io_centerline(22, L("Ungueltige Nummer!","Invalid number!"));
      io_getch();
      ui_feld_aendern();
      return;
  }

  // Aendere Feld.
  weltmatrix->gelaendeform_aendern(adr, form);
  spielstand_ist_nun_modifiziert();
  log('U', "Changed field %ld,%ld to '%s'", x,y,weltmatrix->gelaendeform_attribut(form, "REP"));
  io_centerline(22, L("Feld geaendert.","Field changed."));
  io_getch();
  ui_feld_aendern();
}


/**---------------------------------------------------------------------------
  * UHR::user_interface()
  * 
  * Haupteinsprungstelle des User Interfaces. Enthaelt die Hauptschleife
  * und eine Kontrolle, ob eine Welt exisitert. Laesst den User eine
  * Welt schaffen, falls es noch keine gibt.
  ---------------------------------------------------------------------------*/
short UHR::user_interface()
{
  // Ganz am Anfang dieses Interfaces wird geprueft, ob eine Welt unter dem
  // Namen 'ERDE' existiert. Wenn das nicht der Fall ist, wird eine erzeugt.
  
  if (!objekt_mit_namen("ERDE") && !ui_erde_schaffen()) return 0;

  // Wenn "Autostart" gesetzt ist (Kommandozeilenoption -y), dann wird
  // automatisch die Auswertung gestartet.

  if (attribut_gesetzt("Autoende")) {
    attribut_loeschen("Autoende");
    ui_ende();
    return 0;
  }
  attribut_loeschen("Autoende");
  
  if (attribut_gesetzt("Autostart")) { 
    short ret = ui_weiter();
    attribut_loeschen("Autostart");
    return ret;
  }
  
  while (1) {
    switch (ui_menupunkt[ui_auswahl()][0])
    {
    case L('E','Q'): if (ui_ende()) return 0;                        else    break;
    case L('S','S'): if (ui_weiter()) return 1;                      else    break;
    case L('N','A'): ui_spieler_editieren(ui_neuer_spieler());               break;
    case L('A','C'): ui_spieler_editieren(this);                             break;
    case L('T','T'): ui_spielertabelle();                                    break;
    case L('L','R'): ui_spieler_entfernen();                                 break;
    case L('B','E'): ui_befehle_eingeben();                                  break;
    case L('W','W'): ui_weltkarte_ausdrucken();			break;
    case L('F','F'): ui_feld_aendern();			        break;
    }
  }
}
