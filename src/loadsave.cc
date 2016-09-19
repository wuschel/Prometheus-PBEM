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
  * MODUL:               loadsave.C / LOADSAVE.CPP
  * AUTOR/DATUM:         Mathias Kettner, 23. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Dieses Modul beinhaltet alle Funktionen, die zum Laden und
//      Speichern eines ganzen Objektsystemes inclusive aller Relationen
//      noetig sind. Die meisten Funktionen sind member-Funktionen.
//      
// *************************************************************************

#include <ctype.h>
#include <string.h>

#include "objekt.h"
#include "laengen.h"
#include "zielgrup.h"
#include "einfluss.h"
#include "kompatib.h"
#include "alg.h"
#include "log.h"

extern DOUBLIST globale_objekt_menge;
extern EINFLUSS_LISTE globale_einfluss_menge;

/**---------------------------------------------------------------------------
  * OBJEKT::objekte_speichern()
  * 
  * Diese Funktion speichert einen ganzen Objektbaum als ASCII-Datei
  * ab. Ausgegangen wird davon von einem Wurzelobjekt (this), dessen
  * Name und Typ aber NICHT mitabgespeichert werden. Die Funktion
  * arbeitet rekursiv. Beeinflussungsrelationen werden nicht mit-
  * abgespeichert. Die Objektspezifischen Daten werden von der vir-
  * tuell zu ueberladenden Funktion OBJEKT::speichern() abgespei-
  * chert.
  *
  * @param
  * file:           Filepointer (fopen()), falls in ein schon geoeff-
  * netes File geschrieben werden soll. Der filename
  * wird dann ignoriert. Ansonsten NULL.
  * filename:       Name der Datei, falls sie neu geoeffnet werden soll.
  * wenn der Filepointer nicht uebergeben wurde, so
  * muss der Name ungleich NULL sein. sonst NULL.
  *
  * @return
  * 1 im Fehlerfall, 0 sonst.
  ---------------------------------------------------------------------------*/
short OBJEKT::objekte_speichern(FILE *file, char *filename)
{
  // Einer der beiden Parameter sollte 0 sein, der andere != 0!
  // Uebrigens: Ich muss mir natuerlich unbedingt merken, ob ich das File
  // selbst geoeffnet habe und folglich am Ende wieder schliessen muss,
  // oder ob das File meinem Aufrufer gehoert und ich dann gefaelligst
  // die Finger davon zu lassen habe, wenn ich meine Daten geschrieben
  // habe, da es sein koennte, dass er noch Daten schreibt und ausserdem
  // da er es selbst schliesst. Zum merken nehme ich eine aussagekraeftige
  // Variable:

  short ich_selbst_habe_das_file_geoeffnet = 0;

  if (!file) {
    verzeichnis_gewaehrleisten(filename);
    file = fopen(filename,"w");
    ich_selbst_habe_das_file_geoeffnet = 1;
  }

  if (!file) return 1; // Fehler!
  
  short status = 0;
  
  status |= besitztum_speichern(file);

  fprintf(file, "R>%ld %d %ld\n",phasenzaehler, vernichtenswert,
		namenszaehler);

  // Jetzt speichere ich den Ort und die Adresse ab. Dabei ergiebt sich
  // leider ein Problem: Die Referenz auf den Ort kann innerhalb der
  // Abspeicherungsdatei nur ueber einen Objektnamen erfolgen, waehrend im
  // geladenen Zustand die Verknuepfung ueber Zeiger realisiert ist. Beim
  // Laden ist aber im Allgemeinen nicht garantiert, dass ein Objekt erst
  // nach seinem Ort (Der Ort ist ja auch ein Objekt) geladen wird, ja viel-
  // mehr ist es im Regelfall sogar genau umgekehrt. Folglich kann beim
  // Laden eines Objektes die Referenz auf seinen Ort im Regelfall noch
  // nicht hergestellt werden. Dieses Problem muss jedoch die Routine
  // objekte_laden() loesen. Ich speichere hier auf jeden Fall den Namen
  // des Ortes ab.

  if (!ort()) fprintf(file,"O>. ");
  else  fprintf(file, "O>%s ",ort()->name);
  adresse.to_file(file);
  fprintf(file, "\n");

  fprintf(file, "B>");
  status |= befehlsliste_in_file(file);

  fprintf(file,"S>\n");
  status |= speichern(file);
  fprintf(file,"\n.\n");

  if (ich_selbst_habe_das_file_geoeffnet) fclose(file);

  return status;
}


