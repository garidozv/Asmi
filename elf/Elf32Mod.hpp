#ifndef ELF32_H
#define ELF32_H

#include <stdint.h>

// Definitions for modified Elf32 files

// Elf32 data types

typedef uint32_t    Elf32_Addr;
typedef uint16_t    Elf32_Half;
typedef uint32_t    Elf32_Off;
typedef int32_t     Elf32_Sword;
typedef uint32_t    Elf32_Word; 


#define EI_NIDENT   16


// Elf32 header structure

struct Elf32_Ehdr {
    Elf32_Addr      e_entry;    // Starting address of a program(not used in relocatable files)
    Elf32_Off       e_shoff;    // Offset from beginning fo the file to section header table(offset to program header table is known, because it comes right after header)
    Elf32_Half      e_type;     // Type of object file
    Elf32_Half      e_phnum;    // Number of entries in program header table
    Elf32_Half      e_shnum;    // Number of entries in section header table
    Elf32_Half      e_strndx;   // Section header table entry associated with string table(only one string table will be used for whole file)
};



// Object file types used for Elf32 header member e_type

#define ET_REL      1           // Relocatable file
#define ET_EXEC     2           // Executable file


// Special section indexes in section header tabel

#define SHN_UNDEF           0
#define SHN_ABS             1
#define SHN_COMMON          2


// Elf32 section header structure

struct Elf32_Shdr {
    Elf32_Word      sh_name;        // Index into string table section, whose header is at index specified by e_strndx member of ELF32 header
    Elf32_Word      sh_type;        // Type of section
    //Elf32_Word      sh_flags;       // Access flags TODO - maybe we dont need this
    Elf32_Addr      sh_addr;        // Address at which this sections first byte should reside(if it will appear in memory)
    Elf32_Off       sh_offset;      // Offset from beginning of the file to section contents
    Elf32_Word      sh_size;        // Section size in bytes
    Elf32_Word      sh_link;        // for relocation table header sh_link holds section header index of associated symbol table
    Elf32_Word      sh_info;        // for relocation table header sh_info holds section header index of section to which the relocation applies
    //Elf32_Word      sh_addralign;?    TODO - do we need this? We can just align everything on 4B
};

#define SHDR_SIZE sizeof(Elf32_Shdr)


// Section types used for Elf32 section header member sh_type

#define SHT_NULL        0
#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4
#define SHT_HASH        5
#define SHT_DYNAMIC     6
#define SHT_NOTE        7
#define SHT_NOBITS      8
#define SHT_REL         9
#define SHT_SHLIB       10
#define SHT_DYNSYM      11


// Section attribute flags used for Elf32 section header member sh_flags

#define SHF_WRITE           0x1
#define SHF_ALLOC           0x2
#define SHF_EXECINSTR       0x4
#define SHF_MASKPROC        0xf0000000


// TODO - sh_link and sh_info interpretation; Special sections

/*
    A string table index refers to starting byte in the string table section
    First byte, at index 0, and last byte, always hold null characters
    All of the strings in the string table are null terminated
*/

// Elf32 symbol table entry structure

struct Elf32_Sym {
    Elf32_Word          st_name;    // Index into string table section, whose header is at index specified by e_strndx member of ELF32 header
    Elf32_Addr          st_value;   // Symbols value
    Elf32_Word          st_size;    // Symbols size(will probably only be 0)
    unsigned char       st_info;    // Symbols type and binding attributes
    //unsigned char       st_other;   // Wont be used, but will keep it because of the alignment 
    //Elf32_Half          st_shndx;     
};



#define STN_UNDEF   0


/*
    st_info member of Elf32 symbol table entry specifies symbol's type and binding attributes
    Lower nibble specifies type, and higher nibble specifies binding 
*/ 

// Macros used to manipulate with st_info and the values it holds

#define ELF32_ST_BIND(i)        ((i)>>4)
#define ELF32_ST_TYPE(i)        ((i)&0xf)
#define ELF32_ST_INFO(b, t)     (((b)<<4)+((t)&0xf))


// Symbol bindings used for Elf32 symbol table entry member st_info, specifically its binding value

#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STB_WEAK        2


// Symbol types used for Elf32 symbol table entry member st_info, specifically its type value

#define STT_NOTYPE      0
#define STT_OBJECT      1
#define STT_FUNC        2
#define STT_SECTION     3
#define STT_FILE        4


// Elf32 relocation table entry structure

struct Elf32_Rela {
    Elf32_Addr      r_offset;   // Location at which to apply relocation action
    Elf32_Word      r_info;     // Index of associated symbol and type of relocation
    Elf32_Sword     r_addend;   // Addend
};


/*
    r_info member of Elf32 relocation table entry specifies both symbol table index with respect to which the realocation must be made,
    and the type of relocation
    Lower byte specifies type, and rest of the 24 bits specify symbol table index 
*/ 

// Macros used to manipulate with r_info and the values it holds

#define ELF32_R_SYM(i)        ((i)>>8)
#define ELF32_R_TYPE(i)        ((unsigned char)(i))
#define ELF32_R_INFO(s, t)     (((s)<<8)+(unsigned char)(t))

// Relocation types - for now we only need one

#define R_32        1

// Elf32 program header structure
// TODO - check this

struct Elf32_Phdr {
    Elf32_Word      p_type;
    Elf32_Off       p_offset;
    Elf32_Addr      p_vaddr;
    Elf32_Addr      p_paddr;
    Elf32_Word      p_filesz;
    Elf32_Word      p_memsz;
    Elf32_Word      p_flags;
    Elf32_Word      p_align;
};

#define PHDR_SIZE sizeof(Elf32_Phdr)

// Segment types used for Elf32 program header member p_type

#define PT_NULL         0
#define PT_LOAD         1
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5
#define PT_PHDR         6

// Segment flag bit used for Elf32 program header member p_flags

#define PF_X            0x1
#define PF_W            0x2
#define PF_R            0x4
#define PF_MASKPROC     0xf0000000


// TODO - add Dynamic section definitions if needed

#endif
