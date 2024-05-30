#include "Emulator.hpp"

Emulator* Emulator::emulator = nullptr;
uint32_t Emulator::timer_periods[8] = {500, 1000, 1500, 2000, 5000, 10000, 30000, 60000};

Emulator* Emulator::getInstance() {
  if ( emulator == nullptr ) {
    emulator = new Emulator();
  }

  return emulator;
}

void Emulator::loadMemory() {

  Elf32File file(file_name, 0, true);
  file.readFromFile();

  if ( file.getType() != ET_EXEC ) {
    // Error, non executable file
    std::cout << "emulator: error : file '" + file_name + "' is not executable" << std::endl;
    exit(-1);
  }

  for ( int i = 0; i < file.getNumberOfSegments(); i++) {
    Elf32_Phdr* header = file.getSegmentHeader(i);
    std::vector<uint8_t>& contents_ref = *file.getSegmentContents(i);

    uint32_t base_addr = header->p_vaddr;

    for ( int j = 0; j < contents_ref.size(); j++) {
      memory.write(base_addr + j, contents_ref[j]);
    }
  }

}

void Emulator::setUpTerminal() {
  termios new_attr;

  // Save old attributes so we can restore them at the end
  tcgetattr(STDIN_FILENO, &old_attr);
  tcgetattr(STDIN_FILENO, &new_attr);

  // Disable echo and canonical mode(so it doesn't wait for line-delimeter char)
  new_attr.c_lflag &= ~(ECHO | ICANON);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_attr);

  // Make it so read instruction is non blocking
  old_flags = fcntl(STDIN_FILENO, F_GETFL);
  fcntl(STDIN_FILENO, F_SETFL, old_flags | O_NONBLOCK);
}


void Emulator::restoreTerminal() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_attr);
  //fcntl(STDIN_FILENO, old_flags);
}

void Emulator::startTimer() {
  timer_thread = new std::thread(Emulator::timerBody, std::ref(this->memory), std::ref(this->cpu));
}

void Emulator::timerBody(Memory& memory, CPU& cpu) {
  uint32_t current_period;
  Emulator* emulator = Emulator::getInstance();

  while(emulator->isOn()) {
    current_period = timer_periods[memory.readMMReg(TIM_CFG)];

    std::this_thread::sleep_for(std::chrono::milliseconds(current_period));

    cpu.IR[TIM] = true;
  }
}

void Emulator::startEmulating() {
  loadMemory();
  setUpTerminal();
  runCPU();
  printCPUState();
  restoreTerminal();
}


void Emulator::printCPUState() {
  std::cout << "\n-----------------------------------------------------------------\n"
            << "Emulated processor executed halt instruction\n"
            << "Emulated processor state:\n"
            << std::right;

  for ( int i = 0; i < GPR_CNT; i++) {
    std::string reg = "r" + std::to_string(i);
    std::cout << std::setw(3) << reg << "=";
    printHex(std::cout, cpu.gpr[i], 10, true);

    if ( i % 4 != 3 ) std::cout << std::setw(3) << "";
    else std::cout << '\n';
  }

  std::cout << std::endl;
}

void Emulator::pushWord(uint32_t val) {
  cpu.gpr[SP] -= 4;
  memory.writeWord(cpu.gpr[SP], val);
}

uint32_t Emulator::popWord() {
  uint32_t val = memory.readWord(cpu.gpr[SP]);
  cpu.gpr[SP] -= 4;
  return val;
}

void Emulator::handleInterrupt(uint8_t cause) {
  cpu.csr[CAUSE] = cause + 1;
  cpu.clearInterruptRequest(cause);

  pushWord(cpu.csr[STATUS]);
  pushWord(cpu.gpr[PC]);

  cpu.maskInterrupts();

  cpu.gpr[PC] = cpu.csr[HANDLER];
}

void Emulator::handleTerminal() {
  
  // Write character to console if term_in register was written into
  if ( out_flag ) {
    out_flag = false;

    char out_c = memory.readMMReg(TERM_OUT);
    write(STDOUT_FILENO, &out_c, 1);
  }

  // Try to read a character from terminal
  // If successfull, write it to term_in and set terminal interrupt request bit

  char in_c;
  if ( read(STDIN_FILENO, &in_c, 1) > 0 ) {
    memory.writeMMReg(TERM_IN, in_c);
    cpu.IR[TERM] = true;
  }

}

