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
  * MODUL:               stadtaus.C / STADTAUS.CPP
  * AUTOR/DATUM:         Mathias Kettner, 6. Mai 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Enthaelt die wenigen Funktionen der Klasse STADTAUSBAU.
//
// **************************************************************************

#include <string.h>

#include "stadtaus.h"
#include "prom.h"
#include "kompatib.h"
#include "dauer.h"
#include "log.h"

extern DOUBLIST globale_objekt_menge;
extern EINFLUSS_LISTE globale_einfluss_menge;

/**---------------------------------------------------------------------------
  * STADTAUSBAU::STADTAUSBAU()     // constructor
  ---------------------------------------------------------------------------*/
STADTAUSBAU::STADTAUSBAU(char *n, char*a) : OBJEKT(n, a)
{
  //  Wenn ich nur geladen werde, dann muss ich garnichts tun.

  if (attribut_gesetzt("++LADEN++")) return;

  // Hier muss ich in der Enzyklopaedie meinen Eintrag finden und dann
  // dementsprechend die Beeinflussungen setzen.

  if (!enzyklopaedie()) return; // Dann ist eh' alles am dampfen...

  VORHABEN_ENZ *eintrag = enzyklopaedie()->vorhaben(attribut("GATTUNG"));
  if (!eintrag) {
      log('K', "Missing entry %s in game configuration file",
	  attribut("GATTUNG"));
      return; // Dann mach ich eben nichts, ausser zu existieren...
  }

  // Und jetzt uebe ich noch einen Einfluss auf meinen Besitzer aus.
  // Welcher Einfluss dies ist, steht in der Enzyklopaedie unter einfluss_art
  // , einfluss_name, einfluss_attr, einfluss_besitzer, einfluss_parameter

  beeinflussen(eintrag->einfluss_art, eintrag->einfluss_spez,
	       eintrag->einfluss_parameter);

}

/**---------------------------------------------------------------------------
  * STADTAUSBAU::naechste_phase()
  * 
  * Die Stadtausbaut hat praktisch nichts zu tun, als sich zu ver-
  * sorgen.
  *
  * @param
  * long runde: Laufende Runde.
  ---------------------------------------------------------------------------*/
void STADTAUSBAU::naechste_phase(long runde)
{
  if (runde == 1) einfluesse_pruefen(); // Wegen Staatsformaenderung
  
  if (runde == RUNDE_VERSORGUNG_STADTAUSBAU) {
    unterhalt_einholen();
    einfluesse_pruefen();
  }
}


/**---------------------------------------------------------------------------
  * STADTAUSBAU::zug_abschliessen()
  * 
  * Berechnet bei Sehenswuerdigkeiten je nach dem Alter in Jahren einen
  * Einfluss an Bonuspunkten auf den Staat.
  ---------------------------------------------------------------------------*/
void STADTAUSBAU::zug_abschliessen(long)
{
  if (attribut_gesetzt("Bonuspunkte")) {
    long diese_runde = myatol(objekt_mit_namen("Uhr")->info("ZUGNUMMER"));
    long baurunde = myatol(attribut("BAURUNDE"));
    long alter = jahr_in_zug(diese_runde+1) - jahr_in_zug(baurunde+1);

    globale_einfluss_menge.einfluesse_loeschen(this, "BONUSPUNKTE");

    char ziel[80];
    sprintf(ziel, "%s,,,", besitzer->besitzer->name); // Mein Staat.
    beeinflussen("BONUSPUNKTE",ziel, myltoa(alter / 10));
  }
}


/**---------------------------------------------------------------------------
  * STADTAUSBAU::einfluesse_pruefen()
  * 
  * Aktiviert und Suspendiert die Einfluesse je nach Umstaenden.
  ---------------------------------------------------------------------------*/
void STADTAUSBAU::einfluesse_pruefen()
{
  if (attribut_gesetzt("DM_Verboten") && einfluss_vorhanden("DEMOKRATIE"))
    einfluesse_suspendieren();
  else if (attribut_gesetzt("Unversorgt"))
    einfluesse_suspendieren();
  else einfluesse_aktivieren();
}


