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


// -*- c++ -*-

#ifndef __html_h
#define __html_h

class STAAT;
class OBJEKT;

#include "uhr.h"
extern UHR *g_uhr; // von main.cpp

// Pfade, unter denen die Grafiken zu finden sind. "s" steht fuer small,
// also 24x24 Bilder. "f" fuer full, also 48x48.

#define SMALLIMAGE_BASE   "../s/"
#define SMALLEIMAGE_BASE  "../s/e"
#define FULLIMAGE_BASE    "../f/"
#define FIELDIMAGE_BASE   "../s/"
#define STREETIMAGE_BASE  "../r/"
#define ICON_BASE         "../i/"
#define MANUAL_BASE       "../a/"
#define INFO_BASE         "../infos/"

#define AL_LEFT		NULL
#define AL_RIGHT	"RIGHT"
#define AL_CENTER	"CENTER"

#define VAL_TOP		"TOP"
#define VAL_BOTTOM	"BOTTOM"
#define VAL_CENTER	NULL

class HTML 
{
    static char *documentbase;
    static char *default_background_image;
    
    FILE *file;
    char *filename;
    short subhtml;
    
    short row_open, 
	cell_open,
	bold_flag,
	italic_flag,
	courier_flag,
	fontsize;
    
    short table_border,
	cell_spacing,
	cell_padding,
	image_border;
    
    const char *cell_alignment,
	*cell_valignment,
	*cell_color,
	*row_color,
	*table_color,
	*table_width;
  
    int  cell_width,
	cell_height,
	column_span,
	row_span;
    
public:
    static void set_documentbase(char *);
  
public:
    HTML(char *, char *, char *bg=NULL);
    HTML(HTML& html);
    ~HTML();
    void init(char *, char *, char *bg=0);
    
    void set_to_defaults();
    
    short ok() { return file != NULL; };
    
    HTML& href(char *h, char *a=NULL);
    HTML& email_ref(char *email)  { fprintf(file, "<A HREF=\"mailto:%s\">", email); return *this; };
    HTML& href_raw(char *r)       { fprintf(file, "<A HREF=\"%s\">", r); return *this; };
    HTML& end_href()		  { fprintf(file, "</A>"); return *this; };
    HTML& anchor(char *a)	  { fprintf(file, "\n<A NAME=\"%s\">",a); return *this; };
    
    HTML& text(const char *);
    HTML& text(char a)            { char aa[2]; aa[0]=a; aa[1]=0; text(aa); return *this; };
    HTML& text(int  i)            { fprintf(file, "%d", i); return *this; };
    HTML& text(long l)		{ fprintf(file, "%ld", l); return *this; };
    HTML& text(FILE *);
    HTML& text_from_file(char *);
    HTML& include(char *s) { fprintf(file, "%s", s); return *this; };

    HTML& nonbreaking_space() { fprintf(file, "&nbsp;"); return *this; };
    
    HTML& heading(int l, char *t)
	{ fprintf(file, "\n<H%i>",l); text(t); 
	fprintf(file, "</H%i>\n",l); return *this; };
    HTML& paragraph()   	{ fprintf(file, "\n<P>"); return *this; };
    HTML& linebreak()		{ fprintf(file, "<BR>\n"); return *this; };
    HTML& space(int i)          { while (i-- >= 0) fprintf(file,"&nbsp;"); return *this; };
    HTML& horizontal_rule()	{ fprintf(file, "\n<HR>"); return *this; };
    HTML& center()		{ fprintf(file, "<CENTER>"); return *this; };
    HTML& end_center() 		{ fprintf(file, "</CENTER>"); return *this; };
    HTML& right()               { fprintf(file, "<DIV ALIGN=RIGHT>"); return *this; };
    HTML& end_right()           { fprintf(file, "</DIV>"); return *this; };
    
    HTML& bold()   		{ bold_flag=1; fprintf(file, "<B>"); return *this; };
    HTML& bold(char *s)  	{ bold(); text(s); bold_off(); return *this; };
    HTML& bold_off()		{ bold_flag=0; fprintf(file, "</B>"); return *this; };
    HTML& italic()		{ italic_flag=1; fprintf(file, "<I>"); return *this; };
    HTML& italic(char *s)  	{ italic(); text(s); italic_off(); return *this; };
    HTML& italic_off()		{ italic_flag=0; fprintf(file, "</I>"); return *this; };
    HTML& courier()		{ courier_flag=1; fprintf(file, "<T>"); return *this; };
    HTML& courier(char *s) 	{ courier(); text(s); courier_off(); return *this; };
    HTML& courier_off()		{ courier_flag=0; fprintf(file, "</T>"); return *this; };
    HTML& font_size(int s)      { fprintf(file, "<FONT SIZE=\"%+d\">", s); return *this; };
    HTML& font_color(char *c)   { fprintf(file, "<FONT COLOR=\"%s\">",c); return *this; };
    HTML& font(int s, char *c)  { fprintf(file, "<FONT SIZE=\"%+d\" COLOR=\"%s\">",s,c); return *this; };
    HTML& end_font()            { fprintf(file, "</FONT>"); return *this; };
    HTML& set_font_size(int s) { fontsize = s; font_size(s); return *this; };
    HTML& unset_font_size()    { if (fontsize) { fontsize=0; end_font(); }; return *this; };
    
    HTML& set_image_border(int w)	   { image_border = w; return *this; };
    HTML& unset_image_border()       { image_border = -1; return *this; };
    HTML& image(char *, char *alt=0, char*n1=0, char*n2=0);
    HTML& imagewo(char *);
    
