#ifndef ELF32_H
#define ELF32_H

#include <stdint.h>

// Elf32 data types

typedef uint32_t    Elf32_Addr;
typedef uint16_t    Elf32_Half;
typedef uint32_t    Elf32_Off;
typedef int32_t     Elf32_Sword;
typedef uint32_t    Elf32_Word; 


#define EI_NIDENT   16


// Elf32 header structure

struct Elf32_Ehdr {
    unsigned char   e_ident[EI_NIDENT];
    Elf32_Half      e_type;
    Elf32_Half      e_machine;
    Elf32_Word      e_version;
    Elf32_Addr      e_entry;
    Elf32_Off       e_phoff;
    Elf32_Off       shoff;
    Elf32_Word      e_flags;
    Elf32_Half      e_ehsize;
    Elf32_Half      e_phentsize;
    Elf32_Half      e_phnum;
    Elf32_Half      e_shentsize;
    Elf32_Half      e_shnum;
    Elf32_Half      e_shstrndx;
};


// Object file types used for Elf32 header member e_type

#define ET_NONE     0           // No file type
#define ET_REL      1           // Relocatable file
#define ET_EXEC     2           // Executable file
#define ET_DYN      3           // Shared object file
#define ET_CORE     4           // Core file
#define ET_LOPROC   0xff00      // Processor-specific
#define ET_HIPROC   0xffff      // Processor-specific


// Architecture specifiers used for Elf32 header member e_machine
// We will only be using EM_NONE, since we will be working with abstract architecture specific to this project

#define EM_NONE         0
#define EM_M32          1
#define EM_SPARC        2
#define EM_386          3
#define EM_68K          4
#define EM_88K          5
#define EM_860          7
#define EM_MIPS         8
#define EM_MIPS_RS4_BE  10


// Object file versions used for Elf32 header member e_version

#define EV_NONE     0
#define EV_CURRENT  1


// Identification indexes for used for bytes of Elf32 header member e_type

#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_PAD      7


// ELF object file magic number

#define ELFMAG0     0x7f
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'


// File classes used for EI_CLASS byte of e_type
// Our abstract architecture is 32-bit, so we will only be using ELFCLASS32

#define ELFCLASSNONE    0
#define ELFCLASS32      1
#define ELFCLASS64      2


// Data encodings for data in object file, used for EI_DATA byte of e_type
// Our abstract architecture uses little endian, which has nothing to do with data in Elf32 object file, but, because of the consistency
// we will only be using little endian in ELf32 object files as well

#define ELFDATANONE     0
#define ELFDATA2LSB     1       // 2's complement, little endian
#define ELFDATA2MSB     2       // 2's complement, big endian


// Special section indexes in section header tabel
// TODO - Maybe we will ahve to modify these

#define SHN_UNDEF           0
#define SHN_LORESERVE       0xff00
#define SHN_LOPROC          0xff00
#define SHN_HIPROC          0xff1f
#define SHN_ABS             0xfff1
#define SHN_COMMON          0xfff2
#define SHN_HIRESERVE       0xffff


// Elf32 section header structure

struct Elf32_Shdr {
    Elf32_Word      sh_name;        // Index into section header string table section, whose header is at index specified by e_shstrndx member of ELF32 header
    Elf32_Word      sh_type;
    Elf32_Word      sh_flags;
    Elf32_Addr      sh_addr;
    Elf32_Off       sh_offset;
    Elf32_Word      sh_size;
    Elf32_Word      sh_link;
    Elf32_Word      sh_info;
    Elf32_Word      sh_addralign;
    Elf32_Word      sh_entsize;
};


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
#define SHT_LOPROC      0x70000000
#define SHT_HIPROC      0x7fffffff
#define SHT_LOUSER      0x80000000
#define SHT_HIUSER      0xffffffff


// Section header for index 0 in section header table 

Elf32_Shdr SectionHeaderTable_Entry0 = {
    0, SHT_NULL, 0, 0, 0, 0, SHN_UNDEF, 0, 0, 0
};


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
    Elf32_Word          st_name;
    Elf32_Addr          st_value;
    Elf32_Word          st_size;
    unsigned char       st_info;
    unsigned char       st_other;
    Elf32_Half          st_shndx;
};


// Symbol table entry for index 0 which is reserved

Elf32_Sym SymbolTable_Entry0 {
    0, 0, 0, 0, 0, SHN_UNDEF
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
#define STB_LOPROC      13
#define STB_HIPROC      15


// Symbol types used for Elf32 symbol table entry member st_info, specifically its type value


#define STT_NOTYPE      0
#define STT_OBJECT      1
#define STT_FUNC        2
#define STT_SECTION     3
#define STT_FILE        4
#define STT_LOPROC      13
#define STT_HIPROC      15


// Elf32 relocation table entry structures

struct Elf32_Rel {
    Elf32_Addr      r_offset;
    Elf32_Word      r_info;
};

struct Elf32_Rela {
    Elf32_Addr      r_offset;
    Elf32_Word      r_info;
    Elf32_Sword     r_addend;
};


/*
    Entries of type Elf32_Rela contain an explicit addend, while entries of type Elf32_Rel store an explicit addend in the location to be modified
    We will only be using Elf32_Rela
*/


/*
    r_info member of Elf32 relocation table entry specifies both symbol table index with respect to which the realocation must be made,
    and the type of realication
    Lower byte specifies type, and rest of the 24 bits specify symbol table index 
*/ 

// Macros used to manipulate with r_info and the values it holds

#define ELF32_R_SYM(i)        ((i)>>8)
#define ELF32_R_TYPE(i)        ((unsigned char)(i))
#define ELF32_R_INFO(s, t)     (((s)<<8)+(unsigned char)(t))


// Elf32 program header structure

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


// Segment types used for Elf32 program header member p_type

#define PT_NULL         0
#define PT_LOAD         1
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5
#define PT_PHDR         6
#define PT_LOPROC       0x70000000
#define PT_HIPROC       0x7fffffff


// Segment flag bit used for Elf32 program header member p_flags

#define PF_X            0x1
#define PF_W            0x2
#define PF_R            0x4
#define PF_MASKPROC     0xf0000000


// TODO - add Dynamic section definitions if needed

#endif