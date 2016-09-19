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
  * MODUL:               welt.C / WELT.CPP
  * AUTOR/DATUM:         Mathias Kettner, 30. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Enthaelt alle Funktionen des Objektes WELT
//
// **************************************************************************

#include <string.h>

#include "welt.h"
#include "laengen.h"
#include "staat.h"
#include "layout.h"
#include "kompatib.h"
#include "drucker.h"
#include "listmac.h"
#include "alg.h"
#include "prom.h"
#include "log.h"
#include "einheit.h"


extern short laser; // Ist 0, wenn -a Option gewaehlt wurde, sonst 1.
extern DOUBLIST globale_objekt_menge;

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
WELT::WELT(char *n, char *a) : ZYLINDER_ATLAS(n,a)
{
  if (attribut_gesetzt("++LADEN++")) return;

  // Jetzt erzeuge ich mal eine schoene Welt...
  matrix_leeren(); // Wird das nicht schon vom Konstruktor des 2DAtlas gemacht?
  genesis_teil_3();
}


/**---------------------------------------------------------------------------
  * WELT::naechster_zug()                // virtuell
  * 
  * Z.B. Meteoritenfaelle.
  ---------------------------------------------------------------------------*/
void WELT::naechster_zug(long)
{
  long zugnummer = myatol(besitzer->info("ZUGNUMMER"));
  meteoritenfall(zugnummer);
}


/**---------------------------------------------------------------------------
  * WELT::meteoritenfall()
  * 
  * Laesst ein paar Meteoriten fallen.
  ---------------------------------------------------------------------------*/
void WELT::meteoritenfall(long zugnummer)
{
  // Erstmal schauen, ob was fliegt, und wenn ja, wieviel
  
  long runde_sicher = myatol(lexikon_eintrag("meteoritenfall",1));
  if (runde_sicher>0 && io_random(runde_sicher) > zugnummer) return; // Nein.

  long oberflaeche = breite * hoehe;
  long felder_pro_meteorit = myatol(lexikon_eintrag("meteoritenfall",2));
  if (!felder_pro_meteorit) felder_pro_meteorit = 1500;
  
  short anzahl_max = 2 * (oberflaeche / felder_pro_meteorit);
  if (anzahl_max <= 0) anzahl_max = 1; // Eigentlich irgendwie falsch sonst.
  short anzahl = io_random(anzahl_max);
  
  while (anzahl--) meteorit_werfen();
}

    
/**---------------------------------------------------------------------------
  * WELT::meteorit_werfen()
  * 
  * Laesst genau einen Meteoriten irgendwo fallen.
  ---------------------------------------------------------------------------*/
void WELT::meteorit_werfen()
{
  ADR ziel;
  
  do {
    ziel.x = io_random(breite);
    ziel.y = io_random(hoehe);
    
  } while (anzahl_objekte_bei_adresse(ziel));
  
  // Nun die Groesse bestimmen.
  long groesse = io_random(io_random(100) + 1) + 1;
  RESOURCE_VEKTOR res(groesse, L('X','X'));

  // Am Ziel darf man nicht mehr nach anderen Rohstoffen suchen
  // setze_feld_attribut(ziel, FELD_ATTRIBUT_BODEN_UNTERSUCHT);
  
  // Falls es schon einen Bodenschatz dort gibt, addiere ich den neuen.
  if (bodenschatz_vorhanden(ziel)) {
    bodenschatz_bei(ziel).addiere(res);
  }
  
  // Ansonsten mache ich einen neuen.
  else {
    BODENSCHATZ *schatz = new BODENSCHATZ;
    schatz->adresse = ziel;
    schatz->lager.setzen_auf(res);
    bodenschaetze.insert(schatz);
  }

  // Und nun werden alle Sternwarten informiert...
  
  OBJEKT_LIST_NODE *objnode;
  SCAN(&globale_objekt_menge, objnode)
  {
    if (!mystrcmp(objnode->objekt->a_gattung(), L("Sternwarte","Observatory")))
       objnode->objekt->kommando("METEORIT", &ziel, &groesse);
    NEXT(objnode);
  }
}


/**---------------------------------------------------------------------------
  * WELT::info()                          // virtuell
  ---------------------------------------------------------------------------*/
char *WELT::info(char *info, void *par1, void *, void *)
{
  if (!mystrcmp("BEWIRTSCHAFTUNG_PRUEFEN",info))
			 return info_feld_bewirtschaftung_pruefen(par1);

  else return ""; // Info unbekannt
}


/**---------------------------------------------------------------------------
  * WELT::info_feld_bewirtschaftung_pruefen()
  * 
  * Realisiert die Info 'FB', mit der eine Stadt feststellen kann,
  * ob ein bestimmtes Feld noch bewirtschaftet werden kann, oder
  * ob es bereits von einer anderen Stadt bewirtschaftet wird oder
  * ausserhalb der Landschaft liegt.
  *
  * @param
  *
  * @return
  * "Ja", falls es noch bewirtschaftet werden kann, sonst NULL.
  ---------------------------------------------------------------------------*/
char *WELT::info_feld_bewirtschaftung_pruefen(void *par1)
{
  ADR adr = *(ADR *)par1;
  if (adresse_ist_ausserhalb(adr)) return NULL;

  if (!feld_attribut(adr, FELD_ATTRIBUT_BEWIRTSCHAFTUNG)) return "Ja";
  else return NULL; // Wird bereits bewirtschaftet
}


/**---------------------------------------------------------------------------
  * WELT::info_objekt_aufzaehlung()
  * 
  * Erzeugt die Einheitensichtungen.
  *
  * Objekte von befreundeten Staaten erzeugen keine Fernsichtung.
  *
  * Einheiten und Weltbauten von endgueltig befreundeten Staaten erzeugen
  * ueberhauptkeine Sichtungenm
  *
  * @param
  * (OBJEKT *)par1:     mittleres Objekt
  * (DOUBLIST *)par2: 
  * Vorberechnete Liste aller Objekte in der Umgebung,
  * falls eine zur Hand war. Sie wird nicht geloescht.
  * Ansonsten NULL.
  * @return
  * char *: Dynamisch angelegter String mit der Antwort (s.o.). Wenn
  * kein Objekt gesehen wurde, so ist die Antwort NULL.
  ---------------------------------------------------------------------------*/
char *WELT::objekt_aufzaehlung(void *par1, void *par2)
{
  OBJEKT *eigenes_objekt = (OBJEKT *)par1;
 
  // Den Staat des sichtenden Objektes ermitteln
  OBJEKT *staat_o = eigenes_objekt;
  while (staat_o && !staat_o->typ_ist("STAAT")) staat_o  = staat_o->besitzer;
  if (!staat_o) return NULL;
  STAAT *staat = (STAAT *)staat_o;
  
  ADR adr = eigenes_objekt->adresse;

  // Ich bereite die Liste der gesehenen Objekte vor.
  
  DOUBLIST *objektliste; 
  if (par2) objektliste = (DOUBLIST *)par2;
  else {
    float sichtweite = myatof(eigenes_objekt -> info("SICHTWEITE"));
    objektliste = alle_objekte_im_umkreis_von(adr, sichtweite);
  }
  
  // Nun bereite ich einen Antwortstring vor. Dieser muss vom Empfaenger
  // spaeter freigegeben werden!!!

  char *antwort = NULL;
  FOR_EACH_OBJEKT_IN (objektliste)
  DO
  (
    /* Mitspielerinfo: Eventuell ist das ein Objekt eines bisher unbekannten
       Staates. Dann muss man seine Adresse auf dem Printout bekommen */

    staat->mitspielerinformation(objekt);

    /* Fernsichtungen von Verbuendeten werden ausgeschlossen.
       Vom eigenen Staat wird sowieso nichts ausgegeben. */

    float entfernung = entfernung_zwischen(adr, objekt->adresse);
    short dstatus = staat->dipl_status_gegenueber(objekt);
    short endgueltig_freundlich = staat->endgueltig_freundlich_gegenueber(objekt);
    
    if    (dstatus < 2 
       || (dstatus == 2 
            && entfernung <= 1.5) 
            && (!endgueltig_freundlich || objekt->typ_ist("STADT"))) 
    {
      /* Vorbereitung der Richtungsangabe */
      char *richtung = richtung_von_nach(adr, objekt->adresse).to_string();

      /* Jetzt muss ich den Namen aufbauen, wie [Panzer <STJ> SO] oder
       [Hauptstadt Ulan Bator <STH> N]. Wenn das Objekt
       weiter als 1.5 entfernt ist, wird nur noch eine sehr knappe Ausgabe
       erzeugt. Fuer Einheiten nur noch [NNO] und fuer Staedte und Einrichtungen
       [Bauw. NNO] */

      mystradd(antwort, "[");

      if (entfernung > 1.5) { /*- Knapp -*/
	if (!objekt->typ_ist("EINHEIT")) /* keine Einheit */
	  mystradd(antwort, L("Bauw. ","Edifice "));
	mystradd(antwort, richtung);
      }
      
      else /*- Sehr ausfuehrliche Ausgabe -*/
      {    
	mystradd(antwort, objekt->a_gattung());

	/* Hat das Objekt einen richtigen Namen, dann ist der auch noch
	  wert, aufgenommen zu werden. Das trifft vor allem bei Staedten zu. */

	char *langer_name = objekt->a_name();
	if (langer_name) {
	  mystradd(antwort, " ");
	  mystradd(antwort, langer_name);
	}

        // Und dann noch die Abkuerzung des Staates.
        OBJEKT *gstaat = objekt;
	while (gstaat && !gstaat->typ_ist("STAAT")) gstaat = gstaat->besitzer;
	if (gstaat) {
	  mystradd(antwort, " <");
	  mystradd(antwort,gstaat->name);
	  mystradd(antwort, ">");
	}

	/* jetzt muss ich noch angeben, in welcher Richtung das Objekt gesehen
 	   wurde. */

	if (richtung && richtung[0]) {
	  mystradd(antwort, " ");
	  mystradd(antwort, richtung);
	}
      } /* Objekt nah, ausfuehrliche Angabe */
      mystradd(antwort, "] ");

    } /* if (Objekt soll ausgegeben werden) */

  ) /* FOR_EACH... */

  // Falls ich die Objektliste selber angelegt habe, muss ich sie nun
  // loeschen.

  if (!par2) delete objektliste;
  
  // Nun ist die Antwort fertig.
  return antwort;
}


