#pragma once

#include <tc48/word.h>

#define TOBJ_FORMAT_VERSION ((tc48_tryte)1)

/// 'TOBJ' encoded in TSCS
#define TOBJ_MAGIC                \
    TC48_HALF(                    \
        0,1,0,0,0,1, 0,0,2,2,1,2, \
        0,0,2,1,0,1, 0,0,2,2,0,0  \
    )

/// QQQQ'QQQQ in septemvigesimal
#define TOBJ_SECTION_UNDEF        \
    TC48_HALF(                    \
        2,2,2,2,2,2, 2,2,2,2,2,2, \
        2,2,2,2,2,2, 2,2,2,2,2,2  \
    )

typedef struct tobj_str {
    tc48_half  len;     ///< Length of the string  in trytes
    tc48_tryte chars[]; ///< Actual string data (convention: encoded in TSCS)
} tobj_str;

// NOTE: represented as a tryte
typedef enum tobj_binding {
    TOBJ_BINDING_LOCAL,  ///< Local scope binding (not visible outside the object)
    TOBJ_BINDING_GLOBAL, ///< Global scope binding (visible to other objects)
    TOBJ_BINDING_WEAK,   ///< Weak global binding (can be overridden by strong symbols)
} tobj_binding;

typedef struct tobj_header_section {
    tc48_half name_off;      ///< Offset into the string table for the section's name

    tc48_half off;           ///< Offset to the section data within the object
    tc48_half size;          ///< Size of the section data in trytes
    tc48_half align;         ///< Alignment of the section data

    tc48_half sym_start_idx; ///< Starting index in the symbol table for symbols in this section
    tc48_half sym_count;     ///< Number of symbols associated with this section
} tobj_header_section;

typedef struct tobj_header_symbol {
    tc48_half  name_off;     ///< Offset into the string table for the symbol's name
    tc48_half  section_idx;  ///< Index of the section this symbol belongs to

    tc48_half  off;          ///< Offset in trytes of the symbol within its section
    tc48_half  size;         ///< Size of the symbol's data

    tc48_tryte binding;      ///< Symbol binding type; maps to @ref tobj_binding
} tobj_header_symbol;

// NOTE: represented as a tryte
typedef enum tobj_reloc_type {
    TOBJ_RELOC_ABS, ///< absolute address
    TOBJ_RELOC_REL, ///< relative to RIP
} tobj_reloc_type;

typedef struct tobj_reloc {
    tc48_half  section_idx; ///< Section index within the relocation needs to be aplied
    tc48_half  offset;      ///< Offset within the section; see @ref section_idx
    tc48_half  sym_idx;     ///< Index in the symbol table that this relocation references
    tc48_tryte type;        ///< Type of relocation to apply; maps to @ref tobj_reloc_type
} tobj_reloc;

typedef struct tobj_header {
    tc48_half  magic;              ///< Magic number identifying the file format (TOBJ_MAGIC)
    tc48_tryte version;            ///< Version of the object file standard

    tc48_half  string_count;       ///< Total number of strings in the string table
    tc48_half  string_table_off;   ///< File offset to the start of the string table

    tc48_half  section_count;      ///< Total number of sections in the section table
    tc48_half  sections_table_off; ///< File offset to the start of the sections table

    tc48_half  symbol_count;       ///< Total number of symbols in the symbol table
    tc48_half  symbol_table_off;   ///< File offset to the start of the symbol table

    tc48_half  reloc_count;        ///< Total number of relocations in the relocation table
    tc48_half  reloc_table_off;    ///< File offset to the start of the relocation table

    tc48_tryte data[];             ///< Flexible array member containing the actual raw data payload
} tobj_header;
