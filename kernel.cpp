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
	vector<int> processes;

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
		    printf("Error opening log file!\n");
		    exit(1);
		}



	struct msgbuff message;

	//Waiting Disk to be Created
		int rec_val,send_val;
		rec_val = msgrcv(upqidDisk,&message,sizeof(message.mtext),0,!IPC_NOWAIT);
		disk_id = message.mtext;
    		
		  
  	while (1)
  	{
 		// Get Time in seconds
		  	now = time(0);
		  	tm = localtime (&now);
		  	if (tm->tm_sec != currentTime)
		  	{
		  		currentTime = tm->tm_sec;

		  		//Send SIGUSR2 to all processes every second
		  		for (int i=0;i<processes.size(),i++)
			  		kill(processes(i), SIGUSR2);

			  	kill(disk_id,SIGUSR2);
		  	}
  		

		
		// Recieve from process
  		rec_val = msgrcv(upqidProcess,&message,sizeof(message.mtext),0,IPC_NOWAIT);

  		if (rec_val != -1)
  		{
  			//Something receieved from process
  			
  			if (message.mtype == -1) //Process first time creation
  			{
   				processes.push_back(stoi(message.mtext));
  			}
  			else
  			{
  				// message.mtype equals process id

				//Send signal to Disk to check status 
					kill(disk_id,SIGUSR1);

				// Wait for disk to respond with status
					struct msgbuff messageDiskStatus;
					rec_val = msgrcv(upqidDisk,&messageDiskStatus,sizeof(messageDiskStatus.mtext),0,!IPC_NOWAIT);
					// Recieved mtype contains number of free slots in Disk
					// Recieved mtext contains ids of slots that are used EXAMPLE "9 8 7 6 4 3 2 1 0 " 

				if (rec_val != -1)
		  		{
		  			if (message.mtext[0] == 'A') //Process created before and wants to add or something
					{ // Message is ADD
						if (messageDiskStatus.mtype == 0)
							message.mtext = string(2);	
						else
							message.mtext = string(0);

					}
		  			else if (message.mtext[0] == 'D')
					{	// Message is DEL
						int j =0;
						for (j =0;j<messageDiskStatus.mtext.size();j++)
							if (messageDiskStatus.mtext[j] == message.mtext[1])
								break;
						if (j != messageDiskStatus.mtext.size())
							message.mtext=string(1);
						else
							message.mtext=string(3);

					}
					else
					{
						cout<<"Process is asking for something else not ADD OR DEL !!"<<endl;
					}
					
					// Send feedback to process with mtype = process_id , mtext = response message
					send_val = msgsnd(downqidProcess,&message,sizeof(message.mtext),IPC_NOWAIT);
					
		  		}
		  		else 
		  			cout<<"Kernel Cannot recieve value from DISK"<<endl;
		  		
			
			}
			
  		}
  	
 
	}

	return 0;
}