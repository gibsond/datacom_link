//----------------------------------------------------------------------
// comlinkclnt
// comlinksvr.cpp : Defines the entry point for the console application.
//----------------------------------------------------------------------
// This software application is used in coordination with a Comlink Server program.
// 
// The client allows data collection from the DCC-HUB link via National Instruments DAQ Hardware.  If a Comlink Server
// is running and the proper IP address is specified within SOCKDriver.h, connection can be made in order to 
// commence data tranfer to that machine.
// 3 Files are also generated during the collection which provides a complete diagnostics of the data being 
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
// By default the server will attempt to establish connection with a local PACE Emulation and a remote PACE Emulation
// in order to subject it to the messages it receives from the DCC.
//
//---------------------------------------------------------------------- 
// This program requires the following files in order to build;
//
// comlinkclnt.cpp  comlinkclnt.h
// frame.cpp        frame.h
// msg.cpp          msg.h
// util.cpp         util.h
// paceUtil.cpp     paceUtil.h
// SOCKDriver.cpp   SOCKDriver.h
// FILEDriver.cpp   FILEDriver.h
// PXIDriver.cpp    PXIDriver.h
//
//----------------------------------------------------------------------
// Please refer to the following documentation for further information;
//
//  PNGS A HUB_DCC INTERFACE MONITORING PACE EMUL PROTOTYPE
//
//----------------------------------------------------------------------
//
// Created by: Dan Gibson November 2005
//
//----------------------------------------------------------------------
// Modifications:
//
// 
//----------------------------------------------------------------------



#include "comlinkclnt.h"
#include "PXIDriver.h"
#include "SOCKDriver.h"
#include "FILEDriver.h"
#include "paceUtil.h"

FILE *		logFile;
FILE *		rawDataFile;
FILE *		formDataFile;
FILE *		extendedRawDataFile; // Added April 16, 2007 Dan Gibson

/************************************/
/* Main Method			    */
/************************************/

