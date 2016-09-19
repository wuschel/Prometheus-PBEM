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


#ifndef __spieler_h
#define __spieler_h

class STAAT;

#define SPIELERDATEN_ANZAHL 18

class SPIELERDATENSATZ
{
  char *daten[18];
  long dpz; // DM pro Zug
public:
  SPIELERDATENSATZ(STAAT *st = NULL); // Werte vorbelegen.
  ~SPIELERDATENSATZ() { freimachen(); };
  void freimachen();  // Defacto Destruktor
  char *fehlertext(); // Sind die Werte im gueltigen Bereich?

  char*& name() 	{ return daten[0]; };
  char*& vorname() 	{ return daten[1]; };
  char*& strasse() 	{ return daten[2]; };
  char*& ort() 		{ return daten[3]; };
  char*& telefon() 	{ return daten[4]; };
  char*& reich() 	{ return daten[5]; };
  char*& titel()     	{ return daten[6]; };
  char*& herrscher() 	{ return daten[7]; };
  char*& hauptstadt() 	{ return daten[8]; };
  char*& hautfarbe() 	{ return daten[10]; };
  char*& groesse() 	{ return daten[11]; };
  char*& haarfarbe() 	{ return daten[12]; };
  char*& hauptattr() 	{ return daten[13]; };
  char*& entw1() 	{ return daten[14]; };
  char*& entw2() 	{ return daten[15]; };
  char*& entw3() 	{ return daten[16]; };
  long& dm_pro_zug()    { return dpz; };
};

#endif __spieler_h
