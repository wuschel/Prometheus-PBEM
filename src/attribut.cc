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
  * MODUL:               attribut.C  /  ATTRIBUT.CPP
  * AUTOR/DATUM:         Mathias Kettner, 1. Mai 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Enthaelt genau die Funktionen von ATTRIBUT_LISTE.
//
// *************************************************************************

#include <string.h>

#include "attribut.h"
#include "laengen.h"
#include "alg.h"
#include "mystring.h"
#include "log.h"

/**---------------------------------------------------------------------------
  * ATTRIBUT::ATTRIBUT()          // construktor
  * 
  * Konstruktor der Attributsklasse. Traegt beide Werte ein.
  *
  * @param
  * k:      Klasse des Attributes
  * w:      Wert des Attributes
  ---------------------------------------------------------------------------*/
ATTRIBUT::ATTRIBUT(char *k, char *w)
{
  klasse = mystrdup(k);
  wert   = mystrdup(w);
}

/**---------------------------------------------------------------------------
  * ATTRIBUT::~ATTRIBUT()          // destruktor
  * 
  * Destruktor, der einfach beide Strings freigibt.
  ---------------------------------------------------------------------------*/
ATTRIBUT::~ATTRIBUT()
{
  myfree(klasse);
  myfree(wert);
}

/**---------------------------------------------------------------------------
  * ATTRIBUT_LISTE::aus_string_einlesen()
  * 
  * Liest in eine Attributsliste eine ganze Reihe von Attributen ein,
  * die Darstellung ist als String:
  * GATTUNG=Siedler,KannBewaessern,ALTER=2
  * Die alten Attribute werden nicht geloescht, ggfls aber durch neue
  * Werte ueberschrieben.
  *
  * @param
  * quelle:         String, in dem die Attribute spezifiziert sind.
  ---------------------------------------------------------------------------*/
void ATTRIBUT_LISTE::aus_string_einlesen(char *quelle)
{
  if (!quelle || !quelle[0]) return;

  // Auch ein einzelner Punkt zaehlt als Leerstring (Wegen Laden/Speichern!)
  if (quelle[0]=='.' && !quelle[1]) return;

  // Da ich den String in lauter kleine schoene Einzelteile zerhacken will,
  // mache ich mir zuerst eine Kopie davon. Dazu benoetige ich insgesamt
  // drei Zeiger:

  char *temp; // Zeiger auf das mem, der sich nicht veraendert (zum Freigeben)
  char *tempmem; // Zeiger zum abscannen des Strings
  char *ende; // An diesem Zeiger erkenne ich, wann der Sring zuende ist

  temp = tempmem = mystrdup(quelle);
  ende = temp+strlen(temp); // Zeigt nun auf die Ende-0.

  char *klasse, *wert;
  
  while (tempmem < ende)
  {
    klasse = tempmem;
    while (*tempmem && *tempmem!='=' && *tempmem!=',') tempmem++;
    if (!*tempmem || *tempmem==',') {  // Binaeres Attribut
      *tempmem++ = 0;
      setzen(klasse);
      continue;
    }
    *tempmem++ = 0;
    wert = tempmem;
    while (*tempmem && *tempmem!=',') tempmem++; // Komma oder Ende suchen
    *tempmem++ = 0; // Terminieren

    // Jetzt muss ich aber noch aus dem Wert eventuell vorhandene
    // Steuerzeichen expandieren.
    string_to_wert(wert);
    setzen(klasse, wert);

  } // while (tempmem < ende)
  myfree (temp);

}

/**---------------------------------------------------------------------------
  ---------------------------------------------------------------------------*/
short ATTRIBUT_LISTE::in_file_ausgeben(FILE *file)
{
  fprintf(file,"%s\n",to_string());
  return (0 != ferror(file));
}


/**---------------------------------------------------------------------------
  * ATTRIBUT_LISTE::setzen()
  * 
  * Setzt ein neues oder altes Attribut auf einen Wert.
  *
  * @param
  * klasse:         Klasse des Attributes
  * wert:           Sein Wert
  ---------------------------------------------------------------------------*/