/**---------------------------------------------------------------------------
  * WELT::kommando()                      // virtuell
  ---------------------------------------------------------------------------*/
short WELT::kommando(const char *kommando, const void *par1, const void *, const void *)
{
  if (!mystrcmp("FELD_BEWIRTSCHAFTEN",kommando))
	return kommando_feld_bewirtschaften((void *)par1);

  else if (!mystrcmp("FELD_NICHT_BEWIRTSCHAFTEN",kommando))
	return kommando_feld_nicht_mehr_bewirtschaften((void *)par1);

  else if (!mystrcmp("UEBERSICHTSKARTE",kommando))
	return kommando_uebersichtskarte_drucken((void *)par1);
	
  else  return 1;
}


/**---------------------------------------------------------------------------
  * WELT::kommando_feld_bewirtschaften()
  * 
  * Realisiert das Kommando, das der Welt mitteilt, dass ein bestimmtes
  * Feld nun von einer Stadt bewirtschaftet wird. Die Welt fuehrt darue-
  * ber Buch, so dass kein Feld von zwei Staedten bewirtschaftet wird.
  *
  * @param
  * (ADR *)     Zeiger auf die Adresse des Feldes
  ---------------------------------------------------------------------------*/
short WELT::kommando_feld_bewirtschaften(void *par1)
{
  // Ich gehe von einer korrekten Adresse aus!

  setze_feld_attribut(*(ADR *)par1, FELD_ATTRIBUT_BEWIRTSCHAFTUNG);
  return 0;
}


/**---------------------------------------------------------------------------
  * WELT::kommando_feld_nicht_mehr_bewirtschaften()
  * 
  * Kommando, dass die Bewirtschaftung eines Feldes wieder freigiebt
  * und somit fuer eine andere Stadt freimacht.
  *
  * @param
  * (ADR *)     Zeiger auf die Adresse des Feldes
  ---------------------------------------------------------------------------*/
short WELT::kommando_feld_nicht_mehr_bewirtschaften(void *par1)
{
  // Ich gehe von einer korrekten Adresse aus!
  loesche_feld_attribut(*(ADR *)par1, FELD_ATTRIBUT_BEWIRTSCHAFTUNG);
  return 0;
}


/**---------------------------------------------------------------------------
  * WELT::kommando_uebersichtskarte_drucken(void *)
  *
  * Druckt eine Uebersichtskarte in ein PS-File. Die Felder sind als grau
  * Quadrate dargestellt.
  ---------------------------------------------------------------------------*/
short WELT::kommando_uebersichtskarte_drucken(void *par1)
{
    char *filename = (char *)par1;
    verzeichnis_gewaehrleisten(filename);
    if (drucker_init(filename)) {
	log('W', "Can't open print file '%s' for overview map", filename);
	return 1;
    }

    dr_anfang(false); // Initialisierungssequenzen zum Drucker schicken.

  // Und nun start ich die Funktion, die mir die Uebersichtskarte in die
  // Druckdatei schreibt.
  
  uebersichtskarte_drucken();
  
  // Und dann muss ich den Drucker auch wieder schliessen.
  
  drucker_close();
  return 0; // Alles rodscher.
}

 
/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
RESOURCE_VEKTOR& WELT::resourcen_auf_feld(ADR& adr)
{
  static RESOURCE_VEKTOR returnwert;

  // Hier berechne ich, was fuer Resourcen eine Stadt fuer die Bewirtschaftung
  // dieses Feldes in Normalfall bekommt

  returnwert.setzen_auf(gelaendeform_attribut(adr, "RESOURCEN"));
//  if (feld_attribut(adr, FELD_ATTRIBUT_BEWAESSERUNG)) {
//    RESOURCE_VEKTOR add(gelaendeform_attribut(adr,"BEWAESSERUNG"));
//    returnwert.addiere(add);
//  }
  return returnwert;
}

/**---------------------------------------------------------------------------
  ---------------------------------------------------------------------------*/
short WELT::speichern(FILE *file)
{
  // Zuerst muss ich die unterliegende Matrix speichern...
  if (ZWEID_MATRIX_ATLAS::speichern(file)) return 1;

  // Und nun speichere ich alle Landschaftsabbilder!
  // Zuerst kommt die Anzahl
  fprintf(file, "%ld\n",abbilder.number_of_elements());

  // Jetzt kommt fuer jedes Abbild Name, matrix und bit_matrix gefaechert.
  LANDSCHAFTSABBILD *abbild;
  abbild = (LANDSCHAFTSABBILD *)abbilder.first();
  while (!abbild->is_tail()) {
    fprintf(file, "%s\n", abbild->besitzer);
    for (long spalte=0; spalte<breite; spalte++) {
      if (fputhex(file, abbild->matrix[spalte], hoehe)) return 1;
      if (fputhex(file, (char *)(abbild->bit_matrix[spalte]), hoehe)) return 1;
    }
    fprintf(file, "\n");
    abbild = (LANDSCHAFTSABBILD *)abbild->next();
  }

  // Und nun kommt noch die Liste aller Bodenschaetze...
  return bodenschaetze_speichern(file);

}


/**---------------------------------------------------------------------------
  ---------------------------------------------------------------------------*/
short WELT::laden(FILE *file)
{
  // Zuerst muss ich dafuer sorgen, dass meine Matrix richtig geladen wird..
  if (ZWEID_MATRIX_ATLAS::laden(file)) return 1;

  // Nun lade ich alle Landschaftsabbilder. Zuerst kommt deren Anzahl
  long anzahl=0;
  for (fscanf(file, "%ld", &anzahl); anzahl; anzahl--)
  {
    LANDSCHAFTSABBILD *abbild = new LANDSCHAFTSABBILD(breite, hoehe, NULL, file);
    abbilder.insert(abbild);
  }

  // Und nun noch die Bodenschaetze...
  return bodenschaetze_laden(file);
}


/**---------------------------------------------------------------------------
  * WELT::bodenschaetze_speichern()
  * 
  * Speichert die Liste der Bodenschaetze in ein bereits offenes File.
  * Das Aufzeichnungsformat ist Anzahl der Eintraege gefolgt von
  * ADR::to_string() und RESOURCE_VEKTOR::to_string() fuer
  * jeden Eintrag. Alle acht Eintraege wird ein LF eingefuegt, um
  * das Resultat besser leserlich zu machen.
  *
  * @param
  * FILE *file: Zieldatei.
  *
  * @return
  * short 1 bei ferror(), sonst 0.
  ---------------------------------------------------------------------------*/
short WELT::bodenschaetze_speichern(FILE *file)
{
  long anzahl = bodenschaetze.number_of_elements();
  fprintf(file,"%ld ",anzahl);
  BODENSCHATZ *schatz = (BODENSCHATZ*)bodenschaetze.first();
  while (anzahl) {
    fprintf(file, "%s %s ",schatz->adresse.to_string(), schatz->lager.to_string());
    anzahl--;
    schatz = (BODENSCHATZ *)schatz->next();
    if ((anzahl&7) == 3) fprintf(file, "\n");
  }
  fprintf(file, "\n");
  return (0 != ferror(file));
}


/**---------------------------------------------------------------------------
  * WELT::bodenschaetze_laden()
  * 
  * Laedt die Liste der Bodenschaetze aus einem bereits offenen Ascii-
  * file. Loescht vorher eventuell vorhandene alte Bodenschaetze.
  *
  * @param
  * FILE *file: Quelldatei
  *
  * @return
  * short 1, bei ferror(), sonst short 0.
  ---------------------------------------------------------------------------*/
short WELT::bodenschaetze_laden(FILE *file)
{
  char respuffer[MAX_LAENGE_RESOURCEZEILE+1];
  long anzahl;
  fscanf(file, "%ld", &anzahl);
  while (anzahl) {
    ADR adr(file);
    fscanf(file, "%s", respuffer);
    BODENSCHATZ *schatz = new BODENSCHATZ;
    schatz->adresse = adr;
    schatz->lager.setzen_auf(respuffer);
    bodenschaetze.insert(schatz);
    anzahl--;
  }
  return (0 != ferror(file));
}


/**---------------------------------------------------------------------------
  * WELT::nach_resourcen_bohren()
  * 
  * Ermoeglicht einer Einheit, an einem bestimmten Feld nach Resourcen
  * zu bohren. Ob sich dort welche befinden, und wenn ja, wieviele,
  * wird erst hier in dieser Funktion ermittelt. Das haengt von der
  * Gelaendeform ab, bei der gesucht wird. Die Wahrscheinlichkeiten
  * fuer ein Finden ist in den Gelaendeformattributen gespeichert.
  *
  * @param
  * ADR& adr: Dort soll gebohrt werden.
  *
  * @return
  * short 1, wenn etwas gefunden wurde, sonst 0.
  ---------------------------------------------------------------------------*/
