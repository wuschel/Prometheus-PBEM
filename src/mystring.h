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


#ifndef __mystring_h
#define __mystring_h

#include <ctype.h>

char *mystrdup(const char *);
inline void myfree(char *s) { if (s) delete s; };
void mystradd(char*&, const char *); // Mit dynamischer Speicherumschichtung!
int mystrcmp(const char *, const char *); // Mit Abfang von NULL-Zeigern.
int mystrncmp(const char *, const char *, int); // Ebenfalls mit Abfang von NULL-Zeigern.
int mystrcmp_no_case(const char *, const char *);
int mystrncmp_no_case(const char *, const char *, int);
void mywrapspc(char *); // Entfernt alle isspace()
void mywrap(char *, char); // Entfernt jedes Auftreten v. bestimmtem Zeichen
int mystrlen(const char *); // Liefert 0, wenn string = NULL
long myatol(const char *); //  Liefert 0, wenn string = NULL
long myatol_mit_o(const char *); // wie atol, Os werden aber als Nuller betrachtet
float myatof(const char *s); // Liefert 0.0, wenn string = NULL
char *myltoa(long);  // Belegt statischen Speichern selbst
char *myftoa(float, int prec=-1); // Wie myltoa
char *nice_ltoa(long); // wie myltoa, aber fuer kleine Zahlen Text!

// Inline, aus Effizienzgruenden.

/**---------------------------------------------------------------------------
  * mystrcmp_no_case()
  * 
  * Vergleicht zwei Strings. Dabei wird nicht zwischen Klein- und Gross
  * schreibung unterschieden. Ausserdem werden zwei NULL-Pointer als
  * unterschiedlich betrachtet!
  * @return
  * Immer 1, falls ungleich, sonst 0.
  ---------------------------------------------------------------------------*/
inline int mystrcmp_no_case(const char *s1, const char *s2)
{
  register const char *string1 = s1;
  register const char *string2 = s2;
    
  if (!string1 || !string2) return 1; // Wird als unterschiedlich gewertet.

  while (*string1) {
    if (tolower(*string1++) != tolower(*string2++)) return 1;
  }

  if (*string2) return 1;
  else return 0;
}


#endif // __mystring_h

