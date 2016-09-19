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
  * MODUL:               fileview.C  /  FILEVIEW.CPP
  * AUTOR/DATUM:         Mathias Kettner, 28. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Enthaelt die Funktionen des Fileviewers, das heisst der Klasse
//	FILEVIEWER.
//
// *************************************************************************

#include <stdio.h>
#include <string.h>

#include "fileview.h"
#include "kompatib.h"
#include "alg.h"
#include "mystring.h"


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
FILEVIEWER::FILEVIEWER(char *filename, short ezeile, short lzeile)
{
  erste_zeile = ezeile;
  letzte_zeile = lzeile;
  xoffset = 0;
  dateiname = mystrdup(filename);
  aktzeile = NULL;
  reload();
}

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void FILEVIEWER::reload()
{
  zeilenliste.clear();
  aktzeile = NULL;

  FILE *file = fopen(dateiname,"r");
  if (!file) return;

  char *zeile = new char[4098];
  while (!feof(file))
  {
    fgets(zeile, 4096, file);
    while(zeile[strlen(zeile)-1] == 13 || zeile[strlen(zeile)-1] == 10)
       zeile[strlen(zeile)-1] = 0;
       
    if (ferror(file) || feof(file)) break;
    ZEILE *neu = new ZEILE(zeile);
    zeilenliste.add_tail(neu);
  }
  delete zeile;
  fclose(file);

  aktzeile = (ZEILE *)zeilenliste.first();
  refresh();
}


/**---------------------------------------------------------------------------
  * FILEVIEWER::refresh()
  * 
  * Dies ist die Funktion, die das File tatsaechlich auf dem Bildschirm
  * ausgiebt. Der Fileviewer ist relativ einfach konzipiert: Es gibt
  * kein Scrolling sondern der Ausschnitt wird stets komplett neu auf-
  * gebaut.
  ---------------------------------------------------------------------------*/
void FILEVIEWER::refresh()
{
  io_deleteline(erste_zeile, letzte_zeile);

  ZEILE *zeile = aktzeile;
  if (!zeile) return; // Dann ist gar kein File geladen!

  for (int z=erste_zeile; !zeile->is_tail() && z<=letzte_zeile; z++)
  {
    char string[81]; // Vorbereitung fuer die Ausgabe.

    // Jetzt muss ich den Quellstring hineinkopieren. Dabei habe ich
    // das Problem der Tabulatoren. Ich muss sie stets mitzaehlen und
    // durch Spaces ersetzen. Jeder Tabulator steuert eine durch 8 teil-
    // bare Stelle an.
      
    int spalte=0, spaces_ausstehend = 0;
    char *read = zeile->zeile, *write = string;
      
    while (spalte < 80+xoffset && *read) {
      char c;
      if (spaces_ausstehend) {
        c = ' ';
        spaces_ausstehend--;
      }
      else c = *read++;

      // Wenn ein Tabulator kommt, dann merke ich mir die Spaces.
      if (c == '\t') spaces_ausstehend = 8 - spalte%8;
          
      // Ansonsten gebe ich aus, wenn ich schon im sichtbaren bin.
      else if (spalte++ >= xoffset) *write++ = c;
    }
    *write = 0; // String noch beenden.
   
    io_printxy(0,z,string);
    zeile = (ZEILE *)zeile->next();
  }
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void FILEVIEWER::drucken()
{
  io_init_printer();
  ZEILE *zeile = (ZEILE *)zeilenliste.first();
  while (!zeile->is_tail()) {
    io_printer(zeile->zeile);
    zeile = (ZEILE *)zeile->next();
  }
  io_close_printer();
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void FILEVIEWER::anfang()
{
  aktzeile = (ZEILE *)zeilenliste.first();
  refresh();
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void FILEVIEWER::ende()
{
  aktzeile = (ZEILE *)zeilenliste.last();
  if (aktzeile->is_head()) { // Leer!
    aktzeile = (ZEILE *)zeilenliste.first();
    refresh();
    return;
  }

  for (short z=letzte_zeile; z > erste_zeile; z--)
  {
    if (!aktzeile->is_first()) aktzeile = (ZEILE *)aktzeile->previous();
  }
  refresh();
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void FILEVIEWER::seitlich_bewegen(long aenderung)
{
  long alt = xoffset;
  if (!aenderung) xoffset = 0;
  else xoffset += aenderung;
  if (xoffset < 0)  xoffset = 0;
  if (xoffset != alt) refresh();
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void FILEVIEWER::scroll(long offset)
{
  if (offset>=0) { // Scroll down.
    for (;offset;offset--) {
      if (!aktzeile->is_tail()) aktzeile = (ZEILE *)aktzeile->next();
    }
  }
  else { // Scroll up
    for (;offset;offset++) {
      if (!aktzeile->is_first()) aktzeile = (ZEILE *)aktzeile->previous();
    }
  }
  refresh();
}

/**---------------------------------------------------------------------------
  * FILEVIEWER::interact()
  * 
  * Der Benutzer kann mit verschiedenen Tasten den sichtbaren Ausschnitt
  * beeinflussen und mit einer bestimmten Taste den interaktiven Modus
  * wieder verlassen. 
  ---------------------------------------------------------------------------*/
void FILEVIEWER::interact(int endetaste)
{
  while (1)
  {
    int taste = io_getch();
    if (taste == endetaste) return;
    else if (io_iscursorup(taste))  scroll(-1);
    else if (io_iscursordown(taste))  scroll(1);
    else if (io_iscursorright(taste))  seitlich_bewegen(8);
    else if (io_iscursorleft(taste))  seitlich_bewegen(-8);
    else if (io_ispagedown(taste))  scroll(letzte_zeile - erste_zeile - 1);
    else if (io_ispageup(taste))  scroll(-letzte_zeile + erste_zeile + 1);
    else if (io_ishome(taste)) anfang();
    else if (io_isend(taste)) ende();
  }    
}
