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

bool the_actual_stuff(tobj_builder* bldr, tobj_trytes* out, tobj_header* out_header) {
    push_half (out, TOBJ_MAGIC);
    push_tryte(out, TOBJ_FORMAT_VERSION);
    // TODO: everything.
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