short WELT::nach_resourcen_bohren(ADR& adr)
{
  // Wenn an dieser Stelle schon einmal gesucht wurde, dann kann ich
  // hoechstens in der Liste der Bodenschaetze nachsehen, ob noch
  // etwas uebrig ist. Neue Ressourcen bekommt man aber nicht.

  if (feld_attribut(adr, FELD_ATTRIBUT_BODEN_UNTERSUCHT))
    return bodenschatz_vorhanden(adr);

  setze_feld_attribut(adr, FELD_ATTRIBUT_BODEN_UNTERSUCHT);

  // Ob eine neuer Bodenschatz gefunden wird, haengt von einem Feldattribut
  // ab. Im Attribut FINDEN steht die Prozentuale Wahrscheinlichkeit eines
  // Vorhandenseins.

  // Mal schauen, ob der Spieler Glueck hat...

  long wahrscheinlichkeit = myatol(gelaendeform_attribut(adr, "FINDEN"));
  if (io_random(100) >= wahrscheinlichkeit) return 0; // gibt's nix

  // Nun erwuerfle ich die Foerderquote, die man pro Runde foerdern kann.
  
  long schatz_max = myatol(gelaendeform_attribut(adr, "SCHATZ_MAX"));
  long schatz_min = myatol(gelaendeform_attribut(adr, "SCHATZ_MIN"));
  long menge = io_random(schatz_max - schatz_min + 1) + schatz_min;
  
  // So. Nun hat der Spieler doch tatsaechlich was gefunden.

  RESOURCE_VEKTOR quote(menge, * gelaendeform_attribut(adr, "SCHATZART"));

  // Moeglicherweise liegt auf dem Feld schon etwas: Meteorit!
  if (bodenschatz_vorhanden(adr)) {
    bodenschatz_bei(adr).addiere(quote);
  }
  
  else { // Sonst trage ich einen neuen Schatz in die Liste ein.
    BODENSCHATZ *schatz = new BODENSCHATZ;
    schatz->adresse = adr;
    schatz->lager.setzen_auf(quote);
    bodenschaetze.insert(schatz);
  }

  return 1; // Fuendig. Mit bodenschatz_bei() kann die Menge ermittelt werden.
}


/**---------------------------------------------------------------------------
  * WELT::bodenschatz_bei()
  * 
  * Sucht in der Liste der Bodenschaetze nach einer bestimmten Adresse
  * und gibt eine Referenz auf die dort nocht vorhandenen Bodenschaetze
  * zurueck.
  *
  * @param
  * ADR &adr: Dort soll geschaut werden.
  *
  * @return
  * RESOURCE_VEKTOR& Referenz auf das Lager. Das Lager kann damit
  * auch veraendert werden! Ist kein Bodenschatz vorhanden, so wird
  * eine Referenz auf einen leeren Resourcevektor (static) zurueck-
  * gegeben, der nicht veraendert werden sollte.
  ---------------------------------------------------------------------------*/
RESOURCE_VEKTOR& WELT::bodenschatz_bei(ADR& adr)
{
  static RESOURCE_VEKTOR leer;

  BODENSCHATZ *schatz = (BODENSCHATZ *)bodenschaetze.find((void *)&adr);
  if (!schatz) return leer;

  // Wenn der Schatz leer ist, dann loesche ich ihn gleich aus der Liste.

  if (schatz->lager.ist_null()) {
    delete schatz;
    return leer;
  }

  // Aufgrund einer Programmaenderung muss ich dafuer sorgen, dass die
  // ehemaligen Vorkommen, die sich in 100er Hoehen bewegten, in ein
  // vernuenftiges Mass gebracht werden, da es sich nun um Foerderquoten
  // handeln wird. Viel spaeter kann ich die Zeilen dann wieder 'raus-
  // schmeissen. Der Rohstoff L('X','X') (Meteorit) wird nicht mitgerechnet.
  
  else if (schatz->lager.betrag() - schatz->lager[L('X','X')] > 50) // Neu auswuerfeln.
  {
    delete schatz;
    do {
      loesche_feld_attribut(adr, FELD_ATTRIBUT_BODEN_UNTERSUCHT);
    } while (!nach_resourcen_bohren(adr));
    return bodenschatz_bei(adr); // Rekursion.
  }    

  else return schatz->lager;
}


/**---------------------------------------------------------------------------
  * WELT::bodenschatz_vorhanden()
  * 
  * Ermittelt, ob an einer bestimmte Adresse (noch) gefundene Boden-
  * schaetze liegen.
  *
  * @param
  * ADR& adr: Dort soll geschaut werden.
  *
  * @return
  * short 1, falls ja, short 0, falls nichts mehr da ist.
  ---------------------------------------------------------------------------*/
short WELT::bodenschatz_vorhanden(ADR& adr)
{
  if (bodenschaetze.find((void *)&adr)) return 1;
  else return 0;
}


/**---------------------------------------------------------------------------
  * WELT::landschaftsabbild_anlegen()
  * 
  * Legt fuer ein Objekt ein neues Landschaftsabbild an.
  *
  * @param
  * besitzer:     Objekt, fuer das das Landschaftsabbild angelegt werden
  * soll
  ---------------------------------------------------------------------------*/
void WELT::landschaftsabbild_anlegen(OBJEKT *besitzer)
{
  LANDSCHAFTSABBILD *abbild;
  abbild = new LANDSCHAFTSABBILD(get_breite(), get_hoehe(), besitzer);
  if (!abbild->besitzer) delete abbild; // Hat nicht geklappt
  else { abbilder.insert(abbild);  }
}

/**---------------------------------------------------------------------------
  * WELT::landschaftsabbild()
  * 
  * Sucht nach dem Landschaftsabbild, dass ein Objekt in der Welt
  * angelegt hat.
  *
  * @param
  * besitzer:       Objekt, nach dessen Landschaftsabbild gesucht werden
  * soll
  *
  * @return
  * LANDSCHAFTSABBILD *  Zeiger auf das Landschaftsabbild oder NULL,
  * wenn das Objekt noch kein Landschaftsabbild
  * in der Welt besitzt.
  ---------------------------------------------------------------------------*/
LANDSCHAFTSABBILD *WELT::landschaftsabbild(OBJEKT *besitzer)
{
  return (LANDSCHAFTSABBILD *)abbilder.find((void *)besitzer->name);
}


/**---------------------------------------------------------------------------
  * WELT::landschaftsabbild_vernichten()
  * 
  * Vernichtet das Landschaftsabbild eines Objekts, falls es eines
  * besass.
  *
  * @param
  * besitzer:       Objekt, dessen Landschaftsabbild vernichtet werden
  * soll
  ---------------------------------------------------------------------------*/
void WELT::landschaftsabbild_vernichten(OBJEKT *besitzer)
{
  LANDSCHAFTSABBILD *abbild = landschaftsabbild(besitzer);
  if (abbild) delete abbild;
}


/**---------------------------------------------------------------------------
  * WELT::landschaftsabbild_aktualisieren()
  * 
  * Dies ist eine der zentralen und auch komplexeren Funktionen im Zu-
  * sammenhang mit den Landschaftsabbildern. Unter der Angabe eines
  * Bereiches der Form Mitte-Radius werden alle Felder in diesem
  * Bereich von der wirklichen Welt in das Landschaftsabbild uebertragen.
  * Dabei werden zum einen die Gelaendeformen und die Feldattribute
  * kopiert, zum anderen aber auch die nicht beweglichen Objekte (vor
  * allem Staedte). Letztere werden in den hoeherwertigen Bits der
  * bit_matrix vermerkt. Diese sind fuer die Darstellung von immobilen
  * Objekten vorgesehen und sind permanent. Die unteren Bits hingegen
  * sind fuer die Einzeichnung der mobilen Objekte (Einheiten) und
  * werden jede Runde wieder geloescht.
  * Uebrigens: Sollte diese Funktion von einem Staat aufgerufen werden,
  * der noch kein Landschaftsabbild besitzt, so wird eines fuer ihn an-
  * gelegt.
  *
  * @param
  * besitzer:       Staat, dessen Landschaftsabbild aktualisiert werden
  * soll.
  * zentrum:        Mitte des Bereiches der aktualisiert werden soll
  * radius:         Radius des Bereiches
  ---------------------------------------------------------------------------*/
void WELT::landschaftsabbild_aktualisieren
		(OBJEKT *besitzer, ADR& zentrum, float radius)
{
  // Zuerst hole ich mir das Landschaftsabbild

  LANDSCHAFTSABBILD *abbild = landschaftsabbild(besitzer);
  if (!abbild) {
    landschaftsabbild_anlegen(besitzer);
    abbild = landschaftsabbild(besitzer);
    if (!abbild) {
	log('I', "WELT::landschaftsabbild_aktualisieren(): "
	    "Ich kann kein Landschaftsabbild anlegen!");
	return;
    }
  }

   // Jetzt hole ich mir eine Liste aller Felder, die zu aktualisieren sind
  DOUBLIST *feldliste = alle_adressen_im_umkreis_von(zentrum, radius);

  // Und nun uebertrage ich jedes dieser Felder in das Landschaftsabbild.
  while (!feldliste->is_empty())
  {
    ADR_LIST_NODE *adrnode = (ADR_LIST_NODE *)feldliste->first();
    abbild->feld_aktualisieren(adrnode->adresse, feld(adrnode->adresse));

    // Merkmale ueber immobile Objekte, die sich auf einem der Felder befinden
    // koennen, werden in der bit_matrix in der oberen drei Bits gespeichert.
    // Dadurch koennen sieben verschiedene Objektsymbole im Landschafts-
    // abbild repraesentiert werden. Momentan gibt es nur den Typ STADT.

    long x=adrnode->adresse.x;
    long y=adrnode->adresse.y;
    abbild->bit_matrix[x][y] &= ~LA_IMMOBILMASKE; // Loesch.d.oberen. Bits

    // Nun muss ich schauen, ob sich ein Objekt auf dem Feld befindet.
    if (objekt_bei_adresse(adrnode->adresse)) { // Ja.

      // Ich erstelle die Immobilmaske, indem ich alle immobilen Objekte
      // auf dem Feld suche.

      unsigned char immobil = 0;
      FOR_EACH_OBJEKT_IN (alle_objekte_bei_adresse(adrnode->adresse))
      DO_AND_DELETE
      (
	if (objekt->typ_ist("STADT") || objekt->typ_ist("WELTBAUT"))
	  immobil = MAX(immobil, LA_STADT);
      )

      // Die Maske ist nun fertig, und ich maskiere sie in die bit_matrix..
      abbild->bit_matrix[x][y] |= immobil;

    } // if (objekt_auf_feld(...))

    delete adrnode;
  } // (alle Felder im Bereich abarbeiten)

  delete feldliste;

  // Das wars.
}