int main(int argc, char**argv)
{
    SOURCE	    mySourceType;
    void *          mySource;	       
    void *          mySource2;
    void *          mySource3;
    char            dev2port[]="/Dev2/port0:3";
    char            dev3port[]="/Dev3/port0:3";
    char            dev2clksrc[]="/Dev2/PFI6"; // First 6534 DIO
    char            dev3clksrc[]="/Dev3/PXI_Trig0"; // Second 6534 DIO
    char            dev3clkdest[]="/Dev3/PXI_Trig0"; // 6602 Timer Clock Dest.
    char            dev1clksrc[]="/Dev1/PFI38"; // 6602 Timer Clock Source.
    int 	    mySockfd;
    int             channel = 1;
    int             error;
    bool	    initCall = true;
    bool            connected = false;
    bool            localPACE = true;
    bool            remotePACE = true;
    time_t	    downTime = 0;   
    char	    message[4096];
    char            errBuff[2048];
//    frame	    myFrame;
    msg *           myMsg;
    uInt32	    data;
    uInt32	    data1;    // Added to represent DCC data.  Apr 16, 2007. Dan Gibson
    uInt32	    data2;    // Added to represent HUB data.  Apr 16, 2007. Dan Gibson
    int iSerialNumbers[100];  // Added for Serialization Numbers. March 15, 2007. Dan Gibson.
    int * piSerialNumbers;


/* Initialize Serialization Numbers array.  This assumes that Serialization codes
   are fairly consecutive.  The index i is the Serialization code.
 */
    for (int i=0; i < 100; i++)
	iSerialNumbers[i] = -1;
    piSerialNumbers = &iSerialNumbers[0];

    frame         myFrame(piSerialNumbers);
    
    /* Read in command line parameters */
    if (!readParameters(argc, argv, &localPACE, &remotePACE, &mySourceType, &channel))
	faultExit();
    
    /* Display Title Screen */
    displayTitleScreen();
    
    /* Display chosen run-time settings */
    if (localPACE)
	printf("-Local PACE Emulation is enabled for DCC channel %d\n",channel);
    else
	printf("-Local PACE Emulation is disabled\n");    
    printf("-Remote PACE connection is %s\n",remotePACE?"enabled":"disabled");
    
    /* Initialize data and log files */     
    createTimeStamp(message);
    rawDataFile = createFile(message, FILE_RAW);
    extendedRawDataFile = createFile(message, FILE_EXTRAW); // Added April 16, 2007. Dan Gibson
    formDataFile = createFile(message, FILE_FOR);
    logFile = createFile(message, FILE_LOG);
    
    /* If there is an error creating a file, exit the program */
    if ((rawDataFile == NULL) ||
	(formDataFile == NULL) ||
	(logFile == NULL))
    {
	printf("Error creating files");
	faultExit();
    }
    
    /* Write column headings on the formatted data file */
    writeHeaderToFormDataFile(formDataFile);
    
    /* If localPace is enabled, attach comlinkclnt to the simulator's common database (cdb) */
#ifdef SIMPACE
    if (localPACE)
    {
	if (!(localPACE = cdbAttach()))
	    printf("Failed to start local PACE emulation.\n-Local PACE is now disabled\n");
    }
#endif
		       

    /* Initialize source receiving data from */
    switch(mySourceType)
    {
	case SRC_PXI:

	    /* Create task of reading from the PXI device */
	    mySource = (void *)createMyTask(&error,dev2port,dev2clksrc);
	    if (hasDAQmxFailed(error)) 
	    {
		writeToFile(logFile, DAQmxError(error,(TaskHandle)mySource, errBuff));
		faultExit();
	    }
/*  This exports the master 6534 clock on PF2 to the slave 6534 */
	    exportClockSignal(&error,(TaskHandle)mySource,dev3clksrc);
	    if (hasDAQmxFailed(error))
	    {
		writeToFile(logFile, DAQmxError(error, (TaskHandle)mySource, errBuff));
		faultExit();
	    }

	    /* Create task of reading from the2nd  PXI device */
	    /* Added by D.J. Gibson, Mar. 3, 2006 */
	    mySource2 = (void *)createMyTask(&error,dev3port,dev3clksrc);
	    if (hasDAQmxFailed(error)) 
	    {
		writeToFile(logFile, DAQmxError(error,(TaskHandle)mySource2, errBuff));
		faultExit();
	    }

	    /* Create task for trigger to synchronize the starting og the two PXI devices */
	    /* Added by D.J. Gibson, Mar. 3, 2006 */
/*	    mySource3 = (void *)createMyTriggerTask(&error);
	    if (hasDAQmxFailed(error)) 
	    {
		writeToFile(logFile, DAQmxError(error,(TaskHandle)mySource3, errBuff));
		faultExit();
	    }
*/
	    /* Start the task for Source2 created above, which is the slave */
	    startMyTask((TaskHandle)mySource2, &error);
	    if (hasDAQmxFailed(error))
	    {
		writeToFile(logFile, DAQmxError(error, (TaskHandle)mySource2, errBuff));
		faultExit();
	    }	     
	    

	    /* Start the task for Source created above, which is the master */
	    startMyTask((TaskHandle)mySource, &error);
	    if (hasDAQmxFailed(error))
	    {
		writeToFile(logFile, DAQmxError(error, (TaskHandle)mySource, errBuff));
		faultExit();
	    }

	    /*  Route Clock from 6602 to the two 6534 DIO Boards - D. Gibson May 29, 2006 */
//	    RouteClock(&error,dev1clksrc,dev1clkdest);
/*	    if (hasDAQmxFailed(error))
	    {
		writeToFile(logFile, DAQmxError(error, (TaskHandle)mySource, errBuff));
		faultExit();
	    }
*/	    
	    /* Generate Trigger signal to start sample clock */
/*
	    startMyTriggerTask((TaskHandle)mySource3, &error);
	    if (hasDAQmxFailed(error))
	    {
		writeToFile(logFile, DAQmxError(error, (TaskHandle)mySource3, errBuff));
		faultExit();
	    }	     
*/	    	    
	    /* Status message */
	    printf("\nSampling Link...\n"); 
	    break;

	case SRC_FILE:

	    /* Open the file specified in the command line for reading as source */
	    mySource = (void *)createMyFile(argv[argc-1]);
	    if (mySource == NULL)
	    {
		sprintf(errBuff, "Error: File \'%s\'not found",argv[argc-1]);
		printf("%s", errBuff);
		writeToFile(logFile, errBuff);
		faultExit();
	    }

	    /* Status message */
	    printf("\nSampling FILE...\n");	 
	    break;

	default:

	    printf("Invalid Source Type. Exiting...");
	    faultExit();
	    break;
    }	

    /* Main Continuous Loop
     *  - Under certain conditions attempt to connect to comlinksvr
     *  - Read data from the source created above
     *  - Insert data into a frame object
     *  - Write the data to the rawDataFile
     *  - Under certain conditions attempt to send the data to the comlinksvr
     *  - If the frame is filled, write to logFile and formDataFile     
     *  - Under certain conditions attempt to insert the data to local PACE emulation
     */
    while(1)
    {
	/* If remotePace is enabled, not already connted and is time to attempt connection */
	if (remotePACE && (!connected) && isTimeToConnect(downTime))
	{	    
	    /* Create Client Socket */
	    mySockfd = createMySocket();

	    /* Connect to Comlink Server Socket */
	    if(!(connected = connectMySocket(mySockfd)))
	    {
		/* Connection Failed, record the time of failure to be used in isTimeToConnect */
		downTime = time(NULL);

		/* Generate a log message of this event: connection failed */
		sprintf(message, "\nComlink Server down: Connection Failed.\n%s\n", getCurrentTime());
		printf(message);					
		writeToFile(logFile, message);					
		
	    }
	    else
	    {
		/* Generate a log message of this event: connection established */
		sprintf(message, "\nComlink Server up: Connection Established.\n%s\n", getCurrentTime());
		printf(message);			       
		writeToFile(logFile, message);	
	    }
	}
	
	/* Read 32 bits (2 words) from the data source in a single call
	 *  - word 1 contains the control status bits  
	 *  - word 2 contains the data bits.
         */ 
	if (!readFromDataSource(mySourceType, mySource, mySource2, &myFrame, &data, &data1, &data2))		
	    break;

	/* Write the data as is to the raw data file */
	writeToRawDataFile(rawDataFile, data);
	/* Write the data as is to the Extended raw data file - Added April 16, 2007. Dan Gibson */
	writeToExtendedRawDataFile(extendedRawDataFile, data, data1, data2);
	
	/* If there is a connection to the Comlink Server send the 2 word data read from the source */
	if (connected)
	{
	    if (!(connected = writeToMySocket(mySockfd, data)))
	    {
		/* Connection Failed, record the time of failure to be used in isTimeToConnect */
		downTime = time(NULL);

		/* Generate a log message of this event: connection Failed */
		sprintf(message, "\nComlink Server down: Connection Failed.\n%s\n", getCurrentTime());
		printf(message);
		writeToFile(logFile, message);
	    }
	}
	
	/* If the frame is filled; 
	 *  - write the contents to the logfile
	 *  - write the contents to the formatted data file
	 *  - Determine if the frame is valid by passing it through a checksum algorithm, isValid()
	 *  - If the local PACE is enabled and If the frame is valid insert its messages to the PACE Emulation
	 */
	if (myFrame.isFilled())
	{
	    writeToLogFile(logFile, myFrame);
	    writeFrameToFormDataFile(formDataFile, myFrame);
	    
	    if (localPACE && myFrame.isValid())
	    {
		/* resetMsgList resets a pointer in the frame to the first Message
		 *  - When the popFrontMsg function is called teh pointer is incremented
		 *    to the next message
		 */
		myFrame.resetMsgList();
		
#ifdef SIMPACE
		/* Freeze the simulator DCC in order to ensure a successful insertion to the
		 * PACE emulation
		 */
		freezeDcc(channel);

		/* Step through the list of messages within the reconstructed frame
		 * and insert each of them into the PACE emulation
		 */
		while((myMsg = myFrame.popFrontMsg()))
		    insertMsgToPace(myMsg, channel);

		/* Unfreeze the simulator DCC after the messages have been inserted */
		unfreezeDcc(channel);
#endif
	    }	
	    /* Since only one frame is created (like a static object), we need to empty
	     * it for the next frame to be reconstructed.
	     *  - All message are erased and frame variables are reset.
	     *  - A frame counter is incremented.
	     */
	    myFrame.reset();
	}
    }
    
    /* We should never exit the Main Continuous Loop.  No code here will get executed.*/

    return 0;
}
    

