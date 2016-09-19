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
  * MODUL:               main.C / MAIN.CPP
  * AUTOR/DATUM:         Mathias Kettner 1993
  * KOMPATIBILITAET:     C++
  -----------------------------------------------------------------------------
  *
  *      Dieses Modul enthaelt einzig und allein die main() Routine
  *      zum objektorientieren Simulationssystem. Solange mit dem
  *      Objekt UHR als Wurzelobjekt gearbeitet wird, kann sie stets
  *      unveraendert uebernommen werden, da alle wichtigen Aktionen
  *      ohnehin die Uhr uebernimmt.
  *
  ---------------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>

#include "uhr.h"
#include "alg.h"
#include "kompatib.h"
#include "dauer.h"
#include "titel.h"
#include "enzyklop.h"
#include "log.h"

extern DOUBLIST globale_objekt_menge;
short laser=1; // =1 bedeutet Zugauswertung fuer Laserprinter, GLOBAL !
short zufallsgenerator_deterministisch=0; // Bei 1 immer die gleichen Z-Zahlen
bool  printout_duplex = false;

bool programm_terminieren; // Damit antwortet uhr->zug(0);

ENZYKLOPAEDIE *g_enzyklopaedie=NULL;
UHR           *g_uhr=NULL;

#define NUM_OF_OPTIONS 22

char *options[NUM_OF_OPTIONS] = {
 "--gamefile",       "--printout",    "--htmlout", 
 "--asciiout",       "--befehle",     "--privat",
 "--mitteilungen",   "--infos",       "--grafiken", 
 "--gelaende",       "--enzyklop",    "--logfile",
 "--loglevel",       "--temp",        "--bigmap",
 "--smallmap",       "--htmlmap",     "--gifmap",
 "--emailin",        "--emaildone",   "--gformat",
 "--wait"
};

char *attrib[NUM_OF_OPTIONS] = {
 "GAMEFILE",       "AUSDRUCKE",    "HTMLOUT",
 "ASCIIOUT",       "BEFEHLSPFAD",  "PRIVAT",
 "MITTEILUNGEN",   "INFOS",        "GRAFIKEN",
 "GELAENDE",       "ENZ",          "LOGFILE",
 "LOGLEVEL",       "TEMP",         "BIGMAP",
 "SMALLMAP",       "HTMLMAP",      "GIFMAP",
 "EMAILIN",        "EMAILDONE",    "GFORMAT",
 "WARTE_AUF_BEFEHLE"
};

char *defaultwert[NUM_OF_OPTIONS] = {
    "%s/savings/%R%v.pss", "%s/printout/%n.%r%v",  "%s/htmlout-%r/%n/%v",
    NULL,                  "%s/befehle/%n.%R",     "%s/privat/%n.%R",
    "%s/privat/%R",        
    L("/usr/share/prometheus/infos/de/%v",    "/usr/share/prometheus/infos/en/%v"),
    L("/usr/share/prometheus/graphics/%v", "/usr/share/prometheus/graphics/%v"),
    L("/usr/share/prometheus/config/de/terrain.cfg", "/usr/share/prometheus/config/en/terrain.cfg"),
    L("/usr/share/prometheus/config/de/rules.cfg",   "/usr/share/prometheus/config/en/rules.cfg"),   
    "%s/log",
    "EWIK123",             "%s/tmp.%v",            "%s/mapbg_%r.ps",
    "%s/mapsm_%r.ps",      "%s/htmlmap/%v_%r.htm", "%s/mapsm_%r.gif",
    "emailin",             "emaildone/%s_%n.%R",   ".png",
    "999"
};

char *beschreibung[NUM_OF_OPTIONS] = {
 L("Spielstandsdatei","game file"),
 L("Postscript-Printout","Postscript-printout"),
 L("HTML-Printout","HTML-printout"),
 L("ASCII-Printout","ASCII-printout"),
 L("Befehlsdateien","command files"),
 L("Persoenliche Mitteilungen","Privates messages"),
 L("Allgemeine Mitteilungen","Messages to all players"),
 L("Infos","Infos"),
 L("Grafiken","Graphic files"),
 L("Gelaendedatei","area configuration file"),
 L("Enzyklopaedie","game configuration file"),
 L("Logfile","Logfile"),
 L("aktive Loglevel (\"\" fuer alle)","active loglevels (\"\" for all)"),
 L("Temporaere Dateien","Tempfiles"),
 L("Vollstaendie PostScript-Karte","Full PostScript map"),
 L("Kleine Uebersichtskarte","Small overview map"),
 L("Vollstaende HTML-Karte","Full HTML map"),
 L("GIF Uebersichtskarte","GIF overview map"),
 L("Verzeichnis fuer eingehende email","Directory for incoming email command sheets"),
 L("Verzeichnis fuer verarbeitete","Directory for done emails"),
 L("Grafikformat der erzeugten HTML Printouts (gif oder png)", "Graphics format used in HTML printout (.gif or .png)"),
 L("Starte nur, falls max. n Befehlsboegen fehlen", "Start only, if not more than ... turnsheets are missing")
 };
 
