// cccal: the charge controller calibration program for eSmart3 charge controllers
#include <stdio.h>
#include <fcntl.h>  /* File Control Definitions          */
#include <termios.h>/* POSIX Terminal Control Definitions*/
#include <unistd.h> /* UNIX Standard Definitions         */
#include <errno.h>  /* ERROR Number Definitions          */
#include <stdbool.h>
#include <string.h>


struct charger_data {
    char serial[9],
         date[9],
         version[5],
         model[17]; };

 int getdata( int fd, signed char group[10], char *reply)
     {
     	int replysize;
      int Sent = 0;
      int Received;
    	replysize=group[8]+9;
     	Sent= write(fd,group,10);
     	sleep(1); // allow for a long reply
   	Received = read (fd, reply, replysize);
     	return Received;
     }

int setdata( int fd, signed char group[11], unsigned short parameter, unsigned short pvalue, char *reply)
     {
      int Sent = 0;
      int Received;
      group[6] = parameter;
      memcpy(&group[8],&pvalue,2);
      group[10] = -(-75+group[6]+group[8]+group[9]); //calculate checksum
     	Sent= write(fd,group,11);
     	sleep(1);
   	Received = read (fd, reply, 7);
     	return Received;
     }


void main(int argc, char *argv[])
{ 
  struct termios oldset,newset;	
  struct charger_data charger; 
  signed char temp;
  signed char readRunningData[10]	= {0xaa, 1 , 1, 1, 0, 3, 0, 0, 0x1e, 0x32 };
  signed char readDeviceData[10]	=    {0xaa, 1 , 1, 1, 8, 3, 0, 0, 38,   34 };
  signed char readSamplingParameters[10]	=    {0xaa, 1 , 1, 1, 3, 3, 0, 0, 26, 51 };
  signed char setSamplingParameters[11] =  {0xaa, 1 , 1, 2, 3 ,4 ,0, 0, 0, 0, 0};
  char parameter_name[12][25]={ "PV voltage ratio      ","offset",
                           "Battery voltage ratio ","offset",
                           "Charge current ratio  ","offset",
                           "Load current ratio    ","offset",
                           "Load voltage ratio    ","offset",
                           "Out voltage  ratio    ","offset"};
                       

  short parameters[12]; //parameter list
  short new_parameter,choice;
  void *  pReply ;
  char Reply[128];
  char yn;  
  int i,j,fd, bytes_read;
  unsigned short running_data[6]; 
  bool found;

  printf("ccpars: Modify eSmart3 charge controller sampling parameters\n ");
  if (argc == 1) printf("\nusage: ccpars port, example: ccpars /dev/ttyUSB0\n");
  else {
  found=false;
  pReply = &Reply;
  bzero(&charger, sizeof(charger));
  fd=open(argv[1],O_RDWR | O_NOCTTY ); 
  if(fd == -1) printf("Failed to open comm port.\n");
     else {
        //setup port 9600 8N1 
        tcgetattr(fd, &oldset);
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
        //check if charge controller connected to this port
        bytes_read= getdata(fd,readDeviceData,pReply);
        if ((bytes_read == 47) && (Reply[0] == -86)){
        	  memcpy( &charger.serial, &Reply[10],8);
        	  memcpy( &charger.date, &Reply[18],8);
        	  memcpy( &charger.version, &Reply[26],4);
        	  memcpy( &charger.model, &Reply[30],16);
        	  printf("\n Found charger %s  serial %s date %s version %s\n", charger.model,
        	          charger.serial,charger.date,charger.version);
        	  found=true;        
        	  } 
           else{
           printf("Charge Controller not found.\n");
           tcsetattr(fd,TCSANOW,&oldset);
           close(fd);
           }   	
           
     }  
   if (found) 
	    { do 
       	{
       do {	
       bytes_read= getdata(fd,readSamplingParameters,pReply);
       if((bytes_read == 35) && (Reply[0] == -86))	memcpy( &parameters, &Reply[10],24);
         else{} //ToDo add error code
        bytes_read= getdata(fd,readRunningData,pReply);
        if((bytes_read == 39) && (Reply[0] == -86))	
        {memcpy( &running_data, &Reply[10],12);
        //swap Out Volts and Load current positions
        choice=running_data[5];running_data[5] = running_data[3];running_data[3]=choice;
        }else{} //ToDo add error code
        printf("\n Select Sampling Parameter to modify\n");
        printf(" 0 - Exit Program\n");
        for (i=0;i<12;i+=2) printf("%2d-%s(%5d)    %2d-%s(%5d) reading %3.1f \n",i+1,parameter_name[i],parameters[i],
                                    i+2,parameter_name[i+1],parameters[i+1],(float)running_data[i/2]/10.0);  
        printf("Enter a value from 0 to 12: ");
        scanf("%d",&choice);        
       } 
       while ((choice < 0) || (choice > 12));
        if(choice == 0)  break;
        printf("Modify %s \nCurrent value %d  Enter new value: ",parameter_name[choice-1], parameters[choice-1]);
        scanf("%d",&new_parameter);
        printf("\nChange %s to %d, confirm Y/N:",parameter_name[choice-1], new_parameter);
        scanf(" %c",&yn);
        if(( yn == 'Y') || ( yn == 'y'))
          {bytes_read= setdata( fd, setSamplingParameters, choice, new_parameter, pReply);
           if (Reply[3] == 0) printf("\nParameter Changed\n");
           else printf("Modify failed \n");
          }   
       }
       while (choice != 0);
       }
       tcsetattr(fd,TCSANOW,&oldset);
       close(fd);
   }    
return;
}    
