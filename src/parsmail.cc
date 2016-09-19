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


#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <regex.h>
#include <sys/stat.h>
#include <time.h>

#include "uhr.h"
#include "log.h"
#include "staat.h"


/**---------------------------------------------------------------------------
  * UHR::emails_parsen(long runde)
  *
  * Durchsucht das Emailin Verzeichnis nach emails, die fuer die aktuelle
  * Runde und Partie Befehle enthalten. Generiert aus diesen die Befehlsdateien
  * und kopiert sie ins richtige Verzeichnis. Die geparsten Dateien werden
  * ins emaildone Verzeichnis geschoben.
  ---------------------------------------------------------------------------*/
void UHR::emails_parsen(long runde)
{
    char *emailin    = mystrdup(emailin_dateiname());
    log('2', "Looking for emails in %s", emailin);

    DIR *dir = opendir(emailin);
    if (!dir) {
	log('2', "Directory %s doesn't exist or isn't readable. Skipping email parsing", emailin);
	myfree(emailin);
	return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
	char filename[512];
	sprintf(filename, "%s/%s", emailin, entry->d_name);

	struct stat st;
	stat(filename, &st);
	if (!S_ISDIR(st.st_mode))
	{
	    // Hier nehme ich der Einfachkeit halber POSIX-Konformitaet an, die
	    // besagt, dass time_t = unsigned long int ist.
	    unsigned long int modification_time = (unsigned long int)st.st_mtime;
	    STAAT *staat = email_parsen(filename, runde, modification_time);
	    if (staat)
	    {
		char *moveto = emaildone_dateiname(staat, entry->d_name);
		verzeichnis_gewaehrleisten(moveto);
		if (rename(filename, moveto) == -1)
		    log('W', "File %s couldn't be renamed to '%s'", filename, moveto);
	    }
	    else log('3', "Skipped file %s", filename);
	}
    }
    closedir(dir);
    myfree(emailin);
}


/**---------------------------------------------------------------------------
  * UHR::email_parsen(char *filename, long runde)
  *
  * Parst eine email. Liefert true, wenn sie erfolgreich bearbeitet wurde und
  * fuer die aktuelle Partie und den aktuellen Zug verwendet wurde. Das heisst,
  * dass sie ins emaildone - Verzeichnis geschoben werden soll.
  *
  * Die modification_time wird dazu verwendet, zu gewaehrleisten, dass bei
  * mehreren Befehlsboegen fuer eine Runde/Spieler/Partie der aktuelleste
  * verwendet wird.
  ---------------------------------------------------------------------------*/
