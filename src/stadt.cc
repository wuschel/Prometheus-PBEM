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
  * MODUL:               stadt.C / STADT.CPP
  * AUTOR/DATUM:         Mathias Kettner, 4. Mai 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------
  *
  * Funktionen zur Klasse STADT, die das Objekt Stadt realisiert und
  * zur Klasse VORHABEN, das ein Bau- oder Forschungsvorhaben einer
  * Stadt realisiert.
  *
  ---------------------------------------------------------------------------*/

#include <string.h>
#include <math.h>
#include <ctype.h>

#include "uhr.h"
#include "alg.h"
#include "laengen.h"
#include "dauer.h"
#include "stadt.h"
#include "prom.h"
#include "enzyklop.h"
#include "welt.h"
#include "einfluss.h"
#include "kompatib.h"
#include "staat.h"
#include "weltbaut.h"
#include "drucker.h"
#include "layout.h"
#include "listmac.h"
#include "html.h"
#include "log.h"

extern short laser; // Ist 1, wenn beim Ausdruck der Laserer verwendet werden
		    // soll

extern EINFLUSS_LISTE globale_einfluss_menge;
extern DOUBLIST globale_objekt_menge;

#define prozent(x,a) (long( ((1000*(x)+5)/(a))/10 ) )
#define bonuspunkte(n) (!staat()->kommando("BONUSPUNKTE",myltoa(n)))

extern ENZYKLOPAEDIE *g_enzyklopaedie; // aus main.cpp
extern UHR *g_uhr;		       // aus main.cpp

/**---------------------------------------------------------------------------
  * STADT::STADT()                 // constructor
  ---------------------------------------------------------------------------*/
STADT::STADT(char *name, char *attr) : OBJEKT(name, attr)
{
    vorige_phase = 0;
    bahnkontor_protokoll = NULL;
    anzahl_kaempfe = 0;
    
    benoetigte_nahrung = arbeitskraft_einnahmen = 0;
    forschungszuschuss_defacto = forschungszuschuss = 0;

    // Das Attribut SPEICHERUNG gibt an, wieviele Resourcen die Stadt speichern
    // kann. Mit bestimmte Bauten kann das natuerlich erhoeht werden
    attribut_setzen("SPEICHERUNG",
		    L("9999H9999S9999M9999I9999X9999N","9999W9999S9999R9999I9999X9999F"));

    if (attribut_gesetzt("++LADEN++")) return;

  // Wenn nicht geladen wird, dann kommt die eigentliche Initialisierung.
  // Die Stadt muss der Welt mitteilen, dass sie das Feld unter ihr
  // bewirtschaften will.  Auf diese Art wird auch gleichzeitig ver-
  // hindert, dass zwei Staedte auf dem selben Feld stehen koennen.
  // Ausserdem braucht die Stadt am Anfang schliesslich eine Einnahme.
  // Die Startposition ist in den Attributen STARTX und STARTY festge-
  // legt (hoffentlich!)

  if (!attribut_gesetzt("STARTX") || !attribut_gesetzt("STARTY"))
  {
      log('I', "STADT::STADT(): Startattribute STARTX und STARTY fehlen bei der Stadt %s", name);
      zur_vernichtung_vormerken();
      return;
  }

  OBJEKT *welt = objekt_mit_namen(attribut("WELTNAME"));
  if (!welt) {
      log('I', "STADT::STADT(): Stadt %s: Keine Welt mit Name %s existiert!",
	  name, attribut("WELTNAME"));
      zur_vernichtung_vormerken();
      return;
  }

  long xpos = atol(attribut("STARTX"));
  long ypos = atol(attribut("STARTY"));
  ADR start(xpos, ypos);

  welt->kommando("FELD_BEWIRTSCHAFTEN", &start);

  // Und nun kommt das Feld noch in meine Liste der bewirtschafteten Felder.

  ADR_LIST_NODE *adrnode = new ADR_LIST_NODE;
  adrnode->adresse = start;
  bewirtschaftete_felder.add_head(adrnode);

  // Die Stadt muss noch an den richtigen Fleck
  ort_wechseln(welt, start);

  // Wieviele Einwohner die Stadt am Anfang hat, kann ein Attribut festlegen.
  // Ich muss aber 1500 fuer die Buergerwehr abziehen.
  if (attribut_gesetzt("EINWOHNERZAHL"))
  {
    einwohneraenderung = einwohnerzahl = myatol(attribut("EINWOHNERZAHL")) - 1500;
    attribut_loeschen("EINWOHNERZAHL");
  }
  else einwohneraenderung = einwohnerzahl = 13500;

  // Moeglicherweise faengt die Stadt bereits mit Guetern an.
  if (attribut_gesetzt("ANFANGSGUETER"))
  {
    resourcenspeicher.setzen_auf(attribut("ANFANGSGUETER"));
    attribut_loeschen("ANFANGSGUETER");
  }


  // Das Attribut "Basis" hat jede Stadt automatisch. Das heisst, dass
  // jede Stadt als Stuetzpunkt fuer Flugzeuge in Frage kommt.
  attribut_setzen("Basis");

  // Die Stadt bekommt nun die Gattung Siedlung. Beachte, dass ich die
  // Funktion gattung_berechnen() noch nicht verwenden darf, da im
  // Konstruktor die Objektrelationen noch nicht hergestellt sind, und
  // die obige Funktion die Funktion staat() benoetigt, welche als
  // Besitzer der Staat einen Staat annimmt. Wenn diese Stadt jedoch
  // im Konstruktor des Staates gegruendet wurde, dann ist sie die
  // Hauptstadt. Sie hat dann schon eine Gattung, und ich lasse die
  // Attribute stehen.

  if (!attribut_gesetzt("GATTUNG")) {
    attribut_setzen("GATTUNG",L("Lager","Camp"));
    attribut_setzen("GRAFIK","stadta");
  }
  
  // Verteidigungsbonus in der Stadt
  beeinflussen("SM",",TYP=EINHEIT,,0:!t" ,"100");

  // Jetzt baut die Stadt noch eine Buergerwehr, die auch sogleich fertig
  // wird und zudem eingegraben ist.

  OBJEKT *neu = objekt_schaffen(eindeutiger_name(), "EINHEIT",
	L("GATTUNG=B~urgerwehr,Eingegraben,GRAFIK=stadtwc,NichtKaperbar","GATTUNG=Volunteer_Forces,Eingegraben,GRAFIK=stadtwc,NichtKaperbar"));
  neu->attribut_setzen("BAUJAHR", g_uhr->info("ZUGNUMMER"));

  neu->ort_wechseln(this);      // Nun ist sie kurzzeitig in der Stadt...
  neu->verlassen();             // ... aber jetzt verlaesst sie sie schon!
}

/**---------------------------------------------------------------------------
  * STADT::~STADT()                 // destructor
  ---------------------------------------------------------------------------*/
STADT::~STADT()
{
    if (bahnkontor_protokoll) {
	myfree(bahnkontor_protokoll);
	bahnkontor_protokoll = NULL;
    }
    
  // Ich muss die Liste der bewirtschafteten Felder von Hand loeschen, damit
  // auch die Welt davon in Kenntnis gesetzt wird.

  if (!ort()) return;

  // Die Suche nach der Welt geschieht auf etwas seltsame weise. Dies muss
  // ich aber so machen, da ich nicht weiss, ob sich die Welt im Destruktor
  // befindet und vielleicht schon die Matrix abgebaut hat!

  OBJEKT *welt = objekt_mit_namen(attribut("WELTNAME"));
  if (!welt) return;

  // Wegen der Welt muss ich alle bewirtschafteten Felder explizit freigeben
  while (!bewirtschaftete_felder.is_empty())
  {
    if (!feld_nicht_mehr_bewirtschaften(
    ((ADR_LIST_NODE *)bewirtschaftete_felder.first())->adresse)) break;
  }
}


/**---------------------------------------------------------------------------
  * STADT::laden()                        // virtuell
  ---------------------------------------------------------------------------*/
short STADT::laden(FILE *file)
{
  // Vermischte Daten...
  long dummy;

  char resstring[MAX_LAENGE_RESOURCEZEILE+2];
  fscanf(file, "%ld%ld%ld%ld%ld%ld%ld%s%ld", &einwohnerzahl, &dummy, &dummy,
		&dummy, &dummy, &dummy, &dummy,
		resstring, &dummy);
  resourcenspeicher.setzen_auf(resstring);
  if (ferror(file)) return 1;

  // Nun lade ich die Liste aller von mir bewirtschafteten Felder...
  adressliste_laden(file, &bewirtschaftete_felder);

  // Nun lade ich noch die Liste der Vorhaben, die gerade laufen.
  // Dabei muss ich darauf achten, dass sie in der richtigen Reihen-
  // folge eingelesen werden. Ich muss dehalb immer hinten anhaengen!

  long anzahl;
  fscanf(file, "%ld", &anzahl);
  VORHABEN *vorhaben;
  for (;anzahl;anzahl--) {
    vorhaben = new VORHABEN(file);
    if (ferror(file)) return 1;
    vorhabenliste.add_tail(vorhaben);
  }
  return 0;
}

/**---------------------------------------------------------------------------
  * STADT::speichern()                    // virtuell
  ---------------------------------------------------------------------------*/
short STADT::speichern(FILE *file)
{
  // Vermischte Daten...

  fprintf(file, "%ld %ld %ld %ld %ld %ld %ld %s %ld\n",einwohnerzahl, 0l,
   0l, 0l, 0l, 0l, 0l,
   resourcenspeicher.to_string(), 0l);

  if (ferror(file)) return 1;

  // Als naechstes muss ich die Liste aller Felder speichern, die ich
  // bewirtschafte.
  adressliste_speichern(file, &bewirtschaftete_felder);

  // Und nun speichere ich noch eine Liste aller Vorhaben, die gerade
  // laufen.
  fprintf(file,"\n%ld ", vorhabenliste.number_of_elements());
  VORHABEN *vorhaben = (VORHABEN *)vorhabenliste.first();
  while (!vorhaben->is_tail())
  {
    if (vorhaben->speichern(file)) return 1; // Fehler!
    vorhaben = (VORHABEN *)vorhaben->next();
  }
  return 0;
}


/**---------------------------------------------------------------------------
  * STADT::gattung_berechnen()
  * 
  * Die Gattung der Stadt richtet sich nach ihrer Groesse und
  * nach ihren Industriellen Einrichtungen. Nur die Hauptstadt
  * hat immer die Gattung Hauptstadt.
  ---------------------------------------------------------------------------*/
void STADT::gattung_berechnen()
{
  char *gattung, *grafik;

  // Ich frage meinen Staat, ob ich seine Hauptsstadt bin.
  if (bin_hauptstadt()) {
    // Ich bin die Hauptstadt
    gattung = L("Hauptstadt","Capital City");
    grafik = "stadtx";
  }

  else if (einwohnerzahl == 0) { gattung = L("Ruinen von","Ruins of"); grafik="ruinen"; }

  else if (einwohnerzahl <  15000L) {gattung=L("Lager","Camp");        grafik="stadta"; }
  else if (einwohnerzahl <  25000L) {gattung=L("Siedlung","Settlement");     grafik="stadtb"; }
  else if (einwohnerzahl <  60000L) {gattung=L("Dorf","Hamlet");         grafik="stadtc"; }
  else if (einwohnerzahl < 100000L) {gattung=L("Gemeinde","Village");     grafik="stadtd"; }
  else if (einwohnerzahl < 200000L) {gattung=L("Kleinstadt","Small Town");   grafik="stadte"; }
  else if (einwohnerzahl < 300000L) {gattung=L("Stadt","Town");        grafik="stadtee";}
  else if (einwohnerzahl < 500000L) {gattung=L("Gro~sstadt","Large Town");   grafik="stadtf"; }
  else if (einwohnerzahl <1000000L) {gattung=L("Metropole","Metropole");    grafik="stadtg"; }
  else                            {gattung = L("Megalopolis","Megalopolis");  grafik="stadth"; }

  attribut_setzen("GATTUNG",gattung);
  attribut_setzen("GRAFIK",grafik);
}


/**---------------------------------------------------------------------------
  * STADT::befehl_auswerten()             // virtuell
  ---------------------------------------------------------------------------*/
short STADT::befehl_auswerten(char *befehl, long)
{
  // Falls die Stadt keine Einwohner mehr hat, koennen auch keine Befehle
  // angenommen werden.

  if (einwohnerzahl == 0 && *befehl!='*') {
    report(L("Der Befehl '%s' kann nicht ausgef~uhrt werden, da die ","The command '%s' cannot be exectuted because your "), befehl);
    report(L("Stadt leer und verlassen ist!\n","town is empty and deserted!\n"));
    return 1;
  }

  if (ausnahmezustand()) {
      report(L("Die Stadt ist im Ausnahmezustand and v~ollig unregierbar. Keiner h~ort auf deine Befehle.\n","The town is in state of emergency and out of control. Nobody does listen to your commands.\n"));
      return 1;
  }



  if      (!mystrncmp(L("BA","BU"),befehl,2))  return befehl_bauen(befehl);
  else if (!mystrncmp("*B",befehl,2))  return befehl_bauen(befehl);
  else if (!mystrncmp("B-",befehl,2))  return befehl_bauen(befehl);
  else if (!mystrncmp("*E",befehl,2))  return befehl_mehr_einwohner(befehl);
  else if (!mystrncmp("*R",befehl,2))  return befehl_mehr_resourcen(befehl);
  else if (!mystrncmp(L("AB","AP"),befehl,2))  return befehl_bau_abbrechen(befehl);
  else if (!mystrncmp("A-",befehl,2))  return befehl_bau_abbrechen(befehl);
  else if (!mystrncmp(L("AR","RB"),befehl,2))  return befehl_abreissen(befehl);
  else if (!mystrncmp(L("FB","SP"),befehl,2))  return befehl_feld_bewirtschaften(befehl+2);
  else if (!mystrncmp(L("NB","UP"),befehl,2))  return befehl_feld_nicht_mehr_bewirtschaften(befehl+2);
  else if (!mystrncmp(L("NR","RF"),befehl,2))  return befehl_nahrung_rationieren();
  else if (!mystrncmp(L("AL","RC"),befehl,2))  return befehl_aufloesen(befehl);
  else if (!mystrncmp(L("NA","CN"),befehl,2))  return befehl_name(befehl);
  else if (!mystrncmp(L("BE","RO"),befehl,2))  return befehl_bewirtschaftung_einschraenken(befehl);
  else if (!mystrncmp(L("GV","DC"),befehl,2))  return befehl_gueter_verschicken(befehl);
  else if (!mystrncmp(L("SV","PD"),befehl,2))  return befehl_gueter_verschicken(befehl);
  else if (!mystrncmp(L("AV","UA"),befehl,2))  return befehl_abkuerzung_verwenden(befehl);

  else return unbekannter_befehl(befehl);
}



/**---------------------------------------------------------------------------
  * STADT::befehl_abkuerzung_verwenden()
  * 
  * Setzt fuer eine Abkuerzung einen Text ein, der beim Staat mit
  * ADA bis ADZ definiert werden kann.
  ---------------------------------------------------------------------------*/
short STADT::befehl_abkuerzung_verwenden(char *befehl)
{
  char kennung = befehl[2];
  if (kennung < 'A' || kennung > 'Z' || befehl[3]) {
    report(L("*** %s: Es sind nur die Befehle AVA bis AVZ m~oglich. ***\n","*** %s: Only possible to use commands from UAA to UAZ. ***\n"),
      befehl);
    return 1;
  }
  staat()->abkuerzung_expandieren(this, kennung);
  return 1;
}


/**---------------------------------------------------------------------------
  * STADT::befehl_gueter_verschicken()
  * 
  * Schickt Gueter mit der Eisenbahn oder stellt den stehenden Befehl
  * ein.
  * @param
  * "GVSIL,50N10E,HGA,50H,..."
  * oder "SVSIL,50N10E,HGA,50H,..."
  * @return
  ---------------------------------------------------------------------------*/
short STADT::befehl_gueter_verschicken(char *befehl)
{
  if (!einfluss_vorhanden("EISENBAHN")) {
    report(L("*** Die Befehle GV und SV kann man erst mit der Erfindung der ","*** You need the Railway to use the commands DC and PD. "));
    report(L("Eisenbahn verwenden. ***\n","Did you get that?. ***\n"));
    return 1;
  }

  short stehend = (!mystrncmp(befehl, L("SV","PD"), 2));
  char *parameter = befehl+2;

  // Der stehende Befehl geht erstmal immer

  if (stehend) {
    if (!*parameter) {
      report(L("Wir l~oschen den stehenden Befehl zum G~uterverschicken.\n","We undo the permanent PD (Dispatch) Command.\n"));
      attribut_loeschen("SV");
    }
    else {
      attribut_setzen("SV", parameter);
      report(L("Wir ~andern den stehenden Befehl zum G~uterverschicken auf %s.\n","We change the PD Command to %s.\n"),
         befehl);
      }
    return 1;
  }
 
  else gueter_verschicken(parameter);
  return 1;
} 


/**---------------------------------------------------------------------------
  * STADT::gueter_verschicken()
  * 
  * Nur mit Eisenbahn. Verschickt Gueter und Personen zu einer anderen
  * Stadt mit Bahnhof.
  ---------------------------------------------------------------------------*/
void STADT::gueter_verschicken(char *parameter)
{
  if (!einfluss_vorhanden("BAHNHOF"))  {
      
      report(L("*** G~uter verschicken %s: Entweder besitzen wir keinen Bahnhof, ","*** Dispatch commodities: %s: Either we don´t have any Railway Stations or ")
	     L("oder unser Bahnhof ist unversorgt. ***\n","the Station is not supplied.***\n"),
	     parameter);
      return;
  }

  char *copy, *zeiger;
  copy = mystrdup(parameter);
  zeiger = copy;
  
  // Ich scanne der Reihe nach alle Paare Ziel, Menge, Ziel, Menge,...

  short zuende = 0;
  while (*zeiger && !zuende) {
    char *zielname = zeiger;
    while (*zeiger && *zeiger!=',') zeiger++;
    if (!*zeiger) {
      report(L("*** G~uter verschicken %s: G~uterangabe fehlt. ***\n","*** Dispatch commodities %s: Please state which commodities!. ***\n"), parameter);
      break;
    }
    *zeiger++ = 0; // Hier muesste ein Komma stehen.
    
    char *guetermenge = zeiger;
    while (*zeiger && *zeiger!=',') {
      if (*zeiger == L('P','P'))
	  *zeiger = L('E','E'); // Aus "Personen" Energie machen.
      zeiger++;
    }

    if (*zeiger == 0) zuende = 1;
    else *zeiger++ = 0;
    
    OBJEKT *ziel = objekt_mit_namen(zielname);
    if (!ziel || !ziel->typ_ist("STADT")) {
      report(L("*** G~uter verschicken %s: Es gibt keine Stadt mit der ","*** Dispatch commodities: %s: There is no city with an "), 
        parameter);
      report(L("Abk~urzung %s. ***\n","Abbreviation %s. ***\n"), zielname);
      continue;
    }
  
//    if (!uns_liebt(ziel)) {
//      report(L("*** G~uter verschicken %s: Die Stadt ","*** Dispatch commodities: %s: The town  "), parameter);
//      report(L("%s weigert sich, unsere Bahn einfahren zu lassen. ***\n","%s denies entry to our railway. ***\n"),
//         ziel->a_name());
//      continue;
//   }

    if (!ziel->einfluss_vorhanden("BAHNHOF")) {
      report(L("*** G~uter verschicken %s: Entweder besitzt die Zielstadt ","*** Dispatch commodities: %s: Either target city "), parameter);
      report(L("%s keinen Bahnhof, oder er ist unversorgt. ***\n","%s does not have a Station or the Station is not supplied. ***\n"), ziel->a_name());
      continue;
    }

    if (!es_gibt_eisenbahn_zu(ziel)) {
      report(L("*** G~uter verschicken %s: Von hier aus gibt es keine ","*** Dispatch commodities: %s:  There is no line of rails from here "), parameter);
      report(L("Eisenbahn zu %s. ***\n","to %s. ***\n"), ziel->a_name());
      continue;
    }
  
    // Nun die Gueter errechnen.
    RESOURCE_VEKTOR gueter(guetermenge);
    long personen = gueter[L('E','E')];
    if (personen < 0 || gueter.ist_negativ()) 
    {
      report(L("*** G~uter verschicken %s: Keine negative Angaben erlaubt. ***\n","*** Dispatch commodities: %s: No negative values allowed!. ***\n"),
        parameter);
      continue;
    }

    long min_einwohner = myatol(lexikon_eintrag("einwohnerzahl_fuer_bahn",1));
    if (personen > 0 && einwohnerzahl - personen < min_einwohner)
    {
      if (einwohnerzahl > min_einwohner) personen = einwohnerzahl-min_einwohner;
      else personen = 0;
      report(L("*** G~uter verschicken %s: ","*** Dispatch commodities: %s: "), parameter);
      report(L("%s Einwohner m~ussen in der Stadt zur~uckbleiben. ***\n","%s Citizens have to stay in town. ***\n"),
        myltoa(min_einwohner));
    }
 
    RESOURCE_VEKTOR grenze(L("999999N99999H99999S99999X99999F99999I99999K99999M","999999F99999W99999S99999X99999C99999I99999U99999R"));
    gueter.begrenzen_auf(grenze);

    RESOURCE_VEKTOR aus_eigener_kasse(resourcenspeicher);
    aus_eigener_kasse.begrenzen_auf(gueter);
    resourcenspeicher.subtrahiere(aus_eigener_kasse);
    
    RESOURCE_VEKTOR vom_kontor_zu_besorgen(gueter);
    vom_kontor_zu_besorgen.subtrahiere(aus_eigener_kasse);
    
    RESOURCE_VEKTOR vom_kontor;
    besorge_ueber_bahnkontor(vom_kontor_zu_besorgen, MOEGLICHST_VIEL, vom_kontor);
   
    gueter.setzen_auf(vom_kontor);
    gueter.addiere(aus_eigener_kasse);
    
    report(L("Nach %s ","To  %s "), ziel->a_name());
    if (!personen && gueter.ist_null())
      report(L("wird nichts verschickt.\n","nothing is dispatched.\n"));
    else if (!personen)
      report(L("werden %s verschickt.\n","%s have been dispatched.\n"), gueter.to_string());
    else if (gueter.ist_null())
      report(L("werden %s Personen verschickt.\n","%s people have been transferred. \n"), myltoa(personen));
    else {
      report(L("werden %s und ","%s and "), gueter.to_string());
      report(L("%s Personen verschickt.\n","%s people have been dispatched.\n"), myltoa(personen));
    }
    
    ziel->kommando("EISENBAHN_BRINGT", &gueter, &personen, this);
    einwohnerzahl -= personen;
    einwohneraenderung -= personen;
  }

  myfree(copy);
}

/**---------------------------------------------------------------------------
  * STADT::gueterverkehr_abwickeln()
  * 
  * Von naechster_runde() aufgerufen und beruecksichtigt den stehenden
  * Befehl SV, wenn es einen gibt.
  ---------------------------------------------------------------------------*/
void STADT::gueterverkehr_abwickeln()
{
  char *sv = attribut("SV");
  if (sv && sv[0]) gueter_verschicken(sv);
}


/**---------------------------------------------------------------------------
  * STADT::entfernungsfunktion()           // static
  * 
  * Zum Test, ob zwei Staedte mit Strassen verbunden sind.
  ---------------------------------------------------------------------------*/
short STADT::entfernungsfunktion(void *welt_void, ADR&, ADR& ziel)
{
  WELT *welt = (WELT *)welt_void;

  // Falls Strasse auf dem ziel, dann eins, sonst 0.
  if (welt->feld_attribut(ziel, FELD_ATTRIBUT_STRASSE)) return 1;
  else return -1;
}

  
/**---------------------------------------------------------------------------
  * STADT::es_gibt_eisenbahn_zu()
  * 
  * Testet, ob es eine Strassenverbindung gibt zu einer anderen Stadt.
  ---------------------------------------------------------------------------*/
short STADT::es_gibt_eisenbahn_zu(OBJEKT *ziel)
{
  // Ich selbst brauche eine Strasse.
  if (!welt()->feld_attribut(adresse, FELD_ATTRIBUT_STRASSE)) return 0;

  // Nun lasse ich mir vom Wegsuchealgorithmus den besten Weg berechnen,
  // nur um zu testen, ob es ueberhaupt einen gibt.
  
  DOUBLIST *besterweg = welt()->schnellster_weg(adresse, ziel->adresse,
    STADT::entfernungsfunktion, (void *)(welt()));

  short s_gibt_n_weg = (besterweg != NULL);
  delete besterweg;
  return s_gibt_n_weg;
}


/**---------------------------------------------------------------------------
  * STADT::mit_der_bahn_erreichbar()
  * 
  * Testet, ob alle Bedingungen erfuellt sind, um mit einer Stadt
  * Gueter ueber die Eisenbahn auszutauschen. Wird fuer den Bahnkontor
  * gebraucht. Die Stadt selbst gilt nicht erreichbar.
  ---------------------------------------------------------------------------*/
short STADT::mit_der_bahn_erreichbar(STADT *zielstadt)
{
  if (this == zielstadt) return 0;
  else return 
         (einfluss_vorhanden("BAHNHOF") &&
          zielstadt->einfluss_vorhanden("BAHNHOF") &&
          einfluss_vorhanden("KONTOR") &&
          zielstadt->einfluss_vorhanden("KONTOR") &&
          es_gibt_eisenbahn_zu(zielstadt));
}