/**---------------------------------------------------------------------------
  * WELT::landschaftsabbild_komplett()
  * 
  * Aktualisiert das komplette Lanschaftsabbild eines Staates. Diese
  * Funktion existiert nur zu Testzwecken und wird ueber einen speziel-
  * len Geheimbefehl des Staates aufgerufen.
  *
  * @param
  * besitzer:       Staat, dessen Landschaftsabbild aktualisiert werden
  * soll.
  ---------------------------------------------------------------------------*/
void WELT::landschaftsabbild_komplett(OBJEKT *besitzer)
{
  for (long x=0; x<breite; x++)
    for (long y=0; y<hoehe; y++)
    {
      ADR pos(x,y);
      landschaftsabbild_aktualisieren(besitzer, pos, 0);
    }
}


/**---------------------------------------------------------------------------
  * WELT::sicht_eines_objektes()
  * 
  * Diese Funktion ist relativ komplex. Waehrend Veraenderungen in
  * der Landschaft staendig registriert werden, wenn sie im Sicht-
  * bereich eines Objektes kommen (auch in den einzelnen Phasen), so
  * werden die mobilen Objekte stets nur am Ende des Zuges angezeigt.
  * Auf dem Ergebnisausdruck ist somit immer die Ausgangsposition fuer
  * den naechsten Zug richtig gegeben. Am Ende eines Zuges wird diese
  * Funktion von jedem Objekt aufgerufen, dass ein Gebiet der Landschaft
  * ueberblicken kann. Abhaengig von einem Radius werden alle beweglichen
  * Objekte im Umkreis in die bit_matrix eingetragen. Auch das zentrale
  * Objekt wird eingetragen (falls es mobil ist), allerdings mit einer
  * hoeheren Nummer. Ziel ist, dass auf dem Ergebisausdruck eigene Ein-
  * heiten eine andere Farbe haben, als fremde.
  *
  * Seit neuestem hat die Funktion noch eine zweite Aufgabe: Wenn ein
  * Staat ein gegnerisches Objekt sieht, dann bekommt er die Spieler-
  * daten des Spielers dessen Staat das Objekt gehoert. Dazu stellt der
  * Staat die Funktion mitspielerinformation() zur Verfuegung.
  *
  * @param
  * besitzer:  Besitzer (STAAT) des Landschaftsabbildes und des zentralen
  * Objektes
  * adr:       Mittelpunkt des Bereiches
  * radius:    Radius des Bereiches
  ---------------------------------------------------------------------------*/
void WELT::sicht_eines_objektes(OBJEKT *besitzer, ADR& adr, float radius)
{
  LANDSCHAFTSABBILD *abbild = landschaftsabbild(besitzer);
  if (!abbild) {
    landschaftsabbild_anlegen(besitzer);
    if (!landschaftsabbild(besitzer)) return;
  }

  // Alle Objekte im Sichbereich (adr,radius) werden in die bit_matrix ein-
  // getragen mit der Kennung 1. Am Schluss wird das mittlere Feld mit
  // der Kennung 2 eingetragen. Die Eintragungen werden mit einer MAX-
  // Verknuepfung realisiert. Es werden nur mobile Objekte eingetragen

  // Ich beschaffe mir als erstes eine Liste aller Objekte im Umkreis
  DOUBLIST *objliste = alle_objekte_im_umkreis_von(adr, radius);

  // Nun trage ich bei deren Adressen eine 1 ein, falls dort nicht
  // schon etwas hoeheres steht.

  while (!objliste->is_empty())
  {
    OBJEKT_LIST_NODE *objnode=(OBJEKT_LIST_NODE *)objliste->first();

    // Hier kann es sein, dass der Staat mit einem anderen Staat Kontakt
    // aufnimmt.

    ((STAAT *)besitzer)->mitspielerinformation(objnode->objekt);

    unsigned char eintrag=0;
    if (objnode->objekt->typ_ist("EINHEIT")) eintrag = LA_EINHEIT;
    else { // Objekt nicht eintragen, da es nicht mobil ist.
      delete objnode;
      continue;
    }

    // Falls die Adresse zufaellig die in der Mitte ist, so handelt es
    // sich um das zentrale Objekt, welches mit einer hoeheren Prioritaet
    // eingetragen wird. Zum Wert wird dann etwas addiert.

    if (objnode->objekt->adresse == adr) eintrag += LA_EIGENESOBJEKT;

    long x = objnode->objekt->adresse.x; // Bestimmen der Koordinaten...
    long y = objnode->objekt->adresse.y;

    unsigned char maske = abbild->bit_matrix[x][y] & LA_MOBILMASKE; //ausmask.
    maske = MAX(eintrag, maske); // Neue Maske erstellen
    abbild->bit_matrix[x][y] &= ~LA_MOBILMASKE; // Bereich loeschen
    abbild->bit_matrix[x][y] |= maske; // Neue Maske einkopieren

    delete objnode;
  }
  delete objliste;
}


/**---------------------------------------------------------------------------
  * WELT::landschaftsabbild_objekt_loeschen()
  * 
  * Loescht alle beweglichen Objekte aus der bit_matrix eines Land-
  * schaftsabbildes.
  *
  * @param
  * besitzer:       Staat, fuer den das Landschaftsabbild angelegt wurde
  ---------------------------------------------------------------------------*/
void WELT::landschaftsabbild_objekte_loeschen(OBJEKT *besitzer)
{
  LANDSCHAFTSABBILD *abbild = landschaftsabbild(besitzer);
  if (!abbild) return;

  for (long spalte=0; spalte<breite; spalte++) {
    for (long zeile=0; zeile<hoehe; zeile++) {
      abbild->bit_matrix[spalte][zeile] &= ~LA_MOBILMASKE;
    }
  }
}

/**---------------------------------------------------------------------------
  * WELT::landschaftsabbild_auszaehlen()
  * 
  * Zaehlt im Landschaftsabbild eines Staates, wiviele Felder der
  * Staat schon einmal gesehen hat, also im Abbild eine Gelaende-
  * form eingetragen haben. Wird wird die Punktewertung benoetigt.
  *
  * @param
  * besitzer:       Besitzer des Landschaftsabbildes
  *
  * @return
  * long:   Anzahl der bekannten Felder
  ---------------------------------------------------------------------------*/
long WELT::landschaftsabbild_auszaehlen(OBJEKT *besitzer)
{
  LANDSCHAFTSABBILD *abbild = landschaftsabbild(besitzer);
  if (!abbild) return 0;

  // Nun mache ich zwei nette kleine Schleifen und zaehle der Reihe nach.
  long anzahl=0;
  for (long spalte=0; spalte<breite; spalte++) {
    for (long zeile=0; zeile<hoehe; zeile++)  {
      if (abbild->matrix[spalte][zeile] & 0xff) anzahl++;
    }
  }

  return anzahl;
}


/**---------------------------------------------------------------------------
  * WELT::landschaftsabbild_ausdrucken()
  * 
  * Druckt das Abbild mit der Funktion drucken() aus. Ist momentan
  * leider nur auf Zeichenbasis vorhanden.
  *
  * @param
  * besitzer:       Besitzer des Landschaftsabbildes
  ---------------------------------------------------------------------------*/
