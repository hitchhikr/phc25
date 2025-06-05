/*
	'CP/M Player for Win32'
	
	Origin : MAME Z80 Core
	Author : Takeda.Toshiya
	Date   : 2004.03.05 -
	
	[ CPU Core (Z-80) ]
*/

#include "z80.h"
#include "../win32/monitor.h"

Z80::Z80()
{
	// Initialization
	AF = BC = DE = HL = IX = IY = 0;
	_I = _R = 0;
	exAF = exBC = exDE = exHL = 0;
	PC = START_ADDR;
	SP = STACK_ADDR - 1;
	IM = IFF1 = IFF2 = HALT = 0;
	count = 0;
}

Z80::~Z80()
{
	// Cleaning up
}

void Z80::Reset()
{
	// CP/M Reset
	PC = 0;
	SP = STACK_ADDR - 1;
	PUSH( STACK_ADDR );
	_I = _R = 0;
	AF = 0xffff;
	IM = IFF1 = IFF2 = HALT = 0;
}

void Z80::DoIRQ(uint8 vector)
{
	// Exit when in HALT state
	if(HALT) {
		PC++;
		HALT = 0;
	}
	
	// IRQ execution
	if(IM == 0) {
		// MODE 0 (only supports RST command)
		PUSH(PC);
		switch(vector) {
			case 0xc7:		// RST 00H
				PC = 0x0000;
				break;
			case 0xcf:		// RST 08H
				PC = 0x0008;
				break;
			case 0xd7:		// RST 10H
				PC = 0x0010;
				break;
			case 0xdf:		// RST 18H
				PC = 0x0018;
				break;
			case 0xe7:		// RST 20H
				PC = 0x0020;
				break;
			case 0xef:		// RST 28H
				PC = 0x0028;
				break;
			case 0xf7:		// RST 30H
				PC = 0x0030;
				break;
			case 0xff:		// RST 38H
				PC = 0x0038;
				break;
		}
		count -= 7;
	}
	else if(IM == 1) {
		// MODE 1
		PUSH(PC);
		PC = 0x0038;
		count -= 7;
	}
	else {
		// MODE 2
		PUSH(PC);
		PC = ReadMemory16((_I << 8) | vector);
		count -= 7;
	}
	IFF1 = IFF2 = 0;
}

void Z80::DoNMI()
{
	// NMI occurrence
	if(HALT) {
		PC++;
		HALT = 0;
	}
	PUSH(PC);
	PC = 0x0066;
	count -= 5;
	IFF1 = 0;
}

int Z80::Run()
{
    int i;
	// Execute only one instruction
	count = 0;
	// Processing IFF1
	if(IFF1 > 1)
		if(--IFF1 == 1) InterruptEnabled();
	if(!HALT)
    {
        // Check the breakpoints
        if(Monitor_Is_Opened() && !Z80_Step)
        {
            for(i = 0; i < 8; i++)
            {
                if(Breakpoints[i] != -1)
                {
                    if(PC == Breakpoints[i])
                    {
                        Z80_Pause = TRUE;
                        Print_Registers_From_Outside(TRUE);
                        return 0;
                    }
                }
            }
        }
		execute_op();
    }
	if(!Z80_Pause && Z80_Step)
    {
        // Return to pause
        Z80_Pause = TRUE;
        Print_Registers_From_Outside(FALSE);
    }
	// Returns the number of clock cycles consumed
	return -count;
}

