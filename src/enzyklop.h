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
// MODUL:               enzyklop.h  /  ENZYKLOP.H
// AUTOR/DATUM:         Mathias Kettner, 2. Juli 1993
// KOMPATIBILITAET:     C++
// **************************************************************************
//
//	Deklariert die Klassen, die mit der Enzyklopaedie zu tun haben.
//	Dies sind EINFLUSS_ENZ, LEXIKON_ENZ, STAATSFORM_ENZ,
//	VORHABEN_ENZ, EINHEIT_ENZ und vor allem ENZYKLOPAEDIE.
//
// **************************************************************************

#ifndef __enzyklop_h
#define __enzyklop_h

#include <string.h>
#include <ctype.h>

#include "laengen.h"
#include "staat.h"
#include "resource.h"

struct EINFLUSS_ENZ : DOUBLIST_NODE
{
    char *art;
    char *ziel;
    char *par;
    // Beachte, dass der Konstruktor NICHT kopiert!
    EINFLUSS_ENZ(char*a, char *z, char *p) { art=a; ziel=z; par=p; };
    ~EINFLUSS_ENZ() { myfree(art); myfree(ziel); myfree(par);};
};

class LEXIKON_ENZ : DOUBLIST_NODE
{
    char *name;
    long offset; // Im File
    
public:
    LEXIKON_ENZ(char *, long); // Zeilenpuffer, offset im File
    ~LEXIKON_ENZ() { myfree(name); };
    short matches(void *k) { return !strcmp((char *)k, name); };
    friend class ENZYKLOPAEDIE;
};


struct STAATSFORM_ENZ : DOUBLIST_NODE
{
    char *name;
    DOUBLIST einfluesse;
    
public:
    ~STAATSFORM_ENZ() { myfree(name); };
    short matches(void *m) { return (!strcmp(name, (char *)m)); };
};

struct VORHABEN_ENZ : DOUBLIST_NODE
{
    char *name;
    RESOURCE_VEKTOR benoetigte_resourcen;
    RESOURCE_VEKTOR unterhalt;
    ATTRIBUT_LISTE voraussetzungen_in_der_stadt;
    ATTRIBUT_LISTE voraussetzungen_im_staat;
    long max_anzahl_in_einer_stadt;
    long max_anzahl_in_einem_staat;
    ATTRIBUT_LISTE start_attribute;
    char *einfluss_art;
    char *einfluss_spez;
    char *einfluss_parameter;
    char *info_datei_filename;
    
    ~VORHABEN_ENZ();
    short matches(void *);
    char *abk() { return start_attribute.abfragen("ABK"); };
};

struct VORHABEN_ENZ_SORT_INFO
{
    char *name_oder_abk;
    STAAT *staat;
    VORHABEN_ENZ_SORT_INFO(char *n, STAAT *s=NULL) { name_oder_abk=n; staat=s; };
};

struct RESOURCE_ENZ : DOUBLIST_NODE
{
    char name[MAX_LAENGE_RESOURCENAME];
    char symbol;
    short matches(void *m) {
	char *s=(char *)m;
	if (s[1]) return (!mystrcmp(name, (char *)m));
	else return toupper(s[0]) == symbol;
    };
};

struct EINHEIT_ENZ : DOUBLIST_NODE
{
    char *name;
    long angriffskraft;
    long verteidigungskraft;
    long bewegungspunkte_pro_feld;
    float sichtweite;
public:
    ~EINHEIT_ENZ() { myfree(name); };
    short matches(void *m) { return (!mystrcmp(name, (char *)m)); };
};


struct ENZYKLOPAEDIE : public OBJEKT
{
    DOUBLIST vorhabenliste;
    DOUBLIST resourcenliste;
    DOUBLIST einheitenliste;
    DOUBLIST staatsformenliste;
    DOUBLIST lexikonliste;
    
    long anzahl_verschiedener_resourcen;
    bool i_am_ok;
public:
    ENZYKLOPAEDIE(char *, char *);
    bool ok() const { return i_am_ok; };
    
    VORHABEN_ENZ *vorhaben(char *n, STAAT *staat=NULL);
    RESOURCE_ENZ *resource(char *n)
	{ return (RESOURCE_ENZ *)resourcenliste.find((void *)n); };
    RESOURCE_ENZ *resource(char n);
    EINHEIT_ENZ *einheit(char *n)
	{ return (EINHEIT_ENZ *)einheitenliste.find((void *)n); };
    STAATSFORM_ENZ *staatsform(char *n)
	{ return (STAATSFORM_ENZ *)staatsformenliste.find((void *)n); };
    char *lexikon_eintrag(char *name, short nummer);
    char *infotext_zusatzinfo(char *);
    char *info_grafik(char *);
    short welche_gattung_kommt_zuerst(char *gattung1, char *gattung2);
    void  vorhaben_mit_link(HTML& html, char *vorhaben);
    
private:
  void lexikon_laden(char *, FILE *);
  void vorhaben_laden(char *,FILE *);
  void resourcen_laden(char *,FILE *);
  void einheiten_laden(char *,FILE *);
  void staatsformen_laden(char *, FILE *);
  long hole_zeile(char *, long, FILE *);
  char *hole_wort_aus_string(char*& string);
  long hole_long_aus_string(char*& string);

};

#endif // __enzyklop_h

