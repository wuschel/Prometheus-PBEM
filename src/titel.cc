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
  * MODUL:               titel.C  /  TITEL.CPP
  * AUTOR/DATUM:         Mathias Kettner, 2. August 1993
  * KOMPATIBILITAET:     C++
  -----------------------------------------------------------------------------
  *
  *      Beinhaltet die Funktion titel(), die einen Startbildschirm darstellt.
  *
  ---------------------------------------------------------------------------*/

#include "string.h"

#include "titel.h"
#include "kompatib.h"
#include "version.h"

void titel()
{
  io_cls();
  io_centerline(2,"P-R-O-M-E-T-H-E-U-S");
  if (NUR_TESTVERSION) io_centerline(3, "TESTVERSION");
  
  io_centerline(4,L("Ein Postspiel von Mathias Kettner","A Play-By-Mail game from Mathias Kettner"));
  io_centerline(5,version_programm);
  io_centerline(6,version_cvstag);
  io_centerline(7,version_sprache);
  io_centerline(9,version_copyright);
  
  io_centerline(11, L("Dies ist freie Software, Sie können Sie unter bestimmten Bedingungen\n", 
	     "This is free software, and you are welcome to redistribute it\n"));
  io_centerline(12, L("weiterverbreiten. Einzelheiten erfahren Sie in der Datei",
	     "certain conditions; for details look into"));
  io_centerline(13, "/usr/share/prometheus/COPYING");

  io_centerline(16,L("Konfiguration dieses Programmes:","Configuration:"));
  io_printxy(16,18,L("Betriebssystem:","Operating System:"));
  io_printxy(32,18,version_betriebssystem());
  io_printxy(16,19,L("Druckertreiber:","Printer driver:"));
  io_printxy(32,19,version_druckertreiber());
  io_printxy(16,20,L("Zeichensatz:","Characterset:"));
  io_printxy(32,20,version_zeichensatz());
  
  io_centerline(23,L("Druecken Sie eine Taste...","Press any key..."));
  io_getch();
  io_cls();
}
