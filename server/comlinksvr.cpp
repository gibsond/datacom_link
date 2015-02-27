//----------------------------------------------------------------------
// comlinksvr
// comlinksvr.cpp : Defines the entry point for the console application.
//----------------------------------------------------------------------
// This software application is used in coordination with a Comlink Client program.
// 
// The server allows data collection from a comlink client program via a TCP 
// stream socket connection.  3 Files are also generated during 
// the collection which provides a complete diagnostics of the data being 
// received.  The 3 files generated are;
//
//     logFile.txt
//     rawDataFile.txt
//     formDataFile.txt
// 
// logFile:  Contains a general description of what the software is doing
//           and processing in real time.
//
// rawDataFile:  A recording of the binary numbers sent across the TCP
//               connection in hexadecimal format.
//
// formDataFile:  A decoded representation of the raw data which makes up each
//                of the messages being sent to the paceServer from the DCC. 
//
// *NOTE* THE CONTENTS OF THE LOGFILE ARE DISPLAYED TO SCREEN AS IT IS GENERATED.
//
// By default the server will attempt to establish connection with the Pickering 
// simulator in order to subject it to the messages it receives from the DCC.
//
//---------------------------------------------------------------------- 
// This program requires the following files in order to build;
//
// frame.cpp     frame.h
// msg.cpp       msg.h
// util.cpp      util.h
// paceUtil.cpp  paceUtil.h
// 
//----------------------------------------------------------------------
// Please refer to the following documentation for further information;
//
//  PNGS A HUB_DCC INTERFACE MONITORING PACE EMUL PROTOTYPE
//
//----------------------------------------------------------------------

// Created by: Dan Gibson November 2005
//
//----------------------------------------------------------------------
// Modifications:
//
// 
//----------------------------------------------------------------------


#include <stdio.h>
#include <string.h> 
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "frame.h"
#include "msg.h"
#include "util.h"
#include "paceUtil.h"

#define SERV_TCP_PORT  6543               // arbitrary valid port number 
#define SERV_HOST_ADDR "172.23.26.223"    // pacedebian IP address
#define MAXLINE 4096                      

int main(int, char**);
static int createMySocket(void);
static void bindMySocket(int);
static int acceptFromMySocket(int);
static void displayTitleScreen();
static bool readParameters(int, char **, bool *, int *);
static void faultExit(void);

FILE * rawDataFile;
FILE * formDataFile;
FILE * logFile;


/************************************/
/* Main Method			    */
/************************************/

int main(int argc, char* argv[])
{
  int           mySockfd;
  int           myNewSockfd;
  int           mySize;
  int           cnt = 0;
  bool		initCall = true;
  bool          localPACE = true;
  int           channel = 1;
  unsigned int  myData;
  char          timeStamp[64];
  char		message[128];
  frame         myFrame;
  msg *         myMsg;


  /* Read in command line parameters */
  if(!readParameters(argc, argv, &localPACE, &channel))
    faultExit();
  
  /* Display Title Screen */
  displayTitleScreen();
  
  if (localPACE)
    printf("-Local PACE Emulation is enabled for DCC channel %d\n", channel);  
  else
    printf("-Local PACE Emulation is disabled");

  /* Initialize PACE Emulation */
#ifdef SIMPACE
  if (localPACE)
    if (!(localPACE = cdbAttach()))
      printf("Failed to start PACE Emulation.\n-Local PACE is now disabled\n");
#endif
  
  /* Create Server Socket */
  mySockfd = createMySocket();

  /* Bind Server Socket */
  bindMySocket(mySockfd);

  /* Listen on Server Socket.  
   *  Note: The second parameter sets the maximum number of connected clients at once.
   *        This program only allows one client connected to the server at a given time.
   */
  listen(mySockfd,5);
  
  /* Outter Continuous Loop (for loop)
   *  - Exits if there is a fatal error.
   *  - Contains a nested countinuous loop where the program spends most of its time.
   *  - Responsible for accepting connection from a client and creating files.
   *  
   * Inner Continuous Loop (while loop)
   *  - Exits to Outter Loop if the socket connection to the client is lost.
   *  - Receives data through the socket connection.
   *  - Constructs the frame and messages with the data.
   *  - Writes to the files created in the Outter Loop (logFile, rawDataFile, formDataFile).
   *  - Inserts message data to local Pace if enabled.
   */
    
  /* Outter Continuous Loop */
  for(;;){

    /* Accept connection from Client */    
    myNewSockfd = acceptFromMySocket(mySockfd);

    /* Initialize myFrame */
    myFrame.reset();
    
    /* Create and store time to be used in the filenames created below  */
    createTimeStamp(timeStamp);

    /* Create rawDataFile */
    rawDataFile = createFile(timeStamp, FILE_RAW);

    /* Create formDataFile */
    formDataFile = createFile(timeStamp, FILE_FOR);
    
    /* Create logFile */
    logFile = createFile(timeStamp, FILE_LOG);

    /* Initialize the formDataFile with the column titles */
    writeHeaderToFormDataFile(formDataFile); 
    
    /* Inner Continuous Loop */
    while(1){

      /* Wait and receive data from the client*/
      mySize = recv(myNewSockfd,       /* Connected client */
		    &myData,	       /* Receive buffer   */
		    4,                 /* Length of buffer */
		    0);       	       /* Flags            */
      
      /* If there was a socket error while receiving data, 
       * close the socket, files and return to the Outter Continuous Loop 
       */
      if (mySize <= 0){
	close(myNewSockfd);
	fclose(rawDataFile);
	fclose(formDataFile);
	fclose(logFile);
	break;
      }
      
      /* Fill the frame with the received data.  If the data doesn't conform with what the
       * frame is expecting and initCall is false then return false
       *  - While initCall is true it will ignore unexpected words by always returning true
       *    This is done because more than likely when we first start collecting
       *    data we are already in the middle of a frame being sent.  
       *  - initCall is set false after the first frame is recontructed.
      */
      if (!myFrame.fill(myData, initCall)){
	
	/* cnt initailly 0, denotes the number of consecutive unexpected words */
	cnt++;

	/* For tidiness only the first 5 unexpected words generate a warning message.
	 * After which the else block generates an error message of the total number of 
	 * consecutive unexpected words.
	 */
	if (cnt <= 5){
	  sprintf(message, "Warning: Unexpected word - %8.8X\n", myData);
	  printf(message);
	  writeToFile(logFile, message);
	}
      }      
      else{
	if (cnt > 0){
	  sprintf(message,
		  "Warning: %d unexpected consecutive words encountered\n",
		  cnt);
	  printf(message);
	  writeToFile(logFile, message);
	}
	/* Reset the unexpected consective words counter */
	cnt = 0;
      }

      /* Write the data received from the socket to the rawDataFile */
      writeToRawDataFile(rawDataFile, myData);

      /* If the last word received filled the frame write the contents of the frame to
       * logFile and FormDataFile.
       */
      if (myFrame.isFilled()){
	writeToLogFile(logFile, myFrame);
	writeFrameToFormDataFile(formDataFile, myFrame);
	initCall = false;
	
	/* If localPace is enabled and the frame passes the checksum algorithm (isValid),
	 * insert the messages of the frame to the PACE emulation.
	 */
	if (localPACE && myFrame.isValid()){	  

	  /* resetMsgList resets a pointer in the frame to the first Message 
	   *  - When the popFrontMsg function is called the pointer is incremented 
	   * to the next message. 
           */
	  myFrame.resetMsgList();

#ifdef SIMPACE
	  /* Freeze the simulator DCC in order to ensure a successful insertion to the
	   * PACE emulation 
	   */
	  freezeDcc(channel);

	  /* Step through the list of messages within the reconstructed frame 
	   * and insert each of them into the PACE emulation.
	   */
	  while((myMsg = myFrame.popFrontMsg()))
	    insertMsgToPace(myMsg, channel);

	  /* Unfreeze the simulator DCC after the messages have been inserted */
	  unfreezeDcc(channel);
#endif
	}
	/* Since only one frame is created (like a static object), we need to empty 
	 *  it for the next frame to be reconstructed.  
	 *  - All messages are erased and frame variables are reset.
	 *  - A frame counter is incremented.
	 */
	myFrame.reset();
      }      
    }
    
    /* When the inner loop is exited we generate the followign message */
    printf("Connection Terminated!\n\nFiles created during last connection;\nrawDataFile_%s.txt\nformDataFile_%s\nlogFile_%s.txt\n\n",
	   timeStamp, timeStamp, timeStamp);
  }

  /* We should never exit the Outter Continuous Loop.  No code here will get executed. */
  
  return 0;
}


