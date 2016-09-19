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
// *************************************************************************
// MODUL:               einheit.h / EINHEIT.H
// AUTOR/DATUM:         Mathias Kettner, 28. April 1993
// KOMPATIBILITAET:     C++
// *************************************************************************
//
//      Deklariert das Objekt EINHEIT.
//
// *************************************************************************
#ifndef __einheit_h
#define __einheit_h

#include "welt.h"
#include "staat.h"
#include "weltbaut.h"
#include "stdio.h"
#include "string.h"
#include "strategi.h"
#include "html.h"

class STADT;

class EINHEIT : public OBJEKT
{
    long angriffskraft;
    long verteidigungskraft;
    long bewegungspunkte_pro_feld;
    float sichtweite;
    char *alter_zustand;
    RESOURCE_VEKTOR lager; // Fuer Einheiten, die transportieren koennen
    
    ADR startadresse; // Am Anfang der Runde. Wird vom Ausdruck benoetigt.
    OBJEKT *startort; // Ort am Anfang der Runde. Auch fuer den Ausdruck
    
    long aktuelle_strategie; // Aktuelle Stratigie. (S_UNBEDINGT_ANGREIFEN...)
    long aktuelle_prioritaet; 
    DOUBLIST alarms; // Siehe strategi.h/.cpp usw.
    
    short fussnoten_zaehler; // Wird jeden Zug wieder auf 0 gesetzt.
    short fehlermeldungsnummer;
    short anzahl_kaempfe; // Soviele Kaempfe hat die Einheit diese Runde erlebt.
    short medaillen;      // Soviele Einheiten hat die Einheit vernichtet.
  
public:
    EINHEIT(char *, char *);
    ~EINHEIT();
    short befehl_auswerten(char *, long);
    short kommando(const char *, const void *par1=NULL, const void *par2=NULL, const void *par3=NULL);
    char *info(char *, void *par1=NULL, void *par2=NULL, void *par3=NULL);
    void naechster_zug(long);
    void zug_abschliessen(long);
    void naechste_phase(long);
    short ist_neu();
    short vorsortierungsnummer();
    static short sortfunktion_einheiten(DOUBLIST_NODE *, DOUBLIST_NODE *, void *);
    void abschlussbericht();
    void abschlussbericht_html(HTML);
    void objektgrafik_mit_link(HTML&, OBJEKT *);
    short laden(FILE *);
    short speichern(FILE *);
    short objekt_aufnehmen(OBJEKT *, ADR&);
    void objekt_entlassen(OBJEKT *);
    
private:
    short kommando_angegriffen_werden(void *, void *);
    bool kann_ueberrannt_werden(long, EINHEIT *);
    short kommando_beschossen_werden(void *, void *);
    short kommando_sich_vernichten();
    short kommando_alarm(void *, void *);
    short kommando_resourcen_bekommen(void *);
    short kommando_resourcen_anfordern(void *, void *); // Nur um fuer Basis!
    short kommando_angriff_folgt(void *);
    short kommando_ausstieg();
    
    char *info_verteidigungskraft(const void *par1 = NULL);
    char *sichtungen_ermitteln();
    char *sichtungen_ermitteln(DOUBLIST *);   // Version mit 
    // ... vorberechneter Objekteliste
    char *info_resourcen_anfordern(void *, void *, void *);
    char *info_offensivkraft();
    char *info_angriffs_unterstuetzung();
    char *info_verteidigungs_unterstuetzung();
    
    long effektive_verteidigungskraft(ADR& a = ADR::ausserhalb());
    long effektive_angriffskraft(ADR&);
    DOUBLIST *alle_unterstuetzenden_einheiten(ADR&);
    
