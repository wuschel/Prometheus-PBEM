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
  * MODUL:               maske.C  /  MASKE.CPP
  * AUTOR/DATUM:         Mathias Kettner, 18. Juli 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Funktionen der Klassen MASKE, MASKEFELD, TEXTFELD,
//      NUMFELD und AUSWAHLFELD. Diese Klassen dienen zum Auf-
//      bau von Eingabemasken.
//
// *************************************************************************

#include <string.h>
#include <ctype.h>

#include "maske.h"
#include "kompatib.h"
#include "mystring.h"
#include "log.h"

/**---------------------------------------------------------------------------
  * MASKE::edit()
  * 
  * Nachdem alle Felder refresht wurden, springt der Cursor in das
  * aktuelle Feld der Maske. Wird die Maske zum erstenmal editiert,
  * so wird das erste Feld angesprungen.
  * Nun kann der Benutzer den Wert aller Felder der Maske aendern.
  * Mit CTRL-E wird das Editieren beendet, mit CTRL-A wird es abge-
  * brochen. Bei einem normalen Ende werden die Werte der Felder, bei
  * denen eine Zielbereich angegeben wurde, in diese Zielbereiche ko-
  * piert. Bei Strings funktioniert dies dynamisch, d.h. der Zielbereich
  * besteht nicht aus einem Puffer, sondern aus einem Zeiger auf einen
  * Stringzeiger, der dann von ::edit() auf einen dynamisch belegten
  * Bereich gebogen wird. Ist der Zeiger im Zielbereich vor Beginn
  * nicht NULL, so wird der alte inhalt mit myfree() freigegeben!
  *
  * @return
  * EDIT_END, wenn mit CTRL-E beendet wurde.
  * EDIT_QUIT, wenn mit CTRL-A beendet wurde.
  ---------------------------------------------------------------------------*/
short MASKE::edit()
{
  // Erst muessen alle Felder refresht werden
  felder_refreshen();

  while (!is_empty())
  {
    if (aktuellesfeld->is_tail()) aktuellesfeld = (MASKEFELD *)first();
    if (aktuellesfeld->is_head()) aktuellesfeld = (MASKEFELD *)last();

    switch (aktuellesfeld->edit()) {
      case EDIT_END: werte_in_zielbereiche_kopieren();
		     return 1; // Ok

      case EDIT_QUIT: return 0;

      case EDIT_UP:
	aktuellesfeld = (MASKEFELD *)aktuellesfeld->previous();
	break;

      case EDIT_DOWN:
	aktuellesfeld = (MASKEFELD *)aktuellesfeld->next();
	break;
    }
  }
  return 0; // Wird eigentlich eh nie erreicht.
}


/**---------------------------------------------------------------------------
  * MASKE::werte_in_zielbereiche_kopieren()
  * 
  * Kopiert die Werte von all den Feldern in ihre Zielbereiche, bei
  * denen ein solcher angegeben wurde. Ausserdem wird fuer jedes
  * Feld noch die virtuelle Funktion wert_in_zielbereich() aufgerufen,
  * in der eine aehnliche Aktion ausgeloest werden kann.
  ---------------------------------------------------------------------------*/
void MASKE::werte_in_zielbereiche_kopieren()
{
  MASKEFELD *maskefeld = (MASKEFELD *)first();
  while (!maskefeld->is_tail())
  {
    maskefeld->wert_in_zielbereich(); // Kann noch individuell verwendet werd.
    if (maskefeld->zielbereich) { // Nur dann soll hineingeschrieben werden
      myfree(*maskefeld->zielbereich); // Alten Wert freigeben
      *maskefeld->zielbereich = mystrdup(maskefeld->daten); // Neuen eintragen
    }
    maskefeld = (MASKEFELD *)maskefeld->next();
  }
}



/**---------------------------------------------------------------------------
  * MASKEFELD::MASKEFELD()        // constructor
  * 
  * Konstruktor der allgemeinen Basisklasse MASKEFELD. Er laedt
  * das Feld sogleich aus dem Zielbereich, falls einer angegeben
  * wurde. Ausserdem fuegt er das Feld sogleich einer Maske zu, wenn
  * gewuenscht.
  *
  * @param
  * MASKE *m: != NULL, dann wird das Feld der Maske m hinzugefuegt.
  * short x,y:    Bildschirmkoordinaten des Eingabebereiches
  * short l:      Laenge des Eingabebereiches.
  * char **z:     Zeiger auf einen Zeiger, Zielbereich!
  ---------------------------------------------------------------------------*/
