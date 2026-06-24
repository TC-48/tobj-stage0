#include <tobj/merge.h>
#include <tobj/parse.h>
#include <tobj/builder/bldr.h>

#include <stdlib.h>
#include <string.h>

typedef struct merge_state {
    tobj_builder* bldr;
    tobj_param obj;
    const tobj_header* header;
    tc48_usize* section_map;
    tc48_half* section_offsets;
    tc48_usize* symbol_map;
} merge_state;

static tc48_usize find_or_add_string(tobj_builder* bldr, const tc48_tryte* chars, tc48_half len) {
    for (tc48_usize i = 0; i < VECTOR_SIZE(&bldr->strings); ++i) {
        tobj_bldr_string* s = &bldr->strings.begin[i];
        if (s->len == len && memcmp(s->chars, chars, len * sizeof(tc48_tryte)) == 0) {
            return i;
        }
    }
    return tobj_bldr_add_string(bldr, chars, len);
}

static void
    merge_sections(merge_state* state, const tobj_header_section* input_sections),
    merge_symbols (merge_state* state, const tobj_header_symbol* input_sections),
    merge_relocs  (merge_state* state, const tobj_reloc* input_sections);

tc48_memory* tobj_merge(const tobj_param* objects, tc48_half count) {
    tobj_builder bldr;
    tobj_bldr_init(&bldr);

    for (tc48_half k = 0; k < count; ++k) {
        tobj_param obj = objects[k];

        tobj_header header;
        if (!tobj_param_header(obj, &header)) {
            tobj_bldr_free(&bldr);
            return NULL;
        }

        tobj_header_section* input_sections = malloc(sizeof(tobj_header_section) * header.section_count);
        tobj_header_symbol* input_symbols   = malloc(sizeof(tobj_header_symbol) * header.symbol_count);
        tobj_reloc* input_relocs            = malloc(sizeof(tobj_reloc) * header.reloc_count);

#define cleanup (void)(free(input_sections), free(input_symbols), free(input_relocs))
#define fcleanup (void)(cleanup, tobj_bldr_free(&bldr))

        if (
            (header.section_count > 0 && input_sections == NULL) ||
            (header.symbol_count > 0 && input_symbols == NULL) ||
            (header.reloc_count > 0 && input_relocs == NULL)
        ) return fcleanup, NULL;

        // collect all data to the arrays
        for (tc48_half i = 0; i < header.section_count; ++i) {
            if (!tobj_parse_section(obj.data, obj.off + header.section_table_off + i * TOBJ_SECTION_SIZE_TRYTES, &input_sections[i])) {
                 return fcleanup, NULL;
            }
        }
        for (tc48_half i = 0; i < header.symbol_count; ++i) {
            if (!tobj_parse_symbol(obj.data, obj.off + header.symbol_table_off + i * TOBJ_SYMBOL_SIZE_TRYTES, &input_symbols[i])) {
                return fcleanup, NULL;
            }
        }
        for (tc48_half i = 0; i < header.reloc_count; ++i) {
            if (!tobj_parse_reloc(obj.data, obj.off + header.reloc_table_off + i * TOBJ_RELOC_SIZE_TRYTES, &input_relocs[i])) {
                return fcleanup, NULL;
            }
        }

        tc48_usize* section_map = malloc(sizeof(tc48_usize) * header.section_count);
        tc48_half* section_offsets = malloc(sizeof(tc48_half) * header.section_count);
        tc48_usize* symbol_map = malloc(sizeof(tc48_usize) * header.symbol_count);

        if (
            (header.section_count > 0 && (section_map == NULL || section_offsets == NULL)) ||
            (header.symbol_count > 0 && symbol_map == NULL)
        ) {
            free(section_map);
            free(section_offsets);
            free(symbol_map);
            return fcleanup, NULL;
        }

        merge_state state = {
            .bldr = &bldr,
            .obj = obj,
            .header = &header,
            .section_map = section_map,
            .section_offsets = section_offsets,
            .symbol_map = symbol_map,
        };

        merge_sections(&state, input_sections);
        merge_symbols(&state, input_symbols);
        merge_relocs(&state, input_relocs);

        free(section_map);
        free(section_offsets);
        free(symbol_map);
        cleanup;
    }

    tc48_memory* merged = tobj_bldr_build(&bldr, NULL);
    tobj_bldr_free(&bldr);
    return merged;
}