/************************************/
/* readParameters Method	    */
/************************************/ 
    
static bool readParameters(int argc, char **argv, bool *localPACE, bool *remotePACE, SOURCE *mySourceType, int *channel)
{
    int i;
    char *new_argv[argc];
    
    for (i=2;i<=argc;i++)
    {	
	new_argv[i-1] = strdup(argv[i-1]);
	toUpperCase(new_argv[i-1]);
    }    
    
    *mySourceType = SRC_PXI;
    for (i = 2; i <= argc; i++){
	if (strcmp(new_argv[i-1], "-C1") == 0)
	    *channel = 1;
	else if (strcmp(new_argv[i-1], "-C2") == 0)
	    *channel = 2;
	else if (strcmp(new_argv[i-1], "-NOLP") == 0)
	    *localPACE = false;
	else if (strcmp(new_argv[i-1], "-NORP") == 0)
	    *remotePACE = false;
	else if (i == argc)
	    *mySourceType = SRC_FILE;
	else {
	    printf("Invalid  arguments.\nExisting options are;\n");
	    printf("-c1     Channel 1\n");
	    printf("-c2     Channel 2\n");
	    printf("-nolp   No local PACE\n");
	    return false;
	}
    }
    return true;
}


/************************************/
/* readFromDataSource Method	    */
/************************************/

