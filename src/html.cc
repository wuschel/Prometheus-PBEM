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
  * HTML
  ----------------------------------------------------------------------------
  * Klasse zur Erzeugung von HTML Dateien.
  ---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "html.h"
#include "uhr.h"
#include "kompatib.h"
#include "log.h"

extern UHR *g_uhr; // Wegen Dateinamen

char *HTML::documentbase = NULL;
char *HTML::default_background_image = NULL;

HTML::HTML(char *fn, char *title, char *background_image)
{ 
    init(fn, title, background_image); 
}

void HTML::init(char *fn, char *title, char *background_image)
{
    subhtml = 0; // Kein Unterbereich. Verwaltet das File.
    filename = mystrdup(fn);
    
    verzeichnis_gewaehrleisten(filename);
    file = fopen(filename, "w");
    if (!file) {
	log('W', "Couldn't open file '%s' for output. "
	    "HTML printout will be incomplete", filename);
	return;
    }
    else
	log('h', "Creating HTML file '%s'..", filename);
    
  // Nun den Kopf erzeugen.

    fprintf(file, "<HTML>\n<HEAD>\n <TITLE>");
    text(title); // Expandiert auch die Tilden ~a, ~U, usw.
    fprintf(file,"</TITLE>\n</HEAD>\n<BODY BGCOLOR=\"#c0c0c0\"");
    
    if (background_image)
	fprintf(file, " BACKGROUND=\"%s\"",background_image);
    else if (default_background_image)
	fprintf(file, " BACKGROUND=\"%s\"",default_background_image);
    fprintf(file, ">\n");
    
    row_open = cell_open = 0;
    bold_flag = italic_flag = courier_flag = 0;
    fontsize=0;
    buttonpanel_open = 0;
    buttonpanel_left = buttonpanel_right = buttonpanel_up = 0;
    set_to_defaults();
}

HTML::HTML(HTML& html)
{
    filename = html.filename;
    file = html.file;
    row_open = cell_open = 0;
    bold_flag = italic_flag = courier_flag = 0;
    fontsize = 0;
    set_to_defaults();
    subhtml = 1;
}

void HTML::set_to_defaults()
{
    cell_alignment = AL_LEFT;
    cell_valignment = VAL_CENTER;
    cell_spacing = -1; // -1 Bedeutet: Kein Tag erzeugen, Defaultwert verwenden.
    cell_padding = -1;
    table_border = -1;
    table_width = 0;
    image_border = -1;
    cell_color = 0;
    row_color = 0;
    table_color = 0;
    cell_width = -1;
    cell_height = -1;
    column_span = 1;
    row_span = 1;
}


void HTML::set_documentbase(char *b)
{
  if (documentbase) delete documentbase;
  if (b) documentbase = strdup(b);
  else documentbase = NULL;
}

  

HTML::~HTML()
{
    if (!subhtml) {
	if (buttonpanel_open) make_buttonpannel();
	myfree(buttonpanel_left);
	myfree(buttonpanel_right);
	myfree(buttonpanel_up);
	myfree(filename);
    }
    if (ok() && !subhtml) {
	fprintf(file, "</BODY>\n</HTML>\n");
	fclose(file);
	log('h', "Closed HTML file correctly");
    }
}


/**---------------------------------------------------------------------------
  * HTML::text(char *)
  * 
  * Fuegt einen Text ein. Die Prometheusspezifischen Kombinationen 
  * ~a~o~u~A~O~U~s werden durch die entsprechenden Unicode-Escapes
  * ersetzt. Das Absatzzeichen # wird durch <BR> ersetzt, falls es
  * am Zeilenende vorkommt. Ansonsten wird es geloescht (Trennvorschlag
  * wird ignoriert). HTML-Sonderzeichen werden nicht als solche in-
  * terpretiert sondern ebenfalls durch Escape-Codes ersetzt.
  * Der Untertrich wird durch ein Space ersetzt.
  * @param
  * char *text: Der Text. Wenn text==NULL, dann wird ein non-breaking
  * space eingefuegt.
  ---------------------------------------------------------------------------*/
