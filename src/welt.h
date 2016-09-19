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
// MODUL:               welt.h / WELT.H
// AUTOR/DATUM:         Mathias Kettner, 30. April 1993
// KOMPATIBILITAET:     C++
// **************************************************************************
//
//      Deklariert die Klasse WELT, die von der Kasse ZYLINDER_ATLAS
//      abgeleitet ist und die Landkarte fuer das an Civilization Brief-
//      spiel darstellt.
//
// **************************************************************************

#ifndef __welt_h
#define __welt_h

#include <string.h>
#include <stdio.h>

#include "objekt.h"
#include "resource.h"
#include "landscha.h"
#include "wegsuche.h"
#include "mystring.h"
#include "html.h"

class STAAT;
class HTML;

#define FELD_ATTRIBUT_BODEN_UNTERSUCHT 1 // Wird nun intern von Welt benoetigt.
#define FELD_ATTRIBUT_STRASSE 2
#define FELD_ATTRIBUT_BEWIRTSCHAFTUNG 4
#define FELD_ATTRIBUT_FREI2 5
#define FELD_ATTRIBUT_FREI3 6
#define FELD_ATTRIBUT_FREI4 7

// Konstante fuer die Klasse LANDSCHAFTSABBILD fuer die bit_matrix[][].

#define LA_IMMOBILMASKE 0xe0 // Die obersten drei Bits
#define LA_STADT 0xe0        // Stadt hat die Nummer 7: Hoechste Prioritaet

#define LA_MOBILMASKE 0x1f     // Die unteren Bits fuer mobile Objekte
#define LA_EINHEIT 0x01        // Fuer Einheiten
#define LA_EIGENESOBJEKT 0x10  // Wird addiert um eigene Objekte zu kennz.

/**---------------------------------------------------------------------------
  ---------------------------------------------------------------------------*/

class LANDSCHAFTSABBILD;
class LAYOUT;


class BODENSCHATZ : public DOUBLIST_NODE
{
    ADR adresse;
    RESOURCE_VEKTOR lager;
    short matches(void *k)
	{ return ((ADR *)k)->x==adresse.x && ((ADR *)k)->y==adresse.y; };
    friend class WELT;
};


class WELT : public ZYLINDER_ATLAS
{
    DOUBLIST abbilder;
    DOUBLIST bodenschaetze;
public:
    WELT(char *n, char *a);
public:
    short laden(FILE *);
    short speichern(FILE *);
    void naechster_zug(long);
    
    short kommando(const char *, const void *par1=NULL, const void *par2=NULL, const void *par3=NULL);
    char *info(char *, void *par1=NULL, void *par2=NULL, void *par3=NULL);
    RESOURCE_VEKTOR& resourcen_auf_feld(ADR&);
private:
    char *info_feld_bewirtschaftung_pruefen(void *);
    char *info_feld_beschreibung(void *);
    
    short kommando_feld_bewirtschaften(void *);
    short kommando_feld_nicht_mehr_bewirtschaften(void *);
    short kommando_uebersichtskarte_drucken(void *filename);
    short kommando_startpunkt_zurueck(void *);
    
    void meteoritenfall(long zugnummer);
    void meteorit_werfen();

public:
    void landschaftsabbild_anlegen(OBJEKT *);
    void landschaftsabbild_vernichten(OBJEKT *);
    LANDSCHAFTSABBILD *landschaftsabbild(OBJEKT *);
    void landschaftsabbild_aktualisieren(OBJEKT *, ADR&, float);
    void landschaftsabbild_komplett(OBJEKT *);
    long landschaftsabbild_auszaehlen(OBJEKT *);
    void landschaftsabbild_ausdrucken(OBJEKT *);
    void weltkarte_html(STAAT *);
    void weltkarte_gif(STAAT *, char *);
    void teilmatrix_drucken_html(LANDSCHAFTSABBILD *, STAAT *, int, int, int, int,
				 short, short, short, short, short);
    HTML *html_anlegen(STAAT *, short, int, int, int, int);
    void sicht_eines_objektes(OBJEKT *, ADR&, float);
    void landschaftsabbild_objekte_loeschen(OBJEKT *);
    LAYOUT *feld_layout(float l, float o, short gform, short feldattr);
    LAYOUT *feld_layout(float l, float o, ADR& a);
    void gelaendeform_infos();
    void gelaendeform_infos_html(HTML& html);
  