STAAT *UHR::email_parsen(char *filename, long runde, unsigned long int modification_time)
{
    log('4', "Parsing file %s..", filename);
    FILE *file = fopen(filename, "r");
    if (!file) return 0;

    // Ich suche Zeilen der Form Schluessel=Wert und speichere sie in einer
    // Attributsliste.

    ATTRIBUT_LISTE attribute;

    char zeile[1025];
    zeile[1024]=0;

    regex_t regex;
    regmatch_t matches[3];
    const char *regexstring = "^[[:space:]]*\\([a-zA-Z]*\\)[[:space:]]*[=:]"
	"[[:space:]]*\\([^[:space:]]*\\([[:space:]]\\+[^[:space:]]\\+\\)*\\)[[:space:]]*$";
    regcomp(&regex, regexstring, 0);

    while (fgets(zeile, 1024, file))
    {
	if (0 == regexec(&regex, zeile, 3, matches, 0)) // Zeile passt
	{
	    char *key = &zeile[matches[1].rm_so];
	    zeile[matches[1].rm_eo] = 0;
	    char *value = &zeile[matches[2].rm_so];
	    zeile[matches[2].rm_eo] = 0;
	    if (value[0] && value[strlen(value)-1] == '>')
	    {
		log('4', "Key %s: Value %s looks suspicous. Ignoring it",
		    key, value);
	    }
	    else
	    {
		attribute.setzen(key, value);
		log('5', "Key=%s, Value=%s", key, value);
	    }
	}
    }
    fclose(file);

    if (!attribute.gesetzt(L("Spieltyp","Gametype"))) {
	log('4', "Missing game type");
	return 0;
    }

    if (mystrcmp_no_case(attribute.abfragen(L("Spieltyp","Gametype")), "PROMETHEUS")) {
	log('4', "Wrong game type '%s'", attribute.abfragen(L("Spieltyp","Gametype")));
	return 0; // Nicht Prometheus
    }

    if (!attribute.gesetzt(L("Spiel","Game"))) {
	log('4', "Missing game");
	return 0;
    }

    if (mystrcmp_no_case(attribute.abfragen(L("Spiel","Game")), attribut("SESSIONNAME"))) {
	log('4', "Wrong game '%s'", attribute.abfragen(L("Spiel","Game")));
	return 0; // Falsc. Partie
    }
    
    long r = myatol(attribute.abfragen(L("Runde","Turn")));
    if (r != runde) {
	log('4', "Wrong turn %ld instead of %ld", r, runde);
	return 0; // Falsche runde.
    }
    if (!attribute.gesetzt(L("Position","Position"))) {
	log('3', "Missing abbreviation of empire");
	return 0; // Staat fehlt
    }

    char *staatname = attribute.abfragen(L("Position","Position"));
    STAAT *staat = (STAAT *)objekt_mit_namen(staatname);
    if (!staat) {
	log('3', "Empire '%s' doesn't exist");
	return 0;
    }

    if (!attribute.gesetzt(L("Emailschluessel","Emailkey"))) {
	log('3', "Missing Emailkey");
	return staat; // Emailschluessel fehlt
    }
    char *kennwort = attribute.abfragen(L("Emailschluessel","Emailkey"));

    
    if (mystrcmp(kennwort, staat->attribut("EMAILKEY"))) {
	log('3', "Wrong emailkey '%s'. Correct one would've been '%s'",
	    kennwort, staat->attribut("EMAILKEY"));
	return staat;
    }

    // Jetzt Aktualitaet vergleichen. Der Staat hat dazu das Attribut BEFBOGENZEIT
    if (staat->attribut_gesetzt("BEFBOGENZEIT")) {
	char *befbogenzeit = staat->attribut("BEFBOGENZEIT");
	unsigned long int old_mod = strtoul(befbogenzeit, 0, 10);
	if (old_mod > modification_time) { // Verwerfen, da nicht aktueller
	    log('3', "Ignoring '%s', because there exists a newer command sheet.", filename);
	    return staat; // Muss trotzdem entfernt werden!
	}
    }
    else { // attribut setzen
	char zeitstring[32];
	sprintf(zeitstring, "%lu", modification_time);
	staat->attribut_setzen("BEFBOGENZEIT", zeitstring);
    }
    
    log('4', "Turn: %ld, Empire: %s, Password: %s", myatol(attribute.abfragen("Runde")),
	staatname, kennwort);

    // Befehle uebertragen.
    befehle_aus_email_holen(filename, staat);
  
    return staat;
}


/**----------------------------------------------------------------------------
  * UHR::befehle_aus_email_holen(char *, STAAT *)
  *
  * Holt aus einer Emaildatei alle Zeilen zwischen BEFEHLSANFANG und
  * BEFEHLSENDE und schreibt die brauchbaren Zeilen in die Befehlsdatei fuer
  * den Staat. Alle alten Befehle werden ueberschrieben.
  ---------------------------------------------------------------------------*/
