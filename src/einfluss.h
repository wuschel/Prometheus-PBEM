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
  * MODUL:               einfluss.h / EINFLUSS.H
  * AUTOR/DATUM:         Mathias Kettner, 12. Mai 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Enthaelt Deklarationen der Klassen EINFLUSS und EINFLUSS_LISTE_
//      CLA.
//
// **************************************************************************

#ifndef __einfluss_h
#define __einfluss_h

#include "zielgrup.h"

/**---------------------------------------------------------------------------
  * KLASSE:              EINFLUSS
  * ABGELEITET VON:      DOUBLIST_NODE
  * 
  * Ein Einfluss ist eine spezielle Art von Relation, die zwischen Ob-
  * jekten (von OBJEKT abgeleitet) bestehen kann. Ein Objekt kann
  * dabei auf eine Gruppe von anderen Objekte "einen Einfluss ausueben".
  * Ein solcher Einfluss wird meist beim Neuschaffen eines Objektes
  * von diesem aufgestellt. Wird ein Objekt vernichtet, so werden auto-
  * matisch alle Einfluesse, die von ihm ausgehen, aufgehoben. Alle
  * bestehenden Einfluesse sind in der globalen Variablen globale_ein-
  * fluss_menge organisiert.
  *
  * Die Spezifikation des Einflussbereiches eines Einflusses, d.h. alle
  * Objekte, die beeinflusst werden, geschieht durch eine Variable vom
  * Typ ZIELGRUPPE. Ein Einfluss kann sich statt auf ein individuel-
  * les Objekt auch auf eine Gruppe auswirken, die durch sich durch be-
  * stimmte Merkmale auszeichnet (siehe zielgrup.h / ZIELGRUP.H). Ein
  * Zeiger auf die Zielgruppenstruktur befindet sich in zielgruppe.
  *
  * Identifiziert wird ein Einfluss mit einem Namen.
  *
  * Die Menge der Einfluesse ist im allgemeinen keine binaere Relation.
  * Jeder Einfluss kann mit einem Parameter ausgestattet (gewichtet o.ae.)
  * werden, der in Form eines Strings angegeben wird.
  *
  * FUNKTIONEN:
  * 
  * EINFLUSS(OBJEKT *, char *, char *, char *par=NULL);
  * Konstruktor mit Angabe der Zielgruppe als Strings.
  *
  * EINFLUSS(OBJEKT *, char *art, char *par, ZIELGRUPPE *);
  * Konstruktor mit Angabe der Zielgruppe als Zeiger auf Struktur
  *
  * ~EINFLUSS();
  * Destruktor
  * 
  ---------------------------------------------------------------------------*/

class EINFLUSS : public DOUBLIST_NODE
{
  short aktiv; // Wenn 0, dann Einfluss voruebergehend wirkungslos
  OBJEKT *beeinflusser;  // Ausloesendes Objekt
  char *art_des_einflusses;
  char *parameter;
  ZIELGRUPPE *zielgruppe;
public:
  EINFLUSS(OBJEKT *, char *, char *, char *par=NULL);
  EINFLUSS(OBJEKT *, char *art, char *par, ZIELGRUPPE *, short a=1);
  ~EINFLUSS();
  friend class EINFLUSS_LISTE;
  friend class OBJEKT;
  short matches(void *);
};

#define EINFLUSS_SUMME 1
#define EINFLUSS_PRODUKT 2
#define EINFLUSS_MAXIMUM 3
#define EINFLUSS_MINIMUM 4


/**---------------------------------------------------------------------------
  * KLASSE:              EINFLUSS_LISTE
  * ABGELEITET VON:      DOUBLIST
  * 
  * Liste, in der Variable vom Typ EINFLUSS zusammengefasst werden.
  * Diese Klasse ist nur eingefuert worden, um einige Operationen zu
  * definieren, die ueber die Moeglichkeiten von DOUBLIST hi-
  * nausgehen. Eigene Variable enthaelt die Klasse nicht.
  *
  * FUNKTIONEN:
  *
  * EINFLUSS *finde_einfluss(OBJEKT *, char *);
  * Sucht nach einem bestimmten  Einfluss auf ein Objekt
  *
  * long zusammenfassen(OBJEKT *, char *, short modus);
  * Fasst alle Einfluesse der gleichen Art zusammen
  *
  * void einfluesse_loeschen(OBJEKT *, char *art = NULL)
  * Loescht alle Einfluesse bestimmter Art von einem Objekt
  *
  * short speichern(FILE *, char *);
  * Speichert die Einflussliste in eine Datei.
  *
  * short laden(FILE *, char *);
  * Laedt Einfluesse aus einer Datei.
  *
  ---------------------------------------------------------------------------*/

typedef char *(*EIN_ZUS_FKT)(char *,char *);

class EINFLUSS_LISTE : public DOUBLIST
{
public:
  EINFLUSS *finde_einfluss(OBJEKT *, char *);
  long zusammenfassen(OBJEKT *, char *, short modus);
  char *zusammenfassen(OBJEKT *, char *, EIN_ZUS_FKT);
  void einfluesse_loeschen(OBJEKT *, char *art = NULL, short modus=0);
  short speichern(FILE *, char *);
  short laden(FILE *, char *);
  short kommando_fuer_jeden_einfluss(OBJEKT*, char *art, char *kom);
};


struct EINFLUSS_FIND_SPEC_STR
{
  OBJEKT *obj;
  char *art;
};

#endif // __einfluss_h