void Z80::execute_op()
{
	prvPC = PC;
	
	uint8 cmd = Fetch();
	count -= cc_op[cmd];
	_R = (_R & 0x80) | ((_R + 1) & 0x7f);
	
	switch(cmd)
	{
		case 0x00: // NOP
			break;
		case 0x01: // LD BC, w
			BC = Fetch16();
			break;
		case 0x02: // LD (BC), A
			WriteMemory(BC, _A);
			break;
		case 0x03: // INC BC
			BC++;
			break;
		case 0x04: // INC B
			_B = INC(_B);
			break;
		case 0x05: // DEC B
			_B = DEC(_B);
			break;
		case 0x06: // LD B, n
			_B = Fetch();
			break;
		case 0x07: // RLCA
			RLCA();
			break;
		case 0x08: // EX AF, AF'
			EX_AF();
			break;
		case 0x09: // ADD HL, BC
			HL = ADD16(HL, BC);
			break;
		case 0x0a: // LD A, (BC)
			_A = ReadMemory(BC);
			break;
		case 0x0b: // DEC BC
			BC--;
			break;
		case 0x0c: // INC C
			_C = INC(_C);
			break;
		case 0x0d: // DEC C
			_C = DEC(_C);
			break;
		case 0x0e: // LD C, n
			_C = Fetch();
			break;
		case 0x0f: // RRCA
			RRCA();
			break;
		case 0x10: // DJNZ o
			_B--;
			JR_COND(_B, 0x10);
			break;
		case 0x11: // LD DE, w
			DE = Fetch16();
			break;
		case 0x12: // LD (DE), A
			WriteMemory(DE, _A);
			break;
		case 0x13: // INC DE
			DE++;
			break;
		case 0x14: // INC D
			_D = INC(_D);
			break;
		case 0x15: // DEC D
			_D = DEC(_D);
			break;
		case 0x16: // LD D, n
			_D = Fetch();
			break;
		case 0x17: // RLA
			RLA();
			break;
		case 0x18: // JR o
			JR();
			break;
		case 0x19: // ADD HL, DE
			HL = ADD16(HL, DE);
			break;
		case 0x1a: // LD A, (DE)
			_A = ReadMemory(DE);
			break;
		case 0x1b: // DEC DE
			DE--;
			break;
		case 0x1c: // INC E
			_E = INC(_E);
			break;
		case 0x1d: // DEC E
			_E = DEC(_E);
			break;
		case 0x1e: // LD E, n
			_E = Fetch();
			break;
		case 0x1f: // RRA
			RRA();
			break;
		case 0x20: // JR NZ, o
			JR_COND(!(_F & ZF), 0x20);
			break;
		case 0x21: // LD HL, w
			HL = Fetch16();
			break;
		case 0x22: // LD (w), HL
			EA = Fetch16();
			WriteMemory16(EA, HL);
			break;
		case 0x23: // INC HL
			HL++;
			break;
		case 0x24: // INC H
			_H = INC(_H);
			break;
		case 0x25: // DEC H
			_H = DEC(_H);
			break;
		case 0x26: // LD H, n
			_H = Fetch();
			break;
		case 0x27: // DAA
			DAA();
			break;
		case 0x28: // JR Z, o
			JR_COND(_F & ZF, 0x28);
			break;
		case 0x29: // ADD HL, HL
			HL = ADD16(HL, HL);
			break;
		case 0x2a: // LD HL, (w)
			EA = Fetch16();
			HL = ReadMemory16(EA);
			break;
		case 0x2b: // DEC HL
			HL--;
			break;
		case 0x2c: // INC L
			_L = INC(_L);
			break;
		case 0x2d: // DEC L
			_L = DEC(_L);
			break;
		case 0x2e: // LD L, n
			_L = Fetch();
			break;
		case 0x2f: // CPL
			_A ^= 0xff;
			_F = (_F & (SF | ZF | PF | CF)) | HF | NF | (_A & (YF | XF));
			break;
		case 0x30: // JR NC, o
			JR_COND(!(_F & CF), 0x30);
			break;
		case 0x31: // LD SP, w
			SP = Fetch16();
			break;
		case 0x32: // LD (w), A
			EA = Fetch16();
			WriteMemory(EA, _A);
			break;
		case 0x33: // INC SP
			SP++;
			break;
		case 0x34: // INC (HL)
			WriteMemory(HL, INC(ReadMemory(HL)));
			break;
		case 0x35: // DEC (HL)
			WriteMemory(HL, DEC(ReadMemory(HL)));
			break;
		case 0x36: // LD (HL), n
			WriteMemory(HL, Fetch());
			break;
		case 0x37: // SCF
			_F = (_F & (SF | ZF | PF)) | CF | (_A & (YF | XF));
			break;
		case 0x38: // JR C, o
			JR_COND(_F & CF, 0x38);
			break;
		case 0x39: // ADD HL, SP
			HL = ADD16(HL, SP);
			break;
		case 0x3a: // LD A, (w)
			EA = Fetch16();
			_A = ReadMemory(EA);
			break;
		case 0x3b: // DEC SP
			SP--;
			break;
		case 0x3c: // INC A
			_A = INC(_A);
			break;
		case 0x3d: // DEC A
			_A = DEC(_A);
			break;
		case 0x3e: // LD A, n
			_A = Fetch();
			break;
		case 0x3f: // CCF
			_F = ((_F & (SF | ZF | PF | CF)) | ((_F & CF) << 4) | (_A & (YF | XF))) ^ CF;
			break;
		case 0x40: // LD B, B
			break;
		case 0x41: // LD B, C
			_B = _C;
			break;
		case 0x42: // LD B, D
			_B = _D;
			break;
		case 0x43: // LD B, E
			_B = _E;
			break;
		case 0x44: // LD B, H
			_B = _H;
			break;
		case 0x45: // LD B, L
			_B = _L;
			break;
		case 0x46: // LD B, (HL)
			_B = ReadMemory(HL);
			break;
		case 0x47: // LD B, A
			_B = _A;
			break;
		case 0x48: // LD C, B
			_C = _B;
			break;
		case 0x49: // LD C, C
			break;
		case 0x4a: // LD C, D
			_C = _D;
			break;
		case 0x4b: // LD C, E
			_C = _E;
			break;
		case 0x4c: // LD C, H
			_C = _H;
			break;
		case 0x4d: // LD C, L
			_C = _L;
			break;
		case 0x4e: // LD C, (HL)
			_C = ReadMemory(HL);
			break;
		case 0x4f: // LD C, A
			_C = _A;
			break;
		case 0x50: // LD D, B
			_D = _B;
			break;
		case 0x51: // LD D, C
			_D = _C;
			break;
		case 0x52: // LD D, D
			break;
		case 0x53: // LD D, E
			_D = _E;
			break;
		case 0x54: // LD D, H
			_D = _H;
			break;
		case 0x55: // LD D, L
			_D = _L;
			break;
		case 0x56: // LD D, (HL)
			_D = ReadMemory(HL);
			break;
		case 0x57: // LD D, A
			_D = _A;
			break;
		case 0x58: // LD E, B
			_E = _B;
			break;
		case 0x59: // LD E, C
			_E = _C;
			break;
		case 0x5a: // LD E, D
			_E = _D;
			break;
		case 0x5b: // LD E, E
			break;
		case 0x5c: // LD E, H
			_E = _H;
			break;
		case 0x5d: // LD E, L
			_E = _L;
			break;
		case 0x5e: // LD E, (HL)
			_E = ReadMemory(HL);
			break;
		case 0x5f: // LD E, A
			_E = _A;
			break;
		case 0x60: // LD H, B
			_H = _B;
			break;
		case 0x61: // LD H, C
			_H = _C;
			break;
		case 0x62: // LD H, D
			_H = _D;
			break;
		case 0x63: // LD H, E
			_H = _E;
			break;
		case 0x64: // LD H, H
			break;
		case 0x65: // LD H, L
			_H = _L;
			break;
		case 0x66: // LD H, (HL)
			_H = ReadMemory(HL);
			break;
		case 0x67: // LD H, A
			_H = _A;
			break;
		case 0x68: // LD L, B
			_L = _B;
			break;
		case 0x69: // LD L, C
			_L = _C;
			break;
		case 0x6a: // LD L, D
			_L = _D;
			break;
		case 0x6b: // LD L, E
			_L = _E;
			break;
		case 0x6c: // LD L, H
			_L = _H;
			break;
		case 0x6d: // LD L, L
			break;
		case 0x6e: // LD L, (HL)
			_L = ReadMemory(HL);
			break;
		case 0x6f: // LD L, A
			_L = _A;
			break;
		case 0x70: // LD (HL), B
			WriteMemory(HL, _B);
			break;
		case 0x71: // LD (HL), C
			WriteMemory(HL, _C);
			break;
		case 0x72: // LD (HL), D
			WriteMemory(HL, _D);
			break;
		case 0x73: // LD (HL), E
			WriteMemory(HL, _E);
			break;
		case 0x74: // LD (HL), H
			WriteMemory(HL, _H);
			break;
		case 0x75: // LD (HL), L
			WriteMemory(HL, _L);
			break;
		case 0x76: // HALT
			PC--;
			HALT = 1;
			break;
		case 0x77: // LD (HL), A
			WriteMemory(HL, _A);
			break;
		case 0x78: // LD A, B
			_A = _B;
			break;
		case 0x79: // LD A, C
			_A = _C;
			break;
		case 0x7a: // LD A, D
			_A = _D;
			break;
		case 0x7b: // LD A, E
			_A = _E;
			break;
		case 0x7c: // LD A, H
			_A = _H;
			break;
		case 0x7d: // LD A, L
			_A = _L;
			break;
		case 0x7e: // LD A, (HL)
			_A = ReadMemory(HL);
			break;
		case 0x7f: // LD A, A
			break;
		case 0x80: // ADD A, B
			ADD(_B);
			break;
		case 0x81: // ADD A, C
			ADD(_C);
			break;
		case 0x82: // ADD A, D
			ADD(_D);
			break;
		case 0x83: // ADD A, E
			ADD(_E);
			break;
		case 0x84: // ADD A, H
			ADD(_H);
			break;
		case 0x85: // ADD A, L
			ADD(_L);
			break;
		case 0x86: // ADD A, (HL)
			ADD(ReadMemory(HL));
			break;
		case 0x87: // ADD A, A
			ADD(_A);
			break;
		case 0x88: // ADC A, B
			ADC(_B);
			break;
		case 0x89: // ADC A, C
			ADC(_C);
			break;
		case 0x8a: // ADC A, D
			ADC(_D);
			break;
		case 0x8b: // ADC A, E
			ADC(_E);
			break;
		case 0x8c: // ADC A, H
			ADC(_H);
			break;
		case 0x8d: // ADC A, L
			ADC(_L);
			break;
		case 0x8e: // ADC A, (HL)
			ADC(ReadMemory(HL));
			break;
		case 0x8f: // ADC A, A
			ADC(_A);
			break;
		case 0x90: // SUB B
			SUB(_B);
			break;
		case 0x91: // SUB C
			SUB(_C);
			break;
		case 0x92: // SUB D
			SUB(_D);
			break;
		case 0x93: // SUB E
			SUB(_E);
			break;
		case 0x94: // SUB H
			SUB(_H);
			break;
		case 0x95: // SUB L
			SUB(_L);
			break;
		case 0x96: // SUB (HL)
			SUB(ReadMemory(HL));
			break;
		case 0x97: // SUB A
			SUB(_A);
			break;
		case 0x98: // SBC A, B
			SBC(_B);
			break;
		case 0x99: // SBC A, C
			SBC(_C);
			break;
		case 0x9a: // SBC A, D
			SBC(_D);
			break;
		case 0x9b: // SBC A, E
			SBC(_E);
			break;
		case 0x9c: // SBC A, H
			SBC(_H);
			break;
		case 0x9d: // SBC A, L
			SBC(_L);
			break;
		case 0x9e: // SBC A, (HL)
			SBC(ReadMemory(HL));
			break;
		case 0x9f: // SBC A, A
			SBC(_A);
			break;
		case 0xa0: // AND B
			AND(_B);
			break;
		case 0xa1: // AND C
			AND(_C);
			break;
		case 0xa2: // AND D
			AND(_D);
			break;
		case 0xa3: // AND E
			AND(_E);
			break;
		case 0xa4: // AND H
			AND(_H);
			break;
		case 0xa5: // AND L
			AND(_L);
			break;
		case 0xa6: // AND (HL)
			AND(ReadMemory(HL));
			break;
		case 0xa7: // AND A
			AND(_A);
			break;
		case 0xa8: // XOR B
			XOR(_B);
			break;
		case 0xa9: // XOR C
			XOR(_C);
			break;
		case 0xaa: // XOR D
			XOR(_D);
			break;
		case 0xab: // XOR E
			XOR(_E);
			break;
		case 0xac: // XOR H
			XOR(_H);
			break;
		case 0xad: // XOR L
			XOR(_L);
			break;
		case 0xae: // XOR (HL)
			XOR(ReadMemory(HL));
			break;
		case 0xaf: // XOR A
			XOR(_A);
			break;
		case 0xb0: // OR B
			OR(_B);
			break;
		case 0xb1: // OR C
			OR(_C);
			break;
		case 0xb2: // OR D
			OR(_D);
			break;
		case 0xb3: // OR E
			OR(_E);
			break;
		case 0xb4: // OR H
			OR(_H);
			break;
		case 0xb5: // OR L
			OR(_L);
			break;
		case 0xb6: // OR (HL)
			OR(ReadMemory(HL));
			break;
		case 0xb7: // OR A
			OR(_A);
			break;
		case 0xb8: // CP B
			CP(_B);
			break;
		case 0xb9: // CP C
			CP(_C);
			break;
		case 0xba: // CP D
			CP(_D);
			break;
		case 0xbb: // CP E
			CP(_E);
			break;
		case 0xbc: // CP H
			CP(_H);
			break;
		case 0xbd: // CP L
			CP(_L);
			break;
		case 0xbe: // CP (HL)
			CP(ReadMemory(HL));
			break;
		case 0xbf: // CP A
			CP(_A);
			break;
		case 0xc0: // RET NZ
			RET_COND(!(_F & ZF), 0xc0);
			break;
		case 0xc1: // POP BC
			BC = POP();
			break;
		case 0xc2: // JP NZ, a
			JP_COND(!(_F & ZF));
			break;
		case 0xc3: // JP a
			JP();
			break;
		case 0xc4: // CALL NZ, a
			CALL_COND(!(_F & ZF), 0xc4);
			break;
		case 0xc5: // PUSH BC
			PUSH(BC);
			break;
		case 0xc6: // ADD A, n
			ADD(Fetch());
			break;
		case 0xc7: // RST 0
			RST(0x00);
			break;
		case 0xc8: // RET Z
			RET_COND(_F & ZF, 0xc8);
			break;
		case 0xc9: // RET
			RET();
			break;
		case 0xca: // JP Z, a
			JP_COND(_F & ZF);
			break;
		case 0xcb: // **** CB xx
			execute_opCB();
			break;
		case 0xcc: // CALL Z, a
			CALL_COND(_F & ZF, 0xcc);
			break;
		case 0xcd: // CALL a
			CALL();
			break;
		case 0xce: // ADC A, n
			ADC(Fetch());
			break;
		case 0xcf: // RST 1
			RST(0x08);
			break;
		case 0xd0: // RET NC
			RET_COND(!(_F & CF), 0xd0);
			break;
		case 0xd1: // POP DE
			DE = POP();
			break;
		case 0xd2: // JP NC, a
			JP_COND(!(_F & CF));
			break;
		case 0xd3: // OUT (n), A
			WriteIO(Fetch(), _A, _A);
			break;
		case 0xd4: // CALL NC, a
			CALL_COND(!(_F & CF), 0xd4);
			break;
		case 0xd5: // PUSH DE
			PUSH(DE);
			break;
		case 0xd6: // SUB n
			SUB(Fetch());
			break;
		case 0xd7: // RST 2
			RST(0x10);
			break;
		case 0xd8: // RET C
			RET_COND(_F & CF, 0xd8);
			break;
		case 0xd9: // EXX
			EXX();
			break;
		case 0xda: // JP C, a
			JP_COND(_F & CF);
			break;
		case 0xdb: // IN A, (n)
			_A = ReadIO(Fetch(), _A);
			break;
		case 0xdc: // CALL C, a
			CALL_COND(_F & CF, 0xdc);
			break;
		case 0xdd: // **** DD xx
			execute_opDD();
			break;
		case 0xde: // SBC A, n
			SBC(Fetch());
			break;
		case 0xdf: // RST 3
			RST(0x18);
			break;
		case 0xe0: // RET PO
			RET_COND(!(_F & PF), 0xe0);
			break;
		case 0xe1: // POP HL
			HL = POP();
			break;
		case 0xe2: // JP PO, a
			JP_COND(!(_F & PF));
			break;
		case 0xe3: // EX HL, (SP)
			HL = EXSP(HL);
			break;
		case 0xe4: // CALL PO, a
			CALL_COND(!(_F & PF), 0xe4);
			break;
		case 0xe5: // PUSH HL
			PUSH(HL);
			break;
		case 0xe6: // AND n
			AND(Fetch());
			break;
		case 0xe7: // RST 4
			RST(0x20);
			break;
		case 0xe8: // RET PE
			RET_COND(_F & PF, 0xe8);
			break;
		case 0xe9: // JP (HL)
			PC = HL;
			break;
		case 0xea: // JP PE, a
			JP_COND(_F & PF);
			break;
		case 0xeb: // EX DE, HL
			EX_DE_HL();
			break;
		case 0xec: // CALL PE, a
			CALL_COND(_F & PF, 0xec);
			break;
		case 0xed: // **** ED xx
			execute_opED();
			break;
		case 0xee: // XOR n
			XOR(Fetch());
			break;
		case 0xef: // RST 5
			RST(0x28);
			break;
		case 0xf0: // RET P
			RET_COND(!(_F & SF), 0xf0);
			break;
		case 0xf1: // POP AF
			AF = POP();
			break;
		case 0xf2: // JP P, a
			JP_COND(!(_F & SF));
			break;
		case 0xf3: // DI
			DI();
			break;
		case 0xf4: // CALL P, a
			CALL_COND(!(_F & SF), 0xf4);
			break;
		case 0xf5: // PUSH AF
			PUSH(AF);
			break;
		case 0xf6: // OR n
			OR(Fetch());
			break;
		case 0xf7: // RST 6
			RST(0x30);
			break;
		case 0xf8: // RET M
			RET_COND(_F & SF, 0xf8);
			break;
		case 0xf9: // LD SP, HL
			SP = HL;
			break;
		case 0xfa: // JP M, a
			JP_COND(_F & SF);
			break;
		case 0xfb: // EI
			EI();
			break;
		case 0xfc: // CALL M, a
			CALL_COND(_F & SF, 0xfc);
			break;
		case 0xfd: // **** FD xx
			execute_opFD();
			break;
		case 0xfe: // CP n
			CP(Fetch());
			break;
		case 0xff: // RST 7
			RST(0x38);
			break;
	}
}

