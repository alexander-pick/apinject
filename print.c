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

#include "print.h"

/*
    print a line formated with coloring
*/

void print_line(char *message, char *color, ...)
{
    char buffer[MSG_BUF_LEN];
    va_list args;
    va_start(args, color);

    vsnprintf(buffer, MSG_BUF_LEN, message, args);

    printf("%s[i] %s\n" RESET, color, buffer);

    va_end(args);
}

/*
    print buffer in a readable manner
*/

unsigned char *debug_data; // global to keep debug info across call

void print_buffer(unsigned char *buf, long len, bool match_old)
{
    for (int i = 0; i < len; i++)
    {
        if (
            (debug_data != NULL) &&
            (buf[i] != debug_data[i]) &&
            match_old)
        {
            printf(YEL "%02x " RESET, buf[i]);
        }
        else
        {
            printf("%02x ", buf[i]);
        }
        if (!((i + 1) % 32))
        {
            printf("\n");
        }
    }
}

/*
    debug helper to read and print target memory
*/

void peek_debug(pid_t pid, unsigned long addr, int len)
{
    unsigned char *curr_data = (unsigned char *)malloc(len);

    peek(pid, addr, curr_data, len);

    print_line("peeking at %p (mod16:%d)", BLU, addr, addr % 16);
    print_buffer(curr_data, len, true);

    // preserve buffer for matching
    debug_data = curr_data;
}

void print_info()
{
    printf(YEL "\nAPInject %s\n\n" RESET, MY_VERSION);
}