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
  * MODUL:               enzyklop.C  /  ENZYKLOP.CPP
  * AUTOR/DATUM:         Mathias Kettner, 2. Juli 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Enthaelt alle Funktionen der Klassen ENZYKLOPAEDIE und der
//      ihr untergeordneten Klassen (verschiedenen Eintragstypen)
//
// **************************************************************************

#include <ctype.h>
#include <unistd.h>

#include "enzyklop.h"
#include "kompatib.h"
#include "listmac.h"
#include "uhr.h"
#include "log.h"

extern UHR *g_uhr; // von main.cpp


/**---------------------------------------------------------------------------
  * VORHABEN_ENZ::~VORHABEN_ENZ()    | destruktor
  * 
  * Gibt die dynamisch angelegten Strings frei.
  ---------------------------------------------------------------------------*/
VORHABEN_ENZ::~VORHABEN_ENZ()
{
  myfree(name);
  myfree(einfluss_art);
  myfree(einfluss_spez);
  myfree(einfluss_parameter);
  myfree(info_datei_filename);
}

short VORHABEN_ENZ::matches(void *si)
{
  VORHABEN_ENZ_SORT_INFO *info = (VORHABEN_ENZ_SORT_INFO *)si;
  char *noa = info->name_oder_abk;

  if (!mystrcmp(name, noa)) return 1;
  else if (mystrcmp_no_case(noa,start_attribute.abfragen("ABK")))
    return 0;

  if (!info->staat) return 1; // Keine weiteren Bedingungen.
  if (!start_attribute.gesetzt("MehrfachABK")) return 1; // Keine weiteren B.
  
  // Bei mehreren Vorhaben mit der gleichen ABK ist das Vorhaben gesucht,
  // dass der Staat noch nicht erforscht hat, die Voraussetzungen aber
  // hat.
  
  if (!info->staat->info("HAT_ENTWICKLUNG",voraussetzungen_im_staat.to_string()))
    return 0; // Nicht gemeint.
  
  if (info->staat->info("HAT_ENTWICKLUNG",name)) return 0; // Hat sie schon.
  else return 1;
}

  
/**---------------------------------------------------------------------------
  * LEXIKON_ENZ::LEXIKON_ENZ()      | constructor
  * 
  * Der Konstruktor der Lexikonklasse merkt sich den Namen des Eintrages
  * und berechnet das Offset des ersten Untereintrages.
  *
  * @param
  * char *zeile: Puffer mit dem gesamten Eintrag. Die Untereintraege
  * fangen mit Backslashes an.
  * long offs:   Offset der Zeile innerhalb des Enzyklopaediefiles.
  ---------------------------------------------------------------------------*/
LEXIKON_ENZ::LEXIKON_ENZ(char *zeile, long offs)
{
  // Alles was ich tun muss, ist den Namen kopieren und das Offset
  // auf den Parameterbereich zu merken.

  char puffer[512];
  sscanf(zeile, "%s", puffer);
  name = mystrdup(puffer);

  // und jetzt suche ich in einer Schleife nach dem ersten Backslash
  // merke mir dessen Position.

  long pos=0; // Offset innerhalb der Zeile
  while (zeile[pos] && zeile[pos] != '\\') pos++;
  if (!zeile[pos]) offset = 0; // Zeile ist nicht in Ordnung.
  else offset = offs + pos;
}


/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::lexikon_eintrag()
  * 
  * Holt einen bestimmten Untereintrag aus dem Lexikon der Enzyklopaedie.
  *
  * @param
  * char *name: Names des Eintrages, z.B. Abakus.
  * short nummer: Nummer des Untereintrages (kleinste Nummer ist 1!)
  *
  * @return
  * Zeiger auf einen statischen String, wenn ein Eintrag gefunden wurde.
  * NULL, wenn kein passender Eintrag gefunden wurde.
  ---------------------------------------------------------------------------*/
