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

// //Up and Down Message Queue for Processes
// 	key_t upqidProcess;
// 	key_t downqidProcess;

// //Up and Down Message Queue for Disk
// 	key_t upqidDisk;
// 	key_t downqidDisk;

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

		struct msgbuff message;

		message.mtype = 0;
		
		while(message.mtype != 1)
		{
			rec_val = msgrcv(upqidDisk,&message,sizeof(message.mtext),0,!IPC_NOWAIT);
			if (rec_val != -1 )
			{
				if (message.mtype == 1)
					disk_id = stoi(message.mtext);
				else
					cout<<"Message receieved but not the expected type"<<endl;
			}
			else
				cout<<"Cannot recieve message from up queue"<<endl;
			
		}

	cout<<"Disk created Successfully\n";
		  


  	while (1)
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
  		

		
		// Recieve from process BUT NOT WAIT FOR IT
  			rec_val = msgrcv(upqid,&message,sizeof(message.mtext),1,IPC_NOWAIT);

  		if (rec_val != -1 && message.mtype != 0 && message.mtype != 1)
  		{
  			//Something receieved from process

			// message.mtype equals process id
  			// message.mtext equals "HELLO"
  			// message.mOpMask 0 for Add , 1 for delete

  			  			
		
			//Send signal to Disk to check status 
				kill(disk_id,SIGUSR1);

			// Wait for disk to respond with status
				struct msgbuff messageDiskStatus;
				rec_val = msgrcv(upqid,&messageDiskStatus,sizeof(messageDiskStatus.mtext),0,!IPC_NOWAIT);
				// Recieved mtype contains 0 -> status message
				// Recieved mtext contains No. of free slots
				// Recieved mOpMask 1s at used slots that are used EXAMPLE "9 8 7 6 4 3 2 1 0 " 


			if (rec_val != -1)
	  		{
	  			// Status recieved from Disk



	  	// 		if (message.mtext[0] == 'A') //Process created before and wants to add or something
				// { // Message is ADD
				// 	if (messageDiskStatus.mtype == 0)
				// 		message.mtext = string(2);	
				// 	else
				// 		message.mtext = string(0);

				// }
	  	// 		else if (message.mtext[0] == 'D')
				// {	// Message is DEL
				// 	int j =0;
				// 	for (j =0;j<messageDiskStatus.mtext.size();j++)
				// 		if (messageDiskStatus.mtext[j] == message.mtext[1])
				// 			break;
				// 	if (j != messageDiskStatus.mtext.size())
				// 		message.mtext=string(1);
				// 	else
				// 		message.mtext=string(3);

				// }
				// else
				// {
				// 	cout<<"Process is asking for something else not ADD OR DEL !!"<<endl;
				// }
				
				



				// Send feedback to process with mtype = process_id , mtext = response message
				send_val = msgsnd(downqid,&message,sizeof(message.mtext),IPC_NOWAIT);
				
	  		}
	  		else 
	  			cout<<"Kernel Cannot recieve status from DISK"<<endl;
	  		
		
			}
			
  		}
  	
 
	}

	return 0;
}