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
  * MODUL:               strategi.C  /  STRATEGI.CPP
  * AUTOR/DATUM:         Mathias Kettner, 3. August 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Funktionen zum Strategiesystem. Die Funktionen sind zum Teil von
//      EINHEIT und auch dort deklariert.
//
// **************************************************************************

#include <ctype.h>
#include "einheit.h"
#include "stratab.h" // Bindet tatsaechliche Daten ein!
#include "listmac.h"


/**---------------------------------------------------------------------------
  * ALARM::prioritaet()
  * 
  * Ermittelt die Prioritaet, die dieser Alarm unter einer bestimmten
  * Strategie hat. In der Strategietabelle ist diese Prioritaet durch
  * eine Ziffer angegeben, die an erster Position im Eintrag steht.
  *
  * @param
  * short strategie:        Nummer der Strategie, um die es sich handelt.
  *
  * @return
  * short: Prioritaet.
  ---------------------------------------------------------------------------*/
short ALARM::prioritaet(short strategie)
{
  char code1 = strategie_tabelle[strategie][typ][0];
  if (isdigit(code1)) return (short)code1-'0';
  else return 0;
}


/**---------------------------------------------------------------------------
  * EINHEIT::alarmiert_werden()
  * 
  * Indiziert bei einer Einheit einen neuen Alarm. Teilt der Einheit
  * also das Eintreten einer gewissen Situation oder eines bestimmten
  * Ereignisses mit. Der neue Alarm wird aber noch nicht sofort
  * bearbeitet, sondern kommt in die Liste der anstehenden Alarms.
  *
  * @param
  * short typ:       Typ des Alarms, d.h. eine der Konstanten
  * A_FEINDKONTAKT, A_FEIND_IM_WEG,...
  *
  * char *parameter: Zusaetzliche Parameter. Die Syntax und Semantik
  * haengt vom Alarmtyp ab.
  ---------------------------------------------------------------------------*/
