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
  * MODUL:               einheit.C / EINHEIT.CPP
  * AUTOR/DATUM:         Mathias Kettner, 28. April 1993
  * KOMPATIBILITAET:     C++
  ----------------------------------------------------------------------------
  *
  *      Enthaelt alle Funktionen zum Objekt Einheit, dass innerhalb des
  *      Spiels allen beweglichen Objekten zugrundeliegt.
  *
  ---------------------------------------------------------------------------*/

#include <string.h>
#include <ctype.h>

#include "einheit.h"
#include "stadt.h"
#include "adr_ric.h"
#include "prom.h"
#include "enzyklop.h"
#include "laengen.h"
#include "dauer.h"
#include "welt.h"
#include "kompatib.h"
#include "alg.h"
#include "drucker.h"
#include "layout.h"
#include "staat.h"
#include "listmac.h"
#include "uhr.h"
#include "log.h"

/**---------------------------------------------------------------------------
  * GLOBALE VARIABLE:
  ---------------------------------------------------------------------------*/
extern short laser; // Kommt aus main.C und steht auf 1, wenn der
		    // Ausdruck fuer den Laser formatiert werden soll.

extern long strategie_flags[ANZAHL_STRATEGIEN]; // Kommt aus stratab.h

extern DOUBLIST globale_objekt_menge;
extern UHR *g_uhr; 			// von main.cpp
extern ENZYKLOPAEDIE *g_enzyklopaedie;  // von main.cpp

/**---------------------------------------------------------------------------
  * MAKROS:
  *
  * fehlermeldung1(text);
  * fehlermeldung2(text);
  *
  * Wird nur in den Funktionen befehl_...() verwendet und dort zur
  * Meldung eines nicht ausfuehrbaren Befehls. Ist die aktuelle_priori-
  * taet ungleich 0, so bedeutet das, dass gerade ein automatisches
  * Programm ablaeuft und deshalb keine Fehlermeldungen erwuenscht sind.
  * Bei Version 1 wird automatisch der Befehl mitangegeben, der aber
  * dazu in der Variable befehl vorhanden sein muss.
  * Version 2 gibt lediglich einen Text ohne weitere Parameter aus.
  * Bei Version 3 kann noch ein Parameter angegeben werden, der fuer
  * einen %s oder %ld Platzhalter eingesetzt wird.
  *
  * fortsetzung(text);
  * fortsetzungp(text, parameter);
  * 
  * Reicht bei den oben genannten Makros eine Zeile nicht aus, so
  * kann mit einem dieser beiden Makros verlaengert werden. Die
  * report()-Funktion kann nicht verwendet werden, da sie die aktuelle
  * Prioritaet nicht abfragt!
  *
  ---------------------------------------------------------------------------*/

#define MAX_FEHLERMELDUNGEN 51 // Die 51. Meldung ist die Warnung. 

#define fehlermeldung1(text) { \
		  if (!aktuelle_prioritaet) { \
			  fehlermeldungsnummer++; \
			  if (fehlermeldungsnummer < MAX_FEHLERMELDUNGEN) { \
			    fussnoten_zaehler++; \
			    tagebuch_fuehren("*"," "); \
			    tagebuch_fuehren(myltoa(fussnoten_zaehler)); \
			    tagebuch_fuehren("*"); \
			    report("#\n%s: ",myltoa(fussnoten_zaehler)); \
			    report("%s: ",befehl); report("%s",text); } \
			  else if (fehlermeldungsnummer == MAX_FEHLERMELDUNGEN) \
			    fussnote(L("Weitere Fehlermeldungen werden unterdr~uckt!","Further error messages will not be displayed!")); \
			  } }

#define fehlermeldung2(text)  { \
		  if (!aktuelle_prioritaet) { \
			fehlermeldungsnummer++; \
			if (fehlermeldungsnummer < MAX_FEHLERMELDUNGEN) { \
			  fussnoten_zaehler++; \
			  tagebuch_fuehren("*"," "); \
			  tagebuch_fuehren(myltoa(fussnoten_zaehler)); \
			  tagebuch_fuehren("*"); \
			  report("#\n%s: ",myltoa(fussnoten_zaehler)); \
			  report("%s",text);  \
			} \
			else if (fehlermeldungsnummer == MAX_FEHLERMELDUNGEN) \
			  fussnote(L("Weitere Fehlermeldungen werden unterdr~uckt!","Further error messages will not be displayed!")); \
		     } }

#define fortsetzung(text) { if (!aktuelle_prioritaet && fehlermeldungsnummer <= MAX_FEHLERMELDUNGEN) report("%s",text); }
#define fortsetzungp(text,p) { if (!aktuelle_prioritaet && fehlermeldungsnummer <= MAX_FEHLERMELDUNGEN) report(text,p); }

#define fussnote(text)  { fussnoten_zaehler++; \
			tagebuch_fuehren("*"," "); \
			tagebuch_fuehren(myltoa(fussnoten_zaehler)); \
			tagebuch_fuehren("*"); \
			report("#\n%s: ",myltoa(fussnoten_zaehler)); \
			report("%s",text); }

/**---------------------------------------------------------------------------
  * EINHEIT::EINHEIT()            // constructor
  * 
  * Holt sich aus der Enzyklopaedie alle noetigen Daten und Attribute
  * ueber sich selbst wie Angriffskraft, Verteidigungskraft und Bewe-
  * gungsweite.
  ---------------------------------------------------------------------------*/
EINHEIT::EINHEIT(char *n, char *a) : OBJEKT(n,a)
{
  alter_zustand = NULL;
  fussnoten_zaehler = 0;
  fehlermeldungsnummer = 0;
  anzahl_kaempfe = 0;
  medaillen = 0;

  // Die Attribute wurden schon von objekt_schaffen() gesetzt

  if (!enzyklopaedie()) return; // Darf eigentlich nie passieren!

  VORHABEN_ENZ *eintrag = enzyklopaedie()->vorhaben(a_gattung());
  if (!eintrag) {
      log('K', "Unknown type '%s' of unit. Deleting unit '%s'. Check configuration file",
	  a_gattung(), n);
      zur_vernichtung_vormerken();
      return;
  }

  EINHEIT_ENZ *einheit = enzyklopaedie()->einheit(a_gattung());
  if (!einheit) {
      log('K', "Missing entry '%s' in section 'EINHEITEN:' of configuration file. Deleting"
	  " unit '%s'", a_gattung(), n);
     zur_vernichtung_vormerken();
     return;
  }

  angriffskraft = einheit->angriffskraft;
  verteidigungskraft = einheit->verteidigungskraft;
  bewegungspunkte_pro_feld = einheit->bewegungspunkte_pro_feld;
  sichtweite = einheit->sichtweite;
  
  // Waehrend des Ladens bin ich hier schon fertig, da die restlichen
  // Dinge beim Laden initialisiert werden.

  if (attribut_gesetzt("++LADEN++")) return;

  // Das Attribut VERSORGUNG gibt an, wieviele Resourcen die Einheit
  // benoetigt, um unterhalten zu werden. Der Wert steht in der Enzyklopaedie.

  attribut_setzen("VERSORGUNG",eintrag->unterhalt.to_string());

  // Die Variable aktuelle_prioritaet und aktuelle_strategie werden zu
  // Beginn auf 0 (normaler Befehl) bzw S_BEFEHLSGEBUNDEN gesetzt.

  aktuelle_prioritaet = 0;
  aktuelle_strategie = S_BEFEHLSGEBUNDEN;

  // Hier lese ich nochmals meine Attribute aus der Enzyklopaedie aus,
  // damit auch ein anderweitiges Schaffen der Einheit die richtigen
  // Attribute erzeugt.

  attribute.aus_string_einlesen(eintrag->start_attribute.to_string());

  beeinflussen(eintrag->einfluss_art, eintrag->einfluss_spez,
	       eintrag->einfluss_parameter);

  startort = NULL; // Falls ich waehrend einer Runde geschaffen wurde.

}


/**---------------------------------------------------------------------------
  * EINHEIT::~EINHEIT()            // destructor
  * 
  * Gibt nur einen dynamischen String frei.
  ---------------------------------------------------------------------------*/
EINHEIT::~EINHEIT()
{
  myfree(alter_zustand);
}


/**---------------------------------------------------------------------------
  * EINHEIT::speichern()
  * 
  * Speichert einige spezifische Objektdaten.
  *
  * @param
  * FILE *file: Bereits offenes Asciifile.
  *
  * @return
  * short 1 bei einem Fehler, 0 sonst.
  ---------------------------------------------------------------------------*/
short EINHEIT::speichern(FILE *file)
{
  fprintf(file, "%ld %ld %s", aktuelle_strategie,
	  aktuelle_prioritaet,  lager.to_string());
  return (ferror(file) != 0);
}

/**---------------------------------------------------------------------------
  * EINHEIT::laden()
  * 
  * Laedt die spezifischen Objektdaten.
  *
  * @param
  * FILE *file: Bereits offenes Asciifile.
  *
  * @return
  * short 1 bei einem Fehler, 0 sonst.
  ---------------------------------------------------------------------------*/
short EINHEIT::laden(FILE *file)
{
  fscanf(file, "%ld %ld", &aktuelle_strategie,
	  &aktuelle_prioritaet);
  char puffer[MAX_LAENGE_RESOURCEZEILE+2];
  fscanf(file, "%s",puffer);
  lager.setzen_auf(puffer);
  return (ferror(file) != 0);
}

/**---------------------------------------------------------------------------
  * EINHEIT::staat()
  * 
  * Ermittelt den Staat, dem die Einheit gehoert, in dem solange im
  * Besitzbaum nach oben gegangen wird, bis ein Objekt mit dem Typ
  * STAAT auftaucht oder kein Besitzer mehr existiert.
  *
  * @return
  * OBJEKT * Zeiger auf den Staat oder NULL, wenn es keinen gab.
  ---------------------------------------------------------------------------*/
STAAT *EINHEIT::staat()
{
  OBJEKT *objekt = besitzer;
  while (objekt && !objekt->typ_ist("STAAT")) objekt = objekt->besitzer;
  return (STAAT *)objekt;
}


/**---------------------------------------------------------------------------
  * EINHEIT::kommando()                  // virtuell
  * 
  * Behandelt die Kommandos, wie in OBJEKT::kommando() beschrieben.
  *
  * @param
  * kommando:                 Kommando als String
  * void *par1, *par2, *par3: Optionale Parameter.
  *
  * @return
  * 0, falls alles in Ordnung war.
  * 1, falls das Kommando einen Fehler hervorgerufen hat, unbekannt war..
  ---------------------------------------------------------------------------*/
short EINHEIT::kommando(const char *kommando, const void *par1, const void *par2, const void *)
{
  if (!mystrcmp(kommando, "ANGRIFF"))
	return kommando_angegriffen_werden((void *)par1, (void *)par2);
  else if (!mystrcmp(kommando, "BESCHUSS"))
	return kommando_beschossen_werden((void *)par1, (void *)par2);
  else if (!mystrcmp(kommando, "VERNICHTUNG"))
	return kommando_sich_vernichten();
  else if (!mystrcmp(kommando, "ALARM"))
	return kommando_alarm((void *)par1, (void *)par2);
  else if (!mystrcmp(kommando, "BEFEHLE_ABGEBROCHEN"))
	{ aktuelle_prioritaet=0; attribut_loeschen("Pendel_aktiv"); return 0;}
  else if (!mystrcmp(kommando, "ANGRIFF_FOLGT"))
	return kommando_angriff_folgt((void *)par1);
  else if (!mystrcmp(kommando, "AUSSTIEG"))
	return kommando_ausstieg();
  else if (!mystrcmp(kommando, "RESOURCEN_ANFORDERN"))
  	return kommando_resourcen_anfordern((void *)par1, (void *)par2);
  else if (!mystrcmp(kommando, "TAGEBUCH"))
	{tagebuch_fuehren((char *)(void *)par1, ", "); return 0;}

  else  return 1; // Unbekanntes Kommando. Macht wahrscheinlich nichts.
}


/**---------------------------------------------------------------------------
  * EINHEIT::kommando_resourcen_anfordern()
  * 
  * Brauchen Flugzeuge, um vom Flugzeugtraeger Oel zu bekommen. 
  ---------------------------------------------------------------------------*/
