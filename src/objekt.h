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
// MODUL:               objekt.h / OBJEKT.H
// AUTOR/DATUM:         Mathias Kettner, 20. April 1993
// KOMPATIBILITAET:     C++
// **************************************************************************
//     
//      Enthaelt folgende Datentypen:
//
//       OBJEKT_LIST_NODE
//       OBJEKT
//       BEFEHL
//
// **************************************************************************

#ifndef __objekt_h
#define __objekt_h

#include "doublist.h"
#include "attribut.h"
#include "adr_ric.h"
#include "mystring.h"
#include "einfluss.h"

class ADR;
class BASIS_LANDSCHAFT;
struct OBJEKT_LIST_NODE;
class STAAT;

extern EINFLUSS_LISTE globale_einfluss_menge;


class OBJEKT : public DOUBLIST_NODE
{
public:
  char          *name;         // Eindeutige Abkuerzung
  OBJEKT        *besitzer;     // Verkettung nach oben im Baum
  ATTRIBUT_LISTE attribute;    // Liste von benannten Stringvariablen
  ADR            adresse;      // Gibt genaue "raumliche" Position an
  DOUBLIST       befehlsliste; // Liste von Strings

private:
  short   vernichtenswert;  // != 0, wenn Objekt vernichtet werden soll
  char   *temp_ort_name;    // Temporaerer Zeiger auf Namen des Ort-Objektes
  OBJEKT *ort_objekt;       // In diesem Objekt befinde ich mich
  char   *tagebuch_eintraege;
  long    namenszaehler;

protected:
  DOUBLIST reportliste; // Speichert die Eintraege von report()
  OBJEKT_LIST_NODE *global_node; // Zur Verkettung in globaler Liste
  DOUBLIST besitztum;	     // Baumartige Verkettung nach unten
  long phasenzaehler;		     // Im Zusammenhang mit den Befehlen

  // 1. Konstruktor / Destruktor
public:
  OBJEKT(char *, char *attr = NULL);
  ~OBJEKT();

  // 2. Private Funktionen zur internen Verwaltung
private:
  short besitztum_speichern(FILE *);
  short befehlsliste_in_file(FILE *);
  short befehlsliste_aus_file(FILE *);
  short unterobjekt_laden(FILE *);

  // 3. Servivefunktionen fuer abgeleitete Klassen
protected:
  void reportliste_loeschen() { reportliste.clear(); };
  void tagebuch_loeschen(); // Tagebuch loeschen
  short unbekannter_befehl(char *); // Hilfsfunktion fuer befehl_auswerten()

  // 4. Manipulationen an sich selbst und Informationen ueber sich selbst
  //    fuer abgeleitete Klassen

protected:
  OBJEKT *objekt_schaffen(char *name, char *typ, char *attr);
  void beeinflussen(char *art, char *ziel, char *parameter=NULL);
public: // Wegen den Abkuerzungen. STAAT::abkuerzung_expandieren()
    void befehl_vorschieben(char *);
protected:
  void befehl_nachschieben(char *);
  void befehl_umwandeln(char *);
  void befehl_durch_naechsten_ersetzen();
  short befehl_dauert_noch(long); // Innherhalb von befehl_auswerten()
  void direktes_besitztum_permutieren(char *); // Mischt Objekte 

  // 5. Oeffentliche Funktionen zum Abfragen und korrekten Manipulieren
public:
    char  *a_typ()         { return attribut("TYP");       };
    char  *a_gattung()     { return attribut("GATTUNG");   };
    char  *a_name()        { return attribut("NAME");      };
    short typ_ist(char *t) { return !mystrcmp(a_typ(), t); };
    STAAT *staat();

