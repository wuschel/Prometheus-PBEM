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


#ifdef MSDOS

/**---------------------------------------------------------------------------
  * MODUL:               DOS_GCC.CPP
  * AUTOR/DATUM:         Mathias Kettner, 28. April 1993
  * KOMPATIBILITAET:     C++
  -----------------------------------------------------------------------------
  *
  *     Enthaelt die in kompatib.h deklarierten Kompatibilitaetsfunktionen
  *     speziell implementiert fuer den gcc-Compiler unter MS-DOS. Fuer
  *     die IO-Funktionen werden Funktionen aus der Bibliothek libpc.a
  *     verwendet, die auf DOS-Funktionen zugreift.
  *
  ---------------------------------------------------------------------------*/

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <std.h>
#include <gppconio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "kompatib.h"
#include "alg.h"
#include "version.h"
#include "log.h"

/**---------------------------------------------------------------------------
  * Die Konfigurationsangabe ist eine kleine Funktion, die einen Text
  * zurueckgibt, der dann auf dem Titelbildschirm ausgegeben wird.
  ---------------------------------------------------------------------------*/
char *version_betriebssystem()
{
  return L("MS-DOS 5.00 oder kompatibel","MS-DOS 5.00 or compatible");
}

char *CR() { return ""; };

int printer_file = -1;


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void io_init() {  window(1,1,80,24); }
void io_close() {}
void io_gotoxy(int x, int y) { gotoxy(x+1,y+1); }
void io_cls() { io_deleteline(1,24); }
void io_print(char *text) {  cprintf("%s",text); }

unsigned long io_availmem() { return 0; }; // Uber 128 MB virtual mem!

extern "C" {
  void delay(int);
  void sound(int,int,int,int,int);
}

void io_beep()
{
  sound(600,1,1,1,1);
  delay(50);
  sound(0,0,0,0,0);
}



void io_line(int y)
{
  io_printxy(0,y,"--------------------------------------------------------------------------------");
}

void io_doubleline(int y)
{
  io_printxy(0,y,"================================================================================");
}



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void mycgets(char *puffer)
{
  int maxlaenge = puffer[0];
  char *ziel = puffer+2, *ende = ziel+maxlaenge, *write=ziel;

  int c;
  do {
    c = io_getch();
    if (io_isbackspace(c)) {
      if (write > ziel) {
	write--;
	cprintf("%c %c",c,c);
      }
    }
    else if (isprint(c)) {
      if (write < ende-1) {
	*write++ = c;
	cprintf("%c",c);
      }
    }

  } while (c!=13 && c!=10);
  *write = 0; // String unbedingt beenden!
}

