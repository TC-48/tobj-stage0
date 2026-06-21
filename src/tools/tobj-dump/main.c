#include <argparse/argparse.h>
#include <tobj/gen/tscs.h>

#include <tobj/format.h>
#include <tobj/parse.h>
#include <tobj/dump.h>

#include <tc48/util.h>
#include <tc48/mem.h>
#include <toolcommon/output.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static const char *const usages[] = {
    "tobj-dump [options...] file.o.t48b",
    "tobj-dump file.o.t48b",
    NULL,
};

// TODO: maybe we should convert to unicode,
//       but nobody uses non-ascii characters
//       in symbol names anyway so who cares
static char tscs_to_ascii(tc48_tryte ch) {
    for (int c = 0; c < 128; ++c) {
        if (tscs_from_ascii[c] == (int16_t)ch)
            return (char)c;
    }
    return '?';
}

static void print_tscs_string(const tc48_memory* mem, tc48_word off) {
    if (off >= mem->size) {
        tool_puts("<invalid>");
        return;
    }

    tc48_half len = tc48_mem_load24(mem, off);
    tc48_word pos = off + TC48_HALF_TRYTES;

    tool_putc('"');
    for (tc48_half i = 0; i < len; ++i) {
        tool_putc(tscs_to_ascii(tc48_mem_load6(mem, pos + i)));
    }
    tool_putc('"');
}

static void print_tryte_list(const tc48_memory* mem, tc48_word off, tc48_half len) {
    tool_putc('(');
    for (tc48_half i = 0; i < len; ++i) {
        if (i > 0) tool_putc(' ');
        if (off + i >= mem->size) {
            tool_puts("...");
            break;
        }
        tool_print("%u", (unsigned)tc48_mem_load6(mem, off + i));
    }
    tool_putc(')');
}

static void print_header(const tc48_memory* mem) {
    tobj_header header;
    if (!tobj_parse_header(mem, 0, &header)) {
        tool_show_error("failed to parse TOBJ header");
        return;
    }

    tool_puts("header:\n");
    tobj_dump_header(&header, "  ", stdout);
}

static void print_strings(const tc48_memory* mem) {
    tobj_header header;
    if (!tobj_parse_header(mem, 0, &header)) {
        tool_show_error("failed to parse TOBJ header");
        return;
    }

    tool_puts("strings:\n");
    tc48_word pos = header.string_table_off;
    for (tc48_half i = 0; i < header.string_count; ++i) {
        if (pos + TC48_HALF_TRYTES > mem->size) {
            tool_show_error("string table truncated at index %u", (unsigned)i);
            return;
        }

        tc48_half len = tc48_mem_load24(mem, pos);
        tc48_word chars_off = pos + TC48_HALF_TRYTES;

        tool_print("  [%u] off=%u len=%u ", (unsigned)i, (unsigned)pos, (unsigned)len);
        print_tscs_string(mem, pos);
        tool_putc(' ');
        print_tryte_list(mem, chars_off, len);
        tool_putc('\n');

        pos = chars_off + len;
    }
}

static void print_sections(const tc48_memory* mem) {
    tobj_header header;
    if (!tobj_parse_header(mem, 0, &header)) {
        tool_show_error("failed to parse TOBJ header");
        return;
    }

    tool_puts("sections:\n");
    tc48_word table_pos = header.section_table_off;
    for (tc48_half i = 0; i < header.section_count; ++i) {
        tobj_header_section section;
        if (!tobj_parse_section(mem, table_pos, &section)) {
            tool_show_error("failed to parse section %u", (unsigned)i);
            return;
        }

        tool_print("  [%u] name=", (unsigned)i);
        print_tscs_string(mem, header.string_table_off + section.name_off);
        tool_print(" off=%lu size=%lu align=%lu sym-start=%lu sym-count=%lu\n",
            (unsigned long)section.off,
            (unsigned long)section.size,
            (unsigned long)section.align,
            (unsigned long)section.sym_start_idx,
            (unsigned long)section.sym_count);

        if (section.size > 0) {
            tool_puts("    data:\n");
            tc48_mem_dump(mem, section.off, section.size);
        }

        table_pos += TOBJ_SECTION_SIZE_TRYTES;
    }
}

