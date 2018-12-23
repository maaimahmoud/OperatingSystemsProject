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

int mOpMask;

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
	cout<<CLK<<endl;
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
	msgbuff status_msg;

	status_msg.mtype = 1 ; 
	status_msg.mOpMask = mOpMask;
	
	
	int send_val = msgsnd(upqid,&status_msg,sizeof(status_msg.mtext),!IPC_NOWAIT);

	if(send_val == -1)
		perror("ERROR IN SENDING STATUS");	
		
}


void init()
{

	// Create message queue
		upqid = msgget(777, IPC_CREAT|0644); 
		downqid = msgget(778, IPC_CREAT|0644);

	
	msgbuff init_msg;

	init_msg.mtype = 2;

	convert(getpid(),init_msg.mtext);


	//once created send a message to the kernel containing your pid.
		int send_val = msgsnd(upqid, &init_msg, sizeof(init_msg.mtext), !IPC_NOWAIT);
	
	if(send_val == -1)
		perror("ERROR IN SENDING THE PID");
	
	
	
	for(int i = 0;i < 10;i++)
		taken[i] = false;

	mOpMask = 0;
	
	
	//Start clk	
		CLK = 0;
	
}


bool save_data(msgbuff msg)
{
	int i;

	for (i = 0;i<10;i++)
		if(!taken[i])
			break;

	strcpy(space[i],msg.mtext);

	mOpMask = mOpMask | 1<<i;

	return true;
	
}
bool  free_slot(msgbuff msg)
{

	string s;
	int y;
	s = msg.mtext;
	std::istringstream ss(s);
	ss >> y;

	if(y<0 and y>9)
		return false;
	taken[y] = false;

	mOpMask = mOpMask & !(1<<y);


	return true;
}

void handle_message(msgbuff msg)
{
	int message_type = msg.mtype;

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

	msgbuff message;

	while(true)
	{
		
		int rec_val = msgrcv(downqid, &message, sizeof(message.mtext),0, !IPC_NOWAIT); 
	
		if(rec_val == -1)
			continue;
		else
			handle_message(message);
	}

	return 0;
	
	
}


