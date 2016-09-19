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
  * MODUL:               version.h  /  VERSION.H
  * AUTOR/DATUM:         Mathias Kettner, 10. April 1994
  * KOMPATIBILITAET:     C++
  -----------------------------------------------------------------------------
  *
  *      Deklariert Funktionen, die lediglich Konfigurationsabgaben ausspucken.
  *      Ausserdem sind Konstante mit Versionsangaben deklariert. Diese Datei
  *      wird included von titel.cpp, den Druckertreibern (pcl5.cpp,...),
  *      den Systemanpassungen (msdos.cpp, dos_gcc.cpp,...) und den Zeichen-
  *      satzanpassern (unix_dru.cpp, ibmdruck.cpp)
  *
  ---------------------------------------------------------------------------*/

#ifndef __version_h
#define __version_h

char *version_betriebssystem();
char *version_druckertreiber();
char *version_zeichensatz();

#define version_programm  L("Version 1.2.1 revision 1 - 3. Mai 2001","Version 1.2.1 revision 1 - May 3rd 2001")

#define version_cvstag      "version_1_2_1_revision_1"

#define version_sprache   L("Deutsche Version","English version")
    
#define version_copyright   "Copyright Mathias Kettner 1993-2003"
#define version_lizenz      "TESTVERSION"
    

#endif // __version_h

