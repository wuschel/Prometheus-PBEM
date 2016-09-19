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


// -*- c++ -*-
// **************************************************************************
// MODUL:               stadt.h  /  STADT.H
// AUTOR/DATUM:         Mathias Kettner, 20. Juli 1993
// KOMPATIBILIAET:      C++
// **************************************************************************
//
//      Deklarationen der Klassen VORHABEN und STADT, die zusammen
//      das Objekt "Stadt" beschreiben.
//
// **************************************************************************

#ifndef __stadt_h
#define __stadt_h

#define MAXIMALZAHL_LAUFENDER_PROJEKTE 10

class LAYOUT;

#include "welt.h"
#include "stdio.h"
#include "staat.h"
#include "html.h"

class WELTBAUT;

// Vorhaben der Stadt

struct VORHABEN : public DOUBLIST_NODE
{
  char *vorhaben;  // NULL: Vorhaben ungueltig
  short prioritaet;
  short anzahl; // Anzahl Bei mehreren eingelasteten Vorhaben gleicher Art.
  RESOURCE_VEKTOR noch_benoetigte_resourcen;
public:
  VORHABEN(char *, short, short a=0);
  VORHABEN(FILE *); // Vorhaben von Disk laden
  ~VORHABEN() { myfree(vorhaben); };      // String freigeben...
  short speichern(FILE *);
  short resourcen_ermitteln();
};



class STADT : public OBJEKT
{
    long vorige_phase;  // Vermeidet doppelte und fehlende Phasen bei Einnahmen von Staedten

    long einwohnerzahl;            // Einwohnerzahl der Stadt
    RESOURCE_VEKTOR resourcenspeicher; // Was die Stadt so an Rohstoffen lagert
    DOUBLIST vorhabenliste; // Was die Stadt so baut und entwickelt
    DOUBLIST bewirtschaftete_felder; // Welche Felder werden bearbeitet
    
    // Daten, die fuer den Ergebnisausdruck gespeichert werden:
    RESOURCE_VEKTOR res_alt; // Uebertrag vom letzten Zug
    RESOURCE_VEKTOR res_abk; // Arbeitskraft
    RESOURCE_VEKTOR res_bew; // Bewirtschaftung
    RESOURCE_VEKTOR res_umw; // Umwandelung
    RESOURCE_VEKTOR res_ern; // Ernaehrung
    RESOURCE_VEKTOR res_be;  // Bau und Entwicklung
    RESOURCE_VEKTOR res_unt; // Unterhalt
    RESOURCE_VEKTOR res_kap; // Speicherkapazitaet
    long einwohneraenderung;
    long benoetigte_nahrung;
    long arbeitskraft_einnahmen;
    long forschungszuschuss, forschungszuschuss_defacto; // Genuetzter Zuschuss
    
    char *bahnkontor_protokoll;
    int anzahl_kaempfe; // sooft wurde die Stadt die Runde beschossen. Fuer den Ausdruck.
  
public:
  STADT(char *, char *);
  ~STADT();

private:
    WELT *landschaft() { return (WELT *)ort(); };
    WELT *welt() { return (WELT *)ort(); };
    STAAT *staat() { return (STAAT *)besitzer; };
    short bin_hauptstadt() { return !mystrcmp_no_case(name, staat()->attribut("HAUPTS_ABK")); };
    short spieler_ausgestiegen() { return besitzer->attribut_gesetzt("Ausgestiegen"); };
    char *stadtausbau_geht_kaputt();
    OBJEKT *irgendeine_stadtausbaut();
    void arbeitskraft_berechnen();
    void resourcen_einnahmen_verrechnen();
    void kontrolliere_bewirtschaftung();
    void resourcen_umwandlungen(); // Einfluss UM
    void einwohner_mit_nahrung_versorgen();
    void einwohner_gesundheitlich_versorgen();
    float gesundheitliche_versorgung();
    void restliche_resourcen_speichern();
    void weiterbauen();
    void milizen_bauen();
    int milizen_staerke(); // Ermitteln fuer den Untersuche-Befehl
    void vorhaben_aufraeumen();
    OBJEKT *fertigstellung_von(VORHABEN *);
    void gattung_berechnen();
    short stadt_ist_belagert();
    