void WELT::landschaftsabbild_ausdrucken(OBJEKT *besitzer)
{
    LANDSCHAFTSABBILD *abbild = landschaftsabbild(besitzer);
    if (!abbild) {
	log('I', "WELT::landschaftsabbild_ausdrucken(): "
	    "Landschaftsabbild von %s nicht vorhanden!",
	    besitzer->name);
	return;
    }

  // Es soll ein moeglichst kleiner Rechteckiger Bereich gefunden werden,
  // der die ganze bekannte Welt beinhaltet. Das macht folgender Aufruf.
  // in mitte_x soll die X-Koordinate von der Spalte stehen, die in der
  // Mitte sein soll (Startpunkt des Spielers).

  ADR mitte = *(ADR *)besitzer->info("RELATIVE_ADRESSE", NULL);

  long xstart, xende, ystart, yende; // Transientparameter
  abbild->benutzten_bereich_ermitteln(xstart, ystart, xende, yende, mitte.x);

  // Passt das Ganze auf ein Blatt? Wenn nein muss ich es aufteilen
  // auf mehrere Blaetter. Wieviele Felder auf ein Blatt passen, dass
  // ist konstant.

  const short MAX_FELDER_BREITE = 43;
  const short MAX_FELDER_HOEHE = 64;

  // Und nun muss ich das Ganze so zerteilen, dass es moeglichst optimal
  // aufgeteilt wird. Kein Bereich darf groesser als 43 * 64 sein.
  // Deshalb mache ich eine verschachtelte Schleife und drucke zuerst die
  // untereinanderliegenden Befereich aus (in der inneren Schleife) und
  // in der ausseren die seitlich benachbarten.

  // Die aussere Schleife (x) ist die schwierigere, da unter Umstaenden ein
  // Umbruch durchgefuehrt werden muss. Auf jeden Fall beginnt es bei
  // xstart.

  int teilbreite; // Auszudruckende Breite.
  if (xende >= xstart) teilbreite = xende-xstart+1;
  else teilbreite = breite - xstart + xende + 1;
  int ganze_xportionen = teilbreite / MAX_FELDER_BREITE;
  int restbreite = teilbreite % MAX_FELDER_BREITE;

  while (ganze_xportionen || restbreite)
  {
    int xdrucklinks, xdruckrechts;
    if (ganze_xportionen) { // Dann ganze Breite ausnuetzen
       xdrucklinks = xstart;
       xstart+=MAX_FELDER_BREITE;
       xdruckrechts = (xstart-1) % breite;
       xstart %= breite;
       ganze_xportionen--;
    }
    else { // Restlichen Bereich
      xdrucklinks = xstart;
      xdruckrechts = (xdrucklinks + restbreite - 1) % breite;
      restbreite = 0;
    }

    int teilhoehe = yende-ystart+1;
    int ganze_yportionen = teilhoehe / MAX_FELDER_HOEHE;
    int resthoehe = teilhoehe % MAX_FELDER_HOEHE;
    int ydruckende = yende;
    while (ganze_yportionen || resthoehe)
    {
      if (ganze_yportionen) {
	teilmatrix_drucken(besitzer,xdrucklinks,ydruckende-MAX_FELDER_HOEHE+1,
			 xdruckrechts, ydruckende);
	ydruckende -= MAX_FELDER_HOEHE;
	ganze_yportionen--;
      }
      else {
	teilmatrix_drucken(besitzer, xdrucklinks, ystart, xdruckrechts,
				ydruckende);
	resthoehe = 0;
      }
    }   // y-Schleife
  } // x-Schleife
}


/**---------------------------------------------------------------------------
  * WELT::teilmatrix_drucken()
  * 
  * Druckt einen Teil des Landschaftsabbildes aus und versieht es mit
  * einem eigenen Koordinatensystem.
  *
  * @param
  * OBJEKT *besitzer:              Staat
  * long xstart, ystart, xende, yende: Auszudruckender Bereich in
  * absoluten Koordinaten.
  ---------------------------------------------------------------------------*/
void WELT::teilmatrix_drucken(OBJEKT *besitzer, long xstart,
				 long ystart, long xende, long yende)
{
    LANDSCHAFTSABBILD *abbild = landschaftsabbild(besitzer);
    if (!abbild) {
	log('I', "WELT::teilmatrix_drucken(): Kein Landschaftsabbild von %s",
	  besitzer->name);
	return;
    }

  // teilbreite und teilhoehe geben die Breite und Hoehe des hier auszu-
  // druckenden Bereiches an (in Feldern).

  long teilbreite;
  if (xende >= xstart) teilbreite = xende-xstart+1;
  else teilbreite = breite - xstart + xende + 1;
  long teilhoehe = yende-ystart+1;

  // Nun drucke ich aus. Die Ausmasse eines Feldes gebe ich hier mal
  // als const an.

  const float feldbreite = 0.4064; // cm

  // Jetzt mal ich zur Orientierung eine Umrandung und beschrifte sie
  // dann noch mit Koordinaten. Das macht eine Hilfsfunktion.
  //
  // Die Koordinaten, die ein Spieler bekommt, sind jedoch relativ.

  ADR mitte = *(ADR *)besitzer->info("RELATIVE_ADRESSE");

  // Die Variable links und oben beschreiben die Position des ersten
  // tatsaechlichen Feldes auf dem Blatt Papier

  float links = (19.80-teilbreite*feldbreite)/2;
  float oben = .6;

  // Nun gebe ich den Bereich an, in dem sich die relativen X-Koordinaten
  // bewegen sollen, die der Spieler ausgedruckt bekommt. Die Normierung
  // auf einen moeglichst kleinen Betrag uebernimmt die koord..system()
  // Funktion. Ich schicke ihr also eventuell Koordinaten, die nicht im
  // gueltigen Bereich liegen.

  long xbera; // Subjektive X-Koordinaten, Anfang
  if (xstart > mitte.x) xbera = xstart-breite-mitte.x;
  else xbera = xstart - mitte.x;
  long xbere = breite/2; // Weiter darf es nicht steigen.

  long ybera = yende-mitte.y; // Subjektive Y-Koordinaten, Anfang
  long yr = -1; // Absteigene Skalierung bei Y.

  // Ach ja. Und ich muss beim Umbruchsystem noch anmelden, wieviel Platz
  // ich brauche und ein nettes Koordinatensystem zeichnen...

  float layout_oben = 0.0;

  if (laser)
  {
    layout_oben = dr_abschnitt(oben + teilhoehe*feldbreite + 0.8);
    koordinatensystem(links, oben, layout_oben, teilbreite, teilhoehe,
		    xbera, xbere, ybera, yr, feldbreite);
  }

  // Die beiden Schleifen laufen auf Objektive Koordinaten.
  //  for (long zeile = ystart; zeile <= yende; zeile++) {
  // Ab nun durchlaufe ich die Schleife rueckwaerts, damit die Ausgabe
  // in der Ascii-Uebersicht Orientierungstreu ist.

  for (long zeile = yende; zeile >= ystart; zeile--) {
    for (long spalte = xstart; spalte < xstart+teilbreite; spalte++)
    {
      short noch_unbekannt = 1;

      // Die Position des Layouts kann ich schon vorbereiten. Das ist,
      // besser so, da etwas Rechnung erforderlich ist, die ich am
      // besten nicht an zwei Stellen mache..

      float layout_x = links + (spalte-xstart) * feldbreite;
      float layout_y = oben + (teilhoehe-zeile+ystart-1) * feldbreite;

      // Die Variable fn zeigt am Ende der inneren Schleife auf einen
      // Dateinamen. Die benannte Datei enthaelt die Grafik zu dem Feld.
      // Das Format ist momentan .gra

      char *fn = NULL;

      // Falls in der bit_matrix ein Eintrag ist, so wird nicht
      // die Gelaendeform sondern eine Markierung dargestellt.
      // Die hoechste Prioritaet haben momentan die mobilen Objekte...

      // Uebrigens: Beim Zugriff auf die Matrizen muss ich die Spalte
      // stets module breite nehmen, da der Bereich unter Umstaenden falsch
      // umgebrochen ist. Ich bereite mir den Zugriff von.

      long x = spalte % breite;
      long y = zeile; // Y ist real und unveraendert.
      ADR adr(x, y); // Vorbereiten

      unsigned char mobilmaske =   abbild->bit_matrix[x][y] & LA_MOBILMASKE;
      unsigned char immobilmaske = abbild->bit_matrix[x][y] & LA_IMMOBILMASKE;

      if (immobilmaske == LA_STADT) { // Gilt auch fuer Weltbaut

	// Je nach Gattung der Stadt wird ein anderes Symbol dargestellt.
	// Damit ich ermittlen kann, welche Stadt dort steht, schaue ich
	// nochmal in der Objektemenge nach. Dadurch ergiebt sich aller-
	// dings folgendes Phaenomen: Ueber die Groesse bleibt ein Spieler
	// auf dem laufenden. Das Weltbild ist an dieser Stelle also 
	// Objektiv. Sollte die Stadt inzwischen aufgeloest worden
	// sein, so ergiebt sich ein besonderes Problem.

	OBJEKT *stadt_oder_weltbaut = NULL;
	FOR_EACH_OBJEKT_IN (alle_objekte_bei_adresse(adr)) DO_AND_DELETE
	(
	  if (objekt->typ_ist("STADT") || objekt->typ_ist("WELTBAUT"))
	    stadt_oder_weltbaut = objekt;
	)

	if (stadt_oder_weltbaut) {
	  static char filename[20];
	  char e_oder_g = (besitzer->besitzt(stadt_oder_weltbaut)?'e':'g');
	  sprintf(filename, "%c%s.gra",e_oder_g,
		    stadt_oder_weltbaut->attribut("GRAFIK"));
	  fn = filename;
	  noch_unbekannt = 0;
	}
      } // if (immobilmaske == LA_STADT)

      if (!fn && (mobilmaske & LA_EINHEIT)) {

	// In der mobilmaske ist nur vermerkt, dass ueberhaupt eine Einheit
	// an dieser Stelle gesehen wird. Die genauen Daten ueber alle Ein-
	// heiten hole ich mir aber von der realen Matrix der Welt.

	DOUBLIST *einheitenliste = alle_objekte_bei_adresse(adr);
	OBJEKT_LIST_NODE *objnode;
	objnode = (OBJEKT_LIST_NODE *)einheitenliste->first();
	OBJEKT *einheit=NULL;

	while (!objnode -> is_tail()) {
	  if (!objnode->objekt->zur_vernichtung_vorgemerkt() &&
	      objnode->objekt->typ_ist("EINHEIT")) {
	    einheit = objnode->objekt;
	    break;
	  }
	  objnode = (OBJEKT_LIST_NODE *)objnode->next();
	}
	delete einheitenliste;

	if (einheit) {
	  // Jetzt stelle ich den Filenamen zusammen.
	  char e_oder_g = (besitzer->besitzt(einheit)) ? 'e' : 'g';
	  static char filename[20];
	  sprintf(filename,"%c%s.gra",e_oder_g, einheit->attribut("GRAFIK"));
	  fn = filename;
	  noch_unbekannt = 0;
	}
      } // if (immobilmaske & LA_EINHEIT)

      if (fn == NULL) {
	  // Als letztes kommen die Gelaendeformen. fn ist NULL, wenn ent-
	  // weder keiner der beiden obigen Faelle zutraf (Stadt, Einheit),
	  // oder eine aufgeloeste Stadt gesichtet wurde. In diesem Fall
	  // wird naemlich die Gelaendeform dargestellt (falls keine Einheit
	  // dort steht)

	  unsigned short feldwert = abbild->matrix[x][y];
	  short gform = feldwert & 0xff;
	  short fattr = (feldwert & 0x7f00) / 0x100;

	  if (gform) {
	    noch_unbekannt = 0;
	    if (!laser) 
	      drucken(" %s", gelaendeform(gform)->attribute.abfragen("ABK"));
	    else // Jetzt zeichne ich noch die Gelaendeformen und Strassen ein.
	    {
	      LAYOUT *ly = feld_layout(layout_x, layout_oben + layout_y
						 , gform, fattr);
	      if (ly) {
	      
		// Jetzt zeichne ich noch die Strassen ein.
		float li = layout_x, ob = layout_oben + layout_y;
		const int strmaske = (0x8000 >> FELD_ATTRIBUT_STRASSE);
//		const int bsmaske = (0x8000 >> FELD_ATTRIBUT_BODEN_UNTERSUCHT);
		unsigned short **mat = abbild->matrix;


		if (mat[x][y] & strmaske)
		{
		  if (y>0 && (mat[x][y-1] & strmaske))
			ly->bitmatrix(li, ob, "strassee.gra");
		  if (y>0 && (mat[(x+1)%breite][y-1] & strmaske))
			ly->bitmatrix(li, ob, "strassed.gra");
		  if ((mat[(x+1)%breite][y] & strmaske))
			ly->bitmatrix(li, ob, "strassec.gra");
		  if (y<hoehe-1 && (mat[(x+1)%breite][y+1] & strmaske))
			ly->bitmatrix(li, ob, "strasseb.gra");
		  if (y<hoehe-1 && (mat[x][y+1] & strmaske))
			ly->bitmatrix(li, ob, "strassea.gra");
		  if (y<hoehe-1 && (mat[(x-1+breite)%breite][y+1] & strmaske))
			ly->bitmatrix(li, ob, "strasseh.gra");
		  if ((mat[(x-1+breite)%breite][y] & strmaske))
			ly->bitmatrix(li, ob, "strasseg.gra");
		  if (y>0 && (mat[(x-1+breite)%breite][y-1] & strmaske))
			ly->bitmatrix(li, ob, "strassef.gra");
		}

		ly->ausdrucken();
		delete ly;
	      } // if (ly)
	    } // else: Falls Laserdrucker erwuenscht.
	  } // Falls gform ausgegeben werden soll.
      } // Gelaendeform

      // Nun habe ich den Filename also. Jetzt erzeuge ich ein Layout und
      // drucke das Feld aus. Falls eine Gelaendeform ausgegeben werden soll,
      // so ist dies eh schon geschehen.

      if (noch_unbekannt && laser) fn = "ra"; // Raster drucken.

      if (laser && fn) {
	LAYOUT ly(layout_oben);
	ly.bitmatrix(layout_x, layout_y, fn);
	ly.ausdrucken();
      }
      else if (fn && !laser) {
	char token[4];
	token[3]=0; token[0]=' ';
	strncpy(token+1,fn,2);
	drucken(token);
      }
      else if (noch_unbekannt && !laser) drucken(" ? "); // Noch unbekanntes Gelaende.

    } // for (Jedes Feld in der Zeile)
    if (!laser) drucken("\n");
  } // for (Jede Zeile)
}


