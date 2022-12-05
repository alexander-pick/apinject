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

#include "elf.h"

/*
    finds the location of a given symbol in target binary and returns
    the offset
*/
unsigned long get_offset_from_elf(char *filename, char *symbol_needle)
{

    FILE *fp;
    Elf64_Sym symbol;

    Elf64_Ehdr elf_hdr;

    unsigned long return_val = 0;

    int string_tbl_cnt = 0;

    // we can handle up to 4 string tables should work for most binaries
    // as there are usally 3 (.symtab, .strtab, .shstrtab)

    Elf64_Shdr string_tbl[4];
    Elf64_Shdr dyn_sym_tbl;
    Elf64_Shdr sym_tbl;

    bool found_sym_tbl = false;

    fp = fopen(filename, "r");

    if (!fread(&elf_hdr, sizeof(elf_hdr), 1, fp))
    {
        print_line("error reading elf_hdr from file", RED);
        exit(1);
    }

    if (elf_hdr.e_ident[EI_CLASS] != ELFCLASS64)
    {
        print_line("only 64-bit files are supported for now", RED);
        exit(0);
    }

    print_line("Entrypoint: %x", GRN, elf_hdr.e_entry);
    print_line("Program header table file offset: %x", GRN, elf_hdr.e_phoff);
    print_line("Section header table file offset: %x", GRN, elf_hdr.e_shnum);

    for (int i = 0; i < elf_hdr.e_shnum; i++)
    {

        Elf64_Shdr sect_hdr;

        unsigned long offset = elf_hdr.e_shoff + i * elf_hdr.e_shentsize;

        // seek section header and read it
        fseek(fp, offset, SEEK_SET);
        if (!fread(&sect_hdr, sizeof(Elf64_Shdr), 1, fp))
        {
            print_line("error reading sect_hdr from file", RED);
            exit(1);
        }

        if (sect_hdr.sh_type == SHT_STRTAB)
        {
            print_line("found string table at %p, sized %d", BLU, offset, sect_hdr.sh_size);

            string_tbl[string_tbl_cnt++] = sect_hdr;
        }

        if (sect_hdr.sh_type == SHT_DYNSYM)
        {
            print_line("found dynamic symbol table at %p, sized %d", BLU, offset, sect_hdr.sh_size);

            dyn_sym_tbl = sect_hdr;
        }
        if (sect_hdr.sh_type == SHT_SYMTAB)
        {
            print_line("found symbol table at %p, sized %d", BLU, offset, sect_hdr.sh_size);
            
            sym_tbl = sect_hdr;
            found_sym_tbl = true;
        }
    }

    // we found no symbol table, let's use the dynamic one
    if(!found_sym_tbl) {
        sym_tbl = dyn_sym_tbl;
    }

    char *strtab = malloc(sym_tbl.sh_size);

    fseek(fp, sym_tbl.sh_offset, SEEK_SET);
    if (!fread(strtab, 1, sym_tbl.sh_size, fp))
    {
        print_line("error reading strtab from file (%lu)", RED, sym_tbl.sh_offset);
        exit(1);
    }

    // read the entries
    for (int j = 0; j < sym_tbl.sh_size; (++j * sizeof(Elf64_Sym)))
    {

        unsigned long current_offset = sym_tbl.sh_offset + j;

        fseek(fp, current_offset, SEEK_SET);
        if (!fread(&symbol, sizeof(Elf64_Sym), 1, fp))
        {
            print_line("error reading symbol from file (%lx)", RED, current_offset);
            exit(0);
        }

        // symbol has a name
        if (symbol.st_name != 0)
        {

            // look for the symbol in each of the string tables
            for (int k = 0; k <= string_tbl_cnt; k++)
            {
                char *symbol_name = malloc(128);

                fseek(fp, (string_tbl[k].sh_offset + symbol.st_name), SEEK_SET);

                int ret = fread(symbol_name, 128, 1, fp);
                
#if DEBUG               
                if(ret != 0) {
                    if(strcmp(symbol_name, "")) {
                        print_line("%ld: %s", YEL, k, symbol_name);
                    }    
                }
#endif

                if (strstr(symbol_name, symbol_needle) != NULL)
                {

#if 0
                    print_line("st_name\t=\t%x", BLU, symbol.st_name);
                    print_line("st_info\t=\t%d", BLU, symbol.st_info);
                    print_line("st_other\t=\t%d", BLU, symbol.st_other);
                    print_line("st_shndx\t=\t%d", BLU, symbol.st_shndx);
                    print_line("st_value\t=\t%p", BLU, symbol.st_value); // this is what we are looking for
                    print_line("st_size\t=\t%x", BLU, symbol.st_size);
#endif
                    print_line("found symbol %s, resolving to %p", BLU, symbol_name, symbol.st_value);

                    return_val = symbol.st_value;
                    fclose(fp);
                    return return_val;
                }
            }
        }
    }

    print_line("no symbol found", RED);
    fclose(fp);
    exit(1);
    // never reached
    return return_val;
}