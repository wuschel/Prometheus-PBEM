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
  * MODUL:               landscha.C / LANDSCHA.CPP
  * AUTOR/DATUM:         Mathias Kettner, 30. April 1993
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Enthaelt Funktionen der Klasse LANDSCHAFT.
//
// **************************************************************************

#include "landscha.h"
#include "alg.h"
#include "listmac.h"

extern DOUBLIST globale_objekt_menge;

/**---------------------------------------------------------------------------
  * LANDSCHAFT::alle_objekte_bei_adresse()
  * 
  * Erstellt eine Liste mit Elementen vom Typ OBJEKT_LIST_NODE,
  * die einzig einen Zeiger auf ein Objekt enthalten. In dieser Liste
  * werden alle Objekte vermerkt, die sich an einem bestimmten ort in
  * der Landschaft befinden. Zusaetlich kann noch eine Bedingung an-
  * gegeben werden.
  *
  * @param
  * adr:            Adresse der gesuchten Objekte
  * bed:	        Bedingungsstring, z.B. TYP=STADT oder NULL
  *
  * @return
  * Zeiger auf eine Liste mit Referenzen, die nach Gebrauch vom Aufrufer
  * mit delete freigegeben werden muss.
  ---------------------------------------------------------------------------*/
DOUBLIST *BASIS_LANDSCHAFT::alle_objekte_bei_adresse
		(ADR& adr, char *bed)
{
  DOUBLIST *ergliste = new DOUBLIST;
  OBJEKT_LIST_NODE *objektnode =
      (OBJEKT_LIST_NODE *)globale_objekt_menge.first();

  while (!objektnode->is_tail())
  {
    if (objektnode->objekt->adresse == adr
	&&  objektnode->objekt->ort() == this) {

      // Falls das Objekt allerdings schon zur Vernichtung vorgemerkt ist,
      // dann taucht es nicht mehr in der Liste auf!

      if (!objektnode->objekt->zur_vernichtung_vorgemerkt()) {
	OBJEKT_LIST_NODE *neu = new OBJEKT_LIST_NODE;
	neu->objekt = objektnode->objekt;
	ergliste->insert(neu);
      }

    }
    objektnode = (OBJEKT_LIST_NODE *)objektnode->next();
  }

  // Jetzt filtere ich die Liste noch wegen den Bedingungen
  if (bed) filtere_objekt_liste(ergliste, bed);

  return ergliste;
}


/**---------------------------------------------------------------------------
  * 
  * 
  ---------------------------------------------------------------------------*/
DOUBLIST *BASIS_LANDSCHAFT::alle_objekte_im_umkreis_von
			(ADR& adr, float radius, char *bedingung)
{
  // Ich gehe einfach alle Objekte durch und bestimme ihren Abstand vom
  // Mittelpunkt

  DOUBLIST *ergebnis = new DOUBLIST;

  FOR_EACH_OBJEKT_IN (&globale_objekt_menge)
  DO (
    if (!objekt->zur_vernichtung_vorgemerkt()) {
      if (objekt->ort()==this &&
          entfernung_zwischen(adr, objekt->adresse) <= radius)
      {
        if (!bedingung || objekt->bedingung_erfuellt(bedingung)) {
	  OBJEKT_LIST_NODE *neu = new OBJEKT_LIST_NODE;
	  neu->objekt = objekt;
	  ergebnis->insert(neu);
        }
      }	
    }
  )

  return ergebnis;
}



/**---------------------------------------------------------------------------
  * BASIS_LANDSCHAFT::anzahl_objekte_bei_adresse()
  * 
  ---------------------------------------------------------------------------*/
long BASIS_LANDSCHAFT::anzahl_objekte_bei_adresse(ADR& ziel)
{
  DOUBLIST *adrliste = alle_objekte_bei_adresse(ziel);
  long ergebnis = adrliste->number_of_elements();
  delete adrliste;
  return ergebnis;
}