void ATTRIBUT_LISTE::setzen(char *klasse, char *wert)
{
  loeschen(klasse);
  ATTRIBUT *attr = new ATTRIBUT(klasse, wert);
  insert(attr);
  if (!mystrcmp_no_case(klasse, "TYP")) typ = attr->wert; // cachen: Write through
}

/**---------------------------------------------------------------------------
  * ATTRIBUT_LISTE::loeschen()
  * 
  * Loescht ein Attribut aus der Liste.
  *
  * @param
  * klasse:         Klasse des zu loeschende Attributes
  ---------------------------------------------------------------------------*/
void ATTRIBUT_LISTE::loeschen(char *klasse)
{
    if (!mystrcmp_no_case(klasse, "TYP")) typ = 0; // cachen: Write through
    ATTRIBUT *att = (ATTRIBUT *)find((void *)klasse);
    if (att) delete att;
}

/**---------------------------------------------------------------------------
  * ATTRIBUT_LISTE::abfragen()
  * 
  * Ermittelt zu einem Attribut den Wert, falls das Attribut existiert.
  * Die Funktion ist nicht in der Lage, zwischen einem Attribut mit dem
  * Wert NULL (binaeres Attribut) und einem nicht vorhandnenen Attribut
  * zu unterscheiden. Dazu gibt es die Funktion gesetzt().
  *
  * @param
  * klasse:         Klasse des abzufragenden Attributes
  *
  * @return
  * Zeiger auf den Wert des Attributes oder NULL, wenn das Attribut
  * nicht gesetzt ist oder der Wert NULL ist. Der Zeiger zeigt direkt
  * in die Daten der Attributliste. Er ist also absolut READ-ONLY and
  * SUBJECT-TO-CHANGE, sobald die naechste Attributsfunktion aufgerufen
  * wird. Wer damit weiterarbeiten will, muss mit mystrdup() eine Kopie
  * anlegen.
  ---------------------------------------------------------------------------*/
char *ATTRIBUT_LISTE::abfragen(const char *klasse)
{
    if (!mystrcmp_no_case(klasse, "TYP")) return typ; // Aus dem cache holen.

    ATTRIBUT *att = (ATTRIBUT *)find((void *)klasse);
    if (att) return att->wert;
    else return NULL;
}

/**---------------------------------------------------------------------------
  * ATTRIBUT_LISTE::referenz_zu()
  * 
  * Funktioniert wie abfragen, liefert aber einen Zeiger auf die
  * Pointervariable, die auf den Wert des Attributes zeigt. Damit
  * kann das Attribut von aussen geaendert werden. Der alte Wert
  * muss mit myfree() freigegeben werden und der neue mit mystrdup()
  * hineinkopiert.
  * Wenn das Attribut garnicht existiert, wird es automatisch angelegt.
  *
  * Fuer das Attribut "TYP" kann keine Referenz erzeugt werden!
  * 
  * @return
  * char ** Gibt immer einen Zeiger auf einen Attributswert zurueck
  ---------------------------------------------------------------------------*/
char **ATTRIBUT_LISTE::referenz_zu(char *klasse)
{
    if (!mystrcmp_no_case(klasse, "TYP")) {
	log('I', "ATTRIBUT_LISTE::referenz_zu(\"TYP\")");
	return 0;
    }

    ATTRIBUT *att = (ATTRIBUT *)find((void *)klasse);
    if (att) return &(att->wert);
    else {
	setzen(klasse);
	return referenz_zu(klasse);
    }
}


/**---------------------------------------------------------------------------
  * ATTRIBUT_LISTE::gesetzt()
  * 
  * Ermittelt, ob ein Attribut gesetzt ist.
  *
  * @param
  * klasse:         Klasse des zu pruefenden Attributes
  *
  * @return
  * 1, falls das Attribut gesetzt ist,
  * 0, falls nicht.
  ---------------------------------------------------------------------------*/
short ATTRIBUT_LISTE::gesetzt(char *klasse)
{
    if (!mystrcmp_no_case(klasse, "TYP")) return (typ != 0);

    else return (find((void *)klasse) != NULL);
}


