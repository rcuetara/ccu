/* The charge controller control program collection

ccsetdate: set date and time  for eSmart3 charge controllers
Copyright (C) 2019 Ramón A. De La Cuétara

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#ifdef linux

#define RESTORE_PORT tcsetattr(fd,TCSANOW,&oldset); close(fd);
#define PORT_EXAMPLE printf("/dev/ttyUSB0\n");

#include <fcntl.h>  /* File Control Definitions          */
#include <termios.h>/* POSIX Terminal Control Definitions*/
#include <unistd.h> /* UNIX Standard Definitions         */
#include <errno.h>  /* ERROR Number Definitions          */
/* 
getdata send a 10 byte read command to the cc
input: fd = file descriptor for the comm port
       group = 10 byte read command
       reply = address of buffer for returned data
output: number of bytes received or error, data returned in  buffer
*/
 int getdata( int fd, signed char group[10], char *reply)
     {
     	int replysize;
      int Sent = 0;
      int Received;
    	replysize=group[8]+9; 
     	Sent= write(fd,group,10);
     	if(Sent == 10){
     	sleep(1); 
   	Received = read (fd, reply, replysize);}
   	else Received = 0;
     	return Received;
     }

/* 
setdata send a 25 byte write command to the cc
input: fd = file descriptor for the comm port
       group = 25 byte read command with first 6 bytes initialized, rest zeroed
       reply = address of buffer for returned packet
output: number of bytes received or error, packet in  buffer
*/
int setdata( int fd, signed char group[25], char *reply)
     {
      int Sent = 0;
      int Received;
     	Sent= write(fd,group,25);
     	if(Sent == 25){
     	sleep(1);
   	Received = read (fd, reply, 7); }
   	else Received = 0;
      return Received;
     }
  struct termios oldset,newset;	
  int fd;
#endif

#ifdef windows

#define RESTORE_PORT SetCommState(fd, &oldset); CloseHandle(fd);
#define PORT_EXAMPLE printf("COM5\n");

#include <windows.h>
/* 
getdata send a 10 byte read command to the cc
input: fd = file handle for the comm port
       group = 10 byte read command
       reply = address of buffer for returned data
output: number of bytes received or error, data returned in  buffer
*/
 int getdata( HANDLE fd, signed char group[10], char *reply)
     {
     	int replysize;
      int Sent = 0;
      int Received;
      bool rwok;
    	replysize=group[8]+9;
    	rwok = WriteFile(fd,group,10,&Sent,NULL);
     	if(rwok && (Sent == 10)){
     	Sleep(1);	
     	rwok = ReadFile( fd,reply,replysize,&Received,NULL);
     	if (!rwok) Received=0; }
   	else Received = 0;
     	return Received;
     }

/* 
setdata send a 25 byte write command to the cc
input: fd = file handle for the comm port
       group = 25 byte read command with first 6 bytes initialized, rest zeroed
       reply = address of buffer for returned packet
output: number of bytes received or error, packet in  buffer
*/
int setdata( HANDLE fd, signed char group[25], char *reply)
     {
      int Sent = 0;
      int Received;
      bool rwok;
    	rwok = WriteFile(fd,group,25,&Sent,NULL);
     	if(rwok && (Sent == 25)){
     	Sleep(1);	
     	rwok = ReadFile( fd,reply,7,&Received,NULL);
     	if (!rwok) Received=0; }
      else Received = 0;
      return Received;
     }
  bool status;
  HANDLE fd;
  DCB oldset = { 0 };
  DCB newset = { 0 };
  COMMTIMEOUTS touts = { 0 };
#endif

struct charger_data {
    char serial[9],
         date[9],
         version[5],
         model[17]; };