static void print_symbols(const tc48_memory* mem) {
    tobj_header header;
    if (!tobj_parse_header(mem, 0, &header)) {
        tool_show_error("failed to parse TOBJ header");
        return;
    }

    tool_puts("symbols:\n");
    tc48_word table_pos = header.symbol_table_off;
    for (tc48_half i = 0; i < header.symbol_count; ++i) {
        tobj_header_symbol sym;
        if (!tobj_parse_symbol(mem, table_pos, &sym)) {
            tool_show_error("failed to parse symbol %u", (unsigned)i);
            return;
        }

        tool_print("  [%u] name=", (unsigned)i);
        print_tscs_string(mem, header.string_table_off + sym.name_off);
        tool_print(" section=%u off=%u size=%u binding=%s\n",
            (unsigned)sym.section_idx,
            (unsigned)sym.off,
            (unsigned)sym.size,
            tobj_binding_to_str((tobj_binding)sym.binding, true)
        );

        table_pos += TOBJ_SYMBOL_SIZE_TRYTES;
    }
}

static void print_relocs(const tc48_memory* mem) {
    tobj_header header;
    if (!tobj_parse_header(mem, 0, &header)) {
        tool_show_error("failed to parse TOBJ header");
        return;
    }

    tool_puts("relocs:\n");
    tc48_word table_pos = header.reloc_table_off;
    for (tc48_half i = 0; i < header.reloc_count; ++i) {
        tobj_reloc reloc;
        if (!tobj_parse_reloc(mem, table_pos, &reloc)) {
            tool_show_error("failed to parse reloc %u", (unsigned)i);
            return;
        }

        tool_print("  [%u] section=%lu offset=%lu sym=%lu type=%s\n", (unsigned)i,
            (unsigned long)reloc.section_idx,
            (unsigned long)reloc.offset,
            (unsigned long)reloc.sym_idx,
            tobj_reloc_type_to_str(reloc.type, true)
        );

        table_pos += TOBJ_RELOC_SIZE_TRYTES;
    }
}

int main(int argc, const char* argv[]) {
    bool dump_strings = false, dump_sections = false,
         dump_symbols = false, dump_relocs = false;

    bool dump_all = false;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_BOOLEAN(0,  "strings",  &dump_strings,  "dump the string table",       NULL, 0, 0),
        OPT_BOOLEAN(0,  "sections", &dump_sections, "dump the sections table",     NULL, 0, 0),
        OPT_BOOLEAN(0,  "symbols",  &dump_symbols,  "dump the symbols table",      NULL, 0, 0),
        OPT_BOOLEAN(0,  "relocs",   &dump_relocs,   "dump the rellocations table", NULL, 0, 0),
        OPT_BOOLEAN('a', "all",     &dump_all,      "dump everything",             NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse,
        "dump string/section/symbol/reloc tables and other data from a tobj file",
        "github: https://github.com/TC-48/tobj-stage0/tree/main"
    );
    argc = argparse_parse(&argparse, argc, argv);

    if (argc != 1) {
        argparse_usage(&argparse);
        return 2;
    }

    const char* path = argparse.out[0];
    tc48_memory* mem = tc48_load_t48b(path);
    if (!mem) return 1;

    int ret = 0;

    tobj_header header;
    if (!tobj_parse_header(mem, 0, &header)) {
        tool_show_error("'%s' is not a valid TOBJ file", path);
        ret = 1; goto cleanup;
    }
    if (header.magic != TOBJ_MAGIC) {
        tool_show_error("'%s' has invalid TOBJ magic", path);
        ret = 1; goto cleanup;
    }
    if (header.version != TOBJ_FORMAT_VERSION) {
        tool_show_error("'%s' format version is incompatible", path);
        ret = 1; goto cleanup;
    }

    print_header(mem);

    if (dump_all || dump_strings)  print_strings(mem);
    if (dump_all || dump_sections) print_sections(mem);
    if (dump_all || dump_symbols)  print_symbols(mem);
    if (dump_all || dump_relocs)   print_relocs(mem);

cleanup:
    tc48_mem_free(mem);
    return ret;
}
