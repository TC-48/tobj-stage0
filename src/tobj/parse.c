#include <tobj/parse.h>
#include <tc48/mem.h>
#include <string.h>

bool tobj_parse_header(const tc48_memory* mem, tc48_word addr, tobj_header* out) {
    if (addr >= mem->size) return false;
    if (mem->size - addr < TOBJ_HEADER_SIZE_TRYTES) return false;

    tc48_word current = addr;

    out->magic   = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->version = tc48_mem_load6 (mem, current); current += TC48_TRYTE_TRYTES;

    out->string_count      = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->string_table_off  = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;

    out->section_count     = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->section_table_off = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;

    out->symbol_count      = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->symbol_table_off  = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;

    out->reloc_count       = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->reloc_table_off   = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;

    return true;
}

bool tobj_parse_section(const tc48_memory* mem, tc48_word addr, tobj_header_section* out) {
    if (addr >= mem->size) return false;
    if (mem->size - addr < TOBJ_SECTION_SIZE_TRYTES) return false;

    tc48_word current = addr;

    out->name_off      = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->off           = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->size          = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->align         = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->sym_start_idx = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->sym_count     = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;

    return true;
}

bool tobj_parse_symbol(const tc48_memory* mem, tc48_word addr, tobj_header_symbol* out) {
    if (addr >= mem->size) return false;
    if (mem->size - addr < TOBJ_SYMBOL_SIZE_TRYTES) return false;

    tc48_word current = addr;

    out->name_off    = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->section_idx = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->off         = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->size        = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->binding     = tc48_mem_load6 (mem, current); current += TC48_TRYTE_TRYTES;

    return true;
}

bool tobj_parse_reloc(const tc48_memory* mem, tc48_word addr, tobj_reloc* out) {
    if (addr >= mem->size) return false;
    if (mem->size - addr < TOBJ_RELOC_SIZE_TRYTES) return false;

    tc48_word current = addr;

    out->section_idx = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->offset      = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->sym_idx     = tc48_mem_load24(mem, current); current += TC48_HALF_TRYTES;
    out->type        = tc48_mem_load6 (mem, current); current += TC48_TRYTE_TRYTES;

    return true;
}
