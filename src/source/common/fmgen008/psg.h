// ---------------------------------------------------------------------------
//	PSG-like sound generator
//	Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//	$Id: psg.h,v 1.8 2003/04/22 13:12:53 cisc Exp $

#ifndef PSG_H
#define PSG_H

#include "types.h" // Addition

#define PSG_SAMPLETYPE		int32		// int32 or int16

// ---------------------------------------------------------------------------
//	class PSG
//	A sound source unit that produces sounds similar to PSG
//	
//	interface:
//	bool SetClock(uint clock, uint rate)
//		Initialization. Must read before using this class.
//		Set the PSG clock and PCM rate
//
//		clock:	PSG operating clock
//		rate:	Generate PCM rate
//		retval	True if initialization is successful
//
//	void Mix(Sample* dest, int nsamples)
//		Composites nsamples worth of PCM and add them to the array starting at dest.
//      Since this is just an addition, it is necessary to clear the array to zero first.
//	
//	void Reset()
//		Reset
//
//	void SetReg(uint reg, uint8 data)
//		Write data to register reg
//	
//	uint GetReg(uint reg)
//		Read the contents of the register reg
//	
//	void SetVolume(int db)
//		Adjust the volume of each sound source in units of approximately 1/2 dB.
//
class PSG
{
public:
	typedef PSG_SAMPLETYPE Sample;
	
	enum
	{
		noisetablesize = 1 << 11,	// <- If you want to reduce memory usage, reduce it
		toneshift = 24,
		envshift = 22,
		noiseshift = 14,
		oversampling = 2,		// <- If speed is more important than sound quality, you may want to reduce it.
	};

public:
	PSG();
	~PSG();

	void Mix(Sample* dest, int nsamples);
	void SetClock(int clock, int rate);
	
	void SetVolume(int vol);
	void SetChannelMask(int c);
	
	void Reset();
	void SetReg(uint regnum, uint8 data);
	uint GetReg(uint regnum) { return reg[regnum & 0x0f]; }

protected:
	void MakeNoiseTable();
	void MakeEnvelopTable();
	static void StoreSample(Sample& dest, int32 data);
	
	uint8 reg[16];

	const uint* envelop;
	uint olevel[3];
	uint32 scount[3], speriod[3];
	uint32 ecount, eperiod;
	uint32 ncount, nperiod;
	uint32 tperiodbase;
	uint32 eperiodbase;
	uint32 nperiodbase;
	int volume;
	int mask;

	static uint enveloptable[16][64];
	static uint noisetable[noisetablesize];
	static int EmitTable[32];
};

#endif // PSG_H
