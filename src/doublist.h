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

#ifndef __doublist_h
#define __doublist_h

#include <stdlib.h>

/**---------------------------------------------------------------------------
  * KLASSE:		DOUBLIST
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
  * void insert_after(DOUBLIST_NODE *)
  * Fuegt diesen Knoten in eine Liste nach dem spezifizierten
  * Knoten ein. Beachte, dass zum Einfuegen ein Wissen ueber
  * den Listenkopf nicht erforderlich ist. Zum Einfuegen an
  * den Kopf einer Liste koennte man diese Funktion verwenden,
  * wenn man als Vorgangerknoten den headnode angibt.
  *
  * void insert_before(DOUBLIST_NODE *)
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
  * DOUBLIST_NODE *next()
  * Liefert von diesem Knoten ausgehend die Referenz zum naechsten
  * Knoten in Richtung head...tail.
  *
  * DOUBLIST_NODE *previous()
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
  * FRIEND-FUNKTIONEN:	---
  *
  * FRIEND-KLASSEN:	DOUBLIST
  *
  * OEFFENTLICHE ELEMENTE (VARIABLEN/KONSTANTEN): ---
  *
  * AUTOR:              Mathias Kettner
  * ERSTELLT AM:        13. April 1993
  ---------------------------------------------------------------------------*/

class DOUBLIST_NODE
{
private:
  DOUBLIST_NODE *successor;		// Nachfolgerknoten
  DOUBLIST_NODE *predecessor;		// Vorgaengerknoten
public:
  DOUBLIST_NODE() { successor = predecessor = NULL; };
  virtual ~DOUBLIST_NODE();			// destructor
  void insert_after(DOUBLIST_NODE *);
  void insert_before(DOUBLIST_NODE *);
  void remove();
  short is_last()  { return next()->is_tail(); }
  short is_first() { return previous()->is_head(); };
  // short is_element(DOUBLIST *);
  short is_tail() { return successor == NULL; };
  short is_head() { return predecessor == NULL; };
  DOUBLIST_NODE *next() { return successor; };
  DOUBLIST_NODE *previous() { return predecessor; };

  virtual void Print() {};
  virtual short matches(void *) { return 0; };

  friend class DOUBLIST;
};


/**---------------------------------------------------------------------------
  * KLASSE:
  * 
  ---------------------------------------------------------------------------*/

typedef short((*SORTFKT)(DOUBLIST_NODE *, DOUBLIST_NODE *, void *));

class DOUBLIST
{
private:
  DOUBLIST_NODE headnode;
  DOUBLIST_NODE tailnode;
public:
  DOUBLIST();
  virtual ~DOUBLIST();
  void clear();  // Ganze Liste loeschen
  void add_tail(DOUBLIST_NODE *n) { n->insert_before(&tailnode); };
  void add_head(DOUBLIST_NODE *n) { n->insert_after(&headnode); };
  void insert(DOUBLIST_NODE *to_insert) { add_tail(to_insert); };
  void merge(DOUBLIST *source); // Vereinigung mit einer anderen Menge
  virtual DOUBLIST_NODE *find(void *);
  void sort(SORTFKT, void *data=NULL);
  void shuffle(); // zufaelliges Mischen.
  short is_empty() { return headnode.successor->successor == NULL; };
  DOUBLIST_NODE* first() { return headnode.successor; };
  DOUBLIST_NODE* last()  { return tailnode.predecessor; };
  DOUBLIST_NODE* any()  { return first(); };
  DOUBLIST_NODE* random(); // Zufaelliger Knoten.
  long number_of_elements();
  long count_and_del() { long c=number_of_elements(); delete this; return c; };
  void call_for_each_element(void (*function)(DOUBLIST_NODE*, void *),
			     void *parameter = NULL);
  void Print();

  friend class DOUBLIST_NODE;
};


#endif // __doublist_h

