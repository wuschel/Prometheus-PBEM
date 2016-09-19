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
  * MODUL:               attribut.h / ATTRIBUT.H
  * AUTOR/DATUM:         Mathias Kettner, 28. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Definiert die Klassen ATTRIBUT_LISTE und ATTRIBUT.
//
// **************************************************************************

#ifndef __attribut_h
#define __attribut_h

#include <stdio.h>

#include "doublist.h"
#include "mystring.h"


struct ATTRIBUT : public DOUBLIST_NODE
{
    char *klasse;
    char *wert;

public:
    ATTRIBUT(char *k, char *w=NULL);
    ~ATTRIBUT();
    friend class ATTRIBUT_LISTE;
    short matches(void *);
};

class ATTRIBUT_LISTE : public DOUBLIST
{
    char  *typ; // Dieses Attribut wird gecached!
public:
    ATTRIBUT_LISTE(char *s) { typ=NULL; aus_string_einlesen(s); };
    ATTRIBUT_LISTE()        { typ=NULL; };
    void   setzen(char *, char *wert=NULL);
    void   loeschen(char *);
    char  *abfragen(const char *);
    char **referenz_zu(char *);
    short  gesetzt(char *);
    void   aus_string_einlesen(char *);
    void   wert_to_string(char *, char *);
    char  *to_string(char *trenn=NULL);
    short  in_file_ausgeben(FILE *);
    short  passen_in(ATTRIBUT_LISTE *);
    void   subtrahiere(ATTRIBUT_LISTE *);
    char  *erstes_attribut() { return ((ATTRIBUT *)first())->klasse; };
};

#endif // __attribut_h

