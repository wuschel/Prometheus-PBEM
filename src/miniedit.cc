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
  * MODUL:               miniedit.C  /  MINIEDIT.CPP
  * AUTOR/DATUM:         Mathias Kettner, 28. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Enthaelt die Funktionen der Klasse MINIEDITOR, die einen 
//	kleinen Texteditor darstellt, bei dem jedoch nicht gescrollt
//	werden kann. Es kann also maximal eine Bildschirmseite editiert
//	werden. Er stuetzt sich auf die Klassen TEXTFELD und MASKE.
//
// *************************************************************************

#include "miniedit.h"
#include "kompatib.h"
#include "alg.h"

MINIEDITOR::MINIEDITOR(char *fn, short l, short o, short b, short h)
{
  filename = mystrdup(fn);
  links = l;
  oben = o;
  breite = b;
  hoehe = h;
  zeilen = new char*[hoehe];
  if (zeilen) for (int i=0; i<hoehe; i++) zeilen[i]=NULL;

  einlesen();
  maske_aufbauen();
}

MINIEDITOR::~MINIEDITOR()
{
  if (zeilen) {
    for (int i=0; i<hoehe; i++) if (zeilen[i]) delete zeilen[i];
    delete zeilen;
  }
  myfree(filename);
}


void MINIEDITOR::einlesen()
{
  if (!zeilen) return; // Kein Speicher im Konstruktor

  FILE *file = fopen(filename,"r");
  if (!file) return;

  for (int i=0; i<hoehe && !feof(file); i++)
  {
     zeilen[i] = new char[breite+5];
     if (!zeilen[i]) break;

     if (0 > fgets(zeilen[i], breite+2, file)) break;
     if (zeilen[i][strlen(zeilen[i])-1] == '\n')
		 zeilen[i][strlen(zeilen[i])-1] = 0; // LF am Ende entfernen.
  }
  fclose(file);
}


void MINIEDITOR::maske_aufbauen()
{
  if (!zeilen) return;  // Fehler

  for (int i=0; i<hoehe; i++)
     new TEXTFELD(&maske, links, oben+i, breite, &zeilen[i]);
}


short MINIEDITOR::edit()
{
  short rwert = maske.edit();
  if (rwert == EDIT_END) zurueckschreiben();
  return rwert;
}

void MINIEDITOR::zurueckschreiben()
{
  verzeichnis_gewaehrleisten(filename);
  FILE *file = fopen(filename, "w");
  if (file) {
    for (int i=0;i<hoehe;i++) if (zeilen[i]) fprintf(file,"%s\n",zeilen[i]);
    fclose(file);
  }
  else {
    io_printxy(links, oben, L("Fehler beim Schreiben der Datei!!! ","Write error !!! "));
    io_printxy(links, oben, L("Datei nicht geschrieben. (Taste) ","File(s) not saved  (Press a key) "));
    io_getch();
    return;
  }
}

