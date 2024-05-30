#ifndef EMULATOR_H
#define EMULATOR_H

#include <string>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <thread>
#include <chrono>
#include "../elf/Elf32File.hpp"
#include "ComputerSystem.hpp"

// OC | MOD | REGA | REGB | REGC | DISP | DISP | DISP

#define START_ADDR        0x40000000

#define NIBBLE_MASK       0xf
#define BYTE_MASK         0xff
#define DISP_MASK         0x00000fff
#define DISP_SIGN_MASK    0x00000800
#define DISP_EXTEND_MASK  0xfffff000

#define OCMOD_OFFSET      24
#define REG_A_OFFSET      20
#define REG_B_OFFSET      16
#define REG_C_OFFSET      12


class Emulator {

  std::string file_name;

  Memory memory;
  CPU cpu;
  bool on;

  // Terminal
  termios old_attr;
  int old_flags;
  bool out_flag = false;

  static uint32_t timer_periods[];
  std::thread* timer_thread = nullptr;

  void loadMemory();
  void runCPU();
  void printCPUState();
  void setUpTerminal();
  void restoreTerminal();
  void startTimer();
  static void timerBody(Memory& memory, CPU& cpu);

  bool isOn() { return on; };

  uint32_t fetchInstruction() { 
    uint32_t instr =  memory.readWord(cpu.gpr[PC]);
    cpu.gpr[PC] += 4;
    return instr;
  };

  void pushWord(uint32_t val);
  uint32_t popWord();

  void handleInterrupt(uint8_t cause);
  void handleTerminal();

protected:

  Emulator() {};

  static Emulator* emulator;

public:

  Emulator(Emulator&) = delete;
  void operator=(const Emulator&) = delete;
  static Emulator* getInstance();

  void setFileName(std::string name) { file_name = name; };
  void startEmulating();

  static uint32_t extractDisplacement(uint32_t instr) {
    uint32_t disp = instr & DISP_MASK;
    if ( disp & DISP_SIGN_MASK ) disp |= DISP_EXTEND_MASK;
    return disp;
  }

  static uint8_t extractOcMod(uint32_t instr) {
    return (instr >> OCMOD_OFFSET) & BYTE_MASK;
  }

  static uint8_t extractRegA(uint32_t instr) {
    return (instr >> REG_A_OFFSET) & NIBBLE_MASK;
  }

  static uint8_t extractRegB(uint32_t instr) {
    return (instr >> REG_B_OFFSET) & NIBBLE_MASK;
  }

  static uint8_t extractRegC(uint32_t instr) {
    return (instr >> REG_C_OFFSET) & NIBBLE_MASK;
  }

  void printHex(std::ostream& os, uint32_t number, int width, bool prefix = false) {
      std::ios old_state(nullptr);
      old_state.copyfmt(os);

      if ( prefix ) os << std::showbase;
      os << std::hex << std::internal << std::setfill('0');

      // We do this because showbase wont work if number is 0
      if ( prefix && number == 0 ) os << "0x" << std::setw(width - 2) << "";
      else os << std::setw(width) << number;

      os.copyfmt(old_state);
  }
};

#endif