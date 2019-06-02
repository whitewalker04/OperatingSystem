#include<stdio.h>
#include<conio.h>
#include<dos.h>
#include<stdlib.h>

struct Time
{
	int hours;
	int minutes;
};

struct Date
{
	int month;
	int day;
	int year;
};

struct Date getDate();
int writechar();
struct Time getTime();
void interrupt (*old9)();
void interrupt newint9();
void displayTime();
void removeTime();

char far *scr=(char far*)0xb8000000;
int count = 0 ;
union REGS InReg;
union REGS OutReg;
struct SREGS sregs;

void main()
{
	printf("***** Press shift-alt-d : display time and date,  ctrl-alt-d : to remove it ********");
	old9=getvect(0x09);
	setvect(0x09,newint9);
	keep(0,1500);
}

void interrupt newint9()
{
	InReg.x.ah = 0x12;
	int86(0x16, &InReg, &OutReg);
	if(OutReg.x.al & 0x02)
	{
	if(OutReg.x.al & 0x08)
	{
		InReg.x.al=inportb(0x60);
		if(InReg.x.al==32)
			displayTime();
	}
	count++;
	}
	if((OutReg.x.al & 0x04) && count>0)
	{
		if(OutReg.x.al & 0x08)
		{
			InReg.x.al=inportb(0x60);
			if(InReg.x.al==32)
			removeTime();
		}
	}
	(*old9)();
}

void displayTime()
{
	struct Date d;
	struct Time t;
	writechar();
	d = getDate();
	t = getTime();
	*(scr+(11*2))=(unsigned char)((d.month/10)+0x30);
	*(scr+(12*2))=(unsigned char)((d.month%10)+0x30);
	*(scr+(13*2))='/';
	*(scr+(14*2))=(unsigned char)((d.day/10)+0x30);
	*(scr+(15*2))=(unsigned char)((d.day%10)+0x30);
	*(scr+(16*2))='/';
	*(scr+(17*2))='2';
	*(scr+(18*2))='0';
	*(scr+(19*2))='1';
	*(scr+(20*2))=(unsigned char)((d.year%10)+0x30);
	*(scr+(10*2))=' ';
	*(scr+(21*2))=' ';
	if(t.hours>=12)
		{
			if(t.hours>12)
			t.hours=t.hours-12;
		*(scr+(8*2))='P';
		}	
	else
			*(scr+(8*2))='A';
	if(t.hours==0)
		t.hours=t.hours+12;
	*(scr+(2*2))=(unsigned char)((t.hours/10)+0x30);
	*(scr+(3*2))=(unsigned char)((t.hours%10)+0x30);
	*scr+(4*2))=':';
	*(scr+(5*2))=(unsigned char)((t.minutes/10)+0x30);
	*(scr+(6*2))=(unsigned char)((t.minutes%10)+0x30);
	*(scr+(7*2))=' ';
	*(scr+(9*2))='M';
}

struct Date getDate()
{
	struct Date t;
	InReg.x.ah=0x2a;
	int86(0x21,&InReg,&OutReg);
	t.month=OutReg.x.dh;
	t.day=OutReg.x.dl;
	t.year=OutReg.x.cx;
	return t;
}

int writechar()
{
	int index;
	for(index=2;index<=25;index++)
	{
		*(scr+(index*2)+1)=0x70;
	}
return 0;
}

struct Time getTime()
{
	struct Time t;
	InReg.x.ah=0x2c;
	int86(0x21,&InReg,&OutReg);
	t.hours=OutReg.x.ch;
	t.minutes=OutReg.x.cl;
	return t;
}

void removeTime()
{
	int index;
	for(index=2;index<22;index++)
		*(scr+(index*2))=' ';
	for(index=2;index<=25;index++)
	{
		*(scr+(index*2)+1)=0x00;
	}
}		
	