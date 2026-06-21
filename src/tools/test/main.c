#include "tc48/mem.h"
#include <tobj/builder/bldr.h>

int main() {
    tobj_builder bldr;
    tobj_bldr_init(&bldr);

    // https://tc-48.github.io/tscs-specification/character-codes.html
    const tc48_tryte string[] = {
        /* H */ 70, /* e */ 41, /* y */ 61,
    };
    size_t name_idx =
        tobj_bldr_add_string(&bldr, string, sizeof string / sizeof(tc48_tryte));

    tobj_trytes trytes = {0};
    tobj_trytes_push(&trytes, 123);

    const tobj_bldr_section section = {
        .name_idx = name_idx,
        .data     = trytes,
        .align    = 4,
    };
    tobj_bldr_add_section(&bldr, &section);
    tobj_trytes_free(&trytes);

    tc48_memory* mem = tobj_bldr_build(&bldr, NULL);
    tc48_mem_dump(mem, 0, mem->size);
    tc48_mem_save(mem, "out.o.t48b");
    tc48_mem_free(mem);
    return 0;
}
