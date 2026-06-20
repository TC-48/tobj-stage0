#include <tobj/builder/defs.h>

typedef struct tobj_builder {
    tobj_bldr_strings  strings;
    tobj_bldr_sections sections;
    tobj_bldr_symbols  symbols;
    tobj_bldr_relocs   relocs;
} tobj_builder;

/// Initialize a builder.
void tobj_bldr_init(tobj_builder* bldr);

/// Free all memory owned by the builder.
void tobj_bldr_free(tobj_builder* bldr);

/// Add a string to the string pool.
/// The builder takes a deep copy of the string.
/// Returns the string index.
size_t tobj_bldr_add_string(tobj_builder* bldr, const tc48_tryte* str, tc48_half size);

/// Add a section.
/// The builder takes a deep copy of the section data.
/// Returns the section index.
size_t tobj_bldr_add_section(tobj_builder* bldr, const tobj_bldr_section* section);

/// Add a symbol.
/// Returns the symbol index.
size_t tobj_bldr_add_symbol(tobj_builder* bldr, const tobj_bldr_symbol* sym);

/// Add a relocation.
void tobj_bldr_add_reloc(tobj_builder* bldr, tobj_bldr_reloc reloc);

