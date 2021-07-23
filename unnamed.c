#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <errno.h>

#define offset 10
#define SIZE 1000000 //number of messages for each sender


/////////////////////////////////////////////////////////DECLARATION OF FUNCTIONS/////////////////////////////////////////////////////////////////
void writeG (int fd, int t);
void readR (int fd1, int fd2,int *g1,int *g2, double *bits, double *life, double*latenzamedia);
int writeR (int fd3, int g1, int g2,double bits, double life, double latenzamedia);
int readM(int fd3);

////////////////////////////////////////////////DECLARATION OF STRUCTS//////////////////////////////////////////////////////////////
struct Message { //struct used to store every datum of a single message
    time_t time1;	
    char g [3]; 
    int x;
    };

struct Results { // struct used to store data from calcuations
    int g1;	
    int g2;
    double bits; 
    double life;
    double latenzamedia;
    };

struct timeval start;  // struct from library time_h start time of writing first message
struct timeval end; // struct from library time_h end time of reading last message
//////////////////////////////////////////////////Inizio main/////////////////////////////////////////////////////////////////////

int main () {
//declaration of three file descriptors
int fd1[2]; 
int fd2[2];
int fd3[2];
//creation of 3 unnamed pipes
int pipe1 = pipe (fd1);
int pipe2 = pipe (fd2);
int pipe3 = pipe (fd3); 

pid_t pid0; // pid of process G1
pid_t pid1; // pid of process G2
pid_t pid2; // pid of process R
pid_t pid3; // pid of process M 

pid0= fork ();  // creation of process G1
if (pid0!=0)  {
	pid1= fork ();  // creation of G2
        if(pid1!=0) { 
		pid2 = fork();  // creation of process R
                if (pid2!=0) {
			pid3 = fork(); // creation of process M
			if (pid3 != 0) { 
                                 exit(0);
                                       }
                                 
                        	else {
						//Process M		
				int r = readM(fd3 [0] ); // call of function 
				exit(0);
                                        }
                               }
                 else
			{ 
			//process R
			int g1, g2;
			double bits;
			double latenzamedia;
			double life;
			readR (fd1[0], fd2[0], &g1, &g2, &bits, &life, &latenzamedia); //call of function
			int w = writeR (fd3 [1], g1, g2, bits, life,latenzamedia); // call of function
			exit(0);
			}
                   }
         else
			{
                        //process G2	
			int t = 2; //variable to recognize process G2 inside the function writeG
			writeG (fd2[1], t); //call of function
			exit(0); 
			}
              }
 else   {
  //Process G1
  int t = 1; //variable to recognize process G1 inside the function writeG
  writeG (fd1[1], t); //call of function
  exit(0);
  }


 return 0;
}
////////////////////////////////////////////////////FUNCTIONS//////////////////////////////////////////////////////////////////////
void writeG (int fd, int t) {
	//it takes as input a variable t to recognize the process Gi and a file descriptor
	// it creates 10^6 messages and write it over a named pipe
	int i=0;
	int count = 0;
	int w;
	struct Message msg;
	struct timeval current_time;
	gettimeofday(&current_time, NULL); 
	while (i < SIZE) {	
		if (t == 1) { //the function is used by G1 
			strcpy(msg.g, "G1");
			msg.x = i; //number of messages from G1
		}
		if (t == 2) { //the function is used by G2
			strcpy(msg.g, "G2");
			msg.x = i; // number of messages from G2
		}
		msg.time1 = (current_time.tv_sec*1000000) + current_time.tv_usec; //current time of the day
		int del = offset + random() % (offset + 1);	
		usleep(del); //delay
		w = write (fd, &msg , sizeof (msg)); //syscall
		if (w <0) { //check of errors
			perror ("Error in writing");
		}
	i++; 
	}
} 
void readR (int fd1, int fd2, int *g1, int *g2, double *bits, double *life, double *latenzamedia) { 
	//it takes as input a file descriptor and 5 puntators which are elaborated inside the function. 
	// it reads 2*10^6 messages and calculates bits and time of life of whole writing and reading process
	time_t start;
	struct Message msg;
	int r;
	int G1 = 0;
	int G2 = 0;
	double bits_;
	double latenza = 0;
	time_t lifet; 
	int retval;
	int i=0;
	while (i< 2*SIZE){ // it is the same of the total of messages 
		int file_des;
		fd_set rfds; //set of file descriptors
		FD_ZERO(&rfds); //set to zero the set of file descriptors
		FD_SET(fd1, &rfds); //put fd1 inside the set of file descriptor
		FD_SET (fd2, &rfds); //put fd2 inside the set of file descriptor
		struct timeval timeout;
		timeout.tv_usec = 30;
		retval = select(FD_SETSIZE, &rfds, NULL, NULL, &timeout); //syscall

		if (retval == -1){ //check errors
			perror("err select()");
		}
		else if (retval) {
			if ( FD_ISSET (fd1, &rfds) && !(FD_ISSET (fd2, &rfds))) { //check if only fd1 is ready
				file_des = fd1 ;
			}
			else if  (FD_ISSET (fd1, &rfds) && (FD_ISSET(fd2, &rfds))) { //check if both fd1 and fd2 are ready
				int random = 1 + rand () % 2;	// 1 or 2
				switch (random) { //choose randomly from which one read
				case 1:
				file_des = fd1 ;
				break;

				case 2:
				file_des = fd2;
				break;
				}
			}
			else  {
			file_des= fd2; //only fd2 is ready
			}
			r = read(file_des, &msg , sizeof (msg));  //syscall
			if (r <0) { //check of errors
				perror ("Error in reading");
			}
			struct timeval current_time;
			gettimeofday(&current_time, NULL);
			time_t timenow = (current_time.tv_sec*1000000) + current_time.tv_usec; //current time of receiving a message
			latenza = latenza + (timenow - msg.time1);//calculus of latency for each message

			if (i==0){
				start = msg.time1; //time of the first message sent
			}
			if (strcmp(msg.g,"G1")==0){
				G1++; //counter of received messages from G1
			}
			if (strcmp(msg.g,"G2")==0){
			G2++; //counter of received messages from G2
			}
			i++; // this counter goes on only if we have already read a message
		}
	}
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	time_t end = (current_time.tv_sec*1000000) + current_time.tv_usec; // current time at the end of the process of reading
	lifet = end - start;// timelife of whole sending and receiving process

	bits_ = (G1 + G2) * sizeof (msg);     //calculus of total bits received
	*g1 = G1;
	*g2 = G2;
	*bits = bits_;
	*life = lifet;
	*latenzamedia = latenza / (G1 + G2);//average latency of whole process
}
int writeR (int fd3, int g1, int g2, double bits, double life, double latenzamedia) {
	//it takes as input a file descriptor and the 5 variables that we have calculated in the function ReadR. 
	// it puts them in a message, in struct form, and sends them to M process
	// it gives as return the value of the write syscall.
	int w;   
	struct Results res;
	res.g1=g1; // total number of messages received from G1
	res.g2=g2; //total number of messages received from G2
	res.life= life; //duration of whole sending and receiving process
	res.bits=bits; // total number of bits received
	res.latenzamedia = latenzamedia;
	w = write (fd3, &res, (sizeof(res))); //syscall
	if (w<0) {
		perror ("Error in writing");
	}
	return w;
}


int readM(int fd3) { 
	//it takes as input a file descriptor. 
	// it reads a message, in struct form, and prints out it on the shell as a string.
	// it gives as return the value of the write syscall.
	struct Results res;
	int r;
	r = read (fd3, &res , (sizeof (res))); //syscall
	if (r<0) {
		perror ("Error in writing");
	}
	int mess_tot;
	mess_tot = (res.g1) + (res.g2);	// total number of messages received
	double bits = res.bits;
	double latency = res.latenzamedia;
	double bandwidth = bits/(res.life/1000000);
	printf ("G1: %d \n G2:%d \n messaggi ricevuti: %d \n offset: %d\n latency: %.2f \n bandwidth: %.2f\n", res.g1, res.g2 , mess_tot, offset, res.latenzamedia, bandwidth);
	return r;
}
