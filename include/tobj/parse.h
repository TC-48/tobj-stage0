#pragma once

#include <tobj/format.h>
#include <tc48/mem.h>
#include <stdbool.h>

bool tobj_parse_header(const tc48_memory* mem, tc48_word addr, tobj_header* out);
bool tobj_parse_section(const tc48_memory* mem, tc48_word addr, tobj_header_section* out);
bool tobj_parse_symbol(const tc48_memory* mem, tc48_word addr, tobj_header_symbol* out);
bool tobj_parse_reloc(const tc48_memory* mem, tc48_word addr, tobj_reloc* out);
