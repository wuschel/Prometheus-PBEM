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
  * MODUL:               schaffen.C / SCHAFFEN.CPP
  * AUTOR/DATUM:         
  * KOMPATIBILITAET:     C++
  ---------------------------------------------------------------------------*/
//
//      Dieses Modul enthaelt nur eine einzige Funktion, welche zum schaffen
//      eines neuen Objektes dient. In sie muss fuer jeden Objekttyp die
//      entsprechende Zeile eingefuegt werden.
//
// *************************************************************************
      
#include "objekt.h"
#include "uhr.h"
#include "einheit.h"
#include "stadtaus.h"
#include "stadt.h"
#include "staat.h"
#include "welt.h"
#include "enzyklop.h"
#include "weltbaut.h"


/**---------------------------------------------------------------------------
  * OBJEKT::schaffen()
  * 
  * Diese Funktion muss bei der Spezifizierung eines konketen Modells
  * vom Programmierer erweitert werden. Fuer jeden Objektdtyp, den
  * er definiert, muss er hier eine Zeile einfuegen, die ein neues
  * Objekt von diesem  Typ schafft.
  *
  * Die Funktion setzt anschliessend das Attribut TYP auf den Typ des
  * neuen Objektes und fuegt es dem Besitz des impliziten (aufrufenden)
  * Objektes hinzu.
  *
  * Merke:  Mit diese Funktion kann nur ein Objekt geschaffen werden,
  * wenn bereits ein anderes existiert. Es ist nicht erlaubt,
  * diese Funktion mit einem NULL-Pointer als this-Pointer
  * aufzurufen. Ueberhaupt soll in einem konkreten Modell
  * nur ein einziges Objekts existieren, das keinen Besitzer
  * hat. Dieses wird einfach mit 'new' erzeugt.
  *
  * @param
  * gew_name:               Gewuenschter Name fuer das Objekt.
  * gew_typ:                Typ (String) des zu schaffenden Objektes.
  * gew_attr:               Attribute (String), die das Objekt bekommt
  *
  * @return
  * Zeiger auf das neue Objekt oder NULL, wenn das Objekt nicht ge-
  * schaffen werden konnte (Typ unbekannt oder zu wenig Speicher).
  * Beachten Sie das das neuen Objekt bereits im Besitz des Aufrufenden
  * ist!
  ---------------------------------------------------------------------------*/
OBJEKT *OBJEKT::objekt_schaffen(char *gew_name, char *gew_typ, 
		char *gew_attr=NULL)
{
  OBJEKT *objekt = NULL;

  if      (!strcmp(gew_typ, "UHR"))     
				objekt = new UHR(gew_name, gew_attr);

  else if (!strcmp(gew_typ, "EINHEIT")) 
				objekt = new EINHEIT(gew_name, gew_attr);

//  else if (!strcmp(gew_typ, "HELD")) 
//                                objekt = new HELD(gew_name, gew_attr);

  else if (!strcmp(gew_typ, "STADTAUSBAU"))
				objekt = new STADTAUSBAU(gew_name, gew_attr);

  else if (!strcmp(gew_typ, "STADT"))
				objekt = new STADT(gew_name, gew_attr);

  else if (!strcmp(gew_typ, "WELTBAUT"))
				objekt = new WELTBAUT(gew_name, gew_attr);

  else if (!strcmp(gew_typ, "STAAT"))
				objekt = new STAAT(gew_name, gew_attr);

  else if (!strcmp(gew_typ, "WELT"))
				objekt = new WELT(gew_name, gew_attr);

  else if (!strcmp(gew_typ, "ENZYKLOPAEDIE"))
			   objekt = new ENZYKLOPAEDIE(gew_name, gew_attr);


  if (!objekt) return NULL; // Objekt konnte nicht geschaffen werden!

  objekt->attribut_setzen("TYP", gew_typ);
  in_besitz_nehmen(objekt);
  return objekt;
}