/**---------------------------------------------------------------------------
  * STADT::besorge_ueber_bahnkontor()
  * 
  * Versucht, von anderen Staedten eine Menge von Guetern ueber das
  * Bahnkontor zu bekommen.
  * @param
  * benoetigt: Soviel wird benoetigt.
  * modus = ALLES_ODER_NICHTS, wenn alles oder nichts besorgt werden soll.
  * modus = MOEGLICHST_VIEL, wenn moeglichst viel vom Wunsch besorgt w. s.
  * bekommen: Soviel wurde davon bekommen
  ---------------------------------------------------------------------------*/
void STADT::besorge_ueber_bahnkontor(RESOURCE_VEKTOR& benoetigt, short modus,
			             RESOURCE_VEKTOR& bekommen)
{
  if (modus == ALLES_ODER_NICHTS) 
  {
    // Ich muss testen, ob soviel verfuegbar ist, wie ich brauche.
    // Dazu muss ich alle Staedte fragen.

    RESOURCE_VEKTOR verfuegbar;
    FOR_EACH_OBJEKT_IN(staat()->alle_staedte())
    DO_AND_DELETE({
      if (mit_der_bahn_erreichbar((STADT *)objekt))
        ((STADT *)objekt)->frage_an_bahnkontor(verfuegbar); 
    })
    if (!benoetigt.passt_in(verfuegbar)) return;
  }
  
  RESOURCE_VEKTOR brauche_noch(benoetigt);

  FOR_EACH_OBJEKT_IN(staat()->alle_staedte())
  DO_AND_DELETE({
    if (mit_der_bahn_erreichbar((STADT *)objekt))
      ((STADT *)objekt)->bestellung_an_bahnkontor(brauche_noch, this);
  })
  
  bekommen.setzen_auf(benoetigt);      // Das war die Gesamtforderung
  bekommen.subtrahiere(brauche_noch);  // Das nicht bekommene abziehen
  
}


/**---------------------------------------------------------------------------
  * STADT::frage_an_bahnkontor()
  * 
  * Fragt die Stadt, wieviel Gueter sie mit ihrem Bahnkontor zur Ver-
  * fuegung stellen kann. Die Funktion wird von ANDEREN Staedte auf-
  * gerufen.
  * @param
  * RESOURCE_VEKTOR& ergebnis: Auf diesen Wert wird das verfuegbare
  * einfach draufaddiert.
  ---------------------------------------------------------------------------*/
void STADT::frage_an_bahnkontor(RESOURCE_VEKTOR& ergebnis)
{
  if (!einfluss_vorhanden("KONTOR")) return;
  if (ausnahmezustand()) return;

  RESOURCE_VEKTOR verfuegbar(resourcenspeicher);

  // Arbeitskraft und Energie koennen nicht uebertragen werden.
  verfuegbar[L('A','M')] = 0;
  verfuegbar[L('E','E')] = 0;
  ergebnis.addiere(verfuegbar);
}


/**---------------------------------------------------------------------------
  * STADT::bestellung_an_bahnkontor()
  * 
  * Versucht, von der Stadt ueber ein Bahnkontor Gueter zu bekommen.
  * Die Funktion wird von ANDEREN Staedten aufgerufen.
  * @param
  * benoetigt: Ein- und Ausgabeparameter. Vom benoetigten wird das
  * abgezogen, was bekommen wurde. Am Ende zeigt also benoetigt
  * an, wieviel davon nicht bekommen wurde.
  ---------------------------------------------------------------------------*/
void STADT::bestellung_an_bahnkontor(RESOURCE_VEKTOR& benoetigt, 
				     STADT *empfaenger)
{
  if (!einfluss_vorhanden("KONTOR")) return;
  if (ausnahmezustand()) return;

  RESOURCE_VEKTOR verfuegbar(resourcenspeicher);
  // Keine Arbeitskraft oder Energie kann uebertragen werden.
  verfuegbar[L('A','M')] = 0;
  verfuegbar[L('E','E')] = 0;

  verfuegbar.begrenzen_auf(benoetigt);
  benoetigt.subtrahiere(verfuegbar);
  resourcenspeicher.subtrahiere(verfuegbar);
    
  // Protokollieren
  if (!verfuegbar.ist_null())
  {
    char string[512];
    sprintf(string, L("%s%s an %s","%s%s to %s"), bahnkontor_protokoll == NULL ? "" : ", ",
      verfuegbar.to_string(), empfaenger->a_name());
    mystradd(bahnkontor_protokoll, string);
    sprintf(string, L("%s%s von %s","%s%s from %s"), empfaenger->bahnkontor_protokoll == NULL ?
      "" : ", ", verfuegbar.to_string(), a_name()); 
    mystradd(empfaenger->bahnkontor_protokoll, string);
  }
}
				     
				     
/**---------------------------------------------------------------------------
  * STADT::befehl_bewirtschaftung_einschraenken()
  * 
  *
  * @param
  * "BE100" oder "BE12F" oder sowas.
  ---------------------------------------------------------------------------*/
short STADT::befehl_bewirtschaftung_einschraenken(char *befehl)
{
  long sparen = myatol(befehl+2);
  if (sparen < 0) {
    sparen = -sparen;
    report(L("*** Beim BE-Befehl darf man keinen negativen Wert verwenden. Ich ","*** RO Command:  Hey! Don´t use negative values! ( I do wisely"));
    report(L("~andere daher den Wert auf %s. *** \n","change the value to %s ). *** \n"), myltoa(sparen));
  }
  
  attribut_setzen("A-SPAREN", befehl+2);

  if (sparen && befehl[strlen(befehl) - 1] == L('F','S')) // Anzahl der Felder fest
  {
    report(L("Wir bewirtschaften ab sofort maximal %s Felder.\n","From now on we operate a maximum of %s squares.\n"), myltoa(sparen));
    return 1;
  }

  if (sparen) {
    report(L("Fortan werden %s Arbeitskraft aufgespart ","From now on we save  %s manpower, "),myltoa(sparen));
    report(L("und deshalb eventuell weniger Felder bewirtschaftet, als ","thus we operate fewer squares than "));
    report(L("m~oglich w~are.\n","possible.\n"));
    return 1;
  }
  
  else {
    report(L("Wir sparen nun keine Arbeitskraft mehr f~ur andere Dinge, ","We stop now saving manpower for all sorts of things, "));
    report(L("sondern bewirtschaften soviele Felder, wie m~oglich.\n","and operate as many squares as possible.\n"));
    attribut_loeschen("A-SPAREN"); // sollte ganz weg sein wegen BE0F!
    return 1;
  }
}


/**---------------------------------------------------------------------------
  * STADT::befehl_name()
  * 
  * Aendert schlichtweg den Namen der Stadt.
  ---------------------------------------------------------------------------*/
short STADT::befehl_name(char *befehl)
{
  char *neuer_name = nice_gross_klein(befehl+2);
  if (!neuer_name[0]) return 1;

  attribut_setzen("NAME", neuer_name);
  report(L("Der Stadtrat hat beschlossen, der Stadt den neuen Namen \"","The City Council decided to change the town´s name to "));
  report(L("%s\" zu geben.\n","%s\".\n"), neuer_name);
  return 1;
}
  
  
/**---------------------------------------------------------------------------
  * STADT::befehl_mehr_einwohner()
  * 
  * Fakebefehl, der der Stadt ein Zahl von Einwohnern zuschustert.
  ---------------------------------------------------------------------------*/
short STADT::befehl_mehr_einwohner(char *befehl)
{
  long anzahl = myatol(befehl+2);
  report(L("Wie durch ein Wunder hat die Stadt auf einmal %s mehr Einwohner...\n","It´s a  MIRACLE!  Population suddenly grew by  %s!\n"),
	 myltoa(anzahl));
  einwohnerzahl += anzahl;
  einwohneraenderung += anzahl;
  return 1;
}


/**---------------------------------------------------------------------------
  * STADT::befehl_mehr_resourcen()
  * 
  * Fakebefehl, der der Stadt Ressourcen zuschustert.
  * @param
  * char *befehl: "*R%s",Resourcestring
  ---------------------------------------------------------------------------*/
short STADT::befehl_mehr_resourcen(char *befehl)
{
  RESOURCE_VEKTOR add(befehl+2);
  resourcenspeicher.addiere(add);
  report(L("Auf einmal fallen G~uter vom Himmel (%s).\n","Divine Intervention!!  Mystic commodities  (%s) fall from the sky.\n"),add.to_string());
  return 1;
}

/**---------------------------------------------------------------------------
  * STADT::befehl_nahrung_rationieren()
  * 
  * Toggelt das Attribut "Rationiert", bei dessen Gesetztheit an die
  * Bevoelkerung nur noch das noetigste an Nahrung ausgegeben wird.
  ---------------------------------------------------------------------------*/
short STADT::befehl_nahrung_rationieren()
{
  // Toggle-Befehl, bei dem nichts schiefgehen kann.
  if (attribut_gesetzt("Rationiert")) {
    report(L("Die Nahrungsrationierung in der Stadt wird wieder aufgehoben.\n","The rationing of food in this town is eventually undone.\n"));
    attribut_loeschen("Rationiert");
  }

  else {
    attribut_setzen("Rationiert");
    report(L("Der B~urgermeister ordnet an, die Nahrungsausgabe an die","City Mayor Decree: Dear Citizens! From now on"));
    report(L(" Bev~olkerung auf das notwendigste zu rationiern.\n"," food is rationalized. \n"));
  }
  return 1;
}


/**---------------------------------------------------------------------------
  * STADT::befehl_bauen()
  * 
  * Realisiert den Befehl 'BA', mit dem der Bau einer Einrichtung, das
  * Ausheben einer Einheit, das Entwickeln einer Errungenschaft oder
  * der Bau einer Weltwunders begonnen werden kann.
  *
  * Der Fake-Befehl *B wird auch ueber diese Funktion abgewickelt.
  * Er bedeutet den Bau, mit dem Unterschied, dass keinerlei Abrfagen
  * nach dessen Moeglichkeit gemacht werden, und dass er nichts kostet.
  *
  * Bei B- wird keine Fehlermeldung ausgegeben. Der Befehl kommt vom
  * Staat.
  *
  * @param
  * char *befehl: BA5:Schule oder BA5Schule oder BA2:3Schule
  * @return
  * Ist immer 1, da der Befehl selbst keine Zeit benoetigt
  ---------------------------------------------------------------------------*/
short STADT::befehl_bauen(char *befehl)
{
  long anzahl, prioritaet;
  short fehlermeldung = befehl[1] != '-';

  // Ist eines der beiden letzten Zeichen eine '0', so wird sie durch
  // ein 'O' ersetzt. Das soll Tippfehler korrigieren helfen.

  short l = mystrlen(befehl);
  if (befehl[l-1] == '0') befehl[l-1] = 'O';
  if (befehl[l-2] == '0') befehl[l-2] = 'O';

  char zahl[80], *scan, *write;
  scan = befehl+2, write = zahl;
  while (*scan && isdigit(*scan)) *write++ = *scan++;
  *write = 0;
  if (*scan == ':') {
    scan++;
    prioritaet = myatol(zahl);
    if (prioritaet < 1 || prioritaet > 9) prioritaet = 5;
    write = zahl;
    while (*scan && isdigit(*scan)) *write++ = *scan++;
    *write = 0;
    anzahl = myatol(zahl);
    if (anzahl < 1) anzahl = 1;
  }

  else {
    prioritaet = 5;
    anzahl = myatol(zahl);
    if (anzahl < 1) anzahl = 1;
  }

  char *bauwerk = scan;
  short fake = befehl[0]=='*';


  // Es koennen nur MAXIMALZAHL_LAUFENDER_PROJEKTE Projekte gleichzeitig laufen.

  if (!fake && vorhabenliste.number_of_elements() 
  		>= MAXIMALZAHL_LAUFENDER_PROJEKTE) { // Zu viele
    if (fehlermeldung) {
      report(L("*** Wir k~onnen nicht mehr als %s Projekte auf einmal","*** We cannot manage more than %s projects at once."), 
          nice_ltoa(MAXIMALZAHL_LAUFENDER_PROJEKTE));
      report(L(" verwalten. Das Projekt %s kann daher nicht aufgenommen werden! ***\n"," Therefore,  project  %s cannot be added to the list! ***\n"), bauwerk);
    }
    return 1;
  }

  // Bevor ich anfange, schlage ich erstmal in der Enzyklopaedie alles
  // ueber das gewuenschte Bauwerk nach...

  // Die Spezifikation des Bauwerkes erfolgt durch zwei Buchstaben.
  // Von der Enzyklopaedie kann ich mir dann das zugehoerige Vorhaben
  // suchen lassen.

  VORHABEN_ENZ *vorhaben = g_enzyklopaedie->vorhaben(bauwerk, staat());

  // Gibt es das ueberhaupt?
  if (!vorhaben) {
    report(L("*** BA-Befehl: Keines deiner m~oglichen Projekte hat die Abk~urzung \"%s\". ***\n","*** BU-Command: The abbreviation  \"%s\" does not exist. Think again! ***\n"),
	   bauwerk);
    return 1;
  }

  // Ab jetzt ist das Bauwerk nicht mehr unter seiner Abkuerzung, sondern
  // unter seinem Namen bekannt...
  
  bauwerk = vorhaben->name;

  // In der Enzyklopaedie stehen unter den Vorhaben auch die Weltbauten.
  // Eine Stadt kann aber keine Weltbaut bauen. Dies fange ich hier ab.
  if (!strcmp("WELTBAUT", vorhaben->start_attribute.abfragen("TYP"))) {
    report(L("*** Der Bau %s ist nur au~serhalb der Stadt m~oglich und ","*** The structure  %s can only be built outdoors and "),
	    konjugation(bauwerk, GENITIV_PHRASE));
    report(L("mu~s durch eine geeignete Einheit durchgef~uhrt werden. ***\n","requires the appropriate construction unit..***\n"));
    return 1;
  }

  // Wenn es sich um eine Erfindung handelt kann es sein, dass sie der
  // Staat schon hat. In diesem Fall laste ich garnicht erst ein.
  
  if (staat()->info("HAT_ENTWICKLUNG",bauwerk)) {
    report(L("Die Entwicklung %s ist in unserem Staat bereits vollendet.\n","The development  %s is already very well-known in your Empire!\n"),
	    konjugation(bauwerk, GENITIV_PHRASE | SINGULAR));
    return 1;
  }

  // Hat die Stadt alle noetigen Voraussetzungen? D.h. ich muss die Stadt-
  // ausbauten mit den benoetigten Voraussetzungen vergleichen. Dazu baue
  // ich eine Attributliste mit den Stadtausbauten auf.

  ATTRIBUT_LISTE bauten;
  FOR_EACH_OBJEKT_IN (alle_stadtausbauten()) DO_AND_DELETE
  (
    bauten.setzen(objekt->a_gattung());
  )

  if (!fake && !vorhaben->voraussetzungen_in_der_stadt.passen_in(&bauten)) // Nein!
  {
    if (fehlermeldung) {
      if (!mystrcmp(vorhaben->start_attribute.abfragen("TYP"), "ERFINDUNG")) {
        report(L("F~ur die Entwicklung %s","For the development of %s"), konjugation(vorhaben->name,
	  GENITIV_PHRASE | SINGULAR));
      }
      if (!mystrcmp(vorhaben->start_attribute.abfragen("TYP"), "STADTAUSBAU")) {
        report(L("Zum Bau %s","For the construction of %s"), konjugation(vorhaben->name,
	 GENITIV_PHRASE | SINGULAR));
      }
      if (!mystrcmp(vorhaben->start_attribute.abfragen("TYP"), "EINHEIT")) {
        report(L("F~ur die Aufr~ustung durch %s","For  the armament of %s"),konjugation(vorhaben->name,
         AKKUSATIV | SINGULAR));
      }
      report(L(" ben~otigt die Stadt bestimmte Ausbauten (%s).\n"," our town requires certain buildings.  (%s).\n"), vorhaben->
       voraussetzungen_in_der_stadt.to_string());
    }
    return 1;
  }

  // Hat der Staat alle noetigen Voraussetzungen? Dazu rufe ich beim Staat
  // das Info HAT_ENTWICKLUNG, dem ich einen Attributstring uebergebe, der in die Ent-
  // wicklungen passen muss. Es liefert mir "Ja.", wenn alles klar ist,
  // sonst NULL.

  if (!fake) {
    if (!besitzer->info("HAT_ENTWICKLUNG", 
		     vorhaben->voraussetzungen_im_staat.to_string()))
    {
      report(L("F~ur %s besitzt unser","For %s our Empire"), konjugation(bauwerk, AKKUSATIV|SINGULAR));
      report(L(" Staat die n~otigen wissenschaftlichen und technischen"," does neither have the necessary scientific knowledge nor the"));
      report(L(" Voraussetzungen noch nicht.\n"," necessary technical skill.\n"));
      return 1;
    }
  }

  // Die Anzahl der gleichen Bauten in einer Stadt ist begrenzt. Die
  // Kontrolle wird hier vorgenommen, muss aber bei der Fertigstellung
  // wiederholt werden, damit die Sperre nicht durch gleichzeitiges
  // Bauen betrogen werden kann.

  long vorhanden;
  if (!fake && 0 > (vorhanden = anzahl_an_stadtausbauten_pruefen(bauwerk))) {
    if (fehlermeldung) {
      if (vorhanden == -1) {
        report(L("*** Es kann nicht mehr als %s in einer Stadt gebaut werden. *** \n","*** No more than one %s can be built in  a town. *** \n"),
          konjugation(bauwerk,  NOMINATIV | SINGULAR));
      }
      else {
        report(L("*** Es k~onnen nicht mehr als %s","*** No more than %s"), nice_ltoa(-vorhanden));
        report(L(" %s in einer Stadt gebaut werden. ***\n"," %s can be built in a town. ***\n"),
        konjugation(bauwerk, NOMINATIV | PLURAL));
      }
    }
    return 1;
  }

  VORHABEN *neu = new VORHABEN(bauwerk, prioritaet, anzahl);
  
  if (!neu->vorhaben) {
      log('I', "STADT::befehl_bauen(%s): Bauwerk nicht bekannt", bauwerk);
      return 1;
  }

  // Bei Vorhaben, die es maximal einmal pro Stadt gibt, mache ich
  // eine Optimierung fuer die Spieler: Wenn es schon ein solches
  // Vorhaben als Projekt gibt, wird dessen Prioritaet geaendert,
  // anstatt ein neue eingelastet.

  if (vorhaben->max_anzahl_in_einer_stadt == 1)
  {
      // Alle Vorhaben durchsuchen.
      VORHABEN *vor = (VORHABEN *)vorhabenliste.last();
      while (!vor->is_head()) {
	  if (!mystrcmp(vor->vorhaben, bauwerk)) { // Gefunden.
	      // Dieses Vorhaben loeschen. Die Ressourcen des
	      // neuen Vorhabens von diesem hier nehmen.
	      report(L("Da man nur %s pro Stadt bauen kann, brechen wir","Because only  %s can be built per city Stadt, we cancel")
		     L(" das alte Projekt ab und verwendet die schon bezahlten "," the previous project and use the commodities for the new one. ")
		     L("G~uter f~ur das neue. ","Isn´t that smart?"), konjugation(bauwerk, AKKUSATIV|SINGULAR));
	      neu->noch_benoetigte_resourcen.setzen_auf(vor->noch_benoetigte_resourcen);
	      delete vor;
	      break; // Duerfte nur einmal drin sein!
	  }
	  vor = (VORHABEN *)vor->previous();
      }
  }      


  // Wenn gefaked wird, dann kostet der Bau garnichts.
  if (fake) {
    neu->noch_benoetigte_resourcen.setzen_auf(0);
    // Bei Sachen, die Einwohner brauchen, muss ich diese vorher dazugegen,
    // weil sie nachher wieder abgezogen werden.
    long einwohner = myatol(vorhaben->start_attribute.abfragen("EINWOHNER"));
    einwohnerzahl += einwohner;
    einwohneraenderung += einwohner;
  }
     
  if (!strcmp(vorhaben->start_attribute.abfragen("TYP"), "ERFINDUNG")) {
    report(L("Unsere Wissenschaftler widmen sich der Entwicklung %s.\n","Our ingenious scientists dedicate their efforts to the development of  %s.\n"),
    konjugation(bauwerk, GENITIV_PHRASE));
  }

  else if (!mystrcmp(vorhaben->start_attribute.abfragen("TYP"), "STADTAUSBAU")) {
    report(L("Wir beginnen mit dem Bau ","We start building "));
    if (anzahl == 1) report("%s.\n",konjugation(bauwerk, GENITIV_PHRASE));
    else {
      report(L("von %s "," %s "), nice_ltoa(anzahl));
      report("%s.\n",konjugation(bauwerk, DATIV | PLURAL));
    }
  }

  else if (!mystrcmp(vorhaben->start_attribute.abfragen("TYP"), "EINHEIT")) {
    report(L("Die Stadt entscheidet sich f~ur ","The Town decides on "));
    if (anzahl == 1) report("%s.\n",konjugation(bauwerk, AKKUSATIV|SINGULAR));
    else {
      report("%s ", nice_ltoa(anzahl));
      report("%s.\n",  konjugation(bauwerk, AKKUSATIV | PLURAL));
    }
  }

  // So. Jetzt muss die neue Struktur noch in die Vorhabenliste eingeordnet
  // werden. Diese ist nach Prioritaeten geordnet.

  VORHABEN *suche = (VORHABEN *)vorhabenliste.last();
  while(!suche->is_head())
  {
    if (suche->prioritaet >= prioritaet) break;
    suche = (VORHABEN *)suche->previous();
  }
  neu->insert_after(suche);

  return 1; // Das Beginnen eines Bauwerks kostet keine Zeit.
}


/**---------------------------------------------------------------------------
  * STADT::befehl_bau_abbrechen()
  * 
  * Realisiert den Befehl AB, mit dem ein Projekt der Stadt abgebrochen
  * werden kann. Die bis dahin inverstierten Resourcen gehen verloren.
  *
  * Bei der Befehlsvariante A- werden keine Fehlermeldungen ausgegeben.
  * @param
  * char *befehl: "AB%s" mit der Abkuerzung des Bauwerkes
  ---------------------------------------------------------------------------*/
short STADT::befehl_bau_abbrechen(char *befehl)
{
  short fehlermeldung = befehl[1] != '-';

  // Ist eines der beiden letzten Zeichen eine '0', so wird sie durch
  // ein 'O' ersetzt. Das soll Tippfehler korrigieren helfen.
  
  short l = mystrlen(befehl);
  if (befehl[l-1] == '0') befehl[l-1] = 'O';
  if (befehl[l-2] == '0') befehl[l-2] = 'O';

  char *abk = befehl+2;
  if (!*abk) {
    report(L("*** Der Befehl AB erfordert die Angabe einer Abk~urzung. ***\n","*** The Command AP requires an appropriate abbreviation. ***\n"));
    return 1;
  }
  
  // Die Spezifikation des Bauwerkes erfolgt durch zwei Buchstaben.
  // Von der Enzyklopaedie kann ich mir dann das zugehoerige Vorhaben
  // suchen lassen.

  VORHABEN_ENZ *vorhaben_enz = enzyklopaedie()->vorhaben(abk, staat());

  // Gibt es das ueberhaupt?
  if (!vorhaben_enz) {
    report(L("*** AB-Befehl: Ein Projekt mit der Abk~urzung \"%s\" gibt es nicht (mehr). ***\n","***  AP-Command: A project \"%s\" does not exist (anymore). ***\n"),
	   abk);
    return 1;
  }
  char *bauwerk = vorhaben_enz->name;

  VORHABEN *vorhaben = (VORHABEN *)vorhabenliste.last();
  while (!vorhaben->is_head()) {
    if (!strcmp(vorhaben->vorhaben, bauwerk)) { // Abbrechen.
      
      // Wenn mehrere Vorhaben der gleichen Art eingelastet sind,
      // dann verringere ich einfach nur die Anzahl um eins. Dadurch
      // dass ich von hinten anfange, wird das mit der kleinsten
      // Prioritaet abgebrochen.
      
      if (vorhaben->anzahl > 1)
      {
	 vorhaben->anzahl--;
	 report(L("Wir brechen eines der Vorhaben %s mit der Priorit~at","We abort one of the projects %s ( priority was "), bauwerk);
	 report(L(" %s ab.\n"," %s).\n"), myltoa(vorhaben->prioritaet));
      }
      else {
	 report(L("Wir brechen das Vorhaben %s mit der Priorit~at","We abort the project %s  (priority was"), bauwerk);
	 report(L(" %s ab.\n"," %s).\n"), myltoa(vorhaben->prioritaet));
	 delete vorhaben;
      }        
      return 1;
    }

    vorhaben = (VORHABEN *)vorhaben->previous();
  }

  if (fehlermeldung)
    report(L("*** Es l~auft kein Vorhaben unter dem Namen '%s'. ***\n","*** There is no project '%s' at this very moment.  ***\n"), bauwerk);
  return 1;
}


/**---------------------------------------------------------------------------
  * STADT::befehl_abreissen()
  * 
  * Befiehlt der Stadt, ein bestimmtes Bauwerk abzureissen.
  * @param
  * char *befehl: "AR%s",namedesbauwerkes
  ---------------------------------------------------------------------------*/
