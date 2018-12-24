#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <iostream>
#include <string>
#include <sstream>
#include <bits/stdc++.h> 
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <math.h>

using namespace std;

char space[10][64];
bool taken[10];

key_t upqid;
key_t downqid;

int CLK;

int Mask;

int finishTime;

//Message Queue Struct
struct msgbuff
{
	long mtype;
	char mtext[64];
	int mOpMask;
};


void clkIncrement(int signum)
{
	CLK += 1;

	// cout<<CLK<<endl;
}

void convert(int n,char* result)
{
	std::ostringstream ss;
    ss << n;
    string r = ss.str();
    strcpy(result, r.c_str());
}

void handle_SU1(int signum)
{
	int count_free = 0;

	for(int i = 0 ;i<10;i++)
	  if(!taken[i])
	    count_free += 1;

	//send a message with the number of free slots; 
	struct msgbuff status_msg;

	status_msg.mtype = 3 ; 
	status_msg.mOpMask = Mask;
	convert(count_free,status_msg.mtext);
	
	
	int send_val = msgsnd(upqid,&status_msg,sizeof(status_msg),!IPC_NOWAIT);

	if(send_val == -1)
		perror("ERROR IN SENDING STATUS");	
		
}


void init()
{

	// Create message queue
		upqid = msgget(777, IPC_CREAT|0644); 
		downqid = msgget(778, IPC_CREAT|0644);

	
	msgbuff init_msg;

	init_msg.mtype = 1;

	convert(getpid(),init_msg.mtext);


	//once created send a message to the kernel containing your pid.
		int send_val = msgsnd(upqid, &init_msg, sizeof(init_msg.mtext), !IPC_NOWAIT);
	
	if(send_val == -1)
		perror("ERROR IN SENDING THE PID");
	
	
	
	for(int i = 0;i < 10;i++)
		taken[i] = false;

	Mask = 0;
	
	
	//Start clk	
		CLK = 0;

	finishTime = -1;
	
}


bool save_data(msgbuff msg)
{
	finishTime = CLK + 3;
	//cout<<"CLK = " << CLK<<"Finish time = "<<finishTime<<endl;

	int i;

	for (i = 0;i<10;i++)
		if(!taken[i])
			break;

	strcpy(space[i],msg.mtext);

	Mask = Mask | 1<<i;
	taken[i] = true;

	return true;
	
}
bool  free_slot(msgbuff msg)
{
	finishTime = CLK + 1;
	//cout<<"CLK = " << CLK<<"Finish time = "<<finishTime<<endl;
	

	string s;
	int y = (int)(msg.mtext[0]-'0') ; 
	cout<<"emptying slot "<<y<<endl;
	// s = msg.mtext;
	// std::istringstream ss(s);
	// ss >> y;

	if(y<0 and y>9){
		cout<<"not gonna free it up sorry"<<endl;
		return false;
	}

	taken[y] = false;

	Mask = Mask & ~(1<<y);


	return true;
}

void handle_message(msgbuff msg)
{
	cout<<"Opeation was receieved with mtype= "<<msg.mtype<<" mOpMask"<<msg.mOpMask<<" mtext"<<msg.mtext<<endl;
	int message_type = msg.mOpMask;

	if (message_type == 0)
		save_data(msg);
	else if(message_type == 1)
		free_slot(msg);
	
}

int main()
{
	//Handle Signals
		signal (SIGUSR1,handle_SU1);
		signal (SIGUSR2,clkIncrement);

	cout<<"Disk created with pid = "<<getpid()<<endl;

	init();

	struct msgbuff message;

	while(true)
	{
		
			

	
		if (finishTime <= CLK && finishTime != -1)
		{
			//cout<<"I am free"<<endl;
			cout<<"finished at ::"<< CLK<<endl;

			convert(message.mtype,message.mtext);

			message.mtype = 4;

			int send_val = msgsnd(upqid,&message,sizeof(message),IPC_NOWAIT);

			//if (send_val != -1)
				//cout<<"finish message was sent successfully to kernel"<<endl;


			finishTime = -1;
		}
		else{

			int rec_val = msgrcv(downqid, &message, sizeof(message),0, IPC_NOWAIT); 
	
			if(rec_val != -1)
				handle_message(message);

		}
	}

	return 0;
	
	
}


