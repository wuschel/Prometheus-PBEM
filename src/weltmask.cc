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
  * MODUL:		weltmask.C  /  WELTMASK.CPP
  * AUTOR/DATUM:		Mathias Kettner, 20. Juli 1993
  * KOMPATIBILITAET:	C++
  ---------------------------------------------------------------------------*/
//
//	Routinen fuer die Eingabemaske zur Welterschaffung
//
// *************************************************************************

#include "maske.h"
#include "uhr.h"
#include "attribut.h"
#include "kompatib.h"

/**---------------------------------------------------------------------------
  * UHR::attribute_fuer_welterschaffung()
  * 
  * Stellt im Datenbereich des Bildschirms eine Maske dar und fragt
  * so den Benutzer nach den Daten fuer die Erschaffung einer Welt.
  * Hier wird auch der Genesis III Algorithmus mit Parametern ver-
  * sorgt.
  *
  * @return
  * char *   Attributstring, der direkt dem Konstruktor des Objektes
  * WELT uebergeben werden kann. Durch die Attribute
  * wird alles Konfiguriert.
  ---------------------------------------------------------------------------*/
char *UHR::attribute_fuer_welterschaffung()
{
  // Im ersten Schritt baue ich eine Maske auf.
  MASKE maske(UHR::weltmaske_refresh);
  char *rwert;

  const short oben = Y_DATENBEREICH;
  do { // lokaler Block

    // Welterschaffung
    NUMFELD  breite(&maske, 42,oben+2,4,180);
    NUMFELD  hoehe(&maske, 64,oben+2,4,52);
    TEXTFELD gelaendedat(&maske, 25, oben+3, 24,NULL,"bin/gelaende.dat");
    NUMFELD  keimdichte(&maske, 55,oben+4,4,45);

    maske.refresh();

    // So. jetzt kann editiert werden...
    if (maske.edit()) { // Werte uebernommen
      ATTRIBUT_LISTE attr;
      attr.setzen("Diagonal"); // Ist unbedingt notwendig.
      attr.setzen("NAME","ERDE" );
      attr.setzen("BREITE",breite.charwert());
      attr.setzen("HOEHE",hoehe.charwert());
      attr.setzen("G3_KEIMDICHTE",keimdichte.charwert());
      
      rwert = attr.to_string();
    }
    else rwert = NULL;

    // Und jetzt entferne ich alle Felder wieder aus der Maske, damit
    // sie nicht zweimal freigegeben werden...
    while (!maske.is_empty()) maske.first()->remove();

  } while (0);
  
  return rwert;
}

/**---------------------------------------------------------------------------
  * UHR::weltmaske_refresh()
  * 
  * Baut den Bildschirm fuer die Eingabemaske der Welt auf.
  ---------------------------------------------------------------------------*/
void UHR::weltmaske_refresh()
{
  io_deleteline(Y_DATENBEREICH, Y_DATENBEREICHENDE);
  const short oben = Y_DATENBEREICH;

  io_printxy(0,oben,L("Formung des Gelaendes","Formung des Gelaendes"));

  io_printxy(0,oben+2,L("Die Welt soll                             ____ Felder breit und ____ hoch sein.","Die Welt soll                             ____ Felder breit und ____ hoch sein."));
  io_printxy(0,oben+3,L("Die Gelaendedatei heisst ________________________. Je mehr Keime es gibt, desto","Die Gelaendedatei heisst ________________________. Je mehr Keime es gibt, desto"));
  io_printxy(0,oben+4,L("bizarrer wird die Welt. Bei der neuen Welt soll auf je ____ Felder ein Keim ko-","bizarrer wird die Welt. Bei der neuen Welt soll auf je ____ Felder ein Keim ko-"));
  io_printxy(0,oben+5,L("mmen.","mmen."));


  io_deleteline(oben+16); // Kann eventuell ueberschrieben gewesen sein.
  io_printxy(7,oben+16,L("Phase 1:......","Phase 1:......"));
  io_printxy(29,oben+16,L("Phase 2:.......","Phase 2:......."));
  io_printxy(53,oben+16,L("Versuche:.......","Versuche:......."));

  io_printxy(58,oben+6,L("CTRL-E Welt schaffen","CTRL-E Welt schaffen"));
  io_printxy(58,oben+7,L("CTRL-A Abbrechen","CTRL-A Abbrechen"));
}