/**---------------------------------------------------------------------------
  ---------------------------------------------------------------------------*/
LAYOUT *WELT::feld_layout(float l, float o, ADR& adr)
{
  short gform = gelaendeform_in_feld(adr);
  short fattr = feld_attribute_maske(adr);
  return feld_layout(l, o, gform, fattr);
}


LAYOUT *WELT::feld_layout(float l, float o, short gform, short fattr)
{
  LAYOUT *ly = new LAYOUT;
  if (!ly) return NULL;

  short maske = 0x80 >> FELD_ATTRIBUT_BODEN_UNTERSUCHT;
  short bodensc = (0 != (fattr & maske));
  char *grafikfile = NULL;
  if (bodensc) grafikfile = gelaendeform(gform)->attribute.abfragen("BS_GRAFIK");
  if (!grafikfile || !grafikfile[0]) {
    if (bodensc) ly->bitmatrix(l, o, "bodensc.gra");
    grafikfile = gelaendeform(gform)->attribute.abfragen("GRAFIK");
  }
  ly->bitmatrix(l, o, grafikfile);
  return ly;
}


/**---------------------------------------------------------------------------
  * WELT::koordinatensystem()
  * 
  * Layoutet ein Koordinatensystem derart, dass ein Ausschnitt der Welt-
  * karte genau hineingedruckt werden kann.
  *
  * @param
  * float links, oben:  Eckpunkt des (inneren) Rechteckes (cm,cm)
  * float orand:        Oberer Rand des gesamten Layoutes (cm)
  * long  abreite,ahoehe: Groesse des zu druckenden Bereiches in Feldern
  ---------------------------------------------------------------------------*/
void WELT::koordinatensystem(float links, float oben, float orand,
  long abreite, long ahoehe, long xbera, long, long ybera, long yr,
  float feldbreite)
{
#define KOORDINATEN_ZAHLEN "Letter Gothic",6

  float rechts = links + abreite*feldbreite;
  float unten  = oben + ahoehe*feldbreite;
  LAYOUT ly(orand);
  ly.rechteck(links, oben, rechts, unten);

  // Jetzt male ich an der linken und an der oberen Seite Skalen an.
  // Jeder Strich mit einer durch 10 teilbaren Koordinate wird etwas
  // laenger.

  for (long zeile=0; zeile<ahoehe; zeile++)
  {
    long koord = ybera + zeile*yr;
    float l = links - 0.1 - (koord%10 == 0) * 0.15; // Jeder 10. Strich lang
    float r = rechts + 0.1 + (koord%10 == 0) * 0.15; // Jeder 10. Strich lang
    float o = oben + zeile*feldbreite + feldbreite/2;
    ly.linie(l, o, links, o);
    ly.linie(r, o, rechts, o);

    char string[10]; // Fuer den Aufbau der Zahlen mit sprintf
    sprintf(string,"%3ld",koord);
    ly.text(l - 0.49, o+0.08, string, KOORDINATEN_ZAHLEN);
    sprintf(string,"%-3ld",koord);
    ly.text(r,  o+0.08, string, KOORDINATEN_ZAHLEN);
  }

  for (long spalte=0; spalte<abreite; spalte++)
  {
    long koord = xbera + spalte;
    
    // Normalisierung. Betrag soll klein sein.
    if (koord > breite/2) koord -= breite;
    else if (koord <= -(breite/2)) koord += breite;
    
    float l = links + spalte*feldbreite + feldbreite/2;
    float o = oben - 0.1 - (koord%10 == 0) * 0.15;
    float u = unten + 0.1 + (koord%10 == 0) * 0.15;
    ly.linie(l, o, l, oben);
    ly.linie(l, u, l, unten);

    // Da die Zahlen hier waagrecht gedruckt werden, ist nicht genug Platz,
    // um jeden Strich zu numerieren. Deshalb mache ich das hier nur fuer
    // jeden zweiten.

    if (koord%2 == 0) {
      char string[10];
      sprintf(string,"%ld",koord);
      ly.text(l-0.08*strlen(string), o - 0.07, string,KOORDINATEN_ZAHLEN);
      ly.text(l-0.08*strlen(string), u + 0.22, string,KOORDINATEN_ZAHLEN);
    }
  }

  ly.ausdrucken();
}


/**---------------------------------------------------------------------------
  * WELT::alle_gegner_bei_adresse()
  * 
  * Liefert eine Liste aller Gegner eines Staates, die sich auf einem
  * bestimmten Feld befinden. Dies wird z.B. vom Angriffsbefehl benoe-
  * tigt. Nur EINHEITEN koennen Gegner sein.
  * @param
  * OBJEKT *staat: Der Staat, dessen gegnerische Einheiten
  * gesucht werden sollen.
  * ADR& adr:      Adresse, bei der gesucht werden soll.
  ---------------------------------------------------------------------------*/
DOUBLIST *WELT::alle_gegner_bei_adresse(OBJEKT *staat, ADR& adr)
{
  DOUBLIST *objliste = alle_objekte_bei_adresse(adr, "TYP=EINHEIT");

  // Jetzt muss ich nur noch einige Objekte aus der Liste loeschen, und
  // zwar alle, die dem Staat staat gehoeren.

  OBJEKT_LIST_NODE *objnode = (OBJEKT_LIST_NODE *)objliste->first();
  while (!objnode->is_tail()) {
    if (staat->besitzt(objnode->objekt)) {
      delete objnode;
      objnode = (OBJEKT_LIST_NODE *)objliste->first();
      continue;
    }
    else objnode = (OBJEKT_LIST_NODE *)objnode->next();
  }

  // So. Der Rest sind alles tatsaechliche Gegner.
  return objliste;
}


