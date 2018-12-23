#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <vector>
#include <iostream>
using namespace std;

//Message Queue Struct
struct msgbuff
{
	long mtype;
	char mtext[64];
	int mOpMask;
};

key_t disk_id;

key_t upqid;
key_t downqid;

// Variable to Get System Time
	time_t now;
	struct tm *tm;
	int clockCycles;
	int currentTime;

//Logfile parameters
	FILE *f;

void kernelDead(int signum)
{
	cout<<"Kernel....\nInterrupt Signal #"<<signum<<" received" <<endl;

	cout<<"Deleting message Queues..."<<endl;
	  
  	msgctl(upqid,IPC_RMID,(struct msqid_ds *)0);

  	msgctl(downqid,IPC_RMID,(struct msqid_ds *)0);

	cout<<"Message Queues deleted ... Dead Kernel"<<endl;

	fclose(f);

  	exit(-1);
}

void incrementClk()
{
	// Get Time in seconds
	  	now = time(0);
	  	tm = localtime (&now);
	  	if (tm->tm_sec != currentTime)
	  	{
	  		currentTime = tm->tm_sec;

		  	killpg(1001, SIGUSR2);

		  	kill(disk_id,SIGUSR2);
	  	}
	
}

void waitDiskCreation()
{
	struct msgbuff message;

	int rec_val = msgrcv(upqid,&message,sizeof(message.mtext),2,!IPC_NOWAIT);
	if (rec_val != -1 )
	{
		if (message.mtype == 1)
			{
				disk_id = stoi(message.mtext);
				cout<<"Disk created Successfully\n";
			}
		else
			cout<<"Message receieved but not the expected type"<<endl;
	}

}


msgbuff requestDiskStatus()
{
	//Send signal to Disk to check status 
	kill(disk_id,SIGUSR1);

	// Wait for disk to respond with status
		msgbuff messageDiskStatus;
		int rec_val = msgrcv(upqid,&messageDiskStatus,sizeof(messageDiskStatus.mtext),1,!IPC_NOWAIT);
		// Recieved mtype contains 0 -> status message
		// Recieved mtext contains No. of free slots
		// Recieved mOpMask 1s at used slots that are used EXAMPLE "9 8 7 6 4 3 2 1 0 " 

		if (rec_val != -1)
			cout<<"Cannot receive status from disk"<<endl;

	return messageDiskStatus;
}

int main()
{

	//Signal Handler when Kernel dies
		signal (SIGINT, kernelDead);

	// Create Message Queues
		upqid = msgget(777,IPC_CREAT|0644);

		downqid = msgget(778,IPC_CREAT|0644);


	clockCycles = 0;

	now = time(0);
  	tm = localtime (&now);
  	currentTime = tm->tm_sec;

	
	//Create Log File
		f = fopen("logfile.txt", "w");
		if (f == NULL)
		{
		    cout<<"Error opening log file!\n";
		    exit(1);
		}



	cout<<"Kernel created\n";


	cout<<"Kernel waiting for disk to be created\n";

	
	//Waiting Disk to be Created
		int rec_val,send_val;

	waitDiskCreation();		  


  	while (1)
  	{

 		incrementClk();	

		msgbuff message;
		// check if message recieved from process BUT NOT WAIT FOR IT
  			rec_val = msgrcv(upqid,&message,sizeof(message.mtext),0,IPC_NOWAIT);

  		if (rec_val != -1 && message.mtype != 0 && message.mtype != 1)
  		{
  			//Something receieved from process

			// message.mtype equals process id
  			// message.mtext equals "HELLO"
  			// message.mOpMask 0 for Add , 1 for delete

  			  			
			
			msgbuff messageDiskStatus = requestDiskStatus();

			// Status recieved from Disk
			msgbuff messageRequestToDisk;

			messageRequestToDisk.mtype = message.mOpMask; //0 -> ADD  1-> DEL
			// messageRequestToDisk.mtext = message.mtext; //Hello
			strcpy(messageRequestToDisk.mtext,message.mtext);
			// message.mtext = '';
			strcpy(message.mtext,"");
			messageRequestToDisk.mOpMask = 0; // 0 if valid 1 if not valid

			// Check if it is valid to do the operation
				if (message.mOpMask == 0) // Add operation
				{
					if (stoi(messageDiskStatus.mtext) == 0)
					{
						 // if no empty slots
						message.mtext[0] = '2';
						messageRequestToDisk.mOpMask = 1;
					}
					else
					{
						// empty slot exists
						message.mtext[0] = '1';
					}
				}
				else
				{	
					
					int state = messageDiskStatus.mOpMask;
					int deletedSlot = stoi(message.mtext);
					if ((state >> deletedSlot) & 1)
					{
						// Slot is used
						message.mtext[0] = '3';
					}
					else
					{
						// Slot is empty
						messageRequestToDisk.mOpMask = 1;
						message.mtext[0] = '4';

					}
					
				}

			send_val = msgsnd(downqid,&messageRequestToDisk,sizeof(messageRequestToDisk.mtext),IPC_NOWAIT);
			if (send_val == -1)
			{
				cout<<"Cannot send message to Disk to do perform the operation"<<endl;
			}
			else
			{
				cout<<"Operation message was sent to Disk successfully"<<endl;
			}

			// Send feedback to process with mtype = process_id , mtext = response message
				send_val = msgsnd(downqid,&message,sizeof(message.mtext),IPC_NOWAIT);
			if (send_val == -1)
			{
				cout<<"Cannot send feedback to process"<<endl;
			}
			else
			{
				cout<<"feedback message was sent to Process successfully"<<endl;
			}
				
	  		
		
			
			
  		}
  	
 
	}

	return 0;
}