/**---------------------------------------------------------------------------
  * OBJEKT::besitztum_speichern()
  * 
  * Private Funktion, die nur von OBJEKT::objekte_speicher()
  * aufgerufen wird und rekursiv fuer jedes Objekt im Besitztum des
  * Objekts mittels OBJEKT::objekte_speichern() dessen Objekt-
  * Baum abspeichert.
  *
  * @param
  * file:           Filepointer (fopen()) der Datei.
  *
  * @return
  * 1 im Fehlerfall, 0 falls alles glatt ging.
  ---------------------------------------------------------------------------*/
short OBJEKT::besitztum_speichern(FILE *file)
{
  short status=0; // 1 bedeutet Fehler (wegen einfacher ODER-Verknuepfung)
  OBJEKT *objekt;
  objekt = (OBJEKT *)besitztum.first();
  while (!objekt->is_tail())
  {
    fprintf(file, ">%s %s %s\n", objekt->name,objekt->attribut("TYP"),
       objekt->attribute.to_string());
    status |= objekt->objekte_speichern(file);
    objekt=(OBJEKT *)objekt->next();
  }
  return status;
}

/**---------------------------------------------------------------------------
  * OBJEKT::befehlsliste_in_file()
  * 
  * Private Funktion, die nur von OBJEKT::objekte_speichern()
  * aufgerufen wird und alle Befehle in der Befehlsliste abspeichert.
  *
  * @param
  * file:           Filepointer (fopen())
  *
  * @return
  * 1, im Fehlerfall, 0 sonst.
  ---------------------------------------------------------------------------*/
short OBJEKT::befehlsliste_in_file(FILE *file)
{
  short status = 0;
  
  // Als erstes muss ich die Anzahl ermitteln.
  fprintf(file,"%ld",befehlsliste.number_of_elements());

  // Jetzt der Reihe nach alle Befehle als Zeilen
  BEFEHL *befehl = (BEFEHL *)befehlsliste.first();
  while (!befehl->is_tail())
  {
     fprintf(file," %s",befehl->befehlstext);
     status |= (0 != ferror(file));
     befehl = (BEFEHL *)befehl->next();
  }
  fputc('\n',file); // Zeile beenden
  return status;
}

/**---------------------------------------------------------------------------
  * OBJEKT::befehlsliste_aus_file()
  * 
  * Private Funktion, die nur von OBJEKT::objekte_laden() aufgerufen
  * wird und aus einer ASCII-Datei die Befehlsliste fuer diese Objekt
  * neu laedt. Es wird davon ausgegangen, dass keine Befehle einge-
  * lastet sind!
  *
  * @param
  * file:           Filepointer (fopen())
  *
  * @return
  * 1, fall irgendwas schiefging, 0 sonst.
  ---------------------------------------------------------------------------*/
short OBJEKT::befehlsliste_aus_file(FILE *file)
{
  short status = 0;

  // Als erster Parameter kommt die Anzahl der Befehle

  long anzahl=0;
  fscanf(file, "%ld",&anzahl);
  status |= (0 != ferror(file));
  if (!anzahl) return status;

  char *buffer = new char[MAX_LAENGE_BEFEHL+2];

  while (anzahl)
  {
    do {
      *buffer = 0;
      fscanf(file, "%s",buffer);
      if (feof(file)) {
	delete buffer;
	log('E', "Corrupted (truncated) game file");
	return 1;
      }
    } while (isspace(*buffer) || *buffer==10 || *buffer==13);
    status |= (0 != ferror(file));
    befehl_erteilen(buffer);
    anzahl--;
  }

  delete buffer;
  return status;
}