/**---------------------------------------------------------------------------
  * LANDSCHAFTSABBILD::LANDSCHAFTSABBILD() // constructor
  * 
  * Der Konstruktor legt zuerst die Daten fuer ein neues Landschaftsab-
  * bild an und kann dann entweder alles loeschen, oder alte Daten
  * aus einer Datei laden, aus der er auch den Namen des Besitzers
  * laedt.
  *
  * @param
  * b,h:    Breite, Hoehe der Landschaft
  * be:     Besitzer, falls geloescht werden soll, sonst NULL
  * file:   Datei, falls geladen werden soll, sonst ueberfluessig
  ---------------------------------------------------------------------------*/
LANDSCHAFTSABBILD::LANDSCHAFTSABBILD(long b, long h,
	 OBJEKT *be, FILE *file)
{
  if (file) { // Dann muss ich laden
    char tempname[MAX_LAENGE_NAME+2];
    fscanf(file, "%s",tempname);
    besitzer = mystrdup(tempname);
  }
  else besitzer = mystrdup(be->name);

  breite = b;
  hoehe = h;

  // Nun lege ich Speicherstrukturen fuer die beiden Matrizen an...

  // Im 1. Schritt lege ich die Zeigerleisten fuer die Spalten an.

  matrix = new unsigned short * [breite];
  bit_matrix = new unsigned char * [breite];

  long spalte;
  for (spalte = 0; spalte < breite; spalte++)
  {
    matrix[spalte] = new unsigned short [hoehe];
    bit_matrix[spalte] = new unsigned char [hoehe];

    // Nur, wenn nicht geladen werden soll, loesche ich hier.

    if (!file) for (long zeile = 0; zeile < hoehe; zeile++) {
      matrix[spalte][zeile] = 0;
      bit_matrix[spalte][zeile] = 0;
    }
  } // for(): Anforden von Speicher fuer die Spalten der Matrizen

  // Falls geladen werden soll, so tue ich das nun...
  if (file) {
    for (spalte=0; spalte<breite; spalte++) {
      fgethex(file, matrix[spalte], hoehe);
      fgethex(file, (char *)(bit_matrix[spalte]), hoehe);
    }
  }

}

/**---------------------------------------------------------------------------
  * LANDSCHAFTSABBILD::~LANDSCHAFTSABBILD() // destructor
  ---------------------------------------------------------------------------*/
LANDSCHAFTSABBILD::~LANDSCHAFTSABBILD()
{
  if (matrix && bit_matrix) {
    for (long spalte = 0; spalte<breite; spalte++) {
      delete matrix[spalte];
      delete bit_matrix[spalte];
    }
    delete matrix;
    delete bit_matrix;
    myfree(besitzer);
  }
}

/**---------------------------------------------------------------------------
  * LANDSCHAFTSABBILD::feld_aktualisieren()
  * 
  * Setzt ein Feld in der matrix auf einen bestimmten Wert und prueft
  * vorher die Adresse auf Korrektheit.
  *
  * @param
  * adr:     Adresse des Feldes
  * feld:    Neuer Wert
  ---------------------------------------------------------------------------*/
void LANDSCHAFTSABBILD::feld_aktualisieren(ADR& adr, unsigned short feld)
{
  if (adr.x >= breite || adr.x < 0 || adr.y >= hoehe || adr.y < 0)
  {
      log('I', "LANDSCHAFTSABBILD::feld_aktualisieren(): "
	 "Koordinaten (%d,%d) liegen ausserhalb!", adr.x, adr.y);
      return;
  }

  matrix[adr.x][adr.y] = feld;
}


/**---------------------------------------------------------------------------
  * LANDSCHAFTSABBILD::benutzten_bereich_ermitteln()
  * 
  * Ermittelt ein moeglichst kleines Rechteck im Abbild, dass alle
  * bekannten Felder (Gelaendeform != 0) enthaelt. Dieses dient
  * als Grundlage fuer die Weltkarte, die dem Spieler ausgedruckt wird.
  * Die Ergebnisse werden druch Transientparameter mitgeteilt.
  ---------------------------------------------------------------------------*/
void LANDSCHAFTSABBILD::benutzten_bereich_ermitteln
     (long& xstart, long& ystart, long& xende, long& yende, long mitte_x)
{

  // Zuerst setze ich beide Grenzen auf den jeweils gegenueberliegenden
  // Rand und lasse sie dann wandern.

  ystart = hoehe-1;
  yende = 0;

  // Definitionsgemaess befindet sich der bekannte Bereich in den Spalten,
  // die bei xstart beginnend durch Zaehlen in positive Richtung und Umbre-
  // chen am rechten Rand bis man zu xende kommt ueberstrichen werden.

  xstart = mitte_x;
  xende = mitte_x - 1;
  long scanstart = mitte_x-breite/2;
  long scanende = scanstart+breite;

  for (int spalte = scanstart; spalte < scanende; spalte++) {
    for (int zeile=0; zeile<hoehe; zeile++) {
      if (matrix[(spalte+breite) % breite][zeile]) {
	if (xende  < spalte) xende  = spalte;
	if (xstart > spalte) xstart = spalte;
	if (yende  < zeile)  yende  = zeile;
	if (ystart > zeile)  ystart = zeile;
      }
    }
  }

  xstart = (xstart + breite) % breite;
  xende = (xende + breite) % breite;

}

/**---------------------------------------------------------------------------
  * WELT::uebersichtskarte_drucken()
  * 
  * Druckt einer Uebersichtskarte ueber die komplette Welt, wobei anstatt
  * der normalen Grafikenformen nur Graustufen verwendet werden. Die Start-
  * punkte der Spieler werden durch Kreuze markiert.
  ---------------------------------------------------------------------------*/
void WELT::uebersichtskarte_drucken()
{
  // Zuerst berechne ich den Masstab. Die ganze Karte soll auf ein
  // Din-A4 Blatt passen. An Massstaeben stehen zur Auswahl
  // 3.0 2.5 2.0 1.5 1.0 0.5 0.4 0.3 0.2 0.1 mm pro Kaestchen
  
  float einheit = 0.3;
  while (einheit*hoehe > 17.5 || einheit*breite > 26) {
    if (einheit > 0.05) einheit -= 0.05;
    else einheit-=0.01;
  }

  // Querformat uebrigens.
  
  LAYOUT titel(dr_abschnitt(1.8));
  char string[200];
  sprintf(string,L("Welt '%s', %ld X %ld Felder, Querformat","World '%s', %ld X %ld Squares, Oblong Format"),
    objekt_mit_namen("Uhr") -> info("SESSIONNAME"), breite, hoehe);
  titel.text(1.5, 0.3, string, "Times", 15);
  titel.ausdrucken();
  

  // Nun kommt die Hauptschleife. Ich mache das Ganze zeilenweise, wobei
  // wegen dem Querformat aussen die X-Schleife von breite-1 nach 0 geht
  // und innen die Y-Schleife von hoehe-1 nach 0.
  
  for (long x=breite-1; x>=0; x--)
  {
    LAYOUT ly(dr_abschnitt(0));
    for (long y=hoehe-1; y>=0; y--)
    {
      // Als Grauwert hole ich mir ein Gelaendeformattribut mit dem Namen
      // GRAUWERT. 100 heisst schwarz, 0 heisst weiss.
      
      ADR adr(x,y);
      long grauwert = myatol(gelaendeform_attribut(adr, "GRAUWERT"));
      float links = einheit*(hoehe-y) + 1.5;
      float oben = einheit*(breite-x);
      ly.rechteck_ausgefuellt(links, oben, links+einheit, oben+einheit, grauwert);
      if (einheit>=0.25) { // Dann noch beschriften!
	char *text = gelaendeform_attribut(adr,"ABK");
	ly.text(links, oben+einheit-0.05, text,"Times",7 - (einheit==0.25));
      }

    }
    ly.ausdrucken();
  }
  
  dr_auswurf();
}      


/**---------------------------------------------------------------------------
  * WELT::gelaendeform_infos()
  * 
  * Druckt fuer alle Gelaendeformen eine Beschreibung aus.
  ---------------------------------------------------------------------------*/