short STADT::befehl_abreissen(char *befehl)
{
  // Ist eines der beiden letzten Zeichen eine '0', so wird sie durch
  // ein 'O' ersetzt. Das soll Tippfehler korrigieren helfen.
  
  short l = mystrlen(befehl);
  if (befehl[l-1] == '0') befehl[l-1] = 'O';
  if (befehl[l-2] == '0') befehl[l-2] = 'O';

  char *abk = befehl+2;
  if (!*abk) {
    report(L("*** Der Befehl AR erfordert die Angabe einer Abk~urzung. ***\n","*** The command RB requires an appropriate abbreviation.  ***\n"));
    return 1;
  }
  
  // Die Spezifikation des Bauwerkes erfolgt durch zwei Buchstaben.
  // Von der Enzyklopaedie kann ich mir dann das zugehoerige Vorhaben
  // suchen lassen.

  VORHABEN_ENZ *vorhaben = enzyklopaedie()->vorhaben(abk);

  // Gibt es das ueberhaupt?
  if (!vorhaben) {
    report(L("*** AR-Befehl: Es gibt keine Stadtausbaut mit der Abk~urzung \"%s\". ***\n","*** RB-Command: There are no buildings with an abbreviation \"%s\". ***\n"),
	   abk);
    return 1;
  }
  char *bauwerk = vorhaben->name;

  short schon_abgerissen = 0;
  char attr[80];
  sprintf(attr,"GATTUNG=%s,TYP=STADTAUSBAU",bauwerk);
  FOR_EACH_OBJEKT_IN (alle_objekte_im_direkten_besitz(attr))
  DO_AND_DELETE
  (
    if (!schon_abgerissen) {
      report(L("Wir rei~sen %s ab.\n","We raze %s (Geez, this is real fun)! \n"), konjugation(bauwerk, AKKUSATIV | SINGULAR));
      objekt->zur_vernichtung_vormerken();
      schon_abgerissen = 1;
    }
  )
  
  if (!schon_abgerissen)
  {
    report(L("*** AR-Befehl: Wir haben k%s zum Abrei~sen. ***\n","*** RB-Befehl: We don´t have a k%s! ***\n"),
	konjugation(bauwerk, AKKUSATIV | SINGULAR));
  }
  return 1;


}


/**---------------------------------------------------------------------------
  * STADT::befehl_aufloesen()
  * 
  * Die Stadt ueberstellt
  * alle Einheiten an eine andere Stadt, und loest sich anschlie-
  * ssend auf. Die Stadtausbauten werden vernichtet, alle Projekte 
  * gestoppt, usw. Die Hauptstadt kann sich nicht aufloesen.
  * Ausserdem kann man Staedte nur aufloesen, wenn sie hoechtens 15.000
  * Einwohner haben.
  ---------------------------------------------------------------------------*/
short STADT::befehl_aufloesen(char *befehl)
{
  // Die Hauptstadt kann man nicht aufloesen. Deshalb brauche ich spaeter
  // auch keine Abfrage, ob die letzte Stadt aufgeloest wurde, weil das
  // nie sein kann.
  
  if (bin_hauptstadt()) {
    report(L("*** Die Hauptstadt kann nicht aufgel~ost werden! ***\n","*** Are you lunatic ???? We cannot raze our Capital City!!!  ***\n"));
    return 1;
  }

  if (einwohnerzahl > 15000) {
    report(L("*** Der Befehl AL: Die Stadt kann nicht aufgel~ost werden, ","*** RC Command: You cannot raze this town, "));
    report(L("da sie zu viele Einwohner hat. Man kann St~adte nur dann aufl~osen,","because there are still too many inhabitants. Towns can only be razed"));
    report(L(" wenn sie h~ochstens 15.000 Einwohner haben. ***\n"," if there are less than 15.000 dwellers in it. ***\n"));
    return 1;
  }

  if (befehl_dauert_noch(10)) return 0; // Noch nicht fertig.

  char *abk = befehl+2;  // Abkuerzung der Stadt, die die Einheiten uebernimmt.

  report(L("\n... und dann begannen die schwierigsten Zeiten in %s.","\n... and then, the veils of death shrouded %s."), a_name());
  report(L("\nIn Trauer und Verzweiflung packten die B~urger eilig ihre wichtigsten Sachen","\nShattered by sadness and despair the citizens hurriedly pack their worldly belongings and leave town, "));
  report(L("\nzusammen; was nicht tranportiert werden konnte, ~uberlie~s man den","\nwhile all their familiy members and  possessions left behind are quickly consumed by the dancing flames. "));
  report(L("\nFlammen. Langsam sammelten sich die Menschen auf den Hauptstra~sen","\nA huge mourning crowd gathers before the crumbling city gates while within the city  walls"));
  report(L("\nund bildeten vor den Toren der Stadt einen einzigen riesigen Menschenstrom.","\n death cries reverberate as many pitiful souls die in a fiery grave. "));
  report(L("\nSo verlie~sen sie, der B~urgermeister und die Mitglieder des Rathauses","\nLead by the City Mayor, the sorry crowd slowly moves away"));
  report(L("\nallen voran, mit Blick in eine ungewisse und sorgenvolle Zukunft ihre","\nfrom the smoking pile of rubble and debris which was once their beloved home town"));
  report(L("\ngeliebte Heimtstadt... Das ist das Ende von %s.\n","\ninto a bleak future full of pain and grief.. That was the sad demise of%s.\n"), a_name());

  // Und nun ueberstelle ich alle Einheiten einer anderen Stadt.
  STADT *zielstadt = NULL;
  if (abk[0]) zielstadt = (STADT *)objekt_mit_namen(abk);
  if (!zielstadt || zielstadt->besitzer != staat())
  {
    if (zielstadt) {
      report(L("*** Es ist nicht m~oglich, die Einheiten und Einrichtungen ","*** It is not possible to transfer units and structures")
	     L("einer Stadt eines anderen Reiches zu ~uberstellen. ","to a town owned by another Empire."));
    }
    else if (abk[0])
      report(L("*** Es gibt keine Stadt mit der Abk~urzung '%s'. ","***There is no town with the abbreviation '%s'. "),abk);

    zielstadt = staat()->groesste_stadt_ausser(this); // Muss es geben. Staat hat mind. 2 Staedte
    report(L(" Wir nehmen stattdessen %s. "," We take %s instead. "), zielstadt->a_name());
    
  }
   
  einheiten_ueberstellen(zielstadt);
  staat()->einrichtungen_uebergeben(this, zielstadt);
  einwohnerzahl = 0;
  zur_vernichtung_vormerken();
  return 1;
}  


/**---------------------------------------------------------------------------
  * STADT::einheiten_informieren()
  * 
  * Schickt allen Einheiten einen Tagebuchtext.
  ---------------------------------------------------------------------------*/
void STADT::einheiten_informieren(char *text)
{
  FOR_EACH_OBJEKT_IN (alle_objekte_im_direkten_besitz("TYP=EINHEIT"))
  DO_AND_DELETE  ( objekt->kommando("TAGEBUCH",text) )
}
  

/**---------------------------------------------------------------------------
  * STADT::einheiten_ueberstellen()
  * 
  * Alle Einheiten der Stadt werden ueberstellt.
  ---------------------------------------------------------------------------*/
void STADT::einheiten_ueberstellen(OBJEKT *zielstadt)
{
  report(L("Alle Einheiten werden der %s ","All units are assigned to %s "),zielstadt->a_gattung());
  report(L("%s ~uberstellt.\n","%s.\n"), zielstadt->a_name());
  
  FOR_EACH_OBJEKT_IN (alle_einheiten(true))
  DO_AND_DELETE  ( geben(zielstadt,objekt); )
  
}


/**---------------------------------------------------------------------------
  * STADT::befehl_feld_nicht_mehr_bewirtschaften()
  * 
  * Realisiert den Befehl 'NB', mit dem ein Feld spezifiziert werden
  * kann, das ab so fort nicht mehr bewirtschaftet werden soll, folg-
  * lich also auch keine Resourcen mehr einbringt und von einer
  * anderen Stadt bewirtschaftet werden kann.
  *
  * @return
  * Ist immer 1, da der Befehl keine Zeit benoetigt
  ---------------------------------------------------------------------------*/
short STADT::befehl_feld_nicht_mehr_bewirtschaften(char *richtungsstring)
{
   if (!richtungsstring[0]) {
     report(L("*** Der Befehl NB erfordert die Angabe des Feldes, das nicht ","*** The command UP requires you to specify the "));
     report(L("mehr bewirtschaftet werden soll, z.B. NBNW oder NBSSO. ***\n","squares. For example,  UPNW or UPSSE. ***\n"));
     return 1;
   }

   // Zuerst die Adresse berechnen...
   RIC ric(richtungsstring);
   
   ADR adr=landschaft()->adresse_in_richtung(adresse,ric);
   if (adr == adresse) {
     report(L("*** NB: Das Mittelfeld wird immer bewirtschaftet. ***\n","*** The Capital town´s square is always in operation. ***\n"));
     return 1;
   }
     
   if (adr.ist_ausserhalb()) {
     report(L("*** Ungueltige Richtungsangabe \"%s\" beim NB-Befehl! ***\n","*** Invalid directions  \"%s\" for the UP-Command! ***\n"),richtungsstring);
     return 1;
   }

   if (!feld_nicht_mehr_bewirtschaften(adr))
     report(L("*** Das Feld %s wurde garnicht bewirtschaftet! ***\n","*** Square %s has not been in operation! ***\n"), richtungsstring);

   return 1;
}


/**---------------------------------------------------------------------------
  * STADT::guenstiges_feld_bewirtschaften()
  * 
  * Sucht nach einem bestimmten Kriterium ein Feld in der Umgebung
  * der Stadt aus, und meldet dieses bei Stadt und Welt als bewirt-
  * schaftet, wenn moeglich. In der Liste der bewirtschafteten Felder
  * wird es mit Absicht am Ende eingetragen. Die explizit bewirt-
  * schafteten Felder werden naemlich immer am Anfang eingetragen. Wenn
  * nun eines automatische geloescht werden muss, dann immer vom
  * Ende. So sind die expliziten Felder am besten geschuetzt.
  *
  * @return
  * 1, falls noch eine Feld bekommen wurde, 0 sonst.
  ---------------------------------------------------------------------------*/
short STADT::guenstiges_feld_bewirtschaften()
{
  if (!welt()) return 0;

  // Zuerst brauche ich mal eine Liste aller Felder im Umkreis.
  DOUBLIST *feldliste = welt()->alle_adressen_im_umkreis_von
     (adresse, (2.5 + 1 * einfluss_vorhanden("STARAD")));

  // Jetzt gehe ich alle der Reihe nach durch. Bei jedem Feld muss ich die
  // Welt fragen, ob es noch frei ist, und welche Resourcen es ausspuckt.
  // Am Ende nehme ich das Feld mit der hoechsten Summe an Ressourcen.

  long rekord = -1; // Bis dato noch kein Feld das beste
  ADR rekordfeld;

  while (!feldliste->is_empty()) {
    ADR adr = ((ADR_LIST_NODE *)feldliste->first())->adresse;
    if (feld_ist_noch_frei(adr)) { // Koennte klappen
      long summe = resourcen_auf_feld(adr)[L('N','F')]*2000
		   + resourcen_auf_feld(adr).betrag()*40;

      // Naehere Felder sollen bevorzugt werden.
      summe -= long(welt()->entfernung_zwischen(adr, adresse)*4 + 0.5);

      if (summe > rekord) {
	rekord = summe;
	rekordfeld = adr;
      }
    }
    delete feldliste->first();
  }

  delete feldliste;

  // So. Wenn rekord noch -1 ist, dann gibt's kein Feld mehr.
  if (rekord == -1) return 0;

  // Ansonsten bewirtschafte ich nun das neue Feld.
  ADR_LIST_NODE *adrnode = new ADR_LIST_NODE;
  adrnode->adresse = rekordfeld;
  bewirtschaftete_felder.add_tail(adrnode);

  // Und der Welt teile ichs auch noch mit.
  welt()->kommando("FELD_BEWIRTSCHAFTEN", &rekordfeld);

  return 1; // Alles bestens. Feld bekommen.
}


/**---------------------------------------------------------------------------
  * STADT::feld_nicht_mehr_bewirtschaften()
  * 
  * Veranlasst die Stadt, ein bestimmtes oder das letzte Feld nicht
  * mehr zu bewirtschaften. Ein Report wird gemacht.
  * Das Mittelfeld, auf dem die Stadt steht, nimmt eine besondere
  * Rolle ein. Es kann nur explizit geloescht werden. Bei fehlender
  * Angabe einer Adresse wird nur ein Feld von der Bewirtschaftung
  * entfernt, wenn ausser dem Mittelfeld noch eines bewirtschaftet
  * wird.
  *
  * @param
  * ADR& a: Adresse eine Feldes oder garnichts, wenn egal
  *
  * @return
  * 1, wenn es geklappt hat,
  * 0, wenn dieses Feld nicht bzw. gar kein Feld bewirtschaftet wurde.
  ---------------------------------------------------------------------------*/
short STADT::feld_nicht_mehr_bewirtschaften(ADR& a)
{
  ADR adr = a; // Weil ich es veraendern will.

  // Wenn a ausserhalb ist, dann heisst dass, die Funktion soll irgendein
  // Feld nehmen, nach moeglichkeit keines von den explizit bewirtschafteten.
  // Das Mittelfeld der Stadt darf auf keinen Fall ausgesucht werden.
  // Das klappt also nur, wenn mindestens zwei Felder in der Liste sind.
  // Das Mittelfeld steht immer am Anfang der Liste.

  if (adr == ADR::ausserhalb()) { // irgendeines nehmen
    if (bewirtschaftete_felder.number_of_elements() < 2) return 0;
    adr = ((ADR_LIST_NODE *)bewirtschaftete_felder.last())->adresse;
  }

  // Jetzt muss ich schauen, ob in meiner Felderliste das entsprechende
  // Feld vorkommt.

  ADR_LIST_NODE *feld = (ADR_LIST_NODE *)bewirtschaftete_felder.first();
  while (!feld->is_tail())
  {
    if (feld->adresse.x == adr.x && feld->adresse.y == adr.y) break;
    else feld = (ADR_LIST_NODE *)feld->next();
  }

  if (feld->is_tail()) return 0; // War nicht dabei.

  // Ich mache auch einen Report. Jawohl. Dass diese Funktion hier auch
  // im Destruktor aufgerufen wird und dann auch dort reports verbucht
  // werden, duerfte nicht so sehr stoeren.

  RIC ric = landschaft()->richtung_von_nach(adresse, feld->adresse);
  report(L("Die Bewirtschaftung von Feld %s wird eingestellt. ","Operation of square  %s is now OFF. "),ric.to_string());

  delete feld;

  landschaft()->kommando("FELD_NICHT_BEWIRTSCHAFTEN", &adr);
  return 1;
}


/**---------------------------------------------------------------------------
  * STADT::befehl_feld_bewirtschaften()
  * 
  * Realisiert den Befehl 'FB', mit dem das Bewirtschaften eines Feldes
  * im Umkreis der Stadt angeordnet werden kann, falls bestimmte Be-
  * dingungen erfuellt sind.
  *
  * @return
  * Ist immer 0, wenn erst eine Runde vergangen ist, sonst
  * 1, und der Befehl wurde ausgefuerht.
  ---------------------------------------------------------------------------*/
short STADT::befehl_feld_bewirtschaften(char *richtungsstring)
{
   // Um festzustellen, welches Feld bearbeitet werden soll, muss ich
   // aus der Richtung das Zielfeld bestimmen.

   if (!landschaft()) {
       log('I', "STADT::befehl_feld_bewirtschaften(): landschaft() liefert 0");
       return 1;
   }

   RIC ric(richtungsstring);
   ADR feld(landschaft()->adresse_in_richtung(adresse, ric));
   if (feld.ist_ausserhalb()) {
     report(L("*** Ungueltige Richtungsangabe \"%s\" beim FB-Befehl! ***\n","*** Invalid directions  \"%s\" for SP-Command! ***\n"),richtungsstring);
     return 1;
   }

   if (befehl_dauert_noch(1)) return 0; // Noch nicht fertig.

   // Wenn keine Adresse angegeben wurde, dann bewirtschafte ich das guen-
   // stigste Feld.

   if (feld == adresse) {
     if (!guenstiges_feld_bewirtschaften()) {
       report(L("*** FB: Es ist kein weiteres Feld in der N~ahe der Stadt","*** SP: There is no square left in the town´s vicinity"));
       report(L("frei, das noch bewirtschaftet werden k~onnte. ***\n"," which could be operated. ***\n"));
       return 1;
     }

     // Das neue Feld wurde nach Konvention am Ende der Liste eingetragen.
     // Ich kenne daher seine Adresse.
     ADR feld = ((ADR_LIST_NODE *)(bewirtschaftete_felder.last()))->adresse;
     RIC ric = landschaft()->richtung_von_nach(adresse, feld);
     report(L("Ab jetzt wird Feld %s bewirtschaftet.\n","From now on  square %s will be operated.\n"),ric.to_string());
     return 1;
   }

   // Nun muss ich kontrollieren, ob das Feld nicht zuweit entfernt liegt...

   if (landschaft()->entfernung_zwischen(feld, adresse) > (2.5 + 1 * einfluss_vorhanden("STARAD")))
   {
       report(L("*** Das Feld %s ist zu weit von der Stadt entfernt! ***\n","*** Square %s is too far away from your town!  ***\n"),richtungsstring);
     return 1;
   }

   // Ferner muss ich schauen, ob das Feld nicht schon bearbeitet wird.
   // Dazu muss ich die Liste der bewirtschafteten Felder durchsuchen.
   FOR_EACH_ADR_IN (&bewirtschaftete_felder) DO
   (
     if (adr.x == feld.x && adr.y == feld.y)
     {
	 // Ich aendere die Reihenfolge der Liste und setze das Feld
	 // an zweite Stelle. Das Macro FOR_EACH_ADR_IN definiert
	 // die Variable node.
	 node->remove();
	 node->insert_after(bewirtschaftete_felder.first());
	 report(L("Feld %s wird ab sofort bevorzugt bewirtschaftet.\n","Frow now on, square %s will be a preferred square.\n"),richtungsstring);
	 return 1;
     }
   )

   // Nun muss ich die Landschaft fragen, ob das Feld ueberhaupt (noch)
   // bewirtschaftet werden kann.

   if (!feld_ist_noch_frei(feld)) { // Feld wird schon bewirtschaftet
     report(L("*** Das Feld %s wird schon von einer anderen Stadt genutzt! ***\n","*** Square %s is already operated by another town! ***\n"),
	     richtungsstring);
     return 1;
   }

   // So. Jetzt ist alles klar, und ich bekomme das Feld.
   ADR_LIST_NODE *neu = new ADR_LIST_NODE;
   neu->adresse = feld;

   // Das neue Feld muss nun an die zweite Position in der Liste kommen,
   // da das Mittelfeld an der ersten Stelle stehen muss und die nicht
   // explizit angegebenen am Ende der Liste.

   neu->insert_after(bewirtschaftete_felder.first());

   // Hier teile ich der Landschaft noch mit, dass das Feld ab nun bewirt-
   // schaftet wird.
   landschaft()->kommando("FELD_BEWIRTSCHAFTEN", &feld);

   report(L("Feld %s wird ab jetzt bewirtschaftet.\n","From now on, square %s will be operated.\n"),ric.to_string());

   // Alles fertig...
   return 1;
}

/**---------------------------------------------------------------------------
  * STADT::wir_bewirtschaften_feld()
  * 
  * Stellt fest, ob eine bestimmtes Feld von der Stadt bewirtschaftet
  * wird, indem die Liste aller bewirtschafteten Felder durchsucht wird.
  ---------------------------------------------------------------------------*/
short STADT::wir_bewirtschaften_feld(ADR& feldadr)
{
  ADR_LIST_NODE *feld = (ADR_LIST_NODE *)bewirtschaftete_felder.first();
  while (!feld->is_tail())
  {
    if (feld->adresse.x == feldadr.x && feld->adresse.y == feldadr.y) return 1;
    else feld = (ADR_LIST_NODE *)feld->next();
  }
  return 0;
}


/**---------------------------------------------------------------------------
  * STADT::resourcen_auf_feld()
  * 
  * Berechnet, wieviele Resourcen die Stadt aus dem genannten Feld be-
  * ziehen kann / koennte. Hier werden auch noch Einfluesse 
  * DUENGER, SOLAR und FISCH einberechnet.
  *
  * @param
  * ADR& adr: Besagtes Feld.
  *
  * @return
  * RESOURCE_VEKTOR& : Referenz (statisch) auf Ergebnis.
  ---------------------------------------------------------------------------*/
RESOURCE_VEKTOR& STADT::resourcen_auf_feld(ADR& adr)
{
  static RESOURCE_VEKTOR fehler_antwort;
  if (!welt()) return fehler_antwort;

  short duenger = einfluss_aufsummieren("KD");
  long solarenergie = 0, nahrungsbonus = 0;

  if (einfluss_vorhanden("SO"))
    solarenergie += myatol(welt()->gelaendeform_attribut(adr, "SOLAR"));

  nahrungsbonus = duenger * myatol(welt()->gelaendeform_attribut(adr, "DUENGER"));

  if (einfluss_vorhanden("FISCH"))
    nahrungsbonus += myatol(welt()->gelaendeform_attribut(adr, "FISCH"));

  static RESOURCE_VEKTOR res;
  
  res.setzen_auf(welt()->resourcen_auf_feld(adr));
  RESOURCE_VEKTOR nahrung_res(nahrungsbonus, L('N','F'));
  RESOURCE_VEKTOR energie_res(solarenergie, L('E','E'));
  res.addiere(nahrung_res);
  res.addiere(energie_res);
  return res;
}


/**---------------------------------------------------------------------------
  ---------------------------------------------------------------------------*/
short STADT::feld_ist_noch_frei(ADR& adr)
{
  return NULL != landschaft()->info("BEWIRTSCHAFTUNG_PRUEFEN",&adr);
}


/**---------------------------------------------------------------------------
  * STADT::zug_abschliessen()             // virtuell
  ---------------------------------------------------------------------------*/
void STADT::zug_abschliessen(long)
{
    if (zur_vernichtung_vorgemerkt()) return;

    if (!attribut_gesetzt("FrischGegruendet")) {
	restliche_resourcen_speichern(); // Vom Rest soviel wie moeglich speichern
	gattung_berechnen(); // Eventuell ist die Stadt gewachsen
	vorhaben_aufraeumen(); // Loescht evtl. schon fertige Entwicklugen.
    }

    else attribut_loeschen("FrischGegruendet");

    // Wie bei den Einheiten sieht auch eine Stadt alle Objekte in einem ge-
    // wissen Umkreis. Dem muss ich hier Rechnung tragen.

    landschaft()->sicht_eines_objektes(besitzer, adresse, 3.5);
    landschaft()->landschaftsabbild_aktualisieren(besitzer, adresse, 3.5);
}


/**---------------------------------------------------------------------------
  * STADT::naechster_zug()                // virtuell
  ---------------------------------------------------------------------------*/
void STADT::naechster_zug(long)
{
  // Wird bei jedem Zug vor Auswertung einzelnen Runden aufgerufen. Ich ini-
  // tialisiere hier die Resourcevektoren fuer den Ausdruck, da moeglicher-
  // weise die Funktionen, die sie sonst initialisieren, nicht aufgerufen
  // werden, da die Stadt keine Einwohner hat.
  
  res_alt.setzen_auf(resourcenspeicher); // Alte Resourcen merken
  res_be.setzen_auf(0);
  res_unt.setzen_auf(0);
  res_bew.setzen_auf(0);
  res_umw.setzen_auf(0);
  res_abk.setzen_auf(0);
  res_ern.setzen_auf(0);
  einwohneraenderung = 0; // Wird mit der Zeit veraendert.
  anzahl_kaempfe = 0;
  vorige_phase = 0;

  if (!zur_vernichtung_vorgemerkt())
  {
      if (ausnahmezustand()) {
	  long az = myatol(attribut("AUSNAHMEZUSTAND")) - 1;
	  if (az > 0) {
	      report(L("Der Ausnahmezustand wird noch f~ur volle %s Runden anhalten.\n","The state of emergency will last for another %s turns.\n"), myltoa(az));
	      attribut_setzen("AUSNAHMEZUSTAND", myltoa(az));
	  }
	  else {
	      report(L("Mittlerweile ist genug Gras ~uber die Sache gewachsen, so da~s der Ausnahmezustand ab jetzt aufgehoben ist.\n", "Order has been restored, the state of emergency is now over.\n"));
	      attribut_loeschen("AUSNAHMEZUSTAND");
	  }
      }
  }
}


/**---------------------------------------------------------------------------
  * STADT::naechste_phase()               // virtuell
  ---------------------------------------------------------------------------*/
