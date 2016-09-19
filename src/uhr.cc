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
  * MODUL:               uhr.C  /  UHR.CPP
  * AUTOR/DATUM:         Mathias kettner, 1. April 1993
  * KOMPATIBILITAET:     C++
  -----------------------------------------------------------------------------
  *
  *      Beinhaltet die meisten und wichtigsten Funktionen der Uhr, zum
  *      Steuern, Menuaufbau u.s.w. Funktionen fuer die Befehlsmaske, die
  *      Spieler-Anlegen-Maske und die Welt-Schaffen-Maske befinden sich
  *      jedoch in anderen Modulen, da diese Modul ohnehin schon zu gross
  *      ist.
  *
  ---------------------------------------------------------------------------*/

#include <string.h>

#include "uhr.h"
#include "resource.h"
#include "einfluss.h"
#include "kompatib.h"
#include "fileview.h"
#include "staat.h"
#include "drucker.h"
#include "landscha.h"
#include "miniedit.h"
#include "maske.h"
#include "listmac.h"
#include "prom.h"
#include "alg.h"
#include "laengen.h"
#include "log.h"
#include "version.h"
#include "enzyklop.h"

// Variable fuer die Verwendung des Datenbereiches

extern DOUBLIST globale_objekt_menge;
extern EINFLUSS_LISTE globale_einfluss_menge;
extern short laser;
extern ENZYKLOPAEDIE *g_enzyklopaedie;
extern bool programm_terminieren; // aus main.C

/**---------------------------------------------------------------------------
  * UHR::UHR()                    // constructor
  * 
  ---------------------------------------------------------------------------*/
