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
  * MODUL:               layout.C  /  LAYOUT.CPP
  * AUTOR/DATUM:         Mathias Kettner, 12. Juli 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Funktionen zu den Klassen LAYOUT, SCHRIFTZUG, LINIE
//      und BITMATRIX, die nicht druckerspezifisch sind.
//
// **************************************************************************

#include <ctype.h>
#include <string.h>

#include "objekt.h"
#include "mystring.h"
#include "layout.h"
#include "drucker.h"
#include "alg.h"
#include "uhr.h"
#include "log.h"


extern UHR *g_uhr; // von main.cpp

DOUBLIST BITMATRIX::history;

TORTE::TORTE(OFFSET& m, float r, float *e, short a)
{
  offset = m;
  radius = r;
  anzahl = a;
  eintraege = new float[a];
  for (short i=0; i<a; i++) eintraege[i] = e[i];
}

SCHRIFTZUG::SCHRIFTZUG(char *t, char *zs, float p)
{
  text = mystrdup(t);
  dr_umlaute_ersetzen(text);
  zeichensatz = mystrdup(zs);
  pt = p;
}

RECHTECK_AUSGEFUELLT::RECHTECK_AUSGEFUELLT
		(OFFSET& lo, OFFSET& ru, short s)
{
  offset = lo;
  rechts_unten = ru;
  rechts_unten.subtrahiere(lo); // Relative Angabe
  schattierung = s;
}


BITMATRIX::BITMATRIX(OFFSET& lo, char *filename, float sc)
{
  if (!filename || !filename[0]) return; // Ohne File keine Grafik!
  
  offset = lo;
  scale = sc;

  // Zuerst durchsuche ich die historyliste.
  BITMATRIX_HISTORY *bm =
	 (BITMATRIX_HISTORY *)history.find((void *)filename);

  // Falls schon ein Eintrag existiert, muss ich nicht nochmal laden...
  if (bm) {
    breite = bm->breite;
    hoehe = bm->hoehe;
    daten = bm->daten;
    dateiname = bm->name;
    return;
  }

  // ACHTUNG!: Funktioniert bis jetzt nur mit breiten, die durch 8
  // teilbar sind!

  // Vor den Dateinamen haenge ich noch den Pfadnamen fuer die Grafiken.
  // diesen hole ich mir von der Uhr. Damit ich das nicht jedes mal neu
  // machen muss, merke ich ihn mir in einer statischen Variablen.

  char *pfad = g_uhr->grafikdateiname(filename);

  daten = NULL;
  FILE *file = fopen(pfad, "r");
  if (!file) {
      log('K', "Can't open graphics file '%s'", pfad);
      return;
  }

  // Momentan gehe ich von einer .gra Datei im Mathi-Format aus.
  fscanf(file,"%ld%ld",&breite, &hoehe);
  if (!breite || !hoehe) {
      log('K', "Graphics file '%s' has invalid dimensions %ld x %ld",
	  pfad, breite, hoehe);
      fclose(file);
      return;
  }
  
  if (breite % 8) breite = (breite/8)*8 + 1; // Breite muss durch 8 teilbar s.

  // Jetzt lege ich die Matrix an.
  long anzahlbytes = (breite*hoehe)/8;
  daten = new unsigned char [anzahlbytes];

  // Nun lese ich ein. Jedes Space ueberlese ich, der Schlusspunkt zaehlt
  // als 0, der Rest als 1
  unsigned char *datenzeiger = daten;
  unsigned char byte;

  for (long bytezaehler=0; bytezaehler < anzahlbytes; bytezaehler++)
  {
    byte = 0;
    for (int bit=7; bit >=0; bit--) {
      char c;
      do c = fgetc(file);
      while (isspace(c));
      if (c != '.') byte |= 1 << bit;
    }
    *datenzeiger++ = byte;
  }
  fclose(file);

  // So. Und nun lege ich noch einen Historyeintrag an, damit ich beim
  // naechsten mal nicht schon wieder alles Laden muss..

  bm = new BITMATRIX_HISTORY(breite, hoehe, filename, daten);
  if (bm) {
    history.insert(bm);
    dateiname = bm->name;
  }
  else dateiname = NULL;
  
}

void LAYOUT::torte(float x, float y, float r, float *e, short a)
{
  OFFSET mitte(x, y);
  TORTE *torte = new TORTE(mitte, r, e, a);
  insert(torte);
}


void LAYOUT::text(float x, float y, char *text, char *schriftart, float pt)
{
  SCHRIFTZUG *schrift = new SCHRIFTZUG(text, schriftart,  pt);
  schrift->offset = OFFSET(x,y);
  insert(schrift);
}

void LAYOUT::linie(float links, float oben, float rechts, float unten)
{
  OFFSET lo(links, oben);
  OFFSET ru(rechts, unten);
  LINIE *linie = new LINIE(lo, ru);
  insert(linie); // Das offset im ELEMENT bleibt auf 0.
}

void LAYOUT::trennlinie(float oben)
{
  linie(0, oben, breite, oben);
}

void LAYOUT::bitmatrix(float links, float oben, char *filename, float sc)
{
  OFFSET lo(links, oben);
  BITMATRIX *bm = new BITMATRIX(lo, filename, sc);
  insert(bm);
}


void LAYOUT::rechteck(float l, float o, float r, float u)
{
  linie(l,o,r,o);
  linie(r,o,r,u);
  linie(r,u,l,u);
  linie(l,u,l,o);
}

void LAYOUT::rechteck_ausgefuellt(float l, float o, float r, float u, short s)
{
  OFFSET lo(l, o);
  OFFSET ru(r, u);

  RECHTECK_AUSGEFUELLT *ra = new RECHTECK_AUSGEFUELLT(lo, ru, s);
  insert(ra);
}

void LAYOUT::ueberschrift1(char *beschriftung)
{
  float h = 0.7;
  rechteck_ausgefuellt(0,0.1,breite,0.1+h,15);
  rechteck(0,0.1,breite,0.1+h);
  text((breite-strlen(beschriftung)*0.29)/2,0.68,beschriftung,"Univers",17);
}


void LAYOUT::ausdrucken(float links, float oben)
{
  LAYOUT_ELEMENT *le = (LAYOUT_ELEMENT *)first();
  while (!le->is_tail())
  {
    le->ausdrucken(links+offset.x, oben+offset.y);
    le = (LAYOUT_ELEMENT *)le->next();
  }
}
