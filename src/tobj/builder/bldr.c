#include <tobj/builder/bldr.h>
#include <string.h>
#include <stdlib.h>

void tobj_bldr_init(tobj_builder* bldr) {
    // As far as I know from the documentation,
    // there is no function like init(),
    // and we should just zero initialize all the vectors
    memset(bldr, 0, sizeof(*bldr));
}

void tobj_bldr_free(tobj_builder* bldr) {
    for (tobj_bldr_string* s = bldr->strings.begin; s < bldr->strings.end; s++) {
        free(s->chars);
    }
    tobj_bldr_strings_free(&bldr->strings);

    for (tobj_bldr_section* s = bldr->sections.begin; s < bldr->sections.end; s++) {
        tobj_trytes_free(&s->data);
    }
    tobj_bldr_sections_free(&bldr->sections);

    tobj_bldr_symbols_free(&bldr->symbols);
    tobj_bldr_relocs_free(&bldr->relocs);
}

size_t tobj_bldr_add_string(tobj_builder* bldr, const tc48_tryte* str, tc48_half size) {
    tobj_bldr_string s;
    s.len = size;
    s.chars = malloc(size * sizeof(tc48_tryte));
    if (s.chars) {
        memcpy(s.chars, str, size * sizeof(tc48_tryte));
    }
    
    tobj_bldr_strings_push(&bldr->strings, s);
    return VECTOR_SIZE(&bldr->strings) - 1;
}

size_t tobj_bldr_add_section(tobj_builder* bldr, const tobj_bldr_section* section) {
    tobj_bldr_section new_sec = *section;
    tobj_trytes_init(&new_sec.data, VECTOR_SIZE(&section->data));
    tobj_trytes_duplicate(&new_sec.data, &section->data);
    
    tobj_bldr_sections_push(&bldr->sections, new_sec);
    return VECTOR_SIZE(&bldr->sections) - 1;
}

size_t tobj_bldr_add_symbol(tobj_builder* bldr, const tobj_bldr_symbol* sym) {
    tobj_bldr_symbols_push(&bldr->symbols, *sym);
    return  VECTOR_SIZE(&bldr->symbols) - 1;
}

void tobj_bldr_add_reloc(tobj_builder* bldr, tobj_bldr_reloc reloc) {
    tobj_bldr_relocs_push(&bldr->relocs, reloc);
}