/**---------------------------------------------------------------------------
  * defaultwerte_setzen()
  * 
  * Setzt defaultwerte fuer Kommandozeilenparamter
  ---------------------------------------------------------------------------*/
void defaultwerte_setzen(ATTRIBUT_LISTE& attribute)
{
  for (int i=0; i<NUM_OF_OPTIONS; i++)
  {
    if (defaultwert[i])
      attribute.setzen(attrib[i], defaultwert[i]);
  }
}


/**---------------------------------------------------------------------------
  * main()
  * 
  * Dies ist die Hauptroutine zum ganzen System. Sie ruft solange
  * Die Aktionsfunktion der Uhr auf (die sie vorher erschafft),
  * bis sich die Uhr selbst vernichtet hat und folglich nicht mehr
  * in der globalen_objekt_menge befindet.
  ---------------------------------------------------------------------------*/
ATTRIBUT_LISTE *kommandozeile_auswerten(ATTRIBUT_LISTE&, int, char **);

int main(int argc, char **argv)
{
  ATTRIBUT_LISTE attribute;
  defaultwerte_setzen(attribute);
  kommandozeile_auswerten(attribute, argc, argv);

  io_init();		// Einausgabe initialisieren
  io_init_random();	// Zufallsgenerator mit Startwert versehen

  if (!attribute.gesetzt("Autostart") &&
      !attribute.gesetzt("Kein_Titel")) titel(); // Titelbildschirm mit Copyright etc.

  // Jetzt schaffe ich die Uhr mit den vorbereiteten Attributen
  if (!(g_uhr = new UHR("Uhr",attribute.to_string()))) {
      log('I', "Interner Fehler: Uhr konnte nicht geschaffen werden");
      io_close();
      exit(5);
  }
  if (g_uhr->zur_vernichtung_vorgemerkt()) {
      io_close();
      delete g_uhr;
      exit(5);
  }

  g_uhr->zug(0);
  
  // Wenn das Attribut "WARTE_AUF_BEFEHLE" auf "warte" steht, dann breche ich
  // ab.
  int exitcode = 0;
  if (!mystrcmp(g_uhr->attribut("WARTE_AUF_BEFEHLE"),"warte"))
  {
      log('1', "Next turn not started yet");
      exitcode = 7;
  }
  else
  {
      // Wenn es die Uhr jetzt noch gibt, dann wurde eine Spielrunde
      // gestartet.
  
      if (!programm_terminieren)
      {
	  log('1', "Starting computation of turn %ld", myatol(g_uhr->info("ZUGNUMMER")));
	  for (int i=1; i<=RUNDEN_PRO_ZUG; i++) {
	      log('4', "Phase %d", i);
	      g_uhr->aktion(i);
	  }
	  g_uhr->zug_ende(myatol(g_uhr->info("ZUGNUMMER")));
	  log('1', "Finished computation. Next turn will be %ld",
	      myatol(g_uhr->info("ZUGNUMMER")));
      }
      exitcode = 0;
  }
  delete g_uhr;
  io_close();
  log('2', "Program terminated correctly");
  myfree(logfilename);
  myfree(loglevel);
  return exitcode;
}

short pfad_parameter_auswerten(ATTRIBUT_LISTE& attr, char *option, char *par)
{
  for (int i=0; i<NUM_OF_OPTIONS; i++)
  {
    if (!strcmp(options[i], option)) {
      attr.setzen(attrib[i], par);
      return 1;
    }
  }
  return 0;
}
  

void print_copyright()
{
    printf(L("Dies ist freie Software, Sie können Sie unter bestimmten Bedingungen\n", 
	     "This is free software, and you are welcome to redistribute it\n"));
    printf(L("weiterverbreiten. Einzelheiten erfahren Sie in der Datei \n",
	     "certain conditions; for details look into \n"));
    printf("/usr/share/prometheus/COPYING\n");
}


