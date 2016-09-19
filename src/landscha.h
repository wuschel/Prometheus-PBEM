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
/**---------------------------------------------------------------------------
  * MODUL:               landscha.h / LANDSCHA.H
  * AUTOR/DATUM:         Mathias Kettner, 25. April 1993
  * KOMPATIBILITAET:     C++     
  -----------------------------------------------------------------------------
  * 
  *      Diese Modul enthaelt alle Klassen, die im Zusammenhang mit dem
  *      Objekttyp "Landschaft" stehen. Sie stehen untereinander in ver-
  *      schiedene Erbschaftsbeziehungen.
  *
  *      BASIS_LANDSCHAFT
  *        -> MATRIX_ATLAS
  *          -> 2D_MATRIX_ATLAS
  *            -> RECHTECK_ATLAS
  *            -> ZYLINDER_ATLAS
  *
  *
  *
  *      ADR
  *      ADR_LIST_NODE
  *      RIC
  *
  ---------------------------------------------------------------------------*/

#ifndef __landscha_h
#define __landscha_h

#include "gelaende.h"
#include "adr_ric.h"

/**---------------------------------------------------------------------------
  * KLASSE:              BASIS_LANDSCHAFT
  * ABGELEITET VON:      OBJEKT
  * 
  * Die Klasse BASIS_LANDSCHAFT ist ein algmeines Grundkonzept
  * zur Realisierung von Landschaften in einem Objektorientierten
  * Spiel. Sie legt sich noch nicht auf den geometrischen Aufbau
  * fest, der z.B. 2D-Matrix, 3D-Matrix usw. sein kann. Ferner ent-
  * haelt sie fast ausschliesslich virtuelle Funktionen. Dies liegt
  * daran, dass die realisierung der meisten Funktionen sehr stark
  * vom tatsaechlichen Landschaftsmodell abhaengt und deshalb dort
  * entsprechend defininiert werden muessen.
  *
  * Gewisse Eigenschaften einer Landschaft hat auch schon die Basis-
  * klasse OBJEKT. Sie hat virtuelle Funktion zum Aufnehmen und
  * Entlassen eines Objektes aus einer raeumlichen Beziehung. Dies
  * ist durch die Funktionen objekt_aufnehmen() und objekt_entlassen()
  * realisiert, die jedoch virtuell ueberladen werden koennen.
  * Grundsaetzlich gilt: Alle Funktionen, die in irgendeiner Art von
  * einer Adresse (ADR) abhaengen, sind in BASIS_LANDSCHAFT
  * zu finden. Das Basisobjekt OBJEKT kennt keine Spezifizierung
  * der genauen raeumlichen Lage von Objekten, die sie "enthaelt".
  *
  *
  * FUNKTIONEN:
  * 
  * DOUBLIST *alle_objekt_bei_adresse(ADR)
  * Bestimmt alle Objekte an einem bestimmten Ort.
  *
  * VIRTUELLE FUNKTIONEN:
  *
  * virtual long anzahl_objekte_bei_adresse(ADR)
  * Zaehlt die Anzahl der Objekte mit gleicher Position
  *
  * virtual short benachbart(ADR, ADR)
  * Feststellen, ob zwei Postitionen benachbart sind.
  *
  * virtual ADR adresse_in_richtung(ADR, RIC)
  * Zu einer Position eine Vektor addieren.
  ---------------------------------------------------------------------------*/

#include "objekt.h"
#include "genesis3.h"
#include "adrindex.h"

class OBJEKT; // Ist trotzdem noetig, da diese File von objekt.h
		  // included wird zu einem Zeitpunkt, wo OBJEKT
		  // noch nicht definiert ist.

class RESOURCE_VEKTOR;

class BASIS_LANDSCHAFT : public OBJEKT
{
public:
  BASIS_LANDSCHAFT(char *n, char *a) : OBJEKT(n,a) {};
  virtual DOUBLIST *alle_objekte_bei_adresse(ADR&, char *bed=NULL) = 0;
  virtual DOUBLIST *alle_objekte_im_umkreis_von(ADR&, float, char *bed=NULL) = 0;
  long anzahl_objekte_bei_adresse(ADR&);

  virtual short benachbart(ADR&, ADR&) { return 0; };
  virtual ADR& adresse_in_richtung(ADR&a, RIC&) { return a; };
  virtual RIC& richtung_von_nach(ADR&, ADR&) { return RIC::null(); };
  virtual short adresse_ist_ausserhalb(ADR&) { return 1; };
  virtual float entfernung_zwischen(ADR&, ADR&) { return 0.0; };
  virtual DOUBLIST *alle_adressen_im_umkreis_von(ADR&, float)
	      { return new DOUBLIST; };
};

class MATRIX_ATLAS : public BASIS_LANDSCHAFT
{
protected:
  GELAENDE_FORM_TABELLE gelaende_formen;

public:
    bool ok() const { return gelaende_formen.ok(); };
    MATRIX_ATLAS(char *n, char *a);
    short speichern(FILE *); // Wegen der Startpositionen...
    short laden(FILE *);     // ...muss ich Laden und Speichern
    