/************************************/
/* displayTitleScreen Method	    */
/************************************/

static void  displayTitleScreen(void)
{
  printf("\n ******************************\n");
  printf(" *       COMLINK SERVER       *\n");
  printf(" ******************************\n\n");
}


/************************************/
/* readParameters Method	    */
/************************************/

static bool readParameters(int argc, char **argv, bool *localPACE, int *channel)
{
  int i;
  char *new_argv[argc];

  for(i=2;i<=argc;i++)
  {
      new_argv[i-1] = strdup(argv[i-1]);
      toUpperCase(new_argv[i-1]);
  }
    
  for (i = 2; i <= argc; i++){    
    if (strcmp(new_argv[i-1], "-C1") == 0)
      *channel = 1;
    else if (strcmp(new_argv[i-1], "-C2") == 0)
      *channel = 2;
    else if (strcmp(new_argv[i-1], "-NOLP") == 0)
      *localPACE = false;
    else {
      printf("Invalid arguments.\nExisting options are;\n");
      printf("-c1    Channel 1\n");
      printf("-c2    Channel 2\n");
      printf("-nolp  No local PACE\n");
      return false;
    }
  }
  return true;
}


/************************************/
/* createMySocket Method	    */
/************************************/

static int createMySocket(void)
{
  int sockfd;

  printf("\nCreating Socket...\n");
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    printf("server: can't open stream socket");
    faultExit();
  }
  printf("%s\n",getCurrentTime());
  
  return sockfd;
}


/************************************/
/* bindMySocket Method  	    */
/************************************/

static void bindMySocket(int sockfd)
{
  struct sockaddr_in  serv_addr;  
  
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(SERV_TCP_PORT);
  
  printf("Binding Socket to Port Number %d\n", SERV_TCP_PORT);
  
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){   
    printf("server : can't bind local address\n");
    faultExit();
  }

  return;
}


/************************************/
/* acceptFromMySocket Method	    */
/************************************/

static int acceptFromMySocket(int sockfd)
{
  struct sockaddr_in  cli_addr;
  socklen_t           clilen;
  int                 newSockfd;
  char                myBuff[MAXLINE+1];

  printf("Waiting for connection from client...\n\n");
  clilen = sizeof(cli_addr);
  newSockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  
  printf("paceServer has accepted connection with %s on client port %d\n%sReceiving Data...\n",
	 inet_ntop(AF_INET, &cli_addr.sin_addr, myBuff, sizeof(myBuff)),
	 ntohs(cli_addr.sin_port), getCurrentTime());
  
  if (newSockfd < 0){
    printf("server: accept error\n");
    faultExit();
  }

  return newSockfd;
}


/************************************/
/* acceptFromMySocket Method	    */
/************************************/

static void faultExit(void)
{
  printf("\n\nShutting down program, press Enter key to quit\n");
  getchar();
  exit(-1);
}