void print_usage(char *argv_0)
{
  printf(
  L("Usage: %s <Sessiondatei> <Option> <Option> ...\n\n","Usage: %s <Sessiondatei> <Option> <Option> ...\n\n"), argv_0);
  printf(L("\nOption          Defaultwert          Pfadschema fuer\n","\noption          defaultvalue         pathscheme for\n"));
  for (int i=0; i<NUM_OF_OPTIONS; i++)
    printf("%-15s %-20s %s\n", options[i], defaultwert[i] ? defaultwert[i] :
	   L("(deaktiviert)","(deactivated)"), beschreibung[i]);
    
  printf("\n%s", 
	 L("--duplex        Printouts werden auf gerade Seitenzahl gefuellt\n","--duplex        Printouts are padded to even number of pages\n")
	 L("--simplex       (Default) Schaltet --duplex wieder aus\n","--simplex       (default) switches off --duplex\n")
	 L("-r              Macht den Zufallsgenerator deterministisch\n","-r              Make the random generator deteministic\n")
	 L("-X              Unterdrueckung des Titelbildschirmes\n","-X              Suppress title screen\n")
	 L("-y D1 D2        Automatisches Starten mit D1 = Heute und D2 = Zugannahme\n","-y D1 D2        Start next turn. D1 = today,  D2 = dead line\n")  
	 L("-w <P> <B> <H>  Neue Partie <P> erzeugen. Weltkarten-Breite <B> und Hoehe <H>\n","-w <G> <W> <H>  Create new game <G> erzeugen. World has width <W> und height <H>\n")
	 "-------------------------------------------------------------------------------\n\n");
  print_copyright();
}

ATTRIBUT_LISTE *kommandozeile_auswerten(ATTRIBUT_LISTE& attr, int argc, char **argv)
{
  if (argc < 2) {
    print_usage(argv[0]);
    exit(5);
  }  

  attr.setzen("SESSIONFILE", argv[1]);

  // Ich baue aus den Argumenten einige Attribute fuer die Uhr auf.
  for (int i=2; i<argc; i++) {
    if (i+1 < argc && pfad_parameter_auswerten(attr, argv[i], argv[i+1]))
    {
      i++;
      continue;
    }

    else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      print_usage(argv[0]);
      exit(0);
    }

    else if (!strcmp(argv[i], "--duplex")) printout_duplex = true;
    else if (!strcmp(argv[i], "--simplex")) printout_duplex = false;
    else if (!strcmp(argv[i], "-r")) zufallsgenerator_deterministisch=1;
    else if (!strcmp(argv[i], "-X")) attr.setzen("Kein_Titel");
    else if (!strcmp(argv[i], "-y")) {
      i++;
      if (i+1 >= argc) {
        fprintf(stderr,
		L("Die Option -y (automatisch Auswertung starten) erfordert zwei Parameter:\n","The option -y requires two parameters:\n")
		L("-y <Datum dieser Zugannahme> <Datum der naechsten Zugannahme>\n","-y <date of evaluation> <date of dead line>\n")
		L("z.B. -y 15.6.98 29.6.98\n","e.g. -y 06/15/98 06/30/98\n"));
      }
      else {
        attr.setzen("Autostart");
        attr.setzen("DATUM1", argv[i]);
	i++;
        attr.setzen("DATUM2", argv[i]);
      }
    }        

    else if (!strcmp(argv[i], "-w")) { // Welt erschaffen
      if (i+3 >= argc) {
	  fprintf(stderr, L("Die Option -w (Partie erschaffen) verlangt die drei Parameter\n","The option -w (create game) requires three parameters:\n")
		  L("<Name> <Breite> <Hoehe>, also z.B. \"-w pr02 160 80\"\n","<name> <width> <height>, e.g. \"-w pr02 120 80\"\n"));
	  exit(5);
      }
      else {
	  attr.setzen("WS_NAME", argv[++i]);
	  attr.setzen("SESSIONNAME", argv[i]);
	  attr.setzen("WS_BREITE", argv[++i]);
	  attr.setzen("WS_HOEHE", argv[++i]);
	  attr.setzen("Kein_Titel");
      }
    }
        
    else {
      fprintf(stderr, L("Unbekannte Option '%s'!\n","Unknown option '%s'!\n"), argv[i]);
      print_usage(argv[0]);
      exit(5);
    }
    
  }
  laser = !attr.gesetzt("ASCIIOUT");
  return &attr;
}