static inline tc48_tryte* read_section_name(
    merge_state* state, const tobj_header_section* sec, tc48_half* out_len
) {
    tc48_word string_table_pos = state->obj.off + state->header->string_table_off + sec->name_off;
    tc48_half name_len = tc48_mem_load24(state->obj.data, string_table_pos);
    tc48_tryte* name_chars = malloc(name_len * sizeof(tc48_tryte));
    for (tc48_half j = 0; j < name_len; ++j) {
        name_chars[j] = tc48_mem_load6(state->obj.data,
                                       string_table_pos + TC48_HALF_TRYTES + j);
    }
    *out_len = name_len;
    return name_chars;
}

static void process_section(
    merge_state* state, const tobj_header_section* input_sec,
    const tc48_tryte* name, tc48_half name_len,
    tc48_usize* out_idx, tc48_half* out_offset
) {
    tobj_builder* bldr = state->bldr;

    int found_idx = -1;
    for (tc48_usize s = 0; s < VECTOR_SIZE(&bldr->sections); ++s) {
        tobj_bldr_section* bsec = &bldr->sections.begin[s];
        tobj_bldr_string* bstr = &bldr->strings.begin[bsec->name_idx];
        if (bstr->len == name_len &&
            memcmp(bstr->chars, name, name_len * sizeof(tc48_tryte)) == 0) {
            found_idx = (int)s;
            break;
        }
    }

    tobj_bldr_section* sec;
    bool is_new = (found_idx == -1);

    if (is_new) {
        tobj_bldr_section new_sec;
        new_sec.name_idx = find_or_add_string(bldr, name, name_len);
        new_sec.align = input_sec->align;
        tobj_trytes_init(&new_sec.data, input_sec->size);
        for (tc48_half j = 0; j < input_sec->size; ++j) {
            tc48_tryte val = tc48_mem_load6(
                state->obj.data, state->obj.off + input_sec->off + j);
            tobj_trytes_push(&new_sec.data, val);
        }
        tc48_usize idx = tobj_bldr_add_section(bldr, &new_sec);
        tobj_trytes_free(&new_sec.data);

        sec = &bldr->sections.begin[idx];
        *out_idx = idx;
        *out_offset = 0;
    } else {
        sec = &bldr->sections.begin[found_idx];

        tc48_usize cur_size = VECTOR_SIZE(&sec->data);
        tc48_usize start_offset = cur_size;

        tc48_half align = input_sec->align;
        if (align > 1) {
            tc48_usize rem = cur_size % align;
            if (rem != 0) {
                tc48_usize padding = align - rem;
                for (tc48_usize p = 0; p < padding; ++p) {
                    tobj_trytes_push(&sec->data, 0);
                }
                start_offset += padding;
            }
        }

        for (tc48_half j = 0; j < input_sec->size; ++j) {
            tc48_tryte val = tc48_mem_load6(
                state->obj.data, state->obj.off + input_sec->off + j);
            tobj_trytes_push(&sec->data, val);
        }

        if (align > sec->align) {
            sec->align = align;
        }

        *out_idx = found_idx;
        *out_offset = (tc48_half)start_offset;
    }
}

static void merge_sections(merge_state* state, const tobj_header_section* input_sections) {
    for (tc48_half i = 0; i < state->header->section_count; ++i) {
        tc48_half name_len;
        tc48_tryte* name_chars = read_section_name(state, &input_sections[i], &name_len);

        tc48_usize idx;
        tc48_half offset;
        process_section(state, &input_sections[i], name_chars, name_len, &idx, &offset);

        state->section_map[i] = idx;
        state->section_offsets[i] = offset;

        free(name_chars);
    }
}