void STADT::naechste_phase(long runde)
{
    log('6', "    -> %s", name);
    
    // Statusausgabe waerend des Zuges.
    io_printxy(39,1,name);
    io_printxy(42,1,"  ->           ");

    if (zur_vernichtung_vorgemerkt()) return;
    
    // Die Runde teilt sich in verschiedene Phasen auf. Falls die Stadt entwe-
    // der keine Einwohner hat (Ruinen), oder aber im aktuellen Zug gegruendet
    // wurde, werden keine Aktionen ausgeloest.

    // Durch das Einnehmen von Staedte kann es vorkommen, dass eine Stadt
    // eine Phase zweimal erlebt, oder eine verpasst. Damit trotzdem alle
    // Berechnungen gemacht werden, behalte ich in "vorige_phase" die Nummer
    // der zuletzt ausgefuehrten Phase. Am Anfang der Auswertung steht sie
    // immer auf 0.
    
    if (vorige_phase >= runde) return; // Keine doppelte Auswertung.
    else if (vorige_phase < runde-1)
    {
	// Ich hole die verlorene Phase jetzt nach. Dazu rufe ich die Funktion
	// rekursiv auf
	
	naechste_phase(runde-1);
	
	// Jetzt muss vorige_phase auf runde-1 stehen.
	
	naechste_phase(runde); // Sauber von vorne anfangen
	return;
    }
    vorige_phase = runde; // vorige_phase musste genau runde-1 sein.

    if (einwohnerzahl && !attribut_gesetzt("FrischGegruendet")) switch (runde)
    {
    case RUNDE_GUETERVERKEHR:
        gueterverkehr_abwickeln();
	break;
	
    case RUNDE_ARBEITSKRAFT:
	arbeitskraft_berechnen();
	break;

    case RUNDE_FELDER_BEWIRTSCHAFTEN:
	resourcen_einnahmen_verrechnen();
	break;

    case RUNDE_RESOURCEN_UMWANDELN:
	resourcen_umwandlungen();
	break;

    case RUNDE_EINWOHNER_ERNAEHREN:
	einwohner_mit_nahrung_versorgen();
	einwohner_gesundheitlich_versorgen();
	break;

    case RUNDE_WEITERBAUEN:
	weiterbauen();

	// Wenn der Spieler nicht mehr spielt, muss die Stadt sich selbst
	// verteidigen

	if (spieler_ausgestiegen()) milizen_bauen();
	break;

    }
    
    else if (!einwohnerzahl && runde==RUNDE_FELDER_BEWIRTSCHAFTEN) {
	kontrolliere_bewirtschaftung();
    }
}


/**---------------------------------------------------------------------------
  * STAAT::alle_stadtausbauten()
  * 
  ---------------------------------------------------------------------------*/
DOUBLIST *STADT::alle_stadtausbauten()
{
  DOUBLIST *antwortliste = new DOUBLIST;
  OBJEKT *objekt;
  SCAN(&besitztum, objekt)
  {
    if (objekt->typ_ist("STADTAUSBAU") 
        && !objekt->zur_vernichtung_vorgemerkt()) {
      antwortliste->add_tail(new OBJEKT_LIST_NODE(objekt));
    }
    NEXT(objekt);
  }
  return antwortliste;
}


/**---------------------------------------------------------------------------
  * STAAT::alle_einheiten()
  * 
  * Gibt eine Liste aller einheiten, die der Stadt unterstehen. Wenn
  * das Flag auch_tote gesetzt ist, werden auch die vernichteten Einheiten
  * mitgenommen.
  ---------------------------------------------------------------------------*/
DOUBLIST *STADT::alle_einheiten(bool auch_tote)
{
  DOUBLIST *antwortliste = new DOUBLIST;
  OBJEKT *objekt;
  SCAN(&besitztum, objekt)
  {
    if (objekt->typ_ist("EINHEIT") 
        && (auch_tote || !objekt->zur_vernichtung_vorgemerkt())) {
      antwortliste->add_tail(new OBJEKT_LIST_NODE(objekt));
    }
    NEXT(objekt);
  }
  return antwortliste;
}

/**---------------------------------------------------------------------------
  * STADT::irgendeine_stadtausbaut()
  * 
  * Waehlt zufaellig eine Ausbaut aus.
  *
  * @return
  * (OBJEKT *)  Ausgewaehlte Ausbaut oder NULL, wenn's keine gibt.
  ---------------------------------------------------------------------------*/
OBJEKT *STADT::irgendeine_stadtausbaut()
{
  OBJEKT *antwort = NULL;

  DOUBLIST *objliste = alle_stadtausbauten();
  long anzahl = objliste->number_of_elements();
  if (anzahl) {
    long auswahl = io_random(anzahl) + 1;
    FOR_EACH_OBJEKT_IN (objliste) DO (
      if (auswahl) {
	auswahl--;
	if (!auswahl) antwort = objekt;
      }
    )
  }

  delete objliste;
  return antwort;
}


/**---------------------------------------------------------------------------
  * STADT::stadtausbau_geht_kaputt()
  * 
  * Waehlt zufaellig eine Ausbaut aus und merkt sie zur Vernichtung vor.
  *
  *
  * @return
  * char *: Name der Gattung der kaputten Ausbaut oder NULL, wenn keine
  * mehr da war.
  ---------------------------------------------------------------------------*/
char *STADT::stadtausbau_geht_kaputt()
{
   OBJEKT *opfer = irgendeine_stadtausbaut();
   if (opfer) {
     opfer->zur_vernichtung_vormerken();
     return opfer->a_gattung();
   }
   else return NULL;
}


/**---------------------------------------------------------------------------
  * STADT::resourcen_einnahmen_verrechnen()
  * 
  * Berechnet aus die Liste der bewirtschafteten Felder der Stadt, wie-
  * viele Resourcen sie in einer Runde hinzubekommt und addiert die
  * Resourcen fuer eine Runde zum Speicher der Stadt.
  *
  * Belagerte Staedte bekommen nur einen Teile der Ressourcen gutge-
  * schreiben. Beim ersten mal 75%, dann 50%, dann 25% und dann 0%.
  * Jede Spielrunde der Belagerung ein Schritt weiter.
  ---------------------------------------------------------------------------*/
void STADT::resourcen_einnahmen_verrechnen()
{
  // Erstmal muss ich schauen, ob das mit der Arbeitskraft hinhaut und
  // kontrolliere deshalb die Bewirtschaftung. Das zieht auch gleich
  // die A-Kraft ab, und zwar vom Vektor res_bew, noch nicht vom Gesamt-
  // speicher.
  
  kontrolliere_bewirtschaftung();

  // Um die Einnahmen zu berechnen, gehe ich die Liste aller bewirtschafteten
  // Felder durch und frage die Landschaft, wieviele Resourcen dort zu
  // holen sind.

  ADR_LIST_NODE *feld = (ADR_LIST_NODE *)bewirtschaftete_felder.first();
  while (!feld->is_tail())
  {
    res_bew.addiere(resourcen_auf_feld(feld->adresse));

    // Nun kann es noch sein, dass auf dem bewirtschafteten Feld eine
    // Weltbaut steht. Dabei sind vor allem Bergwerke interressant. In
    // so einem Fall fordere ich von der Weltbaut soviele Resourcen an,
    // wie moeglich. Dazu verwende ich das Info 'RA' an die Weltbaut.
    // Erstmal hole ich mir die Liste 'aller' Weltbauten auf dem Feld.

    DOUBLIST *weltbauten = landschaft()->alle_objekte_bei_adresse
	(feld->adresse);
    filtere_objekt_liste(weltbauten,"TYP=WELTBAUT");

    if (!weltbauten->is_empty()) {

      // Da ohnehin niemals zwei Weltbauten auf einem Feld stehen duerfen,
      // nehme ich einfach die erste in der Liste. Viel schiefgehen kann
      // auch bei einem Programmfehler nicht.

      OBJEKT *weltbaut
	 = ((OBJEKT_LIST_NODE *)weltbauten->first())->objekt;
      RESOURCE_VEKTOR geholt(weltbaut->info("RESSOURCEN_AUSLIEFERN"));
      res_bew.addiere(geholt);

      // Ach, uebrigens: Wenn die Stadt die geholten Ressourcen nicht
      // speichern kann, dann gehen sie verloren! Abhilfe dazu bietet
      // ein Befehl, der die Weltbaut veranlasst, keine Ressourcen mehr
      // auszuliefern.

    }
    delete weltbauten; // Liste brauche ich nun nicht mehr.

    feld = (ADR_LIST_NODE *)feld->next();
  }

  short faktor = 0;
  if (!io_random(3) && bonuspunkte(2)) faktor = 2;
  else if (!io_random(3) && bonuspunkte(1)) faktor = 1  ;

  if (faktor) {
    report(L("Aufgrund von besonders g~unstiger Witterung f~allt die Ernte ","Due to the extremly favorable climate the crop yield "));
    report(L("dieses Jahr %s%% h~oher aus als gew~ohnlich. \n","this year was boosted by %s%%. \n"),
      faktor == 1 ? "10" : "20");
    long n = res_bew[L('N','F')];
    n /= (10/faktor);
    RESOURCE_VEKTOR add(n,L('N','F'));
    res_bew.addiere(add);
  }
  
  // Belagerung verrechnen.
  if (stadt_ist_belagert()) {
    int alter_der_belagerung = myatol(attribut("BELAGERUNG")) + 1;
    int faktor;
    attribut_setzen("BELAGERUNG", myltoa(alter_der_belagerung));
    switch (alter_der_belagerung) {
    case 1:
      report(L("Die Stadt befindet sich im Belagerungszustand. Ein Viertel","The town is in state of siege! This reduces all yield"));
      report(L(" der Einahmen gehen verloren!\n"," by one quarter!!\n"));
      faktor = 3;
      break;
    case 2:
      report(L("%s befindet wird schon seit letzter Runde belagert. ","%s is under siege since last turn. "), a_name());
      report(L("Die Situation verschlimmert sich, und es gehen die H~alfte ","Situation gets worse! Yields are now cut "));
      report(L("der G~utereinnahmen verloren.\n"," by 50%.\n"));
      faktor = 2;
      break;
    case 3:
      report(L("Seit zwei Runden steht %s nun schon unter Belagerung.","For two turns,  %s has been under siege!."), a_name());
      report(L("Die Situation wird ~au~serst kritisch, da nun nur noch ein Viertel ","Situation critical! Total yield is "));
      report(L("der G~utereinnahmen bei der Stadt ankommen.\n","reduced to an abyssmal 25%.\n"));
      faktor = 1;
      break;
    default:
      report(L("%s wird nun seit ","%s is under siege "), a_name());
      report(L("%s Runden belagert. ","for %s turns. "), nice_ltoa(alter_der_belagerung - 1));
      report(L("Die G~uterzufuhr ist vollst~andig unterbrochen.\n","No yields! WARNING!.\n"));
      faktor = 0;
    }
    res_bew.multipliziere_mit(faktor);
    res_bew.teile_durch(4);
  }
  else if (attribut_gesetzt("BELAGERUNG")) // War belagert, nun nicht mehr
  {
    report(L("Der Belagerungszustand ist aufgehoben. Die G~utereinnahmen sind","State of siege has ceased. Yield rate"));
    report(L(" wieder normal.\n"," restored to 100%.\n"));
    attribut_loeschen("BELAGERUNG");
  }
  
  resourcenspeicher.addiere(res_bew);
}


/**---------------------------------------------------------------------------
  * STADT::resourcen_umwandlungen()
  * 
  * Wird dem Einfluss UM gerecht, mit dem Ressourcen der einen Art
  * in solche anderer Art umgewandelt werden koennen. Der Einfluss UM
  * wird vor allem von Fabriken und Kraftwerken ausgeuebt, die dann z.B.
  * Kohle in Energie umwandeln (Kohlekraftwerk).
  *
  * Hier wird auch der statistische Wert 'industrieproduktion' erhoben,
  * der die Anzahl der produzierten Industriegueter wiedergibt.
  *
  ---------------------------------------------------------------------------*/
void STADT::resourcen_umwandlungen()
{
  // Die Einfluesse der Umwandlungen kann ich nicht einfach addieren
  // lassen. Ich lasse fuer jeden der Einfluesse das Kommando UM aus-
  // fuehren. Dort veranlasse ich alles noetige.

  res_umw.setzen_auf(resourcenspeicher);
  globale_einfluss_menge.kommando_fuer_jeden_einfluss(this, "UM1", "UMWANDLUNG");
  globale_einfluss_menge.kommando_fuer_jeden_einfluss(this, "UM2", "UMWANDLUNG");
  globale_einfluss_menge.kommando_fuer_jeden_einfluss(this, "UM3", "UMWANDLUNG");
  res_umw.subtrahiere(resourcenspeicher); // Gibt nun negative Aenderung an.
  res_umw.negieren();

}


/**---------------------------------------------------------------------------
  * STADT::arbeitskraft_berechnen()
  * 
  * Berechnet die Einnahmen aus Arbeitskraft aus der Bevoelkerungszahl
  * fuer eine Runde und fuegt sie dem Resourcenspeicher zu.
  ---------------------------------------------------------------------------*/
void STADT::arbeitskraft_berechnen()
{
  res_bew.setzen_auf(""); // Ich muss die benoetigten Arbeiter negativ eintr.

  // Ausgegangen wird davon, dass jede angebrochenen 1000 Bevoelkerung
  // eine Arbeitskraftresource von 1A erbringen.
  
  long kraft = ((einwohnerzahl)+999)/1000;
  if (kraft && !einwohnerzahl) kraft=1; // Wenigstens eines sollte man haben.
  if (kraft < 0) kraft = 0; // Sollte eigentlich nicht vorkommen!
  

  // Einen Bonus kann ich hier anbringen.
  short faktor = 0;
  if (!io_random(3) && bonuspunkte(1)) faktor = 1;
  else if (!io_random(3) && bonuspunkte(2)) faktor = 2;
  
  if (faktor) {
    report(L("Zur Zeit sind die Einwohner hochmotiviert und besonders ","Thanks to your glorious leadership skills your inhabitants are extremely motivated and industrious. "));
    report(L("flei~sig. Sie leisten durchschnittlich um %s%% mehr. ","This increases their efficiency by %s%%. "),
     faktor==1 ? "15" : "30");
    if (faktor == 1) kraft = (kraft * 115) / 100;
    else kraft = (kraft * 13) / 10;
  }

  arbeitskraft_einnahmen = kraft; // Fuer den Ausdruck merken.

  // Die Einnahmen fuege ich dem resourcenspeicher der Stadt hinzu.
  // Bei der Funktion resourcen_einnahmen_verrechnen() werden
  // dann die Felder berechnet. Erst dort wird die Kraft fuer
  // die Bewirtschaftung abgezogen. Ich bereite schon in dieser
  // Funktion hier den Vektor res_bew mit negativer Arbeitskraft vor.

  resourcenspeicher[L('A','M')] += kraft;

}

/**---------------------------------------------------------------------------
  * STADT::kontrolliere_bewirtschaftung()
  * 
  * Kontrolliert, ob fuer die Bewirtschaftung der Felder genuegend
  * Arbeitskraft vorhanden ist. Wenn zuwenig da ist, wird die bewirt-
  * schaftung von Feldern eingestellt. Wenn zuviel da ist, werden au-
  * tomatisch neue Felder bewirtschaftet. Kontrolliert auch eine
  * eventuelle Belagerung der Stadt. Wenn sie zutrifft, werden alle
  * entfernten Felder aus der Liste geworfen.
  ---------------------------------------------------------------------------*/
void STADT::kontrolliere_bewirtschaftung()
{
  char *attr_sparen = attribut("A-SPAREN");
  long sparen = myatol(attr_sparen);

  // Wenn des Attribut mit einem 'F' endet, dann ist eine maximale
  // Zahl von Feldern gemeint, die man bewirtschaften will.
  
  short max_felder = 0;
  if (attr_sparen && strlen(attr_sparen))
      max_felder = (attr_sparen[strlen(attr_sparen) - 1] == L('F','S'));
  
  long kraft = resourcenspeicher[L('A','M')];
  if (!max_felder) {
    if (sparen < kraft) kraft -= sparen;
    else kraft = 0; // Mehr als alles kann man nicht sparen!
  }
  
  long kraft_pro_feld = myatol(einfluss_der_art("AP"));
  long felderzahl = bewirtschaftete_felder.number_of_elements()-1;

  // Wenn die Felderzahl schon zu hoch ist, dann kann muessen manche
  // Felder gestrichen werden.

  while (felderzahl*kraft_pro_feld > kraft 
	 || (max_felder && (felderzahl+1) > sparen) ) {
    feld_nicht_mehr_bewirtschaften(); // Waehlt irgendeines aus.
    felderzahl--;
  }

  // So. Andererseits kann es aber sein, dass zuwenige Felder bewirtschaftet
  // werden. In diesem Fall muss ich automatisch solange Felder aussuchen,
  // bis keine mehr frei sind, oder der gewuenschte Wert erreicht. Sind mit
  // der verbleibenden Arbeitskraft naemlich noch mehr als 'nicht_bewirtschaften'
  // Felder bewirtschaftbar, so wird die Differenz ausgeglichen. Ein
  // gewisser Teil der Arbeitskraft der Stadt wird jedoch zurueckgehalten,
  // wenn der Spieler mit dem Befehl BE die Bewirtschaftung eingeschraenkt
  // hat und Arbeitskraft spart.
  
  while (kraft - felderzahl*kraft_pro_feld  >= kraft_pro_feld
	 && (!max_felder || (felderzahl+1) < sparen) 
	 && guenstiges_feld_bewirtschaften())
  {
    felderzahl++;
  }

  res_bew[L('A','M')] -= felderzahl * kraft_pro_feld;
}


/**---------------------------------------------------------------------------
  * STADT::stadt_ist_belagert()
  * 
  * Stellt fest, ob sich die Stadt im Belagerungszustand befindet. Dies
  * ist der Fall, wenn von den Feldern N,S,W,O mindestens drei von
  * einer gegnerischen Einheit besetzt sind, die entweder eingegraben ist
  * oder ein Schiff auf See.
  ---------------------------------------------------------------------------*/
short STADT::stadt_ist_belagert()
{
  short anzahl_besetzter_felder = 0;
  FOR_EACH_ADR_IN (welt()->alle_adressen_im_umkreis_von(adresse, 1.0))
  DO_AND_DELETE
  (
    if (!(adr == adresse)) { // Nicht Mittelfeld, also eins von N,S,W,O
      short wasserfeld = (!mystrcmp(welt()->gelaendeform_attribut(adr, "ART"),
			            "See"));
      short gegner = 0;
      FOR_EACH_OBJEKT_IN (welt()->alle_objekte_bei_adresse(adr, "TYP=EINHEIT,Militaerisch"))
      DO_AND_DELETE 
        (gegner |= (
		(objekt->besitzer->besitzer->kommando("DIPL_STATUS",this)==0))
	     && (   objekt->attribut_gesetzt("Eingegraben")
	         || (    objekt->attribut_gesetzt("See")
	              && wasserfeld ) ) )
	              
      if (gegner) anzahl_besetzter_felder ++;
    }
  )
  return (anzahl_besetzter_felder >= 3);
}


/**---------------------------------------------------------------------------
  * STADT::restliche_resourcen_speichern()
  * 
  * Nachdem alle Einnahmen und Ausgaben an Resourcen verrechnet sind,
  * bleibt der Stadt im allgemeinen noch etwas uebrig. Allerdings
  * kann sie bis zur naechsten Runde im allgemeinen nicht alles davon
  * speichern, der Rest geht veroren. Das Speichervolumen setzt sich
  * aus dem Attribut SPEICHERUNG und dem Einfluss SP zusammen.
  *
  * Hier wird ausserdem die Arbeitslosigkeit berechnet. Sie ergiebt
  * sich aus dem uebriggebliebenen Arbeitskraftsymbolen
  ---------------------------------------------------------------------------*/
void STADT::restliche_resourcen_speichern()
{
  // Wieviel ich speichern kann, dass haengt vom Einfluss "SP" ab, den ich
  // ueber Resourcestrings addieren muss. Abgesehen davon habe ich ein
  // gewisses Grundspeichervormoegen, dass in meinem Attribut "SPEICHERUNG"
  // steht. Ich berechne den Wert in meiner Variablen res_kap, die ich
  // dann beim Ausdruck auch mitausgebe.

  res_kap.setzen_auf(attribut("SPEICHERUNG"));

  // Jeder Zusatzspeicher, den ich besitze, uebt einen Einfluss SP aus,
  // der mein Speichervolumen erhoeht.

  RESOURCE_VEKTOR zusatzspeicher(globale_einfluss_menge.
		zusammenfassen(this, "SP", ein_zus_add_resource));

  // Jetzt muss ich noch addieren

  res_kap.addiere(zusatzspeicher);

  // Und jetzt schneide ich alles vom Resourcenspeicher ab, was ich nicht
  // speichern kann

  resourcenspeicher.begrenzen_auf(res_kap);
}

/**---------------------------------------------------------------------------
  * STADT::einwohner_mit_nahrung_versorgen()
  * 
  * Berechnet, wieviel Nahrung fuer die Einwohner der Stadt noetig ist
  * und wieviel vorhanden ist. Die verbrauchte Nahrung wird von Speicher
  * abgezogen und eventuell waechst die Bevoelkerung wegen guter Er-
  * naehrung oder schrumpft wegen Hunger. Entsprechende Reports werden
  * ausgegeben.
  ---------------------------------------------------------------------------*/
void STADT::einwohner_mit_nahrung_versorgen()
{
  // Hier ist der Wachstumsschluessel:
  // 1. Sind pro durchschnittlich 10000 Einwohner weniger als 2 Nahrungssymbole
  //    vorhanden, so nehmen die Einwohner um 1000 pro fehlendem Symbol
  //    ab, fall die Stadt groesser als 15000 Einwohner ist, ansonsten
  //    um 500 Einwohner pro fehlendem Symbol.
  // 2. Fuer jede 10000 Einwohner, fuer die 3 staat 2 Symbole zur Verfue-
  //    gung stehen, waechst die Stadt um 3000 Einwohner, konsumiert dann
  //    aber natuerlich auch mehr. Hat die Stadt weniger als 10000 Einwohner,
  //    so darf sie trotzdem ein Symbol zusaetzlich einsetzen/

  long alt = einwohnerzahl;
  long einheiten = (einwohnerzahl+2500)/5000;
  if (einheiten==0) einheiten=1; // Man braucht mindestens eine Nahrung!
  long verfuegbar = resourcenspeicher[L('N','F')];
  long verbraucht; // Wird dann eingestellt
  benoetigte_nahrung = einheiten; // Fuer den Ausdruck merken.

  if (einheiten > verfuegbar) { // Zu wenig
    report(L("Die Stadt hat %s Nahrungssymbole zuwenig f~ur die Einwohner! ","The town has a lack of  %s food units for its citizens! "),
      myltoa(einheiten-verfuegbar));

    long sterb=(einheiten-verfuegbar)*(3000+io_random(200)-io_random(200));

    if (sterb >= einwohnerzahl) { // Stadt verhungert
      // Fall1: Hauptstadt: Ein Einwohner bleibt ueber: Der Herrscher.
      if (bin_hauptstadt()) {
	  if (einwohnerzahl > 2) // Es leben noch mehr als ich und der Buergermeister
	  {
	      report(L("Die katastrophalen Folgen der gro~sen Hungersnot f~uhren ","Lots of people starve painfully. The great famine wipes out "));
	      report(L("zum Tod fast aller B~urger! %s ","almost all of your worthy citizens!  %s "),
		     staat()->attribut("TITEL"));
	      report(L("%s und der B~urgermeister ","%s and the City Mayor "), staat()->attribut("HERRSCHERNAME"));
	      report(L("sind die einzigen ~Uberlebenden.\n","are the sole survivors!ô.\n"));
	      einwohnerzahl = 2;
	  }
	  else {
	      report("%s", staat()->attribut("TITEL"));
	      report(L(" %s und der B~urgermeister nagen weiterhin am Hungertuch. "," %s and the City Mayor still live in extreme misery. "),
		     staat()->attribut("HERRSCHERNAME"));
	  }
      }
      else {
	report(L("Die letzten %s Einwohner ","The last few %s starving inhabitants "), myltoa(einwohnerzahl));
	report(L("verhungern j~ammerlich. In den Geschichstb~uchern wird das ","die a horrible, twisted, painful death... "));
	report(L("Kapitel \"%s\" beendet.\n"," \"%s\" is wiped out of existence and history! \n"), a_name());
	einwohnerzahl = 0;
	zur_vernichtung_vormerken();
	
	// Und nun muss ich noch alle Einheiten vernichten und ihnen
	// eine Mitteilung schicken...

	FOR_EACH_OBJEKT_IN (alle_einheiten())
	DO_AND_DELETE ( {
           objekt->tagebuch_fuehren(L("Die Einwohner unser Heimatstadt sind verhungert! ","Starvation kills all the inhabitants..... ")
				    L("Wir l~osen uns auf.","We are doomed."), ", ");
	   objekt->kommando("VERNICHTUNG");
	} );
	// Die Einrichtungen gehen an eine andere Stadt.
	staat()->einrichtungen_uebergeben(this, staat()->groesste_stadt_ausser(this));
	
      }
    }
    else {
      einwohnerzahl -= sterb;
      report(L("Es verhungern %s Menschen.\n"," %s people starve.\n"),myltoa(sterb));
    }
    
    verbraucht = verfuegbar; // Trotzdem alles verbraucht.
  }

  // Wachstum, wenn sehr viel Nahrung vorhanden ist, und zwar auch genug
  // um die gewachsene Bevoelkerung mit dem mindesten zu Versorgen.

  else {
    long bonus;
    if (!attribut_gesetzt("Rationiert")) {
      bonus = verfuegbar-einheiten; // Jeder bonus sind 5000 doppelt versorgte

      // Nicht mehr als 50% draufschlagen, mindestens aber 1.
      while (bonus*2 > einheiten && !(bonus==1 && einheiten==1) ) bonus--;
    }
    else bonus = 0;

    // Wenn der Staat noch Bonuspunkte hat, waechst die Stadt vielleicht
    // nicht nur um 3000, sonder um 3500 oder 4000 Einwohner je bonus.

    long plus = 0;    
    if (bonus && !io_random(3) && bonuspunkte(2)) plus = 1000;
    else if (bonus && !io_random(3) && bonuspunkte(1)) plus = 500;

    if (plus) {
      report(L("Diese Jahre sind besonders fruchtbar, und die Bev~olkerung","Those years are especially fertile - the population "));
      report(L(" vermehrt sich sogar um %s Einwohner pro zus~atzlichem "," grows by %s inhabitants per surplus "),
      plus==1000 ? "4000" : "3500");
      report(L("Nahrungspunkt. ","food unit. "));
    }

    long zuwachs = bonus*(3000+plus) + io_random(200);
    verbraucht = einheiten + bonus;
    einwohnerzahl += zuwachs;
    if (zuwachs > 1000) {
      report(L("Wegen guter Ern~ahrung nimmt die Bev~olkerung um %s Einwohner zu.\n","Due to the excellent food supply the population grows by %s inhabitants.\n"),
	  myltoa(zuwachs));
    }

  }

  RESOURCE_VEKTOR verb(verbraucht, L('N','F'));
  res_ern.setzen_auf(verb);                  // Verbrauchtes merken
  resourcenspeicher.subtrahiere(res_ern);    // Verbrauchtes abziehen
  einwohneraenderung += einwohnerzahl-alt;    // Einwohneraenderung merken
}


