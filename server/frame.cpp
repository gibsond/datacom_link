// frame
// frame.cpp: implementation of the frame class.
//----------------------------------------------------------------------
// This class is an abstract representation of an actual frame constructed
// by the DCC.  Amoung several local variables, the class contains 
// a linked list of msg objects which are dynmically created as the frame
// is being constructed.  An integer array structured variable is also one of
// these variable which contains all the data in the frame for ease of access.
//
// See msg.cpp for the subclass.
// A frame contains msgs, a msg is accessed by its frame container.
//
// When modified please copy this file to either comlink client or comlink server to preserve file integrity.

#include "frame.h"


/************************************/
/* frame::Construction Method	    */
/************************************/

frame::frame() : length(0), statusWrd(0), frameCnt(0), frameID(0)
{	
}

/************************************/
/* frame::Destrustion Method	    */
/************************************/
frame::~frame()
{
}


/************************************
 * frame::isFilled Method	    *
 ************************************
  Description:
     Determines whether the frame is filled or not.
  
  Parameters:
     NONE
  
  Returns:
     True if frame is filled otherwise false.
 ************************************/
bool frame::isFilled()
{
  if (this->frameCnt != 0)
    return (this->frameCnt == this->length);
  
  return false;
}


/************************************
 * frame::fill Method		    *
 ************************************
  Description:
     Inserts the data into the frame
  
  Parameters:
     Word - word of data to be inserted into the frame
     initCall - If true prevents false returns
  
  Returns:
      True if frame successfully inserted the word or initCall is true,
      False otherwise.
 ************************************/
bool frame::fill(int word, bool initCall)
{
  bool returnValue = true;
  
  /* If frameCnt is 0 it is expecting word == 0xAAAA */  
  if (this->frameCnt == 0){
    if (isHeaderWord1(word)){
      this->statusWrd = ((word >> 16) & 0xFFFF); 
      this->frameCnt++;
    }
    else{
      if (!initCall)
	returnValue = false;
    }
  }

  /* If frameCnt is 1 it is expecting word == 0x5555 */
  else if (this->frameCnt == 1){	
    if (isHeaderWord2(word))
      this->frameCnt++;		
    else{
      this->frameCnt = 0;			
      this->fill(word, initCall);
      if (!initCall)
	returnValue = false;
    }
  }

  /* If frameCnt is 2 it is expecting word to rep. a valid frame length */
  else if (this->frameCnt == 2){
    if (isValidFrameLength(word)){
      this->length = word & 0xFFFF;
      this->frameCnt++;
    }
    else{
      this->frameCnt = 0;
      this->fill(word, initCall);
      if (!initCall)
	returnValue = false;
    }
  }

  /* If frameCnt is 3 it is expecting 1's comp of the valid frame length */
  else if (this->frameCnt == 3){
    if (isOnesComplement(word))
      this->frameCnt++;
    else{
      this->frameCnt = 0;
      this->fill(word, initCall);
      if (!initCall)
	returnValue = false;
    }	
  }
  /* If frameCnt > 3, insert word into a message
     - The function getConstructMsg determines if the word needs to be 
       placed in a new message or continue filling the current message.
  */
  else{		
    if (this->frameCnt < this->length - 2)		
      (*(this->getConstructMsg())).fill(word);
    this->frameCnt++;
  }
  
  /* Ever word from the first to last of the frame is placed in the 
     rawData array. */
  if (frameCnt > 0)
    this->rawData[frameCnt-1] = (word & 0xFFFF);	
  
  return returnValue;
}


/************************************
 * frame::isHeaderWord1 Method	    *
 ************************************
  Description:
   Determines if the parameters second byte, containing the least
     significant bit, is equal to 0xAAAA
  
  Parameters:
     word - 16 bit integer. 
  
  Returns:
      True if the parameter equals 0xAAAA
      False otherwise.
 ************************************/
bool frame::isHeaderWord1(int word)
{
  if (!(0xAAAA == (word & 0xFFFF)))
    return false;
  
  return true;
}


/************************************
 * frame::isHeaderWord2 Method	    *
 ************************************
  Description:
     Determines if the parameters second byte, containing the least
     significant bit, is equal to 0x5555
  
  Parameters:
     word - 32 bit integer.
  
  Returns:
      True if the parameter equals 0x5555
      False otherwise.
 ************************************/
bool frame::isHeaderWord2(int word)
{
  if (!(0x5555 == (word & 0xFFFF)))
    return false;
  
  return true;
}


