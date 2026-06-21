#include <tobj/builder/bldr.h>

#include <vector/vector.h>
#include <tc48/mem.h>

#include <stdbool.h>
#include <string.h>

static inline void push_tryte(tobj_trytes* out, tc48_tryte t) {
    tobj_trytes_push(out, t);
}
static inline void push_quarter(tobj_trytes* out, tc48_quarter q) {
    push_tryte(out, (tc48_tryte)(q / TC48_TRYTE_VALUES));
    push_tryte(out, (tc48_tryte)(q % TC48_TRYTE_VALUES));
}
static inline void push_half(tobj_trytes* out, tc48_half h) {
    push_quarter(out, (tc48_quarter)(h / TC48_QUARTER_VALUES));
    push_quarter(out, (tc48_quarter)(h % TC48_QUARTER_VALUES));
}
static inline void push_word(tobj_trytes* out, tc48_word w) {
    push_half(out, (tc48_half)(w / TC48_HALF_VALUES));
    push_half(out, (tc48_half)(w % TC48_HALF_VALUES));
}

static void push_header(tobj_trytes* out, const tobj_header* header) {
    push_half (out, header->magic);
    push_tryte(out, header->version);

    push_half(out, header->string_count);
    push_half(out, header->string_table_off);

    push_half(out, header->section_count);
    push_half(out, header->sections_table_off);

    push_half(out, header->symbol_count);
    push_half(out, header->symbol_table_off);

    push_half(out, header->reloc_count);
    push_half(out, header->reloc_table_off);
}

typedef struct {
    tc48_half data_off;
    tc48_half sym_start;
    tc48_half sym_count;
} section_info;

static bool make_sorted_map(tobj_builder* bldr, section_info* secs, size_t** out_map) {
    size_t num_syms = VECTOR_SIZE(&bldr->symbols);
    size_t num_secs = VECTOR_SIZE(&bldr->sections);

    for (size_t i = 0; i < num_secs; ++i) secs[i] = (section_info) {0};
    for (size_t i = 0; i < num_syms; ++i) secs[bldr->symbols.begin[i].section_idx].sym_count++;

    tc48_half* map_pos = malloc(sizeof(tc48_half) * num_secs);
    size_t* map = malloc(sizeof(size_t) * num_syms);
    if ((num_secs > 0 && !map_pos) || (num_syms > 0 && !map)) {
        free(map_pos); free(map); return false;
    }

    tc48_half offset = 0;
    for (size_t i = 0; i < num_secs; ++i) {
        map_pos[i] = secs[i].sym_start = offset;
        offset += secs[i].sym_count;
    }
    for (size_t i = 0; i < num_syms; ++i)
        map[map_pos[bldr->symbols.begin[i].section_idx]++] = i;

    free(map_pos);
    *out_map = map;
    return true;
}

