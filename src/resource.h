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
  * MODUL:               resource.h / RESOURCE.H
  * AUTOR/DATUM:         Mathias Kettner, 1. Mai 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
// 
//      Definiert die Struktur RESOURCE_VEKTOR und zugehoerige Typen 
//      und Konstanten.
// 
// **************************************************************************

#ifndef __resource_h
#define __resource_h

#include <stdlib.h>

struct RESOURCE_VEKTOR
{
  long *lager;
  long anzahl;
public:
  RESOURCE_VEKTOR(char *string=NULL) { init(); setzen_auf(string); };
  RESOURCE_VEKTOR(const RESOURCE_VEKTOR& v) { init(); setzen_auf(v); };
  RESOURCE_VEKTOR(long, char);
  ~RESOURCE_VEKTOR() { if (lager) delete lager; };
  void init(); // Speicher anlegen
  void setzen_auf(const RESOURCE_VEKTOR&);
  void setzen_auf(char *);
  void einheitsvektor_aus(char *);
  void addiere(const RESOURCE_VEKTOR&);
  void subtrahiere(const RESOURCE_VEKTOR&);
  void multipliziere_mit(long);
  void multipliziere_mit_float(float);
  void teile_durch(long);
  void negieren();
  void kompensieren_aus(RESOURCE_VEKTOR&);
  void begrenzen_auf(RESOURCE_VEKTOR&);
  void begrenzen_auf(long);
  long betrag(); // Summe aller Komponenten
  long hemmingnorm(); // Anzahl der Komponenten != 0
  int passt_in(RESOURCE_VEKTOR&);
  int ist_null();
  short ist_negativ(); // 1, wenn mindestens eine Komponente negativ ist.
  char *to_string();
  long& operator [] (char);
};

#endif // __resource_h

