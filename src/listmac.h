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


// Makros fuer das Abscannen von Objektlisten und Adrlisten.
// Bei der Version mit DO() darf auf keinen Fall ein Objekt der
// Liste geloescht werden!

#define FOR_EACH_OBJEKT_IN(liste)				\
{								\
  DOUBLIST *scanliste = liste;				\
  DOUBLIST_NODE *node = scanliste->first();		\
  while (!node->is_tail())					\
  {								\
    OBJEKT *objekt;						\
    objekt = ((OBJEKT_LIST_NODE *)node)->objekt;


#define FOR_EACH_ADR_IN(liste)					\
{								\
  DOUBLIST *scanliste = liste;				\
  DOUBLIST_NODE *node = scanliste->first();		\
  while (!node->is_tail())					\
  {								\
    ADR adr(((ADR_LIST_NODE *)node)->adresse);


#define DO(code)						\
    { code; }							\
    node = node->next();					\
  }								\
}


#define DO_AND_DELETE(code)					\
    { code; }							\
    delete node;						\
    node = scanliste->first();					\
  }								\
  delete scanliste;						\
}


/* Hier kommen Makros, die man verwenden kann, wenn man Listen von
   Hand durchscant. */
   
#define FIRST(list,node) { node = (typeof(node))((list)->first()); }
#define SCAN(list,node) FIRST(list,node); while (!node->is_tail())
#define NEXT(node) { node = (typeof(node))((node)->next()); }

/* Beispiel:

OBJEKT_LIST_NODE *onode;
DOUBLIST liste;

SCAN(&liste, onode)
{
  if (onode ist zu loeschen) {
    delete onode;
    FIRST(&liste, onode);
  }
  else NEXT(onode);
}

*/