MASKEFELD::MASKEFELD(MASKE *m, short x, short y, short l, char **z)
{
  xpos = x;
  ypos = y;
  laenge = l;
  zielbereich = z;
  if (z) daten = mystrdup(*z);  // Aus Zielbereich einlesen
  else daten = NULL;

  // Und nun fuege ich das neue Feld gleich in die Maske m ein, fall dies
  // gewuenscht war.
  if (m) m->add_tail(this);
}

/**---------------------------------------------------------------------------
  * TEXTFELD::TEXTFELD()          // constructor
  * 
  * Konstruktor der Klasse, die ein Texteingabefeld realisiert. Ruft
  * hauptsaechlich MASKEFELD() auf.
  * Ausser dem Zielbereich kann noch ein Startwert angegeben werden,
  * aus dem das Feld auf jeden Fall geladen wird, auch wenn ein
  * Zielbereich angegeben war.
  *
  * @param
  * MASKE *m: != NULL, dann wird das Feld der Maske m hinzugefuegt.
  * short x,y:    Bildschirmkoordinaten des Eingabebereiches
  * short l:      Laenge des Eingabebereiches.
  * char **z:     Zeiger auf einen Zeiger, Zielbereich!
  * char *sw:     Optionaler Startwert fuer das Feld.
  ---------------------------------------------------------------------------*/
TEXTFELD::TEXTFELD(MASKE *m, short x, short y, short l,
  char **z, char *sw) : MASKEFELD(m,x,y,l,z)
{
  // Das Datenfeld kann mit einem Startwert belegt werden. Dazu dient der
  // Parameter sw.

  if (sw) {
    myfree(daten);
    daten = mystrdup(sw);
  }
}


/**---------------------------------------------------------------------------
  * NUMFELD::NUMFELD()            // constructor
  * 
  * Konstruktor der Klasse, die numerische Felder realisiert. Ruft
  * im wesentlichen MASKEFELD() auf. Auch bei NUMFELD kann
  * ein Zielbereich angegeben werden. Diesmal ist dies allerdings
  * ein Zeiger auf eine long-Zahl.
  *
  * @param
  * MASKE *m: != NULL, dann wird das Feld der Maske m hinzugefuegt.
  * short x,y:    Bildschirmkoordinaten des Eingabebereiches
  * short l:      Laenge des Eingabebereiches.
  * long wert:    Startwert, falls kein Zielbereich angegeben wird
  * long *z:      Zeiger auf long-Zahl, mit der das Feld geladen wird.
  * Bei Ende des Editierens wird das Ergebnis wieder dort-
  * hin geschrieben.
  ---------------------------------------------------------------------------*/
NUMFELD::NUMFELD(MASKE *m, short x, short y, short l,
  long wert, long *z)    : MASKEFELD(m,x,y,l,NULL)
{
  ziel = z;
  if (z) wert = *z;

  char puffer[20];
  sprintf(puffer,"%ld",wert);
  myfree(daten);
  daten = mystrdup(puffer);
};


/**---------------------------------------------------------------------------
  * AUSWAHLFELD::AUSWAHLFELD()    // constructor
  * 
  * Im Auswahlfeld muss eine Liste bereitgestellt werden, aus der
  * der Benutzer auswaehlen kann. Definitionsgemaess steht im Feld
  * nie ein Wert, der nicht aus der Liste ist. Mit der Leertaste
  * wird das naechste Element der Liste ausgewaehlt. Mit einem anderen
  * Zeichen wird das naechste Element ausgewaehlt, das mit diesem
  * Zeichen beginnt.
  *
  * @param
  * MASKE *m: != NULL, dann wird das Feld der Maske m hinzugefuegt.
  * short x,y:    Bildschirmkoordinaten des Eingabebereiches
  * short l:      Laenge des Eingabebereiches.
  * char **z:     Zeiger auf einen Zeiger, Zielbereich!
  * char *sw:     Optionaler Startwert fuer das Feld. Ist dieser Wert
  * nicht auch in der Liste vorhanden, so wird der erste
  * Wert aus der Liste genommen. Dies geschieht auch, wenn
  * kein Startwert angegeben wurde.
  * char **liste: Zeiger auf Stringtabelle mit den moeglichen Eintraegen
  * im Feld. Die Tabelle muss mit einem NULL-Zeiger been-
  * det werden!
  ---------------------------------------------------------------------------*/
AUSWAHLFELD::AUSWAHLFELD(MASKE *m, short x, short y, short l, char **z,
  char *sw, char **liste) : MASKEFELD(m, x, y, l, z)
{
  auswahlliste = liste;
  for (anzahl=0; liste[anzahl]; anzahl++); // Zaehlen

  // Assertion: liste[anzahl] == 0;

  // Falls ein Startwert angegeben wurde, kopiere ich diesen in die Daten.
  
  if (sw) {
    myfree(daten);
    daten = mystrdup(sw);
  }

  // Der Startwert sollte schon mit einem Eintrag der Liste ueberein-
  // stimmen.
  
  aktwahl = -1;
  for (int i=0; i<anzahl; i++) if (daten && !strcmp(liste[i], daten)) aktwahl = i;
  if (aktwahl == -1) { // Wert passt nicht und muss ersetzt werden.
    aktwahl = 0;
    myfree(daten);
    daten = mystrdup(liste[0]);
  }
}


/**---------------------------------------------------------------------------
  * AUSWAHLFELD::edit()
  * 
  * Editierfunktion fuer das Auswahlfeld. Akzeptiert SPACE und
  * alle Zeichen wie im Konstruktor beschrieben.
  *
  * @return
  * EDIT_UP:        Wenn der User ein Feld nach oben will,
  * EDIT_DOWN:      Wenn der User ein Feld nach unten will,
  * EDIT_END:       Wenn der User die Eingabe insgesamt abschliessen will,
  * EDIT_QUIT:      Wenn der User die Eingabe ganz abbrechen will.
  ---------------------------------------------------------------------------*/
short AUSWAHLFELD::edit()
{
  // Bei jedem Space wird eins weitergeflippt, bei einem Buchstaben
  // wird das naechste angeflippt, dass den Anfangsbuchsaben hat.

  io_gotoxy(xpos, ypos);

  while (1) {
    int taste = io_getch();

    if      (io_iscursorup(taste))    return EDIT_UP;
    else if (io_iscursordown(taste) || taste==13)  return EDIT_DOWN;
    else if (io_iscursorleft(taste))  weiterflippen(-1);
    else if (io_iscursorright(taste)) weiterflippen(+1);
    else if (taste == 5) return EDIT_END;
    else if (taste == 1) return EDIT_QUIT;

    else if (taste < 255 && isprint(taste)) { // Suchen.
      short wahl = aktwahl;
      do weiterflippen();
      while (wahl != aktwahl && daten[0] != taste);
    }

  } // Endlosschleife
 
}


/**---------------------------------------------------------------------------
  * AUSWAHLFELD::weiterflippen()
  * 
  * Ersetzt in einem Auswahlfeld den Feldinhalt durch das naechste Ele-
  * ment der Auswahlliste bzw. wieder durch das erste, wenn das Ende
  * erreich war.
  * @param
  * short offset:  Bei +1 wird vorwaerts, bei -1 rueckwaerts geflippt.
  * +1 ist defaultwert.
  ---------------------------------------------------------------------------*/
void AUSWAHLFELD::weiterflippen(short offset)
{
  aktwahl = (aktwahl+offset+anzahl) % anzahl;

  myfree(daten);
  daten = mystrdup(auswahlliste[aktwahl]);
  refresh();
  io_gotoxy(xpos, ypos);
};


/**---------------------------------------------------------------------------
  * MASKEFELD::refresh()
  * 
  * Malt den Feldinhalt an die richtige Stelle am Bildschirm. Fuellt
  * den leeren Rest des Darstellungsbereiches mit Unterstrichen.
  ---------------------------------------------------------------------------*/
void MASKEFELD::refresh()
{
  io_setattr(IO_BOLD);
  io_printxy(xpos, ypos, (char *)(daten ? daten : ""));
  for (int sp=0; sp < laenge - mystrlen(daten); sp++) io_print("_");
  io_setattr(IO_NORMAL);
}


/**---------------------------------------------------------------------------
  * MASKE::felder_refreshen()
  * 
  * Ruft fuer jedes Feld der Maske die Funktion refresh() auf.
  ---------------------------------------------------------------------------*/