bool the_actual_stuff(tobj_builder* bldr, tobj_trytes* out, tobj_header* out_header) {
    size_t num_strings = VECTOR_SIZE(&bldr->strings), num_sections = VECTOR_SIZE(&bldr->sections),
           num_symbols = VECTOR_SIZE(&bldr->symbols), num_relocs = VECTOR_SIZE(&bldr->relocs);

    tc48_half* string_offsets   = malloc(sizeof(tc48_half) * num_strings);
    section_info* section_infos = malloc(sizeof(section_info) * num_sections);

    tc48_half string_table_size = 0;
    for (size_t i = 0; i < num_strings; ++i) {
        string_offsets[i] = string_table_size;
    
        string_table_size += TC48_HALF_TRYTES; // length
        string_table_size += bldr->strings.begin[i].len;
    }

    size_t* sorted_indices = NULL;
    if (!make_sorted_map(bldr, section_infos, &sorted_indices)) {
        free(string_offsets); free(section_infos); return false;
    }

    tc48_half off = TOBJ_HEADER_SIZE_TRYTES;
    
    tc48_half string_table_off = off;
    off += string_table_size;
    
    tc48_half sections_table_off = off;
    off += num_sections * TOBJ_SECTION_SIZE_TRYTES;
    
    tc48_half symbol_table_off = off;
    off += num_symbols * TOBJ_SYMBOL_SIZE_TRYTES;
    
    tc48_half reloc_table_off = off;
    off += num_relocs * TOBJ_RELOC_SIZE_TRYTES;

    for (size_t i = 0; i < num_sections; ++i) {
        tc48_half align = bldr->sections.begin[i].align;
        if (align > 1) {
            tc48_half rem = off % align;
            if (rem != 0) {
                off += (align - rem);
            }
        }
        section_infos[i].data_off = off;
        off += VECTOR_SIZE(&bldr->sections.begin[i].data);
    }

    tobj_header header = {
        .magic              = TOBJ_MAGIC,
        .version            = TOBJ_FORMAT_VERSION,
        .string_count       = (tc48_half)num_strings,
        .string_table_off   = string_table_off,
        .section_count      = (tc48_half)num_sections,
        .sections_table_off = sections_table_off,
        .symbol_count       = (tc48_half)num_symbols,
        .symbol_table_off   = symbol_table_off,
        .reloc_count        = (tc48_half)num_relocs,
        .reloc_table_off    = reloc_table_off,
    };

    push_header(out, &header);
    if (out_header) {
        *out_header = header;
    }

    for (size_t i = 0; i < num_strings; ++i) {
        push_half(out, bldr->strings.begin[i].len);
        for (tc48_half j = 0; j < bldr->strings.begin[i].len; ++j) {
            push_tryte(out, bldr->strings.begin[i].chars[j]);
        }
    }

    for (size_t i = 0; i < num_sections; ++i) {
        push_half(out, string_offsets[bldr->sections.begin[i].name_idx]);
        push_half(out, section_infos[i].data_off);
        push_half(out, VECTOR_SIZE(&bldr->sections.begin[i].data));
        push_half(out, bldr->sections.begin[i].align);
        push_half(out, section_infos[i].sym_start);
        push_half(out, section_infos[i].sym_count);
    }

    for (size_t i = 0; i < num_symbols; ++i) {
        size_t orig_idx = sorted_indices[i];
        push_half(out, string_offsets[bldr->symbols.begin[orig_idx].name_idx]);
        push_half(out, bldr->symbols.begin[orig_idx].section_idx);
        push_half(out, bldr->symbols.begin[orig_idx].offset);
        push_half(out, bldr->symbols.begin[orig_idx].size);
        push_tryte(out, (tc48_tryte)bldr->symbols.begin[orig_idx].binding);
    }

    for (size_t i = 0; i < num_relocs; ++i) {
        push_half(out, bldr->relocs.begin[i].section_idx);
        push_half(out, bldr->relocs.begin[i].offset);
        push_half(out, bldr->relocs.begin[i].symbol_idx);
        push_tryte(out, (tc48_tryte)bldr->relocs.begin[i].type);
    }

    for (size_t i = 0; i < num_sections; ++i) {
        while ((tc48_half)VECTOR_SIZE(out) < section_infos[i].data_off) {
            push_tryte(out, 0);
        }
        for (tc48_half j = 0; j < VECTOR_SIZE(&bldr->sections.begin[i].data); ++j) {
            push_tryte(out, bldr->sections.begin[i].data.begin[j]);
        }
    }

    free(string_offsets);
    free(section_infos);
    free(sorted_indices);
    return true;
}

tc48_memory* tobj_bldr_build(tobj_builder* bldr, tobj_header* out_header) {
    tobj_trytes out = {0};
    if (!the_actual_stuff(bldr, &out, out_header)) {
        tobj_trytes_free(&out);
        return NULL;
    }

    size_t size = VECTOR_SIZE(&out);
    tc48_memory* mem = tc48_mem_alloc(size);
    memcpy(mem->data, out.begin, size * sizeof(tc48_tryte));

    tobj_trytes_free(&out);
    return mem;
}