void UHR::befehle_aus_email_holen(char *filename, STAAT *staat)
{
    FILE *file = fopen(filename, "r");
    if (!file) {
	log('W', "Couldn't reopen email file %s. No commands for player ", filename, staat->name);
	return;
    }

    char *outfilename = befehlsdateiname(staat);
    verzeichnis_gewaehrleisten(outfilename);
    FILE *outfile = fopen(outfilename, "w");
    if (!outfile) {
	log('W', "Couldn't open file %s for output. No commans for player ", outfilename, staat->name);
	fclose(file);
	return;
    }

    char zeile[1025];
    zeile[1024] = 0;
    bool active = false;
    long anzzeilen = 0;

    regex_t regex_bef, regex_anf, regex_end;
    regmatch_t matches[3];
    regcomp(&regex_bef,
	    "^[[:space:]]*"
	    "\\([a-z][a-z][a-z][0-9]*\\)"
	    "[[:space:]]*"
	    "\\([^[:space:]].*[^[:space:]]\\)"
	    "[[:space:]]*$", REG_ICASE);

    regcomp(&regex_anf,
	    "^[[:space:]]*"
	    L("befehlsanfang","startofcommands")
	    "[[:space:]]*$",REG_ICASE );
    
    regcomp(&regex_end, "^[[:space:]]*"
	    L("befehlsende","endofcommands")
	    "[[:space:]]*$", REG_ICASE);
    
    while (fgets(zeile, 1024, file))
    {
	if (0 == regexec(&regex_anf, zeile, 0, 0, 0)) active = true;
	else if (0 == regexec(&regex_end, zeile, 0, 0, 0)) active = false;
	else if (active && 0 == regexec(&regex_bef, zeile, 3, matches, 0))
	{
	    char *abkuerzung = &zeile[matches[1].rm_so];
	    zeile[matches[1].rm_eo] = 0;
	    char *befehle = &zeile[matches[2].rm_so];
	    zeile[matches[2].rm_eo] = 0;

	    OBJEKT *o = objekt_mit_namen(abkuerzung);
	    if (o && (o == staat || staat->besitzt(o)))
	    {
		char *neue_befehle = befehlszeile_schoenmachen(befehle);
		fprintf(outfile, "%s %s\n", abkuerzung, neue_befehle);
		log('4', "Out:%s %s", abkuerzung, neue_befehle);
		delete neue_befehle;
		anzzeilen ++;
	    }
	    else
		log('4', "Rejected commandline %s", befehle);
	}
    }
    fclose(file);
    fclose(outfile);
    log('3', "Created from email file %s commands file %s for player %s with %ld lines",
	filename, outfilename, staat->name, anzzeilen);
}


/**---------------------------------------------------------------------------
  * UHR::befehlszeile_schoenmachen(char *)
  *
  * Konvertiert eine Befehlzeile so, dass dem Spieler moeglichst viel Unsinn
  * verziehen wird. Das bezieht sich vor allem auf Gross/Kleinschreibung und
  * Whitespaces. Umlaute und scharfes s werden durch ~a...~s ersetzt. Andere
  * non-ascii zeichen werden durch "_" ersetzt.
  *
  * @param zeile: Zeile OHNE die Abkuerzung.
  ---------------------------------------------------------------------------*/
char *UHR::befehlszeile_schoenmachen(char *zeile)
{
    // Alle Zeichen in Grossbuchstaben umwandeln.
    // Nur der MT-Befehl wird anders behandelt.

    char *lesen;
    char *neue_zeile = new char[strlen(zeile) * 2 + 1];
    char *schreiben = neue_zeile;
    bool  befehlsanfang = true;
    bool  mt_modus = false;
    
    for (lesen = zeile; *lesen; lesen++)
    {
	if (*lesen == '\n' || *lesen=='\r')  break;  // CR, LF
	else if (isspace(*lesen)) {                  // Spaces
	    if (!mt_modus) {
		befehlsanfang = true;
		*schreiben++ = ' ';
	    }
	    else *schreiben++ = '_';
	}

	else if (befehlsanfang && !strncasecmp(lesen, L("MT","ME"), 2)) { // MT-Befehl
	    mt_modus = true;
	    befehlsanfang = false;
	    *schreiben++ = L('M','M');
	    *schreiben++ = L('T','E');
	    lesen++;
	}
	else if (befehlsanfang) { // Andere Befehle
	    befehlsanfang = false;
	    *schreiben++ = toupper(*lesen);
	}
	else if (!mt_modus && *lesen>='a' && *lesen<='z') *schreiben++ = toupper(*lesen);
	else if (!isascii(*lesen)) {
	    switch(*lesen) {
	    case 'ä': *schreiben++ ='~'; *schreiben++='a'; break;
	    case 'ö': *schreiben++ ='~'; *schreiben++='o'; break;
	    case 'ü': *schreiben++ ='~'; *schreiben++='u'; break;
	    case 'Ä': *schreiben++ ='~'; *schreiben++='A'; break;
	    case 'Ö': *schreiben++ ='~'; *schreiben++='O'; break;
	    case 'Ü': *schreiben++ ='~'; *schreiben++='U'; break;
	    case 'ß': *schreiben++ ='~'; *schreiben++='s'; break;
	    default: *schreiben++ = '_';
	    }
	}
	else {
	    *schreiben++ = *lesen; // So lassen.
	    befehlsanfang = false;
	}
    }
    *schreiben++ = 0;
    return neue_zeile;
}