HTML& HTML::text(const char *text)
{
    if (text == NULL) {
	fprintf(file, "&nbsp;"); 
	return *this;
    }
    
    char puffer[strlen(text) * 7 + 256], *w = puffer;
    
    while (*text) {
	if (*text == '~')
	{ 
	    text++;
	    switch(*text)
	    {
	    case 'a': sprintf(w,"&auml;"); w+=6; break;
	    case 'o': sprintf(w,"&ouml;"); w+=6; break;
	    case 'u': sprintf(w,"&uuml;"); w+=6; break;
	    case 'A': sprintf(w,"&Auml;"); w+=6; break;
	    case 'O': sprintf(w,"&Ouml;"); w+=6; break;
	    case 'U': sprintf(w,"&Uuml;"); w+=6; break;
	    case 's': sprintf(w,"&szlig;"); w+=7; break;
	    }
	}
	else if (*text == '#')
	{ 
	    text++;
	    switch(*text)
	    {  
	    case '\n':
	    case '\r':
	    case ' ':
	    case '\t': sprintf(w, "<BR>"); w+=4; break;
	    }
	}
	else switch(*text) {
	case '<': sprintf(w, "&lt;");   w+=4; break;
	case '>': sprintf(w, "&gt;");   w+=4; break;
	case '&': sprintf(w, "&amp;");  w+=5; break;
	case '"': sprintf(w, "&quot;"); w+=6; break;
	case '_': sprintf(w, " ");      w+=1; break;
	default:  *w++ = *text;
	}
	text++;
    }
    *w=0;
    
    fprintf(file, "%s", puffer);
    return *this;
}



/**---------------------------------------------------------------------------
  * HTML::text(FILE *)
  * 
  * Holt alle Zeilen aus einer Datei und uebertraegt diese mit .text()
  * in das HTML-File. Konvertierung findet also wie dort beschrieben
  * statt.
  ---------------------------------------------------------------------------*/
HTML& HTML::text(FILE *infile)
{
  char linebuffer[5001];
  while (fgets(linebuffer, 5000, infile)) text(linebuffer);
  return *this;
}


HTML& HTML::text_from_file(char *filename)
{
  FILE *infile = fopen(filename, "r");
  if (infile) {
    text(infile);
    fclose(infile);
  }
  return *this;
}
  

HTML& HTML::imagewo(char *wosuffix)
{
    char imagename[4096];
    sprintf(imagename, "%s%s", wosuffix, g_uhr->attribut("GFORMAT"));
    return image(imagename);
}


HTML& HTML::image(char *n1, char *alt, char *n2, char *n3)
{
  fprintf(file, "<IMG SRC=\"%s%s%s\"",n1,n2?n2:"",n3?n3:"");
  if (image_border >= 0) fprintf(file, " BORDER=%d", image_border);
  if (alt) {
      fprintf(file, " ALT=\"");
      text(alt); // text expandiert Umlaute
      fprintf(file, "\"");
  }
  fprintf(file, ">");
  return *this; 
}

 
HTML& HTML::table()
{ 
  row_open = cell_open = 0;
  fprintf(file, "\n<TABLE");
  if (table_border >= 0) fprintf(file, " BORDER=\"%d\"", table_border);
  if (cell_spacing >= 0) fprintf(file, " CELLSPACING=\"%d\"", cell_spacing);
  if (cell_padding >= 0) fprintf(file, " CELLPADDING=\"%d\"", cell_padding);
  if (table_color)       fprintf(file, " BGCOLOR=\"%s\"",table_color);
  if (table_width)       fprintf(file, " WIDTH=\"%s\"", table_width);
  fprintf(file, ">");
  return *this;
}

HTML& HTML::next_row()
{
    if (cell_open) end_cell();
    if (row_open)  end_row();
    fprintf(file, "\n<TR");
    if (row_color) fprintf(file," BGCOLOR=\"%s\"", row_color);
    fprintf(file, ">");
    row_open = 1;
    return *this;
}