UHR::UHR(char *name, char *attr) : OBJEKT(name, attr)
{
    spielrunde = 0; // wird von logdateiname() eventuell gelesen.
    logfilename = mystrdup(logdateiname()); // ist nur vorlaeufig!
    loglevel    = mystrdup(attribut("LOGLEVEL"));

    // Das Setzen des Typs muss hier explizit geschehen, da die Uhr
    // das einzige Objekt ist, das nicht mit der Funktion objekt_schaffen()
    // geschaffen wird, die ja member-Funktion von OBJEKT ist und
    // deshalb nur aufgerufen werden kann, wenn bereits ein Objekt existiert.

    attribut_setzen("TYP","UHR");

    // Jetzt muessen noch einige andere Daten gesetzt werden

    spielrunde = 0;
    datum_dieser_auswertung[0]=0;
    datum_der_naechsten_auswertung[0]=0;
    staat_auf_platz_eins = NULL;

    // Spielstand-Modifikations-Zustand
    spielstand_modifiziert = 0;

    // Beim Schaffen der Uhr koennen einige Attribute gesetzt werden, um
    // bestimmte Parameter einzustellen. Wenn diese Attribute nicht gesetzt
    // sind, muss ich Ersatzwerte festlegen. Bei den Pfadnamen fuer Infos
    // und Grafiken druecke ich mich um die Syntax des jeweiligen Betriebs-
    // systemes, indem ich das aktuelle Verzeichnis nehme. Der Pfadname wird
    // naemlich einfach nur vorne an den eigentlichen Namen kopiert.
    
    // Bevor es weitergehen kann, muss ich die Enzyklopaedie erschaffen
    ATTRIBUT_LISTE dateiname;
    dateiname.setzen("DATEINAME", attribut("ENZ"));
    OBJEKT *enz = objekt_schaffen("Enzyklopaedie","ENZYKLOPAEDIE", dateiname.to_string());
    if (!enz) {
	log('I', "UHR::UHR(): Enzyklopaedie koennte nicht geschaffen werden");
	zur_vernichtung_vormerken();
	return; // Katastrophe!!
    }
    else if (!((ENZYKLOPAEDIE *)enz)->ok()) {
	log('E', "Game configuration file not correctly loaded. Aborting");
	zur_vernichtung_vormerken();
	return;
    }
    
    // Die Enzyklopaedie bleibt aber nicht in meinem Besitz, sondern ich
    // uebergebe sie dem Universum, damit sie nicht mitabgespeichert wird
    // usw.
    
    geben(UNIVERSUM, enz);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
UHR::~UHR()
{
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short UHR::laden(FILE *file)
{
  char puffer[50];
  fscanf(file, "%ld%s", &spielrunde, puffer);
  strcpy(datum_dieser_auswertung, string_to_wert(puffer));
  fscanf(file, "%s", puffer);
  strcpy(datum_der_naechsten_auswertung, string_to_wert(puffer));

  return (ferror(file) != 0);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short UHR::speichern(FILE *file)
{
  fprintf(file, "%ld ", spielrunde);
  fputstring(file, datum_dieser_auswertung, " ");
  fputstring(file, datum_der_naechsten_auswertung);
  return (ferror(file) != 0);
}




/**---------------------------------------------------------------------------
  * UHR::info()
  * 
  * Objektfunktion, mit der sich andere Objekte Informationen von der
  * Uhr holen koennen. Ist in OBJEKT deklariert.
  ---------------------------------------------------------------------------*/
char *UHR::info(char *infotext, void *, void *, void *)
{
  if (!strcmp("ZUGNUMMER",infotext)) return info_zugnummer();
  else if (!strcmp("DATUM",infotext)) return info_datum();
  else if (!strcmp("NAECHSTER_ZUG",infotext)) return info_naechster_zug();
  else if (!strcmp("HITLISTEN",infotext)) return info_hitlisten(); // In uhr.h!
  else if (!strcmp("BESTER_STAAT",infotext)) return info_bester_staat(); // .h
  else if (!strcmp("SESSIONNAME",infotext)) return info_sessionname();
  else return NULL; // Ignorieren ist besser.
}




/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
char *UHR::info_sessionname()
{
  return attribut("SESSIONNAME");
} 
  
/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
char *UHR::info_datum()
{
  return datum_dieser_auswertung;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
char *UHR::info_naechster_zug()
{
  return datum_der_naechsten_auswertung;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
char *UHR::info_zugnummer()
{
  static char antwort[10];
  sprintf(antwort, "%ld", spielrunde);
  return antwort;
}


/**---------------------------------------------------------------------------
  * UHR::naechster_zug()
  * 
  * Hier ist der zentrale Einsprungpunkt, mit dem das Menu gestartet
  * wird.
  ---------------------------------------------------------------------------*/
void UHR::naechster_zug(long)
{
  // Wurde das Programm nur zur Welterschaffung aufgerufen?
  if (attribut_gesetzt("WS_NAME"))
  {
    welt_erschaffen(attribut("WS_NAME"), myatol(attribut("WS_BREITE")),
       myatol(attribut("WS_HOEHE")));
    programm_terminieren = true;
    return;
  }

  // Den Spielstand laden
  if (!spielstand_laden()) programm_terminieren = true;
  else {
      emails_parsen(spielrunde);
      if (!user_interface()) programm_terminieren = true;

      // Zu guter Letzt permutiere ich noch die Staaten, damit jedesmal eine
      // andere Reihenfolge dran ist. Wegen der Gerechtigkeit...
      
      else {
	  direktes_besitztum_permutieren("TYP=STAAT");
	  programm_terminieren = false;
      }
  }

}


/**---------------------------------------------------------------------------
  * UHR::zug_abschliessen()
  * 
  * Sie ist die allerletzte Objektfunktion, die zur Berechnung des
  * Zuges aufgerufen wird.
  * Hier werden die Statistiken fuer die Punktewertung erstellt
  * und schliesslich die Ergebnisausdruecke angefertigt.
  * Ganz am Ende wird eventuell noch autogesaved...
  ---------------------------------------------------------------------------*/
void UHR::zug_abschliessen(long)
{
  // Jetzt kommt die Punkteauswertung. Sie findet nicht jeden Zug statt.
  // In welchen Zuegen, dass ermittelt eine Spezialfunktion

  if (auswertung_in_zug(spielrunde)) {
    statistiken_berechnen();

    // Und nun fuer jeden Staat das Kommando, dass fuer eine Auswertung
    // sorgt.
    FOR_EACH_OBJEKT_IN (alle_staaten()) DO_AND_DELETE
    (
      objekt->kommando("PUNKTEAUSWERTUNG",(void *)&hitlisten);
    )

    // Als naechstes will eine Gesamtliste erstellen, in der die Staaten
    // je nach Punkten aufgelistet sind. Ich teile dabei jedem Staat
    // seine Gesamtplatzierung mit und merke mir ausserdem den Staat
    // mit den meisten Punkten. Erst hole ich mir eine Liste mit allen
    // Staaten.

    DOUBLIST *staatenliste = alle_staaten();

    long alte_wertung = -1; // Um zu sehen, ob zwei Spieler gleichgut sind
    long aktueller_platz = 0; // Es kann z.B auch zweimal 1. Platz geben
    long laufende_nummer = 0; // Dies zaehlt aber immer weiter.

    while (!staatenliste->is_empty()) {

      // Zuerst suche ich aus der (restlichen) Liste den (oder einen) Staat
      // mit maximaler Punktzahl

      long beste_wertung = 0;
      OBJEKT_LIST_NODE *bester_staat_node = 0; // Sonst Compilerwarnung
      FOR_EACH_OBJEKT_IN (staatenliste) DO
      (
	if (myatol(objekt->info("PUNKTE")) >= beste_wertung) {
	  bester_staat_node = (OBJEKT_LIST_NODE *)node;
	  beste_wertung = myatol(objekt->info("PUNKTE"));
	}
      )

      // Der gefundene Staat erhaelt nun den naechsten Platz. Falls seine
      // Punkte ebensohoch sind, wie die Punkte des vorherigen Staates,
      // dann bekommt er aber die gleiche Platzierung.

      laufende_nummer++;
      if (alte_wertung != beste_wertung) aktueller_platz = laufende_nummer;
      alte_wertung = beste_wertung;

      // Dem Staat teile ich seine Platzierung jetzt mit.
      bester_staat_node->objekt->kommando("GESAMTWERTUNG_PLATZ",
	 (void *)&aktueller_platz);

      // Falls er auf dem ersten Platz ist, merke ich mir ihn, damit ich
      // den besten Spieler im Ergebnisausdruck angeben kann. Ich tue dies
      // in meiner Variablen staat_auf_platz_eins.

      if (aktueller_platz == 1) staat_auf_platz_eins = bester_staat_node->objekt;

      // Jetzt entferne ich den Staat aus der Liste und mache weiter.
      delete bester_staat_node;
    }
    delete staatenliste; // Ist nun leer und wird auch nicht mehr gebraucht.
  }

  // Und nun erzeuge ich die Ergebnisausdruecke.

  FOR_EACH_OBJEKT_IN (alle_staaten()) DO_AND_DELETE
  (
    objekt->abschlussbericht();
  )

  vorgemerkte_objekte_vernichten(); // WICHTIGER Aufruf!

  // Und jetzt schreibe ich noch die beiden Termine weiter.
  strcpy(datum_dieser_auswertung, datum_der_naechsten_auswertung);
  datum_der_naechsten_auswertung[0] = 0;
  
  spielrunde++;  // Auf in die naechste Runde

  // Und nun noch Speichern!
  spielstand_speichern("x");
  
}


/**---------------------------------------------------------------------------
  * UHR::spielstand_laden()
  * 
  * Laedt den Spielstand gemaess den Kommandozeilenoptionen.
  *
  * @return
  * 0 bei Fehler, sonst 1.
  ---------------------------------------------------------------------------*/
short UHR::spielstand_laden()
{
  io_cls();

  char s[1024];

  FILE *sessionfile = fopen(attribut("SESSIONFILE"),"r");
  if (!sessionfile) {
    sprintf(s,L("Das Sessionfile '%s' kann nicht geladen werden!","The session file '%s' couldn't be opened!"), 
	    attribut("SESSIONFILE"));
    io_centerline(12, s);
    log('E', "Can't open session file '%s'", attribut("SESSIONFILE"));
    return 0;
  }

  char sessionname[1024], variables_symbol[1024];
  fscanf(sessionfile, "%s %ld %s", sessionname, &spielrunde, variables_symbol);
  fclose(sessionfile);

  sprintf(s, L("Lade Partie '%s', Runde %ld, Speicherung '%s'...","Loading game '%s', turn %ld, saving '%s'..."),
    sessionname, spielrunde, variables_symbol);
  io_centerline(12, s);
  attribut_setzen("SESSIONNAME",sessionname);

  // Logdateiname jetzt auf log speziell fuer diese Partie

  myfree(logfilename); // Wurde im Konstruktor bereits vorlaeufig gesetzt.
  myfree(loglevel);
  logfilename = mystrdup(logdateiname());
  loglevel    = mystrdup(attribut("LOGLEVEL"));
  log('1', "----------------------------------------------------------------------");
  log('1', "%s%s", version_programm, NUR_TESTVERSION ? L(" (TESTVERSION)"," (TESTVERSION)") : "");

  char *gamefile = dateinamen_generieren(attribut("GAMEFILE"),NULL,variables_symbol);
  sprintf(s, L("Spielstandsdatei: '%s'","game file: '%s'"), gamefile);
  log('2', "Beginning to load game '%s' from file '%s'", sessionname, gamefile);
  
  io_centerline(14, s);

  if (laden_speichern(gamefile, 0))  
  {
    io_centerline(16, L("FEHLER aufgetreten!","ERROR occured!"));
    log('E', "Error while loading game file. Aborting");
	       
    return 0;
  }

  // Welt ueberpruefen
  WELT *welt = (WELT *)objekt_mit_namen("ERDE");
  if (!welt || !welt->ok()) {
      io_centerline(16, L("FEHLER aufgetreten!","ERROR occured!"));
      log('E', "Error while loading game file. Aborting");
      return 0;
  }

  long anzahl_staaten = alle_staaten()->count_and_del();
  log('2', "Correctly loaded session file with %ld players", anzahl_staaten);
  
  
  return 1;
}


/**---------------------------------------------------------------------------
  * UHR::spielstand_speichern()
  * 
  * Speichert den Spielstand
  * @param
  * char *variables_symbol: Ein variables Symbol, durch das %v ersetzt wird.
  ---------------------------------------------------------------------------*/
short UHR::spielstand_speichern(char *variables_symbol)
{
  if (!ist_spielstand_modifiziert()) return 0;

  io_cls();
  char s[1024];

  char *sessionname = attribut("SESSIONNAME");
  sprintf(s, L("Speichere Partie '%s', Runde %ld, Speicherung '%s'...","Saving game '%s', turn %ld, saving '%s'..."),
	  sessionname, spielrunde, variables_symbol);
  io_centerline(12, s);

  char *gamefile = dateinamen_generieren(attribut("GAMEFILE"),NULL,variables_symbol);
  sprintf(s, L("Spielstandsdatei: '%s'","Game file: '%s'"), gamefile);
  io_centerline(14, s);

  log('2', "Beginning to save game to file '%s'", gamefile);

  if (laden_speichern(gamefile, 1))  //
  {
    io_centerline(16, L("FEHLER aufgetreten!","ERROR occured!"));
    log('E', "Session was not correctly saved");
    return 1;
  }
  
  else {
      verzeichnis_gewaehrleisten(attribut("SESSIONFILE"));
      FILE *sessionfile = fopen(attribut("SESSIONFILE"),"w");
      if (!sessionfile) {
	  sprintf(s,L("Das Sessionfile '%s' kann nicht zum Schreiben geoeffnet werden!","Coudn't open session file '%s' for writing!"), 
		  attribut("SESSIONFILE"));
	  io_centerline(12, s);
	  return 1;
      }

      fprintf(sessionfile, "%s %ld %s\n", sessionname, spielrunde, variables_symbol);
      fclose(sessionfile);

      // Datei mit den Emailkeys erzeugen
      char emailkeys_filename[512];
      sprintf(emailkeys_filename, "emailkeys.%s", sessionname);
      FILE *f = fopen(emailkeys_filename, "w");
      if (f)
      {
	  FOR_EACH_OBJEKT_IN (alle_staaten()) DO_AND_DELETE
	      ({
		  char *key = objekt->attribut("EMAILKEY");
		  if (key)
		  {
		      fprintf(f, "%s %s\n", objekt->name, key);
		  }
	      });
	  fclose(f);
	  log('2', "Updated emailkey file %s", emailkeys_filename);
      }
      else log('E', "Cannot open %s for writing", emailkeys_filename);

      log('2', "Game was saved correctly");
      return 0;
  }
}


/**---------------------------------------------------------------------------
  * UHR::laden_speichern()
  * 
  * @return
  * short 1 bei einem Fehler, 0 bei OK.
  ---------------------------------------------------------------------------*/
short UHR::laden_speichern(char *filename, int speichern_und_nicht_laden)
{
  if (!speichern_und_nicht_laden) besitztum.clear();

  // Wenn ich speichere muss ich zuvor garantieren, dass das Zielverzeichnis
  // existiert.

  else                            verzeichnis_gewaehrleisten(filename);

 
  FILE *file = fopen(filename, speichern_und_nicht_laden ? "w" : "r");
  if (!file) return 1;

  if (speichern_und_nicht_laden ? objekte_speichern(file, NULL)
			        : objekte_laden(file, NULL)) // Fehler
  {
    fclose(file);
    return 1;
  }
  
  // Nun muss der ADRINDEX von ZWEID_MATRIX_ATLAS berechnet werden
  
  if (!speichern_und_nicht_laden) {
    OBJEKT *welt = objekt_mit_namen("ERDE");
    if (!welt) {
	log('K', "Error during load/save of game. Can't find object 'ERDE'");
	fclose(file);
	return 1;
    }
    
    ((WELT *)welt)->adrindex_berechnen();
  }

  // globale Variable enzyklopaedie
  
  g_enzyklopaedie = (ENZYKLOPAEDIE *)objekt_mit_namen("Enzyklopaedie");
  if (!g_enzyklopaedie) {
      log('I', "Fehler in UHR::laden_speichern(): g_enzyklpaedie == NULL");
      fclose(file);
      return 1;
  }

  if (speichern_und_nicht_laden ? globale_einfluss_menge.speichern(file, NULL)
      : globale_einfluss_menge.laden(file, NULL))
  {
    fclose(file);
    return 1;
  }

  fclose(file);
  spielstand_ist_nun_gespeichert();
  return 0; // Alles OK.
}


/**---------------------------------------------------------------------------
  * UHR::welt_erschaffen()
  * 
  * Schafft eine neue Welt und speichert das ganze als Partie ab.
  * Danach sollte das Programm beendet werden.
  ---------------------------------------------------------------------------*/
void UHR::welt_erschaffen(char *weltname, long breite, long hoehe)
{
  io_cls();
  char string[200];
  sprintf(string, L("Schaffe Welt \"%s\", Breite=%ld, Hoehe=%ld","Creating world \"%s\", width=%ld, height=%ld"),
    weltname, breite, hoehe);
  io_centerline(2,string);
  log('1', "Creating world for session '%s', size %ldx%ld. Algorithm is GENESIS-3",
      weltname, breite, hoehe);
  
  ATTRIBUT_LISTE attr;
  attr.setzen("Diagonal"); // Ist unbedingt notwendig.
  attr.setzen("BREITE", myltoa(breite));
  attr.setzen("HOEHE", myltoa(hoehe));
  attr.setzen("G3_KEIMDICHTE", "25");
  OBJEKT *welt = NULL;
  if (!(welt = objekt_schaffen("ERDE", "WELT", attr.to_string()))) {
      log('I', "UHR::welt_erschaffen(): Objekt WELT konnte nicht geschaffen werden.");
      exit(5);
  }
  else if (!((WELT *)welt)->ok())
  {
      log('E', "Error during creation of world occured. Aborting");
      exit(5);
  }
  
 
  spielstand_ist_nun_modifiziert(); // Durch das Schaffen der Welt.
  log('3', "World is created");

  // Die Spielrunde muss vor dem Speichern auf 1 gesetzt werden.

  spielrunde = 1;
  io_centerline(12, L("Moment, ich speichere...","One moment please, I'm saving..."));
  spielstand_speichern("w");

  // Karten erzeugen

  io_centerline(14, L("Erzeuge Uebersichts- und Gesamtkarten","Creating overview and full maps..."));
  uebersichtskarten_erzeugen();
}

/**---------------------------------------------------------------------------
  * UHR::uebersichtskarten_erzeugen()
  *
  * Erzeugt vier verschiedene Weltkarten: Zwei als PostScript, eine als
  * HTML Sammlung, und eine als GIF.
  ---------------------------------------------------------------------------*/
void UHR::uebersichtskarten_erzeugen()
{
    OBJEKT *welt = objekt_mit_namen("ERDE");
    if (!welt) return;

    char *filename = smallmapfilename();
    log('2', "Creating PostScript overview map %s", filename);
    welt->kommando("UEBERSICHTSKARTE", filename);

    filename = bigmapfilename();
    log('2', "Creating PostScript full map %s", filename);
    ((ZWEID_MATRIX_ATLAS *)welt)->g3_weltkarte_ausdrucken(filename);

    filename = htmlmapfilename("????");
    log('2', "Creating HTML full map %s", filename);
    ((ZWEID_MATRIX_ATLAS *)erde())->g3_weltkarte_ausdrucken_html();
}


/**---------------------------------------------------------------------------
  * UHR::alle_staaten()
  * 
  * Hilfsfunktion, die eine Liste aller Staat ermittelt.
  ---------------------------------------------------------------------------*/
DOUBLIST *UHR::alle_staaten(bool auch_verlierer, bool auch_ausgestiegene)
{
  DOUBLIST *antwortliste = new DOUBLIST;
  OBJEKT *objekt;
  SCAN(&besitztum, objekt)
  {
    if (objekt->typ_ist("STAAT")
	&& !objekt->zur_vernichtung_vorgemerkt()
	&& (auch_verlierer || !((STAAT *)objekt)->zivilisation_vernichtet())
        && (auch_ausgestiegene || !objekt->attribut_gesetzt("Ausgestiegen")))
    {
	antwortliste->add_tail(new OBJEKT_LIST_NODE(objekt));
    }
    NEXT(objekt);
  }
  return antwortliste;
}


/**---------------------------------------------------------------------------
 * UHR::aktuelle_forschungskosten()
 * 
 * Berechnet, welchen aktuellen Neupreise eine Forschung hat. Dies haengt
 * naemlich davon ab, wieviele anderer Spieler die Forschung schon haben.
 * 
 * @param
 * char *name:         Name der Forschung, z.B. "Automobilbau"
 * RESOURCE_VEKTOR res Neupreis der Forschung
 ----------------------------------------------------------------------------*/
RESOURCE_VEKTOR UHR::aktuelle_forschungskosten(char *name, RESOURCE_VEKTOR neupreis)
{
    int anz_hat = 0;
    int gesamt = 0;
    int ausgestiegen = 0;

    FOR_EACH_OBJEKT_IN (alle_staaten()) DO_AND_DELETE
    ({
	STAAT *staat = ((STAAT *)objekt);
	if (!staat->ausgestiegen())
	{
	    gesamt ++;
	    if (staat->info("HAT_ENTWICKLUNG", (void *)name)) anz_hat ++;
	}
	else 
	    ausgestiegen ++;
    })
    if (anz_hat == 0) return neupreis;

    RESOURCE_VEKTOR reduziert(neupreis);
    reduziert.multipliziere_mit_float( 1.0 - (((float) anz_hat) / ((float) gesamt)) );
    char *npc = mystrdup(neupreis.to_string());
    char *red = mystrdup(reduziert.to_string());

    log('3', "%d Empires, %d dropped, %d have %s",
	gesamt + ausgestiegen, ausgestiegen, anz_hat, name);
    log('3', "Development %s cheaper: %s --> %s", name, npc, red);
    myfree(npc);
    myfree(red);

    return reduziert;
}



/**---------------------------------------------------------------------------
  * UHR::befehlsdateiname(STAAT *staat)
  * 
  * Gibt den Namen der Datei zurueck, in der die Befehle fuer einen
  * bestimmten Staat fuer diese Runde stehen bzw. stehen sollen.
  ---------------------------------------------------------------------------*/
char *UHR::befehlsdateiname(STAAT *staat)
{
  return dateinamen_generieren(attribut("BEFEHLSPFAD"), staat, NULL);
}

/**---------------------------------------------------------------------------
  * UHR::infodateiname(char *infoname)
  * 
  * Gibt den Namen der Datei zurueck, in der das Info mit einem
  * bestimmten Namen steht.
  ---------------------------------------------------------------------------*/
char *UHR::logdateiname()
{  return dateinamen_generieren(attribut("LOGFILE"), NULL, NULL);}

char *UHR::infodateiname(char *infoname)
{  return dateinamen_generieren(attribut("INFOS"), NULL, infoname);}

char *UHR::persoenliche_mitteilungdateiname(STAAT *staat)
{  return dateinamen_generieren(attribut("PRIVAT"), staat, NULL);}

char *UHR::allgemeine_mitteilungdateiname()
{  return dateinamen_generieren(attribut("MITTEILUNGEN"));}

char *UHR::tempfilename(const char *variabel)
{  return dateinamen_generieren(attribut("TEMP"),NULL,(char *)variabel);}

char *UHR::bigmapfilename()
{ return dateinamen_generieren(attribut("BIGMAP"), NULL, NULL); }

char *UHR::smallmapfilename()
{ return dateinamen_generieren(attribut("SMALLMAP"), 0, 0); }

char *UHR::htmlmapfilename(char *variabel)
{ return dateinamen_generieren(attribut("HTMLMAP"), 0, variabel); }

char *UHR::gifmapfilename()
{ return dateinamen_generieren(attribut("GIFMAP"), 0, 0); }
 
char *UHR::htmldateiname(STAAT *staat, char *variabel)
{  return dateinamen_generieren(attribut("HTMLOUT"), staat, variabel);}

char *UHR::grafikdateiname(char *grafikname)
{  return dateinamen_generieren(attribut("GRAFIKEN"), NULL, grafikname); }

char *UHR::printout_dateiname(STAAT *staat, char *a_oder_b)
{  return dateinamen_generieren(attribut("AUSDRUCKE"), staat, a_oder_b); }

char *UHR::asciiout_dateiname(STAAT *staat)
{  return dateinamen_generieren(attribut("ASCIIOUT"), staat); }

char *UHR::gelaende_dateiname()
{  return dateinamen_generieren(attribut("GELAENDE"), 0, 0); }

char *UHR::emailin_dateiname()
{  return dateinamen_generieren(attribut("EMAILIN"), 0, 0); }

char *UHR::emaildone_dateiname(STAAT *staat, char *v)
{  return dateinamen_generieren(attribut("EMAILDONE"), staat, v); }

/**---------------------------------------------------------------------------
  * UHR::dateinamen_generieren()
  * 
  * Ersetzt Platzhalter in einem String und generiert so einen Dateinamen.
  * @param
  * char *format: Der Formatstring
  * STAAT *staat: Ein Staat oder NULL. Noetig fuer %n und %k
  * char *variabel: Wird fuer %v eingesetzt.
  * @return
  * Der zurueckgegebene String gehoert nach wie vor dieser Funktion und
  * darf nicht freigegeben werden.
  ---------------------------------------------------------------------------*/
char *UHR::dateinamen_generieren(char *format, STAAT *staat, char *variabel)
{
  static char string[3][MAX_LAENGE_DATEINAME];
  static int zaehler=0;
  
  zaehler = (zaehler+1) % 3;
  
  char *read=format;
  char *write=string[zaehler];
  char c;
  
  while (0 != (c = *read++))
  {
    if (c == '%') {
      switch (*read++) {
        case 's': {
          char *sessionname = info_sessionname();
	  if (sessionname) {
	      strcpy(write, sessionname);
	      write += strlen(write);
	  }
	  else {
	      strcpy(write, "default");
	      write += strlen(write);
	  }
          break;
        }
        case 'r':
          sprintf(write, "%02ld", spielrunde % 100);
          write += strlen(write);
          break;
        case 'R':
          sprintf(write, "%03ld", spielrunde % 1000);
          write += strlen(write);
          break;
        case 'n':
          if (staat) {
            strcpy(write, staat->name);
            write += strlen(write);
          }
          break;
        case 'k':
          if (staat) {
            char *kn = staat->attribut("KONTONR");
            if (kn) {
              strcpy(write, kn);
              write += strlen(write);
            }
          }
          break;
        case 'p':
          if (staat) {
            char *ps = staat->attribut("P_SYMBOL");
            if (ps) {
              strcpy(write, ps);
              write += strlen(write);
            }
          }
          break;
        case 'z':
	    if (staat) {
		char *packer = staat->attribut("PACKER");
		if (packer) {
		    strcpy(write, packer);
		    write += strlen(write);
		}
	    }
          break;
        case 'v':
          if (variabel) {
            strcpy(write, variabel);
            write += strlen(variabel);
          }
          break;
        } // switch
      } // if (c == '%')
      else *write++ = c;
   }
   *write=0;
   return string[zaehler];
}


/**---------------------------------------------------------------------------
  * UHR::alle_sind_freundlich()
  * 
  * Liefert 1, wenn alle Staaten sich gegenseitig als EF eingestellt
  * haben, sonst 0.
  ---------------------------------------------------------------------------*/
short UHR::alle_sind_freundlich()
{
    short einer_ist_feindlich = 0;
    
    // Ich gehe nun alle Staaten durch und schaue, ob jeder jeden als EF
    // eingestellt hat. Dabei muss ich Staaten ausnehmen, die entweder
    // Ausgestiegen sind oder die diese Runde ihre letzte Stadt verloren
    // haben.
    
    OBJEKT_LIST_NODE *staatnode1, *staatnode2;
    DOUBLIST *staaten = alle_staaten(false); // false: Verlierer werden ausgeschlossen

    SCAN(staaten, staatnode1) {
	STAAT *staat1 = (STAAT*)(staatnode1->objekt);
	if (!staat1->ausgestiegen())
	{
	    SCAN(staaten, staatnode2) {
		if (staatnode1->objekt != staatnode2->objekt)
		{
		    STAAT *staat2 = (STAAT*)(staatnode2->objekt);
		    if (!staat2->ausgestiegen()
			&& !staat1->endgueltig_freundlich_gegenueber(staatnode2->objekt))
		    {
			einer_ist_feindlich = 1;
			break;
		    } 
		}
		NEXT(staatnode2);
	    }
	}
	NEXT(staatnode1);
    }
    delete staaten;
    return !einer_ist_feindlich;
}   

  
/**---------------------------------------------------------------------------
  * UHR::einer_hat_ursprung_des_lebens()
  * 
  * Prueft, ob einer der Staaten den Ursprung des Lebens gefunden hat
  * und somit das Spiel zuende ist.
  * @param
  * Gibt die Anzahl der Staaten mit Ursprung des Lebens zurueck.
  ---------------------------------------------------------------------------*/
short UHR::einer_hat_ursprung_des_lebens()
{
    short anzahl = 0;
    FOR_EACH_OBJEKT_IN (alle_staaten(true)) // Auch bei zivilisation_vernichtet()
	DO_AND_DELETE({
	    if (((STAAT *)(objekt))->habe_ursprung_des_lebens()) anzahl ++;
	});
    return anzahl;
}
