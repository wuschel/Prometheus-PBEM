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
  * MODUL:               zylinder.C / ZYLINDER.CPP
  * AUTOR/DATUM:         Mathias Kettner, 28. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
// 
//      Enthaelt Funktionen der Klasse ZYLINDER_ATLAS. 
// 
// **************************************************************************

#include <math.h>

#include "landscha.h"

/**---------------------------------------------------------------------------
  * ZYLINDER_ATLAS::entfernung_zwischen() // virtuell
  * 
  * Berechnet fuer den Landschaftstyp Zylinder die Entfernung zwischen
  * zwei Adressen.
  *
  * @param
  * a,b:            Zu pruefende Adressen.
  *
  * @return
  * Entfernung auf kuerzestem Weg, wobei ein Feld geradeaus 1.0 und
  * ein Feld diagonal 1.5 zaehlt.
  ---------------------------------------------------------------------------*/
float ZYLINDER_ATLAS::entfernung_zwischen(ADR& a, ADR& b)
{
  short yd = ABS(a.y - b.y);
  
  // Die Berechnung des X-Abstandes ist etwas schwieriger, da ich den
  // Fall beruecksichtigen muss, dass die Entfernung ueber die "Schweiss-
  // naht" kuerzer ist. Deshalb noch ein abschliesender Vergleich
  
  short xd = ABS(a.x - b.x);
  if (xd*2 > breite) xd = breite - xd; // Um die Schweissnaht kuerzer.

  short min = MIN(xd, yd);
  short max = MAX(xd, yd);
  
  return float(0.5*min + max);
}


/**---------------------------------------------------------------------------
  * ZYLINDER_ATLAS::benachbart()          // virtuell
  * 
  * Stellt fest, ob zwei Felder benachbart zueinander sind. Bei einer
  * diagonalen Landschaft gelten andere Kriterien, als bei einer
  * nicht-diagonalen (Siehe Variable diagonal in 2D_MATRIX_ATLAS)
  *
  * @param
  * a,b:            Die zu pruefenden Adressen
  *
  * @return
  * 1, falls sie benachbart sind, 0 sonst.
  ---------------------------------------------------------------------------*/
short ZYLINDER_ATLAS::benachbart(ADR& a, ADR& b)
{
  // Falls Felder ueber Ecken benachbart sein sollen, steht
  // die Variable diagonal in 2D_MATRIX_ATLAS auf 1. In so
  // einem Fall wird als maximale Entfernung zweier benachbarter
  // Felder enfach 1.5 genommen. Ansonsten 1.0

  return (entfernung_zwischen(a,b) <= 1.0 + diagonal*0.5);
}


/**---------------------------------------------------------------------------
  * ZYLINDER_ATLAS::wrap()                // virtuell
  * 
  * Wandelt eine Adresse, die eingenlich ausserhalb des Matrixbereiches
  * liegt in eine um, die wieder innerhalb liegt, durch eine Modulo-
  * Operation. Dies ist beim Zylinderatlas nur moeglich, wenn die Adres-
  * se ohnehin innerhalb liegt, oder aber nur die X-Koordinate ausserhalb
  * liegt.
  *
  * @param   
  * to_wrap:        Zeiger (!) auf die Adresse, die umgewrappt werden
  * soll.
  *
  * @return
  * 1, falls die Adresse auch nach dem wrappen noch ausserhalb der
  * Landschaft liegt, folglich ungueltig ist.
  * 0, wenn nun eine gueltige Adresse vorliegt.
  ---------------------------------------------------------------------------*/
short ZYLINDER_ATLAS::wrap(ADR *to_wrap)
{
  while (to_wrap->x < 0) to_wrap->x += breite;
  to_wrap->x %= breite;
  return adresse_ist_ausserhalb(*to_wrap);
}


RIC& ZYLINDER_ATLAS::richtung_von_nach(ADR& von, ADR& nach)
{
  static RIC antwort("0");
  antwort.y = nach.y - von.y;

  // Bei der X-Komponente sieht es etwas schwieriger aus, da ich natuer-
  // lich den kuerzesten Weg haben will. Wenn aber bei Punkte ueber den
  // Rand naeher verbunden sind, dann muss ich die Richtung ueber den
  // Rand angeben.

  long xric = nach.x - von.x;
  if (xric > breite/2) xric -= breite;
  else  if (xric < -breite/2) xric += breite;

  antwort.x = xric;
  return antwort;
}