HTML& HTML::next_cell()
{
  if (!row_open) next_row();
  if (cell_open) end_cell();
  
  fprintf(file, "\n<TD");
  if (cell_alignment)    fprintf(file, " ALIGN=%s",       cell_alignment);
  if (cell_valignment)   fprintf(file, " VALIGN=%s",      cell_valignment);
  if (cell_color)        fprintf(file, " BGCOLOR=\"%s\"", cell_color);
  if (cell_width != -1)  fprintf(file, " WIDTH=\"%d\"",   cell_width);
  if (cell_height != -1) fprintf(file, " HEIGHT=\"%d\"",  cell_height);
  if (column_span != 1)  fprintf(file, " COLSPAN=\"%d\"", column_span);
  if (row_span != 1)    fprintf(file, " ROWSPAN=\"%d\"", row_span);
  fprintf(file, ">");
  if (fontsize) fprintf(file, "<FONT SIZE=\"%+d\">", fontsize);
  fprintf(file, "%s%s%s",
	  bold_flag ? "<B>" : "", 
	  italic_flag ? "<I>" : "",
	  courier_flag ? "<TT>" : "");
  cell_open = 1;
  return *this;
}

HTML& HTML::end_cell()
{
    if (bold_flag)    fprintf(file,"</B>");
    if (italic_flag)  fprintf(file, "</I>");
    if (courier_flag) fprintf(file, "</TT>");
    if (fontsize)     fprintf(file, "</FONT>");
    fprintf(file, "</TD>");
    cell_open = 0;
    return *this;
}


HTML& HTML::end_row()
{
    if (cell_open) end_cell();
    fprintf(file, "</TR>");
    row_open = 0;
    return *this;
}


HTML& HTML::end_table()
{
    if (cell_open) end_cell();
    if (row_open)  end_row();
    fprintf(file, "</TABLE>%s%s%s", 
	  bold_flag ? "<B>" : "",
	  italic_flag ? "<I>" : "",
	  courier_flag ? "<TT>" : "");
    
    return *this; 
}

HTML& HTML::href(char *ref, char *anchor)
{
    if (ref)
	fprintf(file, "<A HREF=\"%s.htm%s%s\">", ref, anchor?"#":"", anchor?anchor:"");
    else
	fprintf(file, "<A HREF=\"#%s\">", anchor);
  return *this;
}

// Prometheusspezifische Funktionen

/**---------------------------------------------------------------------------
  * HTML::printout_filename(STAAT *, char *, bool)
  *
  * Konstruiert den Dateinamen einer Spieler-Printoutdatei. Der Namen ergibt sich
  * aus dem Kommandoargument --htmlout und den aktuellen Daten des Spieles.
  *
  * Wird info auf true gesetzt, kommt die Datei in ein spezielles Verzeichnis
  * fuer infos, das nicht von der Runde abhaengig ist.
  ---------------------------------------------------------------------------*/
char *HTML::printout_filename(STAAT *staat, char *variabel, char *extension, bool info)
{
    // Namen und Rundennummer bilden ein eigenes Unterverzeichnis.
    // z.B. web_008/S_HSA.htm
    // Infos kommen nach infos/, z.B. infos/dasrad.htm
    
    char *htmlname = new char[strlen(variabel) + 32];
    if (info)
    {
	sprintf(htmlname, "infos/%s%s", variabel, extension ? extension : "");
    }
    else {
	char *partiename = g_uhr->info("SESSIONNAME");
	long runde = myatol(g_uhr->info("ZUGNUMMER"));
	sprintf(htmlname, "%s_%03ld/%s%s", partiename, runde, variabel,
		extension ? extension : "");
    }

    char *fn = g_uhr->htmldateiname(staat, htmlname);
    delete htmlname;
    return fn;
}



HTML& HTML::fieldimage(const char *name, const char *alt)
{
  char *without_extension = new char[strlen(name)+1];
  strcpy(without_extension, name);
  char *p = without_extension + strlen(without_extension);
  while (--p >= without_extension)
  {
    if (*p == '.') { *p=0; break; };
  }
  image(FIELDIMAGE_BASE, (char *)alt, without_extension, g_uhr->attribut("GFORMAT"));
  delete without_extension;
  return *this;
}


HTML& HTML::streetimage(unsigned char strassenwert, bool bodenschatz)
{
    char filename[32];
    sprintf(filename, "%c%01x/%01x",bodenschatz ? 'b' : 'a',
	    (int)strassenwert / 0x10, (int)strassenwert & 0x0f);
    return streetimage(filename);
}


HTML& HTML::streetimage(char *n)
{ 
    return image(STREETIMAGE_BASE, 0, n, ".gif"); 
}


HTML& HTML::buttonpanel(bool welt, char *left, char *right, char *up)
{
    buttonpanel_with_world = welt;
    buttonpanel_left       = mystrdup(left);
    buttonpanel_right      = mystrdup(right);
    buttonpanel_up         = mystrdup(up);
    make_buttonpannel();
    buttonpanel_open = true;
    return *this;
}
    

