#ifndef COMPUTERSYSTEM_H
#define COMPUTERSYSTEM_H

#include <stdint.h>
#include <unordered_map>

#define FLAG_TR 0x1   // Timer
#define FLAG_TL 0x2   // Terminal
#define FLAG_I  0x4

#define GPR_CNT 16
#define CSR_CNT 3

#define MM_REGS_BASE 0xffffff00

enum {
  STATUS, HANDLER, CAUSE, SP = 14, PC
};

enum {
  TERM_OUT, TERM_IN, TIM_CFG = 4
};

enum {
  INV, TIM, TERM, INT
};

enum Causes {
  INVALID_INSTR = 1,
  TIMER_INT,
  TERMINAL_INT,
  SOFTWARE_INT
};

class Memory {

  std::unordered_map<uint32_t, uint8_t> memory;

public:

  uint8_t read(uint32_t address) const {
    if ( memory.find(address) != memory.end() ) {
      return memory.at(address);
    }
    return 0;
  }

  void write(uint32_t address, uint8_t byte) {
    memory[address] = byte;
  }

  // Reads a word stored in little-endian format from memory
  uint32_t readWord(uint32_t address) const {
    uint32_t word = 0;
    for ( int i = 0; i < 4; i++) {
      word |= ((uint32_t)read(address + i) << i * 8);
    }
    return word;
  }

  // Writes a word to memory in little-endian format
  void writeWord(uint32_t address, uint32_t word) {
    for ( int i = 0; i < 4; i++) {
      write(address + i, (word >> 8 * i) & 0xff );
    }
  }

  uint32_t readMMReg(uint8_t index) const {
    return readWord(MM_REGS_BASE + index * 4);
  }

  void writeMMReg(uint8_t index, uint32_t word) {
    writeWord(MM_REGS_BASE + index * 4, word);
  }
};

struct CPU {

  uint32_t gpr[GPR_CNT] = {};
  uint32_t csr[CSR_CNT] = {};

  // Interrupt request flags
  // 0 - invalid instruction; 1 - timer; 2 - terminal; 3 - software interrupt; 
  // priority: 0 == 3 >> 2 >> 1
  bool IR[4] = {};
  // Value of status registers I bit before entering interrupt handling routine in which I has to be cleared(interrupts masked)
  bool lastI = 0;

  void setInterruptRequest(uint8_t cause) { IR[cause] = true; }
  void clearInterruptRequest(uint8_t cause) { IR[cause] = false; }
  void maskInterrupts() { lastI = csr[STATUS] & FLAG_I; csr[STATUS] |= FLAG_I; }
  void unmaskInterrupts() { if ( !lastI ) csr[STATUS] &= ~FLAG_I; }
  void setIF() { csr[STATUS] |= FLAG_I; }
  void setTrF() { csr[STATUS] |= FLAG_TR; }
  void setTlF() { csr[STATUS] |= FLAG_TL; }
  bool getIF() const { return csr[STATUS] & FLAG_I; }
  bool getTrF() const { return csr[STATUS] & FLAG_TR; }
  bool getTlF() const { return csr[STATUS] & FLAG_TL; }

};









#endif