    DOUBLIST *alle_gegner_bei_adresse(OBJEKT *staat, ADR&);
    char *objekt_aufzaehlung(void *, void *);
private:
    void koordinatensystem(float,float,float,long,long,long,long,long,long,float);
    void teilmatrix_drucken(OBJEKT *, long, long, long, long);
    
    void uebersichtskarte_drucken();
    
    short bodenschaetze_speichern(FILE *);
    short bodenschaetze_laden(FILE *);
public:
    short nach_resourcen_bohren(ADR&);
    RESOURCE_VEKTOR& bodenschatz_bei(ADR&);
    short bodenschatz_vorhanden(ADR&);
    bool WELT::abwehr_durch_mobile_raketen(ADR&, ADR&, long, STAAT *);
    
    // Funktionen fuer die Wegsuche (in wegsuch.C definiert)
public:
    DOUBLIST *schnellster_weg(ADR&,ADR&,EFKT,void *,long *l=NULL);
private:
    short **hilfsmatrix_anlegen();
    void hilfsmatrix_vernichten(short **);
    long hilfsmatrix_numerieren(ADR&, ADR&, short**, EFKT, void *);
    DOUBLIST *weg_zurueckrechnen(short **, ADR&);
};


/**---------------------------------------------------------------------------
  * KLASSE:              LANDSCHAFTSABBILD
  * ABGELEITET VON:      DOUBLIST_NODE
  * 
  * Ein Landschaftsabbild ist ein subjektives Abbild der Welt. Die Er-
  * fordernis einer sbujektiven Ansicht von der Welt erwaechst aus der
  * Subjektiven Perspektive der am Spiel beteiligten Staaten. Jeder
  * Staat hat seine eigene Ansicht von der Welt.
  * Im Klartext bewaeltigt diese Klasse das Problem, dass jeder Spieler
  * nur einen Teil der Welt kennt (den er schon erkundet hat) und auch
  * nur in einem Teil der Welt Bewegungen von fremden Objekten wahrnimmt.
  * Dazu kommt, dass die Welt sich in Abwesenheit des Spielers veraendern
  * kann. Im Verlauf des Spiels ist das Landschaftsabbild jedes Staates
  * zunaechst schwarz und wird dann staendig aktualisiert, und zwar an
  * den Stellen, die der Spieler mit seinen Einheiten oder Staedten er-
  * forschen kann.
  * Zur Realisierung enthaelt jedes Landschaftsabbild zwei Matrizen von
  * denen die eine ("matrix") in der Funktionalitaet der Matrix der
  * ZWEID_MATRIX_ATLAS entspricht und die andere Zusaetliche Ein-
  * traege ueber gesehene Objekte enthaelt.
  *
  * FUNKTIONEN:
  * Konstruktor, Destruktor, matches()
  *
  * feld_aktualisieren(ADR&, unsigned short)
  * Traegt in die Matrix "matrix" einen Wert ein.
  ---------------------------------------------------------------------------*/

class LANDSCHAFTSABBILD : public DOUBLIST_NODE
{
    long breite, hoehe;
    unsigned short **matrix;
    unsigned char **bit_matrix;
    char *besitzer;
public:
    LANDSCHAFTSABBILD(long, long, OBJEKT *, FILE *file=NULL);
    ~LANDSCHAFTSABBILD();
    short matches(void *krit) { return !mystrcmp_no_case(besitzer,(char *)krit); };
    void feld_aktualisieren(ADR&, unsigned short);
    void benutzten_bereich_ermitteln(long&, long&, long&, long&, long);
    unsigned short feld(ADR& a) { return matrix[a.x][a.y]; };
    friend class WELT;
};

#endif // __welt_h

