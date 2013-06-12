/* 
* Authors: Jessica Burroughs and Cristina Formaini
* Assignment: Lab 4
* Section: CPE 315-03
* Date: May 24, 2013
*/

#include "mipsim.hpp"

Stats stats;
Caches caches(0);
static bool jumpflag = false;
static unsigned int pcjump = 0;
static bool usefulBranchDelay = false;
static bool usefulJumpDelay = false;
static bool LUHazard = false;
static unsigned int lwreg = 0;
static bool storeFlag = false;
static unsigned int destReg1 = 0;
static unsigned int destReg2 = 0;

void calcForwards(GenericType rg, RType rt, IType ri);
void check1Reg(unsigned int rs);
void check2Reg(unsigned int rs, unsigned int rt);

unsigned int signExtend16to32ui(short i) {
  return static_cast<unsigned int>(static_cast<int>(i));
}
unsigned int signExtend8to32ui(char i) {
  return static_cast<unsigned int>(static_cast<int>(i));
}
unsigned int zeroExtend8to32ui(unsigned char i) {
  return static_cast<unsigned int>(i);
}
unsigned int zeroExtend16to32ui(unsigned short i) {
  return static_cast<unsigned int>(i);
}


void execute() {
  Data32 instr = imem[pc];
  Data32 temp(0);
  GenericType rg(instr);
  RType rt(instr);
  IType ri(instr);
  JType rj(instr);
  unsigned int pctarget = pc + 4;
  unsigned int addr;
  pc = pctarget;
  
  stats.cycles++;
  if (jumpflag) {
    pc = pcjump;
    jumpflag = false;
  }

  calcForwards(rg, rt, ri);
  
if (instr.operator != (0)) {
  stats.instrs++;
  if (usefulBranchDelay) {
     stats.hasUsefulBranchDelaySlot++;
     usefulBranchDelay = false;
  }
  if (usefulJumpDelay) {
     stats.hasUsefulJumpDelaySlot++;
     usefulJumpDelay = false;
  }
  if(LUHazard)
  {
    if((unsigned int)rt.rs == lwreg || (unsigned int)rt.rt == lwreg || 
      (unsigned int)ri.rs == lwreg || (unsigned int)ri.rt == lwreg)
      stats.loadHasLoadUseHazard++;
    else
      stats.loadHasNoLoadUseHazard++;
    LUHazard = false;
    lwreg = 0;
  }

  switch(rg.op) {
  case OP_SPECIAL:
    switch(rg.func) {
    case SP_ADDU:
      stats.numRType++;
      stats.numRegReads = stats.numRegReads + 2;
      stats.numRegWrites++;
      rf.write(rt.rd, rf[rt.rs] + rf[rt.rt]);
      break;
    case SP_SLL:
      stats.numRegReads++;
      stats.numRegWrites++;
      stats.numRType++;
      rf.write(rt.rd, rf[rt.rt] << rt.sa);
      break;
    case SP_JR:
      usefulJumpDelay = true;
      stats.numRegReads++;
      stats.numRType++;
      pcjump = (unsigned int)rf[rt.rs];
      jumpflag = true;
      break;
    case SP_SLT:
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      if(static_cast<int>(rf[rt.rs]) < static_cast<int>(rf[rt.rt]))
        rf.write(rt.rd, 1);
      else
        rf.write(rt.rd, 0);
      break;
    case SP_SRL:
      stats.numRType++;
      stats.numRegReads++;
      stats.numRegWrites++;
      rf.write(rt.rd, rf[rt.rt] >> rt.sa);
      break;
    case SP_SRA:
      stats.numRType++;
      stats.numRegReads++;
      stats.numRegWrites++;
      rf.write(rt.rd, static_cast<int>(rf[rt.rt]) >> rt.sa);
      break;
    case SP_ADD:
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      rf.write(rt.rd, static_cast<int>(rf[rt.rs]) + static_cast<int>(rf[rt.rt]));
      break;
    case SP_SUB:
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      rf.write(rt.rd, static_cast<int>(rf[rt.rs]) - static_cast<int>(rf[rt.rt]));
      break;
    case SP_SUBU:
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      rf.write(rt.rd, rf[rt.rs] - rf[rt.rt]);
      break;
    case SP_AND:
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      rf.write(rt.rd, rf[rt.rs] & rf[rt.rt]);
      break;
    case SP_OR:
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      rf.write(rt.rd, rf[rt.rs] | rf[rt.rt]);
      break;
    case SP_XOR:
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      rf.write(rt.rd, rf[rt.rs] ^ rf[rt.rt]);
      break;
    case SP_NOR:
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      rf.write(rt.rd, ~(rf[rt.rs] | rf[rt.rt]));
      break;
    case SP_JALR:
      stats.numRType++;
      usefulJumpDelay = true;
      jumpflag = true;
      stats.numRegWrites++;
      stats.numRegReads++;
      rf.write(31, pc + 4);
      pcjump = (unsigned int)rf[rt.rs];
      break;
    default:
      cout << "Unsupported instruction: ";
      instr.printI(instr);
      exit(1);
      break;
    }
    break;
  case OP_ADDIU:
    stats.numRegWrites++;
    stats.numRegReads++;
    stats.numIType++;
    rf.write(ri.rt, rf[ri.rs] + signExtend16to32ui(ri.imm));
    break;
  case OP_SW:
    stats.numRegReads = stats.numRegReads + 2;
    stats.numMemWrites++;
    stats.numIType++;
    storeFlag = true;
    addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
    caches.access(addr);
    dmem.write(addr, rf[ri.rt]);
    break;
  case OP_LW:
    stats.numRegReads++;
    stats.numRegWrites++;
    stats.numMemReads++;
    stats.numIType++;
    LUHazard = true;
    lwreg = (unsigned int)ri.rt;
    addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
    caches.access(addr);
    rf.write(ri.rt, dmem[addr]);
    break;
  case OP_J:
    stats.numJType++;
    usefulJumpDelay = true;
    pcjump = (pc & 0xf0000000) | (rj.target << 2);
    jumpflag = true;
    break;
  case OP_BNE:
    stats.numRegReads = stats.numRegReads + 2;
    stats.numIType++;
    usefulBranchDelay = true;
    if(rf[ri.rs] != rf[ri.rt]) {
      pcjump = pc + (signExtend16to32ui(ri.imm) << 2);
      jumpflag = true;
      if( static_cast<int>(signExtend16to32ui(ri.imm) << 2) < 0) 
        stats.numBackwardBranchesTaken++;
      else 
        stats.numForwardBranchesTaken++;  
    }
    else {
      if(static_cast<int>(signExtend16to32ui(ri.imm) << 2) < 0)
        stats.numBackwardBranchesNotTaken++;
      else
        stats.numForwardBranchesNotTaken++;   
    } 
    break;
  case OP_LUI:
    stats.numRegWrites++;
    stats.numIType++;
    rf.write(ri.rt, signExtend16to32ui(ri.imm) << 16);
    break;
  case OP_BEQ:
    stats.numIType++;
    stats.numRegReads = stats.numRegReads + 2;
    usefulBranchDelay = true;
    if(rf[ri.rs] == rf[ri.rt]) {
      pcjump = pc + (signExtend16to32ui(ri.imm) << 2);
      jumpflag = true;
      if(static_cast<int>(signExtend16to32ui(ri.imm) << 2) < 0) {
        stats.numBackwardBranchesTaken++;
      }
      else {
        stats.numForwardBranchesTaken++;
      }
    }
    else {
      if(static_cast<int>(signExtend16to32ui(ri.imm) << 2) < 0)
        stats.numBackwardBranchesNotTaken++;
      else
        stats.numForwardBranchesNotTaken++;
    }
    break;
  case OP_ORI:
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    rf.write(ri.rt, rf[ri.rs] | signExtend16to32ui(ri.imm));
    break;
  case OP_SB:
    stats.numIType++;
    stats.numRegReads += 2;
    stats.numMemWrites++;
    storeFlag = true;
    addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
    caches.access(addr);
    temp = dmem[addr];
    temp.set_data_ubyte4(0, rf[ri.rt] & 0xff);
    dmem.write(addr, temp);
    break;
  case OP_SLTI:
    stats.numRegWrites++;
    stats.numRegReads++;
    stats.numIType++;
    if(static_cast<int>(rf[ri.rs]) < static_cast<int>(ri.imm))
      rf.write(ri.rt, 1);
    else
      rf.write(ri.rt, 0);
    break;
  case OP_SLTIU:
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    if(rf[ri.rs] < signExtend16to32ui(ri.imm))
      rf.write(ri.rt, 1);
    else
      rf.write(ri.rt, 0);
    break;
  case OP_LBU:
    LUHazard = true;
    lwreg = (unsigned int)ri.rt;
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    stats.numMemReads++;
    addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
    caches.access(addr);
    rf.write(ri.rt, zeroExtend8to32ui((dmem[addr].data_ubyte4(0))));
    break;
  case OP_LB:
    LUHazard = true;
    lwreg = (unsigned int)ri.rt;
    stats.numRegWrites++;
    stats.numRegReads++;
    stats.numIType++;
    stats.numMemReads++;
    addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
    caches.access(addr);
    rf.write(ri.rt, signExtend8to32ui(dmem[addr].data_ubyte4(0)));
    break;
  case OP_BLEZ:
    stats.numIType++;
    usefulBranchDelay = true;
    stats.numRegReads += 2;
    if(static_cast<int>(rf[ri.rs]) <= 0)
    {
      jumpflag = true;
      pcjump = pc + (signExtend16to32ui(ri.imm) << 2);
      if(static_cast<int>(signExtend16to32ui(ri.imm) << 2) < 0)
        stats.numBackwardBranchesTaken++;
      else
        stats.numForwardBranchesTaken++;
    }
    else
    {
      if(static_cast<int>(signExtend16to32ui(ri.imm) << 2) < 0)
        stats.numBackwardBranchesNotTaken++;
      else
        stats.numForwardBranchesNotTaken++;
    }
    break;
  case OP_JAL:
    stats.numRegWrites++;
    stats.numJType++;
    usefulJumpDelay = true;
    rf.write(31, pc + 4);
    pcjump = (pc & 0xf0000000) | (rj.target << 2);
    jumpflag = true;
    break;
  default:
    cout << "Unsupported instruction: ";
    instr.printI(instr);
    exit(1);
    break;
  }
}
else {
  if (usefulJumpDelay) {
     stats.hasUselessJumpDelaySlot++;
     usefulJumpDelay = false;
  }
  if (usefulBranchDelay) {
     stats.hasUselessBranchDelaySlot++;
     usefulBranchDelay = false;
  }
  if (LUHazard) {
    stats.loadHasLoadUseStall++;
    stats.loadHasNoLoadUseHazard++;
    LUHazard = false;
    lwreg = 0;
  }
  stats.numRegReads++;
  stats.numRegWrites++;
}
}

