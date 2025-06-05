//============================================================================
//  main.cpp "Main routine"
//----------------------------------------------------------------------------
//													Programmed by Keiichi Tago
//============================================================================
#include "main.h"
#include "../common/debug.h"
#include "system.h"

void InitMain( void )
{
	InitRender();
	Clear();
}

void ReleaseMain( void )
{
	ReleaseRender();
}

// Out : false = continue, true = end
bool Main( void )
{
	Flip();
	Clear();
	RunCPU();
	return false;
}
