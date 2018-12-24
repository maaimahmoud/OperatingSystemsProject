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
#include <fstream>
#include <algorithm>
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
	ofstream f;

	//f.open("l.txt",std::ofstream::app|std::ofstream::out);

//Messages
	struct msgbuff message;
	struct msgbuff messageDiskStatus;
	struct msgbuff messageRequestToDisk;

	int rec_val,send_val;

	bool diskBusy = false;	  



vector<int>connectedProcesses;

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

	int rec_val = msgrcv(upqid,&message,sizeof(message),1,!IPC_NOWAIT);

	if (rec_val != -1 )
	{
		disk_id = convertCharArrToInt(message.mtext);

		cout<<"Disk created Successfully with id = "<<disk_id<<endl;
	}
	else
		cout<<"Cannnot recieve id from Disk"<<endl;

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
	  		clockCycles++;

	  		for (int i = 0; i < connectedProcesses.size(); ++i)
	  		{
	  			kill(connectedProcesses[i],SIGUSR2);
	  			// cout<<"Sending SIGUSR2 to process with pid = "<<connectedProcesses[i]<<endl;
	  		}

		  	int diskStatus = kill(disk_id,SIGUSR2);
		  	if (diskStatus == -1)
		  	{
		  		cout<<"ERORR NOT DISK !!!"<<endl;
		  		cout<<"WAITING DISK TO CONNECT AGAIN"<<endl;
		  		waitDiskCreation();
		  		diskBusy = false;
		  	}
	  	}
	
}





////////////////////////////////////////////////////////////////////

void send_message(msgbuff &sentMessage,int n)
{
	string s;
	if (n == 0)
		s = "process";
	else
		s = "disk";
	
	// int send_vall=-1;

	int send_vall = msgsnd(downqid,&sentMessage,sizeof(sentMessage),IPC_NOWAIT);

	if(send_vall==-1)
		f<<"ERROR Cannot send [ " <<sentMessage.mtext<<" ] to"<<s<<endl;

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
		f<<"Process requests to add message[ "<< message.mtext<<" ] "<<endl<<flush;  
		if (convertCharArrToInt(messageDiskStatus.mtext) == 0){
			// cout<<"fulllll"<<endl;
			message.mOpMask = 2; // if no empty slots
			f<<"Add Operation is not valid, No Empty Slots\n"<<flush;
		}
		else
		{
			// empty slot exists
			message.mOpMask = 1;
			messageRequestToDisk.mOpMask=0; 
			diskBusy = true;
			f<<"Add Operation is valid\n"<<flush;
		}
	}
	else
	{	
		
		int state = messageDiskStatus.mOpMask;
		int deletedSlot = (int)( message.mtext[0]-'0');
		f<<"Process requests to delete slot number [ "<< deletedSlot<<" ] From the process with pid "<<message.mtype<<"\n"<<flush;  
		
		if ((state & (1<<deletedSlot))>0)
		{
			f<<"Delete Operation is valid\n"<<flush;
			// Slot is used
			message.mOpMask = 3;
			// cout<<"want to del "<< deletedSlot<<endl;
			messageRequestToDisk.mOpMask=1; 
			diskBusy = true;
		}
		else
			{
				message.mOpMask = 4; // Slot is empty
				f<<"Delete Operation is not valid, Nothing to Delete\n"<<flush;

			}
		
	}


}

////////////////////////////////////////////////////////////////////


void messageFromProcess()
{


	//Something receieved from process

		if (message.mtype == 2) // process sending me its ID
		{	
			connectedProcesses.push_back(message.mOpMask);

			f << "**********************"<<endl;
			f<< " Process created with pid = "<<message.mOpMask<<" AT time slot = "<<clockCycles<<endl;
			f << "**********************"<<endl;
		}
		else if (message.mtype == 5)
		{
			f<<"Process with pid = "<<message.mOpMask <<" terminated"<<endl;
			//delete process from vector
			connectedProcesses.erase(std::remove(connectedProcesses.begin(),connectedProcesses.end(),message.mOpMask),connectedProcesses.end());

		}
		else
		{

			f << "----------------------"<<endl;


			f << "PROCESS IS with pid = "<<message.mtype<<" Requesting operation ..."<<" AT time slot = "<<clockCycles<<endl;

			messageDiskStatus = requestDiskStatus();

			f<<"Requesting Disk Status \n"<<flush;


			f<<"Disk Status --> free slots = "<<messageDiskStatus.mtext<<" Mask = "<<messageDiskStatus.mOpMask<<endl;
	

			// f.flush();
			

			
			handleProcessOperation();

			//messageRequestToDisk = message;


			if (diskBusy == true){

				f<<"Request was sent to Disk ... AT time slot "<<clockCycles<< endl<<flush;
				send_message(message,0);
				messageRequestToDisk.mtype=5; 
				strcpy(messageRequestToDisk.mtext,message.mtext);
				send_message(messageRequestToDisk,1);
			}
			else
			{
				f<<"Respond was sent to Process Successfully \n";
				send_message(message,0);
	  		}	

	
			
  		}
  		
	  		
}

////////////////////////////////////////////////////////////////////


void kernelDead(int signum)
{
	cout<<"\nKernel....\nInterrupt Signal #"<<signum<<" received" <<endl;

	rec_val = 0;
	
	while(rec_val != -1 )
	{
		rec_val = msgrcv(upqid,&message,sizeof(message),0,!IPC_NOWAIT);

		if(rec_val != -1)
		{

			if(message.mtype == 2)
				messageFromProcess();

		}
	}

	for (int i = 0; i < connectedProcesses.size(); ++i)
	  	{
	  		kill(connectedProcesses[i],SIGINT);
	  	}

	kill(disk_id,SIGINT);


	cout<<"Deleting message Queues..."<<endl;
	  
  	msgctl(upqid,IPC_RMID,(struct msqid_ds *)0);

  	msgctl(downqid,IPC_RMID,(struct msqid_ds *)0);

	cout<<"Message Queues deleted ... Dead Kernel"<<endl;

	f.close();

  	exit(0);
}

////////////////////////////////////////////////////////////////////

int main()
{
	f.open("logfile.txt");
	if(!f.is_open())
	{
		cout<<"Error opening log file!\n";
	}

	//Signal Handler when Kernel dies
		signal (SIGINT, kernelDead);


	// Create Message Queues
		upqid = msgget(777,IPC_CREAT|0644);

		downqid = msgget(778,IPC_CREAT|0644);

	//Intialization of Time parameters
		now = time(0);
	  	tm = localtime (&now);
	  	currentTime = tm->tm_sec;



	cout<<"Kernel created and waiting for disk to be created\n";

	

	waitDiskCreation();	

	// cout<<"Disk created"<<endl;


  	while (1)
  	{
    	incrementClk();	
		
  		if(diskBusy==true)
  		{
  			// check if message recieved from disk to tell me it is free now

  			rec_val = msgrcv(upqid,&message,sizeof(message),4, IPC_NOWAIT);
	  		if (rec_val != -1)
	  		{
	  			// cout<<"Disk is now free at time "<<now<<endl;
	  			diskBusy = false;
	  			f << "Disk has finished requested operation AT Time slot "<<clockCycles<<endl;
		  		f << "----------------------"<<endl<<endl;	
	  		}

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
