// msg
// msg.cpp: implementation of the msg class.
//----------------------------------------------------------------------
// This class is an abstract representation of an actual message constructed
// by the DCC sent within a frame. 
//
// See frame.cpp for the parent class.
// A message object exits inside a frame object.
//
// When modified please copy this file to either comlink client or comlink server to preserve file integrity.

#include "msg.h"


/************************************/
/* msg::Construction Method	    */
/************************************/

msg::msg() : msgID(0), devCode(0), variableCnt(0), msgCnt(0), paceTime(0), catCount(0)
{
}


/************************************/
/* msg::Destruction Method	    */
/************************************/

msg::~msg()
{
}


/************************************/
/* msg::isFilled Method		    */
/************************************/

bool msg::isFilled()
{
  if (this->msgCnt != 0)
    return ((this->msgCnt - 3) == this->variableCnt);
  
  return false;
}


/************************************/
/* msg::fill Method		    */
/************************************/

void msg::fill(int word)
{
  word = word & 0xFFFF;
  
  if (this->msgCnt == 0){
    this->paceTime = word;
    setEventTimes(word);
  }
  else if (this->msgCnt == 1) 
    this->msgID = word;	
  else if (this->msgCnt == 2){
    this->catCount = word;
    this->devCode = (word >> 10);
    this->variableCnt = word & 0x3FF;
  }
  else
    this->variable[this->msgCnt - 3] = word;
  
  this->msgCnt++;
  
  return;
}


/************************************/
/* msg::getPaceTime Method	    */
/************************************/

int msg::getPaceTime(void)
{
  return this->paceTime;
}


/************************************/
/* msg::getEventTime Method	    */
/************************************/

char * msg::getEventTime(int choice)
{
  switch(choice){
  case 0:
  case 1:
  case 2:
  case 3:
    return this->eventTime[choice];
    break;
  default:
    return NULL;
    break;
  }
}


/************************************/
/* msg::getMsgID Method		    */
/************************************/

int msg::getMsgID(void)
{
  return this->msgID;
}


/************************************/
/* msg::getCatCount Method	    */
/************************************/

int msg::getCatCount(void)
{
  return this->catCount;
}

/************************************/
/* msg::getDevCode Method	    */
/************************************/

int msg::getDevCode(void)
{
  return this->devCode;
}

/************************************/
/* msg::getVarCnt Method	    */
/************************************/

int msg::getVarCnt()
{
  return this->variableCnt;
}

/************************************/
/* msg::getVarInfo Method	    */
/************************************/

int * msg::getVarData(int index)
{
  return &(this->variable[index]);
}

/************************************/
/* frame::getMsgInfo Method	    */
/************************************/

char * msg::getMsgInfo(char * message)
{
  char devCodeString[64];  
  
  sprintf(message,"Msg %4.4X varCnt(%d);devCode(%d - %s);eventTime(%s | %s | %s | %s)",
	  this->msgID, this->variableCnt, this->devCode, 
	  devCodeToString(this->devCode, devCodeString),
	  this->eventTime[0], this->eventTime[1], this->eventTime[2], this->eventTime[3]);
  
  return message;
}


/************************************/
/* frame::devCodeToString Method    */
/************************************/