char *ENZYKLOPAEDIE::lexikon_eintrag(char *name, short nummer)
{
  // Zuerst suche ich nach dem richtigen Eintrag.
  LEXIKON_ENZ *lex = (LEXIKON_ENZ *)lexikonliste.find((void *)name);
  if (!lex) {
      log('K', "Missing entry '%s' in lexical section of configuration file", name);
      return NULL;
  }

  // Jetzt muss ich das Enzyklopaediefile nocheinmal oeffnen!

  FILE *file = fopen(attribut("DATEINAME"), "r");
  if (!file) {
      log('K', "Can't repopen configuration file '%s'", attribut("DATEINAME"));
      return NULL;
  }

  // Nun muss ich den Lesekopf an die richtige Stelle im File bringen.
  fseek(file, lex->offset, SEEK_SET);

  // Hier tritt nun ein kleines Problem auf:
  // Die gespeicherte Fileposition gibt moeglicherweise nicht den tat-
  // saechlichen Anfang des Eintrages an, sondern zeigt auf Kommentarzeilen,
  // die dem Eintrag vorangehen. Deshalb kommt es zur einer Fehlsynchroni-
  // stion beim Suchen nach dem n-ten Eintrag genau dann, wenn in den
  // Kommentaren ein Backslash steht. Dies ist deshalb zu vermeiden,
  // um Probleme zu umgehen.

  // Das ganze File nach dem nummer-ten Backslash durchsuchen
  while (nummer) {
    char c = fgetc(file);
    if (c != '\\' && !feof(file)) continue;
    else if (c == '\\') nummer--;
    else {
      fclose(file);
      return NULL; // Fehler. Eintrag nicht gefunden.
    }
  }

  // So. Nun steht der Filepointer genau auf dem gesuchten Eintrag.
  // Diesen lese ich in meinen konstanten Puffer ein.
  static char antwort[3][1024]; // Beachte diese Grenze!
  static short selector = 0;
  selector++;
  if (selector > 2) selector = 0;

  // In diesen Puffer muss ich den ganzen Schmarrn bis zum naechsten
  // Backslash einlesen. Ich lese ertmal mehr an, als noetig.
  fread(antwort[selector], 1, 1024, file);
  fclose(file);

  // Und nun suche ich das Ende und bonke dort eine Null rein.
  char *temp = antwort[selector];
  while (temp-antwort[selector] < 1023 && *temp != '\\' && *temp != '\n') temp++;
  *temp = 0;
  return antwort[selector]; // Achtung! Antwort ist statisch.
}


/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::ENZYKLOPAEDIE()  | constructor
  * 
  * Konstruktor der Enzyklopaedie. Er oeffnet die Datei mit den Daten
  * und liest alle Daten zu Vorhaben, Staatsformen, Resourcen, Einheiten
  * und Lexikon ein. 
  *
  * @param
  * char *n: Name des Objektes. Empfohlen ist "Enzyklopaedie".
  * char *a: Attributstring.
  ---------------------------------------------------------------------------*/
ENZYKLOPAEDIE::ENZYKLOPAEDIE(char *n, char *a) : OBJEKT(n,a)
{
    i_am_ok = false;
    anzahl_verschiedener_resourcen = 0;

    char *filename = attribut("DATEINAME");
    // Jetzt oeffne ich das File zum Lesen
    FILE *file = fopen(filename,"r");
    if (!file)
    {
	log('E', "Can't open game configuration file '%s'", filename);
	return;
    }
    log ('2', "Using game configuration file '%s'", filename);
  
    char *zeile = new char[MAX_LAENGE_ENZYKLOPAEDIE_ZEILE+2];

    // Jetzt kann es losgehen. Die Funktion hole_zeile() liest mir eine Zeile
    // ein, die nicht mit einem Kommentar anfaengt oder leer ist.

    while (0 <= hole_zeile(zeile, MAX_LAENGE_ENZYKLOPAEDIE_ZEILE+1, file))
    {
	if (!strcmp(zeile, "RESOURCEN:")) resourcen_laden(zeile,file);
	else if (!strcmp(zeile, "EINHEITEN:")) einheiten_laden(zeile, file);
	else if (!strcmp(zeile, "VORHABEN:")) vorhaben_laden(zeile, file);
	else if (!strcmp(zeile, "STAATSFORMEN:")) staatsformen_laden(zeile, file);
	else if (!strcmp(zeile, "LEXIKON:")) lexikon_laden(zeile, file);
	else {
	    log('K', "Unknown section '%s' in configuration file '%s'", zeile, filename);
	    fclose(file);
	    delete zeile;
	    return;
	}
  }

  // So. Nun muesste alles geladen sein...
  fclose (file);
  delete zeile;
  i_am_ok = true;
}


