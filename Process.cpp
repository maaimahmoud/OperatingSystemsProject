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
#include<string>
#include <fstream>
#include <iostream>
using namespace std;

ifstream infile;


struct msgbuff
{
	long mtype;
	char mtext[64];
	int mOpMask;
};

key_t upqid;
key_t downqid;

int clk=-1;

void clk_inc(int signum){
	clk++;
}


void send_msg(string op, string mess){
	mess.erase(0,1);
	msgbuff msg;
	if(toupper(op[0])=='A')
		msg.mOpMask=0;
	else
		msg.mOpMask=1;

	msg.mtype=getpid();
	int  s=mess.size()>64? 64: mess.size();

	for( int i=0; i<s; i++){
		msg.mtext[i]=mess[i];
	}

	cout<<" mssage type "<<msg.mOpMask<<" mtext :"<<msg.mtext<<" process ID: "<<msg.mtype<<endl;
	int send_val = msgsnd(upqid,&msg,sizeof(msg), !IPC_NOWAIT);

	if(send_val==-1)
		cout<<"Error in sending from process "<<endl;

	


}


int get_resp(){
	msgbuff msg;

	int rec_val= msgrcv(downqid, &msg,sizeof(msg),getpid(),!IPC_NOWAIT);

	if(rec_val==-1)
		cout<<"Error in recieving from Kernel in process "<<endl;
	return msg.mOpMask;

}




int main(int argc, char **argv){
	infile.open(argv[1]);

	if(!infile){
		cout<<" error with file man!";
		return 0;
	}

	// Create Message Queues
	upqid = msgget(777,IPC_CREAT|0644);

	downqid = msgget(778,IPC_CREAT|0644);

	//attach clk incrementer handler to signal
	signal(SIGUSR2,clk_inc);


	// read file contents --- assuming commands are sorted 
	string line, op, mess;
	int time;
	while(infile>>time){
		infile>>op;

		getline(infile,mess);


		while(time>clk){
			//wait

		}
		send_msg(op,mess);
		int resp=get_resp();
		if(resp==1|| resp==0)
			cout<<"Wass able to "<< op<<mess<<" successfully!"<<endl;
		else
			cout<<"Was not able to "<< op<<mess<<endl;

	}


	//while(1){}

	infile.close();




	return 0;
}
