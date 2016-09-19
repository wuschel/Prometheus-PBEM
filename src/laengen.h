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
  * MODUL:               limits.h / LIMITS.H
  * AUTOR/DATUM:         Mathias Kettner, 12. Mai 1993
  * KOMPATIBILITAET:     C / C++
  ---------------------------------------------------------------------------*/
//
//      Dieses Headerfile enthaelt nur Konstanten. Sie legen die maximale
//      Laenge verschiedener Strings fest, die im Umgang mit dem System
//      auftreten koennen, da manchen Zwischenspeicher intern aus Gruenden
//      der Effizienz und der Aufwandsminimierung statisch angelegt werden.
//
//      ACHTUNG:
//      Ueberschreitungen von Laengen werden meist nicht abgefangen, so dass
//      es zu unkontrolliertem Verhalten des Programms kommen kann, wenn
//      Strings zu lang sind. Die maximal erlaubten Laengen sind grosszue-
//      gig dimensioniert. Im Zweifelsfalle muss die Laenge in dieser Datei
//      trotzdem erhoeht werden und das komplette Programm neu kompiliert 
//      werden.
//
// **************************************************************************

#define MAX_LAENGE_NAME 60 // Namen von Objekten
#define MAX_LAENGE_ATTRIBUT 256 // Name einer Attributsklasse
#define MAX_LAENGE_ATTRIBUTSWERT 256 // Wert eines Attributes
#define MAX_LAENGE_ATTRIBUTSZEILE 16384 // Ganze Zeile von Attributen
#define MAX_LAENGE_ZEILE 4094 // Beim Einlesen eines Objektfiles.
#define MAX_LAENGE_BEFEHL 512 // Befehl fuer ein Objekt
#define MAX_LAENGE_BEFEHLSZEILE 2048 // Befehl fuer ein Objekt
#define MAX_LAENGE_EINFLUSSART 256 // Name einer Einflussart
#define MAX_LAENGE_EINFLUSSPARAMETER 256 // Parameter eines Einflusses
#define MAX_LAENGE_ENZYKLOPAEDIE_ZEILE 4094 // Beim Einlesen der Enzlyklopaedie
#define MAX_LAENGE_DATEINAME 512 // Ist bei DOS ohnehin max 12 Zeichen
#define MAX_LAENGE_RESOURCENAME 31 // Name fuer Resource, z.B. "Nahrung"
#define MAX_LAENGE_RESOURCEZEILE 120 // Angabe eines Res-Vektors wie "2N12E"