/**---------------------------------------------------------------------------
  * STADTAUSBAU::unterhalt_einholen()
  * 
  * Wird irgenwann waehrend des Zuges aufgerufen und dient dazu, die
  * Einrichtung mit den noetigen Resourcen zu versorgen. Dazu wird
  * bei der Stadt ein Kommando aufgerufen. Wenn nicht genung Re-
  * sourcen da sind, dann suspendiert die Ausbaut ihre Einfluesse.
  * Wenn sie laengere Zeit unversorgt ist, geht sie mit jeweils 10%
  * kaputt.
  ---------------------------------------------------------------------------*/
void STADTAUSBAU::unterhalt_einholen()
{
  // Wieviele Resourcen ich brauche, das erfahre ich aus der Enzyklopaedie
  if (!enzyklopaedie()) return; // Nichts mehr zu retten!

  VORHABEN_ENZ *eintrag = enzyklopaedie()->vorhaben(attribut("GATTUNG"));
  if (!eintrag) { // Kann doch was nicht strimmen!
      log('K', "Missing entry %s in game configuration file",
	  attribut("GATTUNG"));
	 return;
  }
  if (eintrag->unterhalt.ist_null()) return; // Dann brauche ich garnichts.

  char *res = eintrag->unterhalt.to_string();

  if (besitzer->kommando("RESOURCEN_ANFORDERN", (void *)res)) {
    // Nicht bekommen: Ich bin Unversorgt. Wenn das eine Neuigkeit ist,
    // dann teile ich das der Stadt mit. Ansonsten wuerfle ich, ob das
    // Bauwerk verfaellt (12.5%)

    if (attribut_gesetzt("Unversorgt")) // War schon letzte Runde unversorgt
    {
      if (!io_random(8)) { // Bauwerkt geht kaputt
        besitzer->report(L("Die Stadt kann %s","The Town cannot upkeep %s"),
  	   konjugation(attribut("GATTUNG"), AKKUSATIV | SINGULAR));
        besitzer->report(L(" mangels Versorgung nicht mehr halten.\n","any longer  due to lack of supply.\n"));
        zur_vernichtung_vormerken();
      }
    }
    else { // Bis jetzt noch nicht Unversorgt -> Meldung ausgeben
      attribut_setzen("Unversorgt");
      besitzer->report(L("Die Stadt mu~s die Versorgung %s einstellen.\n","The Town has to cancel the supply of %s.\n"),
      konjugation(attribut("GATTUNG"), GENITIV | SINGULAR));
    }
  } // Versorgung nicht bekommen.

  else if (attribut_gesetzt("Unversorgt")) // War unversorgt, wird aber wieder.
  {
    besitzer->report(L("Die Stadt kann %s wieder versorgen.\n","The Town is now able to supply  %s again.\n"),
      konjugation(attribut("GATTUNG"), AKKUSATIV | SINGULAR));
    attribut_loeschen("Unversorgt");
  }
}


/**---------------------------------------------------------------------------
  * STADTAUSBAU::kommando()               // virtuell
  ---------------------------------------------------------------------------*/
short STADTAUSBAU::kommando(const char *kommando, const void *par1, const void *par2, const void *)
{
  if (!strcmp("BESCHUSS",kommando)) return kommando_beschossen_werden((void *)par1, (void *)par2);
  else return 1;
}


/**---------------------------------------------------------------------------
  * STADTAUSBAU::kommando_beschossen_werden()
  * 
  * Wird von der Stadt aufgerufen, wenn sie beschossen wird und diese
  * Ausbaut getroffen wurde. Die Stadtausbaut vernichtet sich, wenn
  * sie nicht standhalten konnte.
  * @return
  * short 0, wenn das Bauwerk noch steht.
  * short stabilitaet, wenn es vernichtet wurde.
  ---------------------------------------------------------------------------*/
short STADTAUSBAU::kommando_beschossen_werden(void *par1, void *)
{
  // Zuerst hole ich mir die Feuerkraft

  long feuerkraft = *(long *)par1;
  long stabilitaet = myatol(attribut("STABILITAET"));

  // Nun muss ich irgendwie entscheiden, wieviel mir dass eigentlich
  // ausmacht.

  short steht_noch = ((io_random(feuerkraft+stabilitaet)) >= feuerkraft);

  if (!steht_noch) {
    zur_vernichtung_vormerken();
    return stabilitaet;
  }
  else return 0;
}