/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::lexikon_laden()
  * 
  * Laedt Daten in die lexikonliste. Solche Daten fangen in der Datei
  * mit dem Header LEXIKON: an.
  *
  * @param
  * char *zeile: Kann als Puffer verwendet werden, um eine Zeile ein-
  * zulesen.
  * FILE *file:  File, aus dem die Daten gelesen werden sollen.
  ---------------------------------------------------------------------------*/
void ENZYKLOPAEDIE::lexikon_laden(char *zeile, FILE *file)
{
  while (1) {

    // hole_zeile() liefert mir das Byteoffset des ersten nichtleeren Zeichens
    // in der geholten Zeile bzw. -1, bei Ende oder Fehler

    long offset = hole_zeile(zeile, MAX_LAENGE_ENZYKLOPAEDIE_ZEILE+1, file);
    if (offset < 0) return; // Fehler oder Ende der Datei
    else if (*zeile == '.') return; // Korrektes Ende des Lexikon

    LEXIKON_ENZ *lex = new LEXIKON_ENZ(zeile, offset);
    lexikonliste.insert(lex);
  }
}


/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::resourcen_laden()
  * 
  * Liest die Daten zu den Resourcen aus der Datei. Der Header heisst
  * RESOURCEN:.
  *
  * @param
  * char *zeile: Kann als Puffer verwendet werden, um eine Zeile ein-
  * zulesen.
  * FILE *file:  File, aus dem die Daten gelesen werden sollen.
  ---------------------------------------------------------------------------*/
void ENZYKLOPAEDIE::resourcen_laden(char *zeile, FILE * file)
{
  anzahl_verschiedener_resourcen = 0; // member von this!

  while (0 <= hole_zeile(zeile, MAX_LAENGE_ENZYKLOPAEDIE_ZEILE+1, file))
  {
    if (*zeile == '.') return;
    RESOURCE_ENZ *neu = new RESOURCE_ENZ;
    sscanf(zeile, "%s %c", neu->name, &neu->symbol);
    resourcenliste.insert(neu);
    anzahl_verschiedener_resourcen++;
  }

  log('K', "Unexpected end of configuration file in section 'RESOURCEN:'");
}


/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::staatsformen_laden()
  * 
  * Laedt die Daten aus dem Chunk STAATSFORMEN:.
  *
  * @param
  * char *zeile: Kann als Puffer verwendet werden, um eine Zeile ein-
  * zulesen.
  * FILE *file:  File, aus dem die Daten gelesen werden sollen.
  ---------------------------------------------------------------------------*/
void ENZYKLOPAEDIE::staatsformen_laden(char *zeile, FILE *file)
{

  while (0 <= hole_zeile(zeile, MAX_LAENGE_ENZYKLOPAEDIE_ZEILE+1, file))
  {
    if (*zeile == '.') break; // Ende dieses Chunks.

    char *string = zeile; // Zeiger zum Abscannen vorbereiten.

    STAATSFORM_ENZ *staatsform = new STAATSFORM_ENZ;
    staatsform->name = hole_wort_aus_string(string); // Aendert string!

    // Und nun koennen eine Reihe von Einfluessen kommen. Jeder
    // Einfluss besteht aus drei Parametern. Solange ich nach
    // ueberlesen von Leerzeichen nicht auf ein Zeilenende stosse,
    // interpretiere ich die kommenden Daten als die Spezifikation
    // eines Einflusses, den die Staatsform ausuebt.

    for (;;) {
      while (isspace(*string)) string++; // Erst ueberlese ich Whitespaces...
      if (!*string) break; // Zeilenende

      char *art = hole_wort_aus_string(string);
      char *ziel = hole_wort_aus_string(string);
      char *par = hole_wort_aus_string(string);

      EINFLUSS_ENZ *einfl = new EINFLUSS_ENZ(art, ziel, par);
      staatsform->einfluesse.insert(einfl);
    }

    // Nun muss ich die neue Staatsform natuerlich noch in die Liste ein-
    // binden (die Bestandteil von this ist).

    staatsformenliste.insert(staatsform);

  } // while (hole_zeile...)

  // Wenn zeile nun nicht auf einen Punkt zeigt, dann ist ein Fehler auf-
  // getreten.

  if (*zeile != '.')
      log('K', "Unexpected end of configuration file in section 'STAATSFORM:'");
}