void calcForwards(GenericType rg, RType rt, IType ri) {
  switch(rg.op) {
  case OP_SPECIAL:
    switch(rg.func) {
    case SP_ADDU:
      check2Reg((unsigned int)rt.rs, (unsigned int)rt.rt);
      destReg1 = destReg2;
      destReg2 = (unsigned int)rt.rd;
      break;
    case SP_SLL:
      check1Reg((unsigned int)rt.rt);
      destReg1 = destReg2;
      destReg2 = (unsigned int)rt.rd;
      break;
    case SP_JR:
      check1Reg((unsigned int)rt.rs);
      destReg1 = destReg2;
      destReg2 = 0;
      break;
    case SP_SLT:
        check2Reg((unsigned int)rt.rs, (unsigned int)rt.rt);
        destReg1 = destReg2;
        destReg2 = (unsigned int)rt.rd;
      break;
    case SP_SRL:
        check1Reg((unsigned int)rt.rt);
        destReg1 = destReg2;
      destReg2 = (unsigned int)rt.rd;
      break;
    case SP_SRA:
      check1Reg((unsigned int)rt.rt);
        destReg1 = destReg2;
      destReg2 = (unsigned int)rt.rd;
      break;
    case SP_ADD:
      check2Reg((unsigned int)rt.rs, (unsigned int)rt.rt);
        destReg1 = destReg2;
      destReg2 = (unsigned int)rt.rd;
      break;
    case SP_SUB:
      check2Reg((unsigned int)rt.rs, (unsigned int)rt.rt);
        destReg1 = destReg2;
      destReg2 = (unsigned int)rt.rd;
      break;
    case SP_SUBU:
      check2Reg((unsigned int)rt.rs, (unsigned int)rt.rt);
        destReg1 = destReg2;
      destReg2 = (unsigned int)rt.rd;
      break;
    case SP_AND:
      check2Reg((unsigned int)rt.rs, (unsigned int)rt.rt);
        destReg1 = destReg2;
      destReg2 = (unsigned int)rt.rd;
      break;
    case SP_OR:
      check2Reg((unsigned int)rt.rs, (unsigned int)rt.rt);
        destReg1 = destReg2;
      destReg2 = (unsigned int)rt.rd;
      break;
    case SP_XOR:
      check2Reg((unsigned int)rt.rs, (unsigned int)rt.rt);
        destReg1 = destReg2;
      destReg2 = (unsigned int)rt.rd;
      break;
    case SP_NOR:
      check2Reg((unsigned int)rt.rs, (unsigned int)rt.rt);
        destReg1 = destReg2;
      destReg2 = (unsigned int)rt.rd;
      break;
    case SP_JALR:
      check1Reg((unsigned int)rt.rs);
        destReg1 = destReg2;
      destReg2 = (unsigned int)31;
      break;
    default:
      exit(1);
      break;
    }
    break;
  case OP_ADDIU:
    check1Reg((unsigned int)ri.rs);
        destReg1 = destReg2;
      destReg2 = (unsigned int)ri.rt;
      break;
  case OP_SW:
    check2Reg((unsigned int)ri.rs, (unsigned int)ri.rt);
        destReg1 = destReg2;
      destReg2 = 0;
      break;
  case OP_LW:
    check1Reg((unsigned int)ri.rs);
        destReg1 = destReg2;
      destReg2 = (unsigned int)ri.rt;
      break;
  case OP_J:
    destReg1 = destReg2;
    destReg2 = 0;
    break;
  case OP_BNE:
    check2Reg((unsigned int)ri.rs, (unsigned int)ri.rt);
        destReg1 = destReg2;
    destReg2 = 0;
    break;
  case OP_LUI:
   destReg1 = destReg2;
    destReg2 = (unsigned int)ri.rt;    
    break;
  case OP_BEQ:
    check2Reg((unsigned int)ri.rs, (unsigned int)ri.rt);
        destReg1 = destReg2;
    destReg2 = 0;
    break;
  case OP_ORI:
    check1Reg((unsigned int)ri.rs);
        destReg1 = destReg2;
    destReg2 = (unsigned int)ri.rt;
    break;
  case OP_SB:
    check2Reg((unsigned int)ri.rs, (unsigned int)ri.rt);
        destReg1 = destReg2;
    destReg2 = 0;
    break;
  case OP_SLTI:
    check1Reg((unsigned int)ri.rs);
        destReg1 = destReg2;
    destReg2 = (unsigned int)ri.rt;
    break;
  case OP_SLTIU:
    check1Reg((unsigned int)ri.rs);
        destReg1 = destReg2;
    destReg2 = (unsigned int)ri.rt;
    break;
  case OP_LBU:
    check1Reg((unsigned int)ri.rs);
        destReg1 = destReg2;
    destReg2 = (unsigned int)ri.rt;
    break;
  case OP_LB:
    check1Reg((unsigned int)ri.rs);
        destReg1 = destReg2;
    destReg2 = (unsigned int)ri.rt;
    break;
  case OP_BLEZ:
    check1Reg((unsigned int)ri.rs);
        destReg1 = destReg2;
    destReg2 = 0;
    break;
  case OP_JAL:
    destReg1 = destReg2;
    destReg2 = (unsigned int)31;
    break;
  default:
    exit(1);
    break;
  }
}

void check1Reg(unsigned int rs) {
  if (destReg2 && destReg2 == rs)
    stats.numForwardsExStage++;
  if (destReg1 && destReg2 != destReg1 && destReg1 == rs)
    stats.numForwardsMemStage++;
}

void check2Reg(unsigned int rs, unsigned int rt) {
  if (destReg2 && (destReg2 == rs || destReg2 == rt))
    stats.numForwardsExStage++;
  if (destReg1 && destReg1 != destReg2 && (destReg1 == rs || destReg1 == rt))
    stats.numForwardsMemStage++;
}
