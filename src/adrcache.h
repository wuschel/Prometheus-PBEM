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


const int x_koords[] =    {  0,  -1,   0,   0,   1,  -1,  -1,   1,   1,  -2, 
                       0,   0,   2,  -2,  -2,  -1,  -1,   1,   1,   2, 
                       2,  -3,  -2,  -2,   0,   0,   2,   2,   3,  -3, 
                      -3,  -1,  -1,   1,   1,   3,   3,  -4,  -3,  -3, 
                      -2,  -2,   0,   0,   2,   2,   3,   3,   4,  -4, 
                      -4,  -3,  -3,  -1,  -1,   1,   1,   3,   3,   4, 
                       4,  -5,  -4,  -4,  -2,  -2,   0,   0,   2,   2, 
                       4,   4,   5,  -5,  -5,  -4,  -4,  -3,  -3,  -1, 
                      -1,   1,   1,   3,   3,   4,   4,   5,   5,  -6, 
                      -5,  -5,  -4,  -4,  -2,  -2,   0,   0,   2,   2, 
                       4,   4,   5,   5,   6,  -6,  -6,  -5,  -5,  -3, 
                      -3,  -1,  -1,   1,   1,   3,   3,   5,   5,   6, 
                       6,  -7,  -6,  -6,  -5,  -5,  -4,  -4,  -2,  -2, 
                       0,   0,   2,   2,   4,   4,   5,   5,   6,   6, 
                       7,  -7,  -7,  -6,  -6,  -5,  -5,  -3,  -3,  -1, 
                      -1,   1,   1,   3,   3,   5,   5,   6,   6,   7, 
                       7,  -8,  -7,  -7,  -6,  -6,  -4,  -4,  -2,  -2, 
                       0,   0,   2,   2,   4,   4,   6,   6,   7,   7, 
                       8};

const int y_koords[] =    {  0,   0,  -1,   1,   0,  -1,   1,  -1,   1,   0, 
                      -2,   2,   0,  -1,   1,  -2,   2,  -2,   2,  -1, 
                       1,   0,  -2,   2,  -3,   3,  -2,   2,   0,  -1, 
                       1,  -3,   3,  -3,   3,  -1,   1,   0,  -2,   2, 
                      -3,   3,  -4,   4,  -3,   3,  -2,   2,   0,  -1, 
                       1,  -3,   3,  -4,   4,  -4,   4,  -3,   3,  -1, 
                       1,   0,  -2,   2,  -4,   4,  -5,   5,  -4,   4, 
                      -2,   2,   0,  -1,   1,  -3,   3,  -4,   4,  -5, 
                       5,  -5,   5,  -4,   4,  -3,   3,  -1,   1,   0, 
                      -2,   2,  -4,   4,  -5,   5,  -6,   6,  -5,   5, 
                      -4,   4,  -2,   2,   0,  -1,   1,  -3,   3,  -5, 
                       5,  -6,   6,  -6,   6,  -5,   5,  -3,   3,  -1, 
                       1,   0,  -2,   2,  -4,   4,  -5,   5,  -6,   6, 
                      -7,   7,  -6,   6,  -5,   5,  -4,   4,  -2,   2, 
                       0,  -1,   1,  -3,   3,  -5,   5,  -6,   6,  -7, 
                       7,  -7,   7,  -6,   6,  -5,   5,  -3,   3,  -1, 
                       1,   0,  -2,   2,  -4,   4,  -6,   6,  -7,   7, 
                      -8,   8,  -7,   7,  -6,   6,  -4,   4,  -2,   2, 
                       0};

const int sectionlist[] = {  0,   0,   4,   8,  12,  20,  28,  36,  48,  60, 
                       72,  88, 104, 120, 140, 160, 180};

const int number_of_sections = 17;