    // void Print(); // Zum Debuggen
  void tagebuch_fuehren(char *, char *trenn="");   // Tagebuch fuehren
  char *tagebuch() { return tagebuch_eintraege; }; // Tagebuch abfragen
  void report(const char *, const char *parameter=NULL); // Ueber Objekt Buchfuehren.
  void report(char *, long);		     //	 Diesmal mit %ld statt %s
  void reportliste_drucken(); // Ausdrucken des Reports mittels drucken()
  void reportliste_in_file(FILE *); // Alle Reports in eine Datei.
  short reportliste_leer() { return reportliste.is_empty(); };
  void zur_vernichtung_vormerken() { vernichtenswert=1; };
  short zur_vernichtung_vorgemerkt() { return vernichtenswert!=0; };
  short objekte_laden(FILE *, char *n=NULL);
  void laden_abschliessen();
  short objekte_speichern(FILE *, char *n=NULL);
  void zug(long); // Fuer dieses und alle Unterobjekte naechster_zug()
  void aktion(long); // F.d. u. Unterob. befehl_auswerten() und naechste_phase();
  void zug_ende(long); // Fuer dieses und alle Unterobjekte zug_abschliessen()
  void attribut_setzen(char *k, char *w=NULL) {attribute.setzen(k,w);};
  void attribut_loeschen(char *k)        {attribute.loeschen(k);};
  char *attribut(char *k)         {return attribute.abfragen(k);};
  char **attribut_referenz(char *k) { return attribute.referenz_zu(k);};
  short attribut_gesetzt(char *k) {return attribute.gesetzt(k);};
  short bedingung_erfuellt(char *);
  OBJEKT *objekt_mit_namen(char *);
  void befehl_erteilen(char *);
  void alle_befehle_abbrechen(short modus = 0);
  void in_besitz_nehmen(OBJEKT *);
  void geben(OBJEKT *, OBJEKT *);
  short besitzt(OBJEKT *);
  DOUBLIST *alle_objekte_im_besitz(char *bedingung = NULL);
  DOUBLIST *alle_objekte_im_direkten_besitz(char *bedingung = NULL);
  DOUBLIST *alle_enthaltenen_objekte(char *bedingung = NULL);
  OBJEKT *ort();
  short ort_wechseln(OBJEKT *, ADR& = ADR::ausserhalb());
  short bewegen(ADR &adr); // Adresse des Objektes konform veraendern
  short verlassen(); // Ort und Adresse des eigenen Ortes annehmen
  short einfluss_vorhanden(char *);
  char *einfluss_der_art(char *);
  long einfluss_aufsummieren(char *e)
    {return globale_einfluss_menge.zusammenfassen(this, e, EINFLUSS_SUMME);};
  void einfluesse_suspendieren() // Hebt Wirkung der Einfluesse temporaer auf
     { globale_einfluss_menge.einfluesse_loeschen(this, NULL, 1); };
  void einfluesse_aktivieren()   // Aktiviert Wirkung wieder
     { globale_einfluss_menge.einfluesse_loeschen(this, NULL, 2); };
  char *eindeutiger_name(); // Haengt an den Objektnamen eine Nummer an


  // 6. Funktionen, die von abgeleiten Klassen definiert werden koennen
public:
  virtual short befehl_auswerten(char *, long) { return 1; }; // 1 zurueck, wenn Befehl fertig
  virtual void naechste_phase(long) {}; // Wird jede Runde ausgefuehrt
  virtual void naechster_zug(long) {};  // Wird  jeden Zug ausgefuehrt
  virtual void zug_abschliessen(long) {}; // Wird am Ende jeden Zuges ausgef.
  virtual void abschlussbericht() {}; // Im Prinzip frei belegbar
  virtual short laden(FILE *) { return 0; };
  virtual short speichern(FILE *) { return 0; };
  virtual short kommando(const char *, const void *a=NULL, const void *b=NULL, const void *c=NULL);
  virtual char *info(char *, void *a=NULL, void *b=NULL, void *c=NULL);
  virtual short objekt_aufnehmen(OBJEKT *,
		ADR& = ADR::ausserhalb()) { return 0; };
  virtual void objekt_entlassen(OBJEKT *) {};
  virtual short objekt_bewegen(OBJEKT *, ADR&) { return 0; };
};

inline short OBJEKT::kommando(const char *, const void *, const void *, const void *) { return 1; };
inline char * OBJEKT::info(char *, void *, void *, void *) { return 0; };

class OBJEKT_LIST_NODE : public DOUBLIST_NODE
{
public:
  OBJEKT_LIST_NODE(OBJEKT *o) { objekt=o; };
  OBJEKT_LIST_NODE() {};
  OBJEKT *objekt;
  short matches(void *);
  void Print() { objekt->Print(); };
};


class BEFEHL : public DOUBLIST_NODE
{
public:
  char *befehlstext;
  BEFEHL(char *befehl=NULL) { befehlstext=mystrdup(befehl);};
  ~BEFEHL() { myfree(befehlstext); };
};

class REPORT : public DOUBLIST_NODE
{
public:
  char *text;
  REPORT(char *s=NULL) { text = mystrdup(s); };
  ~REPORT() { myfree(text); };
};


#endif // __objekt_h

