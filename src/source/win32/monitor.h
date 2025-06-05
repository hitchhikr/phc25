#ifndef MONITOR_H
#define MONITOR_H

extern int Monitor_Opened;
extern int Z80_Pause;
extern int Z80_Step;
extern int Breakpoints[8];

void Monitor_Open();
void Monitor_Close();
int Monitor_Is_Opened();
void Print_Registers_From_Outside(int show_breakpoint);

#endif