/**---------------------------------------------------------------------------
  * ATTRIBUT_LISTE::passen_in()
  * 
  * Vergleicht mit der impliziten Attributsliste eine weitere
  * und prueft, ob die Attribute der impliziten Liste alle
  * in die gegebene passen, d.h. ob alle Attribute dort auch
  * gesetzt sind und den gleichen Wert haben.
  *
  * @param
  * obermenge:      Attributsliste, die die vermeindliche Obermenge
  * ist.
  *
  * @return
  * 1, falls die Attribute in die Obermenge passen,
  * 0, falls nicht.
  ---------------------------------------------------------------------------*/
short ATTRIBUT_LISTE::passen_in(ATTRIBUT_LISTE *obermenge)
{
  // Die Attribute passen genau dann in die Obermenge, wenn jedes
  // Attribut in der Obermenge auch enthalten ist und zudem noch
  // den gleichen Wert hat. Es durfen in der Obermenge natuerlich
  // zusatzlich andere Attribute enthalten sein.

  ATTRIBUT *test = (ATTRIBUT *)first();
  while (!test->is_tail())
  {
    if (!test->wert) { if (!obermenge->gesetzt(test->klasse)) return 0; }
    else if (mystrcmp(test->wert, obermenge->abfragen(test->klasse))) return 0;
    test = (ATTRIBUT *)test->next();
  }
  return 1;
}



void ATTRIBUT_LISTE::subtrahiere(ATTRIBUT_LISTE *sub)
{
  ATTRIBUT *attr = (ATTRIBUT *)sub->first();
  while (!attr->is_tail()) {
    loeschen(attr->klasse);
    attr = (ATTRIBUT *)attr->next();
  }
}


/**---------------------------------------------------------------------------
  * ATTRIBUT::matches()
  * 
  * Wird zum Suchen mit der Funktion find() gebraucht und stellt
  * fest, ob das gerade untersuchte Attribut einen bestimmten
  * Namen bestizt.
  *
  * @param
  * krit:           (void *)Zeiger auf den Namen des Attributs.
  *
  * @return
  * 1, falls es sich um das gesuchte Attribut handelt,
  * 0, sonst.
  ---------------------------------------------------------------------------*/
short ATTRIBUT::matches(void *krit)
{
    // printf("@@%s\n",(char *)krit);
    return !mystrcmp_no_case((char *)krit,  klasse);
}


void ATTRIBUT_LISTE::wert_to_string(char *string, char *wert)
{
  char *ziel = string;
  char *quelle = wert;

  while (*quelle) {
    if (*quelle<=32 || *quelle=='\\' || *quelle>=127 || *quelle=='=' || *quelle==',')
    {
      sprintf(ziel,"\\%03d",*quelle++);
      ziel+=4;
    }
    else *ziel++ = *quelle++;
  }
  *ziel = 0; // String noch terminieren!
}


char *ATTRIBUT_LISTE::to_string(char *trenn)
{
  // Wenn die Attributsliste leer ist, dann ist der String ein einziger
  // Punkt.

  if (is_empty()) return ".";

  // Hier kommt eine einigermassen heikle Stelle, da ich einen relativ
  // grossen Speicherbereich fuer eine relativ kleine Funktion reservie-
  // ren muss. Hinzu kommt, dass jeder Aufruf dieser Funktion jedes
  // Ergebnis eines alten Aufrufes zerstoert (falls dieses nicht vorher
  // kopiert wurde). Das liegt daran, dass der Ausgabestring in einen
  // konstanten, statisch angelegten Puffer geschrieben wird, der hier
  // in der Funktion bleibt und immer wieder verwendet wird. Er hat eine
  // Laenge von um die 4096 Bytes, je nach Eintrag in limits.h

  static char string[MAX_LAENGE_ATTRIBUTSZEILE+2];

  string[0] = 0;
  ATTRIBUT *attribut = (ATTRIBUT *)first();
  while (!attribut->is_tail())
  {
    strcat(string, attribut->klasse);
    if (attribut->wert) {
      strcat(string,"=");
      wert_to_string(string+strlen(string),attribut->wert);
    }
    attribut = (ATTRIBUT *)attribut->next();
    if (!attribut->is_tail()) strcat(string, trenn ? trenn : ",");
  }
  return string;
}
