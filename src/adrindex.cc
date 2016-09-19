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


#include "adrindex.h"
#include "adrcache.h" 

ADRINDEX::ADRINDEX(int w, int h)
{
  welt_breite = w;
  welt_hoehe = h; 
  
  table = new ADRINDEX_ENTRY * * [welt_breite];
  for (int x=0; x<welt_breite; x++) 
  {
    table[x] = new ADRINDEX_ENTRY * [welt_hoehe];
    for (int y=0; y<welt_hoehe; y++)  table[x][y] = ADRINDEX_ENTRY_NIL;
  }
}

ADRINDEX::~ADRINDEX()
{
  for (int x=0; x<welt_breite; x++) {
    for (int y=0; y<welt_hoehe; y++) {
      if (table[x][y]) {
	table[x][y]->deleteChain();
	delete table[x][y];
      }
    }
    delete table[x];
  }
  delete table;
}

void ADRINDEX::enter_objekt(OBJEKT *objekt, ADR& adr)
{
  table[adr.x][adr.y] = new ADRINDEX_ENTRY(objekt, table[adr.x][adr.y]); 
}

void ADRINDEX::remove_objekt(OBJEKT *objekt)
{
  int x = objekt->adresse.x,
      y = objekt->adresse.y;
      
  ADRINDEX_ENTRY *entry = table[x][y];
  ADRINDEX_ENTRY *vorheriger = NULL;

  while (entry != ADRINDEX_ENTRY_NIL)
  {
    if (entry->objekt == objekt) {
      if (vorheriger == NULL) table[x][y] = entry->next;
      else  vorheriger->next = entry->next;
      delete entry; 
      return;
    }
    
    vorheriger = entry;
    entry = entry->next;
  }

  // Fehler!!!
  
  printf("ACHTUNG! Fehler im ADRINDEX! Objekt %s sollte entfernt werden "
  "und war nicht im Index!\n", objekt->name);
}


void ADRINDEX::objekte_in_liste_aufnehmen(DOUBLIST *liste, int x, int y, char *bed)
{
  ADRINDEX_ENTRY *entry = table[(x+welt_breite) % welt_breite][y];
  while (entry != ADRINDEX_ENTRY_NIL)
  {
    if (!entry->objekt->zur_vernichtung_vorgemerkt() &&
        (!bed || entry->objekt->bedingung_erfuellt(bed)))
    {
      liste->insert(new OBJEKT_LIST_NODE(entry->objekt));
    }
    entry = entry->next;
  }
}


DOUBLIST *ADRINDEX::alle_objekte_bei_adresse(ADR& adr, char *bed)
{
  DOUBLIST *resultlist = new DOUBLIST;
  objekte_in_liste_aufnehmen(resultlist, adr.x, adr.y, bed);
  return resultlist;
}

                               
DOUBLIST *ADRINDEX::alle_objekte_im_umkreis_von(ADR& center, 
						float radius, char *bed)
{
  #include "adrcache.h"  // von gencache vorberechnetes Array!
  // const int x_koords[] =    {  0, -1,  0. ...
  // const int x_koords[] =    {  0,  0, -1, ...
  // const int sectionlist[] = {  0,  0,  4, ...
  // const int number_of_sections = ...

  DOUBLIST *feldliste = new DOUBLIST; // Ergebnisliste
  int sectionnumber = (int)floor(radius*2);
   
  if (sectionnumber >= number_of_sections) 
    return langsam_alle_objekte_im_umkreis_von(center.x, center.y, radius, bed);
  
  for (int entry = 0; entry <= sectionlist[sectionnumber]; entry++)
  {
    int x = center.x + x_koords[entry];
    int y = center.y + y_koords[entry];
    if (y<0 || y>=welt_hoehe) continue; // Ausserhalb der Welt.
    objekte_in_liste_aufnehmen(feldliste, x, y, bed);
  }
  
  return feldliste;
}                       

DOUBLIST *ADRINDEX::langsam_alle_objekte_im_umkreis_von
  	(int center_x, int center_y, float radius, char *bed)
{
  DOUBLIST *feldliste = new DOUBLIST;

  // Als erstes bestimme ich ein Quadrat, das so gross ist, dass sich
  // auf jeden Fall alle gesuchten Felder darin befinden. Die Kantenlaenge
  // ist dabei 2*radius+1, da von Mitte zu Mitte gerechnet wird und
  // deshalb auf beiden Seiten noch ein Feld hinzukommt.

  int intradius = (int)ceil(radius);
  long links =  center_x - intradius;
  long rechts = center_x + intradius;
  long oben =   MAX(0, center_y - intradius);
  long unten =  MIN(welt_hoehe-1, center_y + intradius);

  for (long x=links; x<=rechts; x++)
  {
    for (long y=oben; y<=unten; y++)
    {
      if (radius_kleiner_gleich(x-center_x, y-center_y, radius))
                  objekte_in_liste_aufnehmen(feldliste, x, y, bed);
    }
  }
  return feldliste;
}
