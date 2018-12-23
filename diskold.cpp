#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include<iostream>
#include<string>
#include<sstream>
#include <bits/stdc++.h> 
using namespace std;

char space[10][64];
bool taken[10];
int up;
int down;
int CLK;
int up_key = 111;
int down_key = 112;
int mask;

struct msgbuff_1
{
   long mtype;
   int mask;
   char mtext[64];
};
struct msgbuff_2
{
	long mtype;

	char mtext[64];
};


void handle_SU1(int signum)
{
	
	CLK += 1;
}

void convert(int n,char* result)
{
	std::ostringstream ss;
    ss << n;
    string r = ss.str();
    strcpy(result, r.c_str());
}
void handle_SU2(int signum)
{
	int count_free = 0;
	for(int i = 0 ;i<10;i++)
	  if(!taken[i])
	    count_free += 1;

	//send a message with the number of free slots; 
	msgbuff_1 status_msg;
	status_msg.mtype = 1 ; 
	status_msg.mask = mask;
	
	
	int send_val = msgsnd(down,&status_msg,sizeof(status_msg.mtext),!IPC_NOWAIT);
	if(send_val == -1)
		perror("ERROR IN SENDING STATUS");	
		
}

// assumption : the key to create is known for both kernel and disk
void init()
{
	
	msgbuff_1 init_msg;
	init_msg.mtype = 2;
	
	
	for(int i = 0;i<10;i++)
		taken[i] = false;

	mask = 0;
	convert(getpid(),init_msg.mtext);

	
	signal(SIGUSR1,handle_SU1);
	signal(SIGUSR2,handle_SU2);	
	up = msgget(up_key, IPC_CREAT|0644); //0644: explicit. 
	down = msgget(down_key,IPC_CREAT|0664);
	CLK = 0;
	//once created send a message to the kernel containing your pid.
	int send_val = msgsnd(down, &init_msg, sizeof(init_msg.mtext), !IPC_NOWAIT);
	if(send_val == -1)
		perror("ERROR IN SENDING THE PID");
	
}


bool save_data(msgbuff_2 msg)
{



	for (int i = 0;i<10;i++)
		if(!taken[i])
			break;
	strcpy(space[i],msg.mtext);
	mask = mask | 1<<i;

	return true;
	
}
bool  free_slot(msgbuff_2 msg)
{

	string s;
	int y;
	s = msg.mtext;
	std::istringstream ss(s);
	ss >> y;

	if(y<0 and y>9)
		return false;
	taken[y] = false;

	mask = mask & !(1<<y);


	return true;
}

void handle_message(msgbuff_2 msg)
{
	int message_type = msg.mtype;
	if (message_type == 0)
		save_data(msg);
	else if(message_type == 1)
		free_slot(msg);
	
}

int main()
{

	msgbuff_2 message;
	init();
	bool flag = false;
	while(flag)
	{
		
		int rec_val = msgrcv(up, &message, sizeof(message.mtext),0, IPC_NOWAIT); 
	
		if(rec_val == -1)
		continue;
		else
		handle_message(message);
	}
	return 0;
	
	
}











