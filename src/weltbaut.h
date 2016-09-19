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
/**---------------------------------------------------------------------------
  * MODUL:		weltbaut.h  /  WELTBAUT.H
  * AUTOR/DATUM:		Mathias Kettner, 19. August 1993
  * KOMPATIBILITAET:	C++
  ---------------------------------------------------------------------------*/
//
//  	Definiert die Klassse WELTBAUT, die ein Objekt darstellt,
//	das in der Landschaft steht, nicht mobil ist, aber auch keine
//	Stadt ist. Es hat nicht viel Funktionalitaet und gleicht den
//	Staedten zu wenig, um es ueber das Objekt STADT abzuwickeln.
//
// **************************************************************************

#ifndef __weltbaut_h
#define __weltbaut_h

#include "objekt.h"
#include "resource.h"

class STAAT;
class STADT;
class WELT;

class WELTBAUT : public OBJEKT
{
  RESOURCE_VEKTOR resourcenspeicher;

  RESOURCE_VEKTOR gefoerdert; // Nicht speichern, nur fuer den Ausdruck.
  RESOURCE_VEKTOR transfer;  // Auch nur wegen der Statistik.
  RESOURCE_VEKTOR alte_resourcen; // Auch Statistik. Res am Anfang d. Zug
public:
    WELTBAUT(char *, char *);
    ~WELTBAUT() {};
    short speichern(FILE *);
    short laden(FILE *);
    char *info(char *, void *par1=NULL, void *par2=NULL, void *par3=NULL);
    short befehl_auswerten(char *, long);
    short kommando(const char *, const void *par1=NULL, const void *par2=NULL, const void *par3=NULL);
    void naechster_zug(long);
    void naechste_phase(long);
    void abschlussbericht();
    void abschlussbericht_html(HTML);
    STADT *versorger();
    
private:
    char *info_resourcen_anfordern(void *);
    char *info_resourcen_anfordern_2(void *, void *, void *);
    char *info_rohstoffproduktion();
    
    short kommando_resourcen_bekommen(const void *);
    short kommando_resourcen_anfordern(const void *);
    short kommando_eingenommen(const void *, const void *);
    short kommando_beschossen_werden(const void *, const void *);
    short kommando_meteorit(const void *, const void *);
    short kommando_rakete_stationieren(const void *);
    short kommando_atomeinschlag(const void *);
    short kommando_atom_detect(const void *, const void *);

    short befehl_ueberstellen(char *); // Neuer Versorger
    short befehl_auslieferung_einstellen(); // Toggelt ebenfalls
    short befehl_rakete_abschiessen(char *);
    short befehl_aufloesen(); // XX

    void abschlussbericht_layouten();
    void unterhalt_einholen();
    void resourcen_foerdern();
    void restliche_welt_informieren(ADR&);
    WELT *welt();
    ADR& absolute_adresse(char *);
    
};



#endif // __weltbaut_h

