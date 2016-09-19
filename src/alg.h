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
  * MODUL:		alg.h  /  ALG.H
  * AUTOR/DATUM:         Mathias Kettner, 10. November 1993
  * KOMPATIBILITAET:     C++
  ----------------------------------------------------------------------------
  *
  *      Deklariert Funktionen, die sich in keinem Objekt befinden und von
  *	universellem Interesse fuer das ganze Objektsystem sind, unabhaengig
  *	vom konkreten Modell. Dazu gehoeren sowohl vom Objektmodell selbst
  *	unabhaengige Funktionen, z.B. zur Ein- Ausgabe, zum anderen Hilfs-
  *	funktionen fuer das Objektsystem. Die Funktionen sind zum Teil
  *	fuer das Funktionieren des Objektsystems notwendig und wesentlicher
  *	Bestandteil (z.B. vorgemerkte_objekte_vernichten())
  *
  ---------------------------------------------------------------------------*/

#ifndef __alg_h
#define __alg_h

#include <stdio.h>

class OBJEKT;
class DOUBLIST;

// allgemeine IO-Funktionen

short  fputhex(FILE *, char *, long);
short  fgethex(FILE *, char *, long);
short  fputhex(FILE *, unsigned short *, long);
short  fgethex(FILE *, unsigned short *, long);
char  *string_to_wert(char *); // Wandelt \023 Codes wieder zurueck.
void   fputstring(FILE *, char *, char *trenn=NULL); // mit \043 Codes
long   filesize(FILE *file);
short  file_exists(char *filename);
char  *get_file(char *name);
void   verzeichnis_gewaehrleisten(char *);
void   io_deleteline(int,int);

// Druckerfunktionen

short drucker_init(char *redirektfile = NULL);
void  drucker_close();
void  drucken(char *, char *s1, char *s2="", char *s3="");
void  drucken(char *, long l1, long l2=0, long l3=0);
void  drucken(char *s);
void  drucken(char c);
void  drucken_raw(unsigned char *, long);


// Objektsystem

char   *eindeutiger_name();
OBJEKT *objekt_mit_namen(char *);
void    vorgemerkte_objekte_vernichten();
char   *eindeutige_objekt_abkuerzung(char *, short);
void    filtere_objekt_liste(DOUBLIST *, char *pos, char *neg=NULL);
short   adressliste_speichern(FILE *, DOUBLIST *);
short   adressliste_laden(FILE *, DOUBLIST *);

// Makros
#define MAX(c,d) ((c)>(d) ? (c) : (d))
#define MIN(c,d) ((c)<(d) ? (c) : (d))
#define SGN(c)   ((c)>0 ? 1 : ((c)<0 ? -1 : 0))
#define ABS(c)   ((c)<0 ? -(c) : (c))

#endif // __alg_h