/**---------------------------------------------------------------------------
  * STADT::einwohner_gesundheitlich_versorgen()
  * 
  * Berechnet anhand der Einfluesse von medizinischen Einrichtungen
  * und medizinischen Entwicklungen die gesundheitliche Versorgung
  * der Bevoelkerung. Ausgehend davon kann es dann zu verschiedenen
  * Ereignissen kommen.
  ---------------------------------------------------------------------------*/
void STADT::einwohner_gesundheitlich_versorgen()
{
  // Wenn die Stadt keine Einwohner mehr hat, dann ueberspringe
  // ich die Funktion. Kann z.B. sein, wenn bei der Ernaehrung
  // alle Verhungert sind.
  
  if (!einwohnerzahl) return;

  long versorgt = 1000 * einfluss_aufsummieren("GV");
  long wachstum = MIN(versorgt, einwohnerzahl) / 10;
//  float wachsproz = 100 * (float)wachstum / einwohnerzahl;
  
  if (wachstum) {
    const char *z = (einwohneraenderung > 0) ? L("weitere ","further ") : "";
    if (versorgt >= einwohnerzahl) {
      report(L("Alle Einwohner sind medizinisch optimal versorgt. Deshalb ","The inhabitants enjoy a 100%% hospital coverage. Thus "));
      report(L("w~achst die Stadt um %s 10%%, also um ","the town  grows by %s 10%%, that means "),z);
      report(L("%s Einwohner.\n","%s inhabitants.\n"),myltoa(wachstum));
    }
    else {
      report(L("%s Einwohner sind medizinisch versorgt, daher","%s of your inhabitants benefit from hospital coverage, therefore "), myltoa(versorgt));
      report(L(" w~achst die Stadt um %s"," the town grows by  %s"), z);
      report(L("%s Einwohner.\n","%s inhabitants.\n"),myltoa(wachstum));
    }
    einwohnerzahl += wachstum;
    einwohneraenderung += wachstum;
  }
}


/**---------------------------------------------------------------------------
  * STADT::gesundheitliche_versorgung()
  * 
  * Ermittelt die prozentuale aerztliche Versorgung der Stadt.
  ---------------------------------------------------------------------------*/
float STADT::gesundheitliche_versorgung()
{
  if (!einwohnerzahl) return 0;
  else  {
     long versorgt = 1000 * einfluss_aufsummieren("GV");
     return float(versorgt)/float(einwohnerzahl);
   }
}


/**---------------------------------------------------------------------------
  * STADT::weiterbauen()
  * 
  * Steckt in alle Vorhaben, die gerade in der Stadt laufen, soviele
  * Resourcen hinein, wie nur irgendwie moeglich und versucht so, diese
  * fertigzustellen. Dabei wird in Reihenfolge der Prioritaet vorgegan-
  * gen und bei Vorhaben mit gleicher Prioritaet werden aeltere Projekte
  * bevorzugt.
  ---------------------------------------------------------------------------*/
void STADT::weiterbauen()
{
  // Ich Merke mir, wieviele Resourcen die Stadt vor dem Bau hatte, um
  // Buchzufuehren...

  res_be.setzen_auf(resourcenspeicher);

  // Mit dem Einfluss FZ (Forschungszuschuss) kann der Stadt eine gewisse
  // Zahl an Arbeitskrafteinheiten zugeschossen werden, die sie jede Runde
  // in Forschung stecken kann. Fuer diesen Wert lege ich einen zusaetz-
  // lichen Resourcevektor an. Zuschuss, der nicht verbraucht wird, ver-
  // faellt.

  RESOURCE_VEKTOR zuschuss(einfluss_aufsummieren("FZ"), L('A','M'));
  
  // Fuer den Ausdruck merken...
  forschungszuschuss = zuschuss.betrag();

  // res_be setze ich erstmal auf die Maximalemenge an Ressourcen, die
  // ich ueberhaupt verbauen koennte. Am Ende ziehe ich den Rest wieder
  // ab und bekomme so die Menge der verbauten Ressourcen.

  res_be.addiere(zuschuss);

  // Hier muss die ganze Liste der Bauvorhaben durchgegangen werden und
  // festgestellt, ob ein Bauwerk fertig ist.

  // Da alle Bauprojekte nach Prioritaet geordnet sind, baue ich einfach
  // solange die Liste entlang, bis sie zuende ist.

  // Gleichzeitig baue ich eine Liste aller fertiggestellten Objekte auf,
  // damit ich einen schoenen Report machen kann, indem alles nett zusammen-
  // gefasst ist.

  DOUBLIST fertiggestellt;

//  short wegen_gegner_warten = 0; // Wird nachher fuer Report benoetigt...

  VORHABEN *vorhaben = (VORHABEN *)vorhabenliste.first();
  while (!vorhaben->is_tail())
  {
     // Bevor ich weiterbaue, kontrolliere ich, ob die maximale Anzahl
     // in der Stadt nicht schon erreicht ist. In so einem Fall breche
     // ich den Bau sofort ab.

     short anzahl;
     if (0 > (anzahl=anzahl_an_stadtausbauten_pruefen(vorhaben->vorhaben))) {
       report(L("*** Es k~onnen nicht mehr als %s","*** You cannot build more than  %s"), nice_ltoa(-anzahl));
       report(L(" %s in einer Stadt gebaut werden. Deshalb wird ein "," %s in a town. "),
	 konjugation(vorhaben->vorhaben, NOMINATIV | PLURAL));
       report(L("Vorhaben nun eingestellt. ***\n","Project canceled. ***\n"));

       // Jetzt muss ich es aus der Liste entfernen und weitermachen.
       VORHABEN *zu_entfernen = vorhaben;
       vorhaben = (VORHABEN *)vorhaben->next();
       delete zu_entfernen;
       continue;
     }


     // Zuerst pumpe ich soviele Gueter hinein, wie ich auftreiben kann
     // Wenn es sich um eine Entwicklung handelt, dann gibt es einige Son-
     // derregeln. Erstens wirkt dann der Forschungszuschuss, zweitens kann
     // es sich um ein Gemeinschaftsprojekt handeln. Deshalb wird die Res-
     // sourcenverwaltung ueber ein Attribut des Staates abgewickelt.
  
     short ist_erfindung = 0;
     VORHABEN_ENZ *vorenz = enzyklopaedie()->vorhaben(vorhaben->vorhaben);
     if (!vorenz) {
	 log('I', "Fehler in STADT::weiterbauen(): Kein Vorhaben \"%s\""
                  "in der Enzyklopaedie definiert!\n",vorhaben->vorhaben);
       continue;                      
     }
     if (!mystrcmp(vorenz->start_attribute.abfragen("TYP"),"ERFINDUNG"))
     {
	ist_erfindung = 1;

	// Wenn im Staat das Attribut schon gesetzt ist, dann hole ich
	// die benoetigten Ressourcen daher. Ansonsten lasse ich sie, wie
	// sie sind und setze anschliessend das Attribut.

	if (staat()->attribut_gesetzt(vorhaben->vorhaben)
	    && !vorhaben->noch_benoetigte_resourcen.ist_null()) // ist_null() kommt vom *B Befehl!
	{       
	    vorhaben->noch_benoetigte_resourcen.setzen_auf
			(staat()->attribut(vorhaben->vorhaben));
	}

	// Falls der Staat die Entwicklung schon hat, dann wurde diese 
	// inzwischen schon von einer anderen Stadt fertiggestellt,
	// und ich entferne das Vorhaben ohne etwas zu sagen aus der Liste.

	if (staat()->info("HAT_ENTWICKLUNG",vorhaben->vorhaben)) {
	  VORHABEN *zu_entfernen = vorhaben;
	  vorhaben = (VORHABEN *)vorhaben->next();
	  delete zu_entfernen;
	  continue;
	}

	// Nun kommt der Forschungszuschuss zum Tragen.
	vorhaben->noch_benoetigte_resourcen.kompensieren_aus(zuschuss);

     } // (ist Erfindung)

     // Nun baue ich mit den normalen Mitteln ganz normal weiter.
     vorhaben->noch_benoetigte_resourcen.kompensieren_aus(resourcenspeicher);

     // Wenn das Vorhaben jetzt noch nicht fertig ist, dann versuche ich,
     // ueber das Bahnkontor Gueter zu holen.
     
     if (!vorhaben->noch_benoetigte_resourcen.ist_null()) {
       RESOURCE_VEKTOR bekommen;
       besorge_ueber_bahnkontor(vorhaben->noch_benoetigte_resourcen, 
     				MOEGLICHST_VIEL, bekommen);
       vorhaben->noch_benoetigte_resourcen.subtrahiere(bekommen);
       res_be.addiere(bekommen);
     }     

     // Wenn es eine Erfindung war, dann muss ich das Attribut im Staat
     // neu setzen.

     if (ist_erfindung) staat()->attribut_setzen(vorhaben->vorhaben,
	vorhaben->noch_benoetigte_resourcen.to_string());

     // Jetzt schaue ich, ob das Bauwerk fertig ist.
     if (vorhaben->noch_benoetigte_resourcen.ist_null())
    {
       OBJEKT *neu = fertigstellung_von(vorhaben);

       // Wenn ich den this-Pointer zurueckbekomme, dann heisst das, dass
       // ich das Vorhaben momentan nicht fertigstellen kann. In so einem
       // Fall lasse ich es in der Liste und mache mit dem naechsten weiter.
       // Deshalb werden folgende Zeilen nur unter der Bedingung neu!=this
       // ausgefuehrt.
       
       if (neu != this)
       {

	 if (neu) { // In die Liste aufnehmen...
	   OBJEKT_LIST_NODE *node = new OBJEKT_LIST_NODE;
	   node->objekt = neu;
	   fertiggestellt.insert(node);
	 }
	 // Jetzt loesche ich noch das Attribut im Staat, falls es eine
	 // Entwicklung war.

	 if (ist_erfindung) staat()->attribut_loeschen(vorhaben->vorhaben);

	 // Jetzt muss ich es aus der Liste entfernen und weitermachen.
	 // Falls es ein Mehrfachprojekt ist, dann setze ich den Zaehler
	 // um ein runter und loesche nur, wenn er auf 0 kommt.

	 vorhaben->anzahl--;
	 if (!vorhaben->anzahl) {
	   VORHABEN *zu_entfernen = vorhaben;
	   vorhaben = (VORHABEN *)vorhaben->next();
	   delete zu_entfernen;
	   continue;
	 }
	 // Ansonsten Ressourcen wieder auf Startwert.
	 else vorhaben->resourcen_ermitteln();

	 continue; // Extra nicht zum naechsten uebergehen!

       } // if (neu!=this, was heisst, Vorhaben nicht voruebergehen aufgehalten)

     } // if (noch benoetigte Ressource gleich 0.)

     // Hier gehe ich zum naechsten Vorhaben

     vorhaben = (VORHABEN *)vorhaben->next();

  } // Alle Vorhaben durchklappern

  // So. Wieviele Resourcen ich verbraucht habe, rechne ich mit. Auch merke
  // ich mir den verbrauchten Zuschuss fuer den Ausdruck
  
  forschungszuschuss_defacto = forschungszuschuss - zuschuss.betrag();

  res_be.subtrahiere(resourcenspeicher);  // nicht verbrauchte Gueter und ...
  res_be.subtrahiere(zuschuss);  // ... nicht verbrauchten Zuschuss abziehen

  // Nun habe ich auch eine Liste aller diese Runde fertiggestellter Objekte.
  // Ich gebe einen netten Report aus.

  long anzahl = fertiggestellt.number_of_elements();
  if (anzahl) {
    char *aufzaehlung = woertliche_objekt_aufzaehlung(&fertiggestellt, AKKUSATIV);
    report(L("Die Stadt stellt %s fertig.\n","The town completed %s.\n"),aufzaehlung);
    myfree(aufzaehlung);
  }

}


/**---------------------------------------------------------------------------
  * STADT::vorhaben_aufraeumen()
  * 
  * Durchsucht die Liste der Vorhaben nach Entwicklungen, die inzwischen
  * fertiggestellt sind und nimmt diese aus der Liste. Dies geschieht
  * wortlos, ohne Meldung an den Spieler.
  * Das ein Vorhaben einfach fertig werden kann, ohne das die Stadt dies
  * merkt, liegt daran, dass mehrere Staedte gemeinsam eine Entwicklung
  * vorantreiben koennen.
  ---------------------------------------------------------------------------*/
void STADT::vorhaben_aufraeumen()
{
  VORHABEN *vorhaben = (VORHABEN *)vorhabenliste.first();
  while (!vorhaben->is_tail())
  {
    // Damit ich nicht die Enzyklopaedie auch noch anrufen muss, frage
    // ich den Staat ohne Kontrolle ueber den Typ des Vorhabens, ob er
    // eine Entwicklung unter diesem Namen schon besitzt. Da es ohnehin
    // keine Entwicklung gibt, die den gleichen Namen wie ein anderes
    // Vorhaben hat, kann dies nicht schief gehen.
    
    if (staat()->info("HAT_ENTWICKLUNG", vorhaben->vorhaben)) { // Schon fertig, Vorhaben entfernen
      delete vorhaben;
      vorhaben = (VORHABEN *)vorhabenliste.first();
      continue;
    }
    
    // Jetzt kann ich die Schleife schliessen
    vorhaben = (VORHABEN *)vorhaben->next();
  }
}


/**---------------------------------------------------------------------------
  * STADT::fertigstellung_von()
  * 
  * Regelt die Fertigstellung eines Vorhabens inlcusive Schaffung des
  * Objektes und Reportage.
  *
  * @param
  * VORHABEN *  Zeiger auf die Arbeitsstrutkur des Vorhabens
  *
  * @return
  * OBJEKT *:   Zeiger auf das fertiggestellte Objekt, falls es
  * sich um ein solches handelt. Wurde etwas fertig-
  * gestellt, dem kein Objekt entspricht, so wird
  * NULL zurueckgegeben (Entwicklung). Kann das
  * Vorhaben MOMENTAN gerade nicht fertiggestellt wer-
  * den, zu einem spaeteren Zeitpunkt aber schon, so
  * wird this uebergeben!
  ---------------------------------------------------------------------------*/
OBJEKT *STADT::fertigstellung_von(VORHABEN *vorhaben)
{
  // Es gibt drei verschiedene Arten von Vorhaben. Um herauszufinden, um
  // welches es sich handelt, muss ich aber ersteinmal die Enzyklopaedie
  // befragen.

  if (!enzyklopaedie()) return NULL; // Ist eh alles am dampfen...

  VORHABEN_ENZ *vorenz = enzyklopaedie()->vorhaben(vorhaben->vorhaben);
  if (!vorenz) {
      log('K', "Missing entry %s in game configuration file",
	  vorhaben->vorhaben);
      return 0;
  }

  // Manche Bauten, z.B. Alle Einheiten, brauchen Einwohner.

  long mitnehmen = myatol(vorenz->start_attribute.abfragen("EINWOHNER"));
  if (einwohnerzahl <= mitnehmen) {
	report(L("In der Stadt sind nur noch %s Einwohner, ","The town is populated by  %s inhabitants, but "), myltoa(einwohnerzahl));
	report(L("%s ben~otigt ","%s requires "), konjugation(vorenz->name, NOMINATIV|SINGULAR));
	report(L("aber %s Einwohner und kann daher nicht "," %s. That´s the reason why "), myltoa(mitnehmen));
	report(L("fertiggestellt werden. Das Vorhaben mu~s daher noch warten. \n",".the project cannot be completed by now. \n"));
	return this;
  }

  einwohnerzahl -= mitnehmen; // Einwohner abziehen.
  einwohneraenderung -= mitnehmen;

  // So. Jetzt kann ich mal die Attibute untersuchen...
  char *typ = vorenz->start_attribute.abfragen("TYP");

  if (!strcmp(typ, "ERFINDUNG")) {

    // Der Report sollte aus dem Lexikon bedient werden.
    report(L("Unsere Forscher entwickeln %s.\n","Our mad scientists finally  have researched the secret of %s.\n"),
		 konjugation(vorenz->name, AKKUSATIV|SINGULAR));

    // Jetzt teile ich dem Staat ueber ein Kommando mit, dass er
    // eine neue Entwicklung verbuchen darf.
    staat()->kommando("ENTWICKLUNG", vorenz->name);
  }

  else if (!strcmp(typ, "EINHEIT")) // Einheit ausgehoben
  {
    OBJEKT *neu;
    if (!(neu = objekt_schaffen(eindeutiger_name(), typ,
	vorenz->start_attribute.to_string())))
    {
	log('I', "STADT::fertigstellen_von(): Kann Objekt nicht schaffen");
    }
    else {
      // Jetzt muss ich das die Einheit noch in die richtige Landschaft
      // versetzen, und zwar an die Stelle, an der sich auch die Stadt
      // befindet.

      neu->attribut_setzen("BAUJAHR", g_uhr->info("ZUGNUMMER"));
      neu->ort_wechseln(this);  // Nun ist sie kurzzeitig in der Stadt...
      neu->verlassen();         // ... aber jetzt verlaesst sie sie schon!

      // Manche Einheiten (z.B. Siedler) nehmen aus der Stadt einen Anteil
      // der Bevoelkerung mit. Die Zahl der mitzunehmenden steht im Attribut
      // EINWOHNER der Einheit.

      log('b',"Unit builded: %s -> %s -> %s (%s)",
	  besitzer->name, name, neu->name, neu->a_gattung());
      return neu;
    }
  } // EINHEIT

  else if (!strcmp(typ, "STADTAUSBAU"))
  {
    // Zuerst schaffen des neuen Objektes...
    OBJEKT *neu;
    if (!(neu = objekt_schaffen(eindeutiger_name(), typ,
	vorenz->start_attribute.to_string())))
    {
      // Kann Objekt nicht schaffen!
	log('I', "STADT::fertigstellen_von(): Kann Objekt nicht schaffen");
    }
    else { // Geglueckt

      // Der Ort des Gebaudes ist die Stadt. Die genaue Adresse ist unwichtig
      // und wird folglich nicht weiter beachtet.
      neu->ort_wechseln(this);
      log('b',"Building builded: %s -> %s -> %s (%s)",
	  besitzer->name, name, neu->name, neu->a_gattung());
      return neu;
    }
  } // Stadtausbau

  else { // Unbekannter Typ
      log('K', "Unknown type '%s' of project in game configuration file", typ);
  }

  return NULL; // Kein Objekt fertiggestellt.

}


/**---------------------------------------------------------------------------
  * STADT::milizen_bauen()
  * 
  * Ist der Spieler ausgestiegen, so baut die Stadt selbstaendig
  * eine besondere Art von Verteidigunseinheit: die Milizen. Pro
  * 100.000 angebrochene Einwohner unterhaelt sie eine solche
  * Miliz. Sollte sie weniger haben, so baut sie pro Runde eine,
  * und das genau hier in diese Funktion.
  ---------------------------------------------------------------------------*/
void STADT::milizen_bauen()
{
  short anzahl = alle_objekte_im_direkten_besitz(L("GATTUNG=Miliz","GATTUNG=Militia"))->count_and_del();
  if ((einwohnerzahl+99999) / 100000 <= anzahl) return; // Schon genug.

  // Ich erzeuge nun einfach ein Vorhaben und dass lasse ich dann von
  // meine fertigstellung_von(..) fertigstellen.

  VORHABEN vorhaben(L("Miliz","Militia"), 9, 1);
  fertigstellung_von(&vorhaben);
  
  // Die Miliz ist gleich eingegraben, da das schon in ihren Startattri-
  // buten steht.
}

/**---------------------------------------------------------------------------
  * STADT::info()
  * 
  * Infofunktion, die fuer alle Objekte ansprechbar ist wie in
  * objekt.h spezifiziert.
  ---------------------------------------------------------------------------*/
char *STADT::info(char *info, void *par1, void *par2, void *par3)
{
  if (!strcmp("RESSOURCEN_ANFORDERN",info)) return info_resourcen_anfordern(par1, par2, par3);
  else if (!strcmp("EINWOHNERZAHL",info)) return info_einwohnerzahl();
  else if (!strcmp("BAU_UND_ENTWICKLUNG",info)) return info_bau_und_entwicklung();
  else if (!strcmp("ROHSTOFFPRODUKTION",info)) return myltoa(res_bew[L('H','W')]
							     +res_bew[L('S','S')]);
  else if (!strcmp("UNTERSUCHE",info)) return info_untersuche(par1);
  
  else return NULL;
}

char *STADT::info_einwohnerzahl()
{
  return myltoa(einwohnerzahl);
}


/**---------------------------------------------------------------------------
  * STADT::info_resourcen_anfordern()
  * 
  * Dieses Info ist gleichzeitig ein Kommando. Mit ihm koennen von der
  * Stadt Resourcen angefordert werden. Das Ergebnis teilt ueber die
  * tatsaechlich bekommenen Resourcen mit. Ein Staat ohne den Handel
  * kann nicht mit Resourcen handeln. Dieser "Handels-Einfluss" hat
  * den Namen "MP".
  * Es kann ausserdem ein Maximaler Betrag angegeben werden, falls der
  * Empfaenger auf jeden Fall irgendwelche Ressourcen haben will, aber
  * das Angebot nicht genau kennt.
  * Ist der Resstring negativ, so handelt es sich um eine Lieferung, und
  * der Maxbetrag wird ignoriert.
  * @param
  * (char *)par1:           Ressourcestring
  * (OBJEKT *)par2:     Partner
  * (long *)par3:           Maximalbetrag
  * @return
  * char *: Ressourcestring des tatsaechlich bekommenen.
  ---------------------------------------------------------------------------*/
char *STADT::info_resourcen_anfordern(void *par1, void *par2, void *par3)
{
    RESOURCE_VEKTOR erwuenscht((char *)par1);
    OBJEKT *geber = (OBJEKT *)par2;
    long max_betrag = *(long *)par3;
    
    if (!einfluss_vorhanden("MP")) { // Kein Handel
	report(L("Da unserer Staat den \"Handel\" noch nicht kennt, kann die Einheit ","Because our Empire does still not know the secrets of \"Trade\" , the unit "));
	report(L("%s hier keine G~uter umladen.\n","%s is unable to load/unload any commodities.\n"), geber->name);
	return NULL;
    }

    if (ausnahmezustand()) {
	report(L("Die Stadt befindet sich im Ausnahmezustand! Ein Umladen von G~utern ist der Einheit %s daher v~ollig unm~oglich!\n", "The town is in a state of emergency, thus it your unit %s is not able to load or unload any commodities!\n"), geber->name);
    }


  if (erwuenscht.ist_negativ()) { // Ich bekomme also was.
    erwuenscht.negieren();
    // Ich mache keine Kontrolle, ob ich auch Speichern kann!
    resourcenspeicher.addiere(erwuenscht);
    report(L("Wir erhalten eine Lieferung (%s) von der","We happily received  (%s)  delivered by unit"), erwuenscht.to_string());
    report(L(" Einheit %s.\n","  %s.\n"),geber->name);
    return erwuenscht.to_string();
  }
    
  else { // Ich soll etwas herausruecken...
    // Ich gebe nicht mehr, als der Geber haben will.
    if (max_betrag) erwuenscht.begrenzen_auf(max_betrag);

    // Was kann ich davon selbst aufbringen?
    RESOURCE_VEKTOR aus_eigener_kasse(resourcenspeicher);
    aus_eigener_kasse.begrenzen_auf(erwuenscht);
    resourcenspeicher.subtrahiere(aus_eigener_kasse);
    
    // Jetzt vom Rest noch moeglichst viel vom Bahnkontor holen
    RESOURCE_VEKTOR vom_kontor_zu_besorgen(erwuenscht);
    vom_kontor_zu_besorgen.subtrahiere(aus_eigener_kasse);
    RESOURCE_VEKTOR vom_kontor;
    besorge_ueber_bahnkontor(vom_kontor_zu_besorgen, MOEGLICHST_VIEL, vom_kontor);

    erwuenscht.setzen_auf(vom_kontor);
    erwuenscht.addiere(aus_eigener_kasse);

    report(L("An die Einheit %s ","The unit  %s "), geber->name);
    erwuenscht.negieren();
    report(L("wurden G~uter ausgeliefert (%s).\n","picked up some commodities (%s).\n"),erwuenscht.to_string());
    erwuenscht.negieren();
    return erwuenscht.to_string();
  }
  
  return NULL; // control reaches end of non-void function? Wieso denn?
}