void Z80::execute_opCB()
{
	// Execute instructions
	uint8 cmd = Fetch();
	count -= cc_cb[cmd];
	_R = (_R & 0x80) | ((_R + 1) & 0x7f);
	
	switch(cmd)
	{
		case 0x00: // RLC B
			_B = RLC(_B);
			break;
		case 0x01: // RLC C
			_C = RLC(_C);
			break;
		case 0x02: // RLC D
			_D = RLC(_D);
			break;
		case 0x03: // RLC E
			_E = RLC(_E);
			break;
		case 0x04: // RLC H
			_H = RLC(_H);
			break;
		case 0x05: // RLC L
			_L = RLC(_L);
			break;
		case 0x06: // RLC (HL)
			WriteMemory(HL, RLC(ReadMemory(HL)));
			break;
		case 0x07: // RLC A
			_A = RLC(_A);
			break;
		case 0x08: // RRC B
			_B = RRC(_B);
			break;
		case 0x09: // RRC C
			_C = RRC(_C);
			break;
		case 0x0a: // RRC D
			_D = RRC(_D);
			break;
		case 0x0b: // RRC E
			_E = RRC(_E);
			break;
		case 0x0c: // RRC H
			_H = RRC(_H);
			break;
		case 0x0d: // RRC L
			_L = RRC(_L);
			break;
		case 0x0e: // RRC (HL)
			WriteMemory(HL, RRC(ReadMemory(HL)));
			break;
		case 0x0f: // RRC A
			_A = RRC(_A);
			break;
		case 0x10: // RL B
			_B = RL(_B);
			break;
		case 0x11: // RL C
			_C = RL(_C);
			break;
		case 0x12: // RL D
			_D = RL(_D);
			break;
		case 0x13: // RL E
			_E = RL(_E);
			break;
		case 0x14: // RL H
			_H = RL(_H);
			break;
		case 0x15: // RL L
			_L = RL(_L);
			break;
		case 0x16: // RL (HL)
			WriteMemory(HL, RL(ReadMemory(HL)));
			break;
		case 0x17: // RL A
			_A = RL(_A);
			break;
		case 0x18: // RR B
			_B = RR(_B);
			break;
		case 0x19: // RR C
			_C = RR(_C);
			break;
		case 0x1a: // RR D
			_D = RR(_D);
			break;
		case 0x1b: // RR E
			_E = RR(_E);
			break;
		case 0x1c: // RR H
			_H = RR(_H);
			break;
		case 0x1d: // RR L
			_L = RR(_L);
			break;
		case 0x1e: // RR (HL)
			WriteMemory(HL, RR(ReadMemory(HL)));
			break;
		case 0x1f: // RR A
			_A = RR(_A);
			break;
		case 0x20: // SLA B
			_B = SLA(_B);
			break;
		case 0x21: // SLA C
			_C = SLA(_C);
			break;
		case 0x22: // SLA D
			_D = SLA(_D);
			break;
		case 0x23: // SLA E
			_E = SLA(_E);
			break;
		case 0x24: // SLA H
			_H = SLA(_H);
			break;
		case 0x25: // SLA L
			_L = SLA(_L);
			break;
		case 0x26: // SLA (HL)
			WriteMemory(HL, SLA(ReadMemory(HL)));
			break;
		case 0x27: // SLA A
			_A = SLA(_A);
			break;
		case 0x28: // SRA B
			_B = SRA(_B);
			break;
		case 0x29: // SRA C
			_C = SRA(_C);
			break;
		case 0x2a: // SRA D
			_D = SRA(_D);
			break;
		case 0x2b: // SRA E
			_E = SRA(_E);
			break;
		case 0x2c: // SRA H
			_H = SRA(_H);
			break;
		case 0x2d: // SRA L
			_L = SRA(_L);
			break;
		case 0x2e: // SRA (HL)
			WriteMemory(HL, SRA(ReadMemory(HL)));
			break;
		case 0x2f: // SRA A
			_A = SRA(_A);
			break;
		case 0x30: // SLL B
			_B = SLL(_B);
			break;
		case 0x31: // SLL C
			_C = SLL(_C);
			break;
		case 0x32: // SLL D
			_D = SLL(_D);
			break;
		case 0x33: // SLL E
			_E = SLL(_E);
			break;
		case 0x34: // SLL H
			_H = SLL(_H);
			break;
		case 0x35: // SLL L
			_L = SLL(_L);
			break;
		case 0x36: // SLL (HL)
			WriteMemory(HL, SLL(ReadMemory(HL)));
			break;
		case 0x37: // SLL A
			_A = SLL(_A);
			break;
		case 0x38: // SRL B
			_B = SRL(_B);
			break;
		case 0x39: // SRL C
			_C = SRL(_C);
			break;
		case 0x3a: // SRL D
			_D = SRL(_D);
			break;
		case 0x3b: // SRL E
			_E = SRL(_E);
			break;
		case 0x3c: // SRL H
			_H = SRL(_H);
			break;
		case 0x3d: // SRL L
			_L = SRL(_L);
			break;
		case 0x3e: // SRL (HL)
			WriteMemory(HL, SRL(ReadMemory(HL)));
			break;
		case 0x3f: // SRL A
			_A = SRL(_A);
			break;
		case 0x40: // BIT 0, B
			BIT(0, _B);
			break;
		case 0x41: // BIT 0, C
			BIT(0, _C);
			break;
		case 0x42: // BIT 0, D
			BIT(0, _D);
			break;
		case 0x43: // BIT 0, E
			BIT(0, _E);
			break;
		case 0x44: // BIT 0, H
			BIT(0, _H);
			break;
		case 0x45: // BIT 0, L
			BIT(0, _L);
			break;
		case 0x46: // BIT 0, (HL)
			BIT(0, ReadMemory(HL));
			break;
		case 0x47: // BIT 0, A
			BIT(0, _A);
			break;
		case 0x48: // BIT 1, B
			BIT(1, _B);
			break;
		case 0x49: // BIT 1, C
			BIT(1, _C);
			break;
		case 0x4a: // BIT 1, D
			BIT(1, _D);
			break;
		case 0x4b: // BIT 1, E
			BIT(1, _E);
			break;
		case 0x4c: // BIT 1, H
			BIT(1, _H);
			break;
		case 0x4d: // BIT 1, L
			BIT(1, _L);
			break;
		case 0x4e: // BIT 1, (HL)
			BIT(1, ReadMemory(HL));
			break;
		case 0x4f: // BIT 1, A
			BIT(1, _A);
			break;
		case 0x50: // BIT 2, B
			BIT(2, _B);
			break;
		case 0x51: // BIT 2, C
			BIT(2, _C);
			break;
		case 0x52: // BIT 2, D
			BIT(2, _D);
			break;
		case 0x53: // BIT 2, E
			BIT(2, _E);
			break;
		case 0x54: // BIT 2, H
			BIT(2, _H);
			break;
		case 0x55: // BIT 2, L
			BIT(2, _L);
			break;
		case 0x56: // BIT 2, (HL)
			BIT(2, ReadMemory(HL));
			break;
		case 0x57: // BIT 2, A
			BIT(2, _A);
			break;
		case 0x58: // BIT 3, B
			BIT(3, _B);
			break;
		case 0x59: // BIT 3, C
			BIT(3, _C);
			break;
		case 0x5a: // BIT 3, D
			BIT(3, _D);
			break;
		case 0x5b: // BIT 3, E
			BIT(3, _E);
			break;
		case 0x5c: // BIT 3, H
			BIT(3, _H);
			break;
		case 0x5d: // BIT 3, L
			BIT(3, _L);
			break;
		case 0x5e: // BIT 3, (HL)
			BIT(3, ReadMemory(HL));
			break;
		case 0x5f: // BIT 3, A
			BIT(3, _A);
			break;
		case 0x60: // BIT 4, B
			BIT(4, _B);
			break;
		case 0x61: // BIT 4, C
			BIT(4, _C);
			break;
		case 0x62: // BIT 4, D
			BIT(4, _D);
			break;
		case 0x63: // BIT 4, E
			BIT(4, _E);
			break;
		case 0x64: // BIT 4, H
			BIT(4, _H);
			break;
		case 0x65: // BIT 4, L
			BIT(4, _L);
			break;
		case 0x66: // BIT 4, (HL)
			BIT(4, ReadMemory(HL));
			break;
		case 0x67: // BIT 4, A
			BIT(4, _A);
			break;
		case 0x68: // BIT 5, B
			BIT(5, _B);
			break;
		case 0x69: // BIT 5, C
			BIT(5, _C);
			break;
		case 0x6a: // BIT 5, D
			BIT(5, _D);
			break;
		case 0x6b: // BIT 5, E
			BIT(5, _E);
			break;
		case 0x6c: // BIT 5, H
			BIT(5, _H);
			break;
		case 0x6d: // BIT 5, L
			BIT(5, _L);
			break;
		case 0x6e: // BIT 5, (HL)
			BIT(5, ReadMemory(HL));
			break;
		case 0x6f: // BIT 5, A
			BIT(5, _A);
			break;
		case 0x70: // BIT 6, B
			BIT(6, _B);
			break;
		case 0x71: // BIT 6, C
			BIT(6, _C);
			break;
		case 0x72: // BIT 6, D
			BIT(6, _D);
			break;
		case 0x73: // BIT 6, E
			BIT(6, _E);
			break;
		case 0x74: // BIT 6, H
			BIT(6, _H);
			break;
		case 0x75: // BIT 6, L
			BIT(6, _L);
			break;
		case 0x76: // BIT 6, (HL)
			BIT(6, ReadMemory(HL));
			break;
		case 0x77: // BIT 6, A
			BIT(6, _A);
			break;
		case 0x78: // BIT 7, B
			BIT(7, _B);
			break;
		case 0x79: // BIT 7, C
			BIT(7, _C);
			break;
		case 0x7a: // BIT 7, D
			BIT(7, _D);
			break;
		case 0x7b: // BIT 7, E
			BIT(7, _E);
			break;
		case 0x7c: // BIT 7, H
			BIT(7, _H);
			break;
		case 0x7d: // BIT 7, L
			BIT(7, _L);
			break;
		case 0x7e: // BIT 7, (HL)
			BIT(7, ReadMemory(HL));
			break;
		case 0x7f: // BIT 7, A
			BIT(7, _A);
			break;
		case 0x80: // RES 0, B
			_B = RES(0, _B);
			break;
		case 0x81: // RES 0, C
			_C = RES(0, _C);
			break;
		case 0x82: // RES 0, D
			_D = RES(0, _D);
			break;
		case 0x83: // RES 0, E
			_E = RES(0, _E);
			break;
		case 0x84: // RES 0, H
			_H = RES(0, _H);
			break;
		case 0x85: // RES 0, L
			_L = RES(0, _L);
			break;
		case 0x86: // RES 0, (HL)
			WriteMemory(HL, RES(0, ReadMemory(HL)));
			break;
		case 0x87: // RES 0, A
			_A = RES(0, _A);
			break;
		case 0x88: // RES 1, B
			_B = RES(1, _B);
			break;
		case 0x89: // RES 1, C
			_C = RES(1, _C);
			break;
		case 0x8a: // RES 1, D
			_D = RES(1, _D);
			break;
		case 0x8b: // RES 1, E
			_E = RES(1, _E);
			break;
		case 0x8c: // RES 1, H
			_H = RES(1, _H);
			break;
		case 0x8d: // RES 1, L
			_L = RES(1, _L);
			break;
		case 0x8e: // RES 1, (HL)
			WriteMemory(HL, RES(1, ReadMemory(HL)));
			break;
		case 0x8f: // RES 1, A
			_A = RES(1, _A);
			break;
		case 0x90: // RES 2, B
			_B = RES(2, _B);
			break;
		case 0x91: // RES 2, C
			_C = RES(2, _C);
			break;
		case 0x92: // RES 2, D
			_D = RES(2, _D);
			break;
		case 0x93: // RES 2, E
			_E = RES(2, _E);
			break;
		case 0x94: // RES 2, H
			_H = RES(2, _H);
			break;
		case 0x95: // RES 2, L
			_L = RES(2, _L);
			break;
		case 0x96: // RES 2, (HL)
			WriteMemory(HL, RES(2, ReadMemory(HL)));
			break;
		case 0x97: // RES 2, A
			_A = RES(2, _A);
			break;
		case 0x98: // RES 3, B
			_B = RES(3, _B);
			break;
		case 0x99: // RES 3, C
			_C = RES(3, _C);
			break;
		case 0x9a: // RES 3, D
			_D = RES(3, _D);
			break;
		case 0x9b: // RES 3, E
			_E = RES(3, _E);
			break;
		case 0x9c: // RES 3, H
			_H = RES(3, _H);
			break;
		case 0x9d: // RES 3, L
			_L = RES(3, _L);
			break;
		case 0x9e: // RES 3, (HL)
			WriteMemory(HL, RES(3, ReadMemory(HL)));
			break;
		case 0x9f: // RES 3, A
			_A = RES(3, _A);
			break;
		case 0xa0: // RES 4, B
			_B = RES(4, _B);
			break;
		case 0xa1: // RES 4, C
			_C = RES(4, _C);
			break;
		case 0xa2: // RES 4, D
			_D = RES(4, _D);
			break;
		case 0xa3: // RES 4, E
			_E = RES(4, _E);
			break;
		case 0xa4: // RES 4, H
			_H = RES(4, _H);
			break;
		case 0xa5: // RES 4, L
			_L = RES(4, _L);
			break;
		case 0xa6: // RES 4, (HL)
			WriteMemory(HL, RES(4, ReadMemory(HL)));
			break;
		case 0xa7: // RES 4, A
			_A = RES(4, _A);
			break;
		case 0xa8: // RES 5, B
			_B = RES(5, _B);
			break;
		case 0xa9: // RES 5, C
			_C = RES(5, _C);
			break;
		case 0xaa: // RES 5, D
			_D = RES(5, _D);
			break;
		case 0xab: // RES 5, E
			_E = RES(5, _E);
			break;
		case 0xac: // RES 5, H
			_H = RES(5, _H);
			break;
		case 0xad: // RES 5, L
			_L = RES(5, _L);
			break;
		case 0xae: // RES 5, (HL)
			WriteMemory(HL, RES(5, ReadMemory(HL)));
			break;
		case 0xaf: // RES 5, A
			_A = RES(5, _A);
			break;
		case 0xb0: // RES 6, B
			_B = RES(6, _B);
			break;
		case 0xb1: // RES 6, C
			_C = RES(6, _C);
			break;
		case 0xb2: // RES 6, D
			_D = RES(6, _D);
			break;
		case 0xb3: // RES 6, E
			_E = RES(6, _E);
			break;
		case 0xb4: // RES 6, H
			_H = RES(6, _H);
			break;
		case 0xb5: // RES 6, L
			_L = RES(6, _L);
			break;
		case 0xb6: // RES 6, (HL)
			WriteMemory(HL, RES(6, ReadMemory(HL)));
			break;
		case 0xb7: // RES 6, A
			_A = RES(6, _A);
			break;
		case 0xb8: // RES 7, B
			_B = RES(7, _B);
			break;
		case 0xb9: // RES 7, C
			_C = RES(7, _C);
			break;
		case 0xba: // RES 7, D
			_D = RES(7, _D);
			break;
		case 0xbb: // RES 7, E
			_E = RES(7, _E);
			break;
		case 0xbc: // RES 7, H
			_H = RES(7, _H);
			break;
		case 0xbd: // RES 7, L
			_L = RES(7, _L);
			break;
		case 0xbe: // RES 7, (HL)
			WriteMemory(HL, RES(7, ReadMemory(HL)));
			break;
		case 0xbf: // RES 7, A
			_A = RES(7, _A);
			break;
		case 0xc0: // SET 0, B
			_B = SET(0, _B);
			break;
		case 0xc1: // SET 0, C
			_C = SET(0, _C);
			break;
		case 0xc2: // SET 0, D
			_D = SET(0, _D);
			break;
		case 0xc3: // SET 0, E
			_E = SET(0, _E);
			break;
		case 0xc4: // SET 0, H
			_H = SET(0, _H);
			break;
		case 0xc5: // SET 0, L
			_L = SET(0, _L);
			break;
		case 0xc6: // SET 0, (HL)
			WriteMemory(HL, SET(0, ReadMemory(HL)));
			break;
		case 0xc7: // SET 0, A
			_A = SET(0, _A);
			break;
		case 0xc8: // SET 1, B
			_B = SET(1, _B);
			break;
		case 0xc9: // SET 1, C
			_C = SET(1, _C);
			break;
		case 0xca: // SET 1, D
			_D = SET(1, _D);
			break;
		case 0xcb: // SET 1, E
			_E = SET(1, _E);
			break;
		case 0xcc: // SET 1, H
			_H = SET(1, _H);
			break;
		case 0xcd: // SET 1, L
			_L = SET(1, _L);
			break;
		case 0xce: // SET 1, (HL)
			WriteMemory(HL, SET(1, ReadMemory(HL)));
			break;
		case 0xcf: // SET 1, A
			_A = SET(1, _A);
			break;
		case 0xd0: // SET 2, B
			_B = SET(2, _B);
			break;
		case 0xd1: // SET 2, C
			_C = SET(2, _C);
			break;
		case 0xd2: // SET 2, D
			_D = SET(2, _D);
			break;
		case 0xd3: // SET 2, E
			_E = SET(2, _E);
			break;
		case 0xd4: // SET 2, H
			_H = SET(2, _H);
			break;
		case 0xd5: // SET 2, L
			_L = SET(2, _L);
			break;
		case 0xd6: // SET 2, (HL)
			WriteMemory(HL, SET(2, ReadMemory(HL)));
			break;
		case 0xd7: // SET 2, A
			_A = SET(2, _A);
			break;
		case 0xd8: // SET 3, B
			_B = SET(3, _B);
			break;
		case 0xd9: // SET 3, C
			_C = SET(3, _C);
			break;
		case 0xda: // SET 3, D
			_D = SET(3, _D);
			break;
		case 0xdb: // SET 3, E
			_E = SET(3, _E);
			break;
		case 0xdc: // SET 3, H
			_H = SET(3, _H);
			break;
		case 0xdd: // SET 3, L
			_L = SET(3, _L);
			break;
		case 0xde: // SET 3, (HL)
			WriteMemory(HL, SET(3, ReadMemory(HL)));
			break;
		case 0xdf: // SET 3, A
			_A = SET(3, _A);
			break;
		case 0xe0: // SET 4, B
			_B = SET(4, _B);
			break;
		case 0xe1: // SET 4, C
			_C = SET(4, _C);
			break;
		case 0xe2: // SET 4, D
			_D = SET(4, _D);
			break;
		case 0xe3: // SET 4, E
			_E = SET(4, _E);
			break;
		case 0xe4: // SET 4, H
			_H = SET(4, _H);
			break;
		case 0xe5: // SET 4, L
			_L = SET(4, _L);
			break;
		case 0xe6: // SET 4, (HL)
			WriteMemory(HL, SET(4, ReadMemory(HL)));
			break;
		case 0xe7: // SET 4, A
			_A = SET(4, _A);
			break;
		case 0xe8: // SET 5, B
			_B = SET(5, _B);
			break;
		case 0xe9: // SET 5, C
			_C = SET(5, _C);
			break;
		case 0xea: // SET 5, D
			_D = SET(5, _D);
			break;
		case 0xeb: // SET 5, E
			_E = SET(5, _E);
			break;
		case 0xec: // SET 5, H
			_H = SET(5, _H);
			break;
		case 0xed: // SET 5, L
			_L = SET(5, _L);
			break;
		case 0xee: // SET 5, (HL)
			WriteMemory(HL, SET(5, ReadMemory(HL)));
			break;
		case 0xef: // SET 5, A
			_A = SET(5, _A);
			break;
		case 0xf0: // SET 6, B
			_B = SET(6, _B);
			break;
		case 0xf1: // SET 6, C
			_C = SET(6, _C);
			break;
		case 0xf2: // SET 6, D
			_D = SET(6, _D);
			break;
		case 0xf3: // SET 6, E
			_E = SET(6, _E);
			break;
		case 0xf4: // SET 6, H
			_H = SET(6, _H);
			break;
		case 0xf5: // SET 6, L
			_L = SET(6, _L);
			break;
		case 0xf6: // SET 6, (HL)
			WriteMemory(HL, SET(6, ReadMemory(HL)));
			break;
		case 0xf7: // SET 6, A
			_A = SET(6, _A);
			break;
		case 0xf8: // SET 7, B
			_B = SET(7, _B);
			break;
		case 0xf9: // SET 7, C
			_C = SET(7, _C);
			break;
		case 0xfa: // SET 7, D
			_D = SET(7, _D);
			break;
		case 0xfb: // SET 7, E
			_E = SET(7, _E);
			break;
		case 0xfc: // SET 7, H
			_H = SET(7, _H);
			break;
		case 0xfd: // SET 7, L
			_L = SET(7, _L);
			break;
		case 0xfe: // SET 7, (HL)
			WriteMemory(HL, SET(7, ReadMemory(HL)));
			break;
		case 0xff: // SET 7, A
			_A = SET(7, _A);
			break;
	}
}