    HTML& table();
    HTML& next_row();
    HTML& next_cell();
    HTML& next_cell(char *s)      { next_cell(); text(s); return *this; };
    HTML& next_cell(long l)       { next_cell(); text(l); return *this; };
    HTML& empty_cell()		{ next_cell(); fprintf(file, "&nbsp;"); return *this; };
    HTML& end_cell();
    HTML& end_row();
    HTML& end_table();

    // Modifikationen fuer table()
    
    HTML&   set_table_border(int w)	{ table_border = w;      return *this; };
    HTML& unset_table_border()          { table_border = -1;     return *this; };
    HTML&   set_table_color(const char *c)    { table_color = c;       return *this; };
    HTML& unset_table_color()	        { table_color = NULL;    return *this; };
    HTML&   set_cell_spacing(int w)     { cell_spacing = w;      return *this; };
    HTML& unset_cell_spacing()          { cell_spacing = -1;     return *this; };
    HTML&   set_cell_padding(int w)     { cell_padding = w;      return *this; };
    HTML& unset_cell_padding()          { cell_padding = -1;     return *this; };
    HTML&   set_table_width(const char *c)    { table_width = c;       return *this; };
    HTML& unset_table_width()           { table_width = 0;       return *this; };

    // Modifikationen fuer next_row()

    HTML&   set_row_color(char *c)      { row_color = c;         return *this; };
    HTML& unset_row_color()             { row_color = NULL;      return *this; };

    // Modifikationen fuer next_cell()

    HTML&   set_cell_alignment(const char *a) { cell_alignment = a;    return *this; };
    HTML& unset_cell_alignment()        { cell_alignment = NULL; return *this; };
    HTML&   set_cell_valignment(const char *a){ cell_valignment = a;   return *this; };
    HTML&   set_cell_color(const char *c)     { cell_color = c;        return *this; };
    HTML& unset_cell_color()            { cell_color = NULL;     return *this; };
    HTML&   set_cell_width(int w)       { cell_width = w;        return *this; };
    HTML& unset_cell_width()            { cell_width = -1;       return *this; };
    HTML&   set_cell_height(int h)      { cell_height = h;       return *this; };
    HTML& unset_cell_height()           { cell_height = -1;      return *this; };
    HTML&   set_column_span(int s)      { column_span = s;       return *this; };
    HTML& unset_column_span()           { column_span = 1;       return *this; };
    HTML&   set_row_span(int s)         { row_span = s;          return *this; };
    HTML& unset_row_span()              { row_span = 1;          return *this; };
    
    HTML& unset_all()                   { set_to_defaults(); return *this; };
  
    // Forms
    HTML& form(char *email) { fprintf(file, "<form action=\"mailto:%s\" method=\"POST\" "
				      "enctype=\"text/plain\">", email); return *this; };
    HTML& end_form() { fprintf(file, "</form>"); return *this; };
    HTML& hidden_input(char *n, char *v) { fprintf(file, "<input type=\"hidden\" name=\"%s\""
						   " value=\"%s\">", n, v); return *this; };
    HTML& textarea(char *n, int r, int c) { fprintf(file, "<textarea name=\"%s\" rows=\"%d\" cols="
						    "\"%d\">", n, r, c); return *this; };
    HTML& end_textarea() { fprintf(file, "</textarea>"); return *this; };
    HTML& submit_button(char *t) { fprintf(file, "<input type=\"submit\" value=\"")
				       ; text(t); fprintf(file,"\">"); return *this; };
    HTML& reset_button(char *t) { fprintf(file, "<input type=\"reset\" value=\"")
				       ; text(t); fprintf(file,"\">"); return *this; };

    // Grundeinstellung geltend fuer alle Objekte
    static void set_default_background_image(char *i) { default_background_image = i; };
    static void unset_default_background_image()      { default_background_image = NULL; };

    // Unterstuetzung speziell fuer Prometheus
    
    bool buttonpanel_open;
    bool buttonpanel_with_world;
    char *buttonpanel_left;
    char *buttonpanel_right;
    char *buttonpanel_up;
    
    HTML(STAAT *staat, char *var, char *title, bool info=false)
	{ init(printout_filename(staat, var, ".htm", info), title); }
    static char *printout_filename(STAAT *, char *, char *ext=0, bool info=false);
    
    HTML& smallimage(char *n, char *a=0)  { return image(SMALLIMAGE_BASE,  a, n, g_uhr->attribut("GFORMAT")); }
    HTML& smalleimage(char *n, char *a=0) { return image(SMALLEIMAGE_BASE, a, n, g_uhr->attribut("GFORMAT")); }
    HTML& fullimage(char *n, char *a=0)	  { return image(FULLIMAGE_BASE,   a, n, g_uhr->attribut("GFORMAT")); }
    HTML& iconimage(char *n, char *a=0)   { return image(ICON_BASE, a, n, g_uhr->attribut("GFORMAT")); }
    HTML& fieldimage(const char *n, const char *a=0);
    HTML& streetimage(unsigned char, bool);
    HTML& streetimage(char *n);
    
    HTML& buttonpanel(bool w=true, char *l=NULL, char *r=NULL, char*up="index");
    HTML& ueberschrifts_balken(char *text);
    HTML& href_objekt(OBJEKT *);
    HTML& href_manual(char *m, char *anchor=0);
    HTML& href_info(char *);
    
private:
    HTML& make_buttonpannel();
};

#endif // __html_h

