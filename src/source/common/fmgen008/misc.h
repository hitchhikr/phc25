// ---------------------------------------------------------------------------
//	misc.h
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: misc.h,v 1.5 2002/05/31 09:45:20 cisc Exp $

#ifndef MISC_H
#define MISC_H

inline int Limit(int v, int max, int min) 
{ 
	return v > max ? max : (v < min ? min : v); 
}

#endif // MISC_H