short EINHEIT::kommando_resourcen_anfordern(void *par1, void *par2)
{
  RESOURCE_VEKTOR forderung((char *)par1);
  OBJEKT *forderer = (OBJEKT *)par2;
  
  if (forderung.passt_in(lager)) { // wunderbar
    lager.subtrahiere(forderung);
    char string[50];
    sprintf(string, L("(%s betankt)","(%s refueled)"), forderer->name);
    tagebuch_fuehren(string, ", ");
    return 0;
  }
  else return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::kommando_ausstieg()
  * 
  * Teilt der Einheit mit, dass der Spieler soeben ausgestiegen ist.
  ---------------------------------------------------------------------------*/
short EINHEIT::kommando_ausstieg()
{
  alle_befehle_abbrechen();
  return 0;
}


/**---------------------------------------------------------------------------
  * EINHEIT::kommando_sich_vernichten()
  * 
  * Die Einheit wird aufgefordert, sich sofort zu vernichten und darueber
  * auch im Tagebuch Bescheid zu geben.
  *
  * @return
  * short 0;
  ---------------------------------------------------------------------------*/
short EINHEIT::kommando_sich_vernichten()
{
  tagebuch_fuehren(L("XX","DU"),", ");
  enthaltenen_einheiten_berichten(L("Unsere Transporteinheit l~ost sich auf!","Our transport unit has disbanded!"));
  gelaende_aktualisieren(); // Kann auch von Atombombe kommen. Krater sehen!
  zur_vernichtung_vormerken();
  enthaltene_objekte_vernichten();
  return 0;
}


/**---------------------------------------------------------------------------
  * EINHEIT::kommando_angegriffen_werden()
  * 
  * Dies ist eine relativ zentrale Funktion beim Angriff einer Armee.
  * Ueber dieses Kommando teilt eine Armee seinem Opfer mit, dass es
  * nun angegriffen wird und auch mit welcher Kraft. Wer gewinnt ent-
  * scheidet das Opfer in dieser Funktion, indem es die Anfgriffskraft
  * mit der eigenen Verteidigungskraft ins Verhaeltnis setzt und eine
  * Zufallszahl auf dieses Verhaeltnis wirft.
  *
  * @param
  * (long *)        Angriffskraft. Falls diese negativ ist, ist sie
  * positiv gemeint, und es handelt sich um eine 
  * Kaperaktion. Dann muss der Schurke gegen die
  * doppelte Gegenwehr ankommen.
  * (OBJEKT *)  Angreifer
  *
  * @return
  * short -vert_kraft, wenn der Angreifer gewonnen hat.
  * short +vert_kraft, wenn der Verteidiger gewonnen hat.
  * short 0: Bei vert-kraft 0 gewinnt immer der Angreifer.
  ---------------------------------------------------------------------------*/
short EINHEIT::kommando_angegriffen_werden(void *par1, void *par2)
{
    anzahl_kaempfe ++;

    // Zuerst hole ich mir die Werte, naemlich die Kraft des Angriffs und
  // den Namen des Angreifers.

  long angriffsstaerke = *(long *)par1;
  short kapern = angriffsstaerke < 0;
  if (kapern) angriffsstaerke = -angriffsstaerke;
  
  OBJEKT *angobj = (OBJEKT *)par2;
  RIC angric = welt()->richtung_von_nach(adresse, angobj->adresse);

  // Meine eigene Verteidigungs ermittelt eine Spezialfunktion. Ich muss
  // ihr den Ort des Angreifers deshalb mitteilen, da davon abhaengt, von
  // welchen benachbarten Feldern aus Einheiten bei der Verteidigung un-
  // terstuetzen koennen.

  long gesamt_verteidigung = effektive_verteidigungskraft(angobj->adresse);
  if (kapern) gesamt_verteidigung *= 2;
  
  // Wer gewinnt, ermittelt eine zentrale Funktion.
  bool verliere = gewinnt_kampf_gegen(angriffsstaerke, gesamt_verteidigung);
  bool ueberrannt = hat_uebermacht(angriffsstaerke, gesamt_verteidigung);
  
  log('f', "%s attacks %s with %ld against %ld and %s",
      angobj->a_gattung(), a_gattung(), angriffsstaerke, gesamt_verteidigung,
      verliere ? "wins" : "looses");
  
  char string[120], *woher;
  short aus_der_luft = (angobj->attribut_gesetzt("Luft"));

  if (aus_der_luft) woher = L("der Luft","from the air");
  else if (angric.x == 0 && angric.y == 0) woher = L("dem gleichen Feld","the same square");
  else woher = angric.to_string();
  char *aktion;
  if (kapern) aktion = L("Kaperversuch ","Capture attempt ");
  else aktion = L("Angriff","Attack");
  sprintf(string,L("%s der St~arke %ld aus %s gegen Verteidigung %ld: ","%s with Strength %ld from %s versus Defense %ld: "), aktion,
    angriffsstaerke, woher, gesamt_verteidigung);
  tagebuch_fuehren(string, ", ");

  if (verliere)
  {
    // Der Angreifer hat leider gewonnen. Ich muss mich vernichten, bzw.
    // gar nichts tun, wenn gerade gekapert wird. In diesem Fall wird alles
    // weitere mein Gegner uebernehmen.
    
    if (!kapern) {
    
      zur_vernichtung_vormerken();

      // Wenn ich eine Einheit bin, die anderen Einheiten CONVOY geboten
      // hat, dann muessen die anderen Einheiten auch vernichtet werden.
      // Das heisst alle Einheiten, die sich in mir befinden, muessen
      // von ihrem Untergang informiert werden.

      enthaltenen_einheiten_berichten(L("Unsere Transporteinheit wird vernichtet!","Our transport unit has been destroyed!"));
      enthaltene_objekte_vernichten();
 
      sprintf(string,L("Wir werden von %s %svernichtet","We are %sslain by %s"),
	      L(konjugation(angobj->a_gattung(),DATIV|SINGULAR), (ueberrannt ? "overrun and " : "")),
	      L((ueberrannt ? "~uberrannt und " : ""),konjugation(angobj->a_gattung(),DATIV|SINGULAR)));
      tagebuch_fuehren(string,"");
    }
    else { // gekapert. Alle Einheiten werden mitgekapert.
      sprintf(string,L("Wir werden von %s gekapert","We are captured by %s"),
	konjugation(angobj->a_gattung(), DATIV | SINGULAR));
      tagebuch_fuehren(string,"");
      FOR_EACH_OBJEKT_IN (alle_enthaltenen_objekte())
      DO_AND_DELETE ({
        char string[200];
        sprintf(string, L("%s %s aus %s mitgekapert","%s %s captured by %s"), objekt->a_gattung(),
          objekt->name, objekt->besitzer->a_name());
        tagebuch_fuehren(string, ", ");

        sprintf(string, L("(Mitgekapert bei der Kaperung von %s %s)","(Additionally captured while capturing  %s %s)"),
          a_gattung(), name);
        objekt->tagebuch_fuehren(string, ", ");
      })
    }
      
    // Der Rueckgabewert informiert ueber den Erfolg
    return -gesamt_verteidigung;
  }

  else { // Gewonnen!
      if (aus_der_luft)
	  sprintf(string,L("Der Angriff %s bleibt erfolglos","Attack %s : No result."),
		  konjugation(angobj->a_gattung(), GENITIV | SINGULAR));
      else
	  sprintf(string,L("Wir besiegen %s","We defeat %s"),
		  konjugation(angobj->a_gattung(), AKKUSATIV | SINGULAR));
      tagebuch_fuehren(string,"");
      // Jetzt muesste man wissen, ob sich der Angreifer zurueckziehen kann.
      return gesamt_verteidigung; // Einheit hat ueberlebt
  }

}


/**---------------------------------------------------------------------------
  * EINHEIT::kann_ueberrannt_werden(long, EINHEIT *)
  * 
  * Ermittelt, ob ein Angreifer die Uebermacht gegen micht hat. Wenn ja,
  * dann kann er micht ueberrennen.
  ---------------------------------------------------------------------------*/
bool EINHEIT::kann_ueberrannt_werden(long kampfkraft, EINHEIT *gegner)
{
  long gesamt_verteidigung = effektive_verteidigungskraft(gegner->adresse);
  return hat_uebermacht(kampfkraft, gesamt_verteidigung);
}



/**---------------------------------------------------------------------------
  * EINHEIT::kommando_beschossen_werden()
  * 
  * Diese Kommando ruft eine Einheit auf, wenn sie eine andere beschiesst.
  * Sie teilt der beschossenen Einheit ihre Identitaet und die Feuerkraft
  * mit. In dieser Funktion wertet der beschossene die Daten aus und setzt
  * die Feuerkraft mit seiner Verteidigungskraft ins Verhaeltnis. Be-
  * zeichnet sie sich selbst als getroffen, so gibt sie im Tagebuch
  * Meldung und vernichtet sich.
  * Der Staat soll im uebrigen den DS kontrollieren.
  *
  * Feuerkraft 9999 bedeutet atomaren Beschuss aus einem Raketensilo.
  *
  * @param
  * (char *)        Feuerkraft
  * (OBJEKT *)  Bombardeur
  *
  * @return
  * short abwehrkraft, wenn die Einheit vernichtet wurde
  * short 0, falls nicht getroffen wurde,
  * short -1, falls getroffen wurde, aber die abwehrkraft 0 war.
  ---------------------------------------------------------------------------*/
short EINHEIT::kommando_beschossen_werden(void *par1, void *par2)
{
    anzahl_kaempfe ++;

    long feuerkraft = myatol((char *)par1);
    OBJEKT *bombardeur = (OBJEKT *)par2;
    short aus_luft = (bombardeur->attribut_gesetzt("Luft"));
    
    short atomar = (feuerkraft == 9999); // Signal fuer Atomrakete.
  
  // DS ueberpruefen.
  staat()->kommando("AGGRESSION",par2);

  // Wenn der Schuss aus meinem Sichtbereich kommt, dann moechte ich
  // doch gerne erfahren, woher der Schuss kam.
  
  ADR quelle = bombardeur->adresse;
  RIC ric = welt()->richtung_von_nach(adresse, quelle);
  char string[80];
  if (aus_luft) sprintf(string, L("Bombardement mit St~arke %ld aus der Luft","Aerial Bombardment with  Strength %ld "), feuerkraft);
  else {
      const char *teil1 = (atomar ? L("Atomrakete","Nuclear Missile")
		     : L("Beschu~s mit St~arke ","Bombardment with Strength "));
      const char *teil2 = (atomar ? "" : myltoa(feuerkraft));
      
      if (welt()->entfernung_zwischen(quelle, adresse) <= sichtweite)
	  sprintf(string, L("%s%s von %s","%s%s of %s"), teil1, teil2, ric.to_string());
      else 
	  sprintf(string, L("%s%s grob aus Richtung %s","%s%s from direction %s"), teil1, teil2, ric.grob_to_string());
  }
  
  tagebuch_fuehren(string, ", ");

  // Nun muss ich irgendwie entscheiden, wieviel mir dass eigentlich
  // ausmacht. Dazu ziehe ich meinen Verteidigungswert zu rate. Ach ja,
  // bei Atomraketen eruebrigt sich wohl eine detailierte Berechnung.
  
  long verteidigung = (atomar ? 0 : myatol(info_verteidigungskraft("KeineSM")));
  if (attribut_gesetzt("FEUERSCHUTZ")) { // Wird von der Feuerkraft abgezogen.
      long feuerschutz = myatol(attribut("FEUERSCHUTZ"));
      feuerkraft -= feuerschutz;
      char string[256];
      if (feuerkraft <= 0)
	  sprintf(string, L("Unser Feuerschutz von %ld ist gr~o~ser als die Feuerkraft %ld.","Our protection value %ld is higher than the firepower %ld.")
		  L(" Der Schu~s kann uns gar nicht erreichen."," The shot cannot even reach us."), feuerschutz,
		  feuerkraft + feuerschutz);
      else sprintf(string, L("Von der Feuerkraft %ld wird unser Feuerschutz %ld abgezogen. "," Protection value %ld is deducted from our firepower %ld. ")
		   L("Die restliche Feuerkraft ist %ld.","Remaining firepower is  %ld."), feuerschutz + feuerkraft,
		   feuerschutz, feuerkraft);
      fussnote(string);
  }

  if (feuerkraft <= 0) return 0; // Kann mich nicht erreichen.
  
  if (atomar || gewinnt_kampf_gegen(feuerkraft, verteidigung))
  {
    // Das war's. Ich bin im ARSCH.
    if (atomar) {
	log('f', "%s fires on %s with nuclear rocket against %ld and hits",
	    bombardeur->a_gattung(), a_gattung(), verteidigung);
	tagebuch_fuehren(L("wir werden pulverisiert","we are annihilated."), ", ");
	enthaltenen_einheiten_berichten(L("Unsere Transporteinheit wird atomisiert!","Our transport unit is torn into pieces!"));
	gelaende_aktualisieren(); // Damit ich den Krater sehe.
    }
    else {
	log('f', "%s fires on %s with %ld against %ld and hits",
	    bombardeur->a_gattung(), a_gattung(), feuerkraft, verteidigung);
	tagebuch_fuehren(L("wir werden getroffen und vernichtet","we are HIT! and destroyed"),", ");
	enthaltenen_einheiten_berichten(L("Unsere Transporteinheit wird getroffen und vernichtet!","Our transport unit is hit and destroyed!"));
    }
    zur_vernichtung_vormerken();
    enthaltene_objekte_vernichten();

    return verteidigung ? verteidigung : -1;
  }

  else {
	log('f', L("%s fires on %s with %ld against %ld and misses","%s fires at %s with %ld against %ld and misses"),
	    bombardeur->a_gattung(), a_gattung(), feuerkraft, verteidigung);
	return 0; // Puh... Nicht getroffen, Angriff fehlgeschlagen
  }
}


/**---------------------------------------------------------------------------
  * EINHEIT::wehre_rakete_ab()
  * 
  * Wird nur fuer mobile Abwehrraketen aufgerufen. Gibt eine Meldung
  * aus, dass eine Rakete vernichtet wurde und vernichtet sich an-
  * schliessend selbst.
  * @param
  * ADR& vonwo: 	daher kam der Beschuss
  * ADR& ziel:          dahin geht er
  * long feuerkraft:	So stark ist er. 9999 = Atomar
  * STAAT *schiesser:   Der Boesewicht.
  ---------------------------------------------------------------------------*/
bool EINHEIT::wehre_rakete_ab(ADR& vonwo, ADR& ziel, long kraft, STAAT *schiesser)
{
    if (kraft < 100 || attribut_gesetzt("Unversorgt") ||
	welt()->entfernung_zwischen(ziel, adresse) > myatof(attribut("RANGE")))
	return false;

    if (wir_lieben(schiesser)) return false; // Nur von Gegnern und Neutralen abschiessen.
  
    bool atomar = (kraft == 9999);
    RIC ric = welt()->richtung_von_nach(adresse, vonwo);
    char string[80];
    const char *teil1 = (atomar ? L("Atomrakete","Nuclear Missile")
		   : L("Beschu~s mit St~arke ","Bombardment with Strength "));
    const char *teil2 = (atomar ? "" : myltoa(kraft));
    
    if (welt()->entfernung_zwischen(vonwo, adresse) <= sichtweite)
	sprintf(string, L("%s%s von %s: ","%s%s from %s: "), teil1, teil2, ric.to_string());
    else 
	sprintf(string, L("%s%s grob aus Richtung %s: ","%s%s from direction %s: "), teil1, teil2, ric.grob_to_string());

    tagebuch_fuehren(string, ", ");
    tagebuch_fuehren((char *)(atomar ? L("Atomrakete zerschossen, die auf","Nuclear missile smashed. (It was aimed at ")
		     : L("Rakete zerschossen, die auf","Missile destroyed. (It was aimed at")), "");
    
    if (ziel == adresse) tagebuch_fuehren(L(" unser Feld gehen sollte"," one of our squares! )"), "");
    else {
	RIC ric = welt()->richtung_von_nach(adresse, ziel);
	sprintf(string, L(" Feld %s gehen sollte"," our square %s!)"), ric.to_string());
	tagebuch_fuehren(string, "");
    }
    
    zur_vernichtung_vormerken();
    return true; // Erfolgreich abgewehrt.
}


/**---------------------------------------------------------------------------
  * EINHEIT::kommando_angriff_folgt()
  * 
  * Wird vom Gegner aufgerufen, der sich gerade anschickt, mich
  * anzugreifen. Ich loese einen Alarm aus und setzte einen Zaehler
  * auf 2, der meinen Bewegungsbefehle ein oder zwei Runden lang
  * suspendiert. Letztliches wird unterlassen, wenn der Angriff von
  * einer Lufteinheit kam.
  * Ausserdem lasse ich vom Staat den DS checken, ob uns nicht ein
  * Freund uebers Ohr gehauen hat.
  * @param
  * (OBJEKT *)par1:     Angreifer
  * @return
  * 0, wenn OK.
  * 1, wenn einer der beiden Einheiten nicht auf der Welt ist.
  ---------------------------------------------------------------------------*/
short EINHEIT::kommando_angriff_folgt(void *par1)
{
  OBJEKT *angreifer = (OBJEKT *)par1;

  // Der Status wird ueberprueft.
  staat()->kommando("AGGRESSION",par1);

  attribut_setzen("ANGRIFF_FOLGT","2");

  if (!ort_ist_welt() || ort() != angreifer->ort()) return 1;

  RIC ric = welt()->richtung_von_nach(adresse, angreifer->adresse);
  alarmiert_werden(A_FEIND_GREIFT_AN, ric.to_string());
  return 0;
}


/**---------------------------------------------------------------------------
  * EINHEIT::enthaltene_objekte_vernichten()
  * 
  * Vernichtet alle Objekte, die in der Einheit enthalten sind. Das
  * ist zum Beispiel bei Schiffen relevant, die versenkt werden.
  ---------------------------------------------------------------------------*/
void EINHEIT::enthaltene_objekte_vernichten()
{
  FOR_EACH_OBJEKT_IN (alle_enthaltenen_objekte()) DO_AND_DELETE
  (
    objekt->kommando("VERNICHTUNG");
  )
}


/**---------------------------------------------------------------------------
  * EINHEIT::info()
  * 
  * Definiert die virtuelle Funktion info() von OBJEKT. Folgende
  * Informationen kann man von einer Einheit bekommen.
  *
  * @param
  * char *info:     String mit der Klassifizierung
  *
  * @return
  * Alle Antworten sind in Form eines Strings, der im Besitz der
  * info() Funktion bleibt und bei laengerem Gebrauch verdoppelt
  * werden muss. Falls eine unbekannte Informationsklasse ange-
  * fordert wird, wird ein Leerstring ("") zurueckgegeben (nicht
  * NULL!)
  ---------------------------------------------------------------------------*/
char *EINHEIT::info(char *info, void *par1, void *par2, void *par3)
{
  if      (!mystrcmp(info,"VERTEIDIGUNGSKRAFT")) return info_verteidigungskraft(par1);
  else if (!mystrcmp("RESSOURCEN_ANFORDERN",info)) return info_resourcen_anfordern(par1, par2, par3);
  else if (!mystrcmp("OFFENSIVKRAFT",info)) return info_offensivkraft();
  else if (!mystrcmp("ANGRIFFS_UNTERSTUETZUNG",info)) return info_angriffs_unterstuetzung();
  else if (!mystrcmp("VERT_UNTERSTUETZUNG",info)) return info_verteidigungs_unterstuetzung();
  else if (!mystrcmp("SICHTWEITE",info)) return myftoa(sichtweite);

  else return NULL; // Info unbekannt.
}

/**---------------------------------------------------------------------------
  * EINHEIT::info_resourcen_anfordern()
  * 
  * Veranlasst die Einheit, etwas von ihrer Ladung herauszuruecken,
  * wenn die geht. Flugzeuge koennen nur rausruecken, wenn sie bei
  * einer Basis sind.
  * Ist der Wert negativ, so bekommt die Einheit Ressourcen!
  * @param
  * (char *)par1:           Ressourcestring
  * (OBJEKT *)par2:     Partner
  * (long *)par3:           Maximalbetrag
  * @return
  * char *: Resourcestring (::to_string()) von den bekommenen Resourcen.
  ---------------------------------------------------------------------------*/
char *EINHEIT::info_resourcen_anfordern(void *par1, void *par2, void *par3)
{
  // Wenn ich ein Flugzeug bin, dann muss eine Basis hier sein.
  if (attribut_gesetzt("Luft")) {
     if (!basis_hier()) return ".";
  }

  RESOURCE_VEKTOR erwuenscht((char *)par1);
  OBJEKT *geber = (OBJEKT *)par2;
  long max_betrag = *(long *)par3;

  if (erwuenscht.ist_negativ()) // Lieferung erhalten
  {
    if (!attribut_gesetzt("LAGER")) return ".";
    erwuenscht.negieren();
    RESOURCE_VEKTOR bekomme;
    bekomme.einheitsvektor_aus(attribut("LAGER"));
    bekomme.multipliziere_mit(200000000L);
    bekomme.begrenzen_auf(erwuenscht);
    bekomme.begrenzen_auf(myatol(attribut("KAPAZ") - lager.betrag()));
    if (!bekomme.ist_null()) {
      lager.addiere(bekomme);
      if (geber) {
	tagebuch_fuehren(L("~Ubernahme von ","Seizure of "),", ");
	tagebuch_fuehren(L(" von "," of "),bekomme.to_string());
	tagebuch_fuehren(geber->name);
      }
      else { // Wurde von mir selbst aufgerufen!
	tagebuch_fuehren("("," ");
	tagebuch_fuehren(L(" eingeladen)"," loaded)"),bekomme.to_string());
      }
    }
    return bekomme.to_string();
  }

  else // Ressourcen ausliefern
  {
    erwuenscht.begrenzen_auf(lager);
    erwuenscht.begrenzen_auf(max_betrag);

    if (erwuenscht.ist_null()) return ".";

    tagebuch_fuehren(L("~Ubergabe von ","Transfer of "),", ");
    tagebuch_fuehren(L(" an "," to "),erwuenscht.to_string());
    tagebuch_fuehren(geber->name);

    lager.subtrahiere(erwuenscht);
    return erwuenscht.to_string();
  }
}


/**---------------------------------------------------------------------------
  * EINHEIT::info_verteidigungskraft()
  * 
  * Liefert Info zur Verteidigungskraft einer Einheit, jedoch OHNE
  * Beruecksichtigung eventueller Unterstuetzung.
  * Fuer eine Gesamtberechnung ist die Funktion
  * effektive_verteidigungskraft(ADR&) zu verwenden.
  *
  * Man kann angegen, ob man einen Stadtbonus, Stadtmauer und Zitadelle
  * miteinberechnen will oder nicht. Beim Beschuss braucht man es ohne,
  * beim Angriff mit.
  *
  * @param
  * (char *) NULL, wenn mit Stadtmauer, "KeineSM", wenn ohne.
  * @return
  * char *: long-Zahl als ASCII-String: Verteidigungskraft.
  ---------------------------------------------------------------------------*/
char *EINHEIT::info_verteidigungskraft(const void *par1)
{
  short mit_stadtmauer = (par1 == NULL);

  // Wenn die Einheit sich im Konvoi befindet, dann kann sie gar nicht ver-
  // teidigen.

  if (!ort_ist_welt()) return "0";

  // Ferner kann es sein, dass ich in Reserve bin, d.h. nicht mobil. In so
  // einem Fall sieht es natuerlich schlecht aus. Gleiches Gilt fuer eine
  // Einheit, die Unversorgt ist.

  if (attribut_gesetzt("Reserve") || attribut_gesetzt("Unversorgt"))
    return "0";

  // Schiffe, die nicht auf See sind, k~onnen sich ebenfalls nicht verteidigen.

  if (attribut_gesetzt("See") && !mystrcmp(gelaende_attribut("ART"),"Boden"))
     return "0";

  // Die Verteidigungskraft haengt von verschiedenen Faktoren ab. Die Ausgangs-
  // staerke habe ich in meiner Variablen 'angriffskraft'

  long verteidigung = angriffskraft;

  // Wenn ich mich eingegraben habe (befestigt bin), dann darf ich die
  // Verteidigungskraft addieren (die durchaus auch 0 sein kann).

  if (attribut_gesetzt("Eingegraben")) verteidigung += verteidigungskraft;

  // Je nach dem Verteidigungsbonus/ -malus des Feldes, auf dem ich mich
  // befinde, wird ein Vielfaches meines Verteidigungswertes addiert. Der
  // Gelaendeattributswert gibt einen Prozentwert an! 50 bedeutet eine
  // 50% ige Addition der V-Kraft.

  long vert = myatol(landschaft()->gelaendeform_attribut(adresse,"VERTEIDIGUNG"));
  verteidigung += (vert * verteidigungskraft)/100;

  // Der Einfluss SM (Stadtmauer) addiert einen Teil des V-Wertes
  if (mit_stadtmauer) {
      // Falls ich mich bei einer Stadt befinde, die im Ausnahmezustand ist, dann
      // bekomme ich von ihr keinen Bonus. Die kann ich leider nicht bei stadt.cc
      // implementieren, weil ein Objekt die Einfluesse so schlecht kontrollieren
      // kann, die von ihm ausgehen :-(. Also hier ein bisschen mogeln.

      STADT *stadt = stadt_hier();
      if (!stadt || !stadt->ausnahmezustand())
	  verteidigung += (einfluss_aufsummieren("SM") * verteidigungskraft) / 100;
  }

  // So. Jetzt gebe ich den Wert zurueck. Der String bleibt aber meiner,
  // und muss vom Aufrufer kopiert werden, wenn er ihn laenger braucht.

  return myltoa(verteidigung);
}


/**---------------------------------------------------------------------------
  * EINHEIT::info_angriffs_unterstuetzung()
  * 
  * Berechnet die Kraft, mit der diese Einheit eine andere beim
  * Angriff unterstuetzen kann. Die Halbierung des Wertes erfolgt
  * HIER aber noch NICHT (Zur Erinnerung: Eine Einheit kann eine
  * andere beim Angriff nur mit der Haelfte ihrer Kraft unterstuetzen).
  * Grund dafuer ist der Wunsch, die Rundungsungenauigkeit moeglichst
  * erst ganz am Ende der Berechnung zu bekommen, so dass auch Einheiten
  * mit der Kraft 1 unterstuetzen koennen, wenn sie zu zweit sind.
  ---------------------------------------------------------------------------*/
char *EINHEIT::info_angriffs_unterstuetzung()
{
  // Soll diese Einheit eine andere unterstuetzen, so teilt sie ueber dieses
  // Info dem Angreifer die verfuegbare Angriffskraft mit, mit der sie
  // unterstuetzen kann.

  if (zur_vernichtung_vorgemerkt()
     || attribut_gesetzt("Reserve")
     || attribut_gesetzt("Unversorgt")
     || attribut_gesetzt("Eingegraben")) return "0";

  else return myltoa(angriffskraft);
}


/**---------------------------------------------------------------------------
  * EINHEIT::info_verteidigungs_unterstuetzung()
  * 
  * Berechnet die Kraft, mit der diese Einheit eine andere bei der
  * Verteidigung unterstuetzen kann.
  ---------------------------------------------------------------------------*/
char *EINHEIT::info_verteidigungs_unterstuetzung()
{
  // Soll diese Einheit eine andere unterstuetzen, so teilt sie ueber dieses
  // Info dem Angreifer die verfuegbare Verteidigungskraft mit, mit der sie
  // unterstuetzen kann.

  // Im Vergleich zum Angriff gibt es folgende Unterschiede.
  // 1. Es wird nur mir der Verteidigungskraft unterstuetzt.
  // 2. Auch befestigte Einheiten koennen unterstuetzen.

  if (zur_vernichtung_vorgemerkt()
     || attribut_gesetzt("Reserve")
     || attribut_gesetzt("Unversorgt")) return "0";

  else return myltoa(verteidigungskraft);
}



/**---------------------------------------------------------------------------
  * EINHEIT::info_offensivkraft()
  * 
  * Ist von rein statistischem Interesse und gibt NICHT die relevante
  * Angriffskraft einer Einheit fuer einen Angriff an. Sie wird zum
  * Beispiel fuer die Punktewertung verrechnet.
  ---------------------------------------------------------------------------*/
char *EINHEIT::info_offensivkraft()
{
  long kraft = angriffskraft;
  kraft += myatol(attribut("FEUERKRAFT"));
  kraft += myatol(attribut("BOMBEN")) / 2;
  return myltoa(kraft);
}


/**---------------------------------------------------------------------------
  * EINHEIT::sichtungen_ermitteln()
  * 
  * Dies ist eigentlich keine Infofunktion, denn sie kann nicht ueber
  * info() erreicht werden. Sie aber eh' nur intern benuetzt.
  * Sie liefert eine Beschreibung aller immobilen Objekte und aller
  * fremder Einheiten, die ich gerade sehe zusammen mit der
  * Angabe, in welcher Richtung. Ausserdem gibt Sie den Ort der
  * Einheit aus, wenn sie sich nicht auf der Welt befindet.
  *
  * Beispiele: [Panzer NW] [Hauptstadt Uruk O] [NN]
  * (Im Universum)
  * (in Uruk)
  *
  * @param   
  * DOUBLIST *objektliste:
  * Falls schon eine Liste aller Objekte der Umgebung
  * existiert, kann diese hier verwendet werden, um
  * viel Zeit zu sparen. Ansonsten NULL.
  * @return
  * dynamischer (!) String, der den aktuellen Zustand wiederspiegelt.
  ---------------------------------------------------------------------------*/
char *EINHEIT::sichtungen_ermitteln()
{ return sichtungen_ermitteln(NULL); }

char *EINHEIT::sichtungen_ermitteln(DOUBLIST *objektliste)
{
  if (!ort_ist_welt()) // Kein Bericht moeglich
  {
    if (!ort()) return mystrdup(L("(Im Universum)","(In the Universe )"));
    char *n = ort()->a_name();
    if (!n || !*n) n = ort()->name;
    char string[MAX_LAENGE_NAME+20];
    sprintf(string, L("(in %s)","(at %s)"),n);
    return mystrdup(string);
  }

  char *antwort = mystrdup(""); // Damit kein NULL-String als Ergebnis kommt.

  // Jetzt fordere ich eine Beschreibung der Objekte an, die ich
  // sehe. Dazu dient ein info der Welt. Ich gebe ihr ausser
  // meiner Adresse auch noch den Namen meines Staates mit, damit
  // nur gegnerische Einheiten in der Beschreibung auftauchen.

  if (ort_ist_welt()) {
     char *objektbericht = welt()->objekt_aufzaehlung(this, objektliste);
     if (objektbericht) {
       mystradd(antwort," ");
       mystradd(antwort, objektbericht);
       myfree(objektbericht);
     }
  }

  return antwort;
}


/**---------------------------------------------------------------------------
  * EINHEIT::gelaende_attribut()
  * 
  * Liefert den Wert des Gelaendeattributes derjenigen Gelaendeform,
  * auf der die Einheit gerade steht, falls sie in der Welt ist.
  *
  * @param
  * char *klasse:   Klasse des Attributes
  *
  * @return
  * char *: Wert des Attributes, oder NULL, wenn das Attribut nicht
  * gesetzt ist, oder sich die Einheit nicht in der Welt be-
  * findet.
  ---------------------------------------------------------------------------*/
char *EINHEIT::gelaende_attribut(char *klasse)
{
  if (!ort_ist_welt()) return NULL; // Geht natuerlich nur dort!
  return landschaft()->gelaendeform_attribut(adresse, klasse);
}


/**---------------------------------------------------------------------------
  * EINHEIT::stadt_hier()
  * 
  * Untersucht, ob sich die Einheit auf dem gleichen Feld wie eine Stadt
  * befindet, und ermittelt einen Zeiger darauf, falls ja.
  *
  * @return
  * STADT *: Zeiger auf die Stadt oder NULL
  ---------------------------------------------------------------------------*/
STADT *EINHEIT::stadt_hier()
{
  if (!ort_ist_welt()) return NULL;

  DOUBLIST *objliste = welt()->alle_objekte_bei_adresse
						(adresse, "TYP=STADT");
  STADT *stadt;
  if (objliste->is_empty()) stadt = NULL;
  else stadt = (STADT *)((OBJEKT_LIST_NODE *)objliste->first())->objekt;
  delete objliste;
  return stadt;
}


/**---------------------------------------------------------------------------
  * EINHEIT::weltbaut_hier()
  * 
  * Untersucht, ob sich die Einheit auf dem gleichen Feld wie eine
  * Weltbaut befindet, und ermittelt einen Zeiger darauf, falls ja.
  *
  * @return
  * WELTBAUT *: Zeiger auf die Weltbaut oder NULL
  ---------------------------------------------------------------------------*/
WELTBAUT *EINHEIT::weltbaut_hier()
{
  if (!ort_ist_welt()) return NULL;

  DOUBLIST *objliste = welt()->alle_objekte_bei_adresse
						(adresse, "TYP=WELTBAUT");
  WELTBAUT *weltbaut;
  if (objliste->is_empty()) weltbaut = NULL;
  else weltbaut = (WELTBAUT *)((OBJEKT_LIST_NODE *)objliste->first())->objekt;
  delete objliste;
  return weltbaut;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_auswerten()
  * 
  * Befehlsauswertenfunktion von EINHEIT wie in objekt.h definiert.
  *
  * @param
  * char *befehl: Gesamter Befehl, wie er in BEMASKE eingegeben wird.
  * long runde:   Aktuelle Spielphase.
  *
  * @return
  * short: Standardzurueckgabe nach gaengigen Konventionen:
  * 0, wenn der Befehl noch nicht fertig abgearbeitet wurde und in
  * der naechsten Runde nochmal drankommen soll.
  * 1, wenn der Befehl nun fertig abgearbeitet wurde und gleich mit
  * dem naechsten angefangen werden kann. Der Befehl wird dann entfernt
  * und der Rundezaehler wieder auf 0 gesetzt.
  * -1, wenn der Befehl sich selbst momentan zurueckgestellt hat. Es
  * soll sofort wieder der erste Befehl in der Liste bearbeitet werden,
  * welcher jetzt wahrscheinlich ein anderer ist.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_auswerten(char *befehl, long runde)
{
  // Wenn die Einheit unversorgt ist, werden ab sofort keine Befehle mehr
  // ausgefuehrt. Ausnahme: Ueberstellen kann man sie noch

    if (attribut_gesetzt("Unversorgt")
	&& mystrncmp_no_case(L("US","AN"),befehl,2)
	&& mystrncmp_no_case(L("XX","DU"),befehl,2)) {
	befehl_dauert_noch(1); // Zieht 1 vom Phasenzaehler ab, damit dieser
			  // sich nicht anhaeuft.
   return 0;
  };

  // Wenn die Einheit in Reserve ist, dann sind die meisten Befehle
  // nicht ausfuehrbar.
  
  if (attribut_gesetzt("Reserve")
      && mystrncmp_no_case(L("MO","MO"),befehl,2)
      && mystrncmp_no_case(L("US","AN"),befehl,2)
      && mystrncmp_no_case("*P",befehl,2)
      && mystrncmp_no_case(L("SW","SS"),befehl,2)
      && mystrncmp_no_case(L("XX","DU"),befehl,2))
  {
    fehlermeldung1(L("In Reserve k~onnen wir nur die Befehle AA, MO, US, SW", "While in reserve we can only execute the commands CC, MO, AN, SS"));
    fortsetzung(L(" und XX ausf~uhren.\n"," and DU.\n"));
    return 1;   
  }
	
  // Wenn die Einheit nicht mobil ist (Buergerwehr), dann stehen nur wenig
  // Befehle zur Verfuegung.
  
  if (!bewegungspunkte_pro_feld
      && mystrncmp_no_case(L("MO","MO"),befehl,2)
      && mystrncmp_no_case(L("RV","DM"),befehl,2)
      && mystrncmp_no_case("*P",befehl,2)
      && mystrncmp_no_case(L("AS","SE"),befehl,2)
      && mystrncmp_no_case(L("WI","RP"),befehl,2)
      && mystrncmp_no_case(L("WA","WA"),befehl,2)
      && mystrncmp_no_case(L("AU","DO"),befehl,2)
      && mystrncmp_no_case(L("EI","DI"),befehl,2)
      && mystrncmp_no_case(L("XX","DU"),befehl,2)
      && mystrncmp_no_case(L("NA","CN"),befehl,2))
  {
    fehlermeldung1(L("Wir sind eine ortsgebundene Einheit und k~onnen nur die Befehle ","We are a local unit and can only execute the commands "));
    fortsetzung(L("AA, MO, RV, AS, NA, WA, WI, EI, AU und XX ausf~uhren.\n","CC, MO, DM, SE, CN, WA, RP, DO, DI and DU.\n"));
    return 1;
  }    

  if      (!mystrncmp_no_case(L("RE","MV"),befehl,2)) return befehl_reisen(befehl, runde);
  else if (!mystrncmp_no_case(L("SG","FC"),befehl,2)) return befehl_stadt_gruenden(befehl);
  else if (!mystrncmp_no_case(L("EI","DI"),befehl,2)) return befehl_eingraben();
  else if (!mystrncmp_no_case(L("AU","DO"),befehl,2)) return befehl_ausgraben();
  else if (!mystrncmp_no_case(L("RV","DM"),befehl,2)) return befehl_in_reserve();
  else if (!mystrncmp_no_case(L("MO","MO"),befehl,2)) return befehl_mobil_machen();
  else if (!mystrncmp_no_case(L("ST","CR"),befehl,2)) return befehl_strassen_bauen();
  else if (!mystrncmp_no_case(L("WR","DF"),befehl,2)) return befehl_wr_af_tr(befehl);
  else if (!mystrncmp_no_case(L("AF","AF"),befehl,2)) return befehl_wr_af_tr(befehl);
  else if (!mystrncmp_no_case(L("TR","DS"),befehl,2)) return befehl_wr_af_tr(befehl);
  else if (!mystrncmp_no_case(L("AC","CS"),befehl,2)) return befehl_wr_af_tr(befehl);
  else if (!mystrncmp_no_case(L("VW","RZ"),befehl,2)) return befehl_verwuesten();
  else if (!mystrncmp_no_case(L("US","AN"),befehl,2)) return befehl_ueberstellen(befehl);
  else if (!mystrncmp_no_case(L("AN","AT"),befehl,2)) return befehl_angreifen(befehl);
  else if (!mystrncmp_no_case(L("KP","CP"),befehl,2)) return befehl_angreifen(befehl);
  else if (!mystrncmp_no_case(L("EO","CQ"),befehl,2)) return befehl_feld_erobern(befehl, runde);
  else if (!mystrncmp_no_case(L("FE","FI"),befehl,2)) return befehl_feuern(befehl, runde);
  else if (!mystrncmp_no_case(L("BO","BO"),befehl,2)) return befehl_bombardieren(runde);
  else if (!mystrncmp_no_case(L("EN","OC"),befehl,2)) return befehl_einnehmen();
  else if (!mystrncmp_no_case(L("EM","EM"),befehl,2)) return befehl_embark(befehl, runde);
  else if (!mystrncmp_no_case(L("DE","DE"),befehl,2)) return befehl_disembark(befehl, runde);
  else if (!mystrncmp_no_case(L("BL","LO"),befehl,2)) return befehl_umladen(befehl);
  else if (!mystrncmp_no_case(L("EL","UL"),befehl,2)) return befehl_umladen(befehl);
  else if (!mystrncmp_no_case(L("*P","*P"),befehl,2)) return befehl_prioritaet_setzen(befehl);
  else if (!mystrncmp_no_case(L("SW","SS"),befehl,2)) return befehl_strategie_waehlen(befehl);
  else if (!mystrncmp_no_case(L("SC","SC"),befehl,2)) return befehl_schritt_in_richtung(befehl);
  else if (!mystrncmp_no_case(L("GZ","GT"),befehl,2)) return befehl_gehe_zu(befehl);
  else if (!mystrncmp_no_case(L("PV","SH"),befehl,2)) return befehl_pendelverkehr(befehl);
  else if (!mystrncmp_no_case(L("EX","EX"),befehl,2)) return befehl_explorieren(befehl);
  else if (!mystrncmp_no_case(L("BG","EU"),befehl,2)) return befehl_begleiten(befehl);
  else if (!mystrncmp_no_case(L("AS","SE"),befehl,2)) return befehl_ansiedeln();
  else if (!mystrncmp_no_case(L("BS","SM"),befehl,2)) return befehl_bodenschaetze_suchen();
  else if (!mystrncmp_no_case(L("NA","CN"),befehl,2)) return befehl_benennen(befehl);
  else if (!mystrncmp_no_case(L("BA","BU"),befehl,2)) return befehl_bauen(befehl);
  else if (!mystrncmp_no_case(L("WA","WA"),befehl,2)) return befehl_warten(befehl, runde);
  else if (!mystrncmp_no_case(L("WI","RP"),befehl,2)) return befehl_wiederhole(befehl, runde);
  else if (!mystrncmp_no_case(L("UN","ES"),befehl,2)) return befehl_untersuche(befehl);
  else if (!mystrncmp_no_case(L("MB","UM"),befehl,2)) return befehl_meteorit_bergen();
  else if (!mystrncmp_no_case(L("AV","UA"),befehl,2)) return befehl_abkuerzung_verwenden(befehl);

  else if (!mystrncmp_no_case(L("XX","DU"),befehl,2)) return befehl_aufloesen();

  else {
    fehlermeldung1(L("Diesen Befehl gibt es nicht.\n","This command does not exist at all!\n"));
    return 1;
  }
}


/**---------------------------------------------------------------------------
  * EINHEIT::platz_fuer_baut()
  * 
  * Ermittelt, ob an der aktuellen Adresse der Einheit eine Stadt
  * oder Weltbaut gebaut werden kann. Dies geht immer dann, wenn
  * dort nicht schon so ein Ding steht. Dass man eine Stadt auf
  * Felder, die bewirtschaftet sind, nicht bauen kann, wird nicht
  * nicht abgefragt und muss beim SG-Befehl bedacht werden.
  *
  * @return
  * short 0, wenn kein Platz ist, oder der Ort nicht die Welt ist.
  * short 1, sonst.
  ---------------------------------------------------------------------------*/
short EINHEIT::platz_fuer_baut()
{
  if (!ort_ist_welt()) return 0;
  DOUBLIST *objliste = welt()->alle_objekte_bei_adresse(adresse);
  while (!objliste->is_empty()) {
    OBJEKT_LIST_NODE *objnode = (OBJEKT_LIST_NODE *)objliste->first();
    if (!mystrcmp(objnode->objekt->attribut("TYP"),"STADT") ||
	!mystrcmp(objnode->objekt->attribut("TYP"),"WELTBAUT"))
    {
      delete objliste;
      return 0; // Kein Platz mehr
    }

    delete objnode;
  }
  delete objliste;
  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::zielfeld_bestimmen()
  * 
  * Bestimmt aus der aktuellen Position des Objektes und einer
  * Ortsangabe eine Zieladresse.
  *
  * @param
  * ortsangabe:          Richtung als String, z.B. SSW oder 3N,2W
  * oder absolute (vom Spieler aus) Koordinaten: 13,-7
  *
  * @return
  * Adresse des Zielfeldes oder ADR::ausserhalb(), falls die
  * Richtung ungueltig angegeben ist oder sich die Zieladresse
  * ausserhalb der Landschaft befinden wuerde.
  ---------------------------------------------------------------------------*/
ADR &EINHEIT::zielfeld_bestimmen(char *ortsangabe)
{
    // Das geht natuerlich nur auf der Welt
    if (!ort_ist_welt()) return ADR::ausserhalb();

    if (mystrlen(ortsangabe) == 0) return adresse;

    // Mein Kriterium zur Unterscheidung von relativen und absoluten Angaben:
    // Wenn nur Ziffern, Komma und Minus vorkommen, dann ist es absolut.
    // Die folgende Schleife bricht entweder ab, wenn das Stringende erreicht
    // ist, oder 
    
  char *z = ortsangabe;
  while (*z && (isdigit(*z) || *z=='-' || *z==',')) z++;
  if (!*z) return absolute_adresse(ortsangabe);
  else 
  {
    // Ansonsten muss ich aus dem Richtungsstring die Richtung bestimmen.

    RIC richtung(ortsangabe);

    // Anhand der Richtung kann ich nun die Zieladresse berechnen, was eine
    // Funktion der Landschaft uebernimmt.

    return landschaft()->adresse_in_richtung(adresse, richtung);
  }
}


/**---------------------------------------------------------------------------
  * EINHEIT::pruefe_bewegung_in_richtung()
  * 
  * Wird z.B. von befehl_reisen() aufgerufen. Die Funktion testet an-
  * hand von Daten der REALEN Welt und von Attributen der Einheit, ob
  * eine Bewegung in bestimmtes Feld (in einem Zug) moeglich ist. Da-
  * bei gehen ein: Gelaendeformen, Gegner, die im Weg stehen, Lage
  * des Zielfeldes.
  *
  * @param
  * ADR& ziel: Zu untersuchende Zielfeld.
  * short modus: 0: alles normal.
  * 1: Einheiten und Entfernung sind kein Hinderniss. 
  *
  * @return
  * char *string (static), der auf eine Begruendung zeigt, wenn eine
  * Bewegung nicht moeglich ist. Das erste Zeichen ist ein Kennbuchstabe,
  * mit dem man den Grund schnell abfragen kann. Ist eine Bewegung moeg-
  * lich, so bekommt man NULL zurueck.
  *
  * Kennbuchstaben:
  * A ... Lage des Zielfeldes
  * B ... Gelaendeform & Bewegungsmoglichkeit der
  * Einheit (z.B. Boden <-> See)
  * C ... Hindernis als Gelaendeform
  * D ... Gegner steht im Weg.
  * E ... Ausserhalb der Reichweite
  ---------------------------------------------------------------------------*/
char *EINHEIT::pruefe_bewegung_zu_feld(ADR& ziel, short modus)
{
  ADR start = adresse;
  OBJEKT *ortobj = ort();

  short bin_luft = attribut_gesetzt("Luft");

  // Wenn die Einheit irgendwo drinsteckt, dann gehe ich davon aus, dass
  // sie auf dem gleichen Feld ausserhalb steht.

  if (!ort_ist_welt()) // Einheit ist irgendwodrin
  {
    ortobj = ort()->ort();
    start = ort()->adresse;
  }
  WELT *welt = (WELT *)ortobj;

  if (welt->adresse_ist_ausserhalb(ziel))
    return L("AEine Bewegung ~uber die Pole hinaus ist nicht m~oglich.\n","AYou cannot cross the poles.\n");

  // Moeglicherweise ist dieses Feld nicht zu dem Ausgangsfeld benachbart.
  // Dies kann z.B. dann der Fall sein, wenn ein Landschaftstyp gewaehlt
  // wird, bei dem ueber eine Ecke angrenzende Felder nicht benachbart sind.

  if (!modus && !welt->benachbart(start, ziel))  // Bei nicht diagonaler Landschaft erforderl.
    return L("AEine Bewegung ist nur in benachbarte Felder m~oglich.\n","AMovement is only possible to adjacent squares.\n");

  // Jetzt ueberpruefe ich die Gelaendeattribute des Zielfeldes und schaue,
  // ob sich meine Einheit dorthinbewegen kann. Falls es sich um eine Luft-
  // einheit handelt ist dies allderdings nicht noetig, da sich ihr keine
  // Hindernisse in den Weg stellen koennen. Fuer die Bewegungsfaehigkeiten
  // von Einheiten sind die drei Attribute Luft, Boden und See definiert,
  // wobei im Prinzip auch Boden und See kombiniert werden koennen. Es muss
  // aber auf jeden Fall eines gesetzt sein. Es gibt also insgesamt vier 
  // Kombinationen: Boden, See, Boden&See, Luft.

  if (!bin_luft) // Einheit ist keine Lufteinheit
  {
    // Die "ART" eines Feldes bestimmt, welche Fortbewegungsart dort
    // vorherrscht.

    char *art = welt->gelaendeform_attribut(ziel,"ART");

    if (!mystrcmp(art, "See") && !attribut_gesetzt("See"))
      return L("BWir k~onnen uns nicht durch Meere oder Seen bewegen!\n","BWe cannot move across lakes or sea!\n");

    else if (!mystrcmp(art, "Boden") && !attribut_gesetzt("Boden")) {

      // normalerweise waere eine Bewegung jetzt nicht moeglich. Eine
      // Ausnahmen gibt es jedoch, wenn sich auf dem Feld ein Objekt
      // befindet, auf dass der Einfluss 'HA' (Hafen) wirkt. Dieser
      // Einfluss wird z.B. von einem Hafen in einer Stadt oder von
      // einer bestimmten Weltbaut ausgeloest. Eine Bewegung in diesem
      // Fall ist aber nur moeglich, wenn sich das Schiff auf Wasser
      // befindet. Eine Bewegung von Land zu Land ist nicht erlaubt.
      // Die Land-zu-Land-Bedingung teste ich zuerst, damit ich in
      // diesem Fall gar nicht erst nach einem Hafen suchen muss.
      
      if (mystrcmp(gelaende_attribut("ART"),"See"))
	return L("BEine direkte Bewegung von Hafen zu Hafen ist nicht m~oglich.\n","BA direct movement from port to port is not possible. \n");

      short hafen = 0;
      FOR_EACH_OBJEKT_IN (welt->alle_objekte_bei_adresse(ziel))
      DO_AND_DELETE (hafen |= (objekt->einfluss_vorhanden("HA")))

      if (!hafen)
	return L("BEine Bewegung auf Land ist uns ohne Hafen nicht m~oglich.\n","BMovement into the continent is impossible with a port.\n");

    }

    else if (!mystrcmp(art, "Hindernis")) // Hindernis fuer jedermann!
      return L("CDer Weiterreise ist durch unwegsamen Gel~ande versperrt!\n","CMovement is blocked by impassable terrain!\n");

    if (!modus) { 
      // Jetzt kann sich die Einheit eigentlich fortbewegen. Einzig ist noch die
      // Frage offen, was geschieht, wenn eine Einheit einer feindlichen
      // Nation im Weg ist. Flugzeuge koennen dabei nicht im Weg sein.

      short gegner = 0;
      FOR_EACH_OBJEKT_IN (welt->alle_gegner_bei_adresse(staat(), ziel));
      DO_AND_DELETE ( gegner |= (!uns_liebt(objekt) && !objekt->attribut_gesetzt("Luft")); )
      if (gegner) return L("DDer Weg wird durch eine gegnerische Einheit versperrt!\n","DPath is blocked by an enemical unit!\n");
    }

  } // Galt nicht fuer Lufteinheiten
  
  else if (attribut_gesetzt("REICHWEITE"))
  {
    if (!basis_im_umkreis(ziel, myatof(attribut("REICHWEITE"))))
    return L("EWir k~onnen nicht aus der Reichweite unserer Basen fliegen!\n","EWe cannot fly out of our home base range!\n");
  }
  
  return NULL; // Bewegung freigegeben.
}


/**---------------------------------------------------------------------------
  * EINHEIT::alle_unterstuetzenden_einheiten()
  * 
  * Liefert eine Liste alle Einheiten, die:
  * 1. Unserem Staat den Staatus 'freundlich' zugeordent haben.
  * 2. Zum Zielfeld(!) benachbart sind
  * 3. Die weder auf dem Zielfeld noch auf dem Feld dieser Einheit stehen
  *
  * @param
  * ADR& ziel:  Zielfeld
  * @return
  * DOUBLIST *: Dynamische Objektliste mit den gesuchten Ein-
  * heiten.
  ---------------------------------------------------------------------------*/
DOUBLIST *EINHEIT::alle_unterstuetzenden_einheiten(ADR& ziel)
{
  if (!ort_ist_welt()) return new DOUBLIST;

  // Zuerst erstelle ich eine Liste aller Objekte, die zu meinem Feld
  // benachbart sind.

  DOUBLIST *objliste = welt()->alle_objekte_im_umkreis_von(ziel,
				1.5, "TYP=EINHEIT");

  // Im naechsten Schritt entferne ich alle Einheiten, die nicht zum
  // gleichen Staat gehoeren.

  OBJEKT_LIST_NODE *objnode;
  SCAN(objliste, objnode)
  {
    OBJEKT *einheit = objnode->objekt;
    if (!uns_liebt(einheit)
        || einheit->adresse == adresse
        || einheit->adresse == ziel)
    {
      delete objnode;
      FIRST(objliste, objnode);
    }
    else NEXT(objnode);
  }


  // So. Die uebrigen Einheiten kommen als Unterstuetzer in Frage.
  // Mit welcher Kraft sie unterstuetzen koennen haengt jedoch von
  // spezielleren Dingen ab, die hier nicht behandelt werden, um die
  // Allgemeinheit der Funktion nicht weiter einzuschraenken.
  
  // Nun sortiere ich die Liste noch nach Koordinaten!
  objliste->sort(EINHEIT::sort_objliste_koord);

  return objliste;
}


/**---------------------------------------------------------------------------
  * EINHEIT::sort_objliste_koord()
  * 
  ---------------------------------------------------------------------------*/
short EINHEIT::sort_objliste_koord(DOUBLIST_NODE *n1,
 DOUBLIST_NODE *n2, void *)
{
  OBJEKT *e1 = ((OBJEKT_LIST_NODE *)n1)->objekt;
  OBJEKT *e2 = ((OBJEKT_LIST_NODE *)n2)->objekt;

  if (e1->adresse.x > e2->adresse.x) return 1;
  else if (e1->adresse.x < e2->adresse.x) return 0;
  else if (e1->adresse.y > e2->adresse.y) return 1;
  else return 0;
}
  

/**---------------------------------------------------------------------------
  * EINHEIT::effektive_angriffskraft()
  * 
  * Entlastet die Funktion befehl_angreifen(). Berechnet die aus
  * Umstaenden und Unterstuetzung resultierende Kraft eines Angriffs.
  * Dabei geht die Funktion davon aus, dass sie nur aufgerufen wird,
  * wenn die Einheit mobil, unbefestigt und versorgt ist.
  *
  * @param
  * ADR& ziel:  Ziel des Angriffes. Das Ziel kann auch das eigene
  * Feld sein (beim Einmischen in Buergerkrieg.
  *
  * @return
  * long: Effektive Angriffskraft
  ---------------------------------------------------------------------------*/
long EINHEIT::effektive_angriffskraft(ADR& ziel)
{
  // Zuerst benoetige ich eine Liste aller Einheiten, die mich beim
  // Angriff unterstuetzen koennten. Diese liefert mir eine weitere
  // Hilfsfunktion. Dann addiere ich deren Unterstuetzungen.

  long unterstuetzung = 0, letzte_u = 0;
  ADR letzte_adresse(-1,-1);

  // Die Liste muss unbedingt nach Koordinaten irgendwie sortiert sein!

  FOR_EACH_OBJEKT_IN (alle_unterstuetzenden_einheiten(ziel)) DO_AND_DELETE
  (
    long u = myatol(objekt->info("ANGRIFFS_UNTERSTUETZUNG"));
    if (letzte_adresse == objekt->adresse) {
      if (u > letzte_u) {
	unterstuetzung -= letzte_u;
	unterstuetzung += u;
	letzte_u = u;
      }
    }
    else {
      letzte_adresse = objekt->adresse;
      letzte_u = u;
      unterstuetzung += u;
    }
  )
  

  // Jetzt kommt noch meine eigene Kraft drauf und dann alles durch 2,
  // abgerundet.

  return (unterstuetzung + 2*angriffskraft)/2;
}


/**---------------------------------------------------------------------------
  * EINHEIT::effektive_verteidigungskraft()
  * 
  * Berechnet die Effektivkraft, mit der sich die Einheit momentan
  * verteidigen kann. Dabei kommen Effekte wie Befestigung, Feldbonus,
  * Einfluss SM, Unterstuetzung etc. zum tragen. Ist als Quelle des
  * Angriffes ADR::ungueltig() angegeben, so wird eine Unterstuet-
  * zug durch andere Einheiten nicht einberechnet.
  *
  * @param
  * ADR& quelle:  Von diesem Feld aus findet der Angriff statt.
  *
  * @return
  * long: Effektive Verteidigungskraft
  ---------------------------------------------------------------------------*/
long EINHEIT::effektive_verteidigungskraft(ADR& quelle)
{
  // Als erstes berechne ich die Unterstuetzung durch benachbarte Einheiten.
  // Diese Unterstuetzung kommt auf jeden Fall immer zum tragen, auch wenn
  // die Einheit Unversorgt o.ae. ist. Lufteinheiten koennen beim Vertei-
  // digen nicht unterstuetzt werden.

  long unterstuetzung = 0, letzte_u = 0;
  ADR letzte_adresse(-1,-1);

  // Die Liste muss unbedingt nach Koordinaten irgendwie sortiert sein!

  if (!attribut_gesetzt("Luft") && !quelle.ist_ausserhalb()) {
    FOR_EACH_OBJEKT_IN (alle_unterstuetzenden_einheiten(adresse)) DO_AND_DELETE
    (
      long u = myatol(objekt->info("VERT_UNTERSTUETZUNG"));
      if (letzte_adresse == objekt->adresse) {
	if (u > letzte_u) {
	  unterstuetzung -= letzte_u;
	  unterstuetzung += u;
	  letzte_u = u;
	}
      }
      else {
	letzte_adresse = objekt->adresse;
	letzte_u = u;
	unterstuetzung += u;
      }
    )
  } // Nicht Luft und nicht ausserhalb
  
  // Die Unterstuetzung wird zum Basiswert addiert. Der Basiswert berechnet
  // sich ueber ein Info.

  long ergebnis = unterstuetzung + myatol(info_verteidigungskraft());

  return ergebnis;
}


/**---------------------------------------------------------------------------
 // FUNKTION:            EINHEIT::entfernungsfunktion()
  * 
  * Funktion, die ueber einen Zeiger von der Wegsuche aufgerufen wird.
  * Sie teilt dem Wegsuchealgoritmus mit, wieviele Entfernungseinheiten
  * die Bewegung von einer Adresse zu einer anderen braucht. Die Ein-
  * heit wird diese Antworten, aus Gruenden der Fairness, nicht auf-
  * grund der objektiven Daten der Welt beantworten, sondern mit
  * den Daten aus dem Landschaftsabbild!
  *
  * Fremde Einheiten, die eventuell den Weg versperren koennten, 
  * werden UEBERHAUPTNICHT in die berechnung mit einbezogen. Der
  * Reise-Befehl, den GZ oder SC veranlasst wartet dann einfach.
  * Dies ist besser als eine Fehlermeldung.
  *
  * @param
  * void *data: Zeiger auf die Einheit, deren Weg berechnet wird.
  * ADR& start, ziel: Untersuchter Schritt. Die Felder sind benachbart.
  *
  * @return
  * short -1, wenn eine Bewegung von start nach ziel gar nicht moeglich
  * ist.
  * short zahl>0, wenn eine Bewegung moeglich ist. Der Wert gibt dem
  * Algorithmus die Reisezeit an. Er darf nie 0 sein. Er muss nicht
  * unbedingt mit der Rundenzahl uebereinstimmen. Mit ihm kann in
  * gewisser Weise ein Weg vor einem anderen bevorzugt werden.
  ---------------------------------------------------------------------------*/
short EINHEIT::entfernungsfunktion(void *data, ADR& start, ADR& ziel)
{  // static Funktion!

  EINHEIT *einheit = (EINHEIT *)data;

  // Nun muss ich pruefen, ob die Einheit das Zielfeld betreten kann.

  if (!einheit->ort_ist_welt()) return -1;

  // Jetzt muss ich mir von meinem Landschaftsabbild die Daten des
  // Zieles holen.

  LANDSCHAFTSABBILD *abbild =
			einheit->welt()->landschaftsabbild(einheit->staat());
  if (!abbild) return -1; // Huch?
  unsigned short zielfeld = abbild->feld(ziel);
  short form = zielfeld & 0xff;

  // Einige Vorberechnungen...
  short be_boden = einheit -> attribut_gesetzt("Boden");
  short be_luft  = einheit -> attribut_gesetzt("Luft");
  short be_see  = einheit -> attribut_gesetzt("See");
  
  // Jetzt muss ich ermitteln, ob die Einheit da ueberhaupt hin kann.
  if (!be_luft) // Einheit ist keine Lufteinheit
  {
    // Wenn die Form gleich 0 ist, dann heisst das, dass das Feld noch
    // gar nicht erforscht ist, und als Boden- oder Seeeinheit muss ich
    // davon ausgehen, dass ich dort nicht hin kann.
    if (!form) return -1;
      
    // Jetzt brauche ich alle Gegner auf dem Zielfeld. Wenn naemlich
    // ein befestigter Gegner im Weg steht, dann such ich mir lieber
    // einen anderen Weg. Uebrigens sind Flugzeuge niemals eingegraben,
    // weswegen ich mir eine Abfrage sparen kann.
    
    // Wenn ich die Strategie S_ZIELSTREBIG habe, dann schere ich
    // mich ueberhaupt nicht um gegnerische Einheiten.

    // Sollte ich uebrigens gerade ein Feld untersuchen, dass noch au-
    // sserhalb der Sichtweite der Einheit liegt, dann sehe ich nicht
    // nach eingegrabenen Gegnern, die den Weg versperren koennten, son-
    // dern gehe einfach davon aus, dass ich schon irgendwie weiterkomme.

    bool zielstrebig = (einheit->aktuelle_strategie == S_ZIELSTREBIG);

    if (!zielstrebig &&
	einheit->welt()->entfernung_zwischen(einheit->adresse, ziel) <= einheit->sichtweite)
    {
      short gegner = 0;
      FOR_EACH_OBJEKT_IN (
        einheit->welt()->alle_gegner_bei_adresse(einheit->staat(), ziel));
      DO_AND_DELETE (
         gegner |= (!einheit->uns_liebt(objekt)
		  && objekt->attribut_gesetzt("Eingegraben"));
      )
   
      if (gegner) return -1; // Gegner im Weg.
    }


    // Die "ART" eines Feldes bestimmt, welche Fortbewegungsart dort
    // vorherrscht.

    char *art = einheit->welt()->gelaendeform(form)->attribute.abfragen("ART");
    if (!art) return -1; // Huch. Die Form 0 habe ich doch ausgeschlossen.

    if ((!mystrcmp(art, "See") && !be_see)
    ||  (!mystrcmp(art, "Hindernis"))) return -1;

    // Wenn es sich um Boden handelt und die Einheit ist ein Schiff, dann
    // muss ich schauen, ob auf dem Feld nicht vielleicht ein Hafen ist.
    // Schiffe koennen allerdings nicht von Hafen zu Hafen direkt fahren.

    if (!mystrcmp(art,"Boden"))
    {
      if (be_see) { // Es ist also ein Schiff
	short hafen = 0;
	FOR_EACH_OBJEKT_IN (einheit->welt()->alle_objekte_bei_adresse(ziel))
	DO_AND_DELETE (hafen |= (objekt->einfluss_vorhanden("HA")))
	if (!hafen) return -1;

        unsigned short startfeld = abbild->feld(start);
        short startform = startfeld & 0xff;
        char *startart = einheit->welt()->
        		gelaendeform(startform)->attribute.abfragen("ART");
        
	if (mystrcmp(startart,"See")) return -1; //Keine Fahrt Hafen->Hafen.
      }
      else if (!be_boden) return -1;
    }
    
  } // Galt nicht fuer Lufteinheiten

  // Als naechstes kommt noch ein Abfrage der Reichweite, wenn es sich
  // um Flugzeuge handelt. 
  
  if (einheit->attribut_gesetzt("REICHWEITE"))
  {
    if (!einheit->basis_im_umkreis(ziel, myatof(einheit->attribut("REICHWEITE"))))
    return -1; // Ausserhalb der Reichweite einer Basis.
  }

  // Ich kann mich also bewegen und muss nun noch feststellen, wie schnell...

  bool mech = einheit->attribut_gesetzt("Mechanoid");
  long dauer;
  if (!be_luft && !mech) {
    char *bewegung = einheit->welt()->gelaendeform(form)->attribute.abfragen("BEWEGUNG");
    long faktor = myatol(bewegung);
    if (!faktor) faktor = 1;
    dauer = einheit->bewegungspunkte_pro_feld * faktor;

    // Wenn auf dem Zielfeld eine Strasse ist, dann halbiert sich die Dauer.
    // (abgerunden, jedoch mindestens 2)

    if (zielfeld & (0x8000 >> FELD_ATTRIBUT_STRASSE)) dauer = (dauer+1) / 2;
    if (dauer < 2) dauer = 2;
  }
  else dauer = einheit->bewegungspunkte_pro_feld; // Fuer Flugzeuge und Mechs

  return short(dauer);
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_schritt_in_richtung()
  * 
  * Dies ist der unterste Befehl, der auf den Wegsuche-Algorithmus
  * zurueckgreift. Gibt man ihm ein Zielfeld als relative Adresse
  * an, so erzeugt er einen RE-Befehl ein Feld in naeher zu dem
  * gewuenschten Zielfeld auf dem schnellsten Weg.
  *
  * @param
  * char *befehl: SC mit relativer Adressangabe (RIC::to_string()),
  * wenn der Befehl sich in einen RE-Befehl umwandeln soll.
  * GZ mit relativer Adressangabe, wenn der Befehl einen
  * RE-Befehl vorschieben soll, selbst aber weiterbestehen soll. Dies
  * ist dann von noeten, wenn der Befehl von einem anderen Befehl di-
  * rekt aufgerufen wird (GZ).
  *
  * @return
  * 1, bei Fehler, -1 sonst.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_schritt_in_richtung(char *befehl)
{
  ADR ziel = zielfeld_bestimmen(befehl+2);
  if (ziel == ADR::ausserhalb()) {
    befehl = attribut("GEHE_ZU");
    fehlermeldung1(L("Ung~ultige Richtungsangabe!","Invalid direction!!"));
    return 1;
  }

  // Was ist, wenn ich schon da bin? Dann mache ich gar nichts!
  if (ziel == adresse) return 1; // Befehl fertig.

  // Nun lasse ich mir vom Wegsuchealgorithmus den besten Weg berechnen.
  DOUBLIST *besterweg = welt()->schnellster_weg(adresse, ziel,
    EINHEIT::entfernungsfunktion, (void *)this);

  if (!besterweg || besterweg->is_empty()) {
    // Eine Fehlermeldung gebe ich nur aus, wenn ich vom GZ-Befehl aufgeru-
    // fen wurde.

    if (!mystrncmp(befehl,L("GZ","GT"),2)) {
       befehl = attribut("GEHE_ZU");
       fehlermeldung1(L("Dieses Ziel ist f~ur uns nicht erreichbar!\n","We cannot reach that destination!\n"));
    }
       
    return 1;
  }

  // Interessant ist lediglich das erste Feld.
  ADR erstes_feld = ((ADR_LIST_NODE *)besterweg->first())->adresse;
  delete besterweg;

  // Und schiebe ich einen RE-Befehl vor und beende mich selbst.
  char string[80];
  RIC ric = welt()->richtung_von_nach(adresse, erstes_feld);
  sprintf(string, L("RE%s","MV%s"),ric.to_string());

  // Die Rueckgabe haengt davon ab, ob ich von dem GZ-Befehl direkt
  // aufgerufen wurde. Wenn ja, dann gebe ich -1 zurueck und
  // schiebe den Befehl vor. Wenn nein, dann wandle ich den Befehl
  // um und gebe -1 zurueck.

  if (!mystrncmp(befehl, L("GZ","GT"),2)) {
    befehl_vorschieben(string);
    return -1;
  }
  else {
    befehl_umwandeln(string);
    return -1;
  }
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_gehe_zu()
  * 
  * Komplexer Befehl, der solange SC-Befehle ausfuehrt, bis die Einheit
  * am gewuenschten Punkt ist.
  *
  * @param
  * char *befehl: GZ%s Mit Zielangabe als subjektive Adresse. Es ist
  * durchaus auch erlaubt, den Namen eines Objektes als Ziel anzugeben,
  * solange dieses Objekt zum gleichen Staat gehoert.
  *
  * @return
  * Direkt vom SC-Befehl, wird aber von objekt.C fuer den GZ-Befehl
  * interpretiert.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_gehe_zu(char *befehl)
{
  if (!ort_ist_welt()) { // Sollte schon sein.
    fehlermeldung1(L("Das geht nicht, solange wir uns im Konvoi befinden.","That´s impossible while in convoy."));
    attribut_setzen("Pendelfehler");
    return 1;
  }

  // Bevor ich weitermache, merke ich mir den Befehl in einem Attribut,
  // damit ich spaeter eine gute Fehlermeldung ausgeben kann.
  
  attribut_setzen("GEHE_ZU",befehl);

  // Die Zielangabe ist als Adresse relativ zur Hauptstadt gegeben oder als
  // Objektname angegeben. Moeglicherweise wurden statt Nullen Os eingetippt.
  // Deshalb kann es sein, dass eine Koordinatenangabe folgt, welche nicht
  // mit einer Ziffer beginnt. Hinter dem O muss aber dann wenigstens ein
  // Komma folgen.

  ADR ziel;
  if (isdigit(befehl[2]) || befehl[2]=='-' || 
      (tolower(befehl[2])=='o' && befehl[3]==',')) {
    ziel = absolute_adresse(befehl+2);
    if (ziel == ADR::ausserhalb()) {
      fehlermeldung1(L("Ung~ultige Zielangabe.","Invalid destination."));
      attribut_setzen("Pendelfehler");
      return 1;
    }
  }

  else {
    OBJEKT *zielobjekt = objekt_mit_namen(befehl+2);
    if (!zielobjekt) {
      fehlermeldung1(L("Ung~ultige Zielangabe!","Invalid destination!"));
      attribut_setzen("Pendelfehler");
      return 1;
    }
    else if (!uns_liebt(zielobjekt)) {
      fehlermeldung1(L("Es ist nur m~oglich, eigene Objekte oder Objekte von","You can only state own objects or allied objects "));
      fortsetzung(L(" freundlich gesinnten Staaten als Ziel anzugeben."," as destinations."));
      attribut_setzen("Pendelfehler");
      return 1;
    }
    else if (zielobjekt->ort() != welt()) {
      fehlermeldung1(L("Das Zielobjekt mu~s sich auf der Welt befinden.","Target object must be somewhere on this world!"));
      attribut_setzen("Pendelfehler");
      return 1;
    }
    else ziel = zielobjekt->adresse;
  }



  // Sollte ich bereits am Ziel sein, dann bin ich fertig.
  if (ziel == adresse) return 1;

  // Andernfalls rufe ich direkt die Routine des SC-Befehls auf!
  char string[80];
  RIC ric = welt()->richtung_von_nach(adresse, ziel);
  sprintf(string, L("GZ%s","GT%s"), ric.to_string());

  // Der Rueckgabewert der Funktion passt mit dem meinen wunderbar zusammen.
  // Im Fall eines Fehlers wird dieser ausgegeben  (mit L("GZ!","GT!")) und mit
  // der Rueckgabe 1 der Befehl als beendet geklaert. Klappt es dagegen
  // soweit, so wird mit -1 der Befehl nur vorubergehend suspendiert.

  short rwert = befehl_schritt_in_richtung(string);
  if (rwert == 1) attribut_setzen("Pendelfehler");
  return rwert;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_pendelverkehr()
  * 
  * Richtet einen Pendelverkehr zwischen zwei Staedten ein.
  * Der Spieler nennt zwei seiner Staedte und zwei Resourceangaben.
  * Darauf hin pendelt die Einheit staendig zwischen den Staedten
  * hin und her und transportiert Resourcen, bis der Befehl AA
  * erteilt wird. Stadt einer Stadt kann auch eine Weltbaut oder eine
  * andere Einheit genannt werden, solange diese andere Einheit sich
  * nicht vom Fleck bewegt.
  *
  * @param
  * char *befehl: Das Befehlsformat ist: PVname1,resource1,name2,resource2
  * was bedeutet, dass von Stadt name1 resource1 eingeladen
  * wird und zu name2 geschafft wird. Dort wird dann res2
  * eingeladen und wieder zu ersten Stadt geschafft.
  * @return
  * Wie ueblich.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_pendelverkehr(char *befehl)
{
  // Wenn innerhalb eines PV-Befehls einer der beiden GZ-Befehle einen
  // Fehler erzeugt, dann muss der ganze PV-Befehl abgebrochen werden.
  // Deshalb setzt der GZ-Befehl das Attribut Pendelfehler, wenn ein
  // Fehler auftritt.
  
  // PV setzt zu Beginn das Attribut Pendel_aktiv, damit es weiss, ob es
  // schon einmal aktiv geworden ist. Sind beide Attribute zu Eintritt
  // in diese Funktion gesetzt, so muss der PV-Befehl abgebrochen werden.
  
  if (attribut_gesetzt("Pendel_aktiv") && attribut_gesetzt("Pendelfehler"))
  {
    attribut_loeschen("Pendel_aktiv");
    tagebuch_fuehren(befehl, ", ");
    tagebuch_fuehren(L(": abgebrochen",": canceled"),"");
    return 1;
  }
  
  attribut_setzen("Pendel_aktiv");
  attribut_loeschen("Pendelfehler");

  // Das umfangreichste ist die Auswertung der Parameter.
  char *parameter = mystrdup(befehl+2);
  for (char *scan = parameter; *scan; scan++) if (!isalnum(*scan)) *scan = ' ';

  // Hoffentlich liest niemand die folgende Zeile :-)
  char name1[512], name2[512], res1[512], res2[512];

  sscanf(parameter, "%s%s%s%s",name1, res1, name2, res2);
  myfree(parameter);

  // Nun muss ich schauen, ob es die Staedte gibt, und ob sie dem Spieler
  // gehoeren.

  RESOURCE_VEKTOR resource1(res1), resource2(res2);

  // Ein Transport ist nur moeglich, wenn die Einheit auch in der Lage ist,
  // Gueter zu transpoertieren.

  if (!myatol(attribut("KAPAZ")))
  {
    fehlermeldung1(L("Wir sind nicht in der Lage, Ressourcen transportieren!\n","We are not capable of transporting commodities!\n"));
    return 1;
  }

  // So. Nun schiebe ich eine Reihe von Befehlen vor. Anfangen tut es mit
  // dem Gehezu-Befehl fuer die erste Stadt. Enden tut es mit dem VE-
  // Befehl aus der 2. Stadt. Ich muss in der umgekehrten Reihenfolge
  // einpuffern.

  char bef[80];
  sprintf(bef, L("BL%s","LO%s"),resource2.to_string());
  befehl_vorschieben(bef);
  befehl_vorschieben(L("EL","UL"));
  sprintf(bef, L("GZ%s","GT%s"), name2);
  befehl_vorschieben(bef);

  sprintf(bef, L("BL%s","LO%s"),resource1.to_string());
  befehl_vorschieben(bef);
  befehl_vorschieben(L("EL","UL"));
  sprintf(bef, L("GZ%s","GT%s"), name1);
  befehl_vorschieben(bef);
  return -1; // Befehl noch nicht loeschen sondern zurueckstellen.
}



/**---------------------------------------------------------------------------
  * EINHEIT::befehl_explorieren()
  * 
  * Veranlasst die Einheit, bis auf weiteres in eine ungefaehre Richtung
  * zu rennen. Wenn es ueberhaupt nicht weitergeht, auch nicht unge-
  * faehr in die Zielrichtung, dann wird der Befehl beendet. Zur Aus-
  * fuehrung der tatsaechlichen Bewegung werden RE-Befehle vorgeschoben.
  *
  * @param
  * char *befehl: EX%s Mit Angabe einer Himmelsrichtung.
  *
  * @return
  * short 1, wenn abgebrochen wurde, sonst short -1.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_explorieren(char *befehl)
{
  if (ort_ist_einheit()) {
    fehlermeldung1(L("Da wir uns im Transport einer Einheit befinden, k~onnen","While being transported by another unit we cannot "));
    fortsetzung(L(" wir die Gegend nicht erforschen.\n"," explore the landscape.\n"));
    return 1;
  }

  // Als Parameter kommt eine Richtung, die als ungefaehre Erforschungsrichtung
  // anzusehen ist.

  if (!*(befehl+2)) {
    fehlermeldung1(L("Beim Explorationsbefehl mu~st du noch eine Richtung","The explore command requires a direction:"));
    fortsetzung(L(" angeben, z.B. EXS oder EXSO.\n"," e.g. EXS or EXSE.\n"));
    return 1;
  }

  RIC richtung(befehl+2);
  if (richtung.x == 0 && richtung.y == 0)
  {
    fehlermeldung1(L("Beim Explorationsbefehl mu~st du noch eine Richtung"," The explore command requires a direction:"));
    fortsetzung(L(" angeben, z.B. EXO oder EXNW.\n"," e.g  EXE or  EXNW.\n"));
    return 1;
  }
      
  // Jetzt versuche ich, in diese Richtung zu gehen. Aber: Moeglichweise
  // geht das nicht. In so einem Fall drehe ich mich um 45 grad nach rechts
  // dann um 90 nach links, dann um 45 nach links und schliesslich um 180
  // nach rechts. Wennn dann immer noch nichts geht, dann breche ich ab.

  // ACHTUNG: Da mir nichts besseres einfaellt um zu verhindern, das die
  // Einheit immer die gleichen Felder exploriert, lasse ich die aller-
  // letze Moeglichkeit (180 Grad zurueckdrehen, d.h. 90 Grad nach rechts)
  // nicht zu.

  ADR ziel;
  int versuch=0;
  for (versuch=0; versuch<4; versuch++) {
    ziel = welt()->adresse_in_richtung(adresse, richtung);
    char *grund = pruefe_bewegung_zu_feld(ziel);
    if (!grund) break; // Geht also.

    switch (versuch) {
     case 0: richtung.drehen(-45.0); break;
     case 1: richtung.drehen(90.0); break;
     case 2: richtung.drehen(45.0); break;
     // case 3: richtung.drehen(-180.0); break;
    }
  }

  if (versuch == 4) { // Hat also nicht geklappt.
    fehlermeldung1(L("Wir k~onnen nicht weiter explorieren","We cannot explore any farther "));
    fortsetzung(L(", da alle Wege versperrt sind.\n","because all paths are blocked.\n"));
    return 1; // Befehl fertig.
  }

  // Jetzt schiebe ich einen Reisenbefehl vor und hoffe, dass alles gutgeht.
  char befstring[60];
  sprintf(befstring,L("RE%s","MV%s"),richtung.to_string());
  befehl_vorschieben(befstring);
  return -1;

}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_begleiten()
  * 
  * Veranlasst eine Einheit, bis auf weiteres einer anderen (eigenen)
  * auf Schritt und Tritt zu folgen. Dabei wird die Begleitereinheit
  * aber nicht in den Konvoi von anderen Einheiten hineinfolgen.
  * Sobald eine weitere Vorfolgung unmoeglich ist, wird der Befehl
  * abgebrochen, nicht jedoch, wenn die zu begleitende Einheit er-
  * reicht ist.
  * @param
  * char *befehl: BG%s Mit Name (=Abk) der Einheit
  * @return
  * Wie ueblich.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_begleiten(char *befehl)
{
  char *objname = befehl+2;
  if (!objname[0]) {
    fehlermeldung1(L("Dem Begleit-Befehl mu~s die Abk~urzung einer Einheit folgen.\n","The escort command requires the abbreviation of a certain unit\n"));
    return 1;
  }

  OBJEKT *objekt = objekt_mit_namen(objname);
  if (!objekt || objekt->zur_vernichtung_vorgemerkt()) {
    fehlermeldung1(L("Die zu begleitende Einheit existiert nicht (mehr).\n","The escorted unit does not exist (any more).\n"));
    return 1;
  }

  if (mystrcmp(objekt->attribut("TYP"),"EINHEIT")) {
    fehlermeldung1(L("Wir k~onnen nur Einheiten begleiten.\n","We can only escort units.\n"));
    return 1;
  }

  if (!uns_liebt(objekt)) {
    fehlermeldung1(L("Wir k~onnen nur eigene Einheiten oder solche von ","We can only escort our own units or "));
    fortsetzung(L("freundlich gesinnte Staaten begleiten.\n","allied units.\n"));
    return 1;
  }

  // Wenn ich schon bei der Einheit bin, dann mache ich gar nichts.

  if (ort() == objekt->ort() && adresse == objekt->adresse) return 0;

  // Im Konvoi kann ich nicht begleiten.

  if (ort_ist_einheit()) {
    fehlermeldung1(L("Wir k~onnen keine Einheit begleiten, wenn wir uns ","We cannot escort units while being "));
    fortsetzung(L("selbst im Transport einer anderen Einheit befinden.\n","transported by another unit.\n"));
    return 1;
  }

  if (!ort_ist_welt()) return 1; // Da kann was nicht stimmen.

  // jetzt bestimme ich mein naechstes Ziel.

  // Dabei gibt es noch zwei Probleme: 1. Was mache ich, wenn ich der
  // Einheit nicht folgen kann? 2. Was mache ich, wenn die Einheit
  // nicht auf der Welt ist?

  // Wenn sich die Einheit nicht auf der Welt befindet, dann muss
  // sie eigentlich in einer anderen Einheit sein. Ich verfolge sie
  // aber nicht dorthinein, sondern warte draussen.

  ADR ziel;
  EINHEIT *einheit = (EINHEIT *)objekt;

  if (!einheit->ort_ist_welt()) {
    if (ort() != einheit->ort()->ort()) return 1; // Geht nicht.
    ziel = einheit->ort()->adresse; // Adresse der Konvoieinheit
  }

  else ziel = einheit->adresse;
  RIC ric = welt()->richtung_von_nach(adresse, ziel);

  // Gut. Frage 2 waehre geloest. Aber wie kann ich mich vor einem Bombarde-
  // ment von Fehlermeldungen schuetzen, wenn ich der Einheit nicht folgen
  // KANN (z.B. Schiff folgt Panzer)? Der SC-Befehl gibt 1 zurueck, wenn
  // ich das Ziel gar nicht erreichen kann. Ich gebe das Ergebnis des SC-
  // Befehls zurueck (wie beim GZ-Befehl!).

  char scstring[40];
  sprintf(scstring,L("GZ%s","GT%s"),ric.to_string());

  // Merke: Wenn ich den SC-Befehl mit dem Befehlstext GZ... aufrufe, dann
  // wandelt er sich nicht in einen RE-Befehl um, was sehr wichtig ist, da
  // sonst der BG-Befehl umgewandelt werden wuerde!

  if (befehl_schritt_in_richtung(scstring) == -1) return -1;

  // Wenn er 1 zurueckgibt, dann ist das Ziel nicht erreichbar.

  fehlermeldung1(L("Wir sind leider nicht in der Lage, der Einheit zu folgen.\n","Unfortunately we are not able to follow that unit.\n"));
  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_abkuerzung_verwenden()
  * 
  * Setzt fuer eine Abkuerzung einen Text ein, der beim Staat mit
  * ADA bis ADZ definiert werden kann.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_abkuerzung_verwenden(char *befehl)
{
  char kennung = befehl[2];
  if (kennung < 'A' || kennung > 'Z' || befehl[3]) {
    fehlermeldung1(L("Es sind nur die Befehle AVA bis AVZ m~oglich. Ersatzweise wird AVZ verwendet.\n","Only the commands UAA to UAZ are legal. Replacement command is UAZ.\n"));
    tagebuch_fuehren(L("AVZ","UAZ"), ", ");
    kennung = 'Z';
  }
  else tagebuch_fuehren(befehl, ", ");
  
  ((STAAT *)staat())->abkuerzung_expandieren(this, kennung);
  return 1;
}

      
/**---------------------------------------------------------------------------
  * EINHEIT::befehl_untersuche()
  * 
  * Befehle "Untersuche", mit dem eine Einheit ein benachbartes oder
  * das eigene Feld untersuchen kann.
  * @param
  * "UN%s", wobei %s "" oder eine Richtung wie "W" oder "NO" ist.
  * @return
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_untersuche(char *befehl)
{
  // Wenn ich im Konvoi bin, dann geht der Befehl ueberhaupt nicht.
  if (!ort_ist_welt()) {
    fehlermeldung1(L("Der Untersuche-Befehl klappt nicht, wenn wir uns im Konvoi"," We cannot execute the search command while being")
    L(" einer anderen Einheit befinden!\n"," transported by another unit!\n"));
    return 1;
  }

  ADR ziel = zielfeld_bestimmen(befehl+2);

  if (welt()->adresse_ist_ausserhalb(ziel)) {
    fehlermeldung1(L("Wo wir untersuchen sollen, ist die Welt zuende!\n","There is the edge of the world where we want to explore... \n"));
    return 1;
  }

  else if (!welt()->benachbart(ziel, adresse)) {
    fehlermeldung1(L("Wir k~onnen nur die neun Felder untersuchen, die in unserer N~ahe sind.\n","We can only examine the nine adjacent squares.\n"));
    return 1;
  }

  OBJEKT *zielstadt = NULL;
  DOUBLIST *staedte = welt()->alle_objekte_bei_adresse(ziel, "TYP=STADT");
  if (!staedte->is_empty()) 
      zielstadt = ((OBJEKT_LIST_NODE *)(staedte->first()))->objekt;
  delete staedte;

  if (!zielstadt) {
    fehlermeldung1(L("Dort ist keine Stadt, die wir untersuchen k~onnten.\n","There is no town which we could examine.\n"));
    return 1;
  }

  if (befehl_dauert_noch(DA_UNTERSUCHE)) return 0;

  tagebuch_fuehren(befehl,", ");
  tagebuch_fuehren("("," ");
  // char string[2048]; // Fuer die Ausgaben
  
  long info2 = 2, info3 = 3, info4 = 4;
  
  fussnote(zielstadt->info("UNTERSUCHE", &info2));
  fussnote(zielstadt->info("UNTERSUCHE", &info3));
  fussnote(zielstadt->info("UNTERSUCHE", &info4));
    
  tagebuch_fuehren(")","");
  return 1;
}
  
  
/**---------------------------------------------------------------------------
  * EINHEIT::befehl_warten()
  * 
  * Wartet auf ein Objekt oder einfach eine bestimmte Anzahl von
  * Runden, oder fuer immer (bis AA).
  *
  * Zum Programmiertechnischen ist anzumerken, dass der Phasenzaehler
  * in dieser Funktion von Hand wieder auf 0 gesetzt werden muss, da
  * dieser Befehl an zwei Stellen (WA:%ld oder WA%s) keine feste Zeit
  * dauert, sondern erst bei einem bestimmten Ereignis beendet wird.
  * Deshalb wird dort nicht die Funktion befehl_dauert_noch() verwen-
  * det, die von Phasenzaehler einen Wert abzieht.
  *
  * @param
  * char *befehl: WA%s mit Objektabkuerzung, oder
  * WA%ld mit Anzahl der Runden, oder
  * WA, oder
  * WA:%ld mit Angabe der absoluten Rundennummer oder
  * WAL  (wartet, bis ein Schiff leer ist) oder
  * WAx,y   (wartet, bis sich meine Transporteinheit
  * bei x,y befindet.)
  *
  * Os werden als Nullen interpretiert.
  *
  * long runde:   Aktulle Runde.
  *
  * @return
  * Wie ueblich.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_warten(char *befehl, long runde)
{
  char *parameter = befehl+2;
  if (!parameter[0]) return 0; // Warte bis auf weiteres.

  else if (!mystrcmp_no_case(parameter,L("L","E"))) { // Bin Schiff und Leer
    long zahl = alle_enthaltenen_objekte()->count_and_del();
    if (zahl == 0) {
      tagebuch_fuehren(befehl, ", ");
      return 1;
    }
    else return 0;

  }
  
  // Koordinatenangabe? Ich warte, bis ich dort bin (evtl. im Schiff!)
  char *scan = parameter;
  while (*scan) if (*scan++ == ',')  {
    ADR ziel = absolute_adresse(parameter);
    if (ziel == ADR::ausserhalb()) {
      fehlermeldung1(L("Ung~ultige Ortsangabe.","Invalid destination."));
      return 1;
    }
    if (adresse == ziel || ort()->adresse == ziel) {
      tagebuch_fuehren(befehl,", ");
      return 1;
    }
    else return 0;
  }

  if (*parameter == ':') {
    long zielrunde = myatol_mit_o(parameter+1);
    if (zielrunde > runde) return 0; // Noch warten
    tagebuch_fuehren(befehl, ", ");
    return 1;
  }

  OBJEKT *objekt = objekt_mit_namen(parameter);
  if (!objekt) { // Dann ist vielleicht eine Zeit.
    long dauer = myatol_mit_o(parameter);
    if (!dauer) {
      fehlermeldung1(L("Deine Angabe ist weder ein existierendes Objekt, ","That is neither an existing object "));
      fortsetzung(L("noch eine Dauer.\n","nor a period of time.\n"));
      return 1;
    }

    if (befehl_dauert_noch(dauer)) return 0;
    tagebuch_fuehren(befehl,", ");
    return 1;
  }

  // Ich beende den Befehl, wenn die Einheit bei mir oder in(!) mir ist.

  if ((ort() == objekt->ort() && adresse == objekt->adresse)
      || objekt->ort() == this) {
    tagebuch_fuehren(befehl,", ");
    return 1;
  }

  else return 0; // Noch nicht da.
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_benennen()
  * 
  * Befehl ohne Fehlermoeglichkeit, der auch keine Zeit dauert. Erlaubt
  * dem Spieler, fuer seine Einheit das Attribut NAME auf einen belie-
  * bigen Wert zu setzen.
  *
  * @param
  * char *befehl: NA%s Mit neuem Namen der Einheit
  *
  * @return
  * short 1.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_benennen(char *befehl)
{
  attribut_setzen("NAME",nice_gross_klein(befehl+2));
  tagebuch_fuehren(befehl, ", ");
  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_prioritaet_setzen()
  * 
  * Wird nur von strategi.cpp eingeschoben und sorgt dafuer, dass vor
  * oder nach Ausfuehrung eines durch eine Strategie hervorgerufenen
  * Befehls	die Prioritaet richtig gesetzt wird. Und der Phasenzaehler
  * wird auch wieder richtig gesetzt.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_prioritaet_setzen(char *befehl)
{
  long pri = befehl[2] - '0';
  if (pri<0 || pri>9) aktuelle_prioritaet = 0;
  else aktuelle_prioritaet = pri;

  if (mystrlen(befehl) > 3) // Damit alte Spielstaende funktionieren.
    phasenzaehler = myatol(befehl+3);

  // Kein report, da kein Spielerbefehl. Phasenzaehler nicht loeschen.
  befehl_durch_naechsten_ersetzen();
  return -1; // Befehl sofort fertig.
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_strategie_waehlen()
  * 
  * Realisiert den Befehl SW, mit dem der Spieler fuer eine Einheit
  * eine Strategie festlegen kann.
  *
  * @param
  * char *befehl: "SW%d", Nummer der Strategie, 0=keine, O zaehlt als 0.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_strategie_waehlen(char *befehl)
{
  long stra;
  if (befehl[2]=='o' || befehl[2]=='O') stra = 0; // Tippfehler O statt 0
  else stra = myatol(befehl+2);
  if (befehl[3]=='o' || befehl[3]=='O') stra += 10; // Tippfehler O statt 0
  
  if (stra < 0 || stra >= ANZAHL_STRATEGIEN)
  {
    fehlermeldung1(L("Die gew~ahlte Strategie gibt es nicht. M~oglich ","The selected strategy does not exist! Try "));
    fortsetzungp(L("sind die Strategien 0 bis %s.\n","strategy 0 up to %s.\n"),myltoa(ANZAHL_STRATEGIEN));
    return 1;
  }

  // Manche Strategien benoetigen die Entwicklung Heeresleitung.
  if (!staat()->info("HAT_ENTWICKLUNG",(char *)L("Heeresleitung","Militarism"))
       && (strategie_flags[stra] & SB_HEERESLEITUNG)) {
    fehlermeldung1(L("F~ur diese Strategie mu~s dein Staat erst ","To select that strategy our empire must have discovered"));
    fortsetzung(L("die Kunst der Heeresleitung erlernen.\n","the secret of militarism.\n"));
    return 1;
  }

  // Strategie S_BODENSCHATZSUCHE benoetigt den Bergbau
  if (stra == S_BODENSCHATZSUCHE && !staat()->info("HAT_ENTWICKLUNG",(char *)L("Bergbau","Mining"))) {
    fehlermeldung1(L("F~ur die Strategie 5 ben~otigt unser Staat die Entwicklung ","To select startegy  5 our empire must have discovered the "));
    fortsetzung(L("des Bergbaus.\n","secret of Mining.\n"));
    return 1;
  }

  // Die Strategie S_BODENSCHATZSUCHE ist nur fuer Einheiten mit "Graben" moeglich.
  if (stra == S_BODENSCHATZSUCHE && !attribut_gesetzt("Graben")) {
    fehlermeldung1(L("Die Strategie 5 (Bodenschatzsuche) ist nur f~ur Einheiten ","Strategy 5 (Search for Mineral Resources) is only sensible if "));
    fortsetzung(L("sinnvoll, die den Befehl 'BS' ausf~uhren k~onnen.\n","the unit can execute the SM command.\n"));
    return 1;
  }

  // Die Strategie S_STRASSENBAU nur mit Strassenbau und fuer Bautrupps.
  if (stra == S_STRASSENBAU && !staat()->info("HAT_ENTWICKLUNG",(char *)L("Stra~senbau","Road_Construction")))
  {
    fehlermeldung1(L("F~ur die Strategie 10 ben~otigt unser Staat die Entwicklung ","Strategy 10 can only be selected after the discovery of Road Construction "));
    fortsetzung(L("des Stra~senbaus.\n",".\n"));
    return 1;
  }

  if (stra == S_STRASSENBAU && (!attribut_gesetzt("KannBauen")
  			      ||!attribut_gesetzt("Boden")))
  {
    fehlermeldung1(L("Die Strategie 10 ist nur f~ur landgebundene Baueinheiten "," Strategy 10 only makes sense in connection with construction units "));
    fortsetzung(L("wie z.B. Bautrupps m~oglich.\n","like e.g. Bulldozers.\n"));
    return 1;
  }

  // Eventuell nicht fuer Flugzeuge
  if (attribut_gesetzt("Luft") && (strategie_flags[stra] & SB_NICHT_FLUG)) {
    fehlermeldung1(L("Diese Strategie ist f~ur Flugzeuge nicht sinnvoll.\n","This strategy seems pointless for aircrafts.\n"));
    return 1;
  }
  
  // Eventuell nicht fuer zivile Einheiten
  if (!attribut_gesetzt("Militaerisch")
      && (strategie_flags[stra] & SB_NICHT_ZIVIL)) {
    fehlermeldung1(L("Diese Strategie ist f~ur zivile Einheiten nicht sinnvoll.\n","This strategy seems pointless for civil units.\n"));
    return 1;
  }

  // Eventuell nur fuer Flugzeuge
  if (!attribut_gesetzt("Luft")
      && (strategie_flags[stra] & SB_NUR_FLUG)) {
    fehlermeldung1(L("Diese Strategie ist nur f~ur Flugzeuge w~ahlbar.\n","This strategy can only be selected for aircrafts.\n"));
    return 1;
  }

  // Eventuell nur fuer Fernkampfeinheiten
  if (!attribut_gesetzt("FEUERKRAFT")
      && (strategie_flags[stra] & SB_FERNKAMPF)) {
    fehlermeldung1(L("Diese Strategie ist nur f~ur Fernkampfeinheiten sinnvoll.\n","This strategy makes only sense with units capable of firing.\n"));
    return 1;
  }

  aktuelle_strategie = stra;
  tagebuch_fuehren(befehl, ", ");
  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_einnehmen()
  * 
  * Um eine Stadt oder ein Bergwerk einzunehmen genuegt es nicht, wenn
  * sich die Einheit auf das gleiche Feld bewegt. Es muss zudem noch
  * der Befehl EN gegeben werden. Flugzeuge koennen nicht einnehmen.
  *
  * @return
  * short: Wie ueblich.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_einnehmen()
{
  // In der Demokratie und Technokratie ist der Befehl nicht erlaubt.

  if (einfluss_vorhanden("KPEN")) {
    fehlermeldung2(L("EN: Solch militante Aktionen wie das Einnehmen ","OC:  Militant actions like occupying foreign"));
    fortsetzung(L("fremder St~adte sind in einer modernen Staatsform wie der ","towns are not possible in our modern form of government. "));
    fortsetzung(L("unsrigen nicht denkbar!\n","\n"));
    return 1;
  }
  
  if (attribut_gesetzt("Luft")) {
    fehlermeldung2(L("EN: Es ist unm~oglich, mit Lufteinheiten St~adte oder ","OC: It is impossible to occupy towns or structures "));
    fortsetzung(L("Einrichtungen einzunehmen.\n","with aerial units.\n"));
    return 1;
  }

  // Ich kann nur Staedte oder Weltbauten einnehmen. Dazu muss ich mich
  // dort befinden.

  OBJEKT *opfer;
  opfer = stadt_hier();
  if (!opfer) opfer = weltbaut_hier();
  if (!opfer) { // Nichts da zum Einnehmen.
    fehlermeldung2(L("EN: Hier ist weder eine Stadt noch irgendeine Einrichtung.\n","OC: Well, there is neither a town nor a structure.\n"));
    return 1;
  }

  // So. Mal sehen, ob unser Staat das Ding nich eh' schon besitzt.

  if (staat()->besitzt(opfer)) {
    fehlermeldung2(L("EN: Nicht n~otig. Geh~ort ohnehin schon zu unserem Staat.\n","OC: Ridiculous. This already belongs to us.\n"));
    return 1;
  }

  // Nur militaerische Einheiten koennen eine Stadt einnehmen.

  if (!attribut_gesetzt("Militaerisch")) {
    fehlermeldung2(L("EN: Kann nur von mili~arischen Einheiten ausgef~uhrt werden.\n","OC: Can only be executed by military units.\n"));
    return 1;
  }

  opfer->besitzer->kommando("AGGRESSION",this);

  if (befehl_dauert_noch(DA_EROBERN)) return 0; // Dauert seine Zeit

  // Ich kann die Stadt / das Bergwerk nur einnehmen, wenn dort keine
  // Einheiten stehen, die dies verhindern. Siehe Anleitung Befehl EN.
  
  DOUBLIST *gegnerliste = 
      welt()->alle_objekte_bei_adresse(adresse, "TYP=EINHEIT");

  enum { EINNEHMEN, FREUNDE_BESITZER, FREUNDE_BOES,  ANGREIFEN } aktion;
  aktion = EINNEHMEN;
  
  FOR_EACH_OBJEKT_IN(gegnerliste)
      DO ({
	  if (!staat()->besitzt(objekt)) // eigene Einheiten ignorieren
	  {
	      if (staat()->endgueltig_freundlich_gegenueber(objekt)) {
		  if (opfer->staat() == objekt->staat()) {
		      aktion = FREUNDE_BESITZER;
		      break;
		  }
		  else if (!uns_liebt(objekt)) {
		      aktion = FREUNDE_BOES;
		      break;
		  }
	      }
	  }
      });

  if (aktion == EINNEHMEN) // weitere Faelle pruefen
  {
      FOR_EACH_OBJEKT_IN (gegnerliste) 
	  DO ({
	      if (opfer->staat() == objekt->staat()) { // Stadtbesitzer verteidigt immer
		  aktion = ANGREIFEN;
		  break;
	      }
	      else if (!uns_liebt(objekt)) {
		  aktion = ANGREIFEN;
		  break;
	      }
	  });
  }

  delete gegnerliste;

  switch (aktion) {
  case ANGREIFEN:
      // Was tun, wenn die Einheit nicht angreifen KANN? Weil sie z.B. ein Schiff auf Land
      // ist? Wenn ich in so einem Fall einfach einen AN Befehl vorschiebe, dann gibt's
      // ein feine Endlosschleife. 

      // Meines Wissens ist die einzige Situation, wo der befehl EN erlaubt ist, aber man nicht
      // Angreifen kann, der Fall Schiff auf Land. Deshalb fange ich diesen hier explizit ab.
      if (attribut_gesetzt("See") && !mystrcmp(gelaende_attribut("ART"),"Boden")) {
	  fehlermeldung2(L("EN: Schiffe k~onnen nur auf See angreifen. Aber wir m~u~sten Angreifen, um diesen Platz einzunehmen!\n","OC: Ships can only attack while at sea. But this square is being defended against us.\n"));
	  return 1;
      }
      else {
	  befehl_vorschieben(L("AN","AT"));
	  return -1;
      }

  case FREUNDE_BESITZER:
      fehlermeldung2(L("EN: Diese Stadt geh~ort unserem guten Freund. Seine Truppen werden sie verteidigen. Wir können keine Einheiten von endg~ultig freundlichen Staaten angreifen!\n","OC: This town belongs to one of our beloved allies and his/her troops will certainly defend it. We cannot attack units from allied empires.!\n"));
      return 1;

  case FREUNDE_BOES:
      fehlermeldung2(L("EN: Hier stehen Einheiten von einem Staat, den wir als endg~ultig freundlich eingestellt haben, er uns aber als gegnerisch oder neutral. Wir k~onnen weder die Stadt einnehmen noch ihn angreifen.\n","OC: There are troops of an empire for which we have selected allied status. The other empire selected neutral or hostile status. Thus we can neither attack or attempt an occupation.\n"));
      return 1;

  case EINNEHMEN:
      opfer->kommando("EINGENOMMEN", this, staat());
      char *bezeichnung = opfer->a_name();
      if (!bezeichnung) bezeichnung = opfer->a_gattung();
      tagebuch_fuehren(L("EN (","OC ("),", ");
      tagebuch_fuehren(L(" eingenommen)"," successfully occupied! Gotcha!)"), bezeichnung);
      return 1;
  }
  return 1; // Darf eigentlich nicht vorkommen!
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_feuern()
  * 
  * Realisiert Befehl FE, mit dem Fernkampfeinheiten ein bestimmtes
  * Feld bombardieren koennen.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_feuern(char *befehl, long phase)
{
  if (attribut_gesetzt("Gefeuert")) {
    fehlermeldung1(L("Wir k~onnen jede Runde nur ein einziges Mal feuern.\n","We can only fire once each turn..\n"));
    return 1;
  }

  // Eine Einheit muss zum Feuern das Attribut FEUERKRAFT haben.
  if (!attribut_gesetzt("FEUERKRAFT")) {
    fehlermeldung1(L("Wir k~onnen generell nicht feuern.\n","We cannot fire at all..\n"));
    return 1;
  }

  // Ferner muss sie sich ausserhalb befinden.
  if (!ort_ist_welt()) {
    fehlermeldung1(L("Zum Feuern m~ussen wir uns im Freien befinden!\n","We must be off board to fire!\n"));
    return 1;
  }

  if (einfluss_vorhanden("ANEOFE")) {
    fehlermeldung1(L("In der Technokratie ist keine aggressive Handlung ","Under a technocratic government aggessive actions like this "));
    fortsetzung(L("m~oglich!\n","are prohibited!!\n"));
    return 1;
  }


  // Jetzt bestimme ich das Zielfeld
  ADR zielfeld;
  char *fehler = staat()->ortsangabe_auswerten(befehl+2, adresse, zielfeld);
  if (fehler) {
    fehlermeldung1(fehler);
    return 1;
  }

  // Jetzt kann es ausserdem noch sein, dass das Ziel zu weit entfernt ist.
  if (myatof(attribut("SCHUSSWEITE")) <
		 landschaft()->entfernung_zwischen(adresse, zielfeld))
  {
    fehlermeldung1(L("Das Ziel ist zu weit entfernt. Wir k~onnen maximal","Target out of firing range. We can fire over a maximum"));
    fortsetzungp(L(" %s Felder weit schie~sen.\n"," distance of %s squares.\n"), attribut("SCHUSSWEITE"));
    return 1;
  }

  attribut_setzen("Gefeuert");  
  
  tagebuch_fuehren(befehl,", ");

  long kraft = myatol(attribut("FEUERKRAFT"));

  // Eventuell Abwehrraketen zum Zug kommen lassen.
  if (welt()->abwehr_durch_mobile_raketen(adresse, zielfeld, kraft, staat()))
  {
      fussnote(L("Unser Gescho~s wird von einer mobilen Abwehrrakete pulverisiert.","Our missile has been ripped apart by a mobile defensive missile!"));
  }
  else
  {
      feld_beschiessen(zielfeld, kraft);
      // Moeglicherweise sehe ich nun eine Einheit nicht mehr, weil ich sie
      // vernichtet habe. Deshalb:
      sichtungen_aktualisieren(phase);
  }

  // Marschflugkoerper sind Wegwerfartikel.
  if (attribut_gesetzt("Marschflug")) kommando_sich_vernichten();
  
  return 1; // Befehl fertig ausgefuert.
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_bombardieren()
  * 
  * Realisiert den Befehl BO, mit dem Flugzeuge das Feld bebomben koen-
  * nen, ueber dem sie fliegen. Andere Flugzeuge koennen nicht bebombt
  * werden.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_bombardieren(long phase)
{
  long kraft = myatol(attribut("BOMBEN"));
  if (!kraft) {
    fehlermeldung2(L("BO: Wir haben keine Bomben zur Verf~ugung.\n","BO: We got no bombs so we cannot bomb. Got that?\n"));
    return 1;
  }

  if (einfluss_vorhanden("ANEOFE")) {
    fehlermeldung2(L("BO: In der Technokratie ist keine aggressive Handlung ","BO: Under a technocratic government aggessive actions like this "));
    fortsetzung(L("m~oglich!\n","are prohibited!\n"));
    return 1;
  }

  if (attribut_gesetzt("Reserve")) {
    fehlermeldung2(L("BO: Solange wir in Reserve sind, k~onnen wird nicht","BO: We cannot bombard while in"));
    fortsetzung(L(" bombardieren.\n"," reserve.\n"));
    return 1;
  }

  // Pro Runde kann nur einmal gebombt werden. Wenn gebombt wurde, wird
  // das Attribut Gebombt gesetzt.

  if (attribut_gesetzt("Gebombt")) {
    fehlermeldung2(L("BO: Wir k~onnen jede Runde nur ein einziges Mal bombardieren.\n","BO: We can bombard only once each turn.\n"));
    return 1;
  }

  if (!ort_ist_welt()) {
    fehlermeldung2(L("BO: Wir k~onnen Bomben abwerfen, wenn wir uns in der Luft","BO: We can only bombard when launched."));
    fortsetzung(L("befinden.\n","\n"));
    return 1;
  }

  attribut_setzen("Gebombt");
  tagebuch_fuehren(L("BO","BO"),", ");

  // Jetzt versuche ich, etwas zu erwischen.
  feld_beschiessen(adresse, myatol(attribut("BOMBEN")));

  // Moeglicherweise sehe ich nun eine Einheit nicht mehr, weil ich sie
  // vernichtet habe. Deshalb:

  sichtungen_aktualisieren(phase);

  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::feld_beschiessen()
  * 
  * Befehle feuern und bombardieren. Alle Objekte auf dem Feld
  * werden beschossen. Ausgenommen sind eigenen Objekte und solche
  * von endgueltig befreundeten Staaten!
  * @param
  * ADR&     zielfeld
  * long 	feuerkraft
  ---------------------------------------------------------------------------*/
void EINHEIT::feld_beschiessen(ADR& zielfeld, long anfangsfeuerkraft)
{
  // Jedes Objekt auf dem Zielfeld bekommt eine Nachricht davon, dass es
  // beschossen wird. Was es damit macht, ist dessen Sache. Wenn ich auf
  // mein eigenes Feld schiesse, dann schiesse ich als Grundeinheit nur
  // auf Ziele in der Luft

  char *beding = NULL;
  if ((attribut_gesetzt("Boden")||attribut_gesetzt("See")) && zielfeld==adresse)
     beding =  "Luft";

  short flugzeug_test = (attribut_gesetzt("Luft") && zielfeld==adresse);

  // Das Potential nimmt mit jedem Treffer ab. Wenn keines mehr uebrig
  // ist, kann keine Einheit mehr beschossen werden. Damit die Abstufung
  // feiner ist, wird alles um den Faktor 10 gestreckt. Das Potential
  // ist am Anfang also 10 mal so hoch wie die Feuerkraft

  const long streckungsfaktor = 10;
  long potential = streckungsfaktor * anfangsfeuerkraft;
  
  FOR_EACH_OBJEKT_IN (landschaft()->alle_objekte_bei_adresse(zielfeld, beding))
  DO_AND_DELETE
  ({
    /* Keine eigenen und endgueltig befreundeten */
    if (!staat()->besitzt(objekt) && 
        !((STAAT *)staat())->endgueltig_freundlich_gegenueber(objekt))
    {
      /* Jetzt muss ich trotzdem noch abfragen, damit nicht ein Flugzeug
         andere Flugzeuge auf dem gleichen Feld bombardiert */

      if (!flugzeug_test 
           || !objekt->attribut_gesetzt("Luft") 
       	   || objekt->attribut_gesetzt("Reserve") 
           || objekt->attribut_gesetzt("Unversorgt"))
      {     

        long feuerkraft = potential / streckungsfaktor;
        if (objekt->typ_ist("EINHEIT")) {
          if (feuerkraft > 0) {
            short rwert = objekt->kommando("BESCHUSS",myltoa(feuerkraft), this);
            if (rwert) { // getroffen
	      tagebuch_fuehren(objekt->a_gattung(), ", [");
	      tagebuch_fuehren(L("] vernichtet","] destroyed"),"");
	      medaille_bekommen();
	      if (rwert > 0) potential -= io_random(rwert * streckungsfaktor) + 1;
 	      if (potential < 0) potential = 0;
    	    }
          } // Feuerkraft > 0, ansonsten gar nicht schiessen.
        } // EINHEIT 

        else if (!objekt->kommando("BESCHUSS", myltoa(anfangsfeuerkraft), this))
        {
          if (objekt->typ_ist("WELTBAUT")) // Vernichtet!
          {
	      tagebuch_fuehren(objekt->a_gattung(), ", [");
	      tagebuch_fuehren(L("] vernichtet","] destroyed"),"");
	      medaille_bekommen();
          }
        } /* Stadt oder Weltbaut beschiessen */
      } /* flugzeug_test */
    } /* keine eigenen oder EF - Objekte */
  })

  // Fuer die Statistik ueber die Friedlichkeit muss ich mitzaehlen...
  staat()->kommando("ANGRIFF_ZAEHLEN"); /* Fuer die Statistik */
  anzahl_kaempfe ++;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_ueberstellen()
  * 
  * Durch diesen Befehl bekommt die Einheit einen neuen Besitzer. Ich
  * kann an fremde Staaten ueberstellen, die mir freundlich gesonnen
  * sind, wenn ich mich direkt dort befinde.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_ueberstellen(char *befehl)
{
  char *stadtkuerzel = befehl+2;
  OBJEKT *stadt;

  // Wenn man kein Stadtkuerzel angibt heisst das, dass man direkt an 
  // die Stadt ueberstellen will, wo man sich befindet.

  if (!stadtkuerzel[0]) {
    stadt = stadt_hier();
    if (!stadt) {
      fehlermeldung1(L("Der Befehl verlangt die Angabe der Abk~urzung einer Stadt, z.B. USHSA, ","This command requires the abbreviation of a  town, e.g. ANCPA, "));
      fortsetzung(L("es sei denn die Einheit befindet sich auf dem selben Feld wie eine Stadt.\n","except the unit is on the same square as a town.\n"));
      return 1;
    }
  }

  else { // schauen, ob eine Stadt mit dem Namen existiert
    stadt = objekt_mit_namen(stadtkuerzel);
    if (!stadt || !stadt->typ_ist("STADT")) {
      fehlermeldung1(L("Es gibt keine Stadt mit dieser Abk~urzung.\n","There is no town with such an abbreviation.\n"));
      return 1;
    }
  }

  // Gehoert die Stadt einem anderen Mitspieler, so muss ich mich dort
  // befinden.
  
  if (stadt->besitzer != staat()) {
    if ((!(stadt->adresse == adresse)) &&
	(!(stadt->adresse == ort()->adresse))) // Falls ich im Konvoi bin
    {
      fehlermeldung1(L("Beim ~Uberstellen an St~adte von Mitspielern mu~s sich die ","When assigning units to allied towns, the unit "));
      fortsetzung(L("Einheit direkt bei der Stadt befinden.\n","must be next to it.\n"));
      return 1;
    }
  }

  if (!uns_liebt(stadt)) {
    fehlermeldung1(""); // Wegen der Fussnote.
    fortsetzungp(L("%s weigert sich, unsere Versorgung zu ~ubernehmen.\n","%s refuses to assume responsibility for the upkeep.\n"),
      stadt->a_name());
    return 1;
  }
  
  // Aber es sollte die andere Stadt davon informiert werden, dass sie
  // soeben eine Einheit bekommen hat. Sofern wir nicht eh' schon dieser
  // Stadt unterstanden.
  if (besitzer != stadt)
  {
    stadt->report(L("Wir ~ubernehmen die Verantwortung f~ur die Einheit %s,","We assume responsibility for unit  %s,"),name);
    stadt->report(L(" die bis zuvor %s unterstand.\n"," formerly assigned to %s.\n"),besitzer->a_name());
  }

  // alten Staat merken (siehe unten)
  OBJEKT *alter_staat = staat();
  
  // Und jetzt gebe ich mich der anderen Stadt.
  geben(stadt, this);

  // Tagebuch...
  tagebuch_fuehren(befehl, ", ");

  // Und nun nochwas: Wenn sich unser Staat geaendert hat, dann
  // loesche ich alle restlichen Befehle, um gewisse Betrugsmoeglichkeiten
  // zu vermeiden. Ganz wichtig hier: Durch die Uebergabe von 1 an die
  // Funktion alle_befehle_abbrechen wird verhindert, dass der aktuelle
  // US-Befehle geloescht wird. Denn die Hauptschleife in OBJEKT macht
  // ein delete auf den Befehl, wenn ich hier 1 zurueckgebe. Dadurch
  // kaeme es dann zu einem doppelten delete auf dem Befehl!
  
  if (alter_staat != staat()) alle_befehle_abbrechen(1);

  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_bodenschaetze_suchen()
  * 
  * Die Einheit sucht an der aktuellen Stelle nach Bodenschaetzen.
  * Dies koennen nur Einheiten mit dem Attribut "Graben".
  *
  * @return
  * 1, falls Befehl fertig, 0 sonst.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_bodenschaetze_suchen()
{
  if (!ort_ist_welt()) { // Geht nur draussen
    fehlermeldung2(L("BS: Wir k~onnen nur drau~sen nach Bodensch~atzen suchen.\n","SM: We can only search outdoors for mineral resources.\n"));
    return 1;
  }

  // Der Staat braucht die Entwicklung "Bergbau"
  if (!staat()->info("HAT_ENTWICKLUNG",(char *)L("Bergbau","Mining"))) {
    fehlermeldung2(L("BS: F~ur diesen Befehl ben~otigt unser Staat die Entwicklung ","SM: We do not know the secrets of Mining so we cannot execute that "));
    fortsetzung(L("des Bergbaus.\n","command.\n"));
    return 1;
  }
 
  
  if (!attribut_gesetzt("Graben")) {
    fehlermeldung2(L("BS: Wir haben weder das Wissen noch die Ausr~ustung zur ","SM:We neither have the equipment nor the knowledge for "));
    fortsetzung(L("Suche nach Bodensch~atzen.\n","searching mineral resources.\n"));
    return 1;
  }


  if (befehl_dauert_noch(DA_BODENSCHATZSUCHE)) return 0;

  // Finden kann ich nur etwas, wenn eines meiner Attribute Graben und
  // Bohren auch in der Landschaft vorhanden sind.

  tagebuch_fuehren(L("BS:","SM:"),", ");

  if ( attribut_gesetzt("Graben")
	   && welt()->gelaendeform_attribut_gesetzt(adresse, "Graben") )
  {
    // Jetzt suche ich mal.

    if (welt()->nach_resourcen_bohren(adresse)) { // Gefunden

      RESOURCE_VEKTOR gefres(welt()->bodenschatz_bei(adresse));
      gefres[L('X','X')] = 0; // Meteoritium soll man nicht sehen!
      
      if (gefres.betrag()) {
        // Das Ergbnis teile ich ueber eine Fehlermeldung mit, da hier
        // ein schoener Platzhalter eingefuegt wird.
        tagebuch_fuehren(L("Erfolg (","SUCCESS! ("),"");
        char string[150];
        RIC reladr = relative_adresse(adresse);
        sprintf(string, L("Bei (%ld,%ld) Vorkommen von %s gefunden.\n","At (%ld,%ld) we found a deposit of  %s.\n"),reladr.x,
		reladr.y, gefres.to_string());
        fussnote(string);
        tagebuch_fuehren(")","");
        return 1;
      }
    }
  }

  // nichts gefunden.
  tagebuch_fuehren(L("Nichts","Nothing. Niente. Rien. Nix da. Niczego."));
  return 1;
}



/**---------------------------------------------------------------------------
  * EINHEIT::befehl_meteorit_bergen()
  * 
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_meteorit_bergen()
{
  if (!attribut_gesetzt("MeteoritBergen"))
  {
    fehlermeldung2(L("MB: Nur Spezialeinheiten k~onnen Meteoriten bergen\n","UM: Only specialized units may unearth meteorites!\n"));
    return 1;
  }
  
  if (befehl_dauert_noch(DA_METEORIT_BERGEN)) return 0;

  tagebuch_fuehren(L("MB: ","UM: "),", ");

  // So. Nun schaue ich doch 'mal, ob da nichts ist.    
  if (welt()->bodenschatz_vorhanden(adresse)) {
    RESOURCE_VEKTOR& bodenschatz = welt()->bodenschatz_bei(adresse);
    short meteorit = bodenschatz[L('X','X')];
    bodenschatz[L('X','X')] = 0; // Loeschen!
    if (meteorit) {
      RESOURCE_VEKTOR metres(meteorit, L('X','X'));
      lager.addiere(metres);
      tagebuch_fuehren(metres.to_string(),"");
      welt_informieren_ueber_meteorit(meteorit);
      return 1;
    }
  }
  
  tagebuch_fuehren(L("Nichts (mehr) zu finden","Nothing there.")," ");
  return 1;
}


/**---------------------------------------------------------------------------
  * welt_informieren_ueber_meteorit()
  * 
  * Meldung an alle Staaten, dass ein Meteorit geborgen wurde.
  * @param
  * short meteorit: Menge an X
  ---------------------------------------------------------------------------*/
void EINHEIT::welt_informieren_ueber_meteorit(short groesse)
{
  char *klasse;
  if (groesse <= 10) klasse = L("I (1-10 kg)","I (1-10 lbs)");
  else if (groesse <= 20) klasse = L("II (11-20 kg)","II (11-20 lbs)");
  else if (groesse <= 35) klasse = L("III (21-35 kg)","III (21-35 lbs)");
  else if (groesse <= 60) klasse = L("IV (36-60 kg)","IV (36-60 lbs)");
  else                    klasse = L("V (~uber 60 kg)","V (more than 60 lbs)");

  char string[513];
  sprintf(string, L("Unser Kongre~szentrum meldet: Wissenschaftler aus %s bergen einen Meteoriten der Klasse %s.\n","Report from our congress hall : Scientists from %s unearth a level  %s meteorite.\n"),
    staat()->a_name(), klasse);  
 
  FOR_EACH_OBJEKT_IN (&globale_objekt_menge)
  DO ({
    if (objekt->typ_ist("STAAT") && objekt != staat())
    {
      if (objekt->einfluss_vorhanden("KONGRESS"))
        objekt->report(string);
    }
  })

}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_bauen()
  * 
  * Gibt einer Einheit den Befehl, eine Weltbaut zu bauen, deren
  * Gattung als Parameter uebergeben wird. Auch Weltbauten tauchen
  * als Vorhaben in der Enzyklopaedie auf.
  *
  * @param
  * char *befehl: BA%s Abkuerzung der Gattung der Weltbaut, z.B. "BG"
  * fuer Bergwerk
  *
  * @return
  * Wie bei Befehlen Standard.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_bauen(char *befehl)
{
  // Bauen geht nur ausserhalb
  if (!ort_ist_welt()) {
    fehlermeldung1(L("Innerhalb eines Transportmittels k~onnen wir ","While being transported it seems impossible to "));
    fortsetzung(L("nichts bauen.\n","construct anything..\n"));
    return 1;
  }

  // Ist dort schon etwas?
  if (!platz_fuer_baut()) {
    fehlermeldung1(L("Wo schon eine Stadt oder ein Bauwerk in der Landschaft","Hey you! There is already a town or structure!"));
    fortsetzung(L(" steht, k~onnen wir nichts mehr bauen.\n"," Think again!\n"));
    return 1;
  }

  if (!befehl[2]) {
    fehlermeldung1(L("Es fehlt die Angabe, was genau gebaut werden soll.\n","Please state what you exactly want to build/construct\n"));
    return 1;
  }

  // Von der Enzyklopaedie hole ich mir den Eintrag fuer das Bauwerk

  VORHABEN_ENZ *vorhaben = enzyklopaedie()->vorhaben(befehl+2);
  if (!vorhaben) { // Gibt es nicht. Falsche Abkuerzung.
    fehlermeldung1(L("Die Abk~urzung ","The abbreviation "));
    fortsetzungp(L("'%s' gibt es nicht.\n","'%s' does not exist.\n"), befehl+2);
    return 1;
  }

  char *bautenname = vorhaben->name;

  // Nur Bautrupps und Baukonvois koenne bauen.

  if (!attribut_gesetzt("KannBauen")) {
    fehlermeldung1(L("Nur Baueinheiten k~onnen den Befehl BA ausf~uhren.\n","Only construction units can execute BU commands.\n"));
    return 1;
  }

  // Nun schaue ich mir mal den Eintrag in der Enzyklopaedie an. Da steht
  // naemlich, welche Voraussetzungen
  // der Staat haben muss, damit die Einheit das Teil bauen darf.

  char *antwort = staat()->info("HAT_ENTWICKLUNG", 
			  vorhaben->voraussetzungen_im_staat.to_string());
  if (!antwort) {
    fehlermeldung1(L("Unser Staat besitzt die n~otigen Voraussetzungen noch nicht.\n","Our empire does not have the necessary basics.\n"));
    return 1;
  }

  // Das Vorhaben muss den Typ WELTBAUT haben.
  
  char *typ = vorhaben->start_attribute.abfragen("TYP");
  if (mystrcmp(typ, "WELTBAUT")) {
    fehlermeldung1(L("Dieses Projekt kann nur in einer Stadt verwirklicht werden.\n","This project can only be completed in a town.\n"));
    return 1;
  }
  
  // Hat die Einrichtung das Attribut ART, so muss dies mit der Gelaendeform
  // uebereinstimmen, und ausserdem muss der Wert (Boden, See) bei der Einheit
  // als  Attribut gesetzt sein.
  
  char *art = vorhaben->start_attribute.abfragen("ART");
  if (art) {
    if (!attribut_gesetzt(art)) {
      if (!mystrcmp(art, "Boden")) { // Waere eigenlich Boden gewesen
        fehlermeldung1(L("An Land k~onnen wir nichts bauen.\n","We cannot build anything on the continent.\n"));
        return 1;
      }
      else {
        fehlermeldung1(L("Auf See k~onnen wir nichts bauen.\n","We cannot build anything at sea.\n"));
        return 1;
      }
    }
    
    if (mystrcmp(gelaende_attribut("ART"), art))
    {
      if (!mystrcmp(art, "Boden")) { // Waere eigenlich Boden gewesen
        fehlermeldung1(L("Diese Einrichtung kann nur an Land gebaut werden.\n","This structure can only be built on the continent.\n"));
        return 1;
      }
      else {
        fehlermeldung1(L("Diese Einrichtung kann nur auf See gebaut werden.\n","This structure can only be built at sea.\n"));
        return 1;
      }
    }
  } // ART gesetzt


  // Bei Bergwerken muss auch ein Bodenschatz vorhanden und gefunden sein!
  char *fq = vorhaben->start_attribute.abfragen("FOERDERQUOTE");
  if (fq && *fq) 
  {
    if (!welt() -> feld_attribut(adresse, FELD_ATTRIBUT_BODEN_UNTERSUCHT))
    {
      // Bei SW5 muesste der BS-Befehl schon von selbst kommen. Dann warte
      // ich einfach ab!
      
      if (aktuelle_strategie == S_BODENSCHATZSUCHE) return 0;
      
      // Ansonsten bin ich so zuvorkommend, und erzeuge automatisch
      // den noetigen BS-Befehl
      
      else {
        befehl_vorschieben(L("BS","SM")); // Zuerst mal suchen
        return -1;
      }
    }

    // Ist auch das richtige Zeugsel hier? Wenn nicht, dann gibt es
    // eine Fehlermeldung "inline". Ohne Fussnote, damit die Einheiten,
    // die mit WIBABL/REO oder sowas das Meer absuchen nicht ewig viele
    // Fehler produzieren.
    
    RESOURCE_VEKTOR& res = welt()->bodenschatz_bei(adresse);
    char *rohstoffart = fq + strlen(fq) - 1; // letzter Buchstabe
    if (res[*rohstoffart] == 0) // Nix hier
    {
      char string[60];
      sprintf(string,L("(%s: Kein %c da!)","(%s: No %c!)"), befehl, *rohstoffart);
      tagebuch_fuehren(string, ", ");
      return 1;
    }
  }


  // Jetzt dauerts vielleicht auch noch ein bisschen. Schnellbau haben
  // Baukonvois gesetzt.

  long dauer = attribut_gesetzt("Schnellbau") ? 50 : 100;
  
  if (befehl_dauert_noch(dauer)) return 0; // Noch nicht fertig.

  // Und jetzt erst darf ich die Rohstoffe abziehen.
  lager.subtrahiere(vorhaben->benoetigte_resourcen);

  tagebuch_fuehren(befehl, ", ");

  // Jetzt muss ich das neue Objekt schaffen. Zuerst stelle ich die
  // Attribute zusammen.
  char attrstring[100];
  sprintf(attrstring,"GATTUNG=%s,VERSORGER=%s", bautenname, besitzer->name);

  // Eine eindeutige Abkuerzung brauche ich auch. Ich hole sie von meinem
  // Staat mit dem Info "EINDEUTIGER_NAME".

  OBJEKT *weltbaut = objekt_schaffen(staat()->info("EINDEUTIGER_NAME"),
		       "WELTBAUT", attrstring);
  if (!weltbaut) { // Huch?
      log('I', "EINHEIT::befehl_bauen(): Schaffen klappt nicht!");
      return 1;
  }

  // Und nun muss ich das Teil noch an den richtigen Ort bringen.
  if (weltbaut->ort_wechseln(ort(), adresse)) { // Huch!
      log('I', "EINHEIT::befehl_bauen() Ortwechsel klappt nicht!");
      delete weltbaut;
      return 1;
  }

  // Der Spieler bekommt noch ein Info ueber sein neues Bauwerk.
  ((STAAT *)staat())->spielerinformation(vorhaben->info_datei_filename);

  // Und nun gebe ich das ganze noch dem Staat.
  geben(staat(), weltbaut);

  // Und das war's auch schon wieder. In einer anderen Version kann
  // ich mir auch vorstellen, das das Objekt nach dem BA vernichtet
  // wird, da z.B. seine Resourcen fuer den Bau verbraucht sind.

  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_verwuesten()
  * 
  * Macht Strasse und Ackerland auf dem Feld kaputt. Nur fuer militae-
  * rische Einheiten.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_verwuesten()
{
  if (attribut_gesetzt("Luft")
      || attribut_gesetzt("See")
      || !attribut_gesetzt("Militaerisch"))
  {
    fehlermeldung2(L("VW: Nur f~ur milit~arische Bodeneinheiten.\n","RZ: Only for ground-based miltary units.\n"));
    return 1;
  }
  
  if (!ort_ist_welt()) {
    fehlermeldung2(L("VW: Nur au~serhalb m~oglich!\n","RZ: Outdoors only!\n"));
    return 1;
  }

  if (befehl_dauert_noch(DA_VERWUESTEN)) return 0;

  tagebuch_fuehren(L("VW","RZ"), ", ");
  if (welt()->feld_attribut(adresse, FELD_ATTRIBUT_STRASSE))
  {
    welt()->loesche_feld_attribut(adresse, FELD_ATTRIBUT_STRASSE);
    tagebuch_fuehren(L("Stra~se zerst~ort","Road destroyed"), ", ");
  }

  char *ent_ac = gelaende_attribut("ENT_AC");
  if (ent_ac) {
    short form = welt()->gelaendeform_mit_abkuerzung(ent_ac);
    if (form) {
      tagebuch_fuehren(gelaende_attribut("REP"), ", ");
      tagebuch_fuehren(L("verw~ustet","devastated")," ");
      welt()->gelaendeform_aendern(adresse, form);
    }
  }
  
  return 1;
}
  

/**---------------------------------------------------------------------------
  * EINHEIT::befehl_wr_af_tr()
  * 
  * Realisiert meherere Befehle auf einmal (WR, AF, TR, AC). Dies liegt
  * daran, dass sie sehr parallel ablaufen und sich nur geringfuegig
  * unterscheiden (siehe Funktion.)
  *
  * @param
  * char *befehl: Befehl. Keiner der Befehle hat Parameter.
  *
  * @return
  * short: Typischer Befehlsfunktionsreturnwert.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_wr_af_tr(char *befehl)
{
  bool wald_roden   = !mystrncmp(befehl, L("WR","DF"),2);
  bool aufforsten   = !mystrncmp(befehl, L("AF","AF"),2);
  bool trockenlegen = !mystrncmp(befehl, L("TR","DS"),2);
  bool ackerland    = !mystrncmp(befehl, L("AC","CS"),2);

  // Zum Gelaendeformen braucht die Einheit das Attribut Formen
  if (!attribut_gesetzt("Formen")) {
    fehlermeldung1(L("Unsere Einheit ist nicht f~ahig, Gel~ande umzuformen!\n","Our unit is not capable of  Terraforming!\n"));
    return 1;
  }

  if (!ort_ist_welt()) {
    fehlermeldung1(L("Nur au~serhalb m~oglich!\n","Outdoors only!\n"));
    return 1;
  }

  char *zielform = gelaende_attribut(befehl);
  if (!zielform) {
    if (wald_roden)
	tagebuch_fuehren(L("(WR: kein Wald da!)","(DF: no forest, no deforest!)"), ", ");
    else if (aufforsten)
	tagebuch_fuehren(L("(AF: falscher Ort!)","(AF: wrong location!)"), ", ");
    else if (trockenlegen)
	tagebuch_fuehren(L("(TR: kein Sumpf!)","(DS: no swamp, no drain!)"), ", ");
    else if (ackerland)
	tagebuch_fuehren(L("(AC: falscher Ort!)","(CS: wrong place!)"), ", ");
    return 1;
  }

  // Jetzt kommt noch etwas hinzu: Zum Acklerland anlegen braucht man
  // die Entwicklung Pflug.
  if (ackerland && !staat()->info("HAT_ENTWICKLUNG",(char *)L("Pflug","Plow")))
  {
    fehlermeldung2(L("AC: Ohne den Pflug k~onnen wir kein Ackerland erstellen.\n","CS: Without the plow we cannot cultivate our soil.\n"));
    return 1;
  }

  short zwei = attribut_gesetzt("Schnellbau") ? 2 : 1;
  if (wald_roden && befehl_dauert_noch(DA_WALDRODUNG / zwei)) return 0;
  else if (aufforsten && befehl_dauert_noch(DA_AUFFORSTUNG / zwei)) return 0;
  else if (trockenlegen && befehl_dauert_noch(DA_TROCKENLEGUNG / zwei)) return 0;
  else if (ackerland && befehl_dauert_noch(DA_ACKERLAND / zwei)) return 0;

  tagebuch_fuehren(befehl,", "); // Reporten.

  // Wenn man beim Roden des Wald Holzverarbeitung hat, dann kann man das
  // Holz in eine Menge Resourcen umwandeln. Nach den neun Regeln vom 15.10.94
  // kommt das Holz direkt zur Stadt, die mich unterhaelt.

  if (wald_roden && staat()->info("HAT_ENTWICKLUNG",(char *)L("Holzverarbeitung","Woodworking")))
  {
      // Der Bautrupp speichert das Holz. 
      RESOURCE_VEKTOR holz_neu(gelaende_attribut("HOLZ"));
      lager.addiere(holz_neu);
      tagebuch_fuehren(holz_neu.to_string(),":");
  }

  welt()->gelaendeform_aendern(adresse,
			       welt()->gelaendeform_mit_abkuerzung(zielform));

  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_strassen_bauen()
  * 
  * Kann nur von Einheiten mit dem Attribut KannBauen ausgefuehrt werden.
  * Setzt auf dem Feld der Einheit das Landschaftsattribut STRASSE.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_strassen_bauen()
{
  if (!attribut_gesetzt("KannBauen")) {
    fehlermeldung2(L("ST: Wir sind nicht ausgebildet und auch nicht ausge","CR: No knowledge, no equipment...We cannot even think about "));
    fortsetzung(L("r~ustet, um Strassen zu bauen.\n","constructing roads.\n"));
    return 1;
  }

  if (!staat()->info("HAT_ENTWICKLUNG",(char *)L("Stra~senbau","Road_Construction"))) {
    fehlermeldung2(L("ST: In unserem Staat wei~s noch niemand, wie man Stra~sen baut.\n","CR: Noboby here knows how to construct roads.\n"));
    return 1;
  }

  if (!ort_ist_welt()) { // Geht nicht.
    fehlermeldung2(L("ST: Strassen k~onnen nur au~serhalb gebaut werden.\n","CR: Roads can only be built outdoors.\n"));
    return 1;
  }

  // Mal schauen, ob nicht schon eine Strasse da ist.
  if (welt()->feld_attribut(adresse, FELD_ATTRIBUT_STRASSE)) {
    fehlermeldung2(L("ST: Wo wir bauen sollen, ist schon eine Strasse.\n","CR: Hey! There´s a road already!!\n"));
    return 1;
  }

  // So. Jetzt kann es losgehen, wenn wir schon genuegend lange gearbeitet
  // haben. Die Baudauer haengt naemlich von der Gelaendeform ab.

  long dauer;
  if (!attribut_gesetzt("Mechanoid"))
  {
      dauer = myatol(gelaende_attribut("BEWEGUNG"));
      if (dauer==0) dauer=1;
      dauer *= DA_STRASSENBAU;
  }
  else dauer = DA_STRASSENBAU;
  if (attribut_gesetzt("Schnellbau")) dauer /= 2;
  if (befehl_dauert_noch(dauer)) return 0;

  // Gut. Nun setzte ich das Feldattribut Strasse. Die Welt wird's mir
  // schon verzeihen, wenn ich selbstaendig eingreife..

  welt()->setze_feld_attribut(adresse, FELD_ATTRIBUT_STRASSE);
  tagebuch_fuehren(L("ST","CR"),", ");

  return 1;
}

/**---------------------------------------------------------------------------
  * EINHEIT::befehl_stadt_gruenden()
  * 
  * Realisiert den Befehl 'SG', mit dem eine Siedlereinheit (und nur
  * eine Siedlereinheit) eine neue Stadt gruenden kann, wobei sie sich
  * selbst aufloest und ihre Einwohner in die neue Stadt eingliedert.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_stadt_gruenden(char *befehl)
{
  char *name_der_stadt = nice_gross_klein(befehl+2);

  if (!strlen(name_der_stadt)) { // Defaultname
    static char *namen[11] = 
    {
      L("Bombay","Ramersdorf"),
      L("Uruk","Uruk"),
      L("Madras","Madras"),
      L("Nairobi","Nairobi"),
      L("Baghdad","Baghdad"),
      L("Belgrad","Sarajevo"),
      L("Bern","Bern"),
      L("Genf","Genf"),
      L("Moroni","Ry´leh"),
      L("Bridgetown","Bridgetown"),
      L("Belmopan","Belmopan")
    };   
    name_der_stadt = namen[io_random(11)];
  }

  // Staedte koennen nur von Siedlern gegruendet werden
  if (!bedingung_erfuellt(L("GATTUNG=Siedler","GATTUNG=Settlers")) && !attribut_gesetzt("BefehlSG")) {
    fehlermeldung1(L("St~adte k~onnen nur von Siedlern oder Stadtgr~undern gegr~undet werden.\n","Only Settlers or City Founders can found a town.\n"));
    return 1;
  }

    
  if (!ort_ist_welt()) { // Ich befinde mich in einer Stadt!
    fehlermeldung1(L("Eine Stadt kann man nur im Freien gr~unden.\n","Of course you can found a town only when off board!.\n"));
    return 1;
  }

  // Ich kann eine Stadt nur dann Gruenden, wenn hier nicht schon
  // eine steht!

  if (!platz_fuer_baut()) { // Steht schon Weltbaut oder andere Stadt dort
    fehlermeldung1(L("Wo schon eine Stadt oder ein Bauwerk steht, k~onnen wir","Hey! There is already a town or structure here. How can we"));
    fortsetzung(L(" keine neue Stadt gr~unden.\n"," found a NEW town here?\n"));
    return 1;
  }

  // Ein Mindestabstand von den anderen Staedten muss eingehalten werden.
  // (Abstand mit echt > Mindestabstand sein).
  
  float mindest_abstand = myatof(lexikon_eintrag("maximale_staedtezahl",3));

  DOUBLIST *hindernisse = 
     welt()->alle_objekte_im_umkreis_von(adresse, mindest_abstand, "TYP=STADT");
  if (hindernisse->count_and_del())
  {
    char zahl[20];
    sprintf(zahl, "%1.1f", mindest_abstand);
    fehlermeldung1(L("Neu gegr~undete St~adte m~ussen mehr als ","Newly founded towns must be at minimum distance of "));
    fortsetzungp(L("%s Felder von der n~achsten Stadt entfernt sein.\n","%s squares to the next town.\n"), zahl);
    return 1;
  }

  // Eine Stadt kann nur gebaut werden, wenn das Feld nicht von einer
  // anderen Stadt bewirtschaftet wird. Da jede Stadt das Feld, auf
  // der Sie steht auch bewirtschaftet, kann ich so auch gleich sicher-
  // stellen, dass sich dort keine andere Stadt befindet.

   if (!landschaft()->info("BEWIRTSCHAFTUNG_PRUEFEN",&adresse)) {
     fehlermeldung1(L("Auf Feldern, die eine andere Stadt bewirtschaftet, kann kein Stadt gegr~undet werden.\n","You cannot found towns on squares operated by another town.\n"));
     return 1;
   }

  // Das Gruenden der Stadt dauert eine gewisse Zeit
  if (befehl_dauert_noch(DA_STADTGRUENDUNG)) return 0; // Noch nicht fertig!

  // Der interne Name der Stadt ist eine dreibuchstabige Abkuerzung, die
  // ich aus dem Namen berechne!

  char *abkuerzung = eindeutige_objekt_abkuerzung(name_der_stadt, 3);
  if (!abkuerzung) { // Mist!
    fehlermeldung2(L("Es gibt schon zuviele St~adte auf der Welt.\n","There are already too many cities on this world.\n"));
    return 1;
  }

  // Jetzt gruende ich aber endlich!
  OBJEKT *neue_stadt;

  // Ich muss noch ein paar Attribute zusammenstellen...
  ATTRIBUT_LISTE attrib;
  attrib.setzen("WELTNAME",ort()->name);
  char ricstring[20];
  sprintf(ricstring,"%ld",adresse.x);
  attrib.setzen("STARTX",ricstring);
  sprintf(ricstring,"%ld",adresse.y);
  attrib.setzen("STARTY",ricstring);
  attrib.setzen("NAME",name_der_stadt);
  attrib.setzen("FrischGegruendet");
  
  // Die Stadt bekommt als Gueter das, was der Siedler/Stadtgruender
  // dabei hatte.
  attrib.setzen("ANFANGSGUETER",lager.to_string());
  
  // Die Stadt bekommt soviele Einwohner, als sovielen Einwohner die
  // Gruendereinheit bestand.
  attrib.setzen("EINWOHNERZAHL", attribut("EINWOHNER"));

  neue_stadt = objekt_schaffen(abkuerzung,"STADT",attrib.to_string());

  // Uebereignen muss ich die Stadt natuerlich dem Staat!
  geben(staat(), neue_stadt);

  // Jetzt fuehre ich noch das Tagebuch weiter
  tagebuch_fuehren(L("SG","FC") ,", ");
  tagebuch_fuehren(name_der_stadt,"");

  // Durch das Gruenden einer Stadt loest sich die Einheit auf.
  zur_vernichtung_vormerken();

  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_eingraben()
  * 
  * Realisiert den Befehl 'EI', mit dem sich eine militaerische Boden-
  * einheit befestigen kann.
  *
  * Ausserdem wird sie von Atomwaffen gebraucht, um sich in ein Silo
  * zu laden.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_eingraben()
{
  if (attribut_gesetzt("Kernwaffe")) { // Andere Bedeutung
    OBJEKT *silo = weltbaut_hier();
    if (!silo || !silo->attribut_gesetzt("Silo")) {
      fehlermeldung2(L("EI: Kernwaffen brauchen ein Silo, um stationiert zu werden.\n","DI: Nuclear missiles need a Rocket Launcher first.\n"));
      return 1;
    }
    else if (silo->attribut_gesetzt("Unversorgt")) {
	fehlermeldung2(L("EI: Das Silo ist nicht versorgt. Erst, wenn die Versorgung ","DI: Rocket Launcher not supplied. Try again ")
		       L("wieder steht, kann es eine Rakete einlagern.\n","when supplied.\n"));
	return 1;
    }

    if (befehl_dauert_noch(DA_EINGRABUNG)) return 0;

    if (silo->kommando("RAKETE_STATIONIEREN",this))
    {
      fehlermeldung2(L("EI: In diesem Silo ist schon eine Rakete stationiert, und es hat nur eine Platz.\n","DI: Well, there is already a missile loaded and I´m afraid there is only space for one.\n"));
      return 1;
    }
    
    char string[200];
    sprintf(string, L("Die Kernwaffe wird im Silo %s stationiert","Missile loaded into Launcher %s "),silo->name);
    tagebuch_fuehren(string, ", ");
    zur_vernichtung_vormerken();
    return 1;
  }
 
  if (attribut_gesetzt("Eingegraben")) {
    fehlermeldung2(L("EI: Die Einheit ist bereits befestigt!\n","DI: Unit is already dug in!\n"));
    return 1;
  }

  if (!attribut_gesetzt("Festungsturm")) 
  {
      if (!attribut_gesetzt("Militaerisch")) {
	  fehlermeldung2(L("EI: Nur milit~arische Einheiten k~onnen sich befestigen!\n","DI: Only military unit may dig in!\n"));
	  return 1;
      }

      if (!verteidigungskraft) {
	  fehlermeldung2(L("EI: Ohne Verteidigungsbonus n~utzt das Befestigen gar nichts!\n","DI: Pointless when unit has no D-Bonus!\n"));
	  return 1;
      }
  
      if (!attribut_gesetzt("Boden")) {
	  fehlermeldung2(L("EI: Nur Bodeneinheiten k~onnen sich befestigen!\n","DI: Only ground-based units may dig in.\n"));
	  return 1;
      }

  }

  if (!ort_ist_welt()) {
    fehlermeldung2(L("EI: Im Konvoi ist das v~ollig unm~oglich.\n","DI: Completely senseless while in a convoy. \n"));
    return 1;
  }

  if (befehl_dauert_noch(DA_EINGRABUNG)) return 0;
  attribut_setzen("Eingegraben");
  if (attribut_gesetzt("Festungsturm"))
  {
      attribut_setzen("Militaerisch");
      tagebuch_fuehren(L("EI (befestigt!)", "DI ("), ", ");
  }
  else
  {
      tagebuch_fuehren(L("EI","DI"), ", ");
  }
  return 1;
}

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_ausgraben()
{
  if (!attribut_gesetzt("Eingegraben")) {
    fehlermeldung2(L("AU: Wir sind gar nicht befestigt!\n","DO: How?! We are not dug in!\n"));
    return 1;
  }

  if (befehl_dauert_noch(DA_AUSGRABUNG)) return 0;
  attribut_loeschen("Eingegraben");
  tagebuch_fuehren(L("AU","DO"), ", ");
  if (attribut_gesetzt("Festungsturm"))
  {
      attribut_loeschen("Militaerisch");
  }
  return 1;
}

/**---------------------------------------------------------------------------
  * EINHEIT::befehl_in_reserve()
  * 
  * Veranlasst eine militaerische Einheit dazu, in Reserve zu gehen,
  * woraufhin sie keinen Unterhalt mehr benoetigt. Allerdings ist sie
  * dann nicht mal mehr faehig, sich zu verteidigen.
  * Flugzeuge koennen nur in Reserve gehe, wenn sie auf einer Basis
  * sind.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_in_reserve()
{
  if (attribut_gesetzt("Reserve")) {
     fehlermeldung2(L("RV: Wir befinden uns bereits bereits in Reserve!\n","DM: Already demobilized! \n"));
     return 1;
  }

  if (!attribut_gesetzt("Militaerisch")) {
     fehlermeldung2(L("RV: Nur milit~arische Einheiten k~onnen Reserve gehen!\n","DM: Only miltary units may demobilize!\n"));
     return 1;
  }

  if (attribut_gesetzt("Luft")) {
    if (basis_hier() == 1) { // mobile Basis
      fehlermeldung2(L("RV: Um in Reserve zu gehen, m~ussen wir landen oder","DM: To demobilize we must either land or be near a "));
      fortsetzung(L(" uns bei einer Stadt befinden.\n"," town.\n"));
      return 1;
    }
    
    else if (!basis_hier()) { // Gar keine Basis da.
      fehlermeldung2(L("RV: Wir k~onnen nur dort in Reserve gehen, wo wir landen k~onnen.","DM: We can only demobilize where we can land."));
      return 1;
    }
  }  

  if (befehl_dauert_noch(DA_RESERVE)) return 0;
  tagebuch_fuehren(L("RV","DM"), ", ");
  attribut_setzen("Reserve");
  return 1;
}

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_mobil_machen()
{
  if (!attribut_gesetzt("Reserve")) {
     fehlermeldung2(L("MO: Wir sind bereits mobil.\n","MO: We are already mobile! \n"));
     return 1;
  }

  if (befehl_dauert_noch(DA_MOBILISIERUNG)) return 0; // Dauert eben ein bischen...

  attribut_loeschen("Reserve");
  tagebuch_fuehren(L("MO","MO"), ", ");
  return 1;
}

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_aufloesen()
{
  tagebuch_fuehren(L("XX","DU"), ", ");
  enthaltenen_einheiten_berichten(L("Unsere Transporteinheit l~ost sich auf!","Our transport unit is disbanded! "));
  zur_vernichtung_vormerken();
  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_reisen()
  * 
  * Realisiert den zentralen Befehl 'RE'.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_reisen(char *befehl, long phase)
{
  // Es ist erlaubt, beim Reisen mehrere Richtungen durch Kommata getrennt
  // anzugeben. Ich kontrolliere, ob das hier der Fall ist. Wenn ja, dann
  // hole ich die erste Richtung und schiebe einen entsprechenden Reise-
  // befehl vor. Den restlichen Befehl wandle ich dann so um, dass die
  // erste Angabe fehlt.

  char *befdup = mystrdup(befehl);
  char *komma = befdup;
  while (*komma && *komma != ',') komma++;
  if (*komma) { // Komma gefunden.
    *komma=0;
    char *neuer_befehl = NULL;
    mystradd(neuer_befehl, L("RE","MV"));
    mystradd(neuer_befehl, komma+1); // Restliche Richtungen.

    befehl_umwandeln(neuer_befehl); // Erst umwandeln...
    befehl_vorschieben(befdup); // ... dann vorschieben!

    myfree(neuer_befehl);
    myfree(befdup);
    return -1;
  }
  myfree(befdup);


  char *richtungsname = befehl+2;

  // Wenn sie Einheit nicht mobil ist, dann schiebe ich einen Befehl vor,
  // damit sie sich mobil macht.

  if (attribut_gesetzt("Reserve")) {
    befehl_vorschieben(L("MO","MO"));
    return -1;
  }

  // Wenn die Einheit befestigt ist, dann schiebe ich einen Befehl vor, damit
  // sie sich automatisch ausgraebt.

  if (attribut_gesetzt("Eingegraben")) {
    befehl_vorschieben(L("AU","DO"));
    return -1;
  }

  // Und wenn sie sich im Konvoi befindet, dann mache ich aus dem Reisen-
  // befehl einen Disembark-Befehl.

  if (ort_ist_einheit()) {
    char befstring[60];
    sprintf(befstring, L("DE%s","DE%s"), befehl+2);
    befehl_umwandeln(befstring);
    return -1;
  }

  // Jetzt muss ich die Adresse des Zielfeldes bestimmen

  ADR ziel = zielfeld_bestimmen(richtungsname);

  // Gleiches Feld?
  
  if (ziel == adresse) {
    fehlermeldung1(L("Falsche oder fehlende Richtungsangabe.\n","Wrong or missing direction.\n"));
    return 1;
  }

  // Nun pruefe ich, ob eine Bewegung moeglich ist. D bedeutet Gegner im
  // Weg. In so einem Fall warte ich nur

  char *grund = pruefe_bewegung_zu_feld(ziel);
  if (grund && *grund!='D') { // Geht also nicht...
    fehlermeldung1(grund+1);
    return 1;
  }

  // Wenn ein Gegner im Weg steht, dann warte ich, loese aber zugleich einen
  // Alarm aus, damit die Strategie ZIELSTREBIG den Gegner angreift.
  if (grund && *grund=='D') {
    alarmiert_werden(A_FEIND_IM_WEG, richtungsname);
    return 0; // Warten auf Einheit ist besser als Fehlermeldung
  }

  // Wenn gerade ein Angriff auf mich im Gange ist, dann kann ich mich
  // nicht bewegen. Ich muss noch kurz warten.
  
  if (attribut_gesetzt("ANGRIFF_FOLGT")) return 0;

  // Die Bewegungspunkte der Einheit selbst ist das Minimum an Punkten,
  // das sie benoetigt, um sich ein Feld fortzubewegen. Diagonal und
  // Orthogonal wird hier noch nicht beruecksichtigt. Flugzeuge bleiben
  // von Gelaendeunterschieden unbeeinflusst. Ebenso Mechs.

  bool mech = attribut_gesetzt("Mechanoid");
  long faktor = 1;
  if (!attribut_gesetzt("Luft") && !mech) {
    faktor = myatol(welt()->gelaendeform_attribut(ziel, "BEWEGUNG"));
    if (!faktor) faktor = 1;
  }
  long dauer = bewegungspunkte_pro_feld * faktor;

  // Wenn auf dem Zielfeld eine Strasse ist, dann halbiert sich die Dauer.
  // (abgerunden, jedoch mindestens 2). Bei Flugzeugen nicht!

  if (!attribut_gesetzt("Luft") && !mech) {
    if (welt()->feld_attribut(ziel, FELD_ATTRIBUT_STRASSE)) dauer = (dauer+1)/2;
    if (dauer < 2) dauer = 2;
  }

  if (befehl_dauert_noch(dauer)) return 0;

  // Nun kann ich die Einheit endlich bewegen..

  if (bewegen(ziel)) { // Fehler beim Bewegen. Das kann doch gar nicht sein!?
      log('I', "EINHEIT::befehl_reisen()  Fehler beim Aufruf von bewegen()");
      return 1;
  }

  // So. Das Objekt befindet sich nun auf einem neuen Feld (Endlich!).
  // Nun trage ich die Aktion RE in das Logbuch ein.
  // RIC ric(richtungsname); // Einheitlich in Grossbuchstaben.

  tagebuch_fuehren(befehl,", ");

  // Ausserdem sorge ich dafuer, dass mein aktueller Objektbericht
  // im Logbuch aktualisiert wird.

  sichtungen_ins_tagebuch(phase);

  // Nun muss noch mein Lanschaftsabbild aktualisiert werden.

  gelaende_aktualisieren();

  return 1; // Zug Befehl fertig ausgefuert.
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_ansiedeln()
  * 
  * Dient ausschliesslich dazu, die Bevoelkerung, die in einer Einheit
  * steckt und bei deren Bau einer Stadt entzogen wurde, wieder in
  * eine Stadt einzufuegen, wobei sich die Einheit aufloest. Die
  * Einwohner einer Einheit sind im Attribut EINWOHNER gespeichert.
  *
  * @return
  * Wie bei Befehlen ueblich. Moeglicherweise wird ein BE-Befehl vor-
  * geschoben.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_ansiedeln()
{
  // Das Attribut EINWOHNER gibt an, wieviele Einwohner die Einheit bein-
  // haltet.

  long einwohner = myatol(attribut("EINWOHNER"));

  // Zum Ansiedeln muss sich die Einheit bei einer Stadt befinden.

  STADT *stadt = stadt_hier();
  if (!stadt) {
    fehlermeldung2(L("AS: Um uns in einer Stadt anzusiedeln, m~ussen wir uns dort befinden.\n","SE: To settle down in a town, we have to be in one!\n"));
    return 1;
  }

  // Nur fuer freundliche Staedte moeglich.
  if (!uns_liebt(stadt)) {
    fehlermeldung2(L("Wir k~onnen uns nur in eigenen oder zumindest freundlich","We can only settle down in our own or in"));
    fortsetzung(L("gesinnten St~adten ansiedeln.\n","allied towns.\n"));
    return 1;
  }

  // So. Nun mache ich noch einen Abschlussreport und teile der Stadt mit,
  // dass sie Einwohner hinzubekommt. Anschliessend loese ich mich auf.

  if (einwohner) stadt->kommando("ANSIEDLUNG", &einwohner);
    
  tagebuch_fuehren(L("AS","SE"),", ");
  enthaltenen_einheiten_berichten(L("Unsere Transporteinheit siedelt sich an. Wir m~ussen uns aufl~osen.","Our transport unit settles down. We have to disband."));
  zur_vernichtung_vormerken();
  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_angreifen()
  * 
  * Realisiert den Befehl "Angreifen", den eine Einheit erteilt be-
  * kommen kann.
  * Realisiert ausserdem den Befehl "Kapern", der wie das Angreifen
  * funktioniert, wo aber die gekaperte Einheit nicht vernichtet 
  * wird, sondern uebernommen.
  *
  * Einheiten von staaten, die man als EF (endgueltig freundlich) ein-
  * gestuft hat, kann man nicht angreifen!
  *
  * @param
  * befehl:         AN, ANNO, ANO, ANS, ...
  * KP, KPNO, KPO, KPS, ...
  * Richtungsangabe des Angriffs in Form eines Strings.
  * Im Prinzip ist diese Richtung ein Vektor und kann
  * auch ueber eine Distanz von mehreren Feldern gelten.
  *
  * Auch von Erobern wird diese Funktion aufgerufen.
  * befehl faengt dann mit "EO" an.
  * 
  * @return
  * 3, falls der Angriff erfolgreich war, aber noch Gegner da sind.
  * 2, falls der Angriff nicht noetig war, oder alle Gegner besiegt sind.
  * 1, falls der Angriff nicht moeglich war.
  * 0, falls der phasenzaehler noch zu niedrig ist.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_angreifen(char *befehl)
{
  short kapern = (befehl[0] == L('K','C'));

  // In manchen Staatsformen darf man nicht kapern ...
  if (kapern && einfluss_vorhanden("KPEN")) {
    fehlermeldung1(L("Das Kapern von Zivileinheiten ist weder in der Demokratie","Capturing civil units is neither allowed in a democracy nor in a "));
    fortsetzung(L(" noch in der Technokratie erlaubt!\n"," technocracy!\n"));
    return 1;
  }

  // ... bzw. nicht mal angreifen
  if (!kapern && einfluss_vorhanden("ANEOFE")) {
    fehlermeldung1(L("In der Technokratie ist keine aggressive Handlung ","No agressive actions possible under a technocratic"));
    fortsetzung(L("m~oglich!\n","government!\n"));
    return 1;
  }

  // Es koennen nur Militaerische Einheiten Angreifen.
  if (!attribut_gesetzt("Militaerisch")) {
     fehlermeldung1(L("Zivile Einheiten k~onnen nicht angreifen!\n","Civil units cannot attack!\n"));
     return 1;
  }

  // Schiffe k~onnen an Land nicht kaempfen
  else if (attribut_gesetzt("See") && !mystrcmp(gelaende_attribut("ART"),"Boden")) {
    fehlermeldung1(L("Schiffe k~onnen nur auf See angreifen.\n","Ships can only attack while at sea.\n"));
    return 1;
  }

  // Flugzeuge koennen nicht kapern.
  else if (kapern && attribut_gesetzt("Luft")) {
    fehlermeldung1(L("Flugzeuge k~onnen keinen anderen Einheiten kapern.\n","Aircrafts cannot capture other units.\n"));
    return 1;
  }

  // Ist die Einheit eingegraben, so kann sie auch nicht angreifen.
  if (attribut_gesetzt("Eingegraben")) {
     befehl_vorschieben(L("AU","DO")); // Automatisch ausgraben
     return -1; // Diesen Befehl zurueckstellen.
  }

  // So. Die Einheit muss natuerlich auch noch mobil sein.

  if (attribut_gesetzt("Reserve")) {
     fehlermeldung1(L("Wir befinden uns in Reserve sind nicht kampfbereit!\n","We are demobilized and unable to fight!\n"));
     return 1;
  }

  // Wenn sich die Einheit nicht in der Welt befindet, dann kann sie auch
  // nicht angreifen

  if (!ort_ist_welt()) {
    fehlermeldung1(L("Zum Angreifen d~urfen wir uns nicht im Konvoi befinden.\n","Impossible while in a convoy.\n"));
    return 1;
  }

  // Jetzt bestimme ich das Zielfeld
  ADR ziel;
  char *fehler = staat()->ortsangabe_auswerten(befehl+2, adresse, ziel);
  if (fehler) {
    fehlermeldung1(fehler);
    return 1;
  }

  if (!landschaft()->benachbart(adresse, ziel)) {
      fehlermeldung1(L("Du kannst nur gerade oder schräg benachbarte Felder angreifen.\n","You can only attack adjacent squares.\n"));
      return 1;
  }
  
  // Landeinheiten koennen keine Seefelder angreifen.
  
  char *art = welt()->gelaendeform_attribut(ziel,"ART");

  if (!mystrcmp(art, "See") && !attribut_gesetzt("See")
	 && !attribut_gesetzt("Luft")) {
    fehlermeldung1(L("Als Landeinheit k~onnen wir keine Seefelder angreifen.\n","We cannot attack squares at sea.\n"));
    return 1;
  }

  // Jetzt sollte ich zumindest noch schauen, ob sich auf dem Zielfeld
  // ein Objekt befindet, dass ich angreifen kann. Falls nicht gaebe
  // es zwei Strategien: Warten, bis ein Objekt in die Naehe kommt,
  // dass ich angreifen koennte (d.h. 0 zurueckgeben), oder sagen, dass
  // sich auf dem Feld kein Objekt befindet und 1 zurueckgeben, d.h. den
  // Befehl abbrechen, ohne dass Zeit verloren geht fuer den Angreifer
  // Momentan entscheide ich mich fuer die 2. Strategie.

  // Ich hole mir also von der Landschaft die Liste aller gegnerischen
  // Einheiten, die sich auf dem Zielfeld befinden...

  DOUBLIST *gegnerliste =
      landschaft()->alle_gegner_bei_adresse(staat(), ziel);

  // Jetzt loesche ich mich selbst aus der Liste! Ausserdem loesche ich die
  // Objekte, die ich nicht angreifen kann. Das sind alle Einheiten vom
  // eigenen Staat und von Staaten, die ich als EF eingestuft habe.
  // Ausserdem kann ich als Bodeneinheit nur Flugzeuge
  // angreifen, die in Reserve sind. Schiffe kann ich als Bodeneinheit
  // jederzeit angreifen, wenn sie in meiner Naehe sind. Erinnere dich daran,
  // dass ich oben schon ausgeschlossen haben, dass eine Landeinheit ein
  // Seefeld angreift.

  short bin_grund = attribut_gesetzt("Boden") || attribut_gesetzt("See");
  
  OBJEKT_LIST_NODE *objnode;
  SCAN(gegnerliste, objnode)
  {
    OBJEKT *objekt = objnode->objekt;
    if (   objekt == this || staat()->besitzt(objekt)
	|| (bin_grund && objekt->attribut_gesetzt("Luft")
		      && !objekt->attribut_gesetzt("Reserve"))
	|| (((STAAT *)staat())->endgueltig_freundlich_gegenueber(objekt)) )
    {
      delete objnode;
      FIRST(gegnerliste, objnode);
    }
    else NEXT(objnode);
  }  

  // Falls kein Gegner in der Naehe ist, kann ich den Angriffsbefehl erfolg-
  // reich beenden.

  if (gegnerliste->is_empty())
  {
    delete gegnerliste;
    // Einen Eintrag ins Tagebuch mache ich nur, wenn es sich um den
    // echten Angriffsbefehl handelt. Wurde diese Funktion von EO aufgerufen,
    // dann darf ich es nicht machen.
    if (!mystrncmp(befehl, L("AN","AT"), 2)
	|| !mystrncmp(befehl, L("KP","CP"), 2))
    {
	tagebuch_fuehren(befehl, ", ");
	tagebuch_fuehren(L(": kein Gegner",": no opponent"),"");
    }
    return 2; // 2 bedeutet: Feld leergeraeumt oder Feld war schon leer.
  }

  // Ach ja. Und bevor es dann losgeht, sortiere ich die Liste der Gegner
  // in der Reihenfolge ihrer Verteidigungskraft. Ich muss naemlich laut Regeln
  // den Staerksten zuerst angreifen.

  nach_verteidigungskraft_sortieren(gegnerliste);

  // Nun bekommen alle Opfer ein Kommando, dass ihnen mitteilt, dass sie
  // bald angegriffen werden. In diesem Kommando wird dann ein Alarm
  // ausgeloest werden und ferner ein Zaehler gesetzt, der die Bewegungs-
  // befehle zwei Runden lang suspendiert

  FOR_EACH_OBJEKT_IN (gegnerliste)
  DO (
    objekt->kommando("ANGRIFF_FOLGT",(void *)this);
  )

  // Nun hole ich mir mein erstes Opfer und loesche die Liste wieder.
  // Fuer weitere Gegner schiebe ich einfach den gleichen Befehl noch-
  // mal vor.

  EINHEIT *opfer =
	 (EINHEIT *)((OBJEKT_LIST_NODE *)gegnerliste->first())->objekt;

  long anzahl_gegner = gegnerliste->number_of_elements();
  delete gegnerliste;

  // Manche Einheiten kann man nicht kapern.
  
  if (kapern && opfer->attribut_gesetzt("NichtKaperbar"))
  {
    fehlermeldung1(L("Man kann ","You cannot capture "));
    fortsetzungp(L("%s nicht kapern.\n","%s.\n"), konjugation(opfer->a_gattung(), 
    	AKKUSATIV | SINGULAR));
    return 1;
  }

  // Meine Angriffskraft ermittle ich durch eine Funktion, die auch
  // eventuelle Unterstuetzung von anderen meiner Truppen miteinbe-
  // rechnet. Beim Kapern negiere ich meine Angriffskraft als Zeichen
  // fuer das Opfer. Es muss wissen, ob es vernichtet oder gekapert
  // werden soll.

  long kraft = effektive_angriffskraft(ziel) * (kapern ? -1 : 1);

  // Wenn ich normal angreife, kann ich Einheiten ueberrennen, wenn
  // ich die Uebermacht habe. Dann dauert das Angreifen 0 Phasen.

  bool ueberrannt = false;

  // Ueberrennen ist momentan ausser Kraft gesetzt!

//  if (kapern || !opfer->kann_ueberrannt_werden(kraft, this))
//  {
      // An dieser Stelle entscheidet sich nun, wieviele Runden ein Angriff
      // dauert und ob das von dem Typ der Einheit abhaengt oder nicht.
      if (befehl_dauert_noch(DA_ANGRIFF)) return 0;
//  }
//  else ueberrannt = true;

  // So. Jetzt ist es soweit. Die Morgensterne sausen nieder.
  // Jetzt schicke ich dem Opfer die Mitteilung, dass es von mir
  // soundsoviel Angriffsenergie auf seinem Feld erwarten kann.
  // Was es damit anfaengt, ist mir eigentlich relativ egal.

  long rwert = opfer->kommando("ANGRIFF", &kraft, this);
  staat()->kommando("ANGRIFF_ZAEHLEN"); /* Fuer die Statistik */
  anzahl_kaempfe ++;
  
  // Ich mache den Report bevor ich auswerte, damit die Meldungen in der
  // richtigen Reihenfolge kommen.

  char string[80];
  sprintf(string,L("%s mit %ld gegen %ld: ","%s with %ld versus %ld: "), befehl, ABS(kraft), ABS(rwert));
  tagebuch_fuehren(string, ", ");

  if (rwert > 0) // verloren.
  {
      sprintf(string, L("Der Angriff gegen %s scheitert","The attack against %s fails"),
	    konjugation(opfer->a_gattung(), AKKUSATIV | SINGULAR));
      tagebuch_fuehren(string, "");
    
      // Falls ich als Lufteinheit Nicht-Luft-Ziele angreife, kann ich nicht
      // zerstoert werden.

      if (!attribut_gesetzt("Luft") || opfer->attribut_gesetzt("Luft"))
      {

	  // Wenn mir ein Hauptquartier hilft, dann werde ich vielleicht nicht
	  // vernichtet.

	  if (einfluss_vorhanden("HQ") && einfluss_vorhanden("HQmoeglich")) {
	      if (io_random(2) == 1) { // Gerettet!
		  tagebuch_fuehren(L(", aber wir k~onnen uns zur~uckziehen",", but we manage to retreat"),"");
	      }
	      else {
		  tagebuch_fuehren(L(", ein R~uckzug gelingt uns nicht mehr ",", we cannot retreat ")
				   L("und wir werden vernichtet","and are snuffed"),"");
		  enthaltenen_einheiten_berichten(L("Unsere Transporteinheit wird ","Our transport unit did not survive ")
						  L("beim Angriff vernichtet!","the ferocious attack!"));
		  zur_vernichtung_vormerken();
		  enthaltene_objekte_vernichten();
	      }
	  }
     
	  else {  
	      tagebuch_fuehren(L(", wir werden vernichtet",", we are slain"),"");
	      enthaltenen_einheiten_berichten(L("Unsere Transporteinheit wird ","Our transport unit ")
					      L("beim Angriff vernichtet!","did not survive the attack!"));
	      zur_vernichtung_vormerken();
	      enthaltene_objekte_vernichten();
	  }
      }
      if (zur_vernichtung_vorgemerkt()) // Verteidiger bekommt eine Medaille
      {
	  opfer->medaille_bekommen();
      }
  } // Angriff hat nicht geklappt.

  // Was passiert nun, wenn der Angriff klappt?

  else {
      anzahl_gegner --;
      if (!kapern) { // Angriff klappt.
	  tagebuch_fuehren("[", "");
	  if (ueberrannt) tagebuch_fuehren(L("] ~uberrannt","]  overrun"), opfer->a_gattung());
	  else tagebuch_fuehren(L("] vernichtet","] destroyed"), opfer->a_gattung());
	  medaille_bekommen();
      }
    
      else // Einheit gekapert
      {
	  // Und nun hier ein Trick. Meine Stadt uebernimmt das gekaperte Teil nicht,
	  // sondern macht sich ein Abbild davon. Dazu bereite ich Namen und Attribute
	  // vor.
	  
	  OBJEKT *opfer_staat = opfer;
	  while (mystrcmp(opfer_staat->a_typ(),"STAAT"))
	      opfer_staat = opfer_staat -> besitzer;

	  EINHEIT *neu = (EINHEIT *)objekt_schaffen(
	      besitzer->eindeutiger_name(),"EINHEIT", opfer->attribute.to_string());
	  neu->lager.setzen_auf(opfer->lager);
	  geben(besitzer, neu);
	  neu->ort_wechseln(opfer->ort(), opfer->adresse);

	  char string[500];
	  sprintf(string,L("(Diese Einheit haben wir von %s <%s> gekapert)","(We captured the unit from %s <%s> )"),
		  opfer_staat->a_name(), opfer_staat->name);
	  neu->tagebuch_fuehren(string,"");
    
	  // Alle Objekte, die das gekaperte Dinge enthalten hat, muessen
	  // in das neue gesteckt werden. Ausserdem bekommt sie ebenfalls
	  // mein Besitzer.
      
	  FOR_EACH_OBJEKT_IN (opfer->alle_enthaltenen_objekte())
	      DO_AND_DELETE ({
		  objekt->attribut_setzen("Transport_gekapert"); /* wird sonst nicht */
		  objekt->ort_wechseln(neu);                      /* aufgenommen */
		  objekt->attribut_loeschen("Transport_gekapert");
		  geben(besitzer, objekt);
		  objekt->alle_befehle_abbrechen();
	      }) ;

	  opfer->zur_vernichtung_vormerken();
	  
	  // Tagebucheintrag und Schluss.
	  tagebuch_fuehren("[", " ");
	  tagebuch_fuehren(L("] gekapert","] captured"), opfer->a_gattung());
	  medaille_bekommen();
      }
  }


  if (anzahl_gegner) return 3;
  else return 2;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_feld_erobern()
  * 
  * Der Befehl EO (Feld erobern) fuehrt solange Angriffsbefehle aus,
  * bis das Feld leergeraeumt ist und versucht anschliessend, mit einem
  * RE-Befehl dorthin zu reisen.
  * @param
  * Wie beim AN-Befehl eine Richtungsangabe
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_feld_erobern(char *befehl, long phase)
{
    // Ich rufe einfach den AN-Befehl auf. Dieser teilt mir im Ergebnis mit,
    // ob und warum der Befehl beendet wurde.

    while (true) {
	switch (befehl_angreifen(befehl)) {
	case -1: return -1; // Befehl wurde vorgeschoben!
	case 0: return 0; // Befehl noch nicht drangekommen
	case 1: return 1; // Befehl nicht moeglich
	case 2: // Keine Gegner mehr da. Ich versuche zu reisen.
	    return befehl_reisen(befehl, phase);
	}
	// Die dritte Moeglichkeit ist 3: Ein Gegner besiegt, weitere
	// sind aber noch da. Ich wiederhole, bis das Feld leer ist.
    }
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_wiederhole()
  * 
  * Dauerbefehl, der immer eine Zahl von Befehlen vorschiebt.
  * @param
  * char * "WI%s", wobei %s = Befehl1/Befehl2/.../Befehln
  * z.B. WIGZSTA10/BL50M/GZSTA12/BL50M/GZTAR/EL
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_wiederhole(char *befehl, long runde)
{

  // Wenn der letzte WI - Befehle in der gleichen Phase ausgefuert wurde,
  // dann breche ich ab.
  
  long letzter_wi = myatol(attribut("WI_PHASE"));
  if (letzter_wi == runde) { // Abbrechen
    fehlermeldung1(L("Der Wiederhole-Befehl wird abgebrochen, da die ","The repeat command is canceled because the commands "));
    fortsetzung(L("wiederholten Befehle keine Zeit beanspruchen.\n","don´t take any time.\n"));
    attribut_loeschen("WI_PHASE"); // Wegen evtl. naechstem WI-Befehl.
    return 1;
  }
  attribut_setzen("WI_PHASE", myltoa(runde));

  // Ich mache nichts anderes, als die Befehle vorzuschieben, die als
  // Parameter angegeben sind. Der Befehl WI selbst wird nie geloescht.
  // Ich muss die Befehle aber von hinten her einlasten!! Es gilt naemlich
  // das Stapelprinzip!
  
  char puffer[512];
  strcpy(puffer, befehl+2);
  char *scan = puffer+strlen(puffer); // Zeigt nun auf 0.
  // short eingelastet = 0; // Bisher noch kein Befehl eingelastet.

  while (scan > puffer)
  {
    if (*(scan-1) == '/') { // Trennzeichen am Ende abkappen.
      scan--;
      *scan = 0;
      continue;
    }
    
   while (scan > puffer && *(scan-1) != '/') scan--;

   // Assert: scan==puffer oder scan steht eins nach '/', also auf jeden
   // Fall am Anfang eines einzulastenden Befehls.

  if (mystrncmp(scan, L("WI","RP"),2)) { // Keine Wiederholebefehle schachteln!
    befehl_vorschieben(scan);
  }
   
   // Wenn noch weitere Befehle kommen, dann
   // ist links ein '/', das durch ein Stringende ersetzt wird.
  }

  return -1; // Diesen Befehl nicht loeschen, Phasenzaehler lassen, weitermachen
}
    

/**---------------------------------------------------------------------------
  * EINHEIT::befehl_embark()
  * 
  * Befehl zum Einsteigen in ein Schiff oder anderes Transportmittel.
  * Es kann auch eine Richtung mitangegeben werden, wenn das Schiff
  * auf einem benachbarten Feld steht. Flugzeuge koenne nur auf Ein-
  * heiten mit dem Attribut 'Basis' landen. Ausserdem kann ich auf
  * Einheiten von freundlichen Staaten embarken.
  *
  * Als Parameter kann auch die Abkuerzung einer Einheit angegeben
  * werden.
  * @param
  * char *befehl:  EM%s Mit Richtung oder "" oder Einheitenabk.
  * @return
  * Wie ueblich.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_embark(char *befehl, long phase)
{
  
  if (ort_ist_einheit()) { // Direktes Umsteigen ist nicht moeglich.
    fehlermeldung1(L("Ein direktes Umsteigen zwischen zwei Transporteinheiten","Changing from one transport unit to another is not"));
    fortsetzung(L("ist nicht m~oglich. Wir befinden uns bereits im Konvoi.\n","possible and we are already in a convoy.\n"));
    return 1;
  }
  else if (!ort_ist_welt()) {
     fehlermeldung1(L("Um an Bord zu gehen, m~ussen wir uns au~serhalb befinden!\n","We have to be out of it to get into it!\n"));
     return 1;
  }
  // Ich muss eine Landeinheit sein!!!
  if (attribut_gesetzt("See")) {
    fehlermeldung1(L("Nur Landeinheiten und Flugzeuge k~onnen embarkieren.\n","Only ground-based units and aircrafts can embark.\n"));
    return 1;
  }

  // Es ist moeglich, eine Einheitenabkuerzung anzugeben. Ich schaue mal,
  // ob es ein Einheit gemeint ist.

  OBJEKT *ziel_einheit = objekt_mit_namen(befehl + 2);
  if (ziel_einheit && !ziel_einheit->typ_ist("EINHEIT"))
  {
    fehlermeldung1(L("Die Abk~urzung hinter dem EM-Befehl mu~s einer Einheit geh~oren\n","The abbreviation after the EM-command must belong to a specified unit.\n"));
    return 1;
  }

  // Es ist moeglich, aber nicht obligatorisch, beim EM-Befehl eine Richtung
  // mitanzugeben. Bei Vorhandensein einer Richtung zeigt diese auf das
  // Schiff.

  ADR ziel;

  if (!ziel_einheit) {
    char *richtungsstring = befehl+2;
    if (*richtungsstring) ziel = zielfeld_bestimmen(richtungsstring);
    else ziel = adresse;
  }
  
  // Schon jetzt muss ich die Befehlsdauer pruefen...
  if (befehl_dauert_noch(DA_EMBARKATION)) return 0;

  // Wenn gerade ein Angriff auf mich im Gange ist, dann kann ich mich auch
  // nicht bewegen. Ich muss noch kurz warten.
  
  if (attribut_gesetzt("ANGRIFF_FOLGT")) return 0;

  // Als naechstes muss ich schauen, ob sich bei der gleichen Adresse
  // auch eine Einheit befindet, die das Attribut "CONVOY" besitzt.
  // Aber nur, wenn nicht ohnehin schon eine Zieleinheit spezifiziert
  // ist. Ansonsten siehe unten.

  short aufgenommen = 0;

  if (!ziel_einheit)
  {
    DOUBLIST *objliste;
    if (!ort_ist_welt()) // Dann war keine Richtung angegeben.
	    objliste = ort()->alle_enthaltenen_objekte("TYP=EINHEIT,CONVOY");

    else objliste = welt()->alle_objekte_bei_adresse(ziel,"TYP=EINHEIT,CONVOY");

    // Jetzt gehe ich alle Objekte durch und frage sie, ob sie noch Platz
    // haben. Das erste, dass noch Platz hat nimmt mich dann auf. Sollte
    // keines mehr Platz haben, dann wars nichts.

    /* Falls ich ein Flugzeug bin, dann kann ich nur in Einheiten mit
       dem Attribut Basis gehen */

    FOR_EACH_OBJEKT_IN (objliste) DO_AND_DELETE
    (
      if ( (!attribut_gesetzt("Luft") || objekt->attribut_gesetzt("Basis"))
           && !aufgenommen && !ort_wechseln(objekt)) aufgenommen = 1;
    )
  }

  else aufgenommen = 
           ( welt()->entfernung_zwischen(ziel_einheit->adresse, adresse) <= 1.5 
            && (!attribut_gesetzt("Luft") || ziel_einheit->attribut_gesetzt("Basis"))
  	    && !ort_wechseln(ziel_einheit) );

  if (aufgenommen)  {
    tagebuch_fuehren(befehl,", ");
  }    

  // Wenn ich nicht aufgenommen werde, dann warte ich einfach...

  if (!aufgenommen) {
    phasenzaehler = DA_EMBARKATION - 1; // Nicht wieder von vorne warten
    return 0;
  }
   
  sichtungen_aktualisieren(phase);
  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_disembark()
  * 
  * Dient zum Verlassen eines Transportvehikels, z.B. eines Schiffes.
  * Es kann gleichzeitig eine Richtung mitangegeben werden, wohin
  * dann gleich gereist wird. Dies ist z.B. beim Anlanden wichtig.
  *
  * @param
  * char *befehl:  DE%s Mit Richtung oder "".
  *
  * @return
  * Wie ueblich.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_disembark(char *befehl, long phase)
{
  if (!ort_ist_einheit()) {
    fehlermeldung1(L("Wir befinden uns nicht im Transport einer anderen Einheit.\n","We are not transported by another unit.\n"));
    return 1;
  }

  if (befehl_dauert_noch(DA_DISEMBARKATION)) return 0;

  // Falls das Transportmittel selbst irgendwo drinnen ist, dann ist
  // es einfach, und ich nehme einfach dessen ort an.

  if (!einheit()->ort_ist_welt()) {
     verlassen();
     tagebuch_fuehren(L("DE","DE"),", ");
     sichtungen_aktualisieren(phase);
     return 1;
  }

  // Aussteigen aus Flugzeugen ist etwas schwieriger.

  if (ort()->attribut_gesetzt("Luft") 
     && !ort()->attribut_gesetzt("Hubschrauber"))
  {
    if (mystrlen(befehl) > 2) {
      fehlermeldung1(L("Beim Aussteigen aus einem Flugzeug kann man keine Richtung angeben.\n","When disembarking an aircraft you cannot give a direction.\n"));
      return 1;
    }
    
    // Ausserdem muss eine Basis in der Naehe sein.
    if (!attribut_gesetzt("Fallschirm") && !(einheit()->basis_hier())) {
      fehlermeldung1(L("Das Flugzeug kann hier nicht landen.\n","The aircraft cannot land here.\n"));
      return 1;
    }
  }

  // Zielfeld bestimmen. Beim Befehl kann eine Richtung angegeben werden.
  // Ausgangspunkt ist die Adresse des Transportmittels.

  RIC ric(befehl+2);
  ADR ziel =
     (einheit()->welt()) -> adresse_in_richtung(einheit()->adresse, ric);

  // Und jetzt muss ich natuerlich noch pruefen, ob ich da auch hinkann.

  if (pruefe_bewegung_zu_feld(ziel)) // Feld nicht betretbar 
  {
    phasenzaehler = DA_DISEMBARKATION - 1; // Nicht wieder von vorne warten
    return 0;
  }

  // Jetzt ist alles klar. Ich verlasse die Einheit und bewege mich sofort
  // zum Zielfeld.

  if (ort_wechseln(ort()->ort(), ziel)) { // Huch?
      log('I', "EINHEIT::befehl_disembark(): Ortwechseln() hat nicht geklappt!");
      return 1;
  }

  tagebuch_fuehren(befehl,", ");
  sichtungen_aktualisieren(phase);
  gelaende_aktualisieren();
  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_umladen()
  * 
  * Befehle BL und EL, dem eine Einheit Resourcen umladen kann. Sie muss
  * sich dazu auf dem gleichen Feld wie das Partberobjekt befinden.
  * Das Partnerobjekt muss uns freundlich gesinnt sein.
  *
  * @param
  * char *befehl: EL23H:SIL1 oder EL:SIL3 oder EL40H oder EL oder
  * BL11S4N:HAF3 oder BL:SEP12 oder BL1F oder BL u.s.w.
  * @return
  * Wie bei Befehlen ueblich.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_umladen(char *befehl)
{
  short beladen = !mystrncmp(befehl,L("BL","LO"),2);
 
  // Fehlerabfragen schon im Vorfeld testen.

  if (beladen && !attribut_gesetzt("LAGER")) {
    fehlermeldung1(L("Wir haben keine M~oglichkeit, G~uter zu transportieren!\n","We are not able to transport commodities!\n"));
    return 1;
  }

  else if (!beladen && lager.ist_null()) {  // Nichts geladen, aber kein Fehler
    tagebuch_fuehren(L("EL (nichts)","UL (nothing)"),", ");
    return 1; // Nichts geladen.
  }

  // Jetzt hole ich die Parameter: 

  char resstring[MAX_LAENGE_RESOURCEZEILE+2];
  char objname[MAX_LAENGE_NAME+2];
  char *read,*write;

  // Bis vor den Doppelpunkt kommt die Resourceangabe

  read = befehl+2;
  write = resstring;
  while (*read && *read!=':') *write++ = *read++;
  *write = 0; // String beenden

  // Wenn ein Doppelpunkt kam, dann kommt noch ein Objektname.

  if (*read==':') read++;
  write = objname;
  while (*read) *write++ = *read++;
  *write = 0; // String beenden.

  // So. Nun passe ich den Ressourceparameter an. Wenn er Null ist, dann
  // setze ich ihn auf einen anderen Wert, je nach Befehl.

  RESOURCE_VEKTOR umladung(resstring);
  
  if (!*resstring && beladen) {
    // Dann nehme ich das maximale, was ich laden koennte.
    umladung.einheitsvektor_aus(attribut("LAGER"));
    umladung.multipliziere_mit(myatol(attribut("KAPAZ")));
  }
  else if (!*resstring && !beladen)  umladung.setzen_auf(lager);
    
  // Jetzt kommen einige Fehlermeldungen. Beachte, dass ich weder
  // die Kapazitaet fuer den Einladenbefehl, als auch das Lager fuer
  // den Entladenbefehl teste. Das werde ich spaeter bei einer Schleife
  // beruecksichtigen...

  if (umladung.ist_negativ()) { // Darf natuerlich niemals sein!!!
    fehlermeldung1(L("Beim Umladen von G~utern sind keine negativen ","No negative commodity values "));
    fortsetzung(L("Angaben von Ressourcen zugelassen!\n","allowed!\n"));
    return 1;
  }

  // Umladung nur im Freien, sonst wird's mir echt zu kompliziert
  if (!ort_ist_welt()) {
    fehlermeldung1(L("Das Umladen von G~utern ist nur im Freien m~oglich, jedoch nicht im Konvoi.\n","Transferring commodities is only possible off board, but not while in a convoy.\n"));
    return 1;
  }

  // Lufteinheiten koennen nur umladen, wenn eine Basis da ist.
  if (attribut_gesetzt("Luft") && !attribut_gesetzt("Hubschrauber")
      && !basis_hier()) {
    fehlermeldung1(L("Wir k~onnen nur dort umladen, wo wir landen k~onnen.\n","We can only transfer commodities where we also can land.\n"));
    return 1;
  }

  // Wenn er be- oder entladen will, und zwar gar nichts, dann gibt es keine
  // Fehlermeldung, und der Befehl dauert auch gar nicht. Es passiert einfach
  // gar nichts.
  
  if (umladung.ist_null()) {
    if (beladen) tagebuch_fuehren(L("BL (nichts)","LO (nothing)"),", ");
    else tagebuch_fuehren(L("EL (nichts)","UL (nothing)"),", ");
    return 1;
  }
  
  if (beladen) {
    // Nun muss ich schauen, ob ich das Zeug theoretisch Laden kann, von
    // der Zusammensetzung her. In meinem Attribut LAGER stehen die Kenn-
    // buchstaben der Ressourceklassen, die ich laden kann. Ich baue
    // einen maximalen Resourcevektor auf...

    RESOURCE_VEKTOR maximum;
    maximum.einheitsvektor_aus(attribut("LAGER"));
    maximum.multipliziere_mit(200000000L);

    if (!umladung.passt_in(maximum)) {
      fehlermeldung1(L("Wir k~onnen nur die Ressourcen ","We can only load the commodities "));
      char *p;
      for (p = attribut("LAGER"); *p; p++) {
	char string[60];
	sprintf(string,"%c%s", *p, *(p+1)
		? (*(p+2) ? ", " : L(" und "," and "))
		: L(" laden.\n",".\n"));
	fortsetzung(string);
      }
      return 1;
    }
  } // if (beladen)

  // Falls ich nicht belade, begrenze ich die Entladung auf das, was ich
  // im Lager ueberhaupt habe.

  else  umladung.begrenzen_auf(lager); // Mehr darf ich nicht ausladen

  // Ab hier muss ich die Dauer des Befehls beruecksichtigen, da die
  // weiteren moeglichen Fehler zeitabhaenging sind.

  if (befehl_dauert_noch(DA_BELADEN)) return 0; // Noch nicht fertig.

  // Im zweiten Schritt, der nun folgt, ermittle ich alle moeglichen Partner
  // fuer den Transfer. Dazu erstelle ich eine Liste von Objekten, die
  // auf dem gleichen Feld wie ich sind. Wurde beim Befehl ein Name an-
  // gegeben, so verfahre ich geringfuegig anders.

  OBJEKT *expliziter_partner = NULL;
  DOUBLIST partner; // Wird die Liste der Partnerobjekte.
  
  if (objname[0]) {
    expliziter_partner = objekt_mit_namen(objname);
    if (!expliziter_partner) {
      fehlermeldung1(L("Es gibt keine Stadt, Einrichtung oder Einheit mit der \n","There is no town, structure or unit with the \n"));
      fortsetzungp(L("Abk~urzung %s.\n","abbreviation %s.\n"), objname);
      return 1;
    }

    else if (expliziter_partner == this) {
      fehlermeldung1(L("Es macht keinen Sinn, wenn wir auf uns selbst umladen.\n","We cannot transfer commodities to ourselves.\n"));
      return 1;
    }
  
    else if (!(expliziter_partner->adresse == adresse)
       || expliziter_partner->ort()!=ort())
    {
      fehlermeldung1(L("Das Objekt ","The object "));
      fortsetzungp(L("%s befindet sich nicht auf unserem Feld.\n","%s is not available on this square.\n"),objname);
      return 1;
    }
    
    else if (!uns_liebt(expliziter_partner)) {
      fehlermeldung1(L("Der Besitzer diese Objektes ist uns nicht freundlich gesinnt.\n","The owner of that object is an opponent!\n"));
      return 1;
    }

    OBJEKT_LIST_NODE *neu = new OBJEKT_LIST_NODE;
    neu->objekt = expliziter_partner;
    partner.insert(neu);
  }

  else { // Alle moeglichen Partner holen
    // Die Reihenfolge der Auswahl der Partner ist Stadt/Weltbaut, Einheit
    DOUBLIST *staedte = welt()->alle_objekte_bei_adresse(adresse,"TYP=STADT");
    DOUBLIST *weltbauten = welt()->alle_objekte_bei_adresse(adresse,"TYP=WELTBAUT");
    DOUBLIST *einheiten = welt()->alle_objekte_bei_adresse(adresse,"TYP=EINHEIT");
    partner.merge(staedte);
    partner.merge(weltbauten);
    partner.merge(einheiten);
    delete staedte;
    delete weltbauten;
    delete einheiten;

    // Und nun muss ich mich selbst und alle nicht freundlich gesinnten
    // Einheiten und Objekte aus der Liste loeschen.
    
    OBJEKT_LIST_NODE *objnode;
    SCAN(&partner, objnode) {
      if (objnode->objekt==this || !uns_liebt(objnode->objekt)) {
	delete objnode;
	FIRST(&partner, objnode);
      }
      else NEXT(objnode);
    }
  }

  // Habe ich nun ueberhaupt einen Partner?
  if (partner.is_empty()) {
    fehlermeldung1(L("Hier ist keine Stadt, Einrichtung oder Einheit, mit der","There is no town, structure or unit with which we could "));
    fortsetzung(L(" wir G~uter austauschen k~onnen.\n"," swap commodities.\n"));
    return 1;
  }

  // Nun gehe ich die ganze Liste durch. Ich mache solange weiter, bis mein
  // Ziel erfuellt ist, oder meine Kapazitaet erschoepft. Ich summiere dabei
  // das Ergebnis der durchgefuehrten Transaktionen

  RESOURCE_VEKTOR gebucht;

  short kapaz = myatol(attribut("KAPAZ"));
  char infostring[80];

  FOR_EACH_OBJEKT_IN (&partner) DO
  (
    if (umladung.ist_null() || !((beladen && lager.betrag() < kapaz)
	|| (!beladen && lager.betrag())))  break;

    if (beladen) /* BL */
    {
      long maxbetrag = kapaz-lager.betrag();
      RESOURCE_VEKTOR bekommen(objekt->info("RESSOURCEN_ANFORDERN", umladung.to_string(), 
					this, &maxbetrag));
      umladung.subtrahiere(bekommen);
      lager.addiere(bekommen);
      gebucht.addiere(bekommen);
    }
    
    else /* EL */
    { 
      RESOURCE_VEKTOR gegeben;
      long maxbetrag = 0;
      umladung.negieren(); // Denn es wird dasselbe info benutzt
      gegeben.setzen_auf(objekt->info("RESSOURCEN_ANFORDERN",
				  umladung.to_string(), this, &maxbetrag));
      umladung.negieren(); // Nun wieder positiv
      umladung.subtrahiere(gegeben); // Gegebenes abziehen
      lager.subtrahiere(gegeben);
      gebucht.subtrahiere(gegeben);
    }
  )      

  // Jetzt kann ich sehen, ob eine Transaktion moeglich war.
  if (gebucht.ist_null()) {
    if (beladen) {
      fehlermeldung1(L("Die gew~unschten G~uter sind nicht verf~ugbar.\n","The desired commodities are not available.\n"));
      return 1;
    }
    else {
      fehlermeldung1(L("Unsere G~uter k~onnen nicht ausgeladen werden.\n","We cannot unload our commodities.\n"));
      return 1;
    }
  }

  // So. Es wurde zumindest etwas umgeladen. Ich gebe Meldung.
  sprintf(infostring,"%s (%s)", befehl, gebucht.to_string());
  tagebuch_fuehren(infostring,", ");
  return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::uns_liebt()
  * 
  * Stellt fest, ob ein fremder Staat dem unseren freundlich gesonnen ist.
  * @param
  * OBJEKT *obj:  Ein Objekt des fremden Staates oder dieser selbst.
  ---------------------------------------------------------------------------*/
short EINHEIT::uns_liebt(OBJEKT *obj)
{
  // Ich muss den Staat des Objektes ermitteln.
  while (obj && mystrcmp(obj->attribut("TYP"), "STAAT")) obj=obj->besitzer;
  if (!obj) return 1; // Bei Fehler als neutral melden.
 
  return (obj->kommando("DIPL_STATUS", this) >= 2);
}


/**---------------------------------------------------------------------------
  * EINHEIT::sichtungen_ins_tagebuch()
  * 
  * Erzeugt eine aktuelle Sichtung und schreibt sie ins Tagebuch.
  * Merkt sich die Sichtung ausserdem, um spaeter feststellen zu
  * koennen, ob sie sich geaendert hat.
  *
  * @param
  * long phase:    Phase des Zustandes. Wird mit ins Tagebuch aufge-
  * nommen.
  * char *zustand: Kann vorgegeben werden. Dann wird er hier nicht
  * berechnet.
  ---------------------------------------------------------------------------*/
void EINHEIT::sichtungen_ins_tagebuch(long phase, char *zustand)
{
    if (!zustand) { // Zustand erst noch ermitteln
	zustand = sichtungen_ermitteln();
    }
    myfree(alter_zustand);
    alter_zustand = zustand; // Merken der Daten. Ich uebernehme den Speicher

    if (zustand && zustand[0]) { // Sichtung ist nicht leer
	char verweis[20];
	sprintf(verweis, "[%ld]", phase);
	tagebuch_fuehren(verweis, ", ");
	report("#\n%s: ", verweis);
	report(zustand);
    }
}

/**---------------------------------------------------------------------------
  * EINHEIT::sichtungen_aktualisieren()
  * 
  * Der Zustand im Tagebuch gibt an, welche (fremde) Einheiten fuer
  * die Einheit sichtbar sind, und auf welchen Positionen. Auf dem
  * Ausdruck wird dies z.B. als [Miliz NO] oder [Hauptstadt Uruk]
  * ausgegeben.
  *
  * @param
  * long phase:              Zeitpunkt des Zustandes.
  * DOUBLIST *objektliste:   Zeiger auf die Objekte der Umgebung, wenn eine Liste
  *                          schon vorhanden ist (Zeitersparnis). Ansonsten NULL.
  ---------------------------------------------------------------------------*/
void EINHEIT::sichtungen_aktualisieren(long phase,
						DOUBLIST *objektliste)
{
  // Zuerst ermittle ich den aktuellen Zustand
  char *zustand = sichtungen_ermitteln(objektliste);

  // Jetzt vergleiche ich, ob sich der Zustand geaendert hat.
  if (alter_zustand && !mystrcmp(alter_zustand, zustand)) myfree(zustand); // Nein
  else sichtungen_ins_tagebuch(phase, zustand); // Ja.
}


/**---------------------------------------------------------------------------
  * EINHEIT:gelaende_aktualisieren()
  * 
  * Uebertraegt die von der Einheit aus sichtbaren Felder von der echten
  * Welt ins Landschaftsabbild des Spielers. Aber nur, wenn die Einheit,
  * sich nicht im Konvoi befindet.
  ---------------------------------------------------------------------------*/
void EINHEIT::gelaende_aktualisieren()
{
    if (ort_ist_welt()) 
	landschaft()->landschaftsabbild_aktualisieren(staat(), adresse, sichtweite);
}


/**---------------------------------------------------------------------------
  * EINHEIT::naechster_zug()             // virtuell
  * 
  * Wird am Anfang jeder Spielrunde fuer eine Einheit aufgerufen.
  * Wird also vor allen naechste_phase() aufgerufen. 
  ---------------------------------------------------------------------------*/
void EINHEIT::naechster_zug(long)
{
  // Initialisierungen am Anfang: Tagebuch
  tagebuch_loeschen();
  fussnoten_zaehler = 0;

  // Es gibt Einheiten, deren Verteidigungsbonus ergibt sich aus
  // bestimmten Einfluessen. Fuer diese Einheiten setzte ich den
  // Wert nun neu unabhaengig von der Enzyklopaedie. Es handelt
  // sich dabei im Wesentlichen um die Miliz.
 
  long selbstverteidigung = einfluss_aufsummieren("SV");
  if (selbstverteidigung) verteidigungskraft = selbstverteidigung;

  sichtungen_ins_tagebuch(1);

  // Damit ich im Ergebnisausdruck spaeter Bescheid weiss, merke ich mir
  // meine Adresse und Ort am Anfang des Zuges.

  startadresse = adresse;
  startort = ort();

  // Fuer den WI-Befehl. Eigentlich erkennt er daran, ob der WI-Befehl
  // 0 Phasen dauert. Ich muss das Attribut jede Runde loeschen, damit
  // 0 Phasen nicht mit 100, 200, 300... Phasen verwechselt werden.
  attribut_loeschen("WI_PHASE");

  // Die Variable anzahl_kaempfe wird beim Printout verwendet, um zu erkennen,
  // in wiviele Kaempe eine Einheit diese Runde verwickelt war.
  anzahl_kaempfe = 0;
  medaillen = 0;
}


/**---------------------------------------------------------------------------
  * EINHEIT::zug_abschliessen()
  * 
  * Wird am Ende des gesamten Zuges aufgerufen und macht noch einige
  * Abschliessende Operationen.
  ---------------------------------------------------------------------------*/
void EINHEIT::zug_abschliessen(long)
{
  // Jetzt trage ich in mein Landschaftsabbild noch alle Objekte ein, die
  // ich gerade sehe und aktualisiere die Felder um mich herum

    sichtungen_aktualisieren(100); // Andere Staaten kamen eventuell nach mir dran!

    if (ort_ist_welt()) // Geht nur in der Welt
    {
	landschaft()->sicht_eines_objektes(staat(), adresse, sichtweite);
	gelaende_aktualisieren();
    }

    // Das Attribut Gebombt loesche ich hier. Es dient zur Ueberwachung der
    // Regel, dass eine Einheit nur einmal pro Runde bomben darf. Gleiches
    // gilt fuer das Feuern.
    
    attribut_loeschen("Gebombt");
    attribut_loeschen("Gefeuert");
}

/**---------------------------------------------------------------------------
  * EINHEIT::naechste_phase()
  * 
  * Virtuell ueberladene Funktion von OBJEKT, die jede von 100
  * Runden rekursiv (ueber den Besitzbaum) fuer jedes Objekt aufgerufen
  * wird. Sie erledigt bei der Einheit verschiedene Kleinigkeiten.
  ---------------------------------------------------------------------------*/
void EINHEIT::naechste_phase(long runde)
{
    log('7', "           -> %s", name);

    // Statusanzeige zur Kontrolle
    io_printxy(49,1,"         ");
    io_printxy(49,1,name);

    // In einer bestimmten Runde muss ich mich von der Stadt mit Resourcen
    // versorgen.

    if (runde == RUNDE_VERSORGUNG_EINHEITEN) unterhalt_einholen();

    // Nun kommt die Alarmsbehandlung. Diese faengt aber erst in Runde 2 an,
    // damit man noch Gelegenheit hat, Strategien rechtzeitig abzubrechen
    // mit AA SW0.
  
    // Die Funktion objekte_der_umgebung_pruefen() untersucht alle
    // Objekte im Sichradius auf ihre Relevanz auf Alarmerzeugung.
    // Die dabei erzeugte Objektliste ist mit viel Rechenaufwand
    // erzeugt worden, da dazu alle vorhandenen Objekte ueberhaupt
    // untersucht werden mussten. Diese Liste bekomme ich als Ergebnis
    // und verwende sie weiter fuer den Zustand im Tagebuch.
    
    DOUBLIST *objektliste = NULL;
    
    if (runde > 1) {
	objektliste = objekte_der_umgebung_pruefen(); // Erzeugt evtl. Alarms
	weitere_alarmursachen_pruefen();     // Ezeugt vielleicht weitere Alarms
	alarms_pruefen();                    // Beruecksichtigt diese Alarms.
    }
  
    sichtungen_aktualisieren(runde, objektliste); // Berichtet dem Spieler von Bewegungen.

    // Die Landschaft kann sich ohne meine Bewegung geaendert haben, weil z.B.
    // ein anderer Spieler das Gelaende veraendert hat. Deshalb werden zu Beginn
    // jeder Phase die sichtbaren Felder im Umkreis in mein Landschaftsabbild
    // uebertragen.
    
    gelaende_aktualisieren();

    if (objektliste) delete objektliste; // Brauche ich nun nicht mehr.

    // Das Attribut ANGRIFF_FOLGT besagt, dass ich gerade angegriffen werde.
    // Ich muss es ebenfalls einfach herunterzaehlen.

    long angriff_folgt = myatol(attribut("ANGRIFF_FOLGT"));
    if (angriff_folgt) {
	if (angriff_folgt-1) attribut_setzen("ANGRIFF_FOLGT", myltoa(angriff_folgt-1));
	else attribut_loeschen("ANGRIFF_FOLGT");
  }

}


/**---------------------------------------------------------------------------
  * EINHEIT::basis_hier()
  * 
  * Prueft, ob sich die Einheit bei oder in einer Basis befindet.
  * Eine Basis ist ein Objekt des eigenen Staates mit dem Attribut
  * Basis.
  * @return
  * short 0, wenn keine Basis da ist.
  * short 1, wenn eine mobile Basis da ist.
  * short 2, wenn eine immobile Basis da ist.
  * short 3, wenn sich die Einheit in einer Basis befindet.
  ---------------------------------------------------------------------------*/
short EINHEIT::basis_hier()
{
  if (ort()->attribut_gesetzt("Basis")) return 3;

  ADR adr = adresse;
  if (!ort_ist_welt()) adr = ort()->adresse;

  short basis_vorhanden = 0;
  FOR_EACH_OBJEKT_IN (welt()->alle_objekte_bei_adresse(adr,"Basis"))
  DO_AND_DELETE (
    if (staat()->besitzt(objekt))
      basis_vorhanden = MAX(basis_vorhanden, !mystrcmp(objekt->attribut("TYP"),
		       "EINHEIT") ? 1 : 2);
  )
  return basis_vorhanden;
}



/**---------------------------------------------------------------------------
  * EINHEIT::basis_im_umkreis()
  * 
  * Stellt fest, ob im Umkreis eines Feldes ein Basis ist. Eine
  * Basis ist jede Stadt, jeder Fliegerhorst und jeder Flugzeugtraeger
  * des eigenen Staates und von welchen, die mich als endgueltig
  * freundlich eingestellt haben.
  ---------------------------------------------------------------------------*/
short EINHEIT::basis_im_umkreis(ADR& mitte, float radius)
{
  // Da die Funktion WELT::alle_objekte_im_umkreis_von extrem langsam ist,
  // schaue ich erstmal, ob nicht eine Stadt in der Naehe ist. Wenn ja,
  // dann kann ich gleich aufhoeren zu suchen.

  OBJEKT *stadt;
  SCAN(staat()->besitztumsliste(), stadt)
  {
    if (stadt->typ_ist("STADT") || stadt->attribut_gesetzt("Basis"))
    {
      if (welt()->entfernung_zwischen(mitte, stadt->adresse) <= radius) return 1;
    }
    NEXT(stadt);
  }

  // Jetzt muss ich nochmal alle Objekte testen. Wegen der Flugzeugtraeger
  // und der Objekte von anderen Staaten.

  short basis_vorhanden = 0;
  FOR_EACH_OBJEKT_IN (welt()->alle_objekte_im_umkreis_von(mitte, radius, "Basis"))
  DO_AND_DELETE ({ 
    basis_vorhanden |= (staat()->besitzt(objekt));
    basis_vorhanden |= (staat()->mir_ist_endgueltig_freundlich_gegenueber(objekt));
  })
  return basis_vorhanden;

}


/**---------------------------------------------------------------------------
  * EINHEIT::unterhalt_einholen()
  * 
  * Mit dieser Funktion versucht die Einheit, sich bei der Stadt
  * die Resourcen zu holen, die fuer den Unterhalt noetigt sind.
  * Wenn dies nicht gelingt, so wird das Attribut Unversorgt ge-
  * setzt, andernfalls geloescht.
  *
  * Handelt es sich um eine Lufteinheit, so geschieht die Versorgung
  * etwas anders. Einzelheiten findest du im Programmtext.
  ---------------------------------------------------------------------------*/
void EINHEIT::unterhalt_einholen()
{
  // Die Resourcen, die das Objekt benoetigt, um sich zu versorgen, sind
  // im Attribut VERSORGUNG gespeichert.

  char *res = attribut("VERSORGUNG");
  if (!res) return; // Die Einheit benoetigt keine Versorgung

  // Wenn die Einheit in Reserve ist, so benoetigt sie keinen Unterhalt

  if (attribut_gesetzt("Reserve")) return;

  // Einheiten mit dem Attribut REICHWEITE versorgen sich im allgemeinen
  // nicht von der Heimatstadt, sondern von der naechstgelegenen Basis.
  // Alle anderen Einheiten versorgen sich von ihrem Besitzer, was im
  // allgemeinen die Heimatstadt ist.

  if (attribut_gesetzt("REICHWEITE")) {

    // Als Versorger kommen alle Objekte von meinem Staat in Frage, die
    // das Attribut Basis gesetzt haben. Ich ermittle all diese und ver-
    // suche, mich von irgendeiner zu versorgen. Natuerlich muessen diese
    // Objekte auch noch in der Naehe sein. Falls mein Ort nicht die Welt
    // ist, dann kann ich mich nur von meinem Transporter versorgen.
    
    short versorgt = 0;

    if (!ort_ist_welt())
      versorgt = !ort()->kommando("RESOURCEN_ANFORDERN", res, this);

    // Ich muss eine geeignete Basis in der naehe finden. Dabei sollen
    // zuerst Staedte, dann Fliegerhorste, dann Flugzeugtraeger
    // beruecksichtigt werden. Und eigene Basen denen von anderen
    // Staaten. Dies macht eine Unterfunktion
    
    else versorgt = unterhalt_von_basis_einholen();

    // Jetzt weiss ich, ob die Versorgung geklappt hat.
    if (!versorgt) { // Nein!
      if (!attribut_gesetzt("Unversorgt")) {

	attribut_setzen("Unversorgt");

	report(L("#\nEs ist kein St~utzpunkt mehr in unserer Reichweite, der uns","#\nThere is no home base in our range which supplies"));
	report(L(" versorgen kann! "," us! "));

	if (!attribut_gesetzt("Luft") || ort()->attribut_gesetzt("Basis"))
	  report(L("Wir m~ussen bis auf weiteres s~amtliche Aktionen einstellen... \n","We have to cancel all further actions... \n"));

	else {
	  // Ein Absturz kann nur vermieden werden, wenn sich die Einheit
	  // direkt bei einer Basis befindet. Dies versuche ich nun fest-
	  // zustellen.

	  if (basis_hier()) {
	    report(L("Wir sind gezwungen zu landen und m~ussen alle Aktionen ","We are forced to land and have to cease all actions "));
	    report(L("einstellen, bis unsere Versorgung wieder organisiert ist.\n","until supply is again guaranteed.\n"));
	  }

	  else { // Absturz
	    report(L("Wir k~onnen keine Basis mehr erreichen und st~urzen ab! ","We cannot reach any base and nose-dive! "));
	    enthaltenen_einheiten_berichten(L("Unser Flugzeug st~urzt aus Treibstoffmangel ab!","Our aircraft crashes due to lack of fuel!"));
	    zur_vernichtung_vormerken();
	  }
	} // if Luft.
      } // if vorher noch nicht Unversorgt gewesen
      else {
	report(L("Leider ist unsere Versorgung immer noch nicht organisiert,","We have still no adequate supply. Thus we still cannot"));
	report(L(" so da~s wir weiterhin handlungsunf~ahig sind.\n"," act.\n"));
      }
    } // if (!versorgt)

    else if (attribut_gesetzt("Unversorgt"))
    {
      report(L("#\nUnsere Versorgung steht wieder, und wir sind wieder bereit, Befehle anzunehmen.\n","#\n Supply ON. Unit ready to accept commands.\n"));
      attribut_loeschen("Unversorgt");
    }

    return;
  } // if (reichweite)

  // Die Befehle ab hier gelten nur noch fuer Einheiten ohne Reichweiten-
  // beschraenkung.

  if (besitzer->kommando("RESOURCEN_ANFORDERN", (void *)res)) { // Untaetigkeit
    if (!attribut_gesetzt("Unversorgt")) {
      report(L("#\n%s versorgt uns nicht mehr! ","#\n%s does not supply us anymore! "), besitzer->a_name());
      report(L("Wir m~ussen bis auf weiteres alle Handlungen einstellen... \n","From now on, we have to cancel all actions... \n"));
      attribut_setzen("Unversorgt");
    }
  }

  else if (attribut_gesetzt("Unversorgt"))
  {
    report(L("#\nDie Unterst~utzung von %s steht wieder, und wir sind wieder bereit, Befehle anzunehmen!\n","#\n%s supplied again. Ready to accept your new commands!\n"),
    besitzer->a_name());
    attribut_loeschen("Unversorgt");
  }
}

/**---------------------------------------------------------------------------
  * EINHEIT::unterhalt_von_basis_einholen()
  * 
  * Versucht, eine geeignete Basis in der Naehe zu finden und
  * von ihr Unterhalt zu bekommen. Dabei wird eine bestimmte
  * Reihenfolge eingehalten.
  * @return
  * short 1, wenn die Versorgung geklappt hat, sonst 0.
  ---------------------------------------------------------------------------*/
short EINHEIT::unterhalt_von_basis_einholen()
{
  // Erst erstelle ich die Liste aller in Frage kommenden Basen.
  DOUBLIST *basenliste = liste_aller_brauchbaren_basen();
  
  char *ressourcen = attribut("VERSORGUNG");

  // Und jetzt die erste Basis nehmen, von der ich mich versorgen kann.

  short versorgt = 0;
  FOR_EACH_OBJEKT_IN (basenliste)
  DO_AND_DELETE (
    if (!versorgt && !objekt->kommando("RESOURCEN_ANFORDERN", ressourcen,this))
      versorgt = 1;
  )
  return versorgt;
}                                                                           


/**---------------------------------------------------------------------------
  * EINHEIT::liste_aller_brauchbaren_basen()
  * 
  * Erstellt eine Liste aller Objekte, die mir (wenn ich Flugzeug
  * bin) als Basis dienen koennen. Das Besondere: Die Liste ist
  * sortiert nach den Kriterien, die die Spielanleitung als Rang-
  * ordnung fuer die Bevorzugung von Basen vorgiebt.
  * @return
  * DOUBLIST *basenliste: Achtung: Nach Gebrauch wieder freigeben.
  ---------------------------------------------------------------------------*/
DOUBLIST *EINHEIT::liste_aller_brauchbaren_basen()
{
  float reichweite = myatof(attribut("REICHWEITE"));
  DOUBLIST *basenliste = new DOUBLIST;

  OBJEKT_LIST_NODE *objektnode;
  SCAN(&globale_objekt_menge, objektnode)
  {
    OBJEKT *objekt = objektnode->objekt;
    
    if (   objekt->attribut_gesetzt("Basis")
        && (welt()->entfernung_zwischen(adresse, objekt->adresse) <= reichweite)
        && (   staat()->besitzt(objekt) 
            || staat()->mir_ist_endgueltig_freundlich_gegenueber(objekt)))
    {
      OBJEKT_LIST_NODE *neu = new OBJEKT_LIST_NODE;
      neu->objekt = objekt;
      basenliste->insert(neu);
    }
    NEXT(objektnode);
  }

  basenliste->sort(EINHEIT::sortfunction_basen, this);
  return basenliste;
}

/**---------------------------------------------------------------------------
  * EINHEIT::sortfunction_basen
  * 
  * Sortiert eine Liste von Basen bezueglich eines Objektes derart,
  * dass besser geeignete Basen weiter vorne in der Liste stehen.
  ---------------------------------------------------------------------------*/
short EINHEIT::sortfunction_basen(DOUBLIST_NODE *s1,
				  DOUBLIST_NODE *s2, void *data)
{
  EINHEIT *einheit = (EINHEIT *)data;
  OBJEKT *staat = einheit->staat();
  OBJEKT *basis1 = ((OBJEKT_LIST_NODE *)s1)->objekt;
  OBJEKT *basis2 = ((OBJEKT_LIST_NODE *)s2)->objekt;

  // Wenn diese Funktion eine 1 zurueckliefert, heisst das, dass basis1
  // NACH basis2 in der Liste auftauchen soll. Bei 0 wird nicht vertauscht.
  
  // eigen vor fremd,
  // Stadt vor Horst vor Einheit (F-Traeger)
  // nah vor fern
  
  if (staat->besitzt(basis2) && !staat->besitzt(basis1))         return 1;
  if (staat->besitzt(basis1) && !staat->besitzt(basis2))         return 0;

  if (basis2->typ_ist("STADT") && !basis1->typ_ist("STADT"))      return 1;
  if (basis1->typ_ist("STADT") && !basis2->typ_ist("STADT"))      return 0;

  if (basis2->typ_ist("WELTBAUT") && !basis1->typ_ist("WELTBAUT")) return 1;
  if (basis1->typ_ist("WELTBAUT") && !basis2->typ_ist("WELTBAUT")) return 0;
  
  WELT *welt = einheit->welt();
  ADR adresse = einheit->adresse;

  float entfernung1 = welt->entfernung_zwischen(adresse, basis1->adresse);
  float entfernung2 = welt->entfernung_zwischen(adresse, basis2->adresse);

  return (entfernung2 < entfernung1);
}


/**---------------------------------------------------------------------------
  * EINHEIT::ist_neu()
  * 
  * Stellt fest, ob die Einheit in dieser Runde gebaut wurde.
  ---------------------------------------------------------------------------*/
short EINHEIT::ist_neu()
{ 
 return (  myatol(attribut("BAUJAHR")) 
           == 
           myatol(g_uhr->info("ZUGNUMMER")));
}


/**---------------------------------------------------------------------------
  * EINHEIT::vorsortierungsnummer()
  * 
  ---------------------------------------------------------------------------*/
short EINHEIT::vorsortierungsnummer()
{
  if (!mystrcmp(a_gattung(), L("B~urgerwehr","Volunteer_Forces"))) return 0;

  // Kernwaffen habe kein Attribut Militaerisch, sollen aber bei den
  // militaerischen Einheiten auftauchen.

  if (attribut_gesetzt("Kernwaffe"))    return 22; 
  if (attribut_gesetzt("Abwehrrakete")) return 23;

  // Zuerst die zivilen Einheiten
 
  if (!attribut_gesetzt("Militaerisch"))
  {
      if (attribut_gesetzt("MeteoritBergen")) return 13; // Forscher zuletzt.
      if (!mystrcmp(a_gattung(), L("Siedler","Settlers"))) return 1;
      if (attribut_gesetzt("BefehlSG")) return 2;		// Stadtgruender
      if (attribut_gesetzt("KannBauen") && !attribut_gesetzt("See")) return 3; // Tanker hier nich
      if (attribut_gesetzt("Boden"))
      {
	  if (attribut_gesetzt("LAGER")) return 4;
	  else return 5; // Uebrige zivile Bodeneinheiten  // gibt es die ueberhaupt?
      }
      else if (attribut_gesetzt("See")) {
	  if (!mystrcmp(attribut("LAGER"), L("F","C"))) return 9; // Oelschiffe
	  if (attribut_gesetzt("LAGER")) return 8; // uebrige Frachtschiffe vorher!
	  if (attribut_gesetzt("CONVOY")) return 10; // Transportschiffe
      }
      else return 12; // Zivile Lufteinheiten
  }
 
  // Und dann die militaerischen
  if (attribut_gesetzt("Boden"))
  {
    if (!attribut_gesetzt("FEUERKRAFT")) return 20;
    else return 21;
  }
  else if (attribut_gesetzt("See")) 
  {
    if (attribut_gesetzt("Basis")) return 31; // Flugzeugtraeger danach
    else return 30;
  }
  else if (attribut_gesetzt("Luft"))
  { 
    if (!attribut_gesetzt("REICHWEITE")) return 40; // Zeppelin
    return 50 - bewegungspunkte_pro_feld;
  }
 
  return 100; // Alles andere ganz zum Schluss. Keine Ahnung, was das sein soll.
}


/**---------------------------------------------------------------------------
  * EINHEIT::sortfunktion_einheiten()
  * 
  * Definiert eine Ordnung auf einer Menge von Einheiten. Damit wird
  * die Liste der Einheiten fuer den Printout sortiert.
  * @return
  * Wenn die Einheit eins kleiner ist als die Einheit zwei, also in
  * der Liste vorher vorkommen soll, dann wird eine Wert <= 0 zurueck-
  * gegeben. Ein Wert > 0 deutet der Sortiertfunktion ein Vertauschen
  * an.
  ---------------------------------------------------------------------------*/
short EINHEIT::sortfunktion_einheiten(DOUBLIST_NODE *n1, DOUBLIST_NODE *n2, void *)
{
  EINHEIT *einheit1 = (EINHEIT *)(((OBJEKT_LIST_NODE *)n1)->objekt),
          *einheit2 = (EINHEIT *)(((OBJEKT_LIST_NODE *)n2)->objekt);
  
  // Als erstes sortiere ich nach Staaten. Dies brauche ich bei der
  // Auflistung der Einheiten auf dem gleichen Feld beim HTML-Printout.

  STAAT *staat1 = einheit1->staat();
  STAAT *staat2 = einheit2->staat();
  if (staat1 != staat2) return strcmp(staat1->name, staat2->name);

  // Neue Einheiten sollen auf jedem Fall ganz am Schluss kommen.

  short neu1 = einheit1->ist_neu();
  short neu2 = einheit2->ist_neu();
  if (neu1 != neu2) return neu1-neu2;  

  // Jetzt erstmal grob einteilen. Dazu dient die vorsortierungsnummer
  
  short vor1 = einheit1->vorsortierungsnummer();
  short vor2 = einheit2->vorsortierungsnummer();
  if (vor1 != vor2) return (vor1 > vor2);
 
  // Jetzt nach Gattungen sortieren.
  char *gat1 = einheit1->a_gattung();
  char *gat2 = einheit2->a_gattung();
  
  if (mystrcmp(gat1, gat2))  // Verschiedenen Gattungen
  {
    return g_enzyklopaedie->welche_gattung_kommt_zuerst(gat1, gat2);
  }
    
  long bj1 = myatol(einheit1->attribut("BAUJAHR"));
  long bj2 = myatol(einheit2->attribut("BAUJAHR"));
  if (bj1 != bj2) return bj2 < bj1; // Kleinere (=aeltere) zuerst


  // bei gleichem Baujahr Abkuerzung, erste drei Buchstaben
  int str = strncmp(einheit1->name, einheit2->name, 3);
  if (str) return str;
  
  // Bei gleichen ersten drei Buchstaben sortiere ich nach der
  // Nummer. Z.b HSA12 vor HSA13.

  return myatol(einheit1->name+3) > myatol(einheit2->name+3);
}

  
/**---------------------------------------------------------------------------
  * EINHEIT::abschlussbericht()
  * 
  * Funktion aus OBJEKT, die den Ausdruck layoutet.
  ---------------------------------------------------------------------------*/
void EINHEIT::abschlussbericht()
{
  // Zuerst kommt immer das Tagebuch, dann kommen eventuell noch die
  // reports.

  verzeichnis_gewaehrleisten(tempfile_name()); 
  FILE *temp = fopen(tempfile_name(),"w");
  if (!temp) return;

  // Bevor ich das Tagebuch reporte, leite ich es noch mit ein paar
  // Angaben ein.

  const char *kostenlos = mystrlen(attribut("VERSORGUNG")) <= 1 ? 
    L(", kostenlose Einheit",", no upkeep") : "";
    
  // Ich printe drei Leerzeichen am Anfang, damit die Grafik Platz hat.
  // Die Blocksatzroutine macht automatisch aus Unterstrichen Leerzeichen.
  // Spaces werden naemlich als Trennzeichen interpretiert und koennen
  // somit nicht zum Platzhalten verwendet werden.
  
  fprintf(temp, "______%s ",name);
  if (attribut_gesetzt("NAME")) fprintf(temp," \"%s\" ",a_name());
  char *unterhalt = attribut("VERSORGUNG");
  if (mystrlen(unterhalt) <= 1) unterhalt="-";
  fprintf(temp,L("(%ld|%ld|%ld|%s), %s aus %s%s, ","(%ld|%ld|%ld|%s), %s from %s%s, "),
       angriffskraft, verteidigungskraft, bewegungspunkte_pro_feld,
       unterhalt,
       a_gattung(), besitzer->a_name()
       , kostenlos); // Dies Angaben auf jeden Fall.

  // Wenn der Ort zwar die Welt ist, aber die Startadresse noch
  // ungueltig, und ausserdem der Startort nicht eine Einheit war,
  // dann ist die Einheit waehrend dieser Runde geschaffen worden.

  if (ort_ist_welt() && welt()->adresse_ist_ausserhalb(startadresse)
     && (!startort || mystrcmp(startort->attribut("TYP"),"EINHEIT")))
  {
    fprintf(temp,L("startet %s:","starts at %s:"), relative_adressangabe(adresse, ort(), 1));
  }

  // Sind Start und Ziel gleich?

  else if (ort() == startort && adresse == startadresse)
	fprintf(temp,L("ist %s:","is %s:"), relative_adressangabe(adresse, ort(), 1));

  // Start und Ziel sind also verschieden. Das bedeutet, dass ich eine
  // von ... nach ... Darstellung ausgebe.

  else {
    char *start = mystrdup(relative_adressangabe(startadresse, startort, 2));
    fprintf(temp,L("zieht %s %s:","moves %s %s:"),start, relative_adressangabe(adresse, ort(), 3));
    myfree(start);
  }

  if (tagebuch()) fprintf(temp," %s. ", tagebuch());
  else fprintf(temp, L(" Keine Neuigkeiten. "," No news. "));

  // Jetzt kommen noch ein paar weitere Angaben: Ist die Einheit mobil oder
  // in Reserve, ist die Einheit befestigt oder nicht? Stehen noch Befehle
  // an? Aber alles nur, falls die Einheit nicht ohnehin schon zur Ver-
  // nichtung vorgemerkt ist.

  short klammer_auf = 0;
  if (attribut_gesetzt(L("Eingegraben","Dug in (in reserve)"))) {
      fprintf(temp, L("(befestigt","(Dug in"));
      klammer_auf = 1;
  }

  if (attribut_gesetzt(L("Reserve","Reserve"))) {
      fprintf(temp, klammer_auf ? L(", in Reserve",", in reserve")
	      : L("(in Reserve","(in reserve"));
      klammer_auf = 1;
  }

  // Ist die Einheit am Ende gar unversorgt??
  if (attribut_gesetzt("Unversorgt")) {
      fprintf(temp, klammer_auf ? L(", unversorgt",", not supplied")
	      : L("(unversorgt","(not supplied"));
      klammer_auf = 1;
  }

  // Hat die Einheit Ressourcen geladen, wenn ja, welche?
  if (!lager.ist_null()) {
      fprintf(temp, klammer_auf ? ", " : "(");
      fprintf(temp, L("beladen mit %s","carrying %s"), lager.to_string());
      klammer_auf = 1;
  }

  // Stehen noch Befehle an?
  if (!befehlsliste.is_empty()) {
      fprintf(temp, klammer_auf ? L(", Befehle",", commands")
	      : L("(Befehle","(Commands"));
      if (zur_vernichtung_vorgemerkt()) fprintf(temp, L(" w~aren gewesen: "," would have been: "));
      else fprintf(temp, ": ");
      klammer_auf = 1; // Damit ich sie nachher wieder zu mache.
      BEFEHL *befehl;
      SCAN(&befehlsliste, befehl) {
	  if (befehl->befehlstext[0] != '*') 
	      fprintf(temp, "%s ",befehl->befehlstext);
	  NEXT(befehl);
      }
      fprintf(temp, "[%ld]", phasenzaehler);
  }

  // Und jetzt ist noch interessant, welche Strategie die Einheit gerade
  // verfolgt.

  if (aktuelle_strategie) {
      fprintf(temp, klammer_auf ? ", " : "(");
      fprintf(temp,L("Stratgie %ld","Strategy %ld"),aktuelle_strategie);
      klammer_auf = 1;
  }
  if (klammer_auf) fprintf(temp, ")");

  fprintf(temp,"#\n #\n");
  reportliste_in_file(temp);
  fclose(temp);

  char *druckercode = dr_infodatei_formatieren(tempfile_name(), 116);
  if (druckercode) {
    if (laser) {
      long anzahl_zeilen = dr_anzahl_zeilen(druckercode);
      // Ich layoute alle Zeilen einzeln
      char *zeilenanfang = druckercode;
      for (int i=0; i<anzahl_zeilen; i++)
      {
        LAYOUT ly(dr_abschnitt(i==0 ? 0.40 : 0.35));
        // In der ersten Zeile die kleinen Grafiken
        if (i==0)
        {
          char grafikname[16];
          sprintf(grafikname,"e%s.gra",attribut("GRAFIK"));
          ly.bitmatrix(0, 0, grafikname);
          if (ort_ist_welt())
 	    ly.bitmatrix(0.5, 0, gelaende_attribut("GRAFIK"));
 	}
 	char *scan = zeilenanfang;
 	while (*scan && *scan != '\n') scan++;
        *scan = 0; // Zeile terminieren
        dr_zeilenabstand(0.35); // Erst hier. Bei neuer Seite wirds zurueckges.
        ly.text(0, i==0 ? .275 : .225, zeilenanfang, "Courier", 8.063);
        ly.ausdrucken();
        zeilenanfang = scan+1;
      }
      // if (zur_vernichtung_vorgemerkt())  ly.linie(0,0,0,absatz_hoehe);
      
    }
    else drucken(druckercode); // Ohne Laser einfach als Ascii.
  }

  myfree(druckercode);

  reportliste_loeschen();  // Freimachen fuer die naechste Runde.
}


/**---------------------------------------------------------------------------
  * EINHEIT::abschlussbericht_html()
  * 
  * Erzeugt einen Abschlussbericht im HTML-Format.
  * @param
  * HTML html: Ein bestehendes HTML, von dem ein Clone gemacht wird,
  * der in die gleiche Datei schreibt.
  ---------------------------------------------------------------------------*/
void EINHEIT::abschlussbericht_html(HTML html)
{
  html.anchor(name);
  html.smalleimage(attribut("GRAFIK"));
  if (ort_ist_welt()) 
     html.fieldimage(welt()->gelaendeform_attribut(adresse, "GRAFIK"));
  else if (ort_ist_einheit()) objektgrafik_mit_link(html, ort());

  // Ein paar nette Symbole fuer verschiedene Lebenslagen...

  if (ist_neu()) html.iconimage("neu");
  if (attribut_gesetzt("Reserve")) html.iconimage("bierkrug");
  if (attribut_gesetzt("Eingegraben")) html.iconimage("spaten");
  for (int i=0; i<anzahl_kaempfe; i++) html.iconimage("saebel");

  long med = medaillen;
  while (med >= 5) {
      html.iconimage("goldmeda");
      med -= 5;
  }
  while (med--) html.iconimage("silbmeda");
  if (zur_vernichtung_vorgemerkt()) html.iconimage("totnkopf");
  
  html.text(" ");

  // Liste aller Objekte auf dem gleichen Feld.
  if (ort_ist_welt()) {
      if (stadt_hier()) objektgrafik_mit_link(html, stadt_hier());
      if (weltbaut_hier()) objektgrafik_mit_link(html, weltbaut_hier());
      DOUBLIST *einheiten_hier = welt()->alle_objekte_bei_adresse(adresse, "TYP=EINHEIT");
      einheiten_hier->sort(EINHEIT::sortfunktion_einheiten);
      FOR_EACH_OBJEKT_IN (einheiten_hier)
	  DO({
	      if (objekt != this) objektgrafik_mit_link(html, objekt);
	  });
      delete einheiten_hier;
  }
  else if (ort_ist_einheit()) { // Alle anderen Einheiten im selben Boot zeigen!
      FOR_EACH_OBJEKT_IN (einheit()->alle_enthaltenen_objekte())
	  DO_AND_DELETE ({
	      if (objekt != this) objektgrafik_mit_link(html, objekt);
	  });
  }
      

  html.courier().bold().text(" ").text(name).bold_off().courier_off();

  if (a_name()) html.text(" \"").text(a_name()).text("\"");

  // (Angriff|Verteidiung|Bewegung|Unterhalt)
  char *unterhalt = attribut("VERSORGUNG");
  if (mystrlen(unterhalt) <= 1) unterhalt="-";
  html.text(" ( ").text(angriffskraft).text(" | ")
      .text(verteidigungskraft).text(" | ")
      .text(bewegungspunkte_pro_feld).text(" | ")
      .text(unterhalt).text(" )");
  
  html.text(", ");
  g_enzyklopaedie->vorhaben_mit_link(html, a_gattung());

  html.text(L(" aus "," from "));
  html.href_objekt(besitzer).text(besitzer->a_name()).end_href();
  if (mystrlen(attribut("VERSORGUNG")) <= 1) html.text(L(", kostenlose Einheit, ",", no upkeep, "));

  if (  ort_ist_welt() 
     && welt()->adresse_ist_ausserhalb(startadresse)
     && (!startort || startort->typ_ist("EINHEIT"))) {
    html.text(L(" startet "," starts at ")).text(relative_adressangabe(adresse, ort(), 1));
  }

  else if (ort() == startort && adresse == startadresse)
    html.text(L(" ist "," is ")).text(relative_adressangabe(adresse, ort(), 1));

  else {
    char *start = mystrdup(relative_adressangabe(startadresse, startort, 2));
    html.text(L(" zieht "," moves ")).text(start).text(" ")
        .text(relative_adressangabe(adresse, ort(), 3));
    myfree(start);
  }

  html.text(": ");
  if (tagebuch()) html.text(tagebuch()).text(". ");
  else html.text(L(" Keine Neuigkeiten. "," No news. "));

  // Und jetzt noch die Informationen am Ende.

  short klammer_auf = 0;
  if (attribut_gesetzt("Eingegraben")) {
      html.text(L("(befestigt","(Dug in"));
      klammer_auf = 1;
  }

  if (attribut_gesetzt("Reserve")) {
      html.text(klammer_auf ? L(", in Reserve",", in reserve")
		: L("(in Reserve","(in reserve"));
      klammer_auf = 1;
  }

  // Ist die Einheit am Ende gar unversorgt??
  if (attribut_gesetzt("Unversorgt")) {
      html.text(klammer_auf ? L(", unversorgt",", not supplied")
		: L("(unversorgt","(not supplied"));
      klammer_auf = 1;
  }

  // Hat die Einheit Ressourcen geladen, wenn ja, welche?
  if (!lager.ist_null()) {
      html.text(klammer_auf ? ", " : "(");
      html.text(L("beladen mit ","carrying ")).text(lager.to_string());
      klammer_auf = 1;
  }

  // Stehen noch Befehle an?
  if (!befehlsliste.is_empty()) {
      html.text(klammer_auf ? L(", Befehle",", commands")
		: L("(Befehle","(Commands")).bold();
      if (zur_vernichtung_vorgemerkt()) html.text(L(" w~aren gewesen: "," would have been: "));
      else html.text(": ");
      klammer_auf = 1; // Damit ich sie nachher wieder zu mache.
      BEFEHL *befehl;
      SCAN(&befehlsliste, befehl) {
	  if (befehl->befehlstext[0] != '*') 
	      html.text(befehl->befehlstext).text(" ");
	  NEXT(befehl);
      }
      html.bold_off().text("[").text(phasenzaehler).text("]");
  }

  // Und jetzt ist noch interessant, welche Strategie die Einheit gerade
  // verfolgt.

  if (aktuelle_strategie) {
      html.text(klammer_auf ? ", " : "(")
          .text(L("Stratgie ","Strategy ")).text(aktuelle_strategie);
      klammer_auf = 1;
  }
  if (klammer_auf) html.text(")");

  // Reports
  if (!reportliste.is_empty()) {
    html.linebreak();
    REPORT *report;
    SCAN(&reportliste, report)
    {
      html.text(report->text);
      NEXT(report);
    }
  }

}

void EINHEIT::objektgrafik_mit_link(HTML& html, OBJEKT *objekt)
{
    bool eigen = staat()->besitzt(objekt);
    if (eigen) html.href_objekt(objekt);
    char grafikname[MAX_LAENGE_DATEINAME];
    char alt[512];
    if (eigen) strcpy(alt, objekt->name);
    else alt[0]=0;
    if (objekt->attribut_gesetzt("NAME")) {
	if (eigen) strcat(alt, " ");
	strcat(alt, "\"");
	strcat(alt, objekt->a_name());
	strcat(alt, "\"");
    }
    sprintf(grafikname, "%c%s", eigen ? 'e' : 'g', objekt->attribut("GRAFIK"));
    html.set_image_border(0)
	.smallimage(grafikname, alt[0] ? alt : 0)
	.unset_image_border();

    if (eigen) html.end_href();
}


/**---------------------------------------------------------------------------
  * 
  * 
  *
  * @param
  * ADR& adr:    Adresse.
  * OBJEKT *ort: Ort (muss ja nicht immer Welt sein.)
  * short modus:     Art der Praeposition: Bei 1 heisst es bei/in,
  * bei 2 von/aus, bei 3 nach/in.
  ---------------------------------------------------------------------------*/
char *EINHEIT::relative_adressangabe(ADR& adr, OBJEKT *ort,
			 short modus)
{
  static char antwort[50];

  if (!ort) {
    static char *antwort[3]={L("im Universum","in the universe "),
			     L("vom Universum","out of the universe"),
			     L("ins Universum","into the universe")};
    return antwort[modus-1];
  }

  else if (!mystrcmp(ort->attribut("TYP"),"WELT")) {
    if (!staat()) return "0,0";
    RIC *ric = (RIC *)staat()->info("RELATIVE_ADRESSE",&adr);
    static char *praep[3] = { L("bei","at"),
			      L("von","from"),
			      L("nach","to") };
    sprintf(antwort,"%s %ld,%ld",praep[modus-1], ric->x, ric->y);
  }

  else if (!mystrcmp(ort->attribut("TYP"),"STADT")) {
    static char *praep[3] = { L("in","in"),
			      L("von","from"),
			      L("nach","to") };
    sprintf(antwort,"%s %s",praep[modus-1], ort->a_name());
  }

  else { // Alles andere, vor allem auch Einheiten...
    static char *praep[3] = { L("in","in"),
			      L("aus","from"),
			      L("in","in") };
    sprintf(antwort,"%s %s %s",praep[modus-1], ort->a_gattung(), ort->name);
  }

  return antwort;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
ADR& EINHEIT::absolute_adresse(char *relativ)
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


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
RIC& EINHEIT::relative_adresse(ADR& absolut)
{
  static RIC antwort("");

  antwort = *(RIC *)(staat()->info("RELATIVE_ADRESSE",&absolut));
  return antwort;
}

 
/**---------------------------------------------------------------------------
  * EINHEIT::objekt_aufnehmen()
  * 
  * Ist bereits on OBJEKT virtuell definiert. Ich soll hier ent-
  * scheiden, ob ich das Objekt aufnehmen will. Ich nehme im uebrigen
  * nur Objekte an Bord, denen mein Staat freundlich gesonnen ist.
  ---------------------------------------------------------------------------*/
short EINHEIT::objekt_aufnehmen(OBJEKT *gast, ADR&)
{
  // Wenn ein Schiff gekapert wird, dann wechselt der Inhalt seinen
  // Ort. Die Einheiten sind zu dem Zeitpunkt moeglicherweise noch
  // feindlich. Ausserdem soll keine Meldung kommen. Deshalb wird
  // vor dem Aufruf der Funktion ein aeusserst temporaeres Attribut
  // gesetzt.
  
  if (gast->attribut_gesetzt("Transport_gekapert")) return 0;

  if (!wir_lieben(gast)) return 1; // Keine Gegner oder Neutrale aufnehmen.

  long convoy = myatol(attribut("CONVOY"));
  if (!convoy) return 1; // Ich kann nicht transportieren

  // Jetzt brauche ich die Anzahl der Einheiten in mir.
  
  long anzahl = alle_enthaltenen_objekte()->count_and_del();
  if (anzahl >= convoy) return 1; // Dann bin ich schon voll.

  // Bin ich eine Lufteinheit, dann kann ich nur aufnehmen, wenn eine Basis
  // bei mir ist, auf der ich sozusagen landen kann. Ausser bei Hubschraubern.

  if (attribut_gesetzt("Luft")
      && !attribut_gesetzt("Hubschrauber") && !basis_hier()) return 1; // Geht nicht.

  // Ich fuehre Tagebuch darueber, dass ich eine Einheit aufgenommen habe.
  
  char string[50];
  sprintf(string, L("%s %s an Bord gegangen","%s %s embarked"), gast->a_gattung(), gast->name);
  tagebuch_fuehren(string, ", ");
  return 0; // Alles klar, ich erlaube das an Bord gehen.

}

/**---------------------------------------------------------------------------
  * EINHEIT::objekt_entlassen()
  * 
  * Erzeugt eine Meldung, das die Enheit verlassen wurde.
  ---------------------------------------------------------------------------*/
void EINHEIT::objekt_entlassen(OBJEKT *gast)
{
  if (gast->attribut_gesetzt("Transport_gekapert")) return; // Keine Meldung
  
  char string[50];
  sprintf(string, L("%s %s von Bord gegangen","%s %s disembarked"), gast->a_gattung(), gast->name);
  tagebuch_fuehren(string, ", ");
}
  
/**---------------------------------------------------------------------------
  * EINHEIT::enthaltenen_einheiten_berichten()
  * 
  * Alle enthaltenen Einheiten geben einen Text in ihrem Tagebuch aus.
  ---------------------------------------------------------------------------*/
void EINHEIT::enthaltenen_einheiten_berichten(char *text)
{
   FOR_EACH_OBJEKT_IN (alle_enthaltenen_objekte())
   DO_AND_DELETE (objekt->tagebuch_fuehren(text, ", "));
}

/**---------------------------------------------------------------------------
  * EINHEIT::medaille_bekommen()
  * 
  * Zaehlt die Variable um eins rauf.
  ---------------------------------------------------------------------------*/
void EINHEIT::medaille_bekommen()
{
    medaillen ++;
}