/**---------------------------------------------------------------------------
  * STADT::info_untersuche()
  * 
  * Wird von den Einheiten angefordert, wenn sie UN in Richtung einer
  * Stadt machen. Der Parameter gibt an, welche Art von Informationen
  * geholt werden soll. Es ist ein Zeiger auf eine long-Zahl.
  ---------------------------------------------------------------------------*/
char *STADT::info_untersuche(void *par1)
{
  long was = *(long *)par1;
  static char antwort[2000];
  FILE *file;
  int ms; // Staerke der Milizen

  switch (was) {
   case 2:      // Einwohnerzahl
     ms = milizen_staerke();
     sprintf(antwort, L("In %s leben %ld Menschen. %s%s%s\n","In %s live and prosper %ld people. %s%s%s\n"),
       a_name(), einwohnerzahl,
       ms ? L("Der Staat hat keinen Herrscher mehr. Die Milizen der Stadt verteidigen mit der Gesamtst~arke ","The Fallen Empire is without leader. The Town Militias defend with a total strength ") : "",
       ms ? myltoa(ms) : "",
       ms ? ".\n" : "");
     break;
     
   case 3:      // Stadtausbauten
     file = fopen(tempfile_name(), "w");
     if (!file) return "";
     ausbauten_in_file(file, NULL);
     fclose(file);
     file = fopen(tempfile_name(), "r");
     if (!file) return "";
     fgets(antwort, 2000, file);
     fclose(file);
     break;
     
   case 4:      // Speicher
     if (resourcenspeicher.ist_null()) 
       sprintf(antwort, L("%s hat keine G~uter gespeichert.\n","%s has no commodities in store.\n"),a_name());
     else 
       sprintf(antwort, L("%s hat im Speicher %s.\n","%s has %s in store.\n"), a_name(),
	 resourcenspeicher.to_string());
     break;

   default:     // Keine Information
     return L("Keine Information verf~ugbar.","No informations available.");
  }
  
  return antwort;
}
     

/**---------------------------------------------------------------------------
  * STADT::milizen_staerke()
  * 
  * 0, wenn keine Miliz in der Stadt ist.
  * Die Verteidigunskraft der Miliz, falls doch.
  ---------------------------------------------------------------------------*/
int STADT::milizen_staerke()
{
  int staerke = 0;
  FOR_EACH_OBJEKT_IN (welt()->alle_objekte_bei_adresse(adresse, L("GATTUNG=Miliz","GATTUNG=Militia")))
  DO_AND_DELETE (staerke = myatol(objekt->info("VERTEIDIGUNGSKRAFT")) )
  return staerke;
}

     
/**---------------------------------------------------------------------------
  * STADT::info_bau_und_entwicklung()
  * 
  * Wieviele Arbeitskraft wurden in Bau und Entwicklung gesteckt.
  * Forschungsboni und dergleichen miteingerechnet. Der Wert ist der-
  * gleiche, der auf dem Stadtausdruck in der Spalte Bau/Entw. steht.
  ---------------------------------------------------------------------------*/
char *STADT::info_bau_und_entwicklung()
{
  return myltoa(res_be[L('A','M')]);
}


/**---------------------------------------------------------------------------
  * STADT::kommando()
  * 
  * Zweck wie in OBJEKT::kommando() beschrieben.
  * @param
  * Siehe OBJEKT::kommando()
  * @return
  * short je nach kommando. Normalerweise 0 bei OK und 1 bei Misserfolg.
  ---------------------------------------------------------------------------*/
short STADT::kommando(const char *kommando, const void *par1, const void *par2, const void *par3)
{
  if (!mystrcmp("RESOURCEN_ANFORDERN",kommando))
	return kommando_resourcen_anfordern((void *)par1);
  else if (!mystrcmp("EINGENOMMEN", kommando))
	return kommando_eingenommen((void *)par1, (void *)par2);
  else if (!mystrcmp("BESCHUSS",kommando))
	return kommando_beschossen_werden((void *)par1, (void *)par2);
  else if (!mystrcmp("ABWEHRTEST",kommando))
	return kommando_abwehrtest((void *)par1);
  else if (!mystrcmp("RESOURCEN",kommando))
	return kommando_resourcen_bekommen((void *)par1, (void *)par2);
  else if (!mystrcmp("ANSIEDLUNG",kommando))
	return kommando_neue_einwohner((void *)par1);
  else if (!mystrcmp("UMWANDLUNG",kommando))
	return kommando_umwandlung((void *)par1);
  else if (!mystrcmp("AUSSTIEG",kommando))
	return kommando_ausstieg();
  else if (!mystrcmp("EISENBAHN_BRINGT",kommando))
	return kommando_eisenbahn_bringt((void *)par1, (void *)par2, (void *)par3);
  else if (!mystrcmp("WELTBAUT_UEBERNEHMEN",kommando))
  	return kommando_weltbaut_uebernehmen((void *)par1);

  else return 1; // Wird ignoriert.
}


/**---------------------------------------------------------------------------
  * STADT::kommando_weltbaut_uebernehmen(void)
  * 
  * Die Stadt bekommt von einem anderen Staat ein Bergwerk oder sowas.
  * Wird vom Befehl US an eine Einrichtung aufgerufen.
  ---------------------------------------------------------------------------*/
short STADT::kommando_weltbaut_uebernehmen(void *par1)
{
    OBJEKT *weltbaut = (OBJEKT *)par1;
    OBJEKT *alter_staat = weltbaut->besitzer;
    if (!alter_staat || !alter_staat->typ_ist("STAAT")) return 1;
    
    report(L("%s will uns ","%s wants to give us "), alter_staat->a_name());
    report(L("%s ~ubergeben","%s."), konjugation(weltbaut->a_gattung(), AKKUSATIV|SINGULAR));
    
    if (staat()->kommando("DIPL_STATUS", weltbaut) < 2) {
	report(L(". Wir lehnen dies ab, solange dieser Staat nicht zu unseren ",". We stubbornly refuse this because that Empire is not our "));
	report(L("Verb~undeten z~ahlt.\n"," ally!\n"));
	return 1;
    }
    
    alter_staat->geben(staat(), weltbaut);
    weltbaut->attribut_setzen("VERSORGER", this->name);
    report(L(" (%s). Wir nehmen das Angebot an. "," (%s). Honored, we accept this gift. "), weltbaut->name);
    return 0;
}


/**---------------------------------------------------------------------------
  * Die Stadt uebernimmt eine Einrichtung von einer anderen Stadt oder
  * gar von einem anderen Staat. Wird aufgerufen, wenn eine Stadt eingenommen,
  * Atomgebombt oder aufgeloest wurde.
  ---------------------------------------------------------------------------*/
void STADT::einrichtung_uebernehmen(STADT *, WELTBAUT *weltbaut)
{
    report(L("Wir ~ubernehmen die Kontrolle ~uber %s ","We gain control of  %s "), weltbaut->a_gattung());
    report("%s. ", weltbaut->name);
    if (staat() != weltbaut->staat())
	weltbaut->staat()->geben(staat(), weltbaut);
    weltbaut->attribut_setzen("VERSORGER", this->name);
    weltbaut->report(L("Wir wurden der Stadt %s unterstellt. ","We have been assigned to town  %s "), a_name());
}



/**---------------------------------------------------------------------------
  * STADT::kommando_abwehrtest()
  * 
  * Gibt den Abwehrraketen Gelegenheit zu einer Abwehr von Atomraketen.
  ---------------------------------------------------------------------------*/
short STADT::kommando_abwehrtest(void *par1)
{
  OBJEKT *basis = (OBJEKT *)par1;
  DOUBLIST *abwehrraketen = alle_objekte_im_direkten_besitz("ATOMABWEHR");
  short abgewehrt = 0,     // Nur boolscher Wert
        abgeschossen = 0,  // Soviele Raketen wurden abgeschossen
        anzahlraketen = abwehrraketen->number_of_elements();
  
  report(L("Und dann kam die Atombombe aus %s...#\n","The sun was blotted out by the mighty nuclear bomb launched by %s...#\n"),basis->besitzer->a_name());
  while (!abgewehrt && abwehrraketen->number_of_elements())
  {
    OBJEKT *ar = ((OBJEKT_LIST_NODE *)abwehrraketen->first())->objekt;
    delete abwehrraketen->first(); // Aus der Liste nehmen
    long chance = myatol(ar->attribut("ATOMABWEHR"));
    ar->zur_vernichtung_vormerken(); // Rakete selbst verbrauchen
    if (io_random(100) < chance) abgewehrt = 1;
    abgeschossen++;
  }
  
  if (abgewehrt) {
    if (abgeschossen==1 && anzahlraketen==1)
      report(L("Aber unsere Abwehrrakete macht sie souver~an unsch~adlich!\n","but our nifty Defensive Missile crushed the lethal bomb!\n"));
    else if (abgeschossen==1)
      report(L("Aber gleich die erste Abwehrrakete kann sie abschie~sen.\n","but our Defensive Missile rendered it harmless.\n"));
    else 
      report(L("Von unseren Abwehrraketen mussten wir %s losschicken, bis ","Trembling, we had to launch  %s Defensive Missiles until we ")
             L("endlich eine getroffen hat.\n","eventually got rid of  the impending danger!ô \n"), nice_ltoa(abgeschossen));
  }
  else {
    if (anzahlraketen==1)
      report(L("Unsere einzige Abwehrrakete trifft leider nicht!\n","Our only Defensive Missile DID NOT STRIKE!!\n"));
    else if (anzahlraketen>1)
      report(L("Aber keine von unseren %s Abwehrraketen trifft!\n","But none of our %s Defensive Missiles DID STRIKE! \n"),anzahlraketen);
  }

  return abgewehrt;
}    


/**---------------------------------------------------------------------------
  * STADT::kommando_eisenbahn_bringt()
  * 
  * Sagt der Stadt, dass Gueter mit der Bahn eintrudeln.
  ---------------------------------------------------------------------------*/
short STADT::kommando_eisenbahn_bringt(void *par1, void *par2, void *par3)
{
  RESOURCE_VEKTOR *gueter = (RESOURCE_VEKTOR *)par1;
  long personen = *((long *)par2);
  OBJEKT *herkunft = (OBJEKT *)par3;
  
  if (!gueter->ist_null()) {
    report(L("Mit der Eisenbahn aus %s kommen ","From %s come "), herkunft->a_name());
    report("%s", gueter->to_string());
    if (personen) report(L(" und %s Personen.\n"," and %s people by train.\n"), myltoa(personen));
    else report(".\n");
  }
  else if (personen) {
    report(L("%s Personen kommen mit der Eisenbahn ","%s people come by train "), myltoa(personen));
    report(L("aus %s. \n","from %s. \n"), herkunft->a_name());
  }
  
  einwohnerzahl += personen;

  einwohneraenderung += personen;
  resourcenspeicher.addiere(*gueter);
  return 0;
}


/**---------------------------------------------------------------------------
  * STADT::kommando_ausstieg()
  * 
  * Bekommt die Stadt, wenn der Stadt aus dem Spiel ausgestiegen ist.
  ---------------------------------------------------------------------------*/
short STADT::kommando_ausstieg()
{
  vorhabenliste.clear();
  alle_befehle_abbrechen();
  return 0;
}


/**---------------------------------------------------------------------------
  * STADT::kommando_umwandlung()
  * 
  * Kommando, dass nur intern benoetigt wird fuer die Funktion
  * EINFLUSS_LISTE::kommando_fuer_jeden_einfluss(), die wiederum
  * von der Funktion resourcen_umwandlung() aufgerufen wird, welche
  * den Einfluss UMx berechnen soll. Die eigentliche Berechnung
  * der einzelnen UMx-Einfluesse geschieht nun hier.
  *
  * @param
  * (char *) Einflussparameter: Faktor*Res1,Res2. Fuer jede Einheit
  * im Faktor kann einmal eine Resourcemenge Res1 in eine
  * Resourcemenge Res2 umgewandelt werden.
  * 
  * Ein Einflussparameter kann auch "markiert" sein. Dass
  * heisst, dass er als Praefik einen Buchstaben und einen
  * Doppelpunkt hat. Z.B. S:1*10E10M,10I
  ---------------------------------------------------------------------------*/
short STADT::kommando_umwandlung(void *par1)
{
  // Erstmal die Parameter holen.
  char *param = (char *)par1;

  // Markierung holen.

  char markierung;  
  if (param[1] == ':') {
    markierung = param[0];
    param += 2;
  }
  else markierung = 0;

  char *pardup = mystrdup(param);
  char *res1, *res2;
  for (res1 = pardup; *res1 && *res1!='*'; res1++);
  for (res2 = res1; *res2 && *res2!=','; res2++);
  *res1++ = 0;
  *res2++ = 0;

  RESOURCE_VEKTOR quellresource(res1);
  RESOURCE_VEKTOR zielresource(res2);
  long faktor = myatol(pardup);

  // Wenn es eine Markierung gibt, dann kann Einfluss die Zielressourcen
  // erhoehen. Z.B. Der Einfluss MODIF-S fuer die Markierung S:.
  // Der Parameter ist dann eine Anzahl von INDUSTRIEGUETERN! Also
  // noch alles ziemlich SPEZIELL!
  
  if (markierung) {
    char einfluss[50];
    sprintf(einfluss, "MODIF-%c", markierung);
    zielresource[L('I','I')] += einfluss_aufsummieren(einfluss);
  }

  myfree(pardup);

  // Und jetzt probiere ich solange aus, ob die Quellresourcen noch
  // im Resourcenspeicher vorhanden sind, und ersetze sie durch res2,
  // bis der faktor auf 0 ist oder nicht mehr genug Ressourcen da sind.
  // Bei kommando_resourcen_anfordern() verwende ich den zusaetzlichen
  // Parameter 0, damit die aufgewendeten Rohstoffe nicht zu "Versorgung"
  // gerechnet werden, in der Rohstofftabelle der Stadtuebersicht.

  while (faktor && !kommando_resourcen_anfordern(quellresource.to_string(), 0))
  {
    resourcenspeicher.addiere(zielresource);
    faktor--;
  }

  // Und das war's auch schon wieder....
  return 1;
}


/**---------------------------------------------------------------------------
  * STADT::kommando_neue_einwohner()
  * 
  * Fuegt der Stadt neue Einwohner hinzu. Wird z.B. verwendet, wenn eine
  * Siedlereinheit einer Stadt hinzugefuegt wird (mit Befehl AS)
  *
  * @param
  * (long *)        Anzahl neuer Einwohner
  ---------------------------------------------------------------------------*/
short STADT::kommando_neue_einwohner(void *par1)
{
  long neu = *(long *)par1;
  if (!neu) return 1;

  report(L("Die Stadt nimmt %s neue Einwohner auf.\n","The town welcomes  %s new citizens.\n"), neu);
  einwohnerzahl += neu;
  einwohneraenderung += neu;
  return 0;
}


/**---------------------------------------------------------------------------
  * STADT::kommando_resourcen_bekommen()
  * 
  * Die Stadt bekommt von aussen (wahrscheinlich von einer Einheit)
  * Resourcen.
  *
  * @param
  * (char *)        Resourcenstring
  * (OBJEKT *)  Geberobjekt
  *
  * @return
  * short 0, falls die Stadt die Resourcen aufgenommen hat.
  * short 1, sonst.
  ---------------------------------------------------------------------------*/
short STADT::kommando_resourcen_bekommen(void *par1, void *par2)
{
  RESOURCE_VEKTOR bekommen((char *)par1);
  OBJEKT *geber = (OBJEKT *)par2;

  if (bekommen.ist_null()) return 0;

  // Ohne den Handel ist keine Annahme moeglich.

  if (!einfluss_vorhanden("MP")) { // MP wird vom Handel ausguebt.
    report(L("Die Einheit %s m~ochte G~uter an uns ausliefern, wir","Unit  %s wants to unload commodities, but "),geber->name);
    report(L(" k~onnen aber nicht annehmen, da unser Staat dazu die Entwicklung "," we cannot receive them. We need the to know the secrets of Trade "));
    report(L("der Handels haben mu~s!\n","to receive commodities!\n"));
    return 1;
  }

  if (ausnahmezustand()) {
      report(L("Die Einheit %s will G~uter ausliefern. Dies geht nicht, weil sich die Stadt im Ausnahmezustand befindet.\n", "Unit s% is unable to deliver commodities because the town is still in a state of emergency.\n"), geber->name);
  }

  resourcenspeicher.addiere(bekommen);

  report("%s", geber->a_gattung());
  report(L(" %s liefert "," %s delivers "), geber->name);
  report("%s.\n",(char *)par1);
  return 0;
}


/**---------------------------------------------------------------------------
  * STADT::kommando_resourcen_anfordern()
  * 
  * Mit diesem Kommando kann ein Objekt, das von der Stadt versorgt wird,
  * vor der Stadt einer Lieferung an Resourcen beantragen. Die Stadt
  * wird auf jeden Fall versuchen, die benoetigten Resourcen zur Ver-
  * fuegung zu stellen. Zur Not auch ueber ein Bahnkontor.
  *
  * @param
  * (char *)        Benoetigte Resourcen in Stringform.
  * short protokoll	1, wenn die geforderten Guetern in der Spalte
  * Versorgung beim Stadtausdruck erscheinen soll.
  * 0, sonst.
  *
  * @return
  * 0, falls genuegend Resourcen zur Verfuegung standen,
  * 1, falls nicht genug Resourcen uebrig waren. In diesem Fall
  * wurden von der Stadt ueberhaupt keine Resourcen abgezogen.
  ---------------------------------------------------------------------------*/
short STADT::kommando_resourcen_anfordern(void *par1, short protokoll)
{
    if (ausnahmezustand()) return 1;

    RESOURCE_VEKTOR benoetigt((char *)par1);

    // Jetzt muss ich feststellen, ob noch genuegen Resourcen da sind..
    
    RESOURCE_VEKTOR aus_eigener_kasse(resourcenspeicher);
    aus_eigener_kasse.begrenzen_auf(benoetigt);
    benoetigt.subtrahiere(aus_eigener_kasse);

    if (!benoetigt.ist_null()) { // Dann versuche ich es mit dem Bahnkontor
	RESOURCE_VEKTOR bekommen;
	besorge_ueber_bahnkontor(benoetigt, ALLES_ODER_NICHTS, bekommen);
	benoetigt.subtrahiere(bekommen);
	if (!benoetigt.ist_null()) return 1; // Klappt nicht!
	if (protokoll && !attribut_gesetzt("FrischGegruendet"))
	    res_unt.subtrahiere(bekommen);
    }
    
    resourcenspeicher.subtrahiere(aus_eigener_kasse);
    if (protokoll && !attribut_gesetzt("FrischGegruendet"))
	res_unt.subtrahiere(aus_eigener_kasse);
    return 0;
}


/**---------------------------------------------------------------------------
  * STADT::kommando_eingenommen()
  * 
  * Wird vom Feind aufgerufen, wenn die Stadt von ihm eingenommen wird.
  * Erzeugt den Report bei der Stadt und beim Staat. Loest alle Einheiten
  * auf, die die Stadt unterhalten hat. Die aufgeloesten Einheiten
  * werden dann dem Staat unterstellt, damit der Ausdruck beim richtigen
  * Spieler erscheint. Ausserdem wird die Projektliste geloescht. Wenn
  * ich die Hauptstadt war, dann wird die Gattung neu berechnet.
  * Einrichtungen werden der groessten Stadt des restlichen Reiches ue-
  * berstellt. Wenn es keine mehr gibt, bekommt der Angreifer sie.
  *
  * @param
  * (OBJEKT *)  Angreifende Einheit
  * (OBJEKT *)  Gegnerischer Staat
  ---------------------------------------------------------------------------*/
short STADT::kommando_eingenommen(void *par1, void *par2)
{
    anzahl_kaempfe ++;

    // Zuerst hole ich mir die Daten
    
    EINHEIT *angreifer = (EINHEIT *)par1;
    OBJEKT *gegnerstaat = (OBJEKT *)par2;
    
    // Da der alter Besitzer der Stadt ohnehin keinen Report der Stadt mehr
    // bekommt, spreche ich gleich den neuen Besitzer an.
    
    reportliste_loeschen(); // (!) Der neue Besitzer bekommt die alten Reps. nicht.
    report(L("Truppen aus %s ~ubernehmen die Stadt. Alle Projekte werden eingestellt.\n","Troops from  %s conquer the pitiful town. All projects immediately canceled.\n"),
	   gegnerstaat->a_name());
    vorhabenliste.clear();
    
    // Ausnahmezustand
    long azdauer = myatol(lexikon_eintrag("ausnahmezustand", 1));
    if (azdauer > 0) {
	if (ausnahmezustand()) { // Stadt befindet sich bereits im Ausnahmezustand
	    char *gruendername = attribut("GRUENDER");
	    OBJEKT *gruender;
	    if (gruendername && 0 != (gruender = objekt_mit_namen(gruendername)) && gruender == gegnerstaat)
	    {
		attribut_loeschen("AUSNAHMEZUSTAND");
		report(L("Endlich konnte unsere geliebte Stadt aus den F~angen der finsteren Gegner zur~uckerobert werden. Der Ausnahmezustand wird sofort aufgehoben!\n", "Eventually, our beloved town could be snatched from the grasp of our malicious opponents. State of emergency is over!\n"));
	    }
	    else { // vom Regen in die Traufe
		attribut_setzen("AUSNAHMEZUSTAND", 
				myltoa(1 + myatol(lexikon_eintrag("ausnahmezustand", 1))));
		report(L("Vom Regen in die verdammte Traufe! Schon wieder wird die geliebte Stadt erobert. In der Stadt breitet sich Chaos aus. Der Ausnahmezustand wird ab Anfang n~achster Runde %s volle Runden dauern.\n","Jump out of the frying-pan into the fire... Once again, the town is conquered and occupied, and plunging into chaos again. From beginning of next turn, the town will be in state of emergency for %s turns.\n" ), lexikon_eintrag("ausnahmezustand",1));
	    }
	}
	else {
		attribut_setzen("AUSNAHMEZUSTAND", 
				myltoa(1 + myatol(lexikon_eintrag("ausnahmezustand", 1))));
		report(L("Die B~urger sind entsetzt ~uber die gewaltsame Einnahme ihrer geliebten Heimatstadt. Der Ausnahmezustand tritt ein und wird ab Anfang n~achster Runde %s volle Runden dauern.\n", "The citizens are shocked by the brutal invasion of their beloved town. From beginning of next turn, the town will be in state of emergency for %s turns.\n"), lexikon_eintrag("ausnahmezustand",1));
		attribut_setzen("GRUENDER", staat()->a_name());
	}
    }

    // Jetzt informiere ich meinen neuen Besitzer ueber sein Glueck.

    gegnerstaat->kommando("STADT_EROBERT", this);
    
    // Nachdem die Stadt nicht mehr dem Staat gehoert, bekommt dieser
    // auch keinen Bericht mehr. Die Reports gehen bereits an den neuen
    // Staat. Der alte sollte allerdings davon informiert werden. Das
    // mache ich nun:
    
    staat()->kommando("STADT_VERLOREN", this, angreifer);

    // Als drittes uebernimmt der Staat  alle Einheiten, die die Stadt
    // noch besitzt, damit die Reports noch ausgegeben werden.
    // Ich kann hier nicht die alle_objekte_im_besitz() verwenden,
    // da dann die bereits vernichteten Einheiten nicht mit umge-
    // stellt werden.
    
    OBJEKT *objekt = (OBJEKT *)besitztum.first();    
    while (!objekt->is_tail())
    {
	if (objekt->typ_ist("EINHEIT")) {
	    geben(staat(), objekt);
	    objekt->tagebuch_fuehren(L("Unsere Heimatstadt wird erobert","Our Capital City has been conquered!!!"), ", ");
	    objekt->kommando("VERNICHTUNG");
	    objekt = (OBJEKT *)besitztum.first();
	}
	else NEXT(objekt);
    }
    
    
    // Einrichtungen gehen an die groesste Stadt des restlichen Staates.
    // Wenn es keine mehr gibt, dann an den Angreifer.

    staat()->einrichtungen_uebergeben(this, angreifer);

    // Und nun wechsle ich feierlich meinen Besitzer.
    geben(gegnerstaat, this);
    
    // Und nun wird noch die Gattung neu berechnet. Ich koennte ja die
    // Hauptsstadt gewesen sein. Das gattung_berechnen() geht aber nun
    // von einem neuen Staat aus, dessen Hauptstadt ich nicht bin.
    
    gattung_berechnen();
    
    return 0;
}