char * msg::devCodeToString(int devCode, char * devCodeString)
{
  switch(devCode){
  case 0x0000:
    strcpy(devCodeString, "alarm and summary messages");
    break;
  case 0x0001:
    strcpy(devCodeString, "IBM 1816 TTY messages");
    break;
  case 0x0002:
    strcpy(devCodeString, "old data link messages");
    break;
  case 0x0003:
    strcpy(devCodeString, "tape punch messages (tape write from IBM 1800)");
    break;
  case 0x0004:
    strcpy(devCodeString, "tape reader messages (tape read to IBM 1800)");
    break;
  case 0x0007:
    strcpy(devCodeString, "IBM 1800 maintenance page messages");
    break;
  case 0x0014:
    strcpy(devCodeString, "M200 printer messages");
    break;
  case 0x0018:
    strcpy(devCodeString, "F/M quadrant 1 message");
    break;
  case 0x001A:
    strcpy(devCodeString, "F/M quadrant 1 message, clear quadrant");
    break;
  case 0x001C:
    strcpy(devCodeString, "F/M quadrant 2 messages");
    break;
  case 0x001E:
    strcpy(devCodeString, "F/M quadrant 2 messages, clear quadrant");
    break;
  case 0x0020:
    strcpy(devCodeString, "F/M quadrant 3 messages");
    break;
  case 0x0022:
    strcpy(devCodeString, "F/M quadrant 3 messages, clear quadrant");
    break;
  case 0x0024:
    strcpy(devCodeString, "F/M quadrant 4 messages");
    break;
  case 0x0026:
    strcpy(devCodeString, "F/M quadrant 4 messages, clear quadrant");
    break;
  case 0x0028:
    strcpy(devCodeString, "analog alarm summary messages");
    break;
  case 0x0029:
    strcpy(devCodeString, "contact alarm summary messages");
    break;
  case 0x002A:
    strcpy(devCodeString, "TPM alarm summary messages");
    break;
  case 0x002B:
    strcpy(devCodeString, "setpoint checking summary messages");
    break;
  case 0x002C:
    strcpy(devCodeString, "contact alarm jumper summary messages");
    break;
  case 0x002D:
    strcpy(devCodeString, "analog input and core trend messages");
    break;
  case 0x002E:
    strcpy(devCodeString, "football (TPM) messages");
    break;
  case 0x002F:
    strcpy(devCodeString, "numerical variables (CRT display) messages");
    break;
  case 0x0030:
    strcpy(devCodeString, "request handler response messages");
    break;
  case 0x0031:
    strcpy(devCodeString, "key parameter update messages");
    break;
  case 0x0032:
    strcpy(devCodeString, "data link messages");
    break;
  case 0x0033:
    strcpy(devCodeString, "unit (hourly) log messages");
    break;
  case 0x0034:
    strcpy(devCodeString, "reactivity log messages");
    break;
  case 0x0035:
    strcpy(devCodeString, "station power log messages");
    break;
  case 0x0036:
    strcpy(devCodeString, "DCC status messages");
    break;
  case 0x0037:
    strcpy(devCodeString, "TPM response messages");
    break;
  case 0x0038:
    strcpy(devCodeString, "TPM messages");
    break;
  case 0x0039:
    strcpy(devCodeString, "RTD bias messages");
    break;
  case 0x003A:
    strcpy(devCodeString, "F/H setpoint messages");
    break;
  case 0x003B:
    strcpy(devCodeString, "contact jumper messages");
    break;
  case 0x003C:
    strcpy(devCodeString, "TPM RTD bias messages");
    break;
  case 0x003D:
    strcpy(devCodeString, "TPM hourly log messages");
    break;
  case 0x003F:
    strcpy(devCodeString, "turbine run-up display messages");
    break;
  default:
    strcpy(devCodeString, "Unknown");
    break;
  }
  return devCodeString;
}


/************************************/
/* frame::setEventTimes Method	    */
/************************************/

void msg::setEventTimes(int word)
{
  int i;
  char myTime[8];
   
  for (i = 0; i < 4; i++){
    strncpy(myTime, "", sizeof(myTime));
    word = word & 0xFFFF;
    word = word | i*0x10000;
    sprintf(myTime, "%6.6d", word);			
    this->eventTime[i][0] = myTime[0];
    this->eventTime[i][1] = myTime[1];
    this->eventTime[i][2] = ':';
    this->eventTime[i][3] = myTime[2];
    this->eventTime[i][4] = myTime[3];
    this->eventTime[i][5] = ':';
    this->eventTime[i][6] = myTime[4];
    this->eventTime[i][7] = myTime[5];
    this->eventTime[i][8] = '\0';
  }
}
