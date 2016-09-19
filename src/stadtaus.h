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
  * MODUL:               stadtaus.h / STADTAUS.H
  * AUTOR/DATUM:         Mathias Kettner, 6. Mai 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Enthaelt Definition der Klasse STADTAUSBAU, die alle Einrich-
//      tungen realisiert, die ein Spieler in einer Stadt bauen kann.
//
// **************************************************************************
 
#ifndef __stadtaus_h
#define __stadtaus_h

#include "enzyklop.h"


/**---------------------------------------------------------------------------
  * KLASSE:              STADTAUSBAU
  * ABGELEITET VON:      OBJEKT
  * 
  * Alle Objekte, die eine Stadt bauen kann und die anschliessend zu
  * ihrem Besitz gehoeren und in ihr verbleiben (Fabrik, Kirche, ...)
  * werden durch diesen Objekttyp realisiert. Ein Objekt vom Typ
  * STADTAUSBAU hat seinen Hauptlebenssinn darin, zu existieren und
  * die Stadt, in der es steht zu beeinflussen! Deshalb besitzt ein
  * solches Objekt im Normalfall auch weder Befehle, noch Infos noch
  * Kommandos. Das einzige Kommando, das implementiert ist, ist das
  * Kommando 'SU', mit dem ein Bauwerk dazu aufgefordert wird, sich
  * von der Stadt mit Resourcen zu versorgen.
  *
  * FUNKTIONEN:
  * (aus OBJEKT:) initialisieren(), kommando(), laden()
  * 
  ---------------------------------------------------------------------------*/
class STADTAUSBAU : public OBJEKT
{
public:
  STADTAUSBAU(char *, char *);
  short kommando(const char *, const void *par1=NULL, const void *par2=NULL, const void *par3=NULL);
private:
  void naechste_phase(long); // Nur zur Versorgung.
  void zug_abschliessen(long); // Einfluss der Sehenswuerdigkeiten
  short kommando_beschossen_werden(void *, void *);
  void unterhalt_einholen(); // Sich von der Stadt versorgen.
  void einfluesse_pruefen(); // suspendieren oder aktivieren.
};

#endif // __stadtaus_h