void Z80::execute_opDD()
{
	// Execute instructions
	uint8 cmd = Fetch();
	count -= cc_xy[cmd];
	_R = (_R & 0x80) | ((_R + 1) & 0x7f);
	
	switch(cmd)
	{
		case 0x09: // ADD IX, BC
			IX = ADD16(IX, BC);
			break;
		case 0x19: // ADD IX, DE
			IX = ADD16(IX, DE);
			break;
		case 0x21: // LD IX, w
			IX = Fetch16();
			break;
		case 0x22: // LD (w), IX
			EA = Fetch16();
			WriteMemory16(EA, IX);
			break;
		case 0x23: // INC IX
			IX++;
			break;
		case 0x24: // INC HX
			_XH = INC(_XH);
			break;
		case 0x25: // DEC HX
			_XH = DEC(_XH);
			break;
		case 0x26: // LD HX, n
			_XH = Fetch();
			break;
		case 0x29: // ADD IX, IX
			IX = ADD16(IX, IX);
			break;
		case 0x2a: // LD IX, (w)
			EA = Fetch16();
			IX = ReadMemory16(EA);
			break;
		case 0x2b: // DEC IX
			IX--;
			break;
		case 0x2c: // INC LX
			_XL = INC(_XL);
			break;
		case 0x2d: // DEC LX
			_XL = DEC(_XL);
			break;
		case 0x2e: // LD LX, n
			_XL = Fetch();
			break;
		case 0x34: // INC (IX+o)
			EAX();
			WriteMemory(EA, INC(ReadMemory(EA)));
			break;
		case 0x35: // DEC (IX+o)
			EAX();
			WriteMemory(EA, DEC(ReadMemory(EA)));
			break;
		case 0x36: // LD (IX+o), n
			EAX();
			WriteMemory(EA, Fetch());
			break;
		case 0x39: // ADD IX, SP
			IX = ADD16(IX, SP);
			break;
		case 0x44: // LD B, HX
			_B = _XH;
			break;
		case 0x45: // LD B, LX
			_B = _XL;
			break;
		case 0x46: // LD B, (IX+o)
			EAX();
			_B = ReadMemory(EA);
			break;
		case 0x4c: // LD C, HX
			_C = _XH;
			break;
		case 0x4d: // LD C, LX
			_C = _XL;
			break;
		case 0x4e: // LD C, (IX+o)
			EAX();
			_C = ReadMemory(EA);
			break;
		case 0x54: // LD D, HX
			_D = _XH;
			break;
		case 0x55: // LD D, LX
			_D = _XL;
			break;
		case 0x56: // LD D, (IX+o)
			EAX();
			_D = ReadMemory(EA);
			break;
		case 0x5c: // LD E, HX
			_E = _XH;
			break;
		case 0x5d: // LD E, LX
			_E = _XL;
			break;
		case 0x5e: // LD E, (IX+o)
			EAX();
			_E = ReadMemory(EA);
			break;
		case 0x60: // LD HX, B
			_XH = _B;
			break;
		case 0x61: // LD HX, C
			_XH = _C;
			break;
		case 0x62: // LD HX, D
			_XH = _D;
			break;
		case 0x63: // LD HX, E
			_XH = _E;
			break;
		case 0x64: // LD HX, HX
			break;
		case 0x65: // LD HX, LX
			_XH = _XL;
			break;
		case 0x66: // LD H, (IX+o)
			EAX();
			_H = ReadMemory(EA);
			break;
		case 0x67: // LD HX, A
			_XH = _A;
			break;
		case 0x68: // LD LX, B
			_XL = _B;
			break;
		case 0x69: // LD LX, C
			_XL = _C;
			break;
		case 0x6a: // LD LX, D
			_XL = _D;
			break;
		case 0x6b: // LD LX, E
			_XL = _E;
			break;
		case 0x6c: // LD LX, HX
			_XL = _XH;
			break;
		case 0x6d: // LD LX, LX
			break;
		case 0x6e: // LD L, (IX+o)
			EAX();
			_L = ReadMemory(EA);
			break;
		case 0x6f: // LD LX, A
			_XL = _A;
			break;
		case 0x70: // LD (IX+o), B
			EAX();
			WriteMemory(EA, _B);
			break;
		case 0x71: // LD (IX+o), C
			EAX();
			WriteMemory(EA, _C);
			break;
		case 0x72: // LD (IX+o), D
			EAX();
			WriteMemory(EA, _D);
			break;
		case 0x73: // LD (IX+o), E
			EAX();
			WriteMemory(EA, _E);
			break;
		case 0x74: // LD (IX+o), H
			EAX();
			WriteMemory(EA, _H);
			break;
		case 0x75: // LD (IX+o), L
			EAX();
			WriteMemory(EA, _L);
			break;
		case 0x77: // LD (IX+o), A
			EAX();
			WriteMemory(EA, _A);
			break;
		case 0x7c: // LD A, HX
			_A = _XH;
			break;
		case 0x7d: // LD A, LX
			_A = _XL;
			break;
		case 0x7e: // LD A, (IX+o)
			EAX();
			_A = ReadMemory(EA);
			break;
		case 0x84: // ADD A, HX
			ADD(_XH);
			break;
		case 0x85: // ADD A, LX
			ADD(_XL);
			break;
		case 0x86: // ADD A, (IX+o)
			EAX();
			ADD(ReadMemory(EA));
			break;
		case 0x8c: // ADC A, HX
			ADC(_XH);
			break;
		case 0x8d: // ADC A, LX
			ADC(_XL);
			break;
		case 0x8e: // ADC A, (IX+o)
			EAX();
			ADC(ReadMemory(EA));
			break;
		case 0x94: // SUB HX
			SUB(_XH);
			break;
		case 0x95: // SUB LX
			SUB(_XL);
			break;
		case 0x96: // SUB (IX+o)
			EAX();
			SUB(ReadMemory(EA));
			break;
		case 0x9c: // SBC A, HX
			SBC(_XH);
			break;
		case 0x9d: // SBC A, LX
			SBC(_XL);
			break;
		case 0x9e: // SBC A, (IX+o)
			EAX();
			SBC(ReadMemory(EA));
			break;
		case 0xa4: // AND HX
			AND(_XH);
			break;
		case 0xa5: // AND LX
			AND(_XL);
			break;
		case 0xa6: // AND (IX+o)
			EAX();
			AND(ReadMemory(EA));
			break;
		case 0xac: // XOR HX
			XOR(_XH);
			break;
		case 0xad: // XOR LX
			XOR(_XL);
			break;
		case 0xae: // XOR (IX+o)
			EAX();
			XOR(ReadMemory(EA));
			break;
		case 0xb4: // OR HX
			OR(_XH);
			break;
		case 0xb5: // OR LX
			OR(_XL);
			break;
		case 0xb6: // OR (IX+o)
			EAX();
			OR(ReadMemory(EA));
			break;
		case 0xbc: // CP HX
			CP(_XH);
			break;
		case 0xbd: // CP LX
			CP(_XL);
			break;
		case 0xbe: // CP (IX+o)
			EAX();
			CP(ReadMemory(EA));
			break;
		case 0xcb: // ** DD CB xx
			EAX();
			execute_opXY();
			break;
		case 0xe1: // POP IX
			IX = POP();
			break;
		case 0xe3: // EX (SP), IX
			IX = EXSP(IX);
			break;
		case 0xe5: // PUSH IX
			PUSH(IX);
			break;
		case 0xe9: // JP (IX)
			PC = IX;
			break;
		case 0xf9: // LD SP, IX
			SP = IX;
			break;
	}
}

