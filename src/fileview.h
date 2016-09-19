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
  * MODUL:               filevieh.h
  * AUTOR/DATUM:         Mathias Kettner, 1994
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Funktionen des Fileviewers, mit dem Asciifiles angezeigt werden
//	koennen.
//
// *************************************************************************
#ifndef __fileview_h
#define __fileview_h

#include "doublist.h"
#include "mystring.h"

class FILEVIEWER;

class ZEILE : public  DOUBLIST_NODE
{
  char *zeile;
  ZEILE(char *s) { zeile = mystrdup(s); };
  virtual ~ZEILE() { myfree(zeile); };
  friend class FILEVIEWER;
};

class FILEVIEWER
{
  DOUBLIST zeilenliste;
  ZEILE *aktzeile;
  long xoffset;
  char *dateiname;
  short erste_zeile;  // Bildschirmbereich
  short letzte_zeile; // Bildschirmbereich

public:
  FILEVIEWER(char *, short, short);
  ~FILEVIEWER() { myfree(dateiname); };
  void seitlich_bewegen(long);
  void scroll(long);
  void anfang();
  void ende();
  void drucken();
  void reload();
  void refresh();
  void interact(int);
};

#endif // __fileview_h

