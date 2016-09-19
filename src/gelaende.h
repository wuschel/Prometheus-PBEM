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
  * MODUL:               gelaende.h / GELAENDE.H
  * AUTOR/DATUM:         Mathias Kettern, 1. Mai 1993
  * KOMPATIBILITAET:     C++
  -----------------------------------------------------------------------------
  * 
  *      Enthaelt die Klasse GELAENDE_FORM und GELAENDE_FORM_TABELLE.
  *      Eine Gelaendeform ist eine Beschreibung fuer den Typus eines Spiel-
  *      feldes innerhalb einer MATRIX-Landschaft. Jedes einzelne Feld besitzt
  *      genau eine Gelaendeform. Abgesehen davon koennen fuer das Feld dann
  *      noch verschiedene Feldattribute gesetzt oder geloescht sein.
  *
  ---------------------------------------------------------------------------*/

#ifndef __gelaende_h
#define __gelaende_h

#include "attribut.h"


struct GELAENDE_FORM
{
  ATTRIBUT_LISTE attribute;
};


class GELAENDE_FORM_TABELLE
{
    long anzahl;
    GELAENDE_FORM **tabelle;
    bool i_am_ok;
    
public:
    GELAENDE_FORM_TABELLE();
    virtual ~GELAENDE_FORM_TABELLE();
    bool ok() const { return i_am_ok; };
    
    long get_anzahl() { return anzahl; };
    char *attribut_fuer_form(short, char *);
    short attribut_gesetzt_fuer_form(short, char *);
    short form_mit_namen(char *);
    short form_mit_abkuerzung(char *);
    GELAENDE_FORM *form_mit_nummer(short f) { return tabelle[f]; };
};

#endif // __gelaende_h