/**---------------------------------------------------------------------------
  * STADT::kommando_beschossen_werden()
  * 
  * Wird vom Schuetzen aufgerufen, wenn er eine Stadt bombardiert.
  * Die Stadt berechnet dann, ob irgendetwas getroffen wurde (von
  * den Einrichtungen).
  * Der Staat wird gebeten, den DS zu kontrollieren.
  *
  * @param
  * (char *)    Feuerkraft als String
  * (OBJEKT *)  Beschiessendes Objekt
  ---------------------------------------------------------------------------*/
short STADT::kommando_beschossen_werden(void *par1, void *par2)
{
    anzahl_kaempfe ++; // fuer die Anzahl der Saebel im HTML-Printout.

    long feuerkraft = myatol((char *)par1);
    OBJEKT *bombardeur = (OBJEKT *)par2;
    const short streckungsfaktor = 10;

    // Die Feuerkraft 9999 steht symbolisch fuer "undendlich": Atomrakete
    
    if (feuerkraft == 9999) return atomarer_beschuss(bombardeur);
    
    staat()->kommando("AGGRESSION",par2);

    // Report beginnen.

    report(L("Wir werden von %s aus","We are bombarded by %s "),
	   konjugation(bombardeur->a_gattung(), DATIV | SINGULAR));
    report(L(" %s bombardiert!\n"," from %s!\n"),
	   bombardeur->besitzer->besitzer->a_name());
    
    // Als moegliche Opfer stehen Einwohner und Stadtausbauten zur Ver-
    // fuegung. Pro Feuerkrafteinheit gehen im Schnitt 1000 Einwohner drauf.
    
    if (einwohnerzahl) // Nur dann macht's Sinn.
    {
	long opfer = io_random(2000) * feuerkraft;
	if (opfer >= einwohnerzahl)
	    opfer = einwohnerzahl / (io_random(2)+2); // Entweder Haelfte oder Drittel
	
	if (opfer) {
	    report(L("Dabei kommen %s Einwohner um.\n","This results in the painful death of  %s inhabitants.\n"), myltoa(opfer));
	    einwohnerzahl -= opfer;
	    einwohneraenderung -= opfer;
	}
    }

    // Und jetzt koennen noch Stadtausbauten dran glauben. Ich lege eine
    // Liste der zerstoerten Objekte an.
    
    DOUBLIST zerstoert;
    
    // Das Feuerpotential verringert sich mit der Zeit.
    long potential = feuerkraft * streckungsfaktor;
    
    DOUBLIST *ausbauten = alle_stadtausbauten();
    ausbauten->shuffle();
    FOR_EACH_OBJEKT_IN(ausbauten) DO ({
	feuerkraft = potential / streckungsfaktor;
	if (feuerkraft <= 0) break;
	long rwert = objekt->kommando("BESCHUSS", &feuerkraft, par2);
	if (rwert) {
	    potential -= io_random(rwert * streckungsfaktor) + 1;
	    OBJEKT_LIST_NODE *neu = new OBJEKT_LIST_NODE;
	    neu->objekt = objekt;
	    zerstoert.insert(neu);
	}
    });
    delete ausbauten;
    
    // Jetzt reporte ich noch, was so alles draufging...
    if (!zerstoert.is_empty())
    {
	char *aufzaehlung = woertliche_objekt_aufzaehlung(&zerstoert, NOMINATIV);
	report ((zerstoert.number_of_elements() > 1 ?
		  L("Es wurden %s zerst~ort.\n","Oops, %s have been destroyed.\n")
		: L("Es wurde %s zerst~ort.\n","Oops, %s has been destroyed.\n"))
		, aufzaehlung);
	myfree(aufzaehlung);
    }
    
    return 0;
}


/**---------------------------------------------------------------------------
  * STADT::atomarer_beschuss()
  * 
  * Stadt wird mit Atomraketen beschossen und wird dabei vernichtet.
  ---------------------------------------------------------------------------*/
short STADT::atomarer_beschuss(OBJEKT *bombardeur)
{
  staat()->kommando("STADT_ATOMGEBOMBT", this, bombardeur);
  OBJEKT *objekt = (OBJEKT *)besitztum.first();    
  while (!objekt->is_tail())
  {
    if (objekt->typ_ist("EINHEIT")) {
      geben(staat(), objekt);
      objekt->tagebuch_fuehren(L("Unsere Heimatstadt wird von einer Atombombe vernichtet!","Our Capital City has been annihilated by a Nuclear Bomb!"), ", ");
      objekt->kommando("VERNICHTUNG");
      objekt = (OBJEKT *)besitztum.first();
    }
    else NEXT(objekt);
  }

  report(L("Wo vor einer halben Stunde noch %s stand "," A smoking, contaminated crate yawns "), a_name());
  report(L("ist nur noch ein atomar verseuchter Krater.\n"," where half an hour ago the thriving town of %s stood. \n"));

  einwohneraenderung -= einwohnerzahl;
  einwohnerzahl = 0;
  benoetigte_nahrung = 0;
  forschungszuschuss = forschungszuschuss_defacto = 0;
  arbeitskraft_einnahmen = 0;
  resourcenspeicher.setzen_auf(".");
  
  staat()->kommando("AGGRESSION",bombardeur);

  // Alle bewirtschafteten Felder sofort freigeben.
  while (!bewirtschaftete_felder.is_empty())
  {
    if (!feld_nicht_mehr_bewirtschaften(
    ((ADR_LIST_NODE *)bewirtschaftete_felder.first())->adresse)) break;
  }

  
  // Zum letzten Mal bekommt die Stadt die Umgebung aktualisiert.
  // Sonst sieht man den Krater nicht!!
  landschaft()->landschaftsabbild_aktualisieren(besitzer, adresse, 3.5);

  // Die Einrichtung gehen an eine andere Stadt oder an den Gegner.
  staat()->einrichtungen_uebergeben(this, bombardeur->staat());

  zur_vernichtung_vormerken();
  return 0;
}


/**---------------------------------------------------------------------------
  * VORHABEN::VORHABEN()           // construktor
  * 
  * Der Konstruktor der Klasse VORHABEN initialisiert ein neues Vor-
  * haben mit dem Namen des Bauwerks und einer Prioritaet, mit der ge-
  * baut werden soll. Es werden auch die Resourcen ermittelt, die
  * zur Fertigstellung benoetigt werden
  *
  * @param
  * was:            Name des Vorhabens, z.B. "Siedler", o. "Bank"
  * pri:            Prioritaet.
  * anz:            Anzahl bei Mehrfachprojekten
  ---------------------------------------------------------------------------*/
VORHABEN::VORHABEN(char *was, short pri, short anz)
{
  vorhaben = mystrdup(was);
  prioritaet = pri;
  resourcen_ermitteln();
  anzahl = anz;
}


VORHABEN::VORHABEN(FILE *file)
{
  char namefeld[MAX_LAENGE_NAME+2];
  char resfeld[MAX_LAENGE_RESOURCEZEILE+2];

  long pri, anz;
  fscanf(file, "%s%ld%ld%s",namefeld, &pri, &anz, resfeld);
  prioritaet = pri;
  anzahl = anz;
  vorhaben = mystrdup(namefeld);
  noch_benoetigte_resourcen.setzen_auf(resfeld);
}

short VORHABEN::speichern(FILE *file)
{
  fprintf(file, "%s %ld %ld %s ", vorhaben, (long)prioritaet, (long)anzahl, noch_benoetigte_resourcen.to_string());
  return (ferror(file) != 0);
}


/**---------------------------------------------------------------------------
  * VORHABEN::resourcen_ermitteln()
  * 
  * Ermittelt zu einem Vorhaben, wieviele Resourcen zu seiner Fertig-
  * stellung benoetigt werden.
  *
  * @return
  * 1, falls ein solches Vorhaben in der Enzyklopaedie nicht bekannt ist.
  * 0, falls alles OK ist.
  ---------------------------------------------------------------------------*/
short VORHABEN::resourcen_ermitteln()
{
  // Hier muss zu jedem Bauwerk, jeder Eintwicklung stehen, was fuer
  // Resourcen benoetigt werden. Dazu frage ich einfach die Enzyklopaedie

  if (!enzyklopaedie()) return 1; // Kann man leider nichts mehr machen!
  VORHABEN_ENZ *vor = enzyklopaedie()->vorhaben(vorhaben);
  if (!vor) return 1; // Nicht gefunden

  // In der Vorhabenstruktur ist ein Eintrag namens benoetigte_resourcen

  noch_benoetigte_resourcen.setzen_auf(vor->benoetigte_resourcen);
  return 0; // Alles OK.
}

/**---------------------------------------------------------------------------
  * STADT::abschlussbericht()
  * 
  * Erzeugt den Teil des Ergebnisausdruckes fuer eine Stadt. Schickt
  * die Daten gleich mittels der Funktion drucken() zum Drucker.
  * Im Abschlussbericht erscheinen alle Reports waehrend der laufenden
  * Runde sowie eine Uebersichtsgrafik ueber die naehere Umgebung der
  * Stadt und der bewirtschafteten Felder. Ausserdem wird eine Liste
  * aller laufenden Bauvorhaben ausgegeben.
  ---------------------------------------------------------------------------*/
void STADT::abschlussbericht()
{
  // Wenn die neuen Druckerfunktionen in Kraft treten, dann wird der
  // Abschlussbericht der Stadt als Layout gesetzt. Ich lasse aber
  // die alten Funktionen vorserst im Programm, damit besser getestet
  // werden kann.

  if (laser) {
    tabellarischer_teil_des_ausdrucks();
    aufzaehlung_der_bauten_ausdrucken();
    eisenbahn_protokoll_ausdrucken();
    if (!reportliste_leer()) reports_layouten_und_drucken(this);
    return;
  }

  // ASCII Printout gestrichen. Uebersetzung spar ich mir!
  
}

/**---------------------------------------------------------------------------
  * STADT::abschlussbericht_html()
  * 
  * Erzeugt ein HTML-File fuer den Abschlussbericht der Stadt.
  ---------------------------------------------------------------------------*/
void STADT::abschlussbericht_html(OBJEKT *letztestadt, OBJEKT *naechstestadt)
{
    char titel[1024];
    sprintf(titel, "%s %s (%s)", a_gattung(), a_name(), name);
    char htmlname[100];
    sprintf(htmlname, "S_%s", name); // z.B. S_HSA
    HTML html(staat(), htmlname, titel);
    if (!html.ok()) return;

    char link_links[10], link_rechts[10];
    if (letztestadt) sprintf(link_links, "S_%s", letztestadt->name);
    if (naechstestadt) sprintf(link_rechts, "S_%s", naechstestadt->name);
    html.buttonpanel(1, letztestadt ? link_links : (char *)NULL,
		     naechstestadt ? link_rechts : (char *)NULL);

    // Alles wird in eine einzige grosse Tabelle eingebettet.
    html.set_table_width("100%")
	.set_cell_spacing(0)
	.set_table_border(0)
	.table();

    // Ueberschrift
    html.set_row_color("#d0d0d0")
	.next_row()
	.next_cell()
	.font_size(2)
	.bold(name) // HSA
	.end_font()
	.set_column_span(2)
	.next_cell()
	.font_size(2)
	.bold(a_name()) // Ursu Minor Beta
	.end_font()
	.unset_all();

    // Kleine Leerzeile fuer einen Abstand
    html.next_row().set_column_span(3).set_cell_height(5).next_cell().unset_all();

    // Links Icons und Umgebungsgrafik, rechts Vermischtes
    html.next_row()
	.set_cell_valignment(VAL_CENTER)
	.set_cell_alignment(AL_CENTER)
	.next_cell();
    html_icons_und_umgebungsgrafik(html);
    
    // Vermischtes. Generiert von sich aus zwei Zellen.
    html.unset_all();
    html_vermischtes(html);

    // Guetertabelle und Projekte
    html.next_row()
	.set_column_span(2)
	.set_cell_valignment(VAL_TOP)
	.next_cell();
    html_guetertabelle(html);
    html.unset_column_span()
	.next_cell();
    html_projekte(html);

    // Die letzten Drei Abschnitte gehen ueber die ganze Breite.
    html.set_column_span(3);
    
    // Kleine Leerzeile fuer einen Abstand
    html.next_row().set_cell_height(5).next_cell().unset_cell_height();

    // Berichte
    html.next_row()
	.next_cell()
	.bold(L("Berichte: ","Reports: "));
    REPORT *report;
    SCAN(&reportliste, report)
    {
	html.text(report->text);
	NEXT(report);
    }

    // Kleine Leerzeile fuer einen Abstand
    html.next_row().set_cell_height(5).next_cell().unset_cell_height();

    // Ausbauten
    html.next_row()
	.next_cell()
	.font_color("#000080")
	.bold(L("Ausbauten: ","Buildings: "));
    DOUBLIST *objliste = alle_objekte_im_direkten_besitz("TYP=STADTAUSBAU");
    if (objliste->is_empty()) html.text(L("Die Stadt hat keine Ausbauten.","The town has still no buildings."));
    else {
	char *aufzaehlung = woertliche_objekt_aufzaehlung(objliste, NOMINATIV);
	html.text(aufzaehlung).text(".");
	myfree(aufzaehlung);
    }
    delete objliste;
    html.end_font();

    // Bahnkontor
    if (bahnkontor_protokoll || attribut_gesetzt("SV"))
    {
	// Kleine Leerzeile fuer einen Abstand
	html.next_row().set_cell_height(5).next_cell().unset_cell_height();

	html.next_row()
	    .next_cell()
	    .font_color("#800040");

	if (attribut_gesetzt("SV"))
	    html.bold(L("G~utertransport in jeder Runde: ","Commodity transport each turn: "))
		.text(attribut("SV"))
		.text(". ");
	if (bahnkontor_protokoll)
	    html.bold(L("Bahnkontor: ","Railway Depot: "))
		.text(bahnkontor_protokoll)
		.text(".");
    }

    // Kleine Leerzeile fuer einen Abstand
    html.next_row().set_cell_height(5).next_cell().unset_cell_height();

    // Einheiten
    html.next_row()
	.next_cell();
    html_einheiten(html);

    // Ende der Haupttabelle
    html.end_table();
}	
	

/**---------------------------------------------------------------------------
  * STADT::html_icons_und_umgebungsgrafik(HTML)
  * 
  * Untertabelle fuer Icons, Koordinaten und Umgebungsgrafik
  ---------------------------------------------------------------------------*/
void STADT::html_icons_und_umgebungsgrafik(HTML html)
{
    HTML iconhtml(html);
    
    iconhtml.set_cell_spacing(0)
	.set_table_width("100%")
	.table()
	.next_row()
	.set_cell_valignment(VAL_TOP)
	.set_cell_alignment(AL_CENTER)
	.next_cell();
    
    // Einzelne Icons...
    iconhtml.smalleimage(attribut("GRAFIK"));
    if (anzahl_kaempfe > 0)
	for (int i=0; i<anzahl_kaempfe; i++) iconhtml.linebreak().iconimage("saebel");
    if (zur_vernichtung_vorgemerkt()) iconhtml.linebreak().iconimage("totnkopf");
    
    // Umgebungsgrafik
    iconhtml.unset_all()
	.set_row_span(2)
	.set_cell_alignment(AL_CENTER)
	.next_cell();
    html_umgebungsgrafik(iconhtml);
    
    // Koordinaten
    RIC *ric = (RIC *)staat()->info("RELATIVE_ADRESSE", &adresse);
    iconhtml.unset_all()
	.next_row()
	.set_cell_valignment(VAL_BOTTOM)
	.set_cell_alignment(AL_CENTER)
	.next_cell()
	.bold()
	.text(ric->x)
	.text(",")
	.text(ric->y)
	.bold_off();
    
    // Untertabelle Umgebungsgrafik, Icons, Koordinaten abschliessen
    iconhtml.end_table();
}


/**---------------------------------------------------------------------------
  * STADT::html_umgebungsgrafik(HTML)
  * 
  * Die bewirtschaftbaren Felder als Grafiken in ein offenes HTML.
  ---------------------------------------------------------------------------*/
void STADT::html_umgebungsgrafik(HTML html)
{
    // Ich packe die Feldergrafiken in eine Untertabelle

    html.set_cell_spacing(0)
	.set_cell_padding(3)
	.set_table_border(0)
	.table();

    float stadtradius = 2.5 + 1 * einfluss_vorhanden("STARAD");
    // int tabellenbreite = (int)stadtradius * 2 + 1;
    DOUBLIST *feldliste = welt()->alle_adressen_im_umkreis_von(adresse, stadtradius);
  
    // Die Schleife geht Zeile fuer Zeile den moeglichen Bereich durch
    // und durchsucht fuer jede Position, ob es ein passendes Feld 
    // in der Liste der bewirtschafteten Felder gibt.

    for (int zeile = (int)stadtradius; zeile >= -(int)stadtradius; zeile--)
    {
	html.next_row();
	for (int spalte = -(int)stadtradius; spalte <= (int)stadtradius; spalte++)
	{
	    ADR testadr(adresse.x+spalte, adresse.y+zeile);
	    welt()->wrap(&testadr);
	    bool gefunden = false;
	    FOR_EACH_ADR_IN(feldliste) DO ({
		if (adr == testadr) // Feld gefunden, das genau hier ist
		{
		    gefunden = true;
		    const char *grafikname = welt()->gelaendeform_attribut(adr,"GRAFIK");
		    const char *farbe = welt()->feld_attribut(adr, FELD_ATTRIBUT_BEWIRTSCHAFTUNG) ?
			(wir_bewirtschaften_feld(adr) ? "#000000" : "#ff0000") : 0;
		    if (farbe) html.set_cell_color(farbe);
		    html.next_cell()
			.fieldimage(grafikname)
			.unset_cell_color();
		}
	    }) // feldsuche
	    if (!gefunden) html.empty_cell(); //  Zelle an den Ecken.
	} // spalten
    } // zeilen
    delete feldliste;
    html.end_table();
}


/**---------------------------------------------------------------------------
  * STADT::html_vermischtes(HTML&)
  * 
  * Erzeugt die vermischten Informationen ueber Einwohner, Wachstum...
  * Das HTML Objekt wird als Referenz uebergeben. Es wird also
  * kein neuer Kontext erzeugt!
  ---------------------------------------------------------------------------*/
void STADT::html_vermischtes(HTML& html)
{
    // Linke und rechte Seite sind jeweils eine Zelle.
    // Ich muss beide Seiten also genau gleich formatieren.

    html.set_cell_valignment(VAL_TOP)
	.next_cell()
	.font_size(2).text(a_gattung()).end_font().linebreak().linebreak()
	.text(L("Einwohnerzahl:","Population:")).linebreak()
	.text(L("Ben~otigte Nahrung:","Food needed:")).linebreak()
	.text(L("Nahrungsrationierung:","Rationing:")).linebreak()
	.text(L("~Arztliche Versorgung:","Hospital coverage:")).linebreak()
	.text(L("Feldbewirtschaftung:","Operational  Squares")).linebreak()
	.text(L("Forschungszuschu~s:","Research bonus:"));

    html.next_cell()
	.font_size(2).nonbreaking_space().end_font().linebreak().linebreak()
	.bold()
	.text(einwohnerzahl).text(" (")
	.text(einwohneraenderung > 0 ? "+" : "")
	.text(einwohneraenderung)
	.text(")").linebreak()
        .text(benoetigte_nahrung).linebreak()
	.text(attribut_gesetzt("Rationiert")
	      ? L("ja","yes")
	      : L("nein","no")).linebreak()
	.text((int)(100*gesundheitliche_versorgung()+0.499)).text("%").linebreak();
    
    char *a_sparen = attribut("A-SPAREN");
    long sparen = myatol(a_sparen);
    if (!sparen) html.text(L("Maximal","Maximum"));
    else if (a_sparen[strlen(a_sparen)-1] == L('F','S'))
	html.text(L("Maximal ","Maximum ")).text(sparen)
	    .text(L(" Felder"," Squares"));
    else html.text(L("Mindestens ","Minimum ")).text(sparen)
	     .text(L("A reserviert","M reserved"));
    html.linebreak()
	.text(forschungszuschuss_defacto).text(L(" von "," of ")).text(forschungszuschuss)
	.bold_off();

}


/**---------------------------------------------------------------------------
  * STADT::html_guetertabelle()
  * 
  * Erzeugt eine Tabelle ueber die Gueter Einnahmen und Ausgaben in
  * Form eines Unter-HTML-Files.
  ---------------------------------------------------------------------------*/
void STADT::html_guetertabelle(HTML html)
{
    // Guetertabelle
    
    RESOURCE_VEKTOR restab[6];
    restab[0].setzen_auf(res_alt);
    restab[0][L('A','M')] += arbeitskraft_einnahmen;
    restab[1].setzen_auf(res_bew);
    restab[2].setzen_auf(res_umw);
    restab[3].setzen_auf(res_ern);
    restab[3].negieren();
    restab[3].addiere(res_unt); // res_unt ist bereits negativ.
    restab[4].setzen_auf(res_be);
    restab[4].negieren();
    restab[5].setzen_auf(resourcenspeicher);
    
    html.set_cell_spacing(0)
	.set_table_color("#ffffff")
	.table();
    
    html.set_row_color("#e0e0e0")
	.next_row()
	.next_cell().bold(L("G~utertabelle","Commodities"))
	.next_cell(L("Anfang","Starting"))
	.next_cell(L("Felder","Squares"))
	.next_cell(L("Verarbeitung","Processing"))
	.next_cell(L("Versorgung","Supply"))
	.next_cell(L("Bau/Entw.","Cons./Dev."))
	.next_cell(L("Speicher","Stock"))
	.unset_row_color();
    
    RESOURCE_ENZ *res = (RESOURCE_ENZ *)g_enzyklopaedie->resourcenliste.first();
    while (!res->is_tail()) {
	html.next_row()
	    .set_cell_color("#e0e0e0")
	    .next_cell()
	    .text(res->name)
	    .unset_cell_color()
	    .set_cell_alignment(AL_RIGHT);
	for (int i=0; i<6; i++) {
	    if (restab[i][res->symbol]) html.next_cell(restab[i][res->symbol]);
	    else html.empty_cell();
	}
	html.set_cell_alignment(AL_LEFT);
	res = (RESOURCE_ENZ *)res->next();
    }
    html.end_table();
}
  

/**---------------------------------------------------------------------------
  * STADT::html_projekte()
  * 
  * Erzeugt eine Tabelle ueber alle laufenden Projekte in
  * Form eines Unter-HTML-Files.
  ---------------------------------------------------------------------------*/
void STADT::html_projekte(HTML html)
{
    // Projekttabelle
    
    html.unset_all()
	.set_cell_spacing(0)
	.set_table_color("#ffffff")
	.table();
    
    html.set_row_color("#e0e0e0")
	.next_row()
	.next_cell(L("P","P"))
	.next_cell(L("Anz.","Number"))
	.next_cell().bold(L("Projekt","Project"))
	.next_cell(L("Rest","Rem."))
	.unset_row_color();
    
    VORHABEN *vorhaben = (VORHABEN *)vorhabenliste.first();
    int noch_uebrig = MAXIMALZAHL_LAUFENDER_PROJEKTE;
    while (!vorhaben->is_tail())
    {
	noch_uebrig --;
	html.next_row();
	html.set_cell_alignment(AL_LEFT).next_cell().text(vorhaben->prioritaet);
	html.set_cell_alignment(AL_RIGHT).next_cell().text(vorhaben->anzahl);
	html.set_cell_alignment(AL_LEFT).next_cell().text(vorhaben->vorhaben);
	
	// Bei Entwicklungen weiss der Staat, wieviel noch noetig ist.
	char *resstring = staat()->attribut(vorhaben->vorhaben);
	if (!resstring) resstring = vorhaben->noch_benoetigte_resourcen.to_string();
	html.set_cell_alignment(AL_RIGHT);
	html.next_cell()
	    .text(resstring);
	
	vorhaben = (VORHABEN *)vorhaben->next();
    }
    
    // Noch freie Zeilen trotzdem auch aufnehmen
    while (noch_uebrig--) {
	html.next_row()
	    .empty_cell()
	    .empty_cell()
	    .empty_cell()
	    .empty_cell();
    }    
    html.end_table();
}


/**---------------------------------------------------------------------------
  * STADT::html_einheiten()
  * 
  * Erzeugt eine Uebersicht ueber alle Einheiten, die der Stadt
  * unterstehen in Form eines Unter-HTML-Files.
  ---------------------------------------------------------------------------*/
void STADT::html_einheiten(HTML html)
{
    html.bold(L("Unterstellte Einheiten","Assigned Units:"))
	.unset_all()
	.set_cell_alignment(AL_CENTER)
	.table()
	.set_image_border(0);
  
    int umbruchzaehler = 0;
    char grafik[MAX_LAENGE_DATEINAME];
    
    FOR_EACH_OBJEKT_IN (alle_einheiten()) // Keine Toten Einheiten!
	DO_AND_DELETE
	({
	    if (umbruchzaehler++ % 10 == 0) html.next_row();
	    html.next_cell();
	    html.href_objekt(objekt);
	    sprintf(grafik,"e%s",objekt->attribut("GRAFIK"));
	    html.smallimage(grafik);
	    html.linebreak();
	    html.courier();  
	    html.text(objekt->name);  
	    html.courier_off();
	    html.end_href();
	    html.linebreak();
	    
	    char *versorgung = objekt->attribut("VERSORGUNG");
	    if (mystrlen(versorgung) < 2) html.text("-");
	    else html.text(versorgung);
	})
    html.end_table();
}


