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
  * MODUL:               kompatib.h / KOMPATIB.H
  * AUTOR/DATUM:         Mathias Kettner, 11. April 1994
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------
  * 
  *	Deklariert einige Funktionen und enthaelt einige kleine inline-
  *	Funktionen, die diese Funktionen benuetzen. Alle diese Funktionen
  *	realisieren Aufgaben, die bestriebssystemspezifisch implementiert
  *	werden muessen. Gleichzeitig sind sie die einzigen systemabhaengigen
  *	Funktionen. Die Realisierung der Funktionen muss fuer jedes Betriebs-
  *	system in einem eigenen source-File gemacht werden, welches beim
  *	Compilieren zum Objekt bin/_system.o wird. Beim Linken werden somit
  *	die richtigen Funktionen fuer das entsprechende System eingebunden.
  *	
  *	Um alle Funktionen implementieren zu koennen, muss das System folgen-
  *	de Minimalvoraussetzungen erfuellen:
  *	
  *	- Zeichenorientiertes (oder orientierbares) Fenster oder Bildschirm
  *	  mit einer Aufloesung von 24 Zeilen und 80 Spalten.
  *	- Darstellbar muessen alle druckbaren Asciizeichen sein (32 bis 126)
  *	- Tastatur, die ungepuffert abgefragt werden kann (ohne Returntaste
  *	  am Ende).
  *	- Es muessen Daten zu einem Drucker geschickt werden koennen.
  *
  *	Hilfreich ist waere zudem:
  *
  *	- Moeglichkeit, Zeichen hervorzuheben (z.B. durch hellere Zeichen
  *	  wie bei Herkules)
  *	- Deutsche Umlaute auf dem Bildschirm
  *
  *	Nicht noetig sind und auch nicht unterstuetzt werden: Andere Aufloe-
  *	sungen als 80X24, Farbe, hochaufloesende Grafik, Maus, Sound.
  *	Als Lohn fuer diesen Verzicht koennen alle Programme, die als IO-
  *	Funktionen nur die aus kompatib.h verwenden auf alle Platformen
  *	portiert werden, die einen C++-Compiler und einen ausreichenden
  *	Adressraum (real oder virtuell) anbieten koennen, sowie den obigen
  *	minimalen Anforderungen genuegen. Sogar ein Betrieb ueber ein Terminal
  *	wie VT100 ist moeglich.
  *
  *	Implementiert sind die IO-Funktionen bereits fuer Linux/GCC und
  *	MSDOS/DJGPP.
  *	
  ---------------------------------------------------------------------------*/
#ifndef __kompatib_h
#define __kompatib_h

#include <string.h>

// Konstante fuer io_setattr()

#define IO_NORMAL 0
#define IO_BOLD 1
#define IO_REVERSE 2
#define IO_UNDERLINE 4

// Konstante fuer io_setcolour
#define IO_BLACK 0
#define IO_RED 1
#define IO_GREEN 2
#define IO_YELLOW 3
#define IO_BLUE 4
#define IO_MAGENTA 5
#define IO_CYAN 6
#define IO_WHITE 7
#define IO_NORMALCOLOR -1 // lokaler Standard

// Funktionen, die Systemspezifisch implementiert werden muessen

// 1. Bildschirm IO

void io_init();                // IO initialisieren
void io_close();	       // IO beenden

void io_readline(char *, int); // Zeile vom Benutzer einlesen
int io_getch();		       // Zeichen holen, ungepuffert aber mit Warten
int io_isbackspace(int);       // Bezogen auf das Ergebnis von io_getch()
int io_iscursorup(int);
int io_iscursordown(int);
int io_iscursorleft(int);
int io_iscursorright(int);
int io_ispageup(int);
int io_ispagedown(int);
int io_ishome(int);
int io_isend(int);

void io_gotoxy(int, int);      // Cursor bewegen
void io_cls();                 // Bildschirm loeschen
void io_print(char *);         // Text an der Cursorposition ausgeben
void io_setattr(unsigned long a=IO_NORMAL); // Setzt Attribute wie REVERSE, BOLD,...
void io_fgcolour(int c=IO_NORMALCOLOR); // Farbe setzen
void io_bgcolour(int c=IO_NORMALCOLOR); // Farbe setzen
void io_reset(); // Alles Farben und Attr. auf default setzen;
void io_line(int y);           // Waagerechte Linie
void io_doubleline(int);       // Doppelstarke waagerechte Linie
void io_beep();		       // kurzer CTRL-G Peep oder aehnliches

// 2. Druckerausgabe

short io_init_printer();       // Drucker oeffnen, initialisieren
void io_close_printer();       // Drucker schliessen.
void io_printer(char *);       // Text zum Drucker schicken
void io_printer_raw(unsigned char *, long); // Transparentes Drucken
short io_print_file(char *);   // Ganzes File ausdrucken
void io_printer_flush();       // Druckt Puffer leer.

// 3. Diskettenzugriff

short io_mount_floppy();
short io_umount_floppy();
char *io_floppy_path();

// 4. Spezielle Filesystem Zugriffe

void io_mkdir(char *);
char *io_fopen_binary_mode();

// 5. Sonstiges

void io_init_random();		// Zufallsgenerator initialisieren
long io_random(long);		// Zufallszahl mit Bereichsangabe
unsigned long io_availmem();	// Wieviel Speicher noch frei ist.
char *CR();                     // Unter UNIX "\r", unter DOS "".

// Makros

inline void io_printchar(char c)
{ char pack[2];  pack[0]=c;  pack[1]=0; io_print(pack); }

inline void io_printxy(int x, int y, char *text)
{ io_gotoxy(x,y); io_print(text); };

inline void io_deleteline(int y)
{ io_printxy(0,y,"                                                                                "); };

inline void io_centerline(int y, char *t) {
  io_deleteline(y);
  if (!t || !*t) return;
  io_printxy(40-strlen(t)/2,y,t);
};

inline void io_reset() {
  io_fgcolour();
  io_bgcolour();
  io_setattr();
}


#endif // __kompatib_h