    short es_gibt_eisenbahn_zu(OBJEKT *);
    static short entfernungsfunktion(void *, ADR&, ADR&);
    void gueter_verschicken(char *);
    void gueterverkehr_abwickeln();
    
  // Bahnkontore abwickeln
    short mit_der_bahn_erreichbar(STADT *);
#define ALLES_ODER_NICHTS 1
#define MOEGLICHST_VIEL 2
    void besorge_ueber_bahnkontor(RESOURCE_VEKTOR&, short, RESOURCE_VEKTOR&);
    void frage_an_bahnkontor(RESOURCE_VEKTOR&);
    void bestellung_an_bahnkontor(RESOURCE_VEKTOR&, STADT *);
    
    short befehl_gueter_verschicken(char *);
    short befehl_nahrung_rationieren();
    short befehl_mehr_einwohner(char *); // Fakebefehl *E!
    short befehl_mehr_resourcen(char *); // Fakebefehl *R!
    short befehl_bauen(char *);
    short befehl_bau_abbrechen(char *);
    short befehl_abreissen(char *);
    short befehl_feld_bewirtschaften(char *);
    short befehl_feld_nicht_mehr_bewirtschaften(char *);
    short befehl_aufloesen(char *);
    short befehl_name(char *);
    short befehl_bewirtschaftung_einschraenken(char *);
    short befehl_abkuerzung_verwenden(char *);
    
    void einheiten_ueberstellen(OBJEKT *);
    void einheiten_informieren(char *);
    short feld_nicht_mehr_bewirtschaften(ADR& adr=ADR::ausserhalb());
    short guenstiges_feld_bewirtschaften();
    short wir_bewirtschaften_feld(ADR&);
    RESOURCE_VEKTOR& resourcen_auf_feld(ADR&);
    short feld_ist_noch_frei(ADR&);
    short feld_bewirtschaftbar(ADR&);

    short kommando_resourcen_anfordern(void *, short protokoll=1);
    short kommando_beschossen_werden(void *, void *);
    short atomarer_beschuss(OBJEKT *);
    short kommando_abwehrtest(void *);
    short kommando_resourcen_bekommen(void *, void *);
    short kommando_neue_einwohner(void *);
    short kommando_umwandlung(void *);
    short kommando_eingenommen(void *, void *);
    short kommando_ausstieg();
    short kommando_eisenbahn_bringt(void *, void *, void *);
    short kommando_weltbaut_uebernehmen(void *);
    
    char *info_resourcen_anfordern(void *, void *, void *);
    char *info_einwohnerzahl();
    char *info_bau_und_entwicklung();
    char *info_untersuche(void *);
    
    long anzahl_an_stadtausbauten_pruefen(char *);

    void html_icons_und_umgebungsgrafik(HTML);
    void html_umgebungsgrafik(HTML);
    void html_vermischtes(HTML&); // Achtung! hier HTML als Referenz!
    void html_guetertabelle(HTML);
    void html_projekte(HTML);
//    void html_bahnkontor(HTML); 
    void html_einheiten(HTML);
  
    void abschlussbericht_layouten();
    void aufzaehlung_der_bauten_ausdrucken();
    void ausbauten_in_file(FILE *, char *bed = NULL);
    void tabellarischer_teil_des_ausdrucks();
    void eisenbahn_protokoll_ausdrucken();

public:
    static short sortfunction_stadt(DOUBLIST_NODE *,
				    DOUBLIST_NODE *, void *);
    static short sortfunction_stadt_nodelist(DOUBLIST_NODE *s1,
					     DOUBLIST_NODE *s2, void *);
    void naechster_zug(long);
    void naechste_phase(long);
    void zug_abschliessen(long);
    
    void abschlussbericht();
    void abschlussbericht_html(OBJEKT *, OBJEKT *);
    short laden(FILE *);
    short speichern(FILE *);
    short befehl_auswerten(char *, long);
    short kommando(const char *, const void *par1=NULL, const void *par2=NULL, const void *par3=NULL);
    char *info(char *, void *par1=NULL, void *par2=NULL, void *par3=NULL);
    DOUBLIST *alle_einheiten(bool auch_tote=false);
    DOUBLIST *alle_stadtausbauten();
    void einrichtung_uebernehmen(STADT *, WELTBAUT *);
    bool ausnahmezustand();
};


#endif // __stadt_h