void Z80::execute_opED()
{
	// Execute instructions
	uint8 cmd = Fetch();
	count -= cc_ed[cmd];
	_R = (_R & 0x80) | ((_R + 1) & 0x7f);
	
	switch(cmd)
	{
		case 0x40: // IN B, (C)
			_B = ReadIO(_C, _B);
			_F = (_F & CF) | SZP[_B];
			break;
		case 0x41: // OUT (C), B
			WriteIO(_C, _B, _B);
			break;
		case 0x42: // SBC HL, BC
			SBC16(BC);
			break;
		case 0x43: // LD (w), BC
			EA = Fetch16();
			WriteMemory16(EA, BC);
			break;
		case 0x44: // NEG
			NEG();
			break;
		case 0x45: // RETN;
			RETN();
			break;
		case 0x46: // IM 0
			IM = 0;
			break;
		case 0x47: // LD I, A
			_I = _A;
			break;
		case 0x48: // IN C, (C)
			_C = ReadIO(_C, _B);
			_F = (_F & CF) | SZP[_C];
			break;
		case 0x49: // OUT (C), C
			WriteIO(_C, _B, _C);
			break;
		case 0x4a: // ADC HL, BC
			ADC16(BC);
			break;
		case 0x4b: // LD BC, (w)
			EA = Fetch16();
			BC = ReadMemory16(EA);
			break;
		case 0x4c: // NEG
			NEG();
			break;
		case 0x4d: // RETI
			RETI();
			break;
		case 0x4e: // IM 0
			IM = 0;
			break;
		case 0x4f: // LD R, A
			_R = _A;
			break;
		case 0x50: // IN D, (C)
			_D = ReadIO(_C, _B);
			_F = (_F & CF) | SZP[_D];
			break;
		case 0x51: // OUT (C), D
			WriteIO(_C, _B, _D);
			break;
		case 0x52: // SBC HL, DE
			SBC16(DE);
			break;
		case 0x53: // LD (w), DE
			EA = Fetch16();
			WriteMemory16(EA, DE);
			break;
		case 0x54: // NEG
			NEG();
			break;
		case 0x55: // RETN;
			RETN();
			break;
		case 0x56: // IM 1
			IM = 1;
			break;
		case 0x57: // LD A, I
			_A = _I;
			_F = (_F & CF) | SZ[_A] | (IFF2 << 2);
			break;
		case 0x58: // IN E, (C)
			_E = ReadIO(_C, _B);
			_F = (_F & CF) | SZP[_E];
			break;
		case 0x59: // OUT (C), E
			WriteIO(_C, _B, _E);
			break;
		case 0x5a: // ADC HL, DE
			ADC16(DE);
			break;
		case 0x5b: // LD DE, (w)
			EA = Fetch16();
			DE = ReadMemory16(EA);
			break;
		case 0x5c: // NEG
			NEG();
			break;
		case 0x5d: // RETI
			RETI();
			break;
		case 0x5e: // IM 2
			IM = 2;
			break;
		case 0x5f: // LD A, R
			_A = _R;
			_F = (_F & CF) | SZ[_A] | (IFF2 << 2);
			break;
		case 0x60: // IN H, (C)
			_H = ReadIO(_C, _B);
			_F = (_F & CF) | SZP[_H];
			break;
		case 0x61: // OUT (C), H
			WriteIO(_C, _B, _H);
			break;
		case 0x62: // SBC HL, HL
			SBC16(HL);
			break;
		case 0x63: // LD (w), HL
			EA = Fetch16();
			WriteMemory16(EA, HL);
			break;
		case 0x64: // NEG
			NEG();
			break;
		case 0x65: // RETN;
			RETN();
			break;
		case 0x66: // IM 0
			IM = 0;
			break;
		case 0x67: // RRD (HL)
			RRD();
			break;
		case 0x68: // IN L, (C)
			_L = ReadIO(_C, _B);
			_F = (_F & CF) | SZP[_L];
			break;
		case 0x69: // OUT (C), L
			WriteIO(_C, _B, _L);
			break;
		case 0x6a: // ADC HL, HL
			ADC16(HL);
			break;
		case 0x6b: // LD HL, (w)
			EA = Fetch16();
			HL = ReadMemory16(EA);
			break;
		case 0x6c: // NEG
			NEG();
			break;
		case 0x6d: // RETI
			RETI();
			break;
		case 0x6e: // IM 0
			IM = 0;
			break;
		case 0x6f: // RLD (HL)
			RLD();
			break;
		case 0x70: // IN 0, (C)
			_F = (_F & CF) | SZP[ReadIO(_C, _B)];
			break;
		case 0x71: // OUT (C), 0
			WriteIO(_C, _B, 0);
			break;
		case 0x72: // SBC HL, SP
			SBC16(SP);
			break;
		case 0x73: // LD (w), SP
			EA = Fetch16();
			WriteMemory16(EA, SP);
			break;
		case 0x74: // NEG
			NEG();
			break;
		case 0x75: // RETN;
			RETN();
			break;
		case 0x76: // IM 1
			IM = 1;
			break;
		case 0x78: // IN E, (C)
			_A = ReadIO(_C, _B);
			_F = (_F & CF) | SZP[_A];
			break;
		case 0x79: // OUT (C), E
			WriteIO(_C, _B, _A);
			break;
		case 0x7a: // ADC HL, SP
			ADC16(SP);
			break;
		case 0x7b: // LD SP, (w)
			EA = ReadMemory16(PC);
			PC += 2;
			SP = ReadMemory16(EA);
			break;
		case 0x7c: // NEG
			NEG();
			break;
		case 0x7d: // RETI
			RETI();
			break;
		case 0x7e: // IM 2
			IM = 2;
			break;
		case 0xa0: // LDI
			LDI();
			break;
		case 0xa1: // CPI
			CPI();
			break;
		case 0xa2: // INI
			INI();
			break;
		case 0xa3: // OUTI
			OUTI();
			break;
		case 0xa8: // LDD
			LDD();
			break;
		case 0xa9: // CPD
			CPD();
			break;
		case 0xaa: // IND
			IND();
			break;
		case 0xab: // OUTD
			OUTD();
			break;
		case 0xb0: // LDIR
			LDIR();
			break;
		case 0xb1: // CPIR
			CPIR();
			break;
		case 0xb2: // INIR
			INIR();
			break;
		case 0xb3: // OTIR
			OTIR();
			break;
		case 0xb8: // LDDR
			LDDR();
			break;
		case 0xb9: // CPDR
			CPDR();
			break;
		case 0xba: // INDR
			INDR();
			break;
		case 0xbb: // OTDR
			OTDR();
			break;
	}
}

