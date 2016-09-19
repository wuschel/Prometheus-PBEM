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
  * MODUL:		doublist.h  /  DOUBLIST.H
  * AUTOR/DATUM:	Mathias Kettner, 8. Januar 1993
  * KOMPATIBILITAET:	C++
  ---------------------------------------------------------------------------*/
//
//      Definitionen zweier Typen, die zusammen eine doppelt verkettete
//	Liste implementieren. Der eine Typ stellt einen Listenkopf dar,
//	der zweite Typ ein Element in dieser Liste. Von diesem Typ kann
//	eine eigene Klasse abgeleitet werden. Somit koennen eigene Ele-
//	mente in einer Liste zusammengefasst werden.
//	Auf der Liste und auf den Elementen sind Funktionen definiert,
//	die alles noetige zur Verfuegung stellen.
//
// *************************************************************************

#ifndef __list_h
#define __list_h

#include <stdlib.h>

/**---------------------------------------------------------------------------
  * KLASSE:		LIST
  * 
  * Listenkopf. Der Listenkopf der doppelt verketteten Liste enthaelt
  * zwei Knoten, den headnode und den tailnode. Beide enthalten ausser
  * den Verknupfungszeigern keine weiteren Daten und dienen nur als
  * Begrenzungsmarken der Liste. Durch diese beiden Zusaetlichen Knoten
  * werden beim Einfuegen und beim Entfernen Fallunterscheidungen ueber-
  * fluessig. Ausserdem kann eine Knoten ohne eine Information ueber den
  * Listenkopf entfernt werden. Dadurch kann sich ein Knoten in seinem
  * Destructor selbst aus der Liste entfernen.
  *
  * ELEMENTFUNKTIONEN:
  * Hier werden nur diejenigen Funktionen beschrieben, die in der Klassen
  * deklaration als inline-Code vorhanden sind. Die uebrigen Funktionen
  * sind in alg_doublist.C beschrieben.
  *
  * void insert_after(LISTNODE *)
  * Fuegt diesen Knoten in eine Liste nach dem spezifizierten
  * Knoten ein. Beachte, dass zum Einfuegen ein Wissen ueber
  * den Listenkopf nicht erforderlich ist. Zum Einfuegen an
  * den Kopf einer Liste koennte man diese Funktion verwenden,
  * wenn man als Vorgangerknoten den headnode angibt.
  *
  * void insert_before(LISTNODE *)
  * Fuegt diesen Knoten vor dem spezifizierten Knoten ein.
  *
  * short is_last()
  * Gibt 1 zurueck, wenn dies der letze Knoten (tailnode nicht
  * mitgezaehlt!) in der Liste ist, sonst 0.
  *
  * short is_first()
  * Gibt 1 zurueck, wenn es sich um den ersten Knoten in der
  * Liste handelt (headnode nicht mitgezaehlt!), sonst 0.
  *
  * short is_tail()
  * Gibt 1 zurueck, wenn es sich bei dem Knoten um einen tailnode
  * handelt (enthaelt keine Daten mehr), sonst 0.
  *
  * short is_head()
  * Gibt 1 zurueck, wenn es sich bei dem Knoten um einen headnode
  * handelt (enthaelt keine Daten mehr), sonst 0.
  *
  * LISTNODE *next()
  * Liefert von diesem Knoten ausgehend die Referenz zum naechsten
  * Knoten in Richtung head...tail.
  *
  * LISTNODE *previous()
  * Liefert eine Referenz zum vorhergehenden Knoten.
  *
  * virtual void Print()
  * Macht auf der Basisklasse garnichts. Diese Funktion kann
  * ueberladen werden und zur Ausgabe eines Knotens verwendet
  * werden. Sie wird von der Funktion Print() im Listenkopf
  * aufgerufen, mit der dann die ganze Liste ausgegeben werden
  * kann.
  *
  * virtual short matches(void *)
  * Muss ueberladen werden, wenn die Funktion find() des Listen-
  * kopfes verwendet werden soll. Ihr wird ein Zeiger auf
  * eine Spezifizierung (z.B. ein Wert, den ein Eintrag im
  * gesuchten Element haben soll) uebergeben und sie liefert
  * 1, wenn es sich bei dem implizit Knoten um den gesuchten
  * handelt, sonst 0.
  *
  ---------------------------------------------------------------------------*/

class LISTNODE
{
private:
  LISTNODE *successor;		// Nachfolgerknoten
  LISTNODE *predecessor;	// Vorgaengerknoten
public:
  LISTNODE() { successor = predecessor = NULL; };
  virtual ~LISTNODE();			// destructor
  void insert_after(LISTNODE *);
  void insert_before(LISTNODE *);
  void remove();

  short is_last()  { return next()->is_tail(); }
  short is_first() { return previous()->is_head(); };
  short is_tail() { return successor == NULL; };
  short is_head() { return predecessor == NULL; };
  LISTNODE *next() { return successor; };
  LISTNODE *previous() { return predecessor; };

  virtual short matches(void *) { return 0; };

  friend class LIST;
};

typedef short((*SORTFKT)(LISTNODE *, LISTNODE *, void *));

class LIST
{
private:
  LISTNODE headnode;
  LISTNODE tailnode;
public:
  LIST();
  virtual ~LIST();
  void clear();  // Ganze Liste loeschen
  void add_tail(LISTNODE *n) { n->insert_before(&tailnode); };
  void add_head(LISTNODE *n) { n->insert_after(&headnode); };
  void insert(LISTNODE *to_insert) { add_tail(to_insert); };
  void merge(LIST *source); // Vereinigung mit einer anderen Liste
  virtual LISTNODE *find(void *); // Suchen mit Kriterium LISTNODE::matches()
  void sort(SORTFKT, void *data=NULL);
  short is_empty() { return headnode.successor->successor == NULL; };
  LISTNODE* first() { return headnode.successor; };
  LISTNODE* last()  { return tailnode.predecessor; };
  LISTNODE* random(); // Gibt zufaellig irgendeinen Knoten zurueck.
  void shuffle(); // Mischt die Liste zufaellig.
  long number_of_elements();
  long count_and_del() { long c=number_of_elements(); delete this; return c; };

  friend class LISTNODE;
};


#endif // __list_h
