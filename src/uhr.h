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
// MODUL:               uhr.h / UHR.H
// AUTOR/DATUM:         Mathias Kettner, 12. Mai 1993
// KOMPATIBILITAET:     C++
// **************************************************************************
//
//      Enthaelt die Deklaration des Objektes UHR
//
// **************************************************************************

#ifndef __uhr_h
#define __uhr_h

#include "objekt.h"
#include "statisti.h"
#include "resource.h"

#define UNIVERSUM NULL

// Konstante fuer den Bildschirmaufbau

#define Y_EINGABE 4
#define Y_STATUS 22
#define Y_DATENBEREICH 6
#define Y_DATENBEREICHZEILEN (Y_STATUS - Y_DATENBEREICH - 1)
#define Y_DATENBEREICHENDE (Y_DATENBEREICH + Y_DATENBEREICHZEILEN - 1)

// Konstanten fuer die Variable fuer die momentane Verwendung des Datenbereiches

/**---------------------------------------------------------------------------
  * KLASSE:              UHR
  * ABGELEITET VON:      OBJEKT
  * 
  * Im gesamten Objektbaum ( Baum bezueglich der Besitzrelation ) ist
  * die "Uhr" das Wurzelobjekt. Sie hat keinen Besitzer und besitzt
  * selbst die Landschaft, die Enzyklopaedie und die Staaten. Sie
  * hat den Namen "Uhr" und ist vom Typ UHR. Ferner enthaelt sie
  * die gesamte Menusteuerung der Spielverwaltung. In der main() Funk-
  * tion des Programms wird als einziges die Uhr erschaffen und solange
  * die Funktion zug() aufgerufen, bis die Uhr vernichtet ist.
  *
  ---------------------------------------------------------------------------*/

class STAAT;
class SPIELERDATENSATZ;

class UHR : public OBJEKT
{
  long spielrunde;
  char datum_dieser_auswertung[9];
  char datum_der_naechsten_auswertung[9];
  DOUBLIST hitlisten; // Fuer die Punkteverteilung
  OBJEKT *staat_auf_platz_eins; // ist ein Ergebnis der Auswertung
  short spielstand_modifiziert;

public:
    UHR(char *, char *);
    ~UHR();
    char *info(char *, void *par1=NULL, void *par2=NULL, void *par3=NULL);
    DOUBLIST *alle_staaten(bool auch_verlierer=true, bool auch_ausgestiegene=true);
    RESOURCE_VEKTOR aktuelle_forschungskosten(char *name, RESOURCE_VEKTOR neupreis);
    short alle_sind_freundlich();
    short einer_hat_ursprung_des_lebens();
    bool  spiel_ist_zuende() { return alle_sind_freundlich() || einer_hat_ursprung_des_lebens(); };
    
    char *dateinamen_generieren(char *format, STAAT *staat=NULL, char *v=NULL);
    char *logdateiname();
    char *befehlsdateiname(STAAT *staat);
    char *infodateiname(char *infoname); // z.B. "krieger.inf"
    char *tempfilename(const char *bestandteil);
    char *bigmapfilename();
    char *smallmapfilename();
    char *htmlmapfilename(char *bestandteil); // z.B. 0304
    char *gifmapfilename();
    char *persoenliche_mitteilungdateiname(STAAT *staat);
    char *allgemeine_mitteilungdateiname();
    char *htmldateiname(STAAT *staat, char *variabel);
    char *grafikdateiname(char *grafikname);
    char *printout_dateiname(STAAT *staat, char *a_oder_b);
    char *asciiout_dateiname(STAAT *staat);
    char *gelaende_dateiname();
    char *emailin_dateiname();
    char *emaildone_dateiname(STAAT *, char *);

private:
    void spielstand_ist_nun_modifiziert()   { spielstand_modifiziert = 1; };
    void spielstand_ist_nun_gespeichert()   { spielstand_modifiziert = 0; };
    short ist_spielstand_modifiziert() { return spielstand_modifiziert; };
    
    short laden(FILE *);
    short speichern(FILE *);
    
    char *info_zugnummer();
    char *info_datum();
    char *info_naechster_zug();
    char *info_hitlisten() { return (char *)&hitlisten; };
    char *info_bester_staat() { return (char *)staat_auf_platz_eins; };
    char *info_sessionname();

    void naechster_zug(long);
    void zug_abschliessen(long);
    
    short laden_speichern(char *, int);
    short spielstand_laden();
    short spielstand_speichern(char *);
    
    void spielerliste();
    void spielerliste_anzeigen();
    void spieler_darstellen(OBJEKT *, short);
    void spielerliste_spezialtaste(int);
    
    char *attribute_fuer_welterschaffung();
    static void weltmaske_refresh();
    
    void statistiken_berechnen();
    void hitliste_erstellen(char *, char *, char *, short);
    static short sortfunction_hitliste(DOUBLIST_NODE *,
					 DOUBLIST_NODE *, void *);
    void emails_parsen(long);
    STAAT *email_parsen(char *, long, unsigned long int);
    void befehle_aus_email_holen(char *, STAAT *);
    char *befehlszeile_schoenmachen(char *);
    
    // Hier kommt das neue User interface.
    short user_interface();
    short ui_auswahl();
    void ui_menu_aufbauen();
    void ui_befehle_zaehlen(long&, long&);
    void ui_zaehle_befehle_von(OBJEKT *, long&, long &);
    void ui_spielertabelle();
    OBJEKT *ui_spieler_erfragen();
    OBJEKT *ui_neuer_spieler();
    void ui_spieler_editieren(OBJEKT *staat);
    void ui_eventuell_infos_neu(OBJEKT *);
    static void ui_se_maske_aufbauen();
    void ui_spieler_entfernen();
    void ui_befehle_eingeben();
    short ui_weiter();
    short ui_ende();
    short ui_erde_schaffen();
    void ui_weltkarte_ausdrucken();
    void ui_feld_aendern();
    
    void welt_erschaffen(char*, long, long); // Nicht interaktive Funktion
    void uebersichtskarten_erzeugen();
};

#endif // __uhr_h