void Emulator::runCPU() {
  bool running = true;

  cpu.gpr[PC] = START_ADDR;
  memory.writeMMReg(TIM_CFG, 0);
  on = true;

  while(running) {
    //printCPUState();
    uint32_t instr = fetchInstruction();
    uint8_t oc_mod = extractOcMod(instr);
    uint8_t reg_A = extractRegA(instr);
    uint8_t reg_B = extractRegB(instr);
    uint8_t reg_C = extractRegC(instr);
    uint32_t disp = extractDisplacement(instr);

    switch (oc_mod) {
      case 0x00: {    // halt
        running = false;
        break;
      }
      case 0x10: {    // int
        cpu.setInterruptRequest(INT);
        break;
      }
      case 0x20: {    // call instructions
        pushWord(cpu.gpr[PC]);
        cpu.gpr[PC] = cpu.gpr[reg_A] + cpu.gpr[reg_B] + disp;
        break;  
      }
      case 0x21: {    
        pushWord(cpu.gpr[PC]);
        cpu.gpr[PC] = memory.readWord(cpu.gpr[reg_A] + cpu.gpr[reg_B] + disp);
        break;  
      }
      case 0x30: {    // jump instructions
        cpu.gpr[PC] = cpu.gpr[reg_A] + disp;
        break;
      }
      case 0x31: {   
        if ( cpu.gpr[reg_B] == cpu.gpr[reg_C] ) cpu.gpr[PC] = cpu.gpr[reg_A] + disp;
        break;
      }
      case 0x32: {    
        if ( cpu.gpr[reg_B] != cpu.gpr[reg_C] ) cpu.gpr[PC] = cpu.gpr[reg_A] + disp;
        break;
      }
      case 0x33: {   
        if ( (int32_t)cpu.gpr[reg_B] > (int32_t)cpu.gpr[reg_C] ) cpu.gpr[PC] = cpu.gpr[reg_A] + disp;
        break;
      }
      case 0x38: {    
        cpu.gpr[PC] = memory.readWord(cpu.gpr[reg_A] + disp);
        break;
      }
      case 0x39: {   
        if ( cpu.gpr[reg_B] == cpu.gpr[reg_C] ) memory.readWord(cpu.gpr[PC] = cpu.gpr[reg_A] + disp);
        break;
      }
      case 0x3a: {    
        if ( cpu.gpr[reg_B] != cpu.gpr[reg_C] ) memory.readWord(cpu.gpr[PC] = cpu.gpr[reg_A] + disp);
        break;
      }
      case 0x3b: {   
        if ( (int32_t)cpu.gpr[reg_B] > (int32_t)cpu.gpr[reg_C] ) memory.readWord(cpu.gpr[PC] = cpu.gpr[reg_A] + disp);
        break;
      }
      case 0x40: {  // xchng
        uint32_t temp = cpu.gpr[reg_B];
        cpu.gpr[reg_B] = cpu.gpr[reg_C];
        cpu.gpr[reg_C] = temp;
        break;
      }
      case 0x50: {  // arithmetic instruction
        cpu.gpr[reg_A] = (int32_t)cpu.gpr[reg_B] + (int32_t)cpu.gpr[reg_C];
        break;
      }
      case 0x51: { 
        cpu.gpr[reg_A] = (int32_t)cpu.gpr[reg_B] - (int32_t)cpu.gpr[reg_C];
        break;
      }
      case 0x52: { 
        cpu.gpr[reg_A] = (int32_t)cpu.gpr[reg_B] * (int32_t)cpu.gpr[reg_C];
        break;
      }
      case 0x53: { 
        cpu.gpr[reg_A] = (int32_t)cpu.gpr[reg_B] / (int32_t)cpu.gpr[reg_C];
        break;
      }
      case 0x60: {  // logical instructions
        cpu.gpr[reg_A] = ~cpu.gpr[reg_B];
        break;
      }
      case 0x61: { 
        cpu.gpr[reg_A] = cpu.gpr[reg_B] & cpu.gpr[reg_C];
        break;
      }
      case 0x62: { 
        cpu.gpr[reg_A] = cpu.gpr[reg_B] | cpu.gpr[reg_C];
        break;
      }
      case 0x63: { 
        cpu.gpr[reg_A] = cpu.gpr[reg_B] ^ cpu.gpr[reg_C];
        break;
      }
      case 0x70: {  // shift instructions
        cpu.gpr[reg_A] = cpu.gpr[reg_B] << cpu.gpr[reg_C];
        break;
      }
      case 0x71: {
        cpu.gpr[reg_A] = cpu.gpr[reg_B] >> cpu.gpr[reg_C];
        break;
      }
      case 0x80: {  // store instructions
        uint32_t addr = cpu.gpr[reg_A] + cpu.gpr[reg_B] + disp;
        memory.writeWord(addr, cpu.gpr[reg_C]);
        if ( addr == MM_REGS_BASE + TERM_OUT ) out_flag = true;
        break;
      }
      case 0x82: {  
        uint32_t addr = memory.readWord(cpu.gpr[reg_A] + cpu.gpr[reg_B] + disp);
        memory.writeWord(addr, cpu.gpr[reg_C]);
        if ( addr == MM_REGS_BASE + TERM_OUT ) out_flag = true;
        break;
      }
      case 0x81: {
        cpu.gpr[reg_A] += disp;
        uint32_t addr = cpu.gpr[reg_A];
        memory.writeWord(addr, cpu.gpr[reg_C]);
        if ( addr == MM_REGS_BASE + TERM_OUT ) out_flag = true;
        break;
      }     
      case 0x90: {  // load instructions
        cpu.gpr[reg_A] = cpu.csr[reg_B];
        break;
      }
      case 0x91: {
        cpu.gpr[reg_A] = cpu.gpr[reg_B] + disp;
        break;
      }
      case 0x92: {
        cpu.gpr[reg_A] = memory.readWord(cpu.gpr[reg_B] + cpu.gpr[reg_C] + disp);
        break;
      }
      case 0x93: {
        uint32_t old_pc = cpu.gpr[PC];  // if IRET next operation will change PC, so we need to save it
        cpu.gpr[reg_A] = memory.readWord(cpu.gpr[reg_B]);
        cpu.gpr[reg_B] += disp;
        // We have to check if this instruction is part of IRET 
        if ( instr == 0x93FE0004 ) {
          // We feth instruction that should have came after this instruction(before pc was changed)
          uint32_t next_instr = memory.readWord(old_pc);
          if ( next_instr == 0x970E0004 ) {
            // It is part of IRET, so we have to execute this instruction as well, since IRET has to be executed as an atomic instruction
            cpu.csr[extractRegA(next_instr)] = memory.readWord(cpu.gpr[extractRegB(next_instr)]);
            cpu.gpr[extractRegB(next_instr)] += extractDisplacement(next_instr);
          }
        }
        break;
      }
      case 0x94: {  
        cpu.csr[reg_A] = cpu.gpr[reg_B];
        break;
      }
      case 0x95: {  
        cpu.csr[reg_A] = cpu.csr[reg_B] | disp;
        break;
      }
      case 0x96: {  
        cpu.csr[reg_A] = memory.readWord(cpu.gpr[reg_B] + cpu.gpr[reg_C] + disp);
        break;
      }
      case 0x97: {
        cpu.csr[reg_A] = memory.readWord(cpu.gpr[reg_B]);
        cpu.gpr[reg_B] += disp;
        break;
      }
      default: {
        cpu.setInterruptRequest(INV); // Invalid instruction
      }
    }

    /*
      LD LITERAL/SYMBOL, RX		0x92XF0DDD
												      0x92XX0000
     
      for now, i wont be checking for this, so its possible for interrupt to come after first isntruction
      and that way, make this LD non atomic
      even if that happens, registers shouldn't be changed in interrupt handler, so it will continue where it left off after returning from handler
    */


    // Start timer only after handler address has been set
    if ( !timer_thread && cpu.csr[HANDLER] != 0 ) {
      startTimer();
    }

    handleTerminal();

    if ( cpu.IR[INT] ) {
      handleInterrupt(INT);
    } else if ( cpu.IR[INV] ) {
      handleInterrupt(INV);
    } else if ( cpu.IR[TERM] && !(cpu.csr[STATUS] & FLAG_I) && !(cpu.csr[STATUS] & FLAG_TL) ) {
      handleInterrupt(TERM);
    } else if ( cpu.IR[TIM] && !(cpu.csr[STATUS] & FLAG_I) && !(cpu.csr[STATUS] & FLAG_TR) ) {
      handleInterrupt(TIM);
    }


  }

  on = false;
  timer_thread->join();
}