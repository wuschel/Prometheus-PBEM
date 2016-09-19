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
  * MODUL:               mat_alta.C  /  MAT_ATLA.CPP
  * AUTOR/DATUM:         Mathias Kettner, 25. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Funktionen der Klasse MATRIX_ATLAS
//
// **************************************************************************

#include "landscha.h"
#include "alg.h"
#include "log.h"

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
MATRIX_ATLAS::MATRIX_ATLAS(char *name, char *attr)
   : BASIS_LANDSCHAFT(name, attr)
{
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short MATRIX_ATLAS::speichern(FILE *file)
{
  DOUBLIST leere_liste;
  return adressliste_speichern(file, &leere_liste); // Kompatibilitaet
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short MATRIX_ATLAS::laden(FILE *file)
{
  DOUBLIST leere_liste;
  return adressliste_laden(file, &leere_liste); // Kompatibilitaet
}


/**---------------------------------------------------------------------------
  * MATRIX_ATLAS::gelaendeform_aendern()
  * 
  * Mit dieser und nur mit dieser Funktion kann die Gelaendeform eines
  * Feldes geaendert werden. Anderen Methoden sind nur fuer die Klasse
  * MATRIX_ATLAS zulaessig.
  ---------------------------------------------------------------------------*/
void MATRIX_ATLAS::gelaendeform_aendern(ADR& adr, short form)
{
  setze_feld(adr, (feld(adr) & 0xff00) | form);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short MATRIX_ATLAS::gelaendeform_in_feld(ADR& adr)
{
  return feld(adr) & 0x00ff;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void MATRIX_ATLAS::setze_feld_attribute_maske(ADR& adr, short maske)
{
  setze_feld(adr, (feld(adr)&0x80ff) | (unsigned short)(maske&0x7f) * 0x100);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short MATRIX_ATLAS::feld_attribute_maske(ADR& adr)
{
  return((feld(adr)&0x7f00) / 0x100);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void MATRIX_ATLAS::setze_feld_attribut(ADR& adr, short nummer)
{
  if (nummer<1 || nummer>7) {
      log('I', "MATRIX_ATLAS::setze_feld_attribut(): "
	  "Ungueltige Feldattributsnummer %d", nummer);
      return;
  }
  short maske = 0x80 >> nummer; // Bit um nummer Stellen nach rechts schieben
  maske |= feld_attribute_maske(adr);
  setze_feld_attribute_maske(adr, maske);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void MATRIX_ATLAS::loesche_feld_attribut(ADR& adr, short nummer)
{
  if (nummer<1 || nummer>7) {
      log('I', "MATRIX_ATLAS::setze_feld_attribut(): "
	  "Ungueltige Feldattributsnummer %d", nummer);
      return;
  }
  short maske = 0x80 >> nummer; // Bit um nummer Stellen nach rechts schieben
  maske = feld_attribute_maske(adr) & ~maske; // Ausmaskieren
  setze_feld_attribute_maske(adr, maske);
}

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short MATRIX_ATLAS::feld_attribut(ADR& adr, short nummer)
{
  if (nummer<1 || nummer>7) {
      log('I', "MATRIX_ATLAS::setze_feld_attribut(): "
	  "Ungueltige Feldattributsnummer %d", nummer);
      return 0;
  }
  short maske = 0x80 >> nummer; // Bit um nummer Stellen nach rechts schieben
  return ((feld_attribute_maske(adr) & maske) != 0);
}

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
char *MATRIX_ATLAS::gelaendeform_attribut(ADR& adr, char *klasse)
{
 return gelaende_formen.attribut_fuer_form(gelaendeform_in_feld(adr), klasse);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short MATRIX_ATLAS::gelaendeform_attribut_gesetzt(ADR& adr, char *klasse)
{
  return gelaende_formen.attribut_gesetzt_fuer_form
				 (gelaendeform_in_feld(adr), klasse);
}

