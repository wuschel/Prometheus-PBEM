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
  * MODUL:               bemaske.h / BEMASKE.H
  * AUTOR/DATUM:         Mathias Kettner, 1. Juli 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Deklariert die Klasse BEMASKE. Bemaske heisst
//      Befehlseingabemaske. Sie ist die Bedienungsoberflaeche fuer das
//      Eintippen der Befehlskarte eines Spielers.
//
// *************************************************************************
#ifndef bemaske_h
#define bemaske_h

#include "staat.h"
#include "fileview.h"

// Befehlseingabemaske

enum ZUSTAND
{
  OBJEKTNAME_NOCH_NICHT_FERTIG,
  FERTIG_FUER_NAECHSTEN_BEFEHL,
  ERWARTE_ZWEITEN_BUCHSTABEN,
  ERWARTE_PARAMETER,
  QUOTE_MODUS,
};

#define EFLAENGE 79
#define X_EINGABEFELD 0
#define Y_EINGABEFELD 23


struct MEMORY : public DOUBLIST_NODE
{
  char *zeile;
  MEMORY(char *s) { zeile = mystrdup(s); };
  virtual ~MEMORY() { myfree(zeile); };
};


class BEMASKE
{
  STAAT *staat;
  enum ZUSTAND zustand;
  char eingabefeld[EFLAENGE+1];
  char *eingabezeiger;
  char *parameterzeiger; // Hilfszeiger fuer Zustand ERWARTE_PARAMETER
  DOUBLIST besitzliste; // Transitiver Besitzt des Staates
  DOUBLIST memory; // Alle eingegebenen Befehlszeilen
  FILEVIEWER *fileviewer;
public:
  BEMASKE(STAAT *);
  ~BEMASKE() { if (fileviewer) delete fileviewer; };
private:
  void besitzliste_berechnen();
  void maske_aufbauen();
  void eingabephase();
  void transitionsfunktion(int);
  void trans_onnf(int);
  void trans_ffnb(int);
  void trans_ezb(int);
  void trans_ep(int);
  void trans_qm(int);
  short befehl_beginnt_mit(char);
  short befehl_existiert(char *);
  short objektname_vervollstaendigen();
  short objekt_in_besitz_mit_namen(char *);
  short steuertaste_verarbeiten(int);
  void zeile_loeschen();
  void ef_loeschen();
  void ef_anfuegen(char);
  void ef_anfuegen(char *);
  void ef_backspace();
  void ef_refresh();

  void memory_laden(); // Und anzeigen
  void memory_aufnehmen(char *);
  void memory_speichern();
  
  void befehle_von_diskette();
  FILE *diskettendatei_oeffnen();

  void persoenliche_mitteilung();
};

#endif // bemaske_h

