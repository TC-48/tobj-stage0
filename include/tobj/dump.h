#include <tobj/format.h>

#include <stdbool.h>
#include <stdio.h>

const char* tobj_binding_to_str(tobj_binding binding, bool unknown);
const char* tobj_reloc_type_to_str(tobj_reloc_type type, bool unknown);

void tobj_dump_header(const tobj_header* header, const char* prefix, FILE* out);
