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
  * MODUL:               mystring.C  /  MYSTRING.CPP
  * AUTOR/DATUM:         Mathias Kettner, 2. Juli 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Enthaelt die Funktionen von mystring.h, die im wesentlichen eine
//	Huelle um die klassischen Stringfunktionen strcpy, strcmp... sind,
//	mit dem Unterschied, dass der NULL-Pointer speziell abgefragt wird
//	und als gueltiges Argument fuer die Stringfunktionen zugelassen
//	ist. Die Semantik dieser Erweiterung des Definitionsbereiches der
//	Funktionen ist je nach Funktion unterschiedlich und wird dort defi-
//	niert.
//
//	Darueberhinaus werden noch einige verwandte Funktionen definiert,
//	z.B. eine String-Concatenation-Funktion mit dynamischer Speicher-
//
// **************************************************************************

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "mystring.h"


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
char *mystrdup(const char *string)
{
  if (!string) return NULL;
  else {
    char *ret = new char[strlen(string)+1];
    if (!ret) {
      printf("Out of memory!\n");
      return NULL;
    }
    strcpy(ret, string);
    return ret;
  }
}


/**---------------------------------------------------------------------------
  * mystradd()
  * 
  * Haengt an einen String einen weiteren hintendran. Dies funktioniert
  * aber mit dynamischer Speicherverwaltung. Dazu wird der Funktion
  * die Referenz auf eine char * Variable uebergeben, die anschliessend
  * auf den neu angelegten String zeigt.
  * @param
  * char *& string:		Zeiger auf den Quellstring. Dieser Speicher-
  * bereich wird freigegeben, falls string!=NULL.
  * Am Ende zeigt string auf den Zielstring.
  ---------------------------------------------------------------------------*/
void mystradd(char*& string, const char *add)
{
  if (!add) return;

  if (!string) {
    string = mystrdup(add);
    return;
  }
  
  if (!add[0]) return; // Nichts zum Anhaengen.

  long laenge = strlen(string) + strlen(add) + 1;
  char *neu = new char[laenge];
  if (!neu) {
    printf("Out of memory!\n");
    return;
  }
  strcpy(neu, string);
  strcat(neu, add);

  // Jetzt gebe ich den alten String frei und setze den Zeiger auf den neuen
  myfree(string);
  string = neu;
}


/**---------------------------------------------------------------------------
  * 
  * 
  * @param
  * @return
  ---------------------------------------------------------------------------*/
int mystrcmp(const char *string1, const char *string2)
{
  if (!string1 || !string2) return 1; // Wird als unterschiedlich gewertet.
  else return strcmp(string1, string2);
}


/**---------------------------------------------------------------------------
  * 
  * 
  * @param
  * @return
  ---------------------------------------------------------------------------*/
int mystrncmp(const char *string1, const char *string2, int count)
{
  if (!string1 || !string2) return 1; // Wird als unterschiedlich gewertet.
  else return strncmp(string1, string2, count);
}



/**---------------------------------------------------------------------------
  * 
  * 
  * @param
  * @return
  ---------------------------------------------------------------------------*/
int mystrncmp_no_case(const char *string1, const char *string2, int laenge)
{
  if (!string1 || !string2) return 1; // Wird als unterschiedlich gewertet.
  
  while (*string1 && laenge) {
    if (!*string2) return 1;
    if (tolower(*string1++) != tolower(*string2++)) return 1;
    laenge--;
  }
  if (laenge) return 1;
  return 0;
}


// entfernt alle Weisraeume aus einem String;
// *************************************************************************
// FUNKTION:
// 
// @param
// @return
// *************************************************************************
void mywrapspc(char *string)
{
  if (!string) return;

  char *schreib = string; // Verwende gleichen String als Ziel!
  while (*string) {
    if (isspace(*string)) string++; // Spaces ueberlesen
    else *schreib++ = *string++;    // Andere Zeichen kopieren
  }
  *schreib=0; // String muss noch beendet werden!
}


/**---------------------------------------------------------------------------
  * 
  * 
  * @param
  * @return
  ---------------------------------------------------------------------------*/
void mywrap(char *string, char code)
{
  if (!string) return;
  
  char *schreib = string; // Verwende gleichen String als Ziel!
  while (*string) {
    if (*string == code) string++; // ueberlesen
    else *schreib++ = *string++;    // Andere Zeichen kopieren
  }
  *schreib=0; // String noch beenden!
}


/**---------------------------------------------------------------------------
  * 
  * 
  * @param
  * @return
  ---------------------------------------------------------------------------*/
int mystrlen(const char *string)
{
  if (string) return strlen(string);
  else return 0;
}


/**---------------------------------------------------------------------------
  * 
  * 
  * @param
  * @return
  ---------------------------------------------------------------------------*/
float myatof(const char *string)
{
  if (string) return atof(string);
  else return 0.0;
};


/**---------------------------------------------------------------------------
  * 
  * 
  * @param
  * @return
  ---------------------------------------------------------------------------*/
long myatol(const char *string)
{
  if (!string) return 0;
  else return atol(string);
}

/**---------------------------------------------------------------------------
  * myatol_mit_o()
  * 
  * Ist wie myatol(), betrachtet jedoch kleine und grosse Os als Nuller.
  * Diese Funktion wurde benoetigt, da beim Eintippen oft Nuller und Os
  * verwechselt werden, wenn der Eintipper die Befehlssyntax des Spiels
  * nicht kennt.
  ---------------------------------------------------------------------------*/
long myatol_mit_o(const char *string)
{
  if (!string) return 0;
  char *kopie = mystrdup(string);
  char *z = kopie;
  while (*z) {
    if (*z=='o' || *z=='O') *z = '0';
    z++;
  }
  long rwert = myatol(kopie);
  myfree(kopie);
  return rwert;
}
  

/**---------------------------------------------------------------------------
  * 
  * 
  * @param
  * @return
  ---------------------------------------------------------------------------*/
char *myltoa(long wert)
{
  static short toggler = 0;

  toggler++;
  if (toggler > 3) toggler = 0;

  static char antwort[4][20];
  sprintf(antwort[toggler],"%ld",wert);
  return antwort[toggler];
}



/**---------------------------------------------------------------------------
  * 
  * 
  * @param
  * @return
  ---------------------------------------------------------------------------*/
char *myftoa(float wert, int precision)
{
  static short toggler = 0;

  toggler++;
  if (toggler > 3) toggler = 0;

  static char antwort[4][20];
  if (precision>=0) sprintf(antwort[toggler],"%.*f",precision,wert);
  else sprintf(antwort[toggler], "%f", wert);
  return antwort[toggler];
}


/**---------------------------------------------------------------------------
  * 
  * 
  * @param
  * @return
  ---------------------------------------------------------------------------*/
char *nice_ltoa(long zahl)
{
  static char *zahlentab[13] =
  { L("keine","no"),
    L("eins","one"),
    L("zwei","two"),
    L("drei","three"),
    L("vier","four"),
    L("f~unf","five"),
    L("sechs","six"),
    L("sieben","seven"),
    L("acht","eight"),
    L("neun","nine"),
    L("zehn","ten"),
    L("elf","eleven"),
    L("zw~olf","twelve") };

  if (zahl < 0 || zahl > 12) return myltoa(zahl);
  else return zahlentab[zahl];

}