/**---------------------------------------------------------------------------
  * HTML::make_buttonpannel()
  *
  * Erzeugt eine Knopfleiste mit Pfeilen links, rechts, hoch, Anleitung
  * und alternativ auch einen Link auf die Weltkarte.
  ---------------------------------------------------------------------------*/
HTML& HTML::make_buttonpannel()
{
    short old_ib = image_border;
    image_border=0;
    
    if (buttonpanel_open) horizontal_rule();
    
    if (buttonpanel_left) {
	href(buttonpanel_left);
	iconimage("zurueck", L("Zur~uck","Back"));
	end_href();
    }
    else iconimage("nzurueck");
    
    text(" ");
    if (buttonpanel_right) {
	href(buttonpanel_right);
	iconimage("weiter", L("Weiter","Forward"));
	end_href();
    }
    
    else iconimage("nweiter");
    
    text(" ");
    if (buttonpanel_up) {
	href(buttonpanel_up);
	iconimage("hoch", L("Hauptseite","Main Page"));
	end_href();
    }
    else iconimage("nhoch");
    
    text(" ");
    
    if (buttonpanel_with_world)
    {
	text(" ");
	href("welt0101");
	iconimage("welt", L("Weltkarte","World Map"));
	end_href();
    }
	
    text(" "); href_manual("index");  iconimage("anleitun", L("Spielregeln","Rulebook")); end_href();
    image_border = old_ib;
    
  if (!buttonpanel_open) horizontal_rule();

  return *this;
}

HTML& HTML::ueberschrifts_balken(char *text)
{
    HTML html(*this); // Save kontext

    html.paragraph()
	.set_table_color("#d0d0d0")
	.set_table_width("100%")
	.set_cell_spacing(0)
	.set_cell_padding(2)
	.set_table_border(0)
	.table()
	.next_row()
	.next_cell()
	.font_size(2)
	.bold(text)
	.end_font()
	.end_table()
	.paragraph();
    return *this;
}


/**---------------------------------------------------------------------------
  * HTML::href_objekt(OBJEKT *)
  *
  * Fuegt einen Link auf ein Prometheusobjekt ein. Dabei wird der Filename
  * beruecksichtig, damit Links innerhalb der Seite optimiert werden.
  ---------------------------------------------------------------------------*/
HTML& HTML::href_objekt(OBJEKT *objekt)
{
    char seite[100];
    char *anker;
    
    if (objekt->typ_ist("EINHEIT")) {
	strcpy(seite, "einheit");
	anker = objekt->name;
    }
    else if (objekt->typ_ist("STADT")) {
	sprintf(seite, "S_%s", objekt->name);
	anker = 0;
    }
    else if (objekt->typ_ist("WELTBAUT")) {
	strcpy(seite, "einricht");
	anker = objekt->name;
    }
    else if (objekt->typ_ist("STAAT")) {
	strcpy(seite, "staat");
	anker = 0;
    }
    else { // Fehler.
	strcpy(seite, "staat");
	anker = 0;
    }

    // Wenn der Link auf dieselbe Seite zeigt, muss ich die seite
    // bei href weglassen! Ich suche von hinten nach '/'.
    char *pos = strrchr(filename, '/');
    char *rest = pos ? pos+1 : filename;
    if (strlen(rest) == strlen(filename)-4
	&& !strncmp(rest, filename, strlen(filename)-4))
	return href(0, anker);
    else return href(seite, anker);
}

/**---------------------------------------------------------------------------
  * HTML::href_info(char *)
  *
  * Fuegt einen Link auf ein Info ein.
  ---------------------------------------------------------------------------*/
HTML& HTML::href_info(char *name)
{
    char link[256];
    sprintf(link, "%s%s",INFO_BASE, name);
    return href(link);
}


/**---------------------------------------------------------------------------
  * HTML::href_manual(char *, char *)
  *
  * Fuegt einen Link aufs Regelwerkt ein.
  ---------------------------------------------------------------------------*/
HTML& HTML::href_manual(char *name, char *anchor)
{
    char link[256];
    sprintf(link, "%s%s",MANUAL_BASE, name);
    return href(link, anchor);
}