void Z80::execute_opFD()
{
	// Execute instructions
	uint8 cmd = Fetch();
	count -= cc_xy[cmd];
	_R = (_R & 0x80) | ((_R + 1) & 0x7f);
	
	switch(cmd)
	{
		case 0x09: // ADD IY, BC
			IY = ADD16(IY, BC);
			break;
		case 0x19: // ADD IY, DE
			IY = ADD16(IY, DE);
			break;
		case 0x21: // LD IY, w
			IY = ReadMemory16(PC);
			PC += 2;
			break;
		case 0x22: // LD (w), IY
			EA = ReadMemory16(PC);
			PC += 2;
			WriteMemory16(EA, IY);
			break;
		case 0x23: // INC IY
			IY++;
			break;
		case 0x24: // INC HY
			_YH = INC(_YH);
			break;
		case 0x25: // DEC HY
			_YH = DEC(_YH);
			break;
		case 0x26: // LD HY, n
			_YH = Fetch();
			break;
		case 0x29: // ADD IY, IY
			IY = ADD16(IY, IY);
			break;
		case 0x2a: // LD IY, (w)
			EA = ReadMemory16(PC);
			PC += 2;
			IY = ReadMemory16(EA);
			break;
		case 0x2b: // DEC IY
			IY--;
			break;
		case 0x2c: // INC LY
			_YL = INC(_YL);
			break;
		case 0x2d: // DEC LY
			_YL = DEC(_YL);
			break;
		case 0x2e: // LD LY, n
			_YL = Fetch();
			break;
		case 0x34: // INC (IY+o)
			EAY();
			WriteMemory(EA, INC(ReadMemory(EA)));
			break;
		case 0x35: // DEC (IY+o)
			EAY();
			WriteMemory(EA, DEC(ReadMemory(EA)));
			break;
		case 0x36: // LD (IY+o), n
			EAY();
			WriteMemory(EA, Fetch());
			break;
		case 0x39: // ADD IY, SP
			IY = ADD16(IY, SP);
			break;
		case 0x44: // LD B, HY
			_B = _YH;
			break;
		case 0x45: // LD B, LY
			_B = _YL;
			break;
		case 0x46: // LD B, (IY+o)
			EAY();
			_B = ReadMemory(EA);
			break;
		case 0x4c: // LD C, HY
			_C = _YH;
			break;
		case 0x4d: // LD C, LY
			_C = _YL;
			break;
		case 0x4e: // LD C, (IY+o)
			EAY();
			_C = ReadMemory(EA);
			break;
		case 0x54: // LD D, HY
			_D = _YH;
			break;
		case 0x55: // LD D, LY
			_D = _YL;
			break;
		case 0x56: // LD D, (IY+o)
			EAY();
			_D = ReadMemory(EA);
			break;
		case 0x5c: // LD E, HY
			_E = _YH;
			break;
		case 0x5d: // LD E, LY
			_E = _YL;
			break;
		case 0x5e: // LD E, (IY+o)
			EAY();
			_E = ReadMemory(EA);
			break;
		case 0x60: // LD HY, B
			_YH = _B;
			break;
		case 0x61: // LD HY, C
			_YH = _C;
			break;
		case 0x62: // LD HY, D
			_YH = _D;
			break;
		case 0x63: // LD HY, E
			_YH = _E;
			break;
		case 0x64: // LD HY, HY
			break;
		case 0x65: // LD HY, LY
			_YH = _YL;
			break;
		case 0x66: // LD H, (IY+o)
			EAY();
			_H = ReadMemory(EA);
			break;
		case 0x67: // LD HY, A
			_YH = _A;
			break;
		case 0x68: // LD LY, B
			_YL = _B;
			break;
		case 0x69: // LD LY, C
			_YL = _C;
			break;
		case 0x6a: // LD LY, D
			_YL = _D;
			break;
		case 0x6b: // LD LY, E
			_YL = _E;
			break;
		case 0x6c: // LD LY, HY
			_YL = _YH;
			break;
		case 0x6d: // LD LY, LY
			break;
		case 0x6e: // LD L, (IY+o)
			EAY();
			_L = ReadMemory(EA);
			break;
		case 0x6f: // LD LY, A
			_YL = _A;
			break;
		case 0x70: // LD (IY+o), B
			EAY();
			WriteMemory(EA, _B);
			break;
		case 0x71: // LD (IY+o), C
			EAY();
			WriteMemory(EA, _C);
			break;
		case 0x72: // LD (IY+o), D
			EAY();
			WriteMemory(EA, _D);
			break;
		case 0x73: // LD (IY+o), E
			EAY();
			WriteMemory(EA, _E);
			break;
		case 0x74: // LD (IY+o), H
			EAY();
			WriteMemory(EA, _H);
			break;
		case 0x75: // LD (IY+o), L
			EAY();
			WriteMemory(EA, _L);
			break;
		case 0x77: // LD (IY+o), A
			EAY();
			WriteMemory(EA, _A);
			break;
		case 0x7c: // LD A, HY
			_A = _YH;
			break;
		case 0x7d: // LD A, LY
			_A = _YL;
			break;
		case 0x7e: // LD A, (IY+o)
			EAY();
			_A = ReadMemory(EA);
			break;
		case 0x84: // ADD A, HY
			ADD(_YH);
			break;
		case 0x85: // ADD A, LY
			ADD(_YL);
			break;
		case 0x86: // ADD A, (IY+o)
			EAY();
			ADD(ReadMemory(EA));
			break;
		case 0x8c: // ADC A, HY
			ADC(_YH);
			break;
		case 0x8d: // ADC A, LY
			ADC(_YL);
			break;
		case 0x8e: // ADC A, (IY+o)
			EAY();
			ADC(ReadMemory(EA));
			break;
		case 0x94: // SUB HY
			SUB(_YH);
			break;
		case 0x95: // SUB LY
			SUB(_YL);
			break;
		case 0x96: // SUB (IY+o)
			EAY();
			SUB(ReadMemory(EA));
			break;
		case 0x9c: // SBC A, HY
			SBC(_YH);
			break;
		case 0x9d: // SBC A, LY
			SBC(_YL);
			break;
		case 0x9e: // SBC A, (IY+o)
			EAY();
			SBC(ReadMemory(EA));
			break;
		case 0xa4: // AND HY
			AND(_YH);
			break;
		case 0xa5: // AND LY
			AND(_YL);
			break;
		case 0xa6: // AND (IY+o)
			EAY();
			AND(ReadMemory(EA));
			break;
		case 0xac: // XOR HY
			XOR(_YH);
			break;
		case 0xad: // XOR LY
			XOR(_YL);
			break;
		case 0xae: // XOR (IY+o)
			EAY();
			XOR(ReadMemory(EA));
			break;
		case 0xb4: // OR HY
			OR(_YH);
			break;
		case 0xb5: // OR LY
			OR(_YL);
			break;
		case 0xb6: // OR (IY+o)
			EAY();
			OR(ReadMemory(EA));
			break;
		case 0xbc: // CP HY
			CP(_YH);
			break;
		case 0xbd: // CP LY
			CP(_YL);
			break;
		case 0xbe: // CP (IY+o)
			EAY();
			CP(ReadMemory(EA));
			break;
		case 0xcb: // ** FD CB xx
			EAY();
			execute_opXY();
			break;
		case 0xe1: // POP IY
			IY = POP();
			break;
		case 0xe3: // EX (SP), IY
			IY = EXSP(IY);
			break;
		case 0xe5: // PUSH IY
			PUSH(IY);
			break;
		case 0xe9: // JP (IY)
			PC = IY;
			break;
		case 0xf9: // LD SP, IY
			SP = IY;
			break;
	}
}

void Z80::execute_opXY()
{
	// Execute instructions
	uint8 cmd = Fetch();
	count -= cc_xy[cmd];
	_R = (_R & 0x80) | ((_R + 1) & 0x7f);
	
	switch(cmd)
	{
		case 0x00: // RLC B=(XY+o)
			_B = RLC(ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0x01: // RLC C=(XY+o)
			_C = RLC(ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0x02: // RLC D=(XY+o)
			_D = RLC(ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0x03: // RLC E=(XY+o)
			_E = RLC(ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0x04: // RLC H=(XY+o)
			_H = RLC(ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0x05: // RLC L=(XY+o)
			_L = RLC(ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0x06: // RLC (XY+o)
			WriteMemory(EA, RLC(ReadMemory(EA)));
			break;
		case 0x07: // RLC A=(XY+o)
			_A = RLC(ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0x08: // RRC B=(XY+o)
			_B = RRC(ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0x09: // RRC C=(XY+o)
			_C = RRC(ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0x0a: // RRC D=(XY+o)
			_D = RRC(ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0x0b: // RRC E=(XY+o)
			_E = RRC(ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0x0c: // RRC H=(XY+o)
			_H = RRC(ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0x0d: // RRC L=(XY+o)
			_L = RRC(ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0x0e: // RRC (XY+o)
			WriteMemory(EA, RRC(ReadMemory(EA)));
			break;
		case 0x0f: // RRC A=(XY+o)
			_A = RRC(ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0x10: // RL B=(XY+o)
			_B = RL(ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0x11: // RL C=(XY+o)
			_C = RL(ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0x12: // RL D=(XY+o)
			_D = RL(ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0x13: // RL E=(XY+o)
			_E = RL(ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0x14: // RL H=(XY+o)
			_H = RL(ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0x15: // RL L=(XY+o)
			_L = RL(ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0x16: // RL (XY+o)
			WriteMemory(EA, RL(ReadMemory(EA)));
			break;
		case 0x17: // RL A=(XY+o)
			_A = RL(ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0x18: // RR B=(XY+o)
			_B = RR(ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0x19: // RR C=(XY+o)
			_C = RR(ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0x1a: // RR D=(XY+o)
			_D = RR(ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0x1b: // RR E=(XY+o)
			_E = RR(ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0x1c: // RR H=(XY+o)
			_H = RR(ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0x1d: // RR L=(XY+o)
			_L = RR(ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0x1e: // RR (XY+o)
			WriteMemory(EA, RR(ReadMemory(EA)));
			break;
		case 0x1f: // RR A=(XY+o)
			_A = RR(ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0x20: // SLA B=(XY+o)
			_B = SLA(ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0x21: // SLA C=(XY+o)
			_C = SLA(ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0x22: // SLA D=(XY+o)
			_D = SLA(ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0x23: // SLA E=(XY+o)
			_E = SLA(ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0x24: // SLA H=(XY+o)
			_H = SLA(ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0x25: // SLA L=(XY+o)
			_L = SLA(ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0x26: // SLA (XY+o)
			WriteMemory(EA, SLA(ReadMemory(EA)));
			break;
		case 0x27: // SLA A=(XY+o)
			_A = SLA(ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0x28: // SRA B=(XY+o)
			_B = SRA(ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0x29: // SRA C=(XY+o)
			_C = SRA(ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0x2a: // SRA D=(XY+o)
			_D = SRA(ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0x2b: // SRA E=(XY+o)
			_E = SRA(ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0x2c: // SRA H=(XY+o)
			_H = SRA(ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0x2d: // SRA L=(XY+o)
			_L = SRA(ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0x2e: // SRA (XY+o)
			WriteMemory(EA, SRA(ReadMemory(EA)));
			break;
		case 0x2f: // SRA A=(XY+o)
			_A = SRA(ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0x30: // SLL B=(XY+o)
			_B = SLL(ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0x31: // SLL C=(XY+o)
			_C = SLL(ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0x32: // SLL D=(XY+o)
			_D = SLL(ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0x33: // SLL E=(XY+o)
			_E = SLL(ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0x34: // SLL H=(XY+o)
			_H = SLL(ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0x35: // SLL L=(XY+o)
			_L = SLL(ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0x36: // SLL (XY+o)
			WriteMemory(EA, SLL(ReadMemory(EA)));
			break;
		case 0x37: // SLL A=(XY+o)
			_A = SLL(ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0x38: // SRL B=(XY+o)
			_B = SRL(ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0x39: // SRL C=(XY+o)
			_C = SRL(ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0x3a: // SRL D=(XY+o)
			_D = SRL(ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0x3b: // SRL E=(XY+o)
			_E = SRL(ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0x3c: // SRL H=(XY+o)
			_H = SRL(ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0x3d: // SRL L=(XY+o)
			_L = SRL(ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0x3e: // SRL (XY+o)
			WriteMemory(EA, SRL(ReadMemory(EA)));
			break;
		case 0x3f: // SRL A=(XY+o)
			_A = SRL(ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0x40: // BIT 0, B=(XY+o)
		case 0x41: // BIT 0, C=(XY+o)
		case 0x42: // BIT 0, D=(XY+o)
		case 0x43: // BIT 0, E=(XY+o)
		case 0x44: // BIT 0, H=(XY+o)
		case 0x45: // BIT 0, L=(XY+o)
		case 0x46: // BIT 0, (XY+o)
		case 0x47: // BIT 0, A=(XY+o)
			BIT_XY(0, ReadMemory(EA));
			break;
		case 0x48: // BIT 1, B=(XY+o)
		case 0x49: // BIT 1, C=(XY+o)
		case 0x4a: // BIT 1, D=(XY+o)
		case 0x4b: // BIT 1, E=(XY+o)
		case 0x4c: // BIT 1, H=(XY+o)
		case 0x4d: // BIT 1, L=(XY+o)
		case 0x4e: // BIT 1, (XY+o)
		case 0x4f: // BIT 1, A=(XY+o)
			BIT_XY(1, ReadMemory(EA));
			break;
		case 0x50: // BIT 2, B=(XY+o)
		case 0x51: // BIT 2, C=(XY+o)
		case 0x52: // BIT 2, D=(XY+o)
		case 0x53: // BIT 2, E=(XY+o)
		case 0x54: // BIT 2, H=(XY+o)
		case 0x55: // BIT 2, L=(XY+o)
		case 0x56: // BIT 2, (XY+o)
		case 0x57: // BIT 2, A=(XY+o)
			BIT_XY(2, ReadMemory(EA));
			break;
		case 0x58: // BIT 3, B=(XY+o)
		case 0x59: // BIT 3, C=(XY+o)
		case 0x5a: // BIT 3, D=(XY+o)
		case 0x5b: // BIT 3, E=(XY+o)
		case 0x5c: // BIT 3, H=(XY+o)
		case 0x5d: // BIT 3, L=(XY+o)
		case 0x5e: // BIT 3, (XY+o)
		case 0x5f: // BIT 3, A=(XY+o)
			BIT_XY(3, ReadMemory(EA));
			break;
		case 0x60: // BIT 4, B=(XY+o)
		case 0x61: // BIT 4, C=(XY+o)
		case 0x62: // BIT 4, D=(XY+o)
		case 0x63: // BIT 4, E=(XY+o)
		case 0x64: // BIT 4, H=(XY+o)
		case 0x65: // BIT 4, L=(XY+o)
		case 0x66: // BIT 4, (XY+o)
		case 0x67: // BIT 4, A=(XY+o)
			BIT_XY(4, ReadMemory(EA));
			break;
		case 0x68: // BIT 5, B=(XY+o)
		case 0x69: // BIT 5, C=(XY+o)
		case 0x6a: // BIT 5, D=(XY+o)
		case 0x6b: // BIT 5, E=(XY+o)
		case 0x6c: // BIT 5, H=(XY+o)
		case 0x6d: // BIT 5, L=(XY+o)
		case 0x6e: // BIT 5, (XY+o)
		case 0x6f: // BIT 5, A=(XY+o)
			BIT_XY(5, ReadMemory(EA));
			break;
		case 0x70: // BIT 6, B=(XY+o)
		case 0x71: // BIT 6, C=(XY+o)
		case 0x72: // BIT 6, D=(XY+o)
		case 0x73: // BIT 6, E=(XY+o)
		case 0x74: // BIT 6, H=(XY+o)
		case 0x75: // BIT 6, L=(XY+o)
		case 0x76: // BIT 6, (XY+o)
		case 0x77: // BIT 6, A=(XY+o)
			BIT_XY(6, ReadMemory(EA));
			break;
		case 0x78: // BIT 7, B=(XY+o)
		case 0x79: // BIT 7, C=(XY+o)
		case 0x7a: // BIT 7, D=(XY+o)
		case 0x7b: // BIT 7, E=(XY+o)
		case 0x7c: // BIT 7, H=(XY+o)
		case 0x7d: // BIT 7, L=(XY+o)
		case 0x7e: // BIT 7, (XY+o)
		case 0x7f: // BIT 7, A=(XY+o)
			BIT_XY(7, ReadMemory(EA));
			break;
		case 0x80: // RES 0, B=(XY+o)
			_B = RES(0, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0x81: // RES 0, C=(XY+o)
			_C = RES(0, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0x82: // RES 0, D=(XY+o)
			_D = RES(0, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0x83: // RES 0, E=(XY+o)
			_E = RES(0, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0x84: // RES 0, H=(XY+o)
			_H = RES(0, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0x85: // RES 0, L=(XY+o)
			_L = RES(0, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0x86: // RES 0, (XY+o)
			WriteMemory(EA, RES(0, ReadMemory(EA)));
			break;
		case 0x87: // RES 0, A=(XY+o)
			_A = RES(0, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0x88: // RES 1, B=(XY+o)
			_B = RES(1, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0x89: // RES 1, C=(XY+o)
			_C = RES(1, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0x8a: // RES 1, D=(XY+o)
			_D = RES(1, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0x8b: // RES 1, E=(XY+o)
			_E = RES(1, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0x8c: // RES 1, H=(XY+o)
			_H = RES(1, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0x8d: // RES 1, L=(XY+o)
			_L = RES(1, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0x8e: // RES 1, (XY+o)
			WriteMemory(EA, RES(1, ReadMemory(EA)));
			break;
		case 0x8f: // RES 1, A=(XY+o)
			_A = RES(1, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0x90: // RES 2, B=(XY+o)
			_B = RES(2, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0x91: // RES 2, C=(XY+o)
			_C = RES(2, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0x92: // RES 2, D=(XY+o)
			_D = RES(2, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0x93: // RES 2, E=(XY+o)
			_E = RES(2, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0x94: // RES 2, H=(XY+o)
			_H = RES(2, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0x95: // RES 2, L=(XY+o)
			_L = RES(2, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0x96: // RES 2, (XY+o)
			WriteMemory(EA, RES(2, ReadMemory(EA)));
			break;
		case 0x97: // RES 2, A=(XY+o)
			_A = RES(2, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0x98: // RES 3, B=(XY+o)
			_B = RES(3, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0x99: // RES 3, C=(XY+o)
			_C = RES(3, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0x9a: // RES 3, D=(XY+o)
			_D = RES(3, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0x9b: // RES 3, E=(XY+o)
			_E = RES(3, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0x9c: // RES 3, H=(XY+o)
			_H = RES(3, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0x9d: // RES 3, L=(XY+o)
			_L = RES(3, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0x9e: // RES 3, (XY+o)
			WriteMemory(EA, RES(3, ReadMemory(EA)));
			break;
		case 0x9f: // RES 3, A=(XY+o)
			_A = RES(3, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0xa0: // RES 4, B=(XY+o)
			_B = RES(4, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0xa1: // RES 4, C=(XY+o)
			_C = RES(4, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0xa2: // RES 4, D=(XY+o)
			_D = RES(4, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0xa3: // RES 4, E=(XY+o)
			_E = RES(4, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0xa4: // RES 4, H=(XY+o)
			_H = RES(4, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0xa5: // RES 4, L=(XY+o)
			_L = RES(4, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0xa6: // RES 4, (XY+o)
			WriteMemory(EA, RES(4, ReadMemory(EA)));
			break;
		case 0xa7: // RES 4, A=(XY+o)
			_A = RES(4, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0xa8: // RES 5, B=(XY+o)
			_B = RES(5, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0xa9: // RES 5, C=(XY+o)
			_C = RES(5, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0xaa: // RES 5, D=(XY+o)
			_D = RES(5, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0xab: // RES 5, E=(XY+o)
			_E = RES(5, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0xac: // RES 5, H=(XY+o)
			_H = RES(5, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0xad: // RES 5, L=(XY+o)
			_L = RES(5, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0xae: // RES 5, (XY+o)
			WriteMemory(EA, RES(5, ReadMemory(EA)));
			break;
		case 0xaf: // RES 5, A=(XY+o)
			_A = RES(5, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0xb0: // RES 6, B=(XY+o)
			_B = RES(6, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0xb1: // RES 6, C=(XY+o)
			_C = RES(6, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0xb2: // RES 6, D=(XY+o)
			_D = RES(6, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0xb3: // RES 6, E=(XY+o)
			_E = RES(6, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0xb4: // RES 6, H=(XY+o)
			_H = RES(6, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0xb5: // RES 6, L=(XY+o)
			_L = RES(6, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0xb6: // RES 6, (XY+o)
			WriteMemory(EA, RES(6, ReadMemory(EA)));
			break;
		case 0xb7: // RES 6, A=(XY+o)
			_A = RES(6, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0xb8: // RES 7, B=(XY+o)
			_B = RES(7, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0xb9: // RES 7, C=(XY+o)
			_C = RES(7, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0xba: // RES 7, D=(XY+o)
			_D = RES(7, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0xbb: // RES 7, E=(XY+o)
			_E = RES(7, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0xbc: // RES 7, H=(XY+o)
			_H = RES(7, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0xbd: // RES 7, L=(XY+o)
			_L = RES(7, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0xbe: // RES 7, (XY+o)
			WriteMemory(EA, RES(7, ReadMemory(EA)));
			break;
		case 0xbf: // RES 7, A=(XY+o)
			_A = RES(7, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0xc0: // SET 0, B=(XY+o)
			_B = SET(0, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0xc1: // SET 0, C=(XY+o)
			_C = SET(0, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0xc2: // SET 0, D=(XY+o)
			_D = SET(0, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0xc3: // SET 0, E=(XY+o)
			_E = SET(0, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0xc4: // SET 0, H=(XY+o)
			_H = SET(0, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0xc5: // SET 0, L=(XY+o)
			_L = SET(0, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0xc6: // SET 0, (XY+o)
			WriteMemory(EA, SET(0, ReadMemory(EA)));
			break;
		case 0xc7: // SET 0, A=(XY+o)
			_A = SET(0, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0xc8: // SET 1, B=(XY+o)
			_B = SET(1, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0xc9: // SET 1, C=(XY+o)
			_C = SET(1, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0xca: // SET 1, D=(XY+o)
			_D = SET(1, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0xcb: // SET 1, E=(XY+o)
			_E = SET(1, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0xcc: // SET 1, H=(XY+o)
			_H = SET(1, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0xcd: // SET 1, L=(XY+o)
			_L = SET(1, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0xce: // SET 1, (XY+o)
			WriteMemory(EA, SET(1, ReadMemory(EA)));
			break;
		case 0xcf: // SET 1, A=(XY+o)
			_A = SET(1, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0xd0: // SET 2, B=(XY+o)
			_B = SET(2, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0xd1: // SET 2, C=(XY+o)
			_C = SET(2, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0xd2: // SET 2, D=(XY+o)
			_D = SET(2, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0xd3: // SET 2, E=(XY+o)
			_E = SET(2, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0xd4: // SET 2, H=(XY+o)
			_H = SET(2, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0xd5: // SET 2, L=(XY+o)
			_L = SET(2, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0xd6: // SET 2, (XY+o)
			WriteMemory(EA, SET(2, ReadMemory(EA)));
			break;
		case 0xd7: // SET 2, A=(XY+o)
			_A = SET(2, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0xd8: // SET 3, B=(XY+o)
			_B = SET(3, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0xd9: // SET 3, C=(XY+o)
			_C = SET(3, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0xda: // SET 3, D=(XY+o)
			_D = SET(3, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0xdb: // SET 3, E=(XY+o)
			_E = SET(3, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0xdc: // SET 3, H=(XY+o)
			_H = SET(3, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0xdd: // SET 3, L=(XY+o)
			_L = SET(3, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0xde: // SET 3, (XY+o)
			WriteMemory(EA, SET(3, ReadMemory(EA)));
			break;
		case 0xdf: // SET 3, A=(XY+o)
			_A = SET(3, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0xe0: // SET 4, B=(XY+o)
			_B = SET(4, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0xe1: // SET 4, C=(XY+o)
			_C = SET(4, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0xe2: // SET 4, D=(XY+o)
			_D = SET(4, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0xe3: // SET 4, E=(XY+o)
			_E = SET(4, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0xe4: // SET 4, H=(XY+o)
			_H = SET(4, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0xe5: // SET 4, L=(XY+o)
			_L = SET(4, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0xe6: // SET 4, (XY+o)
			WriteMemory(EA, SET(4, ReadMemory(EA)));
			break;
		case 0xe7: // SET 4, A=(XY+o)
			_A = SET(4, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0xe8: // SET 5, B=(XY+o)
			_B = SET(5, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0xe9: // SET 5, C=(XY+o)
			_C = SET(5, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0xea: // SET 5, D=(XY+o)
			_D = SET(5, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0xeb: // SET 5, E=(XY+o)
			_E = SET(5, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0xec: // SET 5, H=(XY+o)
			_H = SET(5, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0xed: // SET 5, L=(XY+o)
			_L = SET(5, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0xee: // SET 5, (XY+o)
			WriteMemory(EA, SET(5, ReadMemory(EA)));
			break;
		case 0xef: // SET 5, A=(XY+o)
			_A = SET(5, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0xf0: // SET 6, B=(XY+o)
			_B = SET(6, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0xf1: // SET 6, C=(XY+o)
			_C = SET(6, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0xf2: // SET 6, D=(XY+o)
			_D = SET(6, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0xf3: // SET 6, E=(XY+o)
			_E = SET(6, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0xf4: // SET 6, H=(XY+o)
			_H = SET(6, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0xf5: // SET 6, L=(XY+o)
			_L = SET(6, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0xf6: // SET 6, (XY+o)
			WriteMemory(EA, SET(6, ReadMemory(EA)));
			break;
		case 0xf7: // SET 6, A=(XY+o)
			_A = SET(6, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
		case 0xf8: // SET 7, B=(XY+o)
			_B = SET(7, ReadMemory(EA));
			WriteMemory(EA, _B);
			break;
		case 0xf9: // SET 7, C=(XY+o)
			_C = SET(7, ReadMemory(EA));
			WriteMemory(EA, _C);
			break;
		case 0xfa: // SET 7, D=(XY+o)
			_D = SET(7, ReadMemory(EA));
			WriteMemory(EA, _D);
			break;
		case 0xfb: // SET 7, E=(XY+o)
			_E = SET(7, ReadMemory(EA));
			WriteMemory(EA, _E);
			break;
		case 0xfc: // SET 7, H=(XY+o)
			_H = SET(7, ReadMemory(EA));
			WriteMemory(EA, _H);
			break;
		case 0xfd: // SET 7, L=(XY+o)
			_L = SET(7, ReadMemory(EA));
			WriteMemory(EA, _L);
			break;
		case 0xfe: // SET 7, (XY+o)
			WriteMemory(EA, SET(7, ReadMemory(EA)));
			break;
		case 0xff: // SET 7, A=(XY+o)
			_A = SET(7, ReadMemory(EA));
			WriteMemory(EA, _A);
			break;
	}
}

