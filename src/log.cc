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


#define __log_c

#include <stdio.h>
#include <time.h>
#include <string.h>

#include "log.h"
#include "alg.h" // verzeichnis_gewaehrleisten()

char *loglevel;
char *logfilename;

bool loglevel_enabled(char level)
{
    if (strlen(loglevel) == 0) return true;
    
    char *scan = loglevel;
    while (*scan && *scan != level) scan++;
    return (*scan != 0);
}


void log(char level, const char *formatstring, ...)
{
    if (loglevel_enabled(level))
    {
	verzeichnis_gewaehrleisten(logfilename);
	FILE *logfile = fopen(logfilename, "a");
	if (logfile) {
	    // Zeitstempel
	    time_t zeit = time(0);
	    struct tm *stm = localtime(&zeit);
	    char zeitstring[512];
	    strftime(zeitstring, 511, "%a %Y-%m-%d %H:%M:%S", stm);
	    fprintf(logfile, "%s ", zeitstring);

	    // Loglevel
	    fprintf(logfile, "[%c] ", level);

	    // Meldung selbst
	    va_list valist;
	    va_start(valist, formatstring);
	    vfprintf(logfile, formatstring, valist);
	    va_end(valist);
	    fprintf(logfile, ".\n");
	    fclose(logfile);
	}
    }
}