/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::einheiten_laden()
  * 
  * Laedt die Daten aus dem Chunk EINHEITEN:, der Angaben wie Bewegungs-
  * dauer, Angriff und Verteidigung fuer die Einheiten enthaelt.
  *
  * @param
  * char *zeile: Kann als Puffer verwendet werden, um eine Zeile ein-
  * zulesen.
  * FILE *file:  File, aus dem die Daten gelesen werden sollen.
  ---------------------------------------------------------------------------*/
void ENZYKLOPAEDIE::einheiten_laden(char *zeile, FILE * file)
{
  // Puffer fuer die Speicherung des Gattungsnamen der Einheit anlegen.
  // Der Gattungsname ist spaeter der Wert des Attributes GATTUNG

  char *namepuffer = new char[MAX_LAENGE_ATTRIBUTSWERT+2];

  while (0 <= hole_zeile(zeile, MAX_LAENGE_ENZYKLOPAEDIE_ZEILE+1, file))
  {
    if (*zeile == '.') break; // Schlusspunkt ist Endezeichen

    EINHEIT_ENZ *neu = new EINHEIT_ENZ;
    sscanf(zeile, "%s%ld%ld%ld%f",namepuffer, &neu->angriffskraft,
	   &neu->verteidigungskraft, &neu->bewegungspunkte_pro_feld,
	   &neu->sichtweite);
    neu->name = mystrdup(namepuffer);
    einheitenliste.insert(neu); // einheitenliste ist member von this
  }

  if (*zeile != '.') {
      log('K', "Unexpected end of configuration file in section 'EINHEITEN:'");
  }

  delete namepuffer;
}


/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::vorhaben_laden()
  * 
  * Laedt den umfangreichen chunk VORHABEN:, der Daten ueber Einheiten,
  * Stadtausbauten und Wissenschaftliche Entwicklungen enthaelt.
  *
  * @param
  * char *zeile: Kann als Puffer verwendet werden, um eine Zeile ein-
  * zulesen.
  * FILE *file:  File, aus dem die Daten gelesen werden sollen.
  ---------------------------------------------------------------------------*/
void ENZYKLOPAEDIE::vorhaben_laden(char *zeile, FILE * file)
{

  while (0 <= hole_zeile(zeile, MAX_LAENGE_ENZYKLOPAEDIE_ZEILE+1, file))
  {
    if (*zeile == '.') break; // Ende dieses Chunks.
    char *string = zeile;

    VORHABEN_ENZ *neu = new VORHABEN_ENZ;

    char *wort;

    neu->name = hole_wort_aus_string(string);

    wort = hole_wort_aus_string(string);
    neu->benoetigte_resourcen.setzen_auf(wort);
    myfree(wort);

    wort = hole_wort_aus_string(string);
    neu->unterhalt.setzen_auf(wort);
    myfree(wort);

    wort = hole_wort_aus_string(string);
    neu->voraussetzungen_in_der_stadt.aus_string_einlesen(wort);
    myfree(wort);

    wort = hole_wort_aus_string(string);
    neu->voraussetzungen_im_staat.aus_string_einlesen(wort);
    myfree(wort);

    neu->max_anzahl_in_einer_stadt = hole_long_aus_string(string);
    neu->max_anzahl_in_einem_staat = hole_long_aus_string(string);

    wort = hole_wort_aus_string(string);
    neu->start_attribute.aus_string_einlesen(wort);

    // Das Attribut GATTUNG wird automatsch gesetzt.
    neu->start_attribute.setzen("GATTUNG",neu->name);
    myfree(wort);

    neu->einfluss_art = hole_wort_aus_string(string);
    neu->einfluss_spez = hole_wort_aus_string(string);
    neu->einfluss_parameter = hole_wort_aus_string(string);

    neu->info_datei_filename = hole_wort_aus_string(string);
    if (mystrlen(neu->info_datei_filename) <= 4 || mystrlen(neu->info_datei_filename) > 12)
    {
	log('K', "Warning: Entry %s has suspicious info filename '%s'",
	    neu->name, neu->info_datei_filename);
    }
    vorhabenliste.insert(neu);
  }

  if (*zeile != '.') report("Unerwartetes Ende in der Datei");
}