void WELT::gelaendeform_infos()
{
  // Fuer Jede Gelaendeform einen kleinen Abschnitt
  
  short links=0; // Ich fange schon links an, wechsle aber immer vorher
  
  for (int form=1; form<gelaende_formen.get_anzahl(); form++)
  {  
    #define ATTR(c) (gelaende_formen.attribut_fuer_form(form,c)) // bequemer

    // Nicht jede Form wird ausgedruckt. Es gibt naemlich einige wenige
    // Formen, die nur wegen der Landschaftserschaffung oder wegen unter-
    // schiedlicher Bodenschaetze definiert sind. Diese doppelten Formen
    // haben das Attribut Doppelt.
    
    if (gelaende_formen.attribut_gesetzt_fuer_form(form,"Doppelt")) continue;
    
    // Ich will immer zwei Formen nebeneinander ausdrucken.
    links = 1 - links;
    
    const float L = 1.5, A = 3.5, B = 5.5, C = 7.7, D = 9.85;
    const float breite = D;
    const float hoehe = 1.5;
    char string[80];
    
    LAYOUT ly(dr_abschnitt(links ? hoehe : 0)); // Nur immer beim linken.
    ly.rechteck(0,0,breite,hoehe);

    // --------------------------------------------------------
    ly.bitmatrix(0.15, 0.15, ATTR("GRAFIK"),3);
    ly.linie(L,0,L,hoehe);
    // --------------------------------------------------------
    ly.text(L+0.15, 0.35, ATTR("REP"),"Times", 8);
    ly.bitmatrix(L+0.15 , hoehe-0.55, ATTR("GRAFIK"));

    char *zielform;
    if (  (NULL != (zielform = ATTR(L("WR", "DF")))) 
	  || (NULL != (zielform = ATTR(L("AF", "AF"))))
	  || (NULL != (zielform = ATTR(L("TR", "DS")))) )
    {
      char *grafik = gelaende_formen.attribut_fuer_form
		(gelaende_formen.form_mit_abkuerzung(zielform), "GRAFIK");

      char *b = L("TR","DS");
      if (ATTR(L("WR", "DF"))) b = L("WR","DF");
      else if (ATTR(L("AF","AF"))) b = L("AF","AF");
      
      ly.bitmatrix(L+1.45, 0.95, grafik);
      ly.linie(L+0.6, 1.25, L+1.4, 1.25);
      ly.linie(L+1.4, 1.25, L+1.3, 1.35);
      ly.linie(L+1.4, 1.25, L+1.3, 1.15);
      ly.text(L+0.8, 1.2, b, "Times",8);
    }                   

    if (ATTR(L("AC", "CS")))
    {
      char *grafik = gelaende_formen.attribut_fuer_form
		(gelaende_formen.form_mit_abkuerzung(ATTR(L("AC", "CS"))), "GRAFIK");

      ly.bitmatrix(L+1.45, 0.45, grafik);
      ly.linie(L+0.8, .75, L+1.4, .75);
      ly.linie(L+1.4, .75, L+1.3, .85);
      ly.linie(L+1.4, .75, L+1.3, .65);
      ly.linie(L+0.6 ,1.05, L+0.8, .75);
      ly.text(L+0.8, 0.7, L("AC","CS"), "Times",8);
    }                   
    ly.linie(A, 0, A, hoehe);
    // --------------------------------------------------------
    ly.text(A+0.05, 0.3, L("Ertrag f~ur die Stadt","Yield for the Town:"), "Times",7);
    char *res = ATTR("RESOURCEN");
    if (mystrlen(res) <= 1) res = "-";
    ly.text(A+1 - strlen(res)* .0833, 0.6, res, "Times", 8);

    if (ATTR("SOLAR")) {
      sprintf(string,L("   Solarkraft: +%sE","   Solar Energy: +%sE"), ATTR("SOLAR"));
      ly.text(A+L(0.2, 0.075), 0.95, string,"Times",6);
    }
    if (ATTR("DUENGER")) {
      sprintf(string,L("Kunstd~unger: +%sN","Fertilizer: +%sF"),ATTR("DUENGER"));
      ly.text(A+L(0.225, 0.4), 1.25, string,"Times",6);
    }
    else if (ATTR("HOLZ")) {
      ly.linie(A, 1.1, B, 1.1);
      sprintf(string,L("Waldroden bringt %s","Deforest. yields %s"),ATTR("HOLZ"));
      ly.text(A+0.05, 1.38, string, "Times", 6);
    }
    ly.linie(B,0,B,hoehe);
    // --------------------------------------------------------
    ly.text(B+ L(0.32, 0.525), 0.3,L("Fortbewegung","Movement"),"Times",7.5);
    if (mystrcmp(ATTR("ART"), "Hindernis")) {
      short faktor = myatol(ATTR("BEWEGUNG"));
      if (faktor > 1)
      {
	sprintf(string,"%dX",faktor);
	ly.text(B+0.61,1.04,string,"Times",22);
	ly.text(B+0.72,1.4,L("so lang","as long"),"Times",7.5);
      }
      else ly.text(B+0.50, 1, L("normal","normal"),"Times",12);
    }
    else ly.text(B+0.18,1,L("unm~oglich","impossible"),"Times",12);
    ly.linie(C,0,C,hoehe);
    // --------------------------------------------------------
    ly.text(C + L(0.01,.275) ,0.3,L("Verteidigungsbonus","Defense Bonus"),"Times", 7.5);
    if (ATTR("VERTEIDIGUNG")) {
      sprintf(string, "%s%%",ATTR("VERTEIDIGUNG"));
      ly.text(C+0.45 - (strlen(string)==4)*0.25 ,1.2,string,"Times",20);
    }
    else ly.text(C+0.9,1.25,"-","Times",35);
    // --------------------------------------------------------
    ly.ausdrucken(links ? 0 : D+0.1, links ? 0 : -hoehe);
  }
}


/**---------------------------------------------------------------------------
  * WELT::gelaendeform_infos_html(HTML&)
  * 
  * Druckt fuer alle Gelaendeformen eine Beschreibung aus in ein
  * offenes HTML-Objekt.
  ---------------------------------------------------------------------------*/
void WELT::gelaendeform_infos_html(HTML& html)
{
    html.set_table_border(1)
	.set_cell_spacing(0)
	.set_table_color("#d0d0d0")
	.table();

    int zeile = 0;
    // form 0 ist die Leere. Die gibt es nur waehrend der Welterschaffung
    for (int form=1; form<gelaende_formen.get_anzahl(); form++)
    {  
#define ATTR(c) (gelaende_formen.attribut_fuer_form(form,c)) // bequemer
	// Nicht jede Form wird ausgedruckt. Es gibt naemlich einige wenige
	// Formen, die nur wegen der Landschaftserschaffung oder wegen unter-
	// schiedlicher Bodenschaetze definiert sind. Diese doppelten Formen
	// haben das Attribut Doppelt.
	
	if (gelaende_formen.attribut_gesetzt_fuer_form(form,"Doppelt")) continue;

	if (zeile++ % 10 == 0)      // Spaltenueberschriften alle 10 Zeilen
	{
	    html.set_row_color("#e0e0e0")
		.next_row()
		.set_cell_alignment(AL_CENTER)
		.empty_cell()
		.next_cell(L("Gel~andeform","Territory"))
		.href_manual(L("AC","CS"))
		.next_cell(L("AC","CS")).end_href()
		.href_manual(L("WR","DF"))
		.next_cell(L("WR","DF")).end_href()
		.href_manual(L("AF","AF"))
		.next_cell(L("AF","AF")).end_href()
		.href_manual(L("TR","DS"))
		.next_cell(L("TR","DS")).end_href()
		.next_cell(L("Ertrag","Yield"))
		.next_cell().text(L("Wald","Deforest")).linebreak()
		.text(L("roden",""))
		.next_cell().text(L("Kunst-","Artificial ")).linebreak()
		.text(L("d~unger","Fertilizer"))
		.next_cell().text(L("Solar-","Solar ")).linebreak()
		.text(L("energie","Energy"))
		.next_cell(L("Bewegung","Movement"))
		.next_cell(L("V-Bonus","D-Bonus"))
		.unset_cell_alignment()
		.unset_row_color();
	}

	char string[80]; // Da werde ich Text hinein sprintfen.
    
	html.next_row()
	    .anchor(myltoa(form));

	html.next_cell() // Grafik
	    .fieldimage(ATTR("GRAFIK"));
	
	html.next_cell(ATTR("REP")); // Name

	char *umwandlung[4] = {L("AC","CS"),
			       L("WR","DF"),
			       L("AF","AF"),
			       L("TR","DS")};
	for (int i=0; i<4; i++)
	{
	    char *zielform = ATTR(umwandlung[i]);
	    if (zielform) {
		html.next_cell();
		char *grafik = gelaende_formen.attribut_fuer_form
		    (gelaende_formen.form_mit_abkuerzung(zielform), "GRAFIK");
		html.fieldimage(grafik);
	    }
	    else html.empty_cell();
	}

	html.set_cell_alignment(AL_CENTER);
	char *res = ATTR("RESOURCEN");   // Ertag fuer die Stadt
	if (mystrlen(res) <= 1) res = "-";
	html.next_cell(res);

	if (ATTR("HOLZ")) html.next_cell().text(ATTR("HOLZ"));
	else html.empty_cell();

	if (ATTR("DUENGER")) html.next_cell()
				 .text("+")
				 .text(ATTR("DUENGER"))
				 .text(L("N","F"));
	else html.empty_cell();

	if (ATTR("SOLAR")) html.next_cell()
			       .text("+")
			       .text(ATTR("SOLAR"))
			       .text(L("E","E"));
	else html.empty_cell();

	if (mystrcmp(ATTR("ART"), "Hindernis")) { // Fortbewegung
	    short faktor = myatol(ATTR("BEWEGUNG"));
	    if (faktor > 1) html.next_cell(faktor).text("X");
	    else html.next_cell(L("normal","normal"));
	}
	else html.next_cell().bold(L("unm~oglich","impossible"));

	if (ATTR("VERTEIDIGUNG")) {
	    sprintf(string, "%s%%",ATTR("VERTEIDIGUNG"));
	    html.next_cell().text(ATTR("VERTEIDIGUNG")).text("%");
	}
	else html.empty_cell();
	html.unset_cell_alignment();

    }
    html.end_table();
}

/**---------------------------------------------------------------------------
  * WELT::abwehr_durch_mobile_raketen()
  *
  * Schaut, ob es eine Einheit gibt, die eine anfliegende Rakete auf ein
  * bestimmtes Ziel abwehren will.
  *
  * @return
  * true, wenns abgewehrt wurde.
  ---------------------------------------------------------------------------*/
bool WELT::abwehr_durch_mobile_raketen(ADR& vonwo, ADR& ziel, long kraft, STAAT *schiesser)
{
    if (kraft < 100) return false; // Momentane moegliche Optimierung.
    
    bool abgewehrt = false;
    FOR_EACH_OBJEKT_IN (alle_objekte_im_umkreis_von(ziel, 2.5, "TYP=EINHEIT,Abwehrrakete"))
	DO_AND_DELETE ({
	    if (!abgewehrt && ((EINHEIT *)objekt)->wehre_rakete_ab(vonwo, ziel, kraft, schiesser))
		abgewehrt=1;
	});
    return abgewehrt;
}
