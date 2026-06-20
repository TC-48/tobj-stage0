#pragma once

#include <tobj/format.h>
#include <tc48/word.h>

#include <vector/vector.h>

VECTOR_DECLARE(tobj_trytes, tobj_trytes, tc48_tryte);

typedef struct tobj_bldr_string {
    tc48_half   len;
    tc48_tryte* chars;
} tobj_bldr_string;

typedef struct tobj_bldr_section {
    size_t name_idx;  ///< Index into the string pool
    tc48_half align;  ///< Alignment in trytes
    tobj_trytes data; ///< Section data (Deep copied on add)
} tobj_bldr_section;

typedef struct tobj_bldr_symbol {
    size_t name_idx;    ///< Index into the string pool
    size_t section_idx; ///< Index of the section this symbol belongs to

    tc48_half offset;   ///< Offset within the section
    tc48_half size;     ///< Size of the symbol

    tobj_binding binding;
} tobj_bldr_symbol;

typedef struct tobj_bldr_reloc {
    size_t section_idx;
    tc48_half offset;
    size_t symbol_idx;
    tobj_reloc_type type;
} tobj_bldr_reloc;

VECTOR_DECLARE(tobj_bldr_strings,  tobj_bldr_strings,  tobj_bldr_string);
VECTOR_DECLARE(tobj_bldr_sections, tobj_bldr_sections, tobj_bldr_section);
VECTOR_DECLARE(tobj_bldr_symbols,  tobj_bldr_symbols,  tobj_bldr_symbol);
VECTOR_DECLARE(tobj_bldr_relocs,   tobj_bldr_relocs,   tobj_bldr_reloc);