/**---------------------------------------------------------------------------
  * OBJEKT::objekte_laden()
  * 
  * Laedt rekursiv eine Objektbaum aus einer Datei, die mit OBJEKT::
  * objekte_speichern() erstellt wurde. Dieses Objekt muss bereits
  * existieren, sein Name und sein Typ werden vom Laden nicht veraendert.
  * Eventuelle Besitztuemer werden vorher vernichtet! Die geladene Ob-
  * jekte werden neu geschaffen.
  *
  * Hier kommt eine kurze Auflistung der Chunks:
  *
  * ><name> <typ> <attribute> ... R> O> B> "usw.: Objekt im Besitztum"
  * R><phasenzaehler> <vernichtenswert> <namenszaehler>
  * O><ort> <adresse> "Ort und Adresse. Ort als Name"
  * B><anzahl> <befehl 1> <befehl 2> ... <befehl anzahl> "Befehle"
  * S> ...: "Objektspezifische Daten"
  *
  * @param
  * file:           Filepointer (fopen()) oder NULL, wenn ein Name
  * angegeben wird mit dem eine neue Datei erst ge-
  * oeffnet werden soll.
  * filename:       Wird ignoriert, wenn ein Filepointer uebergeben
  * wird. Ansonsten wird mit diesem Namen die
  * Datei geoeffnet, aus der gelesen werden soll.
  *
  * @return
  * 1, falls irgendwo ein Fehler auftrat. 
  * 0, falls alles glatt lief.
  ---------------------------------------------------------------------------*/
short OBJEKT::objekte_laden(FILE *file, char *filename)
{
  // Einer der beiden Parameter sollte 0 sein, der andere != 0!
  
  if (file) filename = NULL;
  else file = fopen(filename,"r");
  if (!file) return 1; // Fehler!

  short status = 0;
  
  // Jetzt kommen verschiedene Sektionen, die mit einem Buchstaben oder '>'
  // beginnen. Wenn ein '.' am Anfang kommt, dann handelt es sich um
  // das Ende dieses Objekts.
  
  char *buffer = new char[MAX_LAENGE_ZEILE+2];
  while  (!feof(file)) // Hoffentlich klappt das so mit feof()
  {

    // Jetzt hole ich ein Zeichen, da ich in meinem Format daran erkennen
    // kann, welcher Datensatz nun kommen muss. Dabei ueberlese ich aber
    // Whitespaces und vor allem Zeilenenden, die am Ende der vorhergegan-
    // genen Zeile noch uebergeblieben sein koennen.

    char zeichen;
    do {
      zeichen = fgetc(file);
    } while ((isspace(zeichen) || zeichen==10 || zeichen==13)
    	      && !feof(file) && !ferror(file));

    switch(zeichen) {
      case '.': // Objekt zuende
        if (filename) fclose(file);
        delete buffer;
        return status;

      case '>': // Objekt im Eigentum, das rekursiv geladen werden soll.
	status |= unterobjekt_laden(file);
        break;

      case 'O': // Ort und Adresse
	status |= (fgetc(file) != '>');

	// Hier ist das Dilemma: Ich bekomme den Ort als Namen eines Objektes,
	// muss aber davon ausgehen, dass dieses Objekt noch nicht existiert.
	// Aus diesem Grund gibt es in der Objektstruktur einen Eintrag
	// namens 'temp_ort_name', der solange den Namen des Ortes speichert,
	// bis die Referenzen aufgeloest werden koennen. Achtung: Ich DARF
	// an dieser Stelle sogar nicht einmal versuchen, die Referenz auf-
	// zuloesen, da waehrend des Ladens alte und neue Objekte gemischt
	// auftreten koennen und die Referenz faelschlicherweise auf ein
	// altes Objekt zeigen wuerde!

	fscanf(file, "%s", buffer);
	if (strcmp(buffer, ".")) temp_ort_name = mystrdup(buffer);
	adresse = ADR(file);
	break;

      case 'R': // Zusatzdaten
	status |= (fgetc(file) != '>');
	fscanf(file,"%ld%hd%ld",&phasenzaehler, &vernichtenswert, &namenszaehler);
	break;

      case 'B': // Befehlsliste
	status |= (fgetc(file) != '>');
	status |= befehlsliste_aus_file(file);
        break;

      case 'S': // Spezifische Daten
	fgets(buffer, (int)MAX_LAENGE_ZEILE+1, file);
        status |= ( (strcmp(buffer, ">\n") && strcmp(buffer,">\r\n")) );
        status |= laden(file);
        break;

      default: // Fehler
	fgets(buffer, (int)MAX_LAENGE_ZEILE+1, file);
	log('E', "Corrupted game file contains invalid line '%c%s'",
	    zeichen, buffer);
        status = 1;

    } // switch (zeichen)
  } // while (File nicht zuende)

  delete buffer;
  if (filename) fclose (file);
  log('E', "Unexpected end of game file");
  return 1;
}