    virtual void matrix_leeren() {};
    
    int anzahl_gelaendeformen() { return gelaende_formen.get_anzahl(); };
    
    GELAENDE_FORM *gelaendeform(short f)
	{ return gelaende_formen.form_mit_nummer(f); };

    short gelaendeform_mit_namen(char *n)
	{ return gelaende_formen.form_mit_namen(n); };
    
    short gelaendeform_mit_abkuerzung(char *a)
	{ return gelaende_formen.form_mit_abkuerzung(a); };
    
    char *gelaendeform_attribut(short form, char *klasse)
	{ return gelaende_formen.attribut_fuer_form(form, klasse); };
    
    void   gelaendeform_aendern(ADR&, short);
    short  gelaendeform_in_feld(ADR&);
    void   setze_feld_attribute_maske(ADR&, short);
    short  feld_attribute_maske(ADR&);
    void   setze_feld_attribut(ADR&, short); // Mit Nummer (1..7)
    void   loesche_feld_attribut(ADR&, short);
    short  feld_attribut(ADR&, short); // Ermitteln, ob Attri X gesetzt ist.
    char  *gelaendeform_attribut(ADR&, char *klasse);
    short  gelaendeform_attribut_gesetzt(ADR&, char *);
//    int    farbtabellen_erstellen(byte *, byte *, byte *);
    
    virtual short wrap(ADR *) { return 0; };
    virtual unsigned short feld(ADR&) { return 0; };
    virtual void setze_feld(ADR&, unsigned short) {};
};

class ZWEID_MATRIX_ATLAS : public MATRIX_ATLAS
{
protected:
  long breite, hoehe;
  unsigned short **matrix;
  short diagonal; // Ist 1, wenn Felder mit 1 gemeins. Ecke benachbart sind
  ADRINDEX *adrindex;
public:
  ZWEID_MATRIX_ATLAS(char *, char *);
  ~ZWEID_MATRIX_ATLAS();
  void matrix_leeren();

  long get_breite() { return breite; };
  long get_hoehe() { return hoehe; };
  unsigned short feld(ADR&);
  void setze_feld(ADR&, unsigned short);
  short adresse_ist_ausserhalb(ADR&);
  ADR& adresse_in_richtung(ADR&, RIC&);
  DOUBLIST *alle_adressen_im_umkreis_von(ADR&, float);
  short leeres_feld_aussenrum(ADR& pos, ADR& leeresfeld);
  void irgendein_feld_aussenrum(ADR& pos, ADR& feld);

  short laden(FILE *);
  short speichern(FILE *);

  short objekt_aufnehmen(OBJEKT *o, ADR& adr) { adrindex->enter_objekt(o, adr); return 0; }
  void objekt_entlassen(OBJEKT *o)            {  adrindex->remove_objekt(o); }
  short objekt_bewegen(OBJEKT *o, ADR& a) { objekt_entlassen(o); objekt_aufnehmen(o, a); return 0; };
  DOUBLIST *alle_objekte_bei_adresse(ADR& adr, char *bed=NULL) 
  { return adrindex->alle_objekte_bei_adresse(adr, bed); };
  DOUBLIST *alle_objekte_im_umkreis_von(ADR& adr, float radius, char *bed=NULL)
  { return adrindex->alle_objekte_im_umkreis_von(adr, radius, bed); };
  short objekt_bei_adresse(ADR& a) { return adrindex->objekt_bei_adresse(a); };
  OBJEKT *stadt_bei_adresse(ADR& a);
    
  void adrindex_berechnen();

  void matrix_anzeigen(); // Zum Debuggen.

  void genesis_teil_3();
  void g3_keime_setzen();
  void g3_keim_setzen(short);
  long stringsumme(char *);
  void g3_gelaende_formen();
  void g3_rest_fuellen(long);
  short g3_keim_waechst(KEIM *);
  short g3_klimatest(long, short, short);
  float g3_klimazugehoerigkeit(long,  short, long dy=0);
  short g3_nachbarn_vertraeglich(short, ADR&);
  void g3_weltkarte_ausdrucken(char *filename=NULL);
  void g3_weltkarte_ausdrucken_html();
  void g3_weltkarte_ausdrucken_gif(char *filename=NULL);
  void g3_diagonalen_entfernen();
  short g3_ist_meer(int, int);
};


class ZYLINDER_ATLAS : public ZWEID_MATRIX_ATLAS
{
public:
  ZYLINDER_ATLAS(char *n, char *a) : ZWEID_MATRIX_ATLAS(n,a) {};
  short benachbart(ADR&, ADR&);
  short wrap(ADR *);
  float entfernung_zwischen(ADR&, ADR&);
  RIC& richtung_von_nach(ADR&, ADR&);
};

#endif // __landscha_h