static void merge_symbols(merge_state* state, const tobj_header_symbol* input_symbols) {
    for (tc48_half i = 0; i < state->header->symbol_count; ++i) {
        tc48_word string_table_pos = state->obj.off + state->header->string_table_off + input_symbols[i].name_off;
        tc48_half name_len = tc48_mem_load24(state->obj.data, string_table_pos);
        tc48_tryte* name_chars = malloc(name_len * sizeof(tc48_tryte));
        for (tc48_half j = 0; j < name_len; ++j) {
            name_chars[j] = tc48_mem_load6(state->obj.data, string_table_pos + TC48_HALF_TRYTES + j);
        }

        int found_sym_idx = -1;
        for (tc48_usize s = 0; s < VECTOR_SIZE(&state->bldr->symbols); ++s) {
            tobj_bldr_symbol* bsym = &state->bldr->symbols.begin[s];
            tobj_bldr_string* bstr = &state->bldr->strings.begin[bsym->name_idx];
            if (bstr->len == name_len && memcmp(bstr->chars, name_chars, name_len * sizeof(tc48_tryte)) == 0) {
                found_sym_idx = (int)s;
                break;
            }
        }

        const tc48_usize mapped_sec_idx =
            input_symbols[i].section_idx == TOBJ_SECTION_UNDEF
                ? TOBJ_SECTION_UNDEF
                : state->section_map[input_symbols[i].section_idx];

        const tc48_half mapped_offset =
            input_symbols[i].section_idx == TOBJ_SECTION_UNDEF
                ? input_symbols[i].off
                : (input_symbols[i].off + state->section_offsets[input_symbols[i].section_idx]);

        if (found_sym_idx == -1) {
            tobj_bldr_symbol bsym;
            bsym.name_idx = find_or_add_string(state->bldr, name_chars, name_len);
            bsym.section_idx = mapped_sec_idx;
            bsym.offset = mapped_offset;
            bsym.size = input_symbols[i].size;
            bsym.binding = input_symbols[i].binding;
            tc48_usize new_sym_idx = tobj_bldr_add_symbol(state->bldr, &bsym);
            state->symbol_map[i] = new_sym_idx;
        } else {
            tobj_bldr_symbol* bsym = &state->bldr->symbols.begin[found_sym_idx];
            if (bsym->section_idx == TOBJ_SECTION_UNDEF) {
                if (mapped_sec_idx != TOBJ_SECTION_UNDEF) {
                    bsym->section_idx = mapped_sec_idx;
                    bsym->offset = mapped_offset;
                    bsym->size = input_symbols[i].size;
                    bsym->binding = input_symbols[i].binding;
                }
            } else if (mapped_sec_idx != TOBJ_SECTION_UNDEF) {
                if (bsym->binding == TOBJ_BINDING_WEAK && input_symbols[i].binding == TOBJ_BINDING_GLOBAL) {
                    bsym->section_idx = mapped_sec_idx;
                    bsym->offset = mapped_offset;
                    bsym->size = input_symbols[i].size;
                    bsym->binding = input_symbols[i].binding;
                }
            }
            state->symbol_map[i] = found_sym_idx;
        }
        free(name_chars);
    }
}

static void merge_relocs(merge_state* state, const tobj_reloc* input_relocs) {
    for (tc48_half i = 0; i < state->header->reloc_count; ++i) {
        tobj_bldr_reloc breloc;
        breloc.section_idx = state->section_map[input_relocs[i].section_idx];
        breloc.offset = input_relocs[i].offset + state->section_offsets[input_relocs[i].section_idx];
        breloc.symbol_idx = state->symbol_map[input_relocs[i].sym_idx];
        breloc.type = input_relocs[i].type;
        tobj_bldr_add_reloc(state->bldr, breloc);
    }
}