/**---------------------------------------------------------------------------
  * OBJEKT::unterobjekt_laden()
  * 
  * Laedt die Daten eines Objekts aus einer Datei und schafft aufgrund
  * dieser ein neues Objekt.
  *
  * @param
  * file:           Filepointer (fopen())
  * 
  * @return
  * 1, falls ein Fehler auftrat.
  * 0, sonst.
  ---------------------------------------------------------------------------*/
short OBJEKT::unterobjekt_laden(FILE *file)
{
  short status = 0;

  // Zuerst kommt der Name, dann der Typ des neuen Objektes.
  char gew_name[MAX_LAENGE_NAME+2], gew_typ[MAX_LAENGE_ATTRIBUTSWERT];

  // In der naechsten Zeile stehen dann die Attribute. Ich muss diese
  // Zeile erst in einen Zwischenspeicher laden, den ich besser dynamisch
  // anlege...

  char *attr = new char [MAX_LAENGE_ATTRIBUTSZEILE+2];

  fscanf(file, "%s%s%s", gew_name, gew_typ, attr);
  status |= (0 != ferror(file));

  // Jetzt muss ich noch das Attribut "++LADEN++" anhaengen, damit der
  // Konstruktor des Objektes Bescheid weiss, dass er z.B. nicht eine
  // neue Welt erschaffen soll.

  ATTRIBUT_LISTE attrliste(attr);
  attrliste.setzen("++LADEN++");
  delete attr; // Wird schon jetzt nicht mehr gebraucht

  // So. Bevor ich das neue Objekt schaffe, schaue ich erst, ob es nicht
  // schon ein Objekt mit dem gleichen Namen gibt. Dieser Fall ist nach
  // neuesten Konventionen NICHT MEHR ERLAUBT! Deshalb gebe ich in diesem
  // Fall eine Fehlermeldung aus und breche sofort ab!


  if (objekt_mit_namen(gew_name)) {
      log('E',"Corrupted game file: duplicate object name '%s'", gew_name);
      return 1;
  }

  // Jetzt kann ich das neue Objekt endlich schaffen...

  char *attrstring = mystrdup(attrliste.to_string());
  OBJEKT *objekt = objekt_schaffen(gew_name, gew_typ, attrstring);
  myfree (attrstring);

  if (!objekt) return 1; // Konnte Objekt nicht schaffen

  // An dieser Stelle ist bei dem neuen Objekt bereits der Konstruktor
  // und ggbfls. die Funktion laden() ausgefuehrt worden. Deshalb kann
  // ich das Attribut ++LADEN++ wieder loeschen.

  objekt->attribut_loeschen("++LADEN++");

  // Jetzt lade ich die restlichen Daten
  status |= objekt->objekte_laden(file);

  // Und das war's auch schon.
  return status;
}


void OBJEKT::laden_abschliessen()
{
  // Fuer alle geladenen Objekte die Funktion ort() aufrufen!
  ort();
  OBJEKT *objekt = (OBJEKT *)besitztum.first();
  while (!objekt->is_tail())
  {
    objekt->laden_abschliessen();
    objekt = (OBJEKT *)objekt->next();
  }
}


