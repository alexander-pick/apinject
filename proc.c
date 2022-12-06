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

#include "proc.h"

/* This file contains /proc related functions */

/*
    check if dirname is digits only
*/
int is_numeric(char d_name[256])
{
    for (const char *p = d_name; *p; p++)
    {
        if (!isdigit(*p))
        {
            return 1;
        }
    }
    return 0;
}

FILE *proc_open(pid_t pid)
{

    FILE *fd;
    char map_filename[PATH_MAX];

    if (pid == -1) // this is out own process
    {
        snprintf(map_filename, sizeof(map_filename), "/proc/self/maps");
    }
    else // other proccess
    {
        snprintf(map_filename, sizeof(map_filename), "/proc/%d/maps", pid);
    }

    print_line("opening proc maps file %s", GRN, map_filename);

    if ((fd = fopen(map_filename, "r")) == NULL)
    {
        print_line("Error, could not open maps file for process %d", RED, pid);
        exit(1);
    }

    return fd;
}

int proc_get_pid(const char *needle, bool ourself)
{

    DIR *proc_dir = opendir("/proc");
    FILE *fd;
    struct dirent *proc_entry;

    char proc_file[PATH_MAX];

    char buffer[BUFSIZ];

    if (!proc_dir)
    {

        print_line("opendir failed", RED);
        exit(1);
    }
    while ((proc_entry = readdir(proc_dir)))
    {
        if (is_numeric(proc_entry->d_name))
        {
            continue;
        }

        snprintf(proc_file, sizeof(proc_file), "/proc/%s/stat", proc_entry->d_name);

        fd = fopen(proc_file, "r");

        if (!fd)
        {
            print_line("error opening %s", RED, proc_file);
            continue;
        }

        while (fgets(buffer, sizeof(buffer), fd))
        {

            if (strstr(buffer, needle))
            {

                char pid_str[BUFSIZ];
                char *pid_str_ptr = pid_str;

                for (const char *p = buffer; *p; p++)
                {

                    if (!isdigit(*p))
                    {

                        int pid = strtol(pid_str, NULL, 10);

                        // prevent finding ourself in debug monitor
                        if ((getpid() == pid) && ourself)
                        {
                            continue;
                        }

                        fclose(fd);
                        closedir(proc_dir);
                        return pid;
                    }

                    memcpy(pid_str_ptr++, p, 1);
                }
                break;
            }
        }

        fclose(fd);
    }

    closedir(proc_dir);

    return 0;
}

void *proc_get_libc_name(pid_t pid)
{

    FILE *fd = proc_open(pid);
    char line[BUFSIZ];
    char *filename = malloc(BUFSIZ);

    while (fgets(line, BUFSIZ, fd) != NULL)
    {
        sscanf(line, "%*p-%*p %*s %*s %*s %*s %s", filename);

        if (strstr(line, LIBCSTR) != NULL)
        {
            print_line("found libc at %s", BLU, filename);
            fclose(fd);
            return filename;
        }
    }
    fclose(fd);
    return &"libc.so.6";
}

/*
    find a executable image in memory using /proc

    pid = proc to open
    image_name = search for name, any if empty
    skip = skip x entries
    end = return end adress

*/

unsigned long proc_get_image(pid_t pid, const char *image_name, int skip, bool end)
{

    char line[BUFSIZ];

    void *addr, *addr_end;

    // char remaining[BUFSIZ];
    char perms[5];

    FILE *fd = proc_open(pid);

    // https://linux.die.net/man/5/proc

    while (fgets(line, BUFSIZ, fd) != NULL)
    {
        sscanf(line, "%p-%p %s %*s %*s %*d", &addr, &addr_end, perms);

        // FIXME: this is a possible bug, if the path contains this string and the 
        // page is not a x page the tool will fail, xp is better than x thought
        if (strstr(perms, "xp") != NULL)
        {
            if (skip)
            {
                skip--;
                print_line("skipping page", BLU);
                continue;
            }

            // in case we match a name
            if (image_name != "")
            {

                if (!(strstr(line, image_name) != NULL))
                {
                    continue;
                }
            }

            print_line("found %s for %d at %p - %p", BLU, image_name, pid, addr, addr_end);
            fclose(fd);

            if (!end)
            {
                return (unsigned long)addr;
            }
            else
            {
                return (unsigned long)addr_end;
            }
        }
    }

    print_line("%s: error finding page", RED, image_name);

    fclose(fd);
    exit(1);  // terminating, this is fatal
    return 0; // never reached
}

unsigned long proc_get_base(pid_t pid)
{

    char line[BUFSIZ];

    void *addr;

    FILE *fd = proc_open(pid);

    if(fgets(line, BUFSIZ, fd) != NULL)
    {
        sscanf(line, "%p-%*p %*s %*s %*s %*d", &addr);
#if DEBUG
        print_line("%s", YEL, line);
#endif
        return (unsigned long)addr;
    }     
    
    exit(1);  // terminating, this is fatal
    return 0; // never reached
}