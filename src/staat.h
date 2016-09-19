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
// MODUL:               staat.h / STAAT.H
// AUTOR/DATUM:         Mathias Kettner, 14. Mai 1993
// KOMPATIBILITAET:     C++
// **************************************************************************
//
//      Deklariert die Klasse STAAT.
//
// **************************************************************************

#ifndef __staat_h
#define __staat_h

#include "welt.h"
#include "mystring.h"
#include "html.h"

class VORHABEN_ENZ;
class STADT;
class EINHEIT;

/**---------------------------------------------------------------------------
  * KLASSE:              STAAT
  * ABGELEITET VON:      OBJEKT
  * 
  * Jeder Spieler spielt genau einen Staat. Der Staat besitzt Staedte.
  * Die Staedte besitzen Einheiten, Stadtausbauten, Weltwunder. Auch
  * ein Staat kann Weltwunder besitzen. Ob ein Staat auch Einheiten
  * besitzen kann, ist noch nicht geklaert.
  *
  * Als Option fuer eine zukuenftige Version des Programms koennte ein
  * Staat auch von einem ROBOT-Player gefuehrt werden. Unter Umstaenden
  * sind die Funktionen des Robotplayers auch in diesem Objekt selbst
  * integriert.
  *
  * Die technischen Entwicklungen und Erfindungen eines Staates sind in
  * den Attributen des Staates gespeichert, nicht bei den Staedten (Ob-
  * wohl die Staedte die Entwicklungen bewirken).
  *
  * BASISFUNKTIONEN:
  * auf_vernichtung_vorbereiten(), initialisieren(), naechste_phase(),
  * laden(), speichern(), befehl_auswerten().
  *
  ---------------------------------------------------------------------------*/
class STAAT : public OBJEKT
{
  ATTRIBUT_LISTE bekannte_infos;  // Welche Infos hat der Staat schon?
  ATTRIBUT_LISTE neue_infos;      // Welche Infos bekommt er diese Runde?
  ATTRIBUT_LISTE staat_und_stadt_infos; // wird nicht abgespeichert.
  ATTRIBUT_LISTE neue_mitspieler; // Welche neuen Mitspieler
  ATTRIBUT_LISTE entwicklungen;   // Technische Entwicklungen
public:
  ATTRIBUT_LISTE diplomatie;      // Welches Verhaeltnis zu den anderen
private:
  ATTRIBUT_LISTE *moegliche_entwicklungen; // Dafuer schon Voraussetzung.
  ATTRIBUT_LISTE *neu_moeglich;

  ADR mittelpunkt; // Adresse, von der aus die Koordinaten zaehlen.

  long punkte_letzte_runde;    // Fuer den Ergebnisausdruck gemerkt.
  long gesamtwertung;	       // Platz im Gesamtklassement
  long gesamtwertung_alt;      // Bisheriger Platz im Gesamtklassement
  long anzahl_angriffe;	       // Wird bei jedem Angriff hochgezaehlt

public:
    STAAT(char *, char *);
    ~STAAT();
    void spielerinformation(char *);	    // Neues Info bekommen
    void mitspielerinformation(OBJEKT *); // Neuen Staat getroffen
    void spieler_bekommt_info_ueber(char *name); // Ruft spielerinformation()
    short kommando(const char *, const void *par1=NULL, const void *par2=NULL, const void *par3=NULL);
    char *info(char *, void *par1=NULL, void *par2=NULL, void *par3=NULL);
    short befehl_auswerten(char *, long);
    short dipl_status_gegenueber(OBJEKT *);
    short endgueltig_freundlich_gegenueber(OBJEKT *);
    short mir_ist_endgueltig_freundlich_gegenueber(OBJEKT *);
    short habe_ursprung_des_lebens();
    char *alle_sieger_bzw_verlierer(short);
    short habe_gewonnen();
    bool bin_als_einziger_uebrig();
    short zivilisation_vernichtet();
    short ausgestiegen() { return attribut_gesetzt("Ausgestiegen"); };
    void gewinnerdatei_anlegen(short);
    DOUBLIST *besitztumsliste() { return &besitztum; };
    DOUBLIST *alle_staedte(bool auch_tote=false);
    DOUBLIST *alle_weltbauten();
    DOUBLIST *alle_einheiten(bool auch_tote=false);
    DOUBLIST *alle_einheiten_sortiert(bool auch_tote=false);
    STADT *groesste_stadt_ausser(STADT *ausser);
    STADT *groesste_stadt() { return groesste_stadt_ausser(0); };
    
    ADR& koordinaten_ursprung() { return mittelpunkt; };
    char *ortsangabe_auswerten(char *, ADR&, ADR&);
    void abkuerzung_expandieren(OBJEKT *, char);
    
    void einrichtungen_uebergeben(STADT *, STAAT   *); 
    void einrichtungen_uebergeben(STADT *, STADT   *);
    void einrichtungen_uebergeben(STADT *, EINHEIT *);
    
private:
  void naechste_phase(long);
  void naechster_zug(long);
  void befehle_einlasten();
  void boni_verteilen();
  void bonus_verteilen();
  void zug_abschliessen(long);
  void abschlussbericht();
  short laden(FILE *);
  short speichern(FILE *);
  short befehl_staatsform_aendern(char *);
  short befehl_landschaftsabbild();
  short befehl_diplomatischer_status(char *);
  short befehl_administration(char *);
  short befehl_administration_w(char *);
  short befehl_administration_g();
  short befehl_mitteilung(char *);
  short befehl_bauen_abbrechen(char *);
  char *dipl_status_name(char, char);
  char *dipl_status_name(char *s) { return dipl_status_name(s[0], s[1]); };

  short kommando_einfluesse_berechnen();
  short kommando_neue_entwicklung(void *);
  short kommando_stadt_einnehmen(void *);
  short kommando_stadt_verlieren(void *, void *);
  short kommando_stadt_atomgebombt(void *, void *);
  short kommando_weltbaut_atomgebombt(void *, void *);
  short kommando_weltbaut_beschossen(void *, void *, void *);
  short kommando_punkteauswertung(void *);
  short kommando_gesamtwertung_platz(void *);
  short kommando_angriff_zaehlen();
  short kommando_bonuspunkte(void *);
  short kommando_aggression(void *);
  short kommando_infos_neu();
  short kommando_ausstieg();

  void stadt_wurde_eingenommen(OBJEKT *, OBJEKT *);
  short neue_hauptstadt_waehlen();

  char *info_voraussetzungen_erfuellt(void *);
  char *info_einwohnerzahl();
  char *info_bevoelkerungswachstum();
  char *info_staedtezahl();
  char *info_stand_der_forschung();
  char *info_bekannte_welt();
  char *info_offensivkraft();
  char *info_rohstoffproduktion();
  char *info_bau_und_entwicklung();
  char *info_relative_adresse(void *);
  char *info_absolute_adresse(void *);
  char *info_untersuche(void *);
  char *info_punkte();

  WELT *welt() { return (WELT *)objekt_mit_namen(attribut("WELTNAME")); };
  STADT *hauptstadt();

  void abkuerzung_definieren(char, char *);
  char *abkuerzung_abfragen(char k) { return attribut(kennungattr(k)); };
  char normkennung(char k) { k=toupper(k); if(k<'A' || k>'Z') return 'Z'; else return k; };
  char *kennungattr(char);
  char *av_ad_aa_entfernen(char *);

public:
  void formatieren_und_ausdrucken(char *filename, long breite, char *gn=NULL);
  void trennlinie(float abstand = 0.1);
  static void href_auf_info_einfuegen(HTML&, VORHABEN_ENZ *);
private:
  void sektionsueberschrift(char *, float bpp=0.0161176);
  void ausdruck_sektion_titel();
  void ausdruck_sektion_spieler(short befbog=0);
  void ausdruck_sektion_spielende();
  OBJEKT *irgendeine_stadt();
  void ausdruck_sektion_staat();
  void ausdruck_sektion_staedte();
  void ausdruck_sektion_weltbauten();
  void ausdruck_sektion_einheiten();
  void ausdruck_sektion_stadtinfos();
  void ausdruck_stadtinfo(OBJEKT *, char *);
  void ausdruck_sektion_listen();
  void projekttabelle_drucken(ATTRIBUT_LISTE *);
  void ausdruck_sektion_weltkarte();
  void ausdruck_sektion_mitspieler();
  void ausdruck_sektion_infos();
  void ausdruck_sektion_gelaendeformen();
  void ausdruck_sektion_punktewertung(long);
  void ausdruck_sektion_abkuerzungen();
  void ausdruck_sektion_befehle(long);
  void ausdruck_allgemeine_mitteilungen(long);
  void ausdruck_persoenliche_mitteilungen(long);
  void befehlsbogen_drucken(long);
  
    void ausdruck_hauptindex_html();
    void ausdruck_index_html();
    void ausdruck_staat_html(char *);
    void ausdruck_projekte_html();
    void ausdruck_befehle_html();
    void ausdruck_diplomatie_html();
    void ausdruck_abkuerzungen_html();
    void ausdruck_mitteilungen_html(long);
    void befehlsbogen_html();
    void ausdruck_ps_gesamtkarte(int);

    ATTRIBUT_LISTE *liste_jeder_moeglichen(char *t=NULL, short i=0);
    
    void info_drucken(char *);
    void grafikinfo_drucken(char *, char *);
};

#endif // __staat_h

