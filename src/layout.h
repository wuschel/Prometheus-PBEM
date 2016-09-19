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
  * MODUL:		layout.h  /  LAYOUT.H
  * AUTOR/DATUM:		Mathias Kettner, 11. Juli 1993
  * KOMPATIBILITAET:	C++
  ---------------------------------------------------------------------------*/
//
//	Definiert Klassen, die ein Layout aus Schrift, Vektoren und
//	Bitgraphik beschreiben.
//
//	Einige Konventionen:
//
//	1. Alle Laengeangaben sind in cm und vom Typ float.
//
// **************************************************************************

#ifndef __layout_h
#define __layout_h

#include <stdio.h>

#include "doublist.h"
#include "mystring.h"
#include "string.h"

class LAYOUT;

struct OFFSET
{
  float x,y;
  OFFSET() { x=0; y=0; };
  OFFSET(float x0, float y0) { x=x0; y=y0; };
  void addiere(OFFSET& o) { x+=o.x; y+=o.y; };
  void subtrahiere(OFFSET& o) { x-=o.x; y-=o.y; };
};


class LAYOUT_ELEMENT : public DOUBLIST_NODE
{
public:
  OFFSET offset; // Ist z.B. Kreismittelpunkt oder Linienanfang
  virtual void ausdrucken(float, float) {};
};


class LAYOUT : public DOUBLIST
{
public:
  OFFSET offset; // Auch hier kann noch einmal ein Offset vorhanden sein.
  float breite; // Breite des Papieres in cm
public:
  LAYOUT(float o=0) { breite = 19.80; offset.y=o; };// DIN A4 mit Rand
  void text(float x, float y, char *text, char *schriftart, float pt);
  void linie(float links, float oben, float rechts, float unten);
  void trennlinie(float oben); // Waagerechte Trennlinie
  void bitmatrix(float links, float oben, char *filename, float scale=1);
  void torte(float l, float o, float r, float *e, short a);
  void rechteck(float l, float o, float r, float u);
  void rechteck_ausgefuellt(float l, float o, float r, float u, short s);
  void ueberschrift1(char *);
  float links() { return offset.x; };
  float oben() { return offset.y; };

  void view(); // Auf dem Bildschirm anschauen ?!
  void ausdrucken(float l=0, float o=0); // Druckt das Ganze auf dem Drucker aus.
};

class SCHRIFTZUG : public LAYOUT_ELEMENT
{
  float pt;
  char *zeichensatz;
  char *text;
public:
  SCHRIFTZUG(char *t, char *z="Courier", float s=10);
  virtual ~SCHRIFTZUG() { myfree(text); myfree(zeichensatz); };
  void ausdrucken(float l=0, float o=0);
};


class LINIE : public LAYOUT_ELEMENT
{
  OFFSET endpunkt; // Relativ zu (0,0)
public:
  LINIE(OFFSET& s, OFFSET& e)
    { offset = s; endpunkt = e; endpunkt.subtrahiere(s); };
  ~LINIE() {};
  void ausdrucken(float l=0, float o=0);
};


class BITMATRIX : public LAYOUT_ELEMENT
{
  long breite,hoehe; // Angaben in Pixel!
  float scale; // Vergroesserungsfaktor
  unsigned char *daten;
  static DOUBLIST history;
  char *dateiname; // Zeigt auf History
public:
  BITMATRIX(OFFSET&, char *, float sc=1); // Laden aus einer Datei
  ~BITMATRIX() {}; // Daten gehoeren der HISTORY
  void ausdrucken(float l=0, float o=0);
};

class BITMATRIX_HISTORY : public DOUBLIST_NODE
{
public:
  char *name;
  long breite,hoehe;
  unsigned char *daten;
public:
  BITMATRIX_HISTORY(long b, long h, char *n, unsigned char *d)
     { breite = b; hoehe = h, name = mystrdup(n); daten = d; };

  ~BITMATRIX_HISTORY() { if (daten) delete daten; myfree(name); };

  short matches(void *k) { return !strcmp((char *)k,name); };
};


class TORTE : public LAYOUT_ELEMENT
{
  float radius;
  float *eintraege; // In Anteilen. Wird automatisch auf 1 normalisiert
  short anzahl;
public:
  TORTE(OFFSET& m, float r, float *e, short a);
  ~TORTE() { delete eintraege; };
  void ausdrucken(float l=0, float o=0);
};

class RECHTECK_AUSGEFUELLT : public LAYOUT_ELEMENT
{
  OFFSET rechts_unten; // Relativ zum offset
  short schattierung;
public:
  RECHTECK_AUSGEFUELLT(OFFSET& lo, OFFSET &ru, short s);
  void ausdrucken(float l=0, float o=0);
};

#endif // __layout_h