/**---------------------------------------------------------------------------
  * STADT::tabellarischer_teil_des_ausdrucks()
  ---------------------------------------------------------------------------*/
void STADT::tabellarischer_teil_des_ausdrucks()
{

  // Das genaue Aussehen kann ich hier nur anreissen: Links steht eine
  // Tabelle mit den Einnahmen und Ausgaben an Resourcen. Dabei stehen
  // die verschiedenen Resourcetypen untereinander.
  // Rechts kommt eine kleine Uebersichtsgrafik mit der Umgebung der Stadt
  // wo die bewirtschafteten Felder markiert sind. Dort steht auch Name
  // und Abkuerzung der Stadt.
  // In der Mitte sind die restlichen parameterisierbaren Daten wie
  // Bevoelkerung, -szuwachs, Bau- und Entwicklungsprojekte.
  // In einem weiteren Bereich ganz darunter stehen alle Reports.
  // Eine Uebersicht ueber alle Einrichtungen der Stadt koennte auch noch
  // folgen, aber wahrscheinlich nur in bestimmten Zeitabstaenden ( jeder
  // fuenfte Zug oder so aehnlich ).

  const float rechterrand = 19.80;
  long anzahl_resourcen = g_enzyklopaedie->anzahl_verschiedener_resourcen;
  const float xraster1 = 1.1667; // Raster1 ist fuer die Tabelle links
  const float yraster1 = 0.4;
  const long anzahl_spalten1 = 6;
  const float tabellenende = (anzahl_resourcen + 1) * yraster1;
  const float tabellenrand = (xraster1 * anzahl_spalten1) + 1.9;
  const float tab2links = tabellenrand + 0.05;
  const float tab2rechts = tab2links + 3.1;
  const float tab2strich1 = tab2links + 0.25 ;
  const float tab2strich2 = tab2links + 0.5 ;
  const float tab2strich3 = tab2links + 2.2 ;
  const float tab2ende = tabellenende;
  const float tortex = tab2rechts + 1.45;
  // const float tortey = 1.4;
  const float torterechts = tortex + 1.45;
  const float abklinks = rechterrand - 0.9;
  const float abkunten = 0.45;

  // Positionen im Bereich mit den kleinen Texten: Vier links und vier rechts
  #define POS_1  tab2rechts+0.1, tabellenende-1.72
  #define POS_2  tab2rechts+0.1, tabellenende-1.45
  #define POS_3  tab2rechts+0.1, tabellenende-1.18
  #define POS_4  tab2rechts+0.1, tabellenende-0.91
  #define POS_5  tab2rechts+0.1, tabellenende-0.64
  #define POS_6  tab2rechts+0.1, tabellenende-0.37
  #define POS_7  tab2rechts+0.1, tabellenende-0.1

#define KLEINE_SCHRIFT_PROP "Times",7
#define KLEINE_ZAHLEN "Courier",7.1
#define NAME_SCHRIFT "Times",10
#define ABK_SCHRIFT "Courier",11.5

  dr_abschnitt(0.2); // Kleinen Abstand lassen.
  LAYOUT *ly = new LAYOUT(dr_abschnitt(tabellenende));

  // Umrandung des Tabellenbereiches.

  ly->linie(0,0,rechterrand,0); // Nach oben begrenzen
  ly->linie(0, tabellenende, rechterrand, tabellenende); // Nach unten begr.
  ly->linie(0,0,0,tabellenende); // links
  ly->linie(rechterrand,0,rechterrand,tabellenende); // rechts

  // Als erstes baue ich die Tabelle auf der linken Seite auf. Sie
  // beschreibt die Bilanzen der Resourcen Einnahmen und Ausgaben.

  int i;
  for (i=1; i<=anzahl_resourcen; i++)
     ly->linie(0, yraster1*i, xraster1*anzahl_spalten1 + 1.9, yraster1*i);

  for (i=0; i<anzahl_spalten1; i++)
     ly->linie(i*xraster1+1.9, 0, i*xraster1+1.9, tabellenende);

  ly->linie(tabellenrand, 0, tabellenrand, tabellenende);
  ly->linie(1.85,0,1.85,tabellenende);

  // Jetzt kommt die konstante Beschriftung.
  RESOURCE_ENZ *res = (RESOURCE_ENZ *)g_enzyklopaedie->resourcenliste.first();
  for (i=0; !res->is_tail() && i<anzahl_resourcen; i++)
  {
    ly->text(0.1, (i+2)*yraster1-0.1, res->name, KLEINE_SCHRIFT_PROP);
    res = (RESOURCE_ENZ *)res->next();
  }

  char *titel[] = {L("  Anfang"," Starting at"),
		   L("  Felder","  Squares"),
		   L("Verarbeit.","Processing"),
		   L(" Versorg."," Supply"),
		   L("Bau/Entw.","Constr./Res."),
		   L(" Speicher"," Stock")};
  for (i=0; i<anzahl_spalten1; i++)
    ly->text( i*xraster1 + 1.95, yraster1 - 0.1, titel[i], KLEINE_SCHRIFT_PROP);

  // Hier kommen dann die tatsaechlichen Eintraege der Resourcen.
  // Ich lege eine kleine Tabelle der Resourcen in den verschie-
  // denen Positionen an, damit ich eine schoene Schleife machen
  // kann..

  RESOURCE_VEKTOR restab[6];
  restab[0].setzen_auf(res_alt);
  restab[0][L('A','M')] += arbeitskraft_einnahmen;
  restab[1].setzen_auf(res_bew);
  restab[2].setzen_auf(res_umw);
  restab[3].setzen_auf(res_ern);
  restab[3].negieren();
  restab[3].addiere(res_unt); // res_unt ist bereits negativ.
  restab[4].setzen_auf(res_be);
  restab[4].negieren();
  restab[5].setzen_auf(resourcenspeicher);

  char rechtsbund[90];
  for (i=0; i<6; i++) {
    RESOURCE_ENZ *res = (RESOURCE_ENZ *)g_enzyklopaedie->resourcenliste.first();
    for (int j=0; !res->is_tail() && j<anzahl_resourcen; j++)
    {
      long wert;
      if (0 != (wert = restab[i][res->symbol])) { // Nur Drucken, wenn != 0
	sprintf(rechtsbund,"%7ld",wert);
	ly->text(i*xraster1+2.0, (j+2)*yraster1-0.1, rechtsbund, KLEINE_ZAHLEN);
      }
      res = (RESOURCE_ENZ *)res->next();
    }
  }

  // Tabelle der laufenden Entwicklungen

  ly->linie(tab2links, 0, tab2links, tab2ende);
  ly->linie(tab2rechts, 0, tab2rechts, tab2ende);
  ly->linie(tab2rechts+0.05, 0, tab2rechts+0.05, tabellenende);
  ly->linie(tab2strich1, 0, tab2strich1, tab2ende);
  ly->linie(tab2strich2, 0, tab2strich2, tab2ende);
  ly->linie(tab2strich3, 0, tab2strich3, tab2ende);

  for (i=1; i <= MAXIMALZAHL_LAUFENDER_PROJEKTE; i++)
     ly->linie(tab2links, yraster1*i, tab2rechts, yraster1*i);

  ly->text(tab2links+0.03, yraster1-0.1, L("P.","P."), KLEINE_SCHRIFT_PROP);
  ly->text(tab2links+0.28, yraster1-0.1, L("A.","N."), KLEINE_SCHRIFT_PROP);
  ly->text(tab2strich2+0.1, yraster1-0.1, L("Projekt","Project"), KLEINE_SCHRIFT_PROP);
  ly->text(tab2strich3+0.2, yraster1-0.1, L("Rest","Rem."), KLEINE_SCHRIFT_PROP);

  // Jetzt noch die Daten eintragen. Die Liste der Vorhaben ist in Richtung
  // fallender Prioritaeten sortiert.

  VORHABEN *vorhaben = (VORHABEN *)vorhabenliste.first();
  float oben = 2*yraster1 - 0.1;
  short zaehler = MAXIMALZAHL_LAUFENDER_PROJEKTE;
  while (!vorhaben->is_tail() && zaehler) {

    // Als noch benoetigte Resourcen gebe ich den Resourcestring aus.
    // Wenn es sich um eine Entwicklung handelt, dann ist im Staat ein
    // Attribut unter dem Namen des Vorhabens mit den restlichen Ressourcen
    // angelegt, der statt dem Eintrag im Vorhaben gilt.

    char *resstring = staat()->attribut(vorhaben->vorhaben);
    if (resstring) sprintf(rechtsbund,"%6s",resstring);
    else sprintf(rechtsbund,"%6s",vorhaben->noch_benoetigte_resourcen.to_string());
    
    ly->text(tab2strich3, oben, rechtsbund, KLEINE_ZAHLEN);
    ly->text(tab2links+0.055, oben, myltoa(vorhaben->prioritaet), KLEINE_ZAHLEN);
    ly->text(tab2links+0.305, oben, myltoa(vorhaben->anzahl), KLEINE_ZAHLEN);
   
    strcpy(rechtsbund, vorhaben->vorhaben);
    if (mystrlen(rechtsbund) > 13) sprintf(rechtsbund+11, "...");
    ly->text(tab2strich1+0.35,oben, rechtsbund, KLEINE_SCHRIFT_PROP);

    oben += yraster1;
    zaehler--;
    vorhaben = (VORHABEN *)vorhaben->next();
  }

  // Im Bereich, wo frueher die Tortengrafik war, werden jetzt Symbole
  // fuer die Einheiten dargestellt, die der Stadt unterstehen. 
  
  zaehler = 0;
  FOR_EACH_OBJEKT_IN (alle_einheiten()) // Keine Toten Einheiten
  DO_AND_DELETE
  ({
    const float xabstand = 0.28; // cm
    const float yabstand = 0.15; // cm
    const short xmodulus = 4; // Einheiten pro Zeile
    const float links = tab2rechts + 0.26;
    const float oben = 0.1; // cm
    const short maxzahl = 15; // Soviel passen gerade noch drauf
    
    if (zaehler < maxzahl) {
      char grafik[16];
      sprintf(grafik,"e%s.gra",objekt->attribut("GRAFIK"));
      const float l = links + (zaehler % xmodulus) * (xabstand+0.4);
      const float o = oben  + (zaehler / xmodulus) * (yabstand+0.4);
      ly->bitmatrix(l, o, grafik);

      char *text = objekt->name;
      // = attribut("VERSORGUNG"); if (mystrlen(text) < 2) text = "-";
      ly->text(l + 0.2 - strlen(text)*0.05, o+0.525, text, "Courier",5);
    }
    else if (zaehler == maxzahl)
      ly->text(tab2rechts+2.15, 2.05, L("(mehr)","(more)"), "Courier", 5);
     
    zaehler++;
  })


  ly->linie(torterechts, 0, torterechts, tabellenende);
  ly->linie(torterechts+0.05, 0, torterechts+0.05, tabellenende);
  ly->linie(tab2rechts+0.05, yraster1*6, torterechts, yraster1*6);

  // Am Unteren Rand der Torte steht noch die Einwohnerzahl und der Zuwachs.

  long ez = einwohnerzahl, ea = einwohneraenderung;

  sprintf(rechtsbund,L("%ld Einw. (%c%ld)","%ld Inhabitants. (%c%ld)"),ez, (ea < 0) ? '-' : '+', ABS(ea));
  ly->text(POS_1, rechtsbund, KLEINE_SCHRIFT_PROP);

  // Auf der linken Seite, unter der Projektetabelle, stehen auch noch
  // verschiedene Angaben. Wenn die Stadt aufgeloest wurde, dann faellt
  // dieser Teil ganz weg.
  
  if (einwohnerzahl) {

    sprintf(rechtsbund,L("Ben~otigte Nahrung: %ld","Food needed: %ld"), benoetigte_nahrung);
    ly->text(POS_2, rechtsbund, KLEINE_SCHRIFT_PROP);

    if (attribut_gesetzt("Rationiert")) {
      ly->text(POS_3, L("Nahrung ist rationiert","Food is rationalized"), KLEINE_SCHRIFT_PROP);
    }

    DOUBLIST *einheitenliste = alle_einheiten();
    long anz_einh = einheitenliste->number_of_elements();
    delete einheitenliste;
    sprintf(rechtsbund,L("Unterstellte Einheiten: %ld","Assigned units: %ld"), anz_einh);
    ly->text(POS_5, rechtsbund, KLEINE_SCHRIFT_PROP);

    char *a_sparen = attribut("A-SPAREN");
    long sparen = myatol(a_sparen);
    if (!sparen) sprintf(rechtsbund,L("Maxim. Feldbewirtschaftung","Maximum Square Operation"));
    else if (a_sparen[strlen(a_sparen)-1] == L('F','S'))
      sprintf(rechtsbund, L("Bewirtschaftung: %ld Felder","In Operation: %ld Squares"), sparen);
    else sprintf(rechtsbund,L("Mindestens reserviert: %ldA","Reserved minimum: %ldM"), sparen);
    ly->text(POS_6, rechtsbund, KLEINE_SCHRIFT_PROP);

    sprintf(rechtsbund,L("F-Zuschuss: %ld von %ld","R-Bonus: %ld of  %ld"),
      forschungszuschuss_defacto, forschungszuschuss);
    ly->text(POS_7, rechtsbund, KLEINE_SCHRIFT_PROP);

    sprintf(rechtsbund,L("~Arztliche Versorgung: %2.0f%%","Hospital Coverage: %2.0f%%"),100 * gesundheitliche_versorgung());
    ly->text(POS_4, rechtsbund, KLEINE_SCHRIFT_PROP);

  } // einwohnerzahl != 0

  // Hier kommt der rechte Bereich mit der Uebersichtsgrafik
  // Oben steht der Name der Stadt und ihre Abkuerzung. Links vom Namen
  // steht die Gattung.

  sprintf(rechtsbund,"%s %s",a_gattung(),a_name());
  ly->text(torterechts+0.3, 0.35, rechtsbund, NAME_SCHRIFT);
  ly->linie(abklinks,abkunten,rechterrand,abkunten);
  ly->linie(abklinks,0,abklinks,abkunten);
  ly->text(abklinks+0.05,abkunten-0.13,name,ABK_SCHRIFT);

  // Und nun: Jetzt kommt noch die Uebersichtsgrafik der von der Stadt
  // erreichbaren Felder. Dazu bestimme ich die Felder zuerst.
  // Und bei jedem Feld, das eine andere Stadt bewirtschaftet gebe ich die
  // Grafik gesperrt.gra aus.

  const float mittex = 17.21;
  const float mittey = 2.17;
  const float feldbreite = 0.51;

  if (welt()) // Zur Vorsicht
  {
   
    DOUBLIST *feldliste = welt()->alle_adressen_im_umkreis_von(
      adresse, (2.5 + 1 * einfluss_vorhanden("STARAD")));
       
    while (!feldliste->is_empty()) {
      // Jedes Feld muss an den richtigen Platz
      ADR adr = ((ADR_LIST_NODE *)feldliste->first())->adresse;
      RIC ric = welt()->richtung_von_nach(adresse, adr);
      float links = ric.x*feldbreite + mittex;
      float oben = (-ric.y)*feldbreite + mittey;
      LAYOUT *feldly = welt()->feld_layout(links, ly->oben() + oben, adr);

      // Bewirtschaftete Felder (allgemein) kennzeichnen.
      if (welt()->feld_attribut(adr, FELD_ATTRIBUT_BEWIRTSCHAFTUNG))
	feldly->bitmatrix(links-0.05, ly->oben() + oben-0.04, "gesperrt.gra");

      feldly->ausdrucken();
      delete feldly;
      delete feldliste->first();
    }
    delete feldliste;
  }

  // Und jetzt markiere ich noch alle bewirtschafteten Felder. Dazu gebe ich
  // bei jedem bewirtschafteten Feld noch die Grafik fabewirt.gra aus.

  ADR_LIST_NODE *adrnode = (ADR_LIST_NODE *)bewirtschaftete_felder.first();
  while (!adrnode->is_tail()) {
    RIC ric = welt()->richtung_von_nach(adresse, adrnode->adresse);
    float links = ric.x*feldbreite + mittex - 0.05;
    float oben = (-ric.y)*feldbreite + mittey - 0.04;
    ly->bitmatrix(links, oben, "fabewirt.gra");
    adrnode = (ADR_LIST_NODE *)adrnode->next();
  }

  // Die (relative) Adresse der Stadt traege ich rechts unten ins Eck ein.
  RIC *ric = (RIC *)staat()->info("RELATIVE_ADRESSE", &adresse);
  sprintf(rechtsbund,"%ld,%ld",ric->x, ric->y);
  ly->text(rechterrand-strlen(rechtsbund)*0.15-0.1, tabellenende-0.1,
						 rechtsbund, KLEINE_ZAHLEN);

  // Das war's fuers erste...
  ly->ausdrucken();
  delete ly;

}

/**---------------------------------------------------------------------------
  * STADT::eisenbahn_protokoll_ausdrucken()
  * 
  * Erzeugt den Teil des Printout, wo Angaben ueber den stehenden Befehl
  * SV und ueber die Aktivitaeten des Bahnkontor gemacht werden.
  ---------------------------------------------------------------------------*/
void STADT::eisenbahn_protokoll_ausdrucken()
{
  if (bahnkontor_protokoll != NULL || attribut_gesetzt("SV")) 
  {
    staat()->trennlinie();

    FILE *file = fopen(tempfile_name(),"w");
    if (!file) {
	log('W', "Can't create tempfile %s. No railway info for city %s",
	    tempfile_name(), name);
	return; // Kein Ausdruck moeglich.
    }
 
    if (attribut_gesetzt("SV"))
      fprintf(file,L("G~utertransport in jeder Runde: %s.\n","Commodity transport per turn: %s.\n"), attribut("SV"));
    if (bahnkontor_protokoll) {
      fprintf(file, L("Bahnkontor: %s.\n","Railway Depot: %s.\n"), bahnkontor_protokoll);
    }
  
    // So. Jetzt muss der ganze Schmarrn noch zum Drucker geschickt werden...
    fclose(file);
    staat()->formatieren_und_ausdrucken(tempfile_name(), 116);
  }
}  

/**---------------------------------------------------------------------------
  * STADT::aufzaehlung_der_bauten_ausdrucken()
  ---------------------------------------------------------------------------*/
void STADT::aufzaehlung_der_bauten_ausdrucken()
{
  // Alle Ausgaben gehe zuerst in ein temp-file.
  FILE *temp = fopen(tempfile_name(),"w");
  if (!temp) return; // Huch?

  ausbauten_in_file(temp); // Macht die eigentliche Arbeit
  
  fclose(temp);

  // Jetzt das ganze noch layouten und drucken...

  char *druckercode = dr_infodatei_formatieren(tempfile_name(), 116);
  if (!druckercode) return;

  if (!laser) drucken(druckercode);
  else {
    long anzahl_zeilen = dr_anzahl_zeilen(druckercode);
    LAYOUT ly(dr_abschnitt(anzahl_zeilen * 0.35 + 0.05));
    dr_zeilenabstand(0.35); // Erst hier. Bei neuer Seite wirds zurueckges.
    ly.text(0, 0.275, druckercode, "Courier", 8.063);
    ly.ausdrucken();
  }
  myfree(druckercode);
}


/**---------------------------------------------------------------------------
  * STADT::ausbauten_in_file()
  * 
  * Erzeugt eine Aufzaehlung aller Stadtausbauten (mit bestimmten
  * Attributen) und fprintfed sie in eine Datei.
  * @param
  * FILE *temp:     Ausgabedatei (fopen(...,"w"); )
  * char *bedinung: Attributliste oder NULL
  ---------------------------------------------------------------------------*/
void STADT::ausbauten_in_file(FILE *temp, char *bedingung)
{
  // Ich will eine Liste, die ordentlich sortiert ist. Alle Bauten sollen
  // alphabetisch sortiert sein und gleiche Bauten sollen zusammengefasst
  // werden und mit einer Nummer versehen. Dazu gibt es eine allgemeine
  // Funktion.

  // Falls einwohnerzahl==0, dann handelt es sich um eine Ruinenstadt,
  // und ich gebe leicht variierte Texte aus.

  char *bed;
  if (bedingung) {
    bed = mystrdup(bedingung);
    mystradd(bed, ",TYP=STADTAUSBAU");
  }
  else bed = mystrdup("TYP=STADTAUSBAU");

  DOUBLIST *objliste = alle_objekte_im_direkten_besitz(bed);
  myfree(bed);

  if (objliste->is_empty()) { // Trivialer Fall
    if (einwohnerzahl)
      fprintf(temp, L("Die Stadt besitzt keine Ausbauten.","The city  has no buildings of any importance."));
    else {
      fprintf(temp, L("Vor dem Ende von %s gab es dort","Before the fall of %s there were"), a_name());
      fprintf(temp, L(" keine nennenswerten Einrichtungen."," no important building."));
    }
  }

  else  { // Schoene Ausgabe erzeugen.
    char *aufzaehlung;
    if (einwohnerzahl) {
      fprintf(temp, L("%s besitzt ","%s has "),a_name());
      aufzaehlung = woertliche_objekt_aufzaehlung(objliste, AKKUSATIV);
    }
    else {
      fprintf(temp, L("Vor ihrem Ende besa~s %s "," Before the demise of %s "), a_name()); // aufgeloest.
      aufzaehlung = woertliche_objekt_aufzaehlung(objliste, AKKUSATIV);
    }

    fprintf(temp, "%s.\n", aufzaehlung);
    myfree(aufzaehlung);
  }
  delete objliste;

}


/**---------------------------------------------------------------------------
  * STADT::sortfunction_stadt()
  * 
  * Wird vom Staat aufgerufen, um sein Besitztum zur ordnen. Eigentlich
  * sollen aber nur die Staedte nach ihrer Groesse sortiert werden.
  * Alles andere soll unbeachtet gelassen werden. Die Staedte werden
  * aber auf jeden Fall an ein Ende der Liste zusammengefasst und
  * dort sortiert.
  ---------------------------------------------------------------------------*/
short STADT::sortfunction_stadt(DOUBLIST_NODE *s1,
					DOUBLIST_NODE *s2, void *)
{
  OBJEKT *objekt1 = (OBJEKT *)s1,
  	 *objekt2 = (OBJEKT *)s2;

  if (objekt1->typ_ist("STADT")) {
    if (!objekt2->typ_ist("STADT")) return 1;
  }
  else if (objekt2->typ_ist("STADT")) return -1;
  else return 0; // Beides keine Stadt. Egal welche Reihenfolge.
       
  STADT *stadt1 = (STADT *)objekt1,
  	*stadt2 = (STADT *)objekt2;

  if (stadt1->einwohnerzahl < stadt2->einwohnerzahl) return 1;
  else if (stadt1->einwohnerzahl > stadt2->einwohnerzahl) return -1;
  else return strcmp(stadt1->a_gattung(),stadt2->a_gattung()); // selten!
}


/**---------------------------------------------------------------------------
  * STADT::sortfunction_stadt_nodelist()
  * 
  * Sortiert ebenfalls eine Liste von Staedte, aber diesmal eine Liste
  * mit Verweisen auf Objekte (OBJEKT_LIST_NODEs).
  ---------------------------------------------------------------------------*/
short STADT::sortfunction_stadt_nodelist(DOUBLIST_NODE *s1,
					 DOUBLIST_NODE *s2, void *)
{
    OBJEKT_LIST_NODE *oln1 = (OBJEKT_LIST_NODE *)s1;
    OBJEKT_LIST_NODE *oln2 = (OBJEKT_LIST_NODE *)s2;
    
    return sortfunction_stadt(oln1->objekt, oln2->objekt, 0);
}



/**---------------------------------------------------------------------------
  * STADT::anzahl_an_stadtausbauten_pruefen()
  * 
  * Prueft, wieviele Ausbauten eines Typs die Stadt schon besitzt.
  * Ausserdem wird festgestellt, ob das Bauen einer weiteren Ausbaut
  * der gleichen Gattung noch erlaubt ist. Die Funktion kann auch
  * fuer Einheiten verwendet werden. Jede Stadt darf z.B. nur eine
  * Buergerwehr besitzen.
  *
  * @param
  * char *bauwerk:  Name der Gattung.
  *
  * @return
  * long:   Vom Betrag her Anzahl der bestehenden Ausbauten der Gattung.
  * Negativ, wenn die maximale Grenze bereits erreicht ist.
  ---------------------------------------------------------------------------*/
long STADT::anzahl_an_stadtausbauten_pruefen(char *bauwerk)
{
  VORHABEN_ENZ *vorhaben = enzyklopaedie()->vorhaben(bauwerk);
  if (!vorhaben || vorhaben->max_anzahl_in_einer_stadt == 0) return 0;

  char bed[100];
  sprintf(bed,"GATTUNG=%s",bauwerk);
  DOUBLIST *objliste = alle_objekte_im_direkten_besitz(bed); // SCHLECHT PROGRAMMIERT!
  long anzahl = objliste->number_of_elements();
  delete objliste;

  if (anzahl >= vorhaben->max_anzahl_in_einer_stadt) return -anzahl;
  else return anzahl;
}

bool STADT::ausnahmezustand()
{
    return myatol(attribut("AUSNAHMEZUSTAND")) > 0;
}