void main(int argc, char *argv[])
{ 
  struct charger_data charger; 
  signed char readDeviceData[10]	=    {0xaa, 1 , 1, 1, 8, 3, 0, 0, 38,   34 };
  signed char setDateTime[25] ={0xaa, 1, 1, 2, 6, 18, 0, 0, 85, -86, 4, 0, 
                                0,0,0,0,0,0,0,0,0,0,0,0,0};
  short date[6];
  void *  pReply ;
  char Reply[128];
  char yn;  
  int i,bytes_read;
  bool found;
  time_t T;
  struct  tm m;
  found=false;
  pReply = &Reply;
  bzero(&charger, sizeof(charger));
  printf("ccsetdate: Set date and time for eSmart3 charge controller\n ");
  if (argc != 2) { printf("\nusage: ccpars port, example: ccpars "); PORT_EXAMPLE
                   return;}
#ifdef linux
  fd=open(argv[1],O_RDWR | O_NOCTTY ); 
  if(fd == -1) { printf("Failed to open comm port.\n");
                 return;}
//setup port 9600 8N1 
        tcgetattr(fd, &oldset); //save old port settings
        bzero(&newset, sizeof(newset));
        newset.c_cflag = B9600 | CRTSCTS | CS8 | CLOCAL | CREAD;
        newset.c_iflag = IGNPAR;
        newset.c_oflag = 0;
        newset.c_lflag = 0;
        newset.c_cc[VTIME]    = 0;    
        newset.c_cc[VMIN]     = 128;  
        cfsetispeed(&newset,B9600);
        cfsetospeed(&newset,B9600);
        tcflush(fd, TCIFLUSH);
        tcsetattr(fd,TCSANOW,&newset);
        fcntl(fd, F_SETFL, FNDELAY);
#endif
#ifdef windows
  oldset.DCBlength = sizeof(oldset);
  newset.DCBlength = sizeof(newset);
  fd = CreateFile(argv[1],GENERIC_READ | GENERIC_WRITE, 0,NULL,OPEN_EXISTING,0,NULL);
  if (fd == INVALID_HANDLE_VALUE) { printf("Failed to open comm port.\n");
                                    return;}
//setup port 9600 8N1 
         status = GetCommState(fd, &newset);
         oldset=newset;
         newset.BaudRate = CBR_9600;
         newset.ByteSize = 8;
         newset.StopBits = ONESTOPBIT;
         newset.Parity   = NOPARITY;  
         SetCommState(fd, &newset);
         touts.ReadIntervalTimeout         = 50; 
         touts.ReadTotalTimeoutConstant    = 50;
         touts.ReadTotalTimeoutMultiplier  = 10;
         touts.WriteTotalTimeoutConstant   = 50;
         touts.WriteTotalTimeoutMultiplier = 10;
         SetCommTimeouts(fd, &touts); 
          
#endif
//check if charge controller connected to this port
        bytes_read= getdata(fd,readDeviceData,pReply);
        if ((bytes_read == 47) && (Reply[0] == -86)){
        	  memcpy( &charger.serial, &Reply[10],8);
        	  memcpy( &charger.date, &Reply[18],8);
        	  memcpy( &charger.version, &Reply[26],4);
        	  memcpy( &charger.model, &Reply[30],16);
        	  printf("\nFound charger %s  serial %s date %s version %s\n", charger.model,
        	          charger.serial,charger.date,charger.version);
        	  found=true;} 
           else{
           printf("Charge Controller not found.\n");
           RESTORE_PORT //restore old port settings
           }   	
 if (!found)return;          
 T= time(NULL);
 m = *localtime(&T);
 date[0]=m.tm_year+1900;
 date[1]=m.tm_mon+1;
 date[2]=m.tm_mday;
 date[3]=m.tm_hour;
 date[4]=m.tm_min;
 date[5]=m.tm_sec;
 memcpy(&setDateTime[12],&date,12);
 for(i=0;i<24;++i) setDateTime[24]-=setDateTime[i]; //calculate checksum
 i= setdata(fd,setDateTime,pReply);
 if ((Reply[3] == 0) && ( i == 7) && (Reply[0] == -86))printf("\nDate and time set to %s\n",ctime(&T));
     else printf("Set failed \n");
 RESTORE_PORT
 return;
 }//end of program 

