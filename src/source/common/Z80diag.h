// ---------------------------------------------------------------------------
//	Z80 Disassembler
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: Z80diag.h,v 1.3 1999/10/10 01:46:26 cisc Exp $
 
#ifndef Z80DIAG_H
#define Z80DIAG_H

// ---------------------------------------------------------------------------

typedef unsigned char uint8;
typedef unsigned int uint;
typedef signed char int8;

uint Disassemble(uint pc, char* dest);
uint InstInc(uint ad);
uint InstDec(uint ad);

enum XMode { usehl=0, useix=2, useiy=4 };

char* Expand(char* dest, const char* src);
static void SetHex(char*& dest, uint n);
int GetInstSize(uint ad);
uint InstCheck(uint ad);
uint InstDecSub(uint ad, int depth);

#endif // Z80DIAG_H
