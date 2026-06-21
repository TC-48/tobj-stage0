#include <tobj/dump.h>

const char* tobj_binding_to_str(tobj_binding binding, bool unknown) {
    switch (binding) {
    case TOBJ_BINDING_LOCAL:  return "local";
    case TOBJ_BINDING_GLOBAL: return "global";
    case TOBJ_BINDING_WEAK:   return "weak";
    default:                  return unknown ? "unknown" : NULL;
    }
}

const char* tobj_reloc_type_to_str(tobj_reloc_type type, bool unknown) {
    switch (type) {
    case TOBJ_RELOC_ABS: return "abs";
    case TOBJ_RELOC_REL: return "rel";
    default:             return unknown ? "unknown" : NULL;
    }
}

void tobj_dump_header(const tobj_header* header, const char* prefix, FILE* out) {
    fprintf(out, "%smagic:             %s\n", prefix, header->magic == TOBJ_MAGIC ? "TOBJ" : "invalid");
    fprintf(out, "%sversion:           %lu\n", prefix, (unsigned long)header->version);
    fprintf(out, "%sstring-count:      %lu\n", prefix, (unsigned long)header->string_count);
    fprintf(out, "%sstring-table-off:  %lu\n", prefix, (unsigned long)header->string_table_off);
    fprintf(out, "%ssection-count:     %lu\n", prefix, (unsigned long)header->section_count);
    fprintf(out, "%ssection-table-off: %lu\n", prefix, (unsigned long)header->section_table_off);
    fprintf(out, "%ssymbol-count:      %lu\n", prefix, (unsigned long)header->symbol_count);
    fprintf(out, "%ssymbol-table-off:  %lu\n", prefix, (unsigned long)header->symbol_table_off);
    fprintf(out, "%sreloc-count:       %lu\n", prefix, (unsigned long)header->reloc_count);
    fprintf(out, "%sreloc-table-off:   %lu\n", prefix, (unsigned long)header->reloc_table_off);
}