static bool readFromDataSource(SOURCE srcType, void *mySource, void *mySource2, frame *myFrame, unsigned int * data, unsigned int * data1, unsigned int * data2)
{	
    bool	eof = false;
    static bool        bDCC_Msg = true;
    static bool        bFirstTime = true;
    int         error;
    char        errBuff[2048];
    char	message[64];
    static int	cnt = 0;
//    unsigned int data1;
//    unsigned int data2;
    unsigned int fixbyte;
//    uInt32	    data1;
//    uInt32	    data2;
	   
    switch(srcType)
    {
	case SRC_PXI:
	    (*data) = 0;  // Initialize Data values. Added April 16, 2007. Dan Gibson
	    (*data1) = 0;
	    (*data2) = 0;
	  
	    readFromMyTask((TaskHandle)mySource,data1, &error);
	    if (hasDAQmxFailed(error)) 
	    {
		writeToFile(logFile, DAQmxError(error,(TaskHandle)mySource, errBuff));
		faultExit();
	    }

	    readFromMyTask((TaskHandle)mySource2, data2, &error);
	    if (hasDAQmxFailed(error)) 
	    {
		writeToFile(logFile, DAQmxError(error,(TaskHandle)mySource2, errBuff));
		faultExit();
		} 
/*    Test to determine if a PACE data read of 'FFFF' causes a 2nd spurious read on the PACE NI board.
      To compensate for this, do two reads in a row when you get a 'FFFF' to try to synch back up with
      the DCC NI board DCC/PACE control lines.
 */
	    if(((*data2) & 0xFFFF)  == 0xFFFF)
	    {
		readFromMyTask((TaskHandle)mySource2, data2, &error);
		if (hasDAQmxFailed(error)) 
		{
		    writeToFile(logFile, DAQmxError(error,(TaskHandle)mySource2, errBuff));
		    faultExit();
		}
	    
	    }
	    
/* need to combine data1 and data2 into data depending on data1 control lines *//* Dan Gibson, Mar. 3, 2006 */
//	    if(0xD900 == (data1 >> 16))  // Frame from DCC.
	    if(0xD == ((*data1) >> 28))  // Frame from DCC.
	    {
		if(!bDCC_Msg || bFirstTime) // If previous word was not from DCC, then reset frame fresh. Dan Gibson, Mar. 21, 2007.
		{
		    myFrame->reset();
		    myFrame->isDCCframe = true;
		    sprintf(message, "\n\n *** Direction change. Message direction is DCC to PACE \n\n");
		    printf(message);
		    writeToFile(logFile, message);
		    bDCC_Msg = true;
		    bFirstTime = false;
		}
	        (*data) = (*data1);
	    }
	    else if(0x0D == (((*data1) >> 24) & 0x5F)) // frame from PACE - 1st byte is X0X0 (0101) where X's are don't cares.
        //  else if(0x0D == (((*data1) >> 24) & 0x7F)) // frame from PACE
	    {
		if(bDCC_Msg || bFirstTime) // If previous word was not from PACE, then reset frame fresh. Dan Gibson, Mar. 21, 2007.
		{
		    myFrame->reset();
		    myFrame->isDCCframe = false;
		    sprintf(message, "\n\n *** Direction change. Message direction is PACE to DCC \n\n");
		    printf(message);
		    writeToFile(logFile, message);
		    bDCC_Msg = false;
		    bFirstTime = false;
		}

/*  This code swaps the four bits of the last and 2nd last byte of data2.  June 6, 2006.
    It is a temporary fix to see if the wiring is wrong.
    Aug 22, 2006 - Appears that the bits need to be mirrored as well.  Added mirrorBits().
    This may not be a temporary fix, as this may reflect the swapping of bits required for
    words from a DEC PDP 11 based hardware.
*/
		fixbyte = ((((*data2) & 0x00000F00) << 4) & 0x0000F000);
		fixbyte = mirrorBits(fixbyte,3);
	        fixbyte = fixbyte | (mirrorBits(((((*data2) & 0x0000F000) >> 4) & 0x00000F00),2));
		(*data2) = ((*data2) & 0xFFFF00FF) | fixbyte ;

		fixbyte = ((((*data2) & 0x0000000F) << 4) & 0x000000F0);
		fixbyte = mirrorBits(fixbyte,1);
	        fixbyte = fixbyte | (mirrorBits(((((*data2) & 0x000000F0) >> 4) & 0x0000000F),0));
		(*data2) = ((*data2) & 0xFFFFFF00) | fixbyte ; 
// End of temporary fix

		(*data) = ((*data1) & 0xFFFF0000) | ((*data2) & 0x0000FFFF);
	    }

	    break;

	case SRC_FILE:
	    if (!eof)
	    {
		if ((eof = readFromMyFile((FILE *)mySource, data)))
		    faultExit();

		if(0xD == (*data >> 28))  // Frame from DCC.
		{
		    if(!bDCC_Msg || bFirstTime) // If previous word was not from DCC, then reset frame fresh. Dan Gibson, Mar. 21, 2007.
		    {
			myFrame->reset();
			myFrame->isDCCframe = true;
			sprintf(message, "\n\n *** Direction change. Message direction is DCC to PACE \n\n");
			printf(message);
			writeToFile(logFile, message);
			bDCC_Msg = true;
			bFirstTime = false;
		    }
		}
		else if(0x0D == (((*data1) >> 24) & 0xAF)) // frame from PACE - 1st byte is X0X0 where X's are don't cares.
		    //	else if(0x0D == ((*data >> 24) & 0x7F)) // frame from PAC  // frame from PACE
		{
		    if(bDCC_Msg || bFirstTime) // If previous word was not from PACE, then reset frame fresh. Dan Gibson, Mar. 21, 2007.
		    {
			myFrame->reset();
			myFrame->isDCCframe = false;
			sprintf(message, "\n\n *** Direction change. Message direction is PACE to DCC \n\n");
			printf(message);
			writeToFile(logFile, message);
			bDCC_Msg = false;
			bFirstTime = false;
		    }
		}
	    }
	    else
		return false; 
	    break;
	default:
	    break;
	}
    
    if (!myFrame->fill(*data, FALSE))
    {
	cnt++;
	if (cnt <= 5)
	{
	    sprintf(message, "Warning: Unexpected word - %8.8X\n", *data);
	    printf(message);
	    writeToFile(logFile, message);
	}
    }
    else
    {
	if (cnt > 0)
	{
	    sprintf(message, "Warning: %d unexpected consecutive words encountered\n", cnt);
	    printf(message);
	    writeToFile(logFile, message);
	}
	cnt = 0;
    }
    
    return true;
}