void MASKE::felder_refreshen()
{
  MASKEFELD *feld = (MASKEFELD *)first();
  while (!feld->is_tail())
  {
    feld->refresh();
    feld = (MASKEFELD *)feld->next();
  }
}


/**---------------------------------------------------------------------------
  * MASKEFELD::edit_textfeld()
  * 
  * Supportfunktion, die von den edit()-Funktionen der Felder benuetzt
  * werden kann. Sie ist vor allem fuer Textfelder interessant. Der User
  * editiert das Feld innerhalb dieser Funktion, bis er es verlaesst,
  * oder die ganze Maske mit CTRL-E oder CTRL-A beendet.
  *
  * @param
  * short modus:	0 wenn alle Zeichen erlaubt sind, bei 1 sind nur
  * Ziffern und - erlaubt.
  *
  * @return
  * EDIT_UP:        Wenn der User ein Feld nach oben will,
  * EDIT_DOWN:      Wenn der User ein Feld nach unten will,
  * EDIT_END:       Wenn der User die Eingabe insgesamt abschliessen will,
  * EDIT_QUIT:      Wenn der User die Eingabe ganz abbrechen will.
  ---------------------------------------------------------------------------*/
short MASKEFELD::edit_textfeld(short modus)
{
  char *puffer = new char[laenge+1];

  if (daten) strcpy(puffer, daten);
  else puffer[0] = 0;

  io_setattr(IO_BOLD);
  io_printxy(xpos, ypos, puffer); // Cursor steht danach richtig!
  io_setattr(IO_NORMAL);

  int taste;
  do {
    taste = io_getch();
    if (io_isbackspace(taste) && strlen(puffer)) {
      puffer[strlen(puffer)-1] = 0;
      io_setattr(IO_BOLD);
      io_printxy(xpos + strlen(puffer), ypos, "_");
      io_setattr(IO_NORMAL);
      io_gotoxy(xpos + strlen(puffer), ypos);
    }

    else if (io_isbackspace(taste)) { // Feld leer, in vorhergehendes Feld gehen
      myfree(daten);
      daten = puffer; // Leeres Feld uebernehmen.
      return EDIT_UP;
    }

    else if (taste<256 && isprint((char)taste) && (int)(strlen(puffer)) < laenge
              && (!modus || isdigit((char)taste) || taste=='-')) {
      puffer[strlen(puffer)+1] = 0;
      puffer[strlen(puffer)] = taste;
      io_setattr(IO_BOLD);
      io_printchar(taste);
      io_setattr(IO_NORMAL);
    }
  } while (taste != 13 && taste != 1  && taste != 5
           && !io_iscursorup(taste) && !io_iscursordown(taste));

  myfree(daten);
  daten = puffer;

  if (taste == 13 || io_iscursordown(taste)) return EDIT_DOWN;
  else if (taste == 5 )  return EDIT_END;
  else if (taste == 1 ) return EDIT_QUIT;
  else if (io_iscursorup(taste)) return EDIT_UP; // CTRL-P
  else return EDIT_UP; // Huch?
}


/**---------------------------------------------------------------------------
  * TEXTFELD::edit()
  * 
  * Editierfunktion fuer Textfelder.
  *
  * @return
  * EDIT_UP:        Wenn der User ein Feld nach oben will,
  * EDIT_DOWN:      Wenn der User ein Feld nach unten will,
  * EDIT_END:       Wenn der User die Eingabe insgesamt abschliessen will,
  * EDIT_QUIT:      Wenn der User die Eingabe ganz abbrechen will.
  ---------------------------------------------------------------------------*/
short TEXTFELD::edit()
{
  return edit_textfeld();
}


/**---------------------------------------------------------------------------
  * 
  * 
  * Editierfunktion fuer numerische Felder.
  *
  * @return
  * EDIT_UP:        Wenn der User ein Feld nach oben will,
  * EDIT_DOWN:      Wenn der User ein Feld nach unten will,
  * EDIT_END:       Wenn der User die Eingabe insgesamt abschliessen will,
  * EDIT_QUIT:      Wenn der User die Eingabe ganz abbrechen will.
  ---------------------------------------------------------------------------*/
short NUMFELD::edit()
{
  return edit_textfeld();
}
