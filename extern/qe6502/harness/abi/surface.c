#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#else
#    include <dlfcn.h>
#endif

#define QE6502_SURFACE_MAX_REASONABLE_SYMBOLS 65536u

static const char *const qe6502abi_expected[] = {
    "qe6502abi_irq_assert",
    "qe6502abi_is_irq_asserted",
    "qe6502abi_nmi_assert",
    "qe6502abi_is_nmi_asserted",
    "qe6502abi_get_a",
    "qe6502abi_get_model",
    "qe6502abi_get_p",
    "qe6502abi_get_pc",
    "qe6502abi_get_s",
    "qe6502abi_get_x",
    "qe6502abi_get_y",
    "qe6502abi_goto",
    "qe6502abi_load",
    "qe6502abi_restart",
    "qe6502abi_save",
    "qe6502abi_set_a",
    "qe6502abi_set_model",
    "qe6502abi_set_p",
    "qe6502abi_set_pc",
    "qe6502abi_set_s",
    "qe6502abi_set_x",
    "qe6502abi_set_y",
    "qe6502abi_setup",
    "qe6502abi_tick",
    "qe6502abi_version"
};

typedef struct qe6502abi_symbol_set_t {
    char **items;
    size_t count;
    size_t capacity;
} qe6502abi_symbol_set_t;

typedef struct qe6502abi_file_t {
    unsigned char *data;
    size_t size;
} qe6502abi_file_t;

typedef struct qe6502abi_library_t {
#if defined(_WIN32)
    HMODULE handle;
#else
    void *handle;
#endif
} qe6502abi_library_t;

