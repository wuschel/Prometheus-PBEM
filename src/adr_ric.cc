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
  * MODUL:		adr_ric.C  /  ADR_RIC.CPP
  * AUTOR/DATUM:		Mathias Kettner, 22. Juli 1993
  * KOMPATIBILITAET:	C++
  ---------------------------------------------------------------------------*/
//
//	Enthaelt die wenigen Funktionen der kleinen Strukturen ADR
//	und RIC.
//
// **************************************************************************

#include <ctype.h>

#include "adr_ric.h"
#include "mystring.h"
#include "alg.h"

ADR& ADR::ausserhalb()
{
  static ADR a; // Wird bei Beginn automatisch richtig initialisiert
  return a;
}

char *ADR::to_string()
{
 static char s[32];
 sprintf(s,"%ld %ld",x,y);
 return s;
}

ADR::ADR(char *string)
{
  if (!*string) {
    x=0;
    y=0;
    return;
  }

  char *komma = string;
  while (*komma && *komma!=',') komma++;
  if (*komma==',') { // Dann gibt's eines, sonst nicht.
    x = atol(string);
    y = atol(komma+1);
  }
  else  sscanf(string, "%ld %ld", &x, &y); // Darstellung mit Spaces.
}


ADR::ADR(FILE *file)
{
  fscanf(file, "%ld %ld", &x, &y);
}


RIC::RIC(char *richtungsstring)
{
  // Die Angabe folgt nach folgender Syntax
  // kommt eine Zahl, so wird diese gespeichert.
  // Kommt einer der Buchstaben N,S,W,O,n,s,w,o, so wird die Richtung um
  // ein Feld je nach Buchstabe geaendert, falls keine Zahl gespeichert war.
  // Ansonsten um den in der Zahl angegebenen Betrag, wonach die Zahl wieder
  // geloescht wird. Kommata werden ignoriert. Beispiele:
  // N  NO  nnOnonwsN  2N,2W  1S,1N2W...

  x=0, y=0; // Zahlen erstmal zuruecksetzen;
  long anzahl=0;

  while (*richtungsstring) {
    if (isalpha(*richtungsstring)) // Gehe von Darstellung 1 aus
    {
      long offset = (anzahl ? anzahl : 1);
      anzahl=0;
      switch(*richtungsstring) {
	case L('n','n'):
	case L('N','N'): y+=offset; break;
	case L('s','s'):
	case L('S','S'): y-=offset; break;
	case L('o','e'):
	case L('O','E'): x+=offset; break;
	case L('w','w'):
	case L('W','W'): x-=offset; break;
      }
    }
    
    else if (isdigit(*richtungsstring)) // Ziffer
      anzahl = anzahl * 10 + *richtungsstring - '0';
        
    richtungsstring++; // Alle anderen Zeichen werden ignoriert.        
  }
}


RIC& RIC::null()
{
  static RIC null("0");
  return null;
}

void RIC::drehen()  // Ist immer um 45 Grad gg. Uhrzeiger.
{
  switch ((x+1)*3 + (y+1)) {
    case 0: x=0; y=-1; break;
    case 1: x=-1; y=-1; break;
    case 2: x=-1; y=0; break;
    case 3: x=1; y=-1; break;
    case 4: x=0; y=0; break;
    case 5: x=-1; y=1; break;
    case 6: x=1; y=0; break;
    case 7: x=1; y=1; break;
    case 8: x=0; y=1; break;
  }
}

void RIC::drehen(float winkel)
{
  // Der Winkel ist im mathematisch positiven Drehsinn und im Gradmass ge-
  // geben. Momentan kann ich nur Richtungen drehen, die 1. nur ein
  // Feld weit gehen und 2. um Vielfache von 45 Grad gedreht werden.

  while (winkel < 0) winkel += 360.0;

  while (winkel > 0) {
    drehen();
    winkel-=45.0;
  }
}


char *RIC::to_string()
{
  static char *tab1[25]=
  {L("SSWW","SSWW"),
   L("SSW","SSW"),
   L("SS","SS"),
   L("SSO","SSE"),
   L("SSOO","SSEE"),
   L("SWW","SWW") ,
   L("SW","SW") ,
   L("S","S") ,
   L("SO","SE") ,
   L("SOO","SEE"),
   L("WW","WW")  ,
   L("W","W")  ,
   L("","") ,
   L("O","E")  ,
   L("OO","EE"),
   L("NWW","NWW") ,
   L("NW","NW") ,
   L("N","N") ,
   L("NO","NE") ,
   L("NOO","NEE"),
   L("NNWW","NNWW"),
   L("NNW","NNW"),
   L("NN","NN"),
   L("NNO","NNE"),
   L("NNOO","NNEE")};

  if (ABS(x)<=2 && ABS(y)<=2) return tab1[x+5*y+12];

  else { // Darstellung wie z.B. 4O,2S oder 6W,1S oder 4O
    static char antwort[32];
    char ns = (y>0 ? L('N','N')
	       : L('S','S'));
    char ow = (x>0 ? L('O','E')
	       : L('W','W'));
    if (x && y) sprintf(antwort,"%ld%c,%ld%c",ABS(x),ow,ABS(y),ns);
    else if (x) sprintf(antwort,"%ld%c",ABS(x),ow);
    else sprintf(antwort,"%ld%c",ABS(y),ns);
    return antwort;
  }
}

char *RIC::grob_to_string()
{
  // Als Ergebnis kommen in Frage N,S,W,O,NW,SW,NO,SO
  int quadrant = (x<0) + 2* (y<0);
  int sektor = (ABS(x)>2*ABS(y)) + 2*(ABS(y)>2*ABS(x));
  static char *tab[4][3] = {
   { L("NO","NE"),
     L("O","E"),
     L("N","N") },
   { L("NW","NW"),
     L("W","W"),
     L("N","N") },
   { L("SO","SE"),
     L("O","E"),
     L("S","S") },
   { L("SW","SW"),
     L("W","W"),
     L("S","S") }
  };
  
  return tab[quadrant][sektor];
}