/************************************
 * frame::isValidFrameLength Method *
 ************************************
  Description:
     Determines if the parameters second byte, containing the 
     least significant bit, is a valid frame length.
  
  Parameters:
     word - 32 bit integer.
  
  Returns:
      True if the parameter is a valid frame length
      False otherwise.
 ************************************/

bool frame::isValidFrameLength(int word)
{
  if (!(((word & 0xFFFF) >= MIN_FRAME_SIZE) && 
	((word & 0xFFFF) <= MAX_FRAME_SIZE))) 
    return false;
  
  return true;
}


/************************************
 * frame::isOnesComplement Method   *
 ************************************
  Description:
     Determines if the parameters second byte containing the 
     least significant bit is the ones compliment of the frame length.
  
  Parameters:
     word - 32 bit integer.
  
  Returns:
      True if the parameter is the ones compliment of the frame length
      False otherwise.
 ************************************/
bool frame::isOnesComplement(int word)
{
  if (!(((this->length) | (word & 0xFFFF)) == 0xFFFF))
    return false;
  
  return true;
}


/************************************
 * frame::getConstructMsg Method    *
 ************************************
  Description:
     Either creates a new message or retrieves the last created message 
     depending on if the last created message is filled or not.
  
  Parameters:
     NONE
  
  Returns:
     The last message in the frames message list.
 ************************************/
list<msg>::iterator frame::getConstructMsg(void)
{
  if (this->msgList.empty() || (*(--this->msgList.end())).isFilled())
    this->msgList.push_back(msg());
  
  return --(this->msgList.end());
}


/************************************
 * frame::isValid Method	    *
 ************************************
  Description:
     Determines whether the frame passes the checksum algorithm.
  
  Parameters:
     NONE
  
  Returns:
     True if the frame is a valid otherwise false.
 ************************************/
bool frame::isValid()
{
  int chksum = 0x44545242;
  int frameChkSum;
  int lsb;
  int i;
  
  if (!this->isFilled())
    return false;
  
  for(i = 0; i < (this->length - 2); i++){
    chksum = chksum ^ (this->rawData[i] << 16);
    lsb = chksum & 1;
    chksum = (chksum >> 1) & 0x7FFFFFFF;
    chksum = chksum | (lsb << 31);
  }
  frameChkSum = ((this->rawData[this->length - 2] << 16) | (this->rawData[this->length - 1] & 0xFFFF));
  
  return (frameChkSum == chksum);	
}


/************************************/
/* frame::getRawData Method	    */
/************************************/

int * frame::getRawData(int index)
{
  return &(this->rawData[index]);
}


/************************************/
/* frame::getStatusWrd Method	    */
/************************************/

int frame::getStatusWrd(void)
{
  return this->statusWrd;
}


/************************************/
/* frame::getFrameID Method	    */
/************************************/

int frame::getFrameID()
{
  return this->frameID;
}


/************************************/
/* frame::popFrontMsg Method	    */
/************************************/

msg * frame::popFrontMsg(void)
{
  list<msg>::iterator preMsgListPtr = this->msgListPtr;
  
  if (preMsgListPtr == this->msgList.end())
    return NULL;
  
  this->msgListPtr++;
  
  return &(*preMsgListPtr);	
}


/************************************/
/* frame::resetMsgList Method	    */
/************************************/

void frame::resetMsgList(void)
{
  this->msgListPtr = msgList.begin();
}


/************************************/
/* frame::getFrameInfo Method	    */
/************************************/

char * frame::getFrameInfo(char * message)
{	
  sprintf(message,"Frame %d length(%d);msg(%d);statWrd(%X)",
	  this->frameID, this->length, this->msgList.size(), this->statusWrd);
  
  return message;
}


/************************************/
/* frame::getFrameMsgInfo Method    */
/************************************/
	
char * frame::getFrameMsgInfo(char * message)
{
  char msgInfo[256];
  list<msg>::iterator iter;
  
  strcpy(message,"");	
  for (iter = this->msgList.begin(); iter != this->msgList.end(); iter++){
    if (strcmp(message, "") != 0)
      strcat(message, "\n");
    strcat(message, "      ");
    strcat(message, (*iter).getMsgInfo(msgInfo));
  }
  strcat(message, "\n");
  
  return message;
}


/************************************/
/* frame::reset Method		    */
/************************************/

void frame::reset()
{
  list<msg>::iterator iter;
  
  this->length = 0;
  this->statusWrd = 0;
  this->frameCnt = 0;
  this->frameID++;
  
  while(!this->msgList.empty())
    this->msgList.erase(this->msgList.begin());
}