/************************************/
/* Mirroring the bits of a byte Method */
/* the first arg is an 8 byte value, and
   the 2nd arg is the byte to be mirrored 
   (0 to 7 from right).  This may not be the
   most efficient way to do this.
   D. Gibson, August 22, 2006.  */
/************************************/
static unsigned int mirrorBits(unsigned int in_uint, int bytepos)
{
    unsigned int inbyte;
    unsigned int workbyte;
    unsigned int out_uint;

    inbyte = ((in_uint >> (bytepos*4)) & 0xF);
    workbyte = 0;
    workbyte = (((inbyte & 0x1)  << 3) & 0x8);
    workbyte = workbyte | (((inbyte & 0x2)  << 1) & 0x4);
    workbyte = workbyte | (((inbyte & 0x4)  >> 1) & 0x2);
    workbyte = workbyte | (((inbyte & 0x8)  >> 3) & 0x1);

    out_uint = 0;
    out_uint = (workbyte << (bytepos*4));

    return out_uint;

}

/************************************/
/* displayTitleScreen Method	    */
/************************************/

static void displayTitleScreen(void)
{
    printf("\n ******************************\n");
    printf(" *       COMLINK CLIENT V2    *\n");
    printf(" ******************************\n\n");
}

/************************************/
/* isTimeToConnect Method	    */
/************************************/
	
static bool isTimeToConnect(time_t downTime)
{
    return ((time(NULL) - downTime) >= SCR);
}

/************************************/
/* faultExit Method	            */
/************************************/

static void faultExit()
{
    printf("\n\nShutting down program, press Enter key to quit\n");
    getchar();
    exit(-1);
}
