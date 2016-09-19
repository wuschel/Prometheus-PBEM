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
  * MODUL:               alg.C  /  ALG.CPP
  * AUTOR/DATUM:         Mathias Kettner, 17.1.1994
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Enthaelt alle Funktionen, die nicht zu einem Objekt gehoeren, und
//      die gleichzeitig fuer das komplette Objektsystem unabhaengig vom
//      konkreten Modell zur Verfuegung stehen. Enthaelt also keine
//      Prometheusspezifischen Funktionen
//
// *************************************************************************

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "objekt.h"
#include "alg.h"
#include "kompatib.h"
#include "listmac.h"
#include "log.h"

FILE *drucker_redirekt = NULL;

extern DOUBLIST globale_objekt_menge;



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short fputhex(FILE *file, char *buffer, long laenge)
{
  short status = 0;
  int wrap = 24;

  while (laenge)
  {
    fprintf(file,"%02x ",((unsigned short)*buffer++) & 0xff);
    status |= (0 != ferror(file));
    laenge--;
    wrap--;
    if (!wrap && laenge) { // Nur wrappen, wenn noch Bytes kommen!
       fputs("\n",file);
       wrap = 24;
    }
  }
  fputs("\n",file);
  return status;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short fgethex(FILE *file, char *buffer, long laenge)
{
  long wert;
  while (laenge)
  {
    fscanf(file,"%lx",&wert);
    if (ferror(file)) return 1;
    *buffer++ = (char)wert;
    laenge--;
  }
  return 0;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short fputhex(FILE *file, unsigned short *buffer, long laenge)
{
  short status = 0;
  int wrap = 16;

  while (laenge)
  {
    fprintf(file,"%04x ",*buffer++);
    status |= (0 != ferror(file));
    laenge--;
    wrap--;
    if (!wrap && laenge) { // Nur wrappen, wenn noch ushorts kommen!
       fputs("\n",file);
       wrap = 16;
    }
  }
  fputs("\n",file);
  return status;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short fgethex(FILE *file, unsigned short *buffer, long laenge)
{
  long wert;
  while (laenge)
  {
    fscanf(file,"%lx",&wert);
    if (ferror(file)) return 1;
    *buffer++ = (unsigned short)wert;
    laenge--;
  }
  return 0;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void fputstring(FILE *file, char *quelle, char *trenn)
{
  // Wenn der String leer ist, dann schreibe ich die Markierung
  // \.

  if (!quelle || !*quelle) fprintf(file,"\\.");

  else while (*quelle) {
    if (*quelle<=32 || *quelle=='\\' || *quelle>=127)
      fprintf(file,"\\%03d",*quelle++);
    else fputc(*quelle++, file);
  }
  if (trenn) fprintf(file, "%s", trenn);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
char *string_to_wert(char *wert)
{
  char *antwort = wert;

  // Der leere String ist durch \. gekennzeichnet.
  if (!strcmp(wert,"\\.")) *wert=0;

  else {
    char *quelle = wert;
    char *ziel = wert;  // Geht,  da Code verkuerzt.

    while (*quelle) {
      if (*quelle != '\\') *ziel++ = *quelle++;
      else {
	quelle++;
	char konvert[4];
	konvert[0] = *quelle++;
	konvert[1] = *quelle++;
	konvert[2] = *quelle++;
	konvert[3] = 0;
	*ziel++ = (char)atoi(konvert);
      }
    }
    *ziel=0;
  }
  return antwort;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
char *eindeutiger_name()
{
  static long laufende_nummer = 1;
  static char name[20];

  sprintf(name, "Name%ld",laufende_nummer);
  laufende_nummer++;
  return name;
}



// globale Version von...
// *************************************************************************
// FUNKTION:
// 
// *************************************************************************
OBJEKT *objekt_mit_namen(char *name)
{
  OBJEKT_LIST_NODE *node;
  node = (OBJEKT_LIST_NODE *)globale_objekt_menge.find((char *)name);
  if (!node) return NULL;
  else return node->objekt;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void vorgemerkte_objekte_vernichten()
{
  OBJEKT_LIST_NODE *del = (OBJEKT_LIST_NODE *)globale_objekt_menge.first();
  while (!del->is_tail())
  {
    if (del->objekt->zur_vernichtung_vorgemerkt()) {
      delete del->objekt;
      del = (OBJEKT_LIST_NODE *)globale_objekt_menge.first();
    }
    else del = (OBJEKT_LIST_NODE *)del->next();
  }
}



/**---------------------------------------------------------------------------
  * eindeutige_objekt_abkuerzung()
  * 
  * Berechnet aus einem Namen eine Abkuerzung, in dem eine Teilmenge
  * aus den Zeichen ausgewaehlt wird. Es werden auf diese Art solange
  * Teilmengen generiert, bis eine im Objektsystem eindeutige Abkuer-
  * zung gefunden ist, d.h. dass kein anderes Objekt einen Namen hat,
  * der mit der Abkuerzung gleichlautet. Sind alle Teilmenge erfolglos
  * ausprobiert worden, so wird ein Fehler gemeldet.
  *
  * @param
  * ausgeschrieben: Der abzukuerzende Name.
  * laenge:         Anzahl der Zeichen der Abkuerzung. Die Laenge des
  * ausgeschriebenen Namens muss auf jeden Fall minde-
  * stens um eins groesser sein als die gewuenschte
  * Laenge der Abkuerzung.
  *
  * @return
  * Zeiger auf die Abkuerzung (static) oder NULL, wenn keine eindeutige
  * Abkuerzung gefunden werden konnte.
  ---------------------------------------------------------------------------*/

	// Kleine Hilfsfunktion...

	short naechste_k_teilmenge(short *p, short k, short n)
	{
	  if (k==0 || n<k) return 1; // Es gibt keine k-Teilmenge!
	  p[k-1]++;
	  if (p[k-1] >= n) {
	    if (naechste_k_teilmenge(p, k-1, n-1)) return 1;
	    p[k-1] = p[k-2]+1;
	  }
	  return 0;
	}


char *eindeutige_objekt_abkuerzung(char *klangname, short laenge)
{
  char ausgeschrieben[100];
  sprintf(ausgeschrieben,"%sabcdefghijklmnopqrstuvwxyz", klangname);
  static char antwort[32]; // Hier wird sie aufgebaut
  static short teilmenge[32]; // Zur Bildung einer Teilmenge

  if (laenge > 31) {
      log('I', "eindeutige_objekt_abkuerzung(): grosse Laenge %ld", laenge);
      return NULL;
  }

  // Jetzt initialisiere ich das Teilmengenfeld
  for (int i=0; i<laenge; i++) teilmenge[i]=i;
  antwort[laenge] = 0;
  do {
    short unbrauchbar = 0;
    for (int i=0; i<laenge; i++) {
      antwort[i] = tolower(ausgeschrieben[teilmenge[i]]);
      // Umlaute werden umgewandelt!
      switch ((unsigned char)(antwort[i])) {
	case 142:
	case 132: antwort[i] = 'a'; break;
	case 153:
	case 148: antwort[i] = 'o'; break;
	case 154:
	case 129: antwort[i] = 'u'; break;
	case 225: antwort[i] = 's'; break;
      }
      // Wenn es sich nich um das erste Zeichen in der Antwort handelt,
      // dann sind auch noch die Ziffern 0-9 zugelassen. Fuer das erste
      // Zeichen sind nur die Kleinbuchstaben a-z zugelassen.

      unbrauchbar |= (!(islower(antwort[i]) || (i && isdigit(antwort[i]))));

    }
    if (unbrauchbar) continue;

    // Und muss ich schauen, ob es das Objekt nicht schon gibt...
    if (!objekt_mit_namen(antwort)) break; // Gut.
  }
  while (!naechste_k_teilmenge(teilmenge, laenge, strlen(ausgeschrieben)));

  if (objekt_mit_namen(antwort)) {
      log('I', "eindeutige_objekt_abkuerzung(): Kein Eindeutiger Name festlegbar"
	  "fuer vollen Namen '%s'", klangname);
      return NULL;
  }
  return antwort;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void io_deleteline(int y0, int y1)
{
  for (int i=y0; i<=y1; i++) io_deleteline(i);
};



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short drucker_init(char *filename)
{
  if (filename) {
    if (drucker_redirekt) fclose(drucker_redirekt);
    verzeichnis_gewaehrleisten(filename);
    drucker_redirekt = fopen(filename, "w");
    if (drucker_redirekt) return 0;
    else return 1;
  }
  else {
    return io_init_printer();
  }
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void drucker_close()
{
  if (drucker_redirekt) {
    fclose(drucker_redirekt);
    drucker_redirekt = NULL;
  }

  else io_close_printer();
  drucker_redirekt = NULL;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void drucken(char *text)
{
  if (drucker_redirekt) fprintf(drucker_redirekt, "%s", text);
  else io_printer(text);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void drucken(char c)
{
  char ch[2];
  ch[1]=0;
  ch[0]=c;
  drucken(ch);
}

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void drucken_raw(unsigned char *daten, long laenge)
{
  if (drucker_redirekt) fwrite(daten, 1, laenge, drucker_redirekt);
  else io_printer_raw(daten, laenge);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void drucken(char *format, char *s1, char *s2, char *s3)
{
  char *tempstring = new char[4096];
  sprintf(tempstring,format, s1, s2, s3);
  drucken(tempstring);
  delete tempstring;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void drucken(char *format, long l1, long l2, long l3)
{
  char *tempstring = new char[4096];
  sprintf(tempstring,format, l1, l2, l3);
  drucken(tempstring);
  delete tempstring;
}


/**---------------------------------------------------------------------------
  * filtere_objekt_liste()
  * 
  * Dient dazu, in einer Liste von OBJEKT_LIST_NODE Eintraegen
  * alle Objektnodes zu entfernen, deren Objekte nicht bestimmten
  * Bedingungen genuegen. Die Bedingungen sind in Form von Attribut-
  * strings gegeben, die in die Attribute eines Objektes passen muss.
  *
  * @param
  * DOUBLIST *objliste: Liste von OBJEKT_LIST_NODE-Nodes(!)
  * char *pos:                  Bedingung, die erfuellt sein muss, damit
  * die Node weiterhin in der Liste bleiben
  * darf (z.B."TYP=EINHEIT,Boden")
  * char *neg:                  Bedingung, die nicht erfuellt sein darf,
  * damit die Node weiterhin in der Liste
  * bleibt (z.B."Unversorgt")
  ---------------------------------------------------------------------------*/
void filtere_objekt_liste(DOUBLIST *objliste, char *pos, char *neg)
{
  OBJEKT_LIST_NODE *objnode;
  SCAN(objliste, objnode) {
    if (!objnode->objekt->bedingung_erfuellt(pos)
     || (neg && objnode->objekt->bedingung_erfuellt(neg)))
    {
      delete objnode;
      FIRST(objliste, objnode);
    }
    else NEXT(objnode);
  }
}


/**---------------------------------------------------------------------------
  * filesize()
  * 
  * Ermittelt die Laenge eines offenen Files in Bytes. Dabei wird die
  * aktuelle Position des Filepointers nicht veraendert.
  * @param
  * FILE *file:     offenes File.
  * @return
  * long:   Laenge in Butes.
  ---------------------------------------------------------------------------*/
long filesize(FILE *file)
{
   long position, laenge;

   position = ftell(file); // Alte Position merken
   fseek(file, 0L, SEEK_END);
   laenge = ftell(file);
   fseek(file, position, SEEK_SET);
   return laenge;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short file_exists(char *name)
{
  FILE *file = fopen(name, "r");
  if (!file) return 0;
  else {
    fclose(file);
    return 1;
  }
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
char *get_file(char *name)
{
  FILE *file;
  file = fopen(name, "r");
  if (!file) return NULL;

  long laenge = filesize(file);
  char *puffer = new char[laenge+1];

  long actual = fread(puffer, 1, laenge, file);
  if (actual <= 0 && actual != laenge)  {
      log('W', "io error while reading file '%s': Read %ld bytes instead of %ld",
	  name, actual, laenge);
      delete puffer;
      fclose(file);
      return NULL;
  }

  // Nun hat wohl alles geklappt.

  fclose(file);
  puffer[actual] = 0; // Das ganze noch als String beenden.
  return puffer;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short adressliste_speichern(FILE *file, DOUBLIST *liste)
{
  fprintf(file, "%ld ",liste->number_of_elements());
  FOR_EACH_ADR_IN (liste) DO
  (
    fprintf(file, "%s ",adr.to_string());
  )

  fprintf(file,"\n");
  return (ferror(file) != 0);
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
short adressliste_laden(FILE *file, DOUBLIST *liste)
{
  long anzahl;
  fscanf(file, "%ld", &anzahl);

  ADR_LIST_NODE *adrnode;
  for (;anzahl; anzahl--)
  {
    adrnode = new ADR_LIST_NODE;
    adrnode->adresse = ADR(file);
    liste->insert(adrnode);
    if (ferror(file)) return 1;
  }
  return (ferror(file) != 0);
}

/**---------------------------------------------------------------------------
  * verzeichnis_gewaerleisten();
  * 
  * Stellt sicher, dass alle im Pfad vorkommenden Verzeichnisse 
  * existieren. Im Zweifelsfall werden die fehlenden Verzeichnisse
  * erzeugt.
  * @param
  * char *pfad: Der Pfad einer Datei (!). Alle Komponenten bis zum
  * letzten "/" werden als Verzeichnisse angesehen.
  ---------------------------------------------------------------------------*/
void verzeichnis_gewaehrleisten(char *pfad)
{
  char *kopie = mystrdup(pfad);
  char *w=kopie;
  while (*w) {
    w++;
    if (*w == '/') {
      *w=0;
      io_mkdir(kopie);
      *w='/';
    }
  }
  myfree(kopie);
}