/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::hole_zeile()
  * 
  * Holt aus einer Datei (insbesondere natuerlich der Enzyklopaedieedatei)
  * eine Zeile in einen Puffer mit einer begrenzten Laenge. Dabei werden
  * Leerzeilen, Kommentarzeilen (beginnen mit #) und fuehrende Whitespa-
  * ces ueberlesen.
  *
  * @param
  * char *zeile: Zeiger auf den Puffer
  * long laenge: Laenge des Puffers
  * FILE *file:  Datei, aus der gelesen werden soll.
  *
  * @return
  * Das Offset im File (mit ftell()) der zuletzt gelesenen Zeile, falls
  * noch korrekt gelesen werden konnte.
  * -1 bei einem Fehler oder bei Dateiende
  ---------------------------------------------------------------------------*/
long ENZYKLOPAEDIE::hole_zeile(char *zeile, long laenge, FILE *file)
{
  long offset;
  char *p;
  do {
    offset = ftell(file);
    fgets(zeile, (int)laenge, file);
    if (feof(file)) return -1;
    if (ferror(file)) return -1;

    p = zeile;
    while (*p && isspace(*p)) {
       p++; // Whitespaces ueberlesen
       offset++; // Offset anpassen
    }
  } while (!*p || *p=='#');

  // Jetzt entferne ich noch eventuell vorhandenen LFs und CRs vom Ende...
  p = zeile+strlen(zeile)-1; // Auf letztes Zeichen zeigen

  while (*p==10 || *p==13) *p-- = 0;

  return offset;
}

/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::hole_wort_aus_string()
  * 
  * Holt aus einem String das naechste Wort (durch WS begrenzt) und
  * schreibt den Stringzeiger auf das erste Zeichen nach dem Wort
  * weiter. Als Besonderheit wird das Wort ".", das nur aus dem
  * Schlusspunkt besteht, als NULL-Wort interpretiert.
  *
  * @param
  * char*& string: Referenzparameter: Zeiger auf den String aus dem
  * gelesen werden soll. Er wird anschliessend auf das
  * naechste Zeichen nach dem Wort gesetzt.
  * @return
  * char *  Zeiger auf eine dynamische Kopie des geholten Wortes.
  ---------------------------------------------------------------------------*/
char *ENZYKLOPAEDIE::hole_wort_aus_string(char*& string)
{
  // Erst ueberlese ich Whitespaces...
  while (isspace(*string)) string++;

  // Jetzt warte ich bis zum Ende des Wortes.
  char *start = string;
  while (*string && !isspace(*string)) string++;

  // Jetzt steht der Zeiger string auf dem Ende. Ich schreibe zwischenzeitig
  // mal kurz 'ne 0 rein...

  char merken = *string;
  *string = 0;

  // Jetzt kann ich den String kopieren. Falls er leer ist oder nur aus
  // einem Schlusspunkt besteht, gebe ich NULL zurueck.

  char *erg;
  if (start[0]==0 || (start[0]=='.' && start[1]==0)) erg = NULL;
  else erg = mystrdup(start);
  *string = merken;
  return erg;
}


/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::hole_long_aus_string()
  * 
  * Setzt auf hole_wort_aus_string (siehe oben) auf, konvertiert aber
  * das Ergbnis noch in eine long-Zahl.
  *
  * @param
  * char*& string: Zeiger auf den Quellstring.
  *
  * @return
  * long: Geholter Wert oder 0, wenn ein Fehler auftrat.
  ---------------------------------------------------------------------------*/
long ENZYKLOPAEDIE::hole_long_aus_string(char*& string)
{
  char *a = hole_wort_aus_string(string);
  if (!a) return 0;

  long erg=atol(a);
  myfree(a);
  return erg;
}

/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::welche_gattung_kommt_zuerst()
  * 
  ---------------------------------------------------------------------------*/
short ENZYKLOPAEDIE::welche_gattung_kommt_zuerst(char *gattung1, char *gattung2)
{
  VORHABEN_ENZ *eintrag;
  SCAN (&vorhabenliste, eintrag)
  {
    if (!mystrcmp(eintrag->name, gattung1)) {
      return (!mystrcmp(eintrag->name, gattung2)) ? 0 : -1;
    }
    else if (!mystrcmp(eintrag->name, gattung2)) return 1;
    NEXT(eintrag);
  }
  return 0; // Keines von beiden gefunden. Das duerfte nicht passieren!
}


/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::infotext_zusatzinfo()
  * 
  * Gibt zusaetzliche Beschreibungen ueber die Werte eines Vorhabens
  * in Stringform. Dieser String kann dann bei der Ausgabe von Info-
  * texten im Ausdruck des Spielers unter jedem Info ausgegeben werden.
  * In dem String stehen Daten wie Bau- und Unterhaltskosten, Kampfwerte...
  * Vor allem steht hier auch immer die Abkuerzung.
  *
  * @param
  * char *infoname: Name der Infodatei, zu der Zusatzinformationen
  * ermittelt werden sollen.
  * @return
  * char *string: Dynamischer (!) String, mit den Werten, gemaess
  * drucker.cpp (mit '#' usw.) mit Formatierungszeichen
  * versehen. NULL, wenn Info nicht vorhanden.
  ---------------------------------------------------------------------------*/
char *ENZYKLOPAEDIE::infotext_zusatzinfo(char *infoname)
{
  // Erstmal muss ich alle Vorhaben durchsuchen und schauen, ob eines davon
  // (was ich doch stark hoffe) die gesuchte Datei als Infodatei angibt.
  
  short gefunden = 0;
  VORHABEN_ENZ *eintrag;
  SCAN (&vorhabenliste, eintrag)
  {
    if (!mystrcmp(eintrag->info_datei_filename, infoname)) {
      gefunden=1;
      break;
    }
    NEXT(eintrag);
  }
  if (!gefunden) return NULL; // Nichts gefunden

  // So. Nun stelle ich den String zusammen. Je nach TYP des Vorhabens gebe
  // ich aber unterschiedliche Dinge an. In typ merke ich mir einen Kennbuch-
  // staben fuer den Typ (zweiter Buchstabe: R, T, E oder I)

  ATTRIBUT_LISTE *attr = &eintrag->start_attribute; // Abkuerzung
  char *typstring = attr->abfragen("TYP");
  if (!typstring) return NULL; // Darf bei korrekter Enzyklopaedie nicht sein.
  char typ = typstring[1];

  // Nun bereite ich einen Zeiger fuer den Ausgabestring und einen Puffer
  // fuer Zwischenwerte vor.

  char *ausgabe=NULL, string[100];

  // Zuerst die Abkuerzung und Baukosten.

  mystradd(ausgabe, L("Abk~urzung: ","Abbreviation: "));
  mystradd(ausgabe, attr->abfragen("ABK"));

  long einwohner = myatol(attr->abfragen("EINWOHNER"));
  if (eintrag->benoetigte_resourcen.ist_null()) {
    mystradd(ausgabe,L(", keine Baukosten",", no building costs"));
    if (einwohner) {
      sprintf(string,L(", bn~otigt aber %ld Einwohner",", requires %ld Citizens"), einwohner);
      mystradd(ausgabe, string); 
    }
  }
  
  else {
    mystradd(ausgabe, L(", Kosten: ",", Cost: "));
    mystradd(ausgabe, eintrag->benoetigte_resourcen.to_string());
    if (einwohner) {
      sprintf(string,L(" und %ld Einwohner",", and %ld Citizens"), einwohner);
      mystradd(ausgabe, string); 
    }
  }
  

  // Nun die Unterhaltskosten im Falle von Ausbaut, Weltbaut und Einheit.  
  if (typ=='I' || typ=='T' || typ=='E') {
    if (eintrag->unterhalt.ist_null())
      mystradd(ausgabe, L(", keine Unterhaltskosten",", no upkeep"));
    else {
      mystradd(ausgabe, L(", Unterhalt: ",", Upkeep: "));
      mystradd(ausgabe, eintrag->unterhalt.to_string());
    }
  }
  
  // Eventuell sind noch Vor. in der Stadt noetig.
  if (!eintrag->voraussetzungen_in_der_stadt.is_empty()) {
    mystradd(ausgabe, L(", ben~otigt ",", requires "));
    mystradd(ausgabe, eintrag->voraussetzungen_in_der_stadt.to_string());
  }
  
  // Maximale Anzahl in einer Stadt, falls Stadtausbau.
  if (typ == 'T') {
    mystradd(ausgabe, L(", maximale Anzahl pro Stadt: ",", maximum number per Town "));
    mystradd(ausgabe, myltoa(eintrag->max_anzahl_in_einer_stadt));
  }
  
  // Nun kommen noch Angabe speziell fuer Weltbauten.

  if (typ == 'E') { // WELTBAUT
    // Stabilitaet gibt die Verteidigungsstaerke an.
    char *wert = attribut("STABILITAET");
    if (wert) {
      mystradd(ausgabe, L(", Stabilit~at: ",", Stability: "));
      mystradd(ausgabe, wert);
    }
    
    // Foerderquote und Lagerplatz
    if (0 != (wert = attribut("LAGER"))) {
      mystradd(ausgabe,L(", Lagerkapazit~at: ",", Stockroom: "));
      mystradd(ausgabe, wert);
    }
    
    if (0 != (wert = attribut("FOERDERQUOTE"))) {
      mystradd(ausgabe, L(", max. F~orderquote: ",", max. Haulage Quota: "));
      mystradd(ausgabe, wert);
    }
  } // if WELTBAUT

  // Im Falle einer Einheit kommen nun noch einige Angaben zu den Kampfwerten
  // etc.
  
  else if (typ == 'I') { 
    // Erstmal muss ich den Eintrag in der Einheitenliste finden.
    EINHEIT_ENZ *eh = einheit(eintrag->name);
    if (eh) { // Sonst Enzyklopaedie fehlerhaft
      sprintf(string,L(", Kampfkraft: %ld, V-Bonus: %ld, Bewegungsdauer: %ld, Sichtweite: %2.1f",", Offense: %ld, D-Bonus: %ld, Movement Cost: %ld, Sight: %2.1f"),
	      eh->angriffskraft, eh->verteidigungskraft,
	      eh->bewegungspunkte_pro_feld, eh->sichtweite);
      mystradd(ausgabe, string);
    }
    
    // Alle weiteren Angaben sind optional je nach Einheit.

    if (attr->gesetzt("KAPAZ")) { // Lagermoeglichmeit
      sprintf(string,L(", Transportkapazit~at: %s, G~uterarten: ",", Capacity: %s, Commodities: "),
	 attr->abfragen("KAPAZ"));
	 
      char *lager = attr->abfragen("LAGER");
      for (int i=0; i<(int)strlen(lager); i++) {
	if (i) strcat(string,",");
	char s[2]; s[0]=lager[i]; s[1]=0;
	strcat(string,s);
      }
      
      mystradd(ausgabe, string);
    }

    // Nun kommen Daten ueber Fernkampf
    
    if (attr->gesetzt("FEUERKRAFT")) {
      sprintf(string,L(", Feuerkraft: %s, Schu~sweite: %s",", Firepower: %s, Range: %s"),
	      attr->abfragen("FEUERKRAFT"), attr->abfragen("SCHUSSWEITE"));
      mystradd(ausgabe, string);
    }
    
    // Daten uber Konvoimoeglichkeit
    if (attr->gesetzt("CONVOY")) {
      sprintf(string, L(", Konvoi: %s Einheiten",", Convoy: %s Unit (s)"), attr->abfragen("CONVOY"));
      mystradd(ausgabe, string);
    }
    
    // Und zu guterletzt Daten ueber die Reichweite bei Flugzeugen.
    if (attr->gesetzt("REICHWEITE")) {
      sprintf(string, L(", Flugreichweite: %s ",", Flight Range: %s "), attr->abfragen("REICHWEITE"));
      mystradd(ausgabe, string);
    }
  
  } // if EINHEIT: Sonderausgaben fuer Einheiten.
      
  // So. Das war alles. Nun gebe ich das Ergenis zurueck. Der Aufrufer muss
  // dann am Ende noch myfree() machen.
  
  return ausgabe;
}   
      
/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::info_grafik()
  * 
  * Sucht nach einem Vorhaben-Eintrag, der als Infodatei den genannten 
  * Namen hat. Falls gefunden, wird das ATTRIBUT GRAFIK= geholt, vorne
  * ein 'e' und hinten ein '.gra' drangehaengt und das zurueckgegeben.
  * Ansonsten einfach NULL.
  * @param
  * char *infoname:	Name einer Infodatei
  * @return
  * char *:		Name der Grafikdatei, falls vorhanden.
  ---------------------------------------------------------------------------*/
char *ENZYKLOPAEDIE::info_grafik(char *infoname)
{
  char *grafik = NULL;
  VORHABEN_ENZ *vorhaben = (VORHABEN_ENZ *)vorhabenliste.first();
  while (!vorhaben->is_tail())
  {
    if (!mystrcmp(vorhaben->info_datei_filename, infoname))
    {
      grafik = vorhaben->start_attribute.abfragen("GRAFIK");
      break;
    }
    vorhaben = (VORHABEN_ENZ *)vorhaben->next();
  }

  if (!grafik) return NULL;
  
  static char grafikname[200];
  sprintf(grafikname, "e%s.gra", grafik);
  return grafikname;
}
 
 
RESOURCE_ENZ *ENZYKLOPAEDIE::resource(char c)
{
  static char s[2];
  s[0]=c;
  s[1]=0;
  return resource(s);
}

/**---------------------------------------------------------------------------
  * ENZYKLOPAEDIE::vorhaben_mit_link(HTML&, char *)
  *
  * Fuegt in ein HTML-File den Namen eines Vorhabens ein, der der mit einem
  * Link auf das Info versehen ist.
  ---------------------------------------------------------------------------*/
void ENZYKLOPAEDIE::vorhaben_mit_link(HTML& html, char *vor)
{
    VORHABEN_ENZ *vorenz = vorhaben(vor);
    if (vorenz) {
	char *s = mystrdup(vorenz->info_datei_filename);
	s[strlen(s)-4]=0; // Da muss der "." sein!
	html.href_info(s);
	myfree(s);
    }
    html.text(vor);
    if (vorenz) html.end_href();
}

VORHABEN_ENZ *ENZYKLOPAEDIE::vorhaben(char *n, STAAT *staat)
{
    VORHABEN_ENZ_SORT_INFO si(n,staat);
    VORHABEN_ENZ *antwort = (VORHABEN_ENZ *)vorhabenliste.find(&si);

    // Wenn es nicht gefunden wurde, dann versuch ich es nochmal ohne
    // staat information. Dann werden die Vorhaben mit MehrfachABK normal
    // behandelt und ich finde die erste davon in der Enz. Das brauche ich
    // damit der Befehl *B auch eine Forschung von Maschinenbau_I, Kernwaffentechnik_I,..
    // schenken kann.
    
    if (staat && !antwort) return vorhaben(n);
    else return antwort;
}