    short befehl_reisen(char *, long);
    short befehl_stadt_gruenden(char *);
    short befehl_eingraben();
    short befehl_ausgraben();
    short befehl_in_reserve();
    short befehl_mobil_machen();
    short befehl_strassen_bauen();
    short befehl_brandschatzen();
    short befehl_wr_af_tr(char *);
    short befehl_ueberstellen(char *);
    short befehl_einnehmen();
    short befehl_embark(char *, long);
    short befehl_disembark(char *, long);
    short befehl_angreifen(char *);
    short befehl_feld_erobern(char *, long);
    short befehl_feuern(char *, long);
    short befehl_aufloesen();
    short befehl_umladen(char *);
    short befehl_prioritaet_setzen(char *);
    short befehl_strategie_waehlen(char *);
    short befehl_gehe_zu(char *);
    short befehl_schritt_in_richtung(char *);
    short befehl_pendelverkehr(char *);
    short befehl_explorieren(char *);
    short befehl_begleiten(char *);
    short befehl_warten(char *, long);
    short befehl_ansiedeln();
    short befehl_benennen(char *);
    short befehl_bodenschaetze_suchen();
    short befehl_bauen(char *);
    short befehl_bombardieren(long);
    short befehl_wiederhole(char *, long);
    short befehl_untersuche(char *);
    short befehl_meteorit_bergen();
    short befehl_abkuerzung_verwenden(char *);
    void welt_informieren_ueber_meteorit(short);
    short befehl_verwuesten();
    
    void unterhalt_einholen();  // Damit versorgt sich die Einheit von der Stadt.
    
public:
    STAAT *staat();
    short ort_ist_welt() { return !strcmp(ort()->attribut("TYP"),"WELT"); };
//    short ort_ist_stadt() { return !strcmp(ort()->attribut("TYP"),"STADT"); };
//    short ort_ist_weltbaut() { return !strcmp(ort()->attribut("TYP"),"WELTBAUT"); };
    short ort_ist_einheit() { return !strcmp(ort()->attribut("TYP"),"EINHEIT"); };
    WELT *welt() { return (WELT *)ort(); };
    WELT *landschaft() { return (WELT *)ort(); };
    EINHEIT *einheit() { return (EINHEIT *)ort(); };
    char *gelaende_attribut(char *);
    bool wehre_rakete_ab(ADR&, ADR&, long, STAAT *);
    
private:
    STADT *stadt_hier();
    WELTBAUT *weltbaut_hier();
    short basis_hier();
    short basis_im_umkreis(ADR&, float);
    short unterhalt_von_basis_einholen();
    DOUBLIST *liste_aller_brauchbaren_basen();
    static short sortfunction_basen(DOUBLIST_NODE *,DOUBLIST_NODE *, void *);
    
private:
    short platz_fuer_baut(); // 1, wenn Stadt oder Weltbaut auf gleichem Ort.
    void feld_beschiessen(ADR&, long); // Hilfe fuer b_feuern und b_bombardieren
    ADR& zielfeld_bestimmen(char *);
    char *pruefe_bewegung_zu_feld(ADR&, short modus = 0);
    static short entfernungsfunktion(void *, ADR&, ADR&);
    static short sort_objliste_koord(DOUBLIST_NODE *,
				     DOUBLIST_NODE *, void *);
    void sichtungen_ins_tagebuch(long, char *zustand = NULL);
    void sichtungen_aktualisieren(long phase, DOUBLIST *l = NULL);
    void gelaende_aktualisieren();
    void enthaltene_objekte_vernichten();
    char *relative_adressangabe(ADR&, OBJEKT *, short);
    ADR& absolute_adresse(char *);
    RIC& relative_adresse(ADR&);
    short wir_hassen(OBJEKT *o)
	{ return (staat()->kommando("DIPL_STATUS",(void *)o) == 0); };
    short wir_dulden(OBJEKT *o)
	{ return (staat()->kommando("DIPL_STATUS",(void *)o) == 1); };
    short wir_lieben(OBJEKT *o)
	{ return (staat()->kommando("DIPL_STATUS",(void *)o) >= 2); };
    short uns_hasst(OBJEKT *o);
    short uns_duldet(OBJEKT *o);
    short uns_liebt(OBJEKT *o);
    
    void enthaltenen_einheiten_berichten(char *);
    void medaille_bekommen();
    
    // Funktionen von strategi.C
    void alarmiert_werden(short typ, char *parameter=NULL);
    void alle_alarms_loeschen();
    ALARM *wichtigster_alarm();
    void einheit_alarmieren(OBJEKT *ziel, short typ, char *par=NULL);
    void alarms_pruefen();
    void alarm_verarbeiten(ALARM *alarm);
    short befehl_darf_eingeschoben_werden(char *);
    short sc_pruefen(char *);
    DOUBLIST *objekte_der_umgebung_pruefen();
    void weitere_alarmursachen_pruefen();
};

#endif // __einheit_h