void EINHEIT::alarmiert_werden(short typ, char *parameter)
{
  ALARM *alarm = new ALARM(typ, parameter);
  alarms.insert(alarm); // Reihenfolge spielt keine Rolle
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void EINHEIT::alle_alarms_loeschen()
{
  alarms.clear();
}


/**---------------------------------------------------------------------------
  * EINHEIT::wichtigster_alarm()
  * 
  * Ermittelt denjenigen Alarm aus der liste der anstehenden Alarms,
  * der eine maximale Prioritaet hat. Bei Alarms mit gleicher Prio-
  * ritaet wird derjenige bevorzugt, der als erster in der Liste
  * auftaucht, also auch derjenige, der zuerst ausgeloest wurde
  * (da die Liste chronologisch angelegt wird).
  *
  * @return
  * ALARM *: Zeiger auf den Alarm oder NULL.
  ---------------------------------------------------------------------------*/
ALARM *EINHEIT::wichtigster_alarm()
{
  short bis_jetzt_hoechste_pri = 0;
  ALARM *bis_jetzt_wichtigster_alarm = NULL;

  ALARM *alarm = (ALARM *)alarms.first();
  while (!alarm->is_tail())
  {
    if (alarm->prioritaet(aktuelle_strategie) > bis_jetzt_hoechste_pri) {
      bis_jetzt_hoechste_pri = alarm->prioritaet(aktuelle_strategie);
      bis_jetzt_wichtigster_alarm = alarm;
    }
    alarm = (ALARM *)alarm->next();
  }

  return bis_jetzt_wichtigster_alarm; // Kann natuerlich auch NULL sein.
}


/**---------------------------------------------------------------------------
  * EINHEIT::einheit_alarmieren()
  * 
  * Mit dieser Funktion wird nicht etwa die implizite Einheit alarmiert.
  * Nein: Die implizite Einheit alarmiert eine fremde Einheit. Dieser
  * Mechanismus wird z.B. bei dem Alarmtyp A_FEIND_GREIFT_AN verwendet,
  * bei dem der Angreifer den Verteidiger alarmiert, sobald der Angriffs-
  * befehl gestartet wird.
  *
  * @param
  * OBJEKT *ziel:       Zu alarmierende Einheit
  * short typ:              Alarmtyp
  * char *par:              Alarmparameter
  ---------------------------------------------------------------------------*/
void EINHEIT::einheit_alarmieren(OBJEKT *ziel, short typ, char *par)
{
  long typlong = typ;
  ziel->kommando("ALARM", (void *)&typlong, (void *)par);
}


/**---------------------------------------------------------------------------
  * EINHEIT::kommando_alarm()
  * 
  * Realisiert das Kommando ALARM, mit dem bei der Einheit ein Alarm aus-
  * geloest werden kann, der dann unter Umstaenden das Einschieben eines
  * Befehls zur Folge hat. Diese Befehlseinschieben findet jedoch NICHT
  * in dieser Funktion staat. Hier wird der Alarm lediglich angereiht.
  *
  * @param
  * (long *)        Typ des Alarms (Nummer)
  * (char *)        Parameter
  *
  * @return
  * short 0.
  ---------------------------------------------------------------------------*/
short EINHEIT::kommando_alarm(void *par1, void *par2)
{
  alarmiert_werden(*(long *)par1, (char *)par2);
  return 0;
}


/**---------------------------------------------------------------------------
  * EINHEIT::alarms_pruefen()
  * 
  * Ist die zentrale Routine, die jede Runde prueft, ob Alarms einge-
  * gangen sind, und ggfls. eine ensprechende Aktion einleitet. Am
  * Ende werden alle Alarms wieder geloescht, auch solche, die aufgrund
  * zu niedriger Prioritaet ignoriert wurden. Wenn ein Alarm akut ist,
  * so wird er ohnehin jede Runde neu ausgeloest.
  ---------------------------------------------------------------------------*/
void EINHEIT::alarms_pruefen()
{
  // Zuerst hole ich mir den Alarm mit der hoechsten Prioritaet.

  ALARM *alarm = wichtigster_alarm();

  // Nur wenn dessen Prioritaet hoeher ist, als die gerade aktuelle, gehe
  // ich ueberhaupt in Aktion. Es kann natuerlich auch sein, dass garkein
  // Alarm vorlag.

  if (alarm && alarm->prioritaet(aktuelle_strategie) > aktuelle_prioritaet)
       alarm_verarbeiten(alarm);

  // Jetzt kann ich alle Alarms wieder loeschen...
  alle_alarms_loeschen();
}


/**---------------------------------------------------------------------------
  * EINHEIT::alarm_verarbeiten()
  * 
  * Hier ist die Funktion, in der dann endlich tatsaechlich ein auto-
  * matisch erzeugter Befehl vorgeschoben wird. Anhand des Alarms, der
  * uebergeben wird, werden aus der richtigen Tabelle die Befehle he-
  * rausgesucht und auch noch der Parameter richtig eingesetzt.
  * @param
  * ALARM *alarm:       Alarm, dem Tribut gezollt werden soll.
  ---------------------------------------------------------------------------*/
void EINHEIT::alarm_verarbeiten(ALARM *alarm)
{
  // Die aktuelle Prioritaet wird nun auf den neuen Wert gesetzt, die
  // alte aber vorher gemerkt.

  short alte_prioritaet = aktuelle_prioritaet;
  aktuelle_prioritaet = alarm->prioritaet(aktuelle_strategie);

  // Zuerst wird ein Befehl vorgeschoben, der dafuer sorgt, dass nach
  // Ablauf des Notprogrammes die Prioritaet wieder auf den richtigen
  // (alten) Wert gesetzt wird. Der Befehl heisst *P. Das erste
  // Argumentzeichen gibt die alte Prioritaet an. Die folgende Zahl
  // den aktuellen Phasenzaehler. Dieser wird gespeichert, damit nach
  // Ende des Strategiebefehls wieder der alte Stand erreicht ist.
  // Ausserdem wird dann der Phasenzaehler auf 0 gesetzt.
  
  char prbefehl[5];
  sprintf(prbefehl,"*P%1d%ld",alte_prioritaet, phasenzaehler);
  befehl_vorschieben(prbefehl);
  phasenzaehler = 0;

  // Jetzt muss noch in der Strategietabelle ermittelt werden, welche Befehle
  // zur Bearbeitung des Alarms ausgefuehrt werden sollen. Der Eintrag dieser
  // Tabelle hat als erstes Zeichen noch eine Ziffer fuer die Prioritaet.
  // Diese Ziffer muss ich hier ueberlesen. Ausserdem kann der Eintrag das
  // Kuerzel % enthalten. An dessen Stelle muss dann der Parameter des
  // Alarms eingetragen werden.
  // Mehrere Befehle werden durch Leerzeichen getrennt. Das Kuerzel %s darf
  // pro Befehl einmal auftauchen.

  char *befehlsformat = strategie_tabelle[aktuelle_strategie][alarm->typ]+1;
  const char *par = (alarm->parameter ? alarm->parameter : "");

  while (*befehlsformat)
  {
    while (*befehlsformat==' ') befehlsformat++; // Evtl. Trennspaces ul..
    char befehl[100], *write=befehl;
    while (*befehlsformat && *befehlsformat!=' ') {
      char c = *befehlsformat++;
      if (c=='%') {
	strcpy(write, par);
	write += strlen(par);
      }
      else *write++ = c;
    }
    
    *write=0; // String noch beenden.
    	
    // Nun steht der komplette befehl in meinem Feld befehl[], und ich
    // kann ihn einlasten. Die Befehle werden in der umgekehrten Reihen-
    // folge ausgefuert!!
    // Bevor ich den Befehl einlaste, hoert, hoert, kommt noch eine
    // Kontrollfunkion zu Wort, die das Einlasten bestimmter Befehle
    // verhindert, wenn diese zu einer Fehlermeldung fuerhren wuerden.
    // Dazu gehoeren FE und BO, wenn in dieser Runde schon FE oder
    // BO gemacht wurde.

    if (befehl_darf_eingeschoben_werden(befehl)) befehl_vorschieben(befehl);

  } // while (*befehlsformat)
}


/**---------------------------------------------------------------------------
  * EINHEIT::befehl_darf_eingeschoben_werden()
  * 
  * Wird nur von alarm_verarbeiten aufgerufen und testet, ob der Befehl
  * nicht sehr unguenstig zum vorschieben waere. Z.B. Feuern, wenn
  * diese Runde schon gefeuert war.
  * @param
  * Vollstaendiger Befehlsstring.
  * @return
  * short 1, wenn gruenes Licht, sonst 0.
  ---------------------------------------------------------------------------*/
short EINHEIT::befehl_darf_eingeschoben_werden(char *befehl)
{
  if (!mystrncmp_no_case(befehl,L("FE","FI"),2) && attribut_gesetzt("Gefeuert"))
    return 0;
  else if (!mystrcmp_no_case(befehl,L("BO","BO")) && attribut_gesetzt("Gebombt"))
    return 0;
  else if (!mystrncmp_no_case(befehl,L("SC","SC"),2)) return sc_pruefen(befehl+2);
  else return 1;
}


/**---------------------------------------------------------------------------
  * EINHEIT::sc_pruefen()
  * 
  * Den SCHRITT-IN-EINE-RICHTUNG-Befehl schiebe ich nur ein, wenn ich
  * das Zielfeld auch betreten kann. Sonst will ein Schiff z.B. immer
  * in Richtung Land fahren, kommt aber garnicht hin.
  ---------------------------------------------------------------------------*/
short EINHEIT::sc_pruefen(char *richtung)
{
  RIC ric(richtung);
  ADR ziel = welt()->adresse_in_richtung(adresse, ric);
  
  if (pruefe_bewegung_zu_feld(ziel, 1)) return 0;
  else return 1;
}
  

/**---------------------------------------------------------------------------
  * EINHEIT::objekte_der_umgebung_pruefen()
  * 
  * Prueft alle Objekte, die sich in der Umgebung der Einheit befinden,
  * da ihre Existenz einen Alarm erfordern koennte. Die noetigen Alarms
  * werden dann ausgeloest.
  * Hier wird auch der diplomatische Status geprueft, den unser Staat
  * gegenueber einem anderen einnimmt. Nur bei feindlichen Einheiten
  * wird ein Alarm ausgeloest.
  * @return
  * Liste alle Objekte in der Umgebung. Diese kann fuer weitere
  * Zwecke verwendet werden. Ihre Erstellung ist ein grosser Aufwand!
  ---------------------------------------------------------------------------*/
DOUBLIST *EINHEIT::objekte_der_umgebung_pruefen()
{
  // Zuerst mal alles in der Umgebung holen, falls das geht.

  if (!ort_ist_welt()) return NULL; // Dann geht's (noch) nicht.
  
  DOUBLIST *objektliste = 
	welt()->alle_objekte_im_umkreis_von(adresse,sichtweite);

  FOR_EACH_OBJEKT_IN (objektliste)
  DO
  (
    /* Interessant sind nur gegnerische Objekte. */

    if (wir_hassen(objekt)) {

      /* Jetzt ist der Typ interessant. Und ausserdem die Adresse des
	 Objektes. */

      ADR adr = objekt->adresse;
      char *ric_str = (welt()->richtung_von_nach(adresse, adr)).to_string();
      char *typ = objekt->attribut("TYP");

      if (!strcmp(typ,"STADT")) {
	if (adr == adresse) alarmiert_werden(A_FREMDE_STADT);
	else alarmiert_werden(A_FREMDE_STADT_IN_SICHT, ric_str);
      }

      else if (!strcmp(typ,"EINHEIT")) {
	if (adr == adresse) /* Feind auf gleichem Feld */
	{
	  if (objekt->attribut_gesetzt("Luft")) 
	    alarmiert_werden(A_FLUGZEUG_HIER);
	  else
	    alarmiert_werden(A_FEIND_HIER);
	}

	else if (welt()->benachbart(adr, adresse)) /* Beruehrt die Einheit */
	{
	  if (objekt->attribut_gesetzt("Luft"))
	    alarmiert_werden(A_FLUGZEUG_KONTAKT, ric_str);
	  else if (!objekt->attribut_gesetzt("Militaerisch"))
	    alarmiert_werden(A_KAPERBARE_EINHEIT, ric_str);
	  else
	    alarmiert_werden(A_FEINDKONTAKT, ric_str);
	}

	else  /* Feind im Sichtbereich, aber nicht in Beruehrung. */
	{
	  if (objekt->attribut_gesetzt("Luft"))
	    alarmiert_werden(A_FLUGZEUG_IN_SICHT, ric_str);
	  else
	    alarmiert_werden(A_FEIND_IN_SICHT, ric_str);
	}
      }
    }
  ) /* FOR_EACH_OBJEKT_IN... */

  return objektliste;
}


/**---------------------------------------------------------------------------
  * EINHEIT::weitere_alarmursachen_pruefen()
  * 
  * Prueft weitere moegliche Alarmquellen, z.B. das Feld, auf dem
  * sich die Einheit befindet.
  ---------------------------------------------------------------------------*/
void EINHEIT::weitere_alarmursachen_pruefen()
{
  // Fuer den Alarm A_BODENSCHATZ_VERMUTET muss ich pruefen, ob dieses
  // Feld der Welt schon nach Bodenschaetzen untersucht wurde.
  
  // Den Alarm A_KEINE_STRASSE loese ich aus, wenn auf dem Feld keine
  // Strasse ist und es gleichzeitig Boden ist.

  if (ort_ist_welt())
  {
    if (!welt()->feld_attribut(adresse, FELD_ATTRIBUT_BODEN_UNTERSUCHT))
	alarmiert_werden(A_BODENSCHATZ_VERMUTET);
    if ((!welt()->feld_attribut(adresse, FELD_ATTRIBUT_STRASSE))
       && (!mystrcmp(gelaende_attribut("ART"),"Boden")))
	 alarmiert_werden(A_KEINE_STRASSE);
  }
}
