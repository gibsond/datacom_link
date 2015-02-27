// util
// util.cpp: implementation of all common functions for comlink.
//----------------------------------------------------------------------
// This file contains all general functions common for both comlink server/client
//
// When modified please copy this file to either comlink client or comlink server to preserve file integrity.


#include "util.h"

/************************************/
/* toUpperCase Method		    */
/************************************/

char * toUpperCase(char * string)
{
  int i;
  for (i = 0; string[i] != '\0'; i++)
    string[i] = toupper(string[i]);
  
  return string;
}

/************************************/
/* createFile Method	      	    */
/************************************/

FILE * createFile(char * timeStamp, FILETYPE fileType)
{
  char	filename[256];
  
  switch (fileType){
  case FILE_RAW:
    sprintf(filename, "rawDataFile_%s.txt",timeStamp);
    break;
  case FILE_FOR:
    sprintf(filename, "formDataFile_%s.csv", timeStamp);
    break;
  case FILE_LOG:
    sprintf(filename, "logFile_%s.txt", timeStamp);
    break;
  default:
    break;
  }
  printf("Creating %s\n", filename);	
  
  return fopen(filename, "w"); //fopen(filename, "w");
}

/************************************/
/* createTimeStamp Method	    */
/************************************/

char * createTimeStamp(char * timeStamp)
{
  struct tm	*today;
  struct timeb	tstruct;
  time_t	ltime;
  char		ms[8];
  
  /* Use time structure to build a customized time string. */
  time( &ltime );
  ftime( &tstruct );
  sprintf( ms, "%u", tstruct.millitm );
  
  /* Use strftime to build a customized time string. */
  today = localtime( &ltime );
  strftime(timeStamp, 128,
	   "%Y-%b-%d_%H-%M-%S-\0",today);
  strcat(timeStamp, ms);

  return timeStamp;
}

/************************************/
/* writeToFile Method		    */
/************************************/

int writeToFile(FILE * outFile, char * message)
{	
  int nret;
  
  nret = fprintf(outFile, "%s", message);	
  fflush(outFile);
  
  return nret;	
}


/************************************/
/* writeFrameToRawDataFile Method   */
/************************************/

bool writeFrameToRawDataFile(FILE * outFile, frame myFrame)
{
  int	  i;
  char	  message[32];
  uInt32* data = (uInt32 *)myFrame.getRawData(0);
  uInt32  statusWrd = (uInt32)myFrame.getStatusWrd();
  
  
  for (i = 0; i < (data[2] & 0xFFFF); i++){		
    sprintf(message, "%8.8X\n", ((statusWrd << 16) & 0xFFFF0000) | data[i]);
    if (writeToFile(outFile, message) <= 0) 
      return false;
  }
  
  return true;		
}

/************************************/
/* writeFrameToFormDataFile Method  */
/************************************/

bool writeFrameToFormDataFile(FILE * outFile, frame myFrame)
{
  int	  i;
  char	  message[4096];
  char	  devCodeStr[64];
  uInt32* data;
  uInt32  statusWrd = (uInt32)myFrame.getStatusWrd();	
  msg *   myMsg;
  
  myFrame.resetMsgList();
  myMsg = myFrame.popFrontMsg();
  
  while(myMsg){	
    data = (uInt32 *)myMsg->getVarData(0);
    sprintf(message, "%s,%4.4X,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%s,%s,%4.4X,%d,%s,%d,",
	    myFrame.isValid()?"PASS":"FAIL",
	    statusWrd,
	    ((statusWrd >> 0x08) & 0x80)?1:0,	 // 80 HEX = 10000000 BINARY
	    ((statusWrd >> 0x08) & 0x40)?1:0,	 // 40 HEX = 01000000 BINARY
	    ((statusWrd >> 0x08) & 0x20)?1:0,	 // 20 HEX = 00100000 BINARY
	    ((statusWrd >> 0x08) & 0x10)?1:0,	 // 10 HEX = 00010000 BINARY
	    ((statusWrd >> 0x08) & 0x08)?1:0,	 // 08 HEX = 00001000 BINARY
	    ((statusWrd >> 0x08) & 0x04)?1:0,	 // 04 HEX = 00000100 BINARY
	    ((statusWrd >> 0x08) & 0x02)?1:0,	 // 02 HEX = 00000010 BINARY
	    ((statusWrd >> 0x08) & 0x01)?1:0,	 // 01 HEX = 00000001 BINARY
	    myMsg->getEventTime(0), myMsg->getEventTime(1), 
	    myMsg->getEventTime(2), myMsg->getEventTime(3),
	    myMsg->getMsgID(),
	    myMsg->getDevCode(),			
	    myMsg->devCodeToString(myMsg->getDevCode(), devCodeStr),
	    myMsg->getVarCnt());
    
    for(i = 0; i < myMsg->getVarCnt(); i++)
      if (i == 0)
	sprintf(message, "%s%4.4X",message, data[i]);
      else 
	sprintf(message, "%s %4.4X",message, data[i]);
    
    strcat(message, "\n");
    writeToFile(outFile, message);	
    myMsg = myFrame.popFrontMsg();
  }
  return true;
}


/************************************/
/* writeHeaderToFormDataFile Method */
/************************************/

bool writeHeaderToFormDataFile(FILE * outFile)
{
  char message[256];
  
  strcpy(message, "CheckSum,StatusWord,STB,STA,RDY,FN1,FN2,FN3,ATTN,BSY,EventTime 0,EventTime 1,EventTime 2,EventTime 3,MsgID,DevCode,DevDecoded,VarCnt,Variables\n");
  if (writeToFile(outFile, message) <= 0) 
    return false;
  
  return true;
}

/************************************/
/* getCurrentTime Method	    */
/************************************/

char * getCurrentTime(void)
{
  time_t      rawtime;
  struct tm * timeinfo;
  
  time ( &rawtime );
  timeinfo = localtime (&rawtime);
  
  return asctime(timeinfo);
}

/************************************/
/* writeToLogFile Method	    */
/************************************/

void writeToLogFile(FILE * outFile, frame myFrame)
{ 
  char myFrameInfo[1024];
  char myFrameMsgInfo[20000];
  char message[20000];
  char timeStamp[64];
  
  sprintf(message, "Recv: %s;%s %s%s",
	  myFrame.getFrameInfo(myFrameInfo),
	  myFrame.isValid()?"PASS":"FAIL",	  
	  createTimeStamp(timeStamp),
	  myFrame.getFrameMsgInfo(myFrameMsgInfo));
  writeToFile(outFile, message);
  printf(message);
}


/************************************/
/* writeToRawDataFile Method	    */
/************************************/

void writeToRawDataFile(FILE * outFile, unsigned int myData)
{
  char message[64];
  
  sprintf(message, "%8.8X\n",myData);
  writeToFile(outFile, message);
}