void io_readline(char *puffer, int laenge)
{
  char *buf = new char[laenge+3];
  buf[0] = laenge+1;
  buf[2] = 0;
  mycgets(buf);
  strcpy(puffer, &buf[2]);
  delete buf;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short io_init_printer()
{
  if (printer_file != -1) io_close_printer();
  printer_file = open("PRN",O_WRONLY|O_BINARY,0);
  if (printer_file == -1) return 1;
  else return 0;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void io_close_printer()
{
  if (printer_file != -1) close(printer_file);
  printer_file = -1;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void io_printer(char *text)
{
  if (printer_file==-1 || !text) return;
  write(printer_file, text, strlen(text));
}


void io_printer_raw(unsigned char *daten, long laenge)
{
  if (printer_file==-1 || !daten) return;
  write(printer_file, daten, laenge);
}


short io_print_file(char *name)
{
  int file = open(name, O_RDONLY|O_BINARY ,0);
  if (file == -1) return 1;

  int laenge;
  char *puffer = new char[4096];
  if (!puffer) {
    close (file);
    return 1;
  }

  io_init_printer();
  while (0 != (laenge = read(file, puffer, 4096)))
    io_printer_raw((unsigned char *)puffer, laenge);
  io_close_printer();

  delete puffer;
  close (file);
  return 0;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void io_setattr(unsigned long maske)
{
  if (maske & IO_BOLD) highvideo();
  else normvideo();
}
void io_bgcolour(int) {};
void io_fgcolour(int) {};

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void io_init_random()
{
  srand(time(NULL));
}

long io_random(long bereich)
{
  if (bereich >= 0x40000000) {
      log('I', "io_random(): Bereich fuer Zufallszahl ist zu gross");
      return 0;
  }

  // rand() liefert eine Zahl von 0 bis 2^15-1
  if (bereich < 0x8000) return rand()%bereich;
  else {
    long rand1 = rand();
    long rand2 = rand1*0x8000 + rand();
    return rand2 % bereich;
  }
}


int io_getch()
{
  static int zeichen_auf_vorrat = 0;

  if (zeichen_auf_vorrat) {
    int rwert = zeichen_auf_vorrat;
    zeichen_auf_vorrat = 0;
    return rwert;
  }
  int taste = getch();
  
  switch (taste) {
    case 148: zeichen_auf_vorrat = 'o'; return '~';
    case 132: zeichen_auf_vorrat = 'a'; return '~';
    case 129: zeichen_auf_vorrat = 'u'; return '~';
    case 153: zeichen_auf_vorrat = 'O'; return '~';
    case 142: zeichen_auf_vorrat = 'A'; return '~';
    case 154: zeichen_auf_vorrat = 'U'; return '~';
    case 225: zeichen_auf_vorrat = 's'; return '~';
  }
  
  if (taste) return taste;
  
  else return (256 + io_getch());
}


/**---------------------------------------------------------------------------
  * io_is...()
  * 
  * Stellt fest, ob es sich bei dem Ergebnis von io_getch() um eine be-
  * stimmte Steuertaste handelt.
  ---------------------------------------------------------------------------*/
int io_isbackspace(int taste) { return taste==4 || taste==8 || taste==127; }
int io_iscursorup(int c)  { return c==328; };
int io_iscursordown(int c)  { return c==336; };
int io_iscursorleft(int c)  { return c==331; };
int io_iscursorright(int c)  { return c==333; };
int io_ispageup(int c)  { return c==329; };
int io_ispagedown(int c)  { return c==337; };
int io_ishome(int) { return 388; };
int io_isend(int) { return 374; };


short io_mount_floppy() { return 1; };  // Kein Mounten noetig
short io_umount_floppy() { return 1; }; // Auch kein Unmounten.
char *io_floppy_path() { return "a:/"; };

void io_mkdir(char *path) { mkdir(path, 775); };

char *io_fopen_binary_mode() { return "wb"; };

#else // Ende MSDOS Teil, Start Linux Teil.
 
/**---------------------------------------------------------------------------
  * MODUL:               unix.C  /  UNIX.CPP
  * AUTOR/DATUM:         Mathias Kettner, 28. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Enthaelt die in kompatib.h deklarierten Kompatibilitaetsfunktionen
//	speziell implementiert fuer UNIX-Systeme mit der Bibliothek ncurses.
//
// *************************************************************************

extern "C" {
  #include <curses.h>
}

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>

#include "kompatib.h"
#include "version.h"
#include "log.h"

// #include "colortty.h" // Holt Sourcecode.
extern short zufallsgenerator_deterministisch; // Bei 1 immer die gleichen Z-Zahlen

char *CR() { return "\r"; };

/**---------------------------------------------------------------------------
  * Die Konfigurationsangabe ist eine kleine Funktion, die einen Text
  * zurueckgibt, der dann auf dem Titelbildschirm ausgegeben wird.
  ---------------------------------------------------------------------------*/
char *version_betriebssystem()
{
  return L("Linux für Intel 386","Linux Intel 386 based");
}

int printer_file = -1; // Filedeskriptor (open, close, write) fuer Drucker

void io_init()
{
  initscr();
  cbreak();
  noecho();
  nonl();
  // keypad(stdscr, TRUE);
}

void io_close()
{
  echo();
  nocbreak();
  endwin();
}

int io_getch()
{
  static int zeichen_auf_vorrat = 0;

  if (zeichen_auf_vorrat) {
    int rwert = zeichen_auf_vorrat;
    zeichen_auf_vorrat = 0;
    return rwert;
  }

  int c = getch();
  if (c < 0) c += 256;

  if (c==27 && getch() == 91) // Sonderzeichen
  {
    switch (getch()) {
      case 53:
     	if (getch()==126) return 1000;
     	else return io_getch();
      case 54:
     	if (getch()==126) return 1001;
     	else return io_getch();
      case 65: return 259;
      case 66: return 258;
      case 67: return 261;
      case 68: return 260;
      default: return io_getch();
    }
  }

  switch (c) {
    case 148: zeichen_auf_vorrat = 'o'; return '~';
    case 132: zeichen_auf_vorrat = 'a'; return '~';
    case 129: zeichen_auf_vorrat = 'u'; return '~';
    case 153: zeichen_auf_vorrat = 'O'; return '~';
    case 142: zeichen_auf_vorrat = 'A'; return '~';
    case 154: zeichen_auf_vorrat = 'U'; return '~';
    case 225: zeichen_auf_vorrat = 's'; return '~';
  }
  
  return c;
  
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void mycgets(char *puffer)
{
  int maxlaenge = puffer[0];
  char *ziel = puffer+2;
  char *ende = ziel+maxlaenge, *write=ziel;
  char string[4]; // Hilfsstring;

  int c;
  do {
    c = getch();
    if (io_isbackspace(c)) {
      if (write > ziel) {
        write--;
	int Y,X;
	getyx(stdscr,Y,X);
	io_gotoxy(X-1,Y);
	io_print(" ");
	io_gotoxy(X-1,Y);
      }
    }
    else if (isprint(c)) {
      if (write < ende-1) {
        *write++ = c;
        string[0]=c;
        string[1]=0;
        io_print(string);
      }
    }

  } while (c!=13 && c!=10);
  *write=0; // String muss unbedingt beendet werden!
}


void io_readline(char *puffer, int laenge)
{
  char *buf = new char[laenge+3];
  buf[0] = laenge+1;
  buf[2] = 0;
  mycgets(buf);
  strcpy(puffer, &buf[2]);
  delete buf;
}


void io_gotoxy(int x, int y)
{
  move(y,x);
  refresh();
}

void io_cls()
{
  clear();
  refresh();
}

void io_print(char *text)
{
  addstr(text);
  refresh();
}

void io_setattr(unsigned long)
{
//  if (maske & IO_BOLD) io_print("\033[01m");
//  else io_print("\033[00m");
}

void io_line(int y)
{
  io_gotoxy(0,y);
  io_print("--------------------------------------------------------------------------------");
}

void io_doubleline(int y)
{
  io_gotoxy(0,y);
  io_print("================================================================================");
}

void io_pause(long) // millis)
{
  // Ich finden momentan keine Funktion, die unter Unix eine Anzahl von Milli
  // oder Microsekunden wartet...

  // void usleep(long);
  // usleep(1000 * millis);
}

void io_beep()
{
  printf("\007");
  // beep(); // ncurses-Funktion
  // refresh();
}


short io_init_printer()
{
  if (printer_file != -1) io_close_printer();
  printer_file = open("/dev/lp1",O_WRONLY);
  if (printer_file == -1) return 1;
  else return 0;
}

void io_close_printer()
{
  if (printer_file != -1) close(printer_file);
  printer_file = -1;
}

void io_printer(char *text)
{
  if (printer_file==-1 || !text) return;
  write(printer_file, text, strlen(text));
}

void io_printer_raw(unsigned char *daten, long laenge)
{
  if (printer_file==-1 || !daten) return;
  write(printer_file, daten, laenge);
}

short io_print_file(char *name)
{
  int file = open(name, O_RDONLY);
  if (file == -1) return 1;

  int laenge;
  char *puffer = new char[4096];
  if (!puffer) {
    close (file);
    return 1;
  }

  io_init_printer();
  while (0 != (laenge = read(file, puffer, 4096)))
    io_printer_raw((unsigned char *)puffer, laenge);
  io_close_printer();

  delete puffer;
  close (file);
  return 0;
}


void io_init_random()
{
  if (zufallsgenerator_deterministisch) srand(1);
  else srand(time(NULL));
}

long io_random(long bereich)
{
  if (bereich >= 0x40000000) {
      log('I', "io_random(): Bereich fuer Zufallszahl ist zu gross");
      return 0;
  }

  // rand() liefert eine Zahl von 0 bis 2^15-1
  if (bereich < 0x8000) return rand()%bereich;
  else {
    long rand1 = rand();
    long rand2 = rand1*0x8000 + rand();
    return rand2 % bereich;
  }
}

int io_isbackspace(int c) { return c==8 || c==127 || c==263; }
int io_iscursorup(int c)  { return c==259; };
int io_iscursordown(int c)  { return c==258; };
int io_iscursorleft(int c)  { return c==260; };
int io_iscursorright(int c)  { return c==261; };
int io_ispageup(int c)  { return c==339; };
int io_ispagedown(int c)  { return c==338; };
int io_ishome(int c) { return c==1000; };
int io_isend(int c) { return c==1001; };

unsigned long io_availmem()
{
  return 0; // Weiss nicht, wie ich das unter Unix ermitteln soll...
}

short io_mount_floppy()
{ 
  system("mount /a");
  return 1;
}

short io_umount_floppy()
{
  system("umount /a");
  return 1;
}

char *io_floppy_path()
{
  return "/a/";
}

// int mkdir(const char *, mode_t);

void io_mkdir(char *path)
{ 
  mkdir(path, 0775); 
}

char *io_fopen_binary_mode() { return "w"; };

#endif // Unterscheidung MSDOS / Unix
