#include <tobj/link.h>
#include <tobj/parse.h>

#include <stdlib.h>
#include <string.h>

#define e(CODE, ...) \
    ((tobj_link_result){ .code = (CODE), .details = { __VA_ARGS__ } })

tobj_link_result tobj_to_raw_exe(const tc48_memory* object, const tobj_header* stuff, tc48_memory** out) {
    tobj_header header;
    if (stuff == NULL) {
        if (!tobj_parse_header(object, 0, &header)) {
            return e(TOBJ_LINK_BAD_HEADER);
        }
    } else header = *stuff;

    if (header.magic != TOBJ_MAGIC) {
        return e(TOBJ_LINK_BAD_MAGIC, .magic = header.magic);
    }

    tobj_header_section* sections = malloc(sizeof(tobj_header_section) * header.section_count);
    tobj_header_symbol* symbols   = malloc(sizeof(tobj_header_symbol) * header.symbol_count);
    tobj_reloc* relocs            = malloc(sizeof(tobj_reloc) * header.reloc_count);
    tc48_word* section_vaddrs     = malloc(sizeof(tc48_word) * header.section_count);

    // there is no defer in C );
    #define cleanup (void)(free(sections), free(symbols), free(relocs), free(section_vaddrs))

   // this condition will probably always be false because all modern operating systems
   // (NOTE that windows is NOT a modern operating system) implement overcommit. so if there
   // is really no memory the OOM killer will do its job. but its a "good practice" so "why not".
   if (sections == NULL || symbols == NULL || relocs == NULL || section_vaddrs == NULL) {
       cleanup; // free(NULL) is no-op
       return e(TOBJ_LINK_OUT_OF_MEM);
   }

    for (tc48_half i = 0; i < header.section_count; i++) {
        if (!tobj_parse_section(object, header.section_table_off + i * TOBJ_SECTION_SIZE_TRYTES, &sections[i])) {
            cleanup;
            return e(TOBJ_LINK_BAD_HEADER);
        }
    }

    for (tc48_half i = 0; i < header.symbol_count; i++) {
        if (!tobj_parse_symbol(object, header.symbol_table_off + i * TOBJ_SYMBOL_SIZE_TRYTES, &symbols[i])) {
            return cleanup, e(TOBJ_LINK_BAD_HEADER);
        }

        if (symbols[i].section_idx == TOBJ_SECTION_UNDEF)
            return cleanup, e(TOBJ_LINK_UNDEF_REF, .sym = i);
        if (symbols[i].section_idx >= header.section_count)
            return cleanup, e(TOBJ_LINK_BAD_SEC_IDX);
        if (symbols[i].off >= sections[symbols[i].section_idx].size)
            return cleanup, e(TOBJ_LINK_BAD_OFF);
    }

    tc48_word current_vaddr = 0;
    for (tc48_half i = 0; i < header.section_count; i++) {
        if (sections[i].align > 1) {
            tc48_word rem = current_vaddr % sections[i].align;
            if (rem != 0) current_vaddr += (sections[i].align - rem);
        }
        section_vaddrs[i] = current_vaddr;
        current_vaddr += sections[i].size;
    }

    tc48_memory* raw = tc48_mem_alloc(current_vaddr);
    if (raw == NULL)
        return cleanup, e(TOBJ_LINK_OUT_OF_MEM);

#define cleanup2 (void)(cleanup, tc48_mem_free(raw))

    for (tc48_half i = 0; i < header.section_count; i++) {
        if (sections[i].size > 0) {
            memcpy(&raw->data[section_vaddrs[i]],
                   &object->data[sections[i].off],
                   sections[i].size * sizeof(tc48_tryte));
        }
    }

    for (tc48_half i = 0; i < header.reloc_count; i++) {
        if (!tobj_parse_reloc(object, header.reloc_table_off + i * TOBJ_RELOC_SIZE_TRYTES, &relocs[i])) {
            return cleanup2, e(TOBJ_LINK_BAD_HEADER);
        }

        tobj_reloc* r = &relocs[i];
        if (r->sym_idx >= header.symbol_count)
            return cleanup2, e(TOBJ_LINK_BAD_SYM_IDX);
        if (r->section_idx >= header.section_count)
            return cleanup2, e(TOBJ_LINK_BAD_SEC_IDX);

        tobj_header_symbol* s = &symbols[r->sym_idx];
        if (r->offset >= sections[r->section_idx].size) {
            return cleanup2, e(TOBJ_LINK_BAD_OFF);
        }

        tc48_word symbol_value = section_vaddrs[s->section_idx] + s->off;
        tc48_word patch_addr = section_vaddrs[r->section_idx] + r->offset;

        if (patch_addr + TC48_WORD_TRYTES > raw->size) {
            return cleanup2, e(TOBJ_LINK_BAD_OFF);
        }

        tc48_word the_actual_stuff = r->type == TOBJ_RELOC_REL ? symbol_value - patch_addr : symbol_value;
        tc48_mem_store48(raw, patch_addr, the_actual_stuff);
    }

    cleanup;
    *out = raw;
    return e(TOBJ_LINK_SUCCESS);
}

// TODO: print symbol names instead of indexes
void tobj_print_link_error(tobj_link_result res, tobj_printf_like* fn) {
    switch (res.code) {
    case TOBJ_LINK_BAD_MAGIC:   fn("linking failed: bad magic (got %llu)", (unsigned long long)res.details.magic); break;
    case TOBJ_LINK_UNDEF_REF:   fn("linking failed: undefined symbol %llu", (unsigned long long)res.details.sym);  break;
    case TOBJ_LINK_BAD_HEADER:  fn("linking failed: bad object header");                                           break;
    case TOBJ_LINK_OUT_OF_MEM:  fn("linking failed: out of memory");                                               break;
    case TOBJ_LINK_BAD_SEC_IDX: fn("linking failed: bad section index");                                           break;
    case TOBJ_LINK_BAD_SYM_IDX: fn("linking failed: bad symbol index");                                            break;
    case TOBJ_LINK_BAD_OFF:     fn("linking failed: bad offset");                                                  break;
    default:                    fn("linking failed (code %d)", (int)res.code);                                     break;
    }
}