static int qe6502abi_starts_with(const char *text, const char *prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static int qe6502abi_expected_contains(const char *symbol)
{
    size_t i;

    for(i = 0u; i < (sizeof(qe6502abi_expected) / sizeof(qe6502abi_expected[0])); ++i) {
        if(strcmp(symbol, qe6502abi_expected[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

static char *qe6502abi_strdup(const char *text)
{
    size_t length;
    char *copy;

    length = strlen(text) + 1u;
    copy = (char *)malloc(length);
    if(copy == NULL) {
        return NULL;
    }

    (void)memcpy(copy, text, length);
    return copy;
}

static int qe6502abi_symbol_set_contains(const qe6502abi_symbol_set_t *set, const char *symbol)
{
    size_t i;

    if((set == NULL) || (symbol == NULL)) {
        return 0;
    }

    for(i = 0u; i < set->count; ++i) {
        if(strcmp(set->items[i], symbol) == 0) {
            return 1;
        }
    }

    return 0;
}

static int qe6502abi_symbol_set_add(qe6502abi_symbol_set_t *set, const char *symbol)
{
    char **next_items;
    char *copy;
    size_t next_capacity;

    if((set == NULL) || (symbol == NULL) || (symbol[0] == '\0')) {
        return 0;
    }

    if(qe6502abi_symbol_set_contains(set, symbol)) {
        return 0;
    }

    if(set->count >= QE6502_SURFACE_MAX_REASONABLE_SYMBOLS) {
        fprintf(stderr, "refusing unreasonable export table size\n");
        return 1;
    }

    if(set->count == set->capacity) {
        next_capacity = set->capacity == 0u ? 32u : set->capacity * 2u;
        next_items = (char **)realloc(set->items, next_capacity * sizeof(set->items[0]));
        if(next_items == NULL) {
            return 1;
        }
        set->items = next_items;
        set->capacity = next_capacity;
    }

    copy = qe6502abi_strdup(symbol);
    if(copy == NULL) {
        return 1;
    }

    set->items[set->count] = copy;
    set->count += 1u;
    return 0;
}

static void qe6502abi_symbol_set_free(qe6502abi_symbol_set_t *set)
{
    size_t i;

    if(set == NULL) {
        return;
    }

    for(i = 0u; i < set->count; ++i) {
        free(set->items[i]);
    }

    free(set->items);
    set->items = NULL;
    set->count = 0u;
    set->capacity = 0u;
}

static int qe6502abi_read_file(qe6502abi_file_t *file, const char *path)
{
    FILE *fp = NULL;
    long length;
    size_t read_length;

    if((file == NULL) || (path == NULL)) {
        return 1;
    }

    file->data = NULL;
    file->size = 0u;

#if defined(_WIN32)
    if(fopen_s(&fp, path, "rb") != 0) {
        fp = NULL;
    }
#else
    fp = fopen(path, "rb");
#endif
    if(fp == NULL) {
        fprintf(stderr, "failed to open ABI library for export scan: %s\n", path);
        return 1;
    }

    if(fseek(fp, 0L, SEEK_END) != 0) {
        (void)fclose(fp);
        return 1;
    }

    length = ftell(fp);
    if(length <= 0L) {
        (void)fclose(fp);
        return 1;
    }

    if(fseek(fp, 0L, SEEK_SET) != 0) {
        (void)fclose(fp);
        return 1;
    }

    file->size = (size_t)length;
    file->data = (unsigned char *)malloc(file->size);
    if(file->data == NULL) {
        (void)fclose(fp);
        return 1;
    }

    read_length = fread(file->data, 1u, file->size, fp);
    (void)fclose(fp);

    if(read_length != file->size) {
        free(file->data);
        file->data = NULL;
        file->size = 0u;
        return 1;
    }

    return 0;
}

static void qe6502abi_file_free(qe6502abi_file_t *file)
{
    if(file == NULL) {
        return;
    }

    free(file->data);
    file->data = NULL;
    file->size = 0u;
}

static int qe6502abi_has_range(size_t file_size, uint64_t offset, uint64_t size)
{
    uint64_t file_size64;

    file_size64 = (uint64_t)file_size;
    return (offset <= file_size64) && (size <= (file_size64 - offset));
}

static uint16_t qe6502abi_read_u16le(const unsigned char *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}

static uint32_t qe6502abi_read_u32le(const unsigned char *p)
{
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8u) | ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

static uint64_t qe6502abi_read_u64le(const unsigned char *p)
{
    return ((uint64_t)qe6502abi_read_u32le(p)) | ((uint64_t)qe6502abi_read_u32le(p + 4) << 32u);
}

static const char *qe6502abi_string_at(const qe6502abi_file_t *file, uint64_t offset, uint64_t table_end)
{
    uint64_t i;

    if((file == NULL) || !qe6502abi_has_range(file->size, offset, 1u) || (offset >= table_end)) {
        return NULL;
    }

    for(i = offset; i < table_end; ++i) {
        if(file->data[i] == '\0') {
            return (const char *)(const void *)(file->data + offset);
        }
    }

    return NULL;
}

static int qe6502abi_collect_normalized_export(qe6502abi_symbol_set_t *exports, const char *raw_name)
{
    const char *name;

    if(raw_name == NULL) {
        return 0;
    }

    name = raw_name;
    if((name[0] == '_') && (qe6502abi_starts_with(name + 1, "qe6502"))) {
        name += 1;
    }

    if(qe6502abi_starts_with(name, "qe6502")) {
        return qe6502abi_symbol_set_add(exports, name);
    }

    return 0;
}

static int qe6502abi_collect_elf_exports(const qe6502abi_file_t *file, qe6502abi_symbol_set_t *exports)
{
    const unsigned char *data;
    uint8_t elf_class;
    uint64_t shoff;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
    uint64_t shstr_offset;
    uint64_t shstr_size;
    uint16_t i;
    int found_dynsym;

    if((file == NULL) || (file->size < 16u)) {
        return 1;
    }

    data = file->data;
    if((data[0] != 0x7fu) || (data[1] != 'E') || (data[2] != 'L') || (data[3] != 'F')) {
        return 1;
    }

    if(data[5] != 1u) {
        fprintf(stderr, "unsupported non-little-endian ELF ABI library\n");
        return 1;
    }

    elf_class = data[4];
    if(elf_class == 2u) {
        if(!qe6502abi_has_range(file->size, 0u, 64u)) {
            return 1;
        }
        shoff = qe6502abi_read_u64le(data + 40);
        shentsize = qe6502abi_read_u16le(data + 58);
        shnum = qe6502abi_read_u16le(data + 60);
        shstrndx = qe6502abi_read_u16le(data + 62);
    } else if(elf_class == 1u) {
        if(!qe6502abi_has_range(file->size, 0u, 52u)) {
            return 1;
        }
        shoff = qe6502abi_read_u32le(data + 32);
        shentsize = qe6502abi_read_u16le(data + 46);
        shnum = qe6502abi_read_u16le(data + 48);
        shstrndx = qe6502abi_read_u16le(data + 50);
    } else {
        return 1;
    }

    if((shoff == 0u) || (shnum == 0u) || (shstrndx >= shnum) || (shentsize == 0u)) {
        fprintf(stderr, "ELF ABI library has no section table for export scan\n");
        return 1;
    }

    if(!qe6502abi_has_range(file->size, shoff, (uint64_t)shentsize * (uint64_t)shnum)) {
        return 1;
    }

    if(elf_class == 2u) {
        const unsigned char *shstr;
        shstr = data + shoff + ((uint64_t)shentsize * (uint64_t)shstrndx);
        shstr_offset = qe6502abi_read_u64le(shstr + 24);
        shstr_size = qe6502abi_read_u64le(shstr + 32);
    } else {
        const unsigned char *shstr;
        shstr = data + shoff + ((uint64_t)shentsize * (uint64_t)shstrndx);
        shstr_offset = qe6502abi_read_u32le(shstr + 16);
        shstr_size = qe6502abi_read_u32le(shstr + 20);
    }

    if(!qe6502abi_has_range(file->size, shstr_offset, shstr_size)) {
        return 1;
    }

    found_dynsym = 0;
    for(i = 0u; i < shnum; ++i) {
        const unsigned char *sh;
        const char *section_name;
        uint32_t sh_name;
        uint32_t sh_type;
        uint32_t link_index;
        uint64_t sym_offset;
        uint64_t sym_size;
        uint64_t sym_entsize;
        uint64_t str_offset;
        uint64_t str_size;
        uint64_t n;
        uint64_t count;

        sh = data + shoff + ((uint64_t)shentsize * (uint64_t)i);
        sh_name = qe6502abi_read_u32le(sh + 0);
        if(!qe6502abi_has_range(file->size, shstr_offset + (uint64_t)sh_name, 1u)) {
            continue;
        }
        section_name = qe6502abi_string_at(file, shstr_offset + (uint64_t)sh_name, shstr_offset + shstr_size);
        if((section_name == NULL) || (strcmp(section_name, ".dynsym") != 0)) {
            continue;
        }

        if(elf_class == 2u) {
            sh_type = qe6502abi_read_u32le(sh + 4);
            link_index = qe6502abi_read_u32le(sh + 40);
            sym_offset = qe6502abi_read_u64le(sh + 24);
            sym_size = qe6502abi_read_u64le(sh + 32);
            sym_entsize = qe6502abi_read_u64le(sh + 56);
        } else {
            sh_type = qe6502abi_read_u32le(sh + 4);
            link_index = qe6502abi_read_u32le(sh + 24);
            sym_offset = qe6502abi_read_u32le(sh + 16);
            sym_size = qe6502abi_read_u32le(sh + 20);
            sym_entsize = qe6502abi_read_u32le(sh + 36);
        }

        if((sh_type != 11u) || (link_index >= (uint32_t)shnum) || (sym_entsize == 0u)) {
            continue;
        }

        if(!qe6502abi_has_range(file->size, sym_offset, sym_size)) {
            return 1;
        }

        {
            const unsigned char *strsh;
            strsh = data + shoff + ((uint64_t)shentsize * (uint64_t)link_index);
            if(elf_class == 2u) {
                str_offset = qe6502abi_read_u64le(strsh + 24);
                str_size = qe6502abi_read_u64le(strsh + 32);
            } else {
                str_offset = qe6502abi_read_u32le(strsh + 16);
                str_size = qe6502abi_read_u32le(strsh + 20);
            }
        }

        if(!qe6502abi_has_range(file->size, str_offset, str_size)) {
            return 1;
        }

        count = sym_size / sym_entsize;
        for(n = 0u; n < count; ++n) {
            const unsigned char *sym;
            const char *name;
            uint32_t name_offset;
            uint8_t info;
            uint8_t other;
            uint16_t shndx;
            uint8_t bind;
            uint8_t type;
            uint8_t visibility;

            sym = data + sym_offset + (sym_entsize * n);
            name_offset = qe6502abi_read_u32le(sym + 0);
            if(elf_class == 2u) {
                info = sym[4];
                other = sym[5];
                shndx = qe6502abi_read_u16le(sym + 6);
            } else {
                info = sym[12];
                other = sym[13];
                shndx = qe6502abi_read_u16le(sym + 14);
            }

            bind = (uint8_t)(info >> 4u);
            type = (uint8_t)(info & 0x0fu);
            visibility = (uint8_t)(other & 0x03u);

            if((name_offset == 0u) || (shndx == 0u) || ((bind != 1u) && (bind != 2u)) ||
               ((type != 2u) && (type != 0u)) || ((visibility != 0u) && (visibility != 3u))) {
                continue;
            }

            if(!qe6502abi_has_range(file->size, str_offset + (uint64_t)name_offset, 1u)) {
                continue;
            }

            name = qe6502abi_string_at(file, str_offset + (uint64_t)name_offset, str_offset + str_size);
            if(qe6502abi_collect_normalized_export(exports, name) != 0) {
                return 1;
            }
        }

        found_dynsym = 1;
    }

    if(!found_dynsym) {
        fprintf(stderr, "ELF ABI library has no .dynsym section for export scan\n");
        return 1;
    }

    return 0;
}

static int qe6502abi_collect_macho_exports(const qe6502abi_file_t *file, qe6502abi_symbol_set_t *exports)
{
    const unsigned char *data;
    uint32_t magic;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint64_t load_offset;
    uint64_t symoff;
    uint64_t nsyms;
    uint64_t stroff;
    uint64_t strsize;
    int is64;
    uint32_t cmd_index;
    int found_symtab;

    if((file == NULL) || !qe6502abi_has_range(file->size, 0u, 28u)) {
        return 1;
    }

    data = file->data;
    magic = qe6502abi_read_u32le(data);
    if(magic == 0xfeedfacfu) {
        is64 = 1;
        if(!qe6502abi_has_range(file->size, 0u, 32u)) {
            return 1;
        }
        ncmds = qe6502abi_read_u32le(data + 16);
        sizeofcmds = qe6502abi_read_u32le(data + 20);
        load_offset = 32u;
    } else if(magic == 0xfeedfaceu) {
        is64 = 0;
        ncmds = qe6502abi_read_u32le(data + 16);
        sizeofcmds = qe6502abi_read_u32le(data + 20);
        load_offset = 28u;
    } else {
        return 1;
    }

    if(!qe6502abi_has_range(file->size, load_offset, sizeofcmds)) {
        return 1;
    }

    symoff = 0u;
    nsyms = 0u;
    stroff = 0u;
    strsize = 0u;
    found_symtab = 0;

    for(cmd_index = 0u; cmd_index < ncmds; ++cmd_index) {
        uint32_t cmd;
        uint32_t cmdsize;

        if(!qe6502abi_has_range(file->size, load_offset, 8u)) {
            return 1;
        }

        cmd = qe6502abi_read_u32le(data + load_offset);
        cmdsize = qe6502abi_read_u32le(data + load_offset + 4u);
        if((cmdsize < 8u) || !qe6502abi_has_range(file->size, load_offset, cmdsize)) {
            return 1;
        }

        if(cmd == 0x2u) {
            if(cmdsize < 24u) {
                return 1;
            }
            symoff = qe6502abi_read_u32le(data + load_offset + 8u);
            nsyms = qe6502abi_read_u32le(data + load_offset + 12u);
            stroff = qe6502abi_read_u32le(data + load_offset + 16u);
            strsize = qe6502abi_read_u32le(data + load_offset + 20u);
            found_symtab = 1;
        }

        load_offset += cmdsize;
    }

    if(!found_symtab || !qe6502abi_has_range(file->size, stroff, strsize)) {
        fprintf(stderr, "Mach-O ABI library has no usable symbol table for export scan\n");
        return 1;
    }

    if(!qe6502abi_has_range(file->size, symoff, nsyms * (uint64_t)(is64 ? 16u : 12u))) {
        return 1;
    }

    {
        uint64_t i;
        for(i = 0u; i < nsyms; ++i) {
            const unsigned char *sym;
            const char *name;
            uint32_t strx;
            uint8_t n_type;
            uint8_t n_sect;

            sym = data + symoff + (i * (uint64_t)(is64 ? 16u : 12u));
            strx = qe6502abi_read_u32le(sym);
            n_type = sym[4];
            n_sect = sym[5];

            if((strx == 0u) || ((n_type & 0x01u) == 0u) || ((n_type & 0x0eu) == 0x00u) || (n_sect == 0u)) {
                continue;
            }

            if(!qe6502abi_has_range(file->size, stroff + (uint64_t)strx, 1u)) {
                continue;
            }

            name = qe6502abi_string_at(file, stroff + (uint64_t)strx, stroff + strsize);
            if(qe6502abi_collect_normalized_export(exports, name) != 0) {
                return 1;
            }
        }
    }

    return 0;
}

static uint64_t qe6502abi_pe_rva_to_offset(const qe6502abi_file_t *file, uint64_t section_table, uint16_t section_count,
                                            uint32_t rva)
{
    uint16_t i;

    for(i = 0u; i < section_count; ++i) {
        const unsigned char *section;
        uint32_t virtual_size;
        uint32_t virtual_address;
        uint32_t raw_size;
        uint32_t raw_pointer;
        uint32_t mapped_size;

        if(!qe6502abi_has_range(file->size, section_table + ((uint64_t)i * 40u), 40u)) {
            return UINT64_MAX;
        }

        section = file->data + section_table + ((uint64_t)i * 40u);
        virtual_size = qe6502abi_read_u32le(section + 8u);
        virtual_address = qe6502abi_read_u32le(section + 12u);
        raw_size = qe6502abi_read_u32le(section + 16u);
        raw_pointer = qe6502abi_read_u32le(section + 20u);
        mapped_size = virtual_size > raw_size ? virtual_size : raw_size;

        if(((uint64_t)rva >= (uint64_t)virtual_address) &&
           ((uint64_t)rva < ((uint64_t)virtual_address + (uint64_t)mapped_size))) {
            return (uint64_t)raw_pointer + ((uint64_t)rva - (uint64_t)virtual_address);
        }
    }

    return UINT64_MAX;
}

static int qe6502abi_collect_pe_exports(const qe6502abi_file_t *file, qe6502abi_symbol_set_t *exports)
{
    const unsigned char *data;
    uint32_t pe_offset;
    uint16_t number_of_sections;
    uint16_t size_of_optional_header;
    uint64_t coff_offset;
    uint64_t optional_offset;
    uint64_t section_table;
    uint16_t magic;
    uint64_t data_directory_offset;
    uint32_t export_rva;
    uint32_t export_size;
    uint64_t export_offset;
    uint32_t number_of_names;
    uint32_t address_of_names_rva;
    uint64_t address_of_names_offset;
    uint32_t i;

    if((file == NULL) || !qe6502abi_has_range(file->size, 0u, 0x40u)) {
        return 1;
    }

    data = file->data;
    if((data[0] != 'M') || (data[1] != 'Z')) {
        return 1;
    }

    pe_offset = qe6502abi_read_u32le(data + 0x3cu);
    if(!qe6502abi_has_range(file->size, pe_offset, 24u) || (memcmp(data + pe_offset, "PE\0\0", 4u) != 0)) {
        return 1;
    }

    coff_offset = (uint64_t)pe_offset + 4u;
    number_of_sections = qe6502abi_read_u16le(data + coff_offset + 2u);
    size_of_optional_header = qe6502abi_read_u16le(data + coff_offset + 16u);
    optional_offset = coff_offset + 20u;

    if(!qe6502abi_has_range(file->size, optional_offset, size_of_optional_header)) {
        return 1;
    }

    magic = qe6502abi_read_u16le(data + optional_offset);
    if(magic == 0x20bu) {
        data_directory_offset = optional_offset + 112u;
    } else if(magic == 0x10bu) {
        data_directory_offset = optional_offset + 96u;
    } else {
        return 1;
    }

    if(!qe6502abi_has_range(file->size, data_directory_offset, 8u)) {
        return 1;
    }

    export_rva = qe6502abi_read_u32le(data + data_directory_offset);
    export_size = qe6502abi_read_u32le(data + data_directory_offset + 4u);
    if((export_rva == 0u) || (export_size == 0u)) {
        fprintf(stderr, "PE ABI library has no export table\n");
        return 1;
    }

    section_table = optional_offset + (uint64_t)size_of_optional_header;
    if(!qe6502abi_has_range(file->size, section_table, (uint64_t)number_of_sections * 40u)) {
        return 1;
    }

    export_offset = qe6502abi_pe_rva_to_offset(file, section_table, number_of_sections, export_rva);
    if((export_offset == UINT64_MAX) || !qe6502abi_has_range(file->size, export_offset, 40u)) {
        return 1;
    }

    number_of_names = qe6502abi_read_u32le(data + export_offset + 24u);
    address_of_names_rva = qe6502abi_read_u32le(data + export_offset + 32u);
    address_of_names_offset = qe6502abi_pe_rva_to_offset(file, section_table, number_of_sections, address_of_names_rva);
    if((address_of_names_offset == UINT64_MAX) ||
       !qe6502abi_has_range(file->size, address_of_names_offset, (uint64_t)number_of_names * 4u)) {
        return 1;
    }

    for(i = 0u; i < number_of_names; ++i) {
        uint32_t name_rva;
        uint64_t name_offset;
        const char *name;

        name_rva = qe6502abi_read_u32le(data + address_of_names_offset + ((uint64_t)i * 4u));
        name_offset = qe6502abi_pe_rva_to_offset(file, section_table, number_of_sections, name_rva);
        if(name_offset == UINT64_MAX) {
            continue;
        }

        name = qe6502abi_string_at(file, name_offset, (uint64_t)file->size);
        if(qe6502abi_collect_normalized_export(exports, name) != 0) {
            return 1;
        }
    }

    return 0;
}

static int qe6502abi_collect_binary_exports(const char *path, qe6502abi_symbol_set_t *exports)
{
    qe6502abi_file_t file;
    uint32_t magic;
    int result;

    if(qe6502abi_read_file(&file, path) != 0) {
        return 1;
    }

    result = 1;
    magic = file.size >= 4u ? qe6502abi_read_u32le(file.data) : 0u;

    if((file.size >= 4u) && (file.data[0] == 0x7fu) && (file.data[1] == 'E') && (file.data[2] == 'L') &&
       (file.data[3] == 'F')) {
        result = qe6502abi_collect_elf_exports(&file, exports);
    } else if((magic == 0xfeedfaceu) || (magic == 0xfeedfacfu)) {
        result = qe6502abi_collect_macho_exports(&file, exports);
    } else if((file.size >= 2u) && (file.data[0] == 'M') && (file.data[1] == 'Z')) {
        result = qe6502abi_collect_pe_exports(&file, exports);
    } else {
        fprintf(stderr, "unsupported ABI library binary format for export scan: %s\n", path);
    }

    qe6502abi_file_free(&file);
    return result;
}

static int qe6502abi_library_open(qe6502abi_library_t *library, const char *path)
{
    if(library == NULL) {
        return 1;
    }

#if defined(_WIN32)
    library->handle = LoadLibraryA(path);
    if(library->handle == NULL) {
        fprintf(stderr, "failed to load ABI library: %s (GetLastError=%lu)\n", path, (unsigned long)GetLastError());
        return 1;
    }
#else
    library->handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if(library->handle == NULL) {
        const char *message = dlerror();
        fprintf(stderr, "failed to load ABI library: %s%s%s\n", path, message != NULL ? ": " : "",
                message != NULL ? message : "");
        return 1;
    }
#endif

    return 0;
}

static void qe6502abi_library_close(qe6502abi_library_t *library)
{
    if(library == NULL) {
        return;
    }

#if defined(_WIN32)
    if(library->handle != NULL) {
        (void)FreeLibrary(library->handle);
        library->handle = NULL;
    }
#else
    if(library->handle != NULL) {
        (void)dlclose(library->handle);
        library->handle = NULL;
    }
#endif
}

static int qe6502abi_library_has_symbol(qe6502abi_library_t *library, const char *symbol)
{
    if((library == NULL) || (symbol == NULL)) {
        return 0;
    }

#if defined(_WIN32)
    return GetProcAddress(library->handle, symbol) != NULL;
#else
    (void)dlerror();
    (void)dlsym(library->handle, symbol);
    return dlerror() == NULL;
#endif
}

static int qe6502abi_validate_exports(const qe6502abi_symbol_set_t *exports)
{
    size_t i;
    int failures;

    failures = 0;

    for(i = 0u; i < (sizeof(qe6502abi_expected) / sizeof(qe6502abi_expected[0])); ++i) {
        if(!qe6502abi_symbol_set_contains(exports, qe6502abi_expected[i])) {
            fprintf(stderr, "ABI library export table is missing symbol: %s\n", qe6502abi_expected[i]);
            failures += 1;
        }
    }

    for(i = 0u; i < exports->count; ++i) {
        const char *symbol;

        symbol = exports->items[i];
        if(qe6502abi_starts_with(symbol, "qe6502abi_") && !qe6502abi_expected_contains(symbol)) {
            fprintf(stderr, "ABI library export table contains unexpected ABI symbol: %s\n", symbol);
            failures += 1;
        } else if(qe6502abi_starts_with(symbol, "qe6502_") && !qe6502abi_starts_with(symbol, "qe6502abi_")) {
            fprintf(stderr, "ABI library export table contains private core symbol: %s\n", symbol);
            failures += 1;
        }
    }

    return failures == 0 ? 0 : 1;
}

static int qe6502abi_validate_loader(qe6502abi_library_t *library)
{
    size_t i;
    int failures;

    failures = 0;
    for(i = 0u; i < (sizeof(qe6502abi_expected) / sizeof(qe6502abi_expected[0])); ++i) {
        if(!qe6502abi_library_has_symbol(library, qe6502abi_expected[i])) {
            fprintf(stderr, "ABI library loader cannot resolve exported symbol: %s\n", qe6502abi_expected[i]);
            failures += 1;
        }
    }

    return failures == 0 ? 0 : 1;
}

int main(int argc, char **argv)
{
    qe6502abi_library_t library;
    qe6502abi_symbol_set_t exports;
    const char *library_path;
    int result;

    if(argc != 2) {
        fprintf(stderr, "usage: %s <qe6502 shared library>\n", argc > 0 ? argv[0] : "qe6502_abi_surface");
        return 2;
    }

    library.handle = NULL;
    exports.items = NULL;
    exports.count = 0u;
    exports.capacity = 0u;
    library_path = argv[1];
    result = 0;

    if(qe6502abi_collect_binary_exports(library_path, &exports) != 0) {
        qe6502abi_symbol_set_free(&exports);
        return 1;
    }

    if(qe6502abi_validate_exports(&exports) != 0) {
        result = 1;
    }

    if(qe6502abi_library_open(&library, library_path) != 0) {
        qe6502abi_symbol_set_free(&exports);
        return 1;
    }

    if(qe6502abi_validate_loader(&library) != 0) {
        result = 1;
    }

    qe6502abi_library_close(&library);

    if(result == 0) {
        printf("shared ABI surface OK: %lu expected exports, %lu qe6502 exports scanned\n",
               (unsigned long)(sizeof(qe6502abi_expected) / sizeof(qe6502abi_expected[0])),
               (unsigned long)exports.count);
    }

    qe6502abi_symbol_set_free(&exports);
    return result == 0 ? 0 : 1;
}
