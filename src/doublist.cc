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
  * MODUL:               doublist.C  /  DOUBLIST.CPP
  * AUTOR/DATUM:         Mathias Kettner, 28. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//	Enthaelt die Funktionen zu den Klassen DOUBLIST	und 
//	DOUBLIST_NODE, die zusammen eine Datenstruktur fuer
//	doppelseitig verkettete Liste mit Head- und Tailnode imple-
//	mentieren.
//
// *************************************************************************

#include <string.h>

#include "doublist.h"

long io_random(long); // Soll Zufallszahl in bestimmten Bereich bringen.

/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
DOUBLIST::DOUBLIST()
{
  headnode.successor   = &tailnode;
  headnode.predecessor = NULL;
  tailnode.successor   = NULL;
  tailnode.predecessor = &headnode;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
DOUBLIST::~DOUBLIST()
{
  clear();
}



/**---------------------------------------------------------------------------
  * DOUBLIST::merge()
  * 
  * Verschmilzt zwei Listen auf folgende Weise: Aus der uebergebenen
  * Liste werden alle Elemente entfernt und an das Ende der impliziten
  * Liste angehaengt. Die Reihenfolgen der Elemente beider Listen bleibt
  * erhalten. Aus Effizienzueberlegungen werden nicht alle Elemente der
  * zweiten Liste der Reihe nach bearbeitet, sondern es wird die
  * gesamte Kette der Elemente (alle Nodes bis auf Head und Tail) kom-
  * plett entfernt und in die implizite Liste eingebunden (Wie wenn
  * ein Virus seine DNA in eine andere DNA einbindet).
  * 
  * Merke: Die uebergebene Liste ist anschliessen LEER! Die Elemente
  * werden also nicht kopiert, sondern nur einige wenige Zeiger geaendert.
  * @param
  * DOUBLIST *source:  Quell-Liste, aus der die Elemente entfernt
  * werden.
  ---------------------------------------------------------------------------*/
void DOUBLIST::merge(DOUBLIST *source)
{
  if (source->is_empty()) return;

  // Neue Teilkette hinten anheften (Bildet eine Schlaufe)
  source->first()->predecessor = tailnode.predecessor;
  source->last()->successor = &tailnode;

  // Alte Liste auftrennen und neue Teilkette einbinden
  tailnode.predecessor->successor = source->first();
  tailnode.predecessor = source->last();

  // Teilkette aus alter Liste entfernen -> leere Liste
  source->headnode.successor = &source->tailnode;
  source->tailnode.predecessor = &source->headnode;
}



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
DOUBLIST_NODE *DOUBLIST::find(void *feature)
{
  DOUBLIST_NODE *search=first();

  while (!search->is_tail())
  {
    if (search->matches(feature)) return search;
    else search = search->next();
  }
  return NULL;
}



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
long DOUBLIST::number_of_elements()
{
  long anzahl=0;
  DOUBLIST_NODE *node = first();
  while (!node->is_tail())
  {
    anzahl++;
    node = node->next();
  }
  return anzahl;
}



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void DOUBLIST::clear()
{
  while (!is_empty()) delete any();
}



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void DOUBLIST::call_for_each_element
		(void (*function)(DOUBLIST_NODE *, void *), void *par)
{
  DOUBLIST_NODE *node, *next;
  node = first();
  while (!node->is_tail()) {
    next = node->next();
    function(node, par);
    node = next;
  }
}



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void DOUBLIST::Print()
{
  DOUBLIST_NODE *node = first();
  while (!node->is_tail())
  {
    node->Print();
    node = node->next();
  }
}



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
DOUBLIST_NODE::~DOUBLIST_NODE()
{
  remove();
}



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void DOUBLIST_NODE::insert_after(DOUBLIST_NODE *node)
{
  predecessor = node;
  successor = node->successor;
  predecessor->successor = this;
  successor->predecessor = this;
}



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void DOUBLIST_NODE::insert_before(DOUBLIST_NODE *node)
{
  successor = node;
  predecessor = node->predecessor;
  predecessor->successor = this;
  successor->predecessor = this;
}  



/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
void DOUBLIST_NODE::remove()
{
  if (!this) {
    return;
  }

  if (!predecessor || !successor) return; // Node ungueltig!
  successor->predecessor = predecessor;
  predecessor->successor = successor;
  successor = predecessor = NULL; // Zur Sicherheit!
}



/**---------------------------------------------------------------------------
  * DOUBLIST::sort()
  * 
  * Sortiert eine Liste nach einem beliebigen Kriterium.
  ---------------------------------------------------------------------------*/
void DOUBLIST::sort(SORTFKT sortfunction, void *data)
{
  if (number_of_elements() <= 1) return; // Muessig.

  // Ich mache einen Bubblesort, da er auf den Listen noch einigermassen
  // uebersichtlich funktioniert.

  short sortiert;
  do {
    sortiert = 1;
    DOUBLIST_NODE *node = first();
    while (!node->is_last()) {
      if (sortfunction(node, node->next(), data) > 0) // Dann tauschen...
      {
	DOUBLIST_NODE *nextnode = node->next();
	nextnode->remove();
	nextnode->insert_before(node); // Dadurch wandert node nach vorne
	sortiert = 0;
      }
      else node = node->next();
    }

  } while (!sortiert);
}


/**---------------------------------------------------------------------------
  * DOUBLIST::shuffle()
  * 
  * Bewirkt praktisch das Gegenteil von sort(), denn hier wird die
  * List gemischt. Komplexitaet quadratisch in der Anzahl der Knoten.
  ---------------------------------------------------------------------------*/
void DOUBLIST::shuffle()
{
  long anzahl = 3 * number_of_elements();
  while (anzahl) {
    
    // Ich nehme immer einen zufaelligen Knoten und haenge ihn wieder
    // abwechselnd an das Ende bzw. an den Anfang. 
    
    DOUBLIST_NODE *node = random();
    node->remove();
    if (anzahl % 2) add_head(node);
    else add_tail(node);
    anzahl--;
    
  }
}

/**---------------------------------------------------------------------------
  * DOUBLIST::random()
  * 
  * Waehlt irgendeinen Knoten der Liste zufaellig aus. NULL, wenn die
  * Liste leer ist. Komplexitaet linear in der Kontenzahl.
  ---------------------------------------------------------------------------*/
DOUBLIST_NODE *DOUBLIST::random()
{
  long anzahl = number_of_elements();
  if (!anzahl) return NULL;
  
  long auswahl = io_random(anzahl);
  DOUBLIST_NODE *node = first();
  while (auswahl) {
    node = node->next();
    auswahl--;
  }
  return node;
}

