/*
    APInject
    Copyright (C) 2022  Alexander Pick

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "apinject.h"

//helper 
int is_numeric(char d_name[256]);
FILE *proc_open(pid_t pid);

//proc
int proc_get_pid(const char *needle, bool ourself);
void* proc_get_libc_name(pid_t pid);

unsigned long proc_get_image(pid_t pid, const char *image_name, int skip, bool end);
unsigned long proc_get_base(pid_t pid);