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
#include <fcntl.h>
#define offset 10
#define SIZE 1000000

///////////////////////////////////////////////////DECLARATION OF FUNCTIONS/////////////////////////////////////////////////////////////////
int writeG (int t, int fd);
void readR (int fd,int *g1,int *g2, double *bits,  double *life, double*latenzamedia);
int writeR (int fd3, int g1, int g2,double bits, double life, double latenzamedia);
int readM(int fd3);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////DECLARATION OF STRUCTS//////////////////////////////////////////////////////////////
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
struct timeval start;  // struct to store start time of writing first message
struct timeval end;    // struct to store end time of reading last message
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main () {  
char * myfifo = "/tmp/myfifo";   //path name of the named pipe
if ( mkfifo(myfifo, S_IRUSR | S_IWUSR) != 0){
	perror("Cannot create fifo. Already existing?");
}
int fd3[2]; //file descriptor used by the unnamed pipe for communication between R and M
int pipe3 = pipe (fd3); //unnamed pipe for communication between R and M
if (pipe3 < 0){ //check errors opening unnamed pipe
	perror ("Error opening pipe between M and receiver");
	return -3;
}
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
			pid3 = fork();
			if (pid3 != 0) { // creation of process M
                                 
                                 exit(0);
                                       }
                        else           {
				//Process M
				int r = readM(fd3 [0] ); // call of function 
				exit(0);
                                        }
                               }
                 else
			{ 
                         // Process R 
			int g1, g2;
			double bits;
			double latenzamedia;
			double life;
			int fd = open(myfifo, O_RDONLY);  //open pipe to read only
                     	if (fd == 0) {  //check of errors
                		perror("Cannot open fifo");
                		unlink(myfifo);
                		exit(1);
            		}
       
			readR (fd, &g1, &g2, &bits, &life, &latenzamedia);  //call of function
			int w = writeR (fd3 [1], g1, g2,bits, life,latenzamedia);  //call of function
			exit(0);
			}
                   }
         	 else
			{
                        //process G2
		
			int t = 2;  //variable to recognize process G2 inside the function writeG
			int fd = open(myfifo, O_WRONLY); //open pipe for writing only
        		if (fd == 0) {  //check errors
				perror("Cannot open fifo");
				unlink(myfifo);
				exit(1);
            		}
        		writeG (t,fd); //call of function
			exit(0); 
			}
              }
 else   {
  //Process G1
  int t = 1; //variable to recognize process G1 inside the function writeG
  int fd = open(myfifo, O_WRONLY);
  if (fd == 0) {
        perror("Cannot open fifo");
        unlink(myfifo);
        exit(1);
    }
  writeG (t,fd);
  exit(0);
  }
}



////////////////////////////////////////////////////////////FUNCTIONS//////////////////////////////////////////////////////////////////////
int writeG (int t, int fd) {
	//it takes as input a variable t to recognize the process Gi and a file descriptor
	// it creates 10^6 messages and write it over a named pipe
	// it gives as return the value of the write syscall
	char * myfifo = "/tmp/myfifo"; //path of fifo
	int i=0;
	int count = 0;
	int nb;
	struct Message msg;
	if (t == 1) { //the function is used by G1
		strcpy(msg.g, "G1");
	}
	else if (t==2) { //the function is used by G2
		strcpy(msg.g, "G2");
	}
    
	while (i<SIZE) {
		struct timeval current_time;
		gettimeofday(&current_time, NULL);
		msg.time1 = (current_time.tv_sec*1000000) + current_time.tv_usec;
		msg.x = ++count; //number of message
		int del = offset + random() % (offset + 1);	// offset is inclusive here.
		usleep(del); 	//delay
		int nb = write(fd, &msg, sizeof(msg)); //syscall to write a message in a struct
		if (nb == 0){ //check of errors
			fprintf(stderr, "Write error\n");
			close(fd);
			unlink(myfifo);
		}	
 	i++;   
	}
	return nb; //value of the write syscall
}

void readR (int fd, int *g1, int *g2, double *bits, double *life, double *latenzamedia) { 
//it takes as input a file descriptor and 5 puntators which are elaborated inside the function. 
	// it reads 2*10^6 messages and calculates bits and time of life of whole writing and reading process
	// it gives as return the value of the read syscall.
	char * myfifo = "/tmp/myfifo";
	time_t start;
	struct Message msg;
	int nb;
	int G1 = 0;
	int G2 = 0;
	double bits_;
	double latenza = 0;
	time_t lifet;
	int i=0;
	while (i<2 * SIZE){
		nb = read(fd, &msg , sizeof (msg)); //syscall
		struct timeval current_time;
		gettimeofday(&current_time, NULL);
		time_t timenow = (current_time.tv_sec*1000000) + current_time.tv_usec;//current time of receiving a message
		latenza = latenza + (timenow - msg.time1);	//calculus of latency for each message
		if (strcmp(msg.g,"G1")) {
			G1++; //count of messages from G1
		}
		else if (strcmp(msg.g,"G2")){
			G2++; //count of messages from G2
		}
		if (nb == 0){ //check errors
			fprintf(stderr, "Read error\n");
			close(fd); //close the file descriptor
			unlink(myfifo); //delate the fifo
		}	
		if (i==0){
			start = msg.time1;
		}
		i++;
	}
	close(fd);
	unlink(myfifo);
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	time_t end = (current_time.tv_sec*1000000) + current_time.tv_usec; // current time at the end of the process of reading
	lifet = end - start; // timelife of whole sending and receiving process
	bits_ = (G1 + G2) * sizeof (msg);   //calculus of total bits received   
	*g1 = G1;
	*g2 = G2;
	*bits = bits_;
	*life = lifet;
	*latenzamedia = latenza / (G1 + G2); //average latency of whole process
}
    int writeR (int fd3, int g1, int g2, double bits, double life, double latenzamedia) {
    //it takes as input a file descriptor and the 5 variables that we have calculated in the function ReadR. 
	// it puts them in a message, in struct form, and sends them to M process
	// it gives as return the value of the write syscall.
    struct Results res;
    res.g1=g1; //total number of messages received from G1
    res.g2=g2;// total number of messages received from G2
    res.life= life;//duration of whole sending and receiving process
    res.bits=bits;// total number of bits received
    res.latenzamedia = latenzamedia;
    int w = write (fd3, &res, (sizeof(res))); //syscall
	if (w<0) { //check of errors
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
	if (r<0){ //check of errors
		perror ("Error in reading");
	}
    int mess_tot;
    mess_tot = (res.g1) + (res.g2);	 //total messages from both g1 and g2
    double bits = res.bits;
    double latency = res.latenzamedia;
    double bandwidth = bits/(res.life/1000000);
    printf ("G1: %d \n G2:%d \n messaggi ricevuti: %d \n offset: %d\n latency: %.2f \n bandwidth: %.2f\n", res.g1, res.g2 , mess_tot, offset, res.latenzamedia, bandwidth);
    return r;
   exit (0);
}
