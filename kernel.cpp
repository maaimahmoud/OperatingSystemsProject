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

int disk_id;

key_t upqid;
key_t downqid;

// Variable to Get System Time
	time_t now;
	struct tm *tm;
	int clockCycles;
	int currentTime;

//Logfile parameters
	FILE *f;

//Messages
	struct msgbuff message;
	struct msgbuff messageDiskStatus;
	struct msgbuff messageRequestToDisk;

	int rec_val,send_val;

	bool diskBusy = false;	  



vector<int>connectedProcesses;

void kernelDead(int signum)
{
	cout<<"\nKernel....\nInterrupt Signal #"<<signum<<" received" <<endl;

	cout<<"Deleting message Queues..."<<endl;
	  
  	msgctl(upqid,IPC_RMID,(struct msqid_ds *)0);

  	msgctl(downqid,IPC_RMID,(struct msqid_ds *)0);

	cout<<"Message Queues deleted ... Dead Kernel"<<endl;

	fclose(f);

  	exit(-1);
}


////////////////////////////////////////////////////////////////////

void send_message(msgbuff sentMessage,int n)
{
	string s;
	if (n == 0)
		s = "process";
	else
		s = "disk";

	send_val = msgsnd(downqid,&sentMessage,sizeof(sentMessage),IPC_NOWAIT);
				
	if (send_val == -1)
		cout<<"Cannot send message to process"<<endl;

}

////////////////////////////////////////////////////////////////////

void incrementClk()
{
	// Get Time in seconds
	  	now = time(0);
	  	tm = localtime (&now);
	  	if (tm->tm_sec != currentTime)
	  	{
	  		currentTime = tm->tm_sec;

	  		for (int i = 0; i < connectedProcesses.size(); ++i)
	  		{
	  			kill(connectedProcesses[i],SIGUSR2);
	  		}

		  	kill(disk_id,SIGUSR2);
	  	}
	
}

////////////////////////////////////////////////////////////////////

int convertCharArrToInt(char* arr)
{
	string str(arr);

	return stoi(str);
}

////////////////////////////////////////////////////////////////////

void waitDiskCreation()
{
	struct msgbuff message;

	int rec_val = msgrcv(upqid,&message,sizeof(message.mtext),1,!IPC_NOWAIT);
	if (rec_val != -1 )
	{
		disk_id = convertCharArrToInt(message.mtext);
		cout<<"Disk created Successfully with id = "<<disk_id<<endl;
	}
	else
		cout<<"Cannnot recieve from Disk"<<endl;

}

////////////////////////////////////////////////////////////////////

struct msgbuff requestDiskStatus()
{

	//Send signal to Disk to check status 
	kill(disk_id,SIGUSR1);

	// Wait for disk to respond with status
		struct msgbuff messageDiskStatus;
		int rec_val = msgrcv(upqid,&messageDiskStatus,sizeof(messageDiskStatus),3,!IPC_NOWAIT);

		if (rec_val == -1)
			cout<<"Cannot receive status from disk"<<endl;

	return messageDiskStatus;
}

////////////////////////////////////////////////////////////////////


void handleProcessOperation()
{
	// Check if it is valid to do the operation

	if (message.mOpMask == 0) // Add operation
	{

		if (convertCharArrToInt(messageDiskStatus.mtext) == 0)
			message.mOpMask = 2; // if no empty slots
		else
		{
			// empty slot exists
			message.mOpMask = 1;
			diskBusy = true;
		}
	}
	else
	{	
		
		int state = messageDiskStatus.mOpMask;
		int deletedSlot = (int)( message.mtext[0]);


		if ((state >> deletedSlot) & 1)
		{
			// Slot is used
			message.mOpMask = 3;
			diskBusy = true;
		}
		else
			message.mOpMask = 4; // Slot is empty
		
	}
}

////////////////////////////////////////////////////////////////////

// msgbuff createCopy(msgbuff copied)
// {
// 	msgbuff newMsg;
// 	newMsg.mtype = copied.mtype;
// 	newMsg.mOpMask = copied.mOpMask;

// 	strcpy(newMsg.mtext,copied.mtext); 
// 	return newMsg;
// }

////////////////////////////////////////////////////////////////////


void messageFromProcess()
{

	//Something receieved from process

		if (message.mtype == 2)
		{	
			connectedProcesses.push_back(message.mOpMask);
			cout<<"Process created"<<endl;
		}

		else
		{

			messageDiskStatus = requestDiskStatus();

			cout<<"Disk Status : free : "<<messageDiskStatus.mtext<<" Mask :"<<messageDiskStatus.mOpMask<<endl;
	

			messageRequestToDisk = message;
			

			
			handleProcessOperation();


			if (diskBusy == true)
				send_message(messageRequestToDisk,1);
			else
				send_message(message,0);
	  					
			
  		}
	  		
}

////////////////////////////////////////////////////////////////////

int main()
{

	//Signal Handler when Kernel dies
		signal (SIGINT, kernelDead);


	// Create Message Queues
		upqid = msgget(777,IPC_CREAT|0644);

		downqid = msgget(778,IPC_CREAT|0644);

	//Intialization of Time parameters
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


	cout<<"Kernel created and waiting for disk to be created\n";

	

	waitDiskCreation();	


  	while (1)
  	{
  		incrementClk();	
		

		// check if message recieved from process BUT NOT WAIT FOR IT
  			rec_val = msgrcv(upqid,&message,sizeof(message),3, IPC_NOWAIT);
  		if (rec_val != -1)
  		{
  			cout<<"Disk is now free"<<endl;
  			diskBusy = false;
  		}


 		
		if (diskBusy == false)
		{

			// check if message recieved from process BUT NOT WAIT FOR IT
	  			rec_val = msgrcv(upqid,&message,sizeof(message),0, IPC_NOWAIT);


	  		if (rec_val != -1)
  				messageFromProcess();

  		}	

  	}


	return 0;
}
