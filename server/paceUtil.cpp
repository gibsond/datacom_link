// util
// util.cpp: implementation of all PACE Emulation Functions.
//----------------------------------------------------------------------
// This file contains all accessor and mutator functions for the PACE emulation common for both comlink server/client
//
// When modified please copy this file to either comlink client or comlink server to preserve file integrity.

#include "paceUtil.h"


/**************************************/
/* cdbAttach method                   */                  
/**************************************/

#ifdef SIMPACE
bool cdbAttach(void)
{
  if(cdb_attach_lite()) {
    fprintf(stderr, "Can't attach to the CDB on the alpha.\n");
    return false;
  }
  return true;
}
#endif


/**************************************/
/* freezeDcc method                   */                  
/**************************************/

#ifdef SIMPACE
void freezeDcc(int channel)
{
  char            label[MAX_LABEL_SIZE];
  struct          fetchkey_format fk;
  int             status;
  IXDCCSTATMSG    *pIXDCCSTATMSGtRec;
  IXDCCSTATMSG    IXDCCSTATMSGtRec;

  if (channel == 1)
      sprintf(label,"IXDCCSTAT");
  else if (channel == 2)
      sprintf(label, "JXDCCSTAT");

  if(!get_fetchkey_status(label, &status, &fk)) {
      fprintf(stderr, "Error:  Couldn't get fetchkey for label IXDCCSTAT to freeze\n");
      return ;      
  }

  pIXDCCSTATMSGtRec = (IXDCCSTATMSG *) FetchkeyToPointer(fk);   
  (*pIXDCCSTATMSGtRec).dccfreeze = 0xFF;

}
#endif

/**************************************/
/* unfreezeDcc method                 */                  
/**************************************/

#ifdef SIMPACE
void unfreezeDcc(int channel)
{
  char            label[MAX_LABEL_SIZE];
  struct          fetchkey_format fk;
  int             status;
  IXDCCSTATMSG    *pIXDCCSTATMSGtRec;
  IXDCCSTATMSG    IXDCCSTATMSGtRec;

 if (channel == 1)
     sprintf(label,"IXDCCSTAT");
 else if (channel == 2)
     sprintf(label, "JXDCCSTAT");

 if(!get_fetchkey_status(label, &status, &fk)) {
     fprintf(stderr, "Error:  Couldn't get fetchkey for label IXDCCSTAT to unfreeze\n");
     return ;      
 }

  pIXDCCSTATMSGtRec = (IXDCCSTATMSG *) FetchkeyToPointer(fk);   
  (*pIXDCCSTATMSGtRec).dccfreeze = 0x00;

}
#endif

/**************************************/
/* insertMsg method                   */                  
/**************************************/

#ifdef SIMPACE
bool insertMsgToPace(msg *myMsg, int channel)
{
  if ((myMsg->getDevCode() == 0x0000) || (myMsg->getDevCode()== 0x0014))
    return insertAnnunMsg(myMsg, channel);  
  else
    return insertReqOutMsg(myMsg, channel);
}
#endif

/**************************************/
/* insertAnnunMsg method              */                  
/**************************************/

#ifdef SIMPACE
bool insertAnnunMsg(msg *myMsg, int channel)
{  
  char            label[MAX_LABEL_SIZE];
  struct          fetchkey_format fk;
  int             status;
  short           sInPtr_sav;
  short    *      psInPtr;
  ANNUNMSG *      pANNUNMSGtRec;
  ANNUNMSG        ANNUNMSGtRec;

  
  sprintf(label,"N%dH00ANNUN_IN_PTR",channel);
  if(!get_fetchkey_status(label, &status, &fk)) {
      fprintf(stderr, "Error:  Couldn't get fetchkey for label %s\n", label);
      return false;      
  }
  psInPtr = (short *)FetchkeyToPointer(fk);

  sInPtr_sav = (*psInPtr)+1;
  if (sInPtr_sav >= ANNUNSTACKSTRU)
    sInPtr_sav = 0;

  if (sInPtr_sav < 0)
      oops();

  sprintf(label,"N%dH00ANNUN_STACK(%d)",channel, sInPtr_sav);
  if(!get_fetchkey_status(label, &status, &fk)) {
    fprintf(stderr, "Error:  Couldn't get fetchkey for label %s\n", label);
    return false;
  }
  pANNUNMSGtRec = (ANNUNMSG *)FetchkeyToPointer(fk);   
  
  printf("Copying a new Message to ANNUN STACK location %d\n", sInPtr_sav);

  ANNUNMSGtRec.sPaceTime = (unsigned short)(myMsg->getPaceTime());
  ANNUNMSGtRec.sMessageId = (unsigned short)(myMsg->getMsgID());
  ANNUNMSGtRec.sCatCount = (unsigned short)(myMsg->getCatCount());  
  ANNUNMSGtRec.sVariable1 = (unsigned short)*(myMsg->getVarData(0));
  ANNUNMSGtRec.sVariable2 = (unsigned short)*(myMsg->getVarData(1));
  ANNUNMSGtRec.sVariable3 = (unsigned short)*(myMsg->getVarData(2));
  ANNUNMSGtRec.sVariable4 = (unsigned short)*(myMsg->getVarData(3));
  ANNUNMSGtRec.sVariable5 = (unsigned short)*(myMsg->getVarData(4));

  memcpy(pANNUNMSGtRec,&ANNUNMSGtRec,sizeof(ANNUNMSGtRec));  
  /*
  printf("Writing the following message to CDB %s;", label);
  printf("STACK MSG  ... %d\n",sInPtr_sav);
  printf("Event Time ... %.4X\n",(*pANNUNMSGtRec).sPaceTime);
  printf("Message ID ... %.4X\n",(*pANNUNMSGtRec).sMessageId);
  printf("Var Count  ... %.4X\n",(*pANNUNMSGtRec).sCatCount);
  printf("Variable 1 ... %.4X\n",(*pANNUNMSGtRec).sVariable1);
  printf("Variable 2 ... %.4X\n",(*pANNUNMSGtRec).sVariable2);
  printf("Variable 3 ... %.4X\n",(*pANNUNMSGtRec).sVariable3);
  printf("Variable 4 ... %.4X\n",(*pANNUNMSGtRec).sVariable4);
  printf("Variable 5 ... %.4X\n\n",(*pANNUNMSGtRec).sVariable5);
  */
  (*psInPtr)++;
  if (*psInPtr < 0)
      oops();

  if (*psInPtr >= ANNUNSTACKSTRU || 
      *psInPtr < 0)
    *psInPtr = 0;
  
  return true;
}
#endif

/**************************************/
/* oops method                        */                  
/**************************************/

void oops(void)
{
    printf("N1H00ANNUN_IN_PTR is negative");
}

/**************************************/
/* insertReqOutMsg method             */                  
/**************************************/

#ifdef SIMPACE
bool insertReqOutMsg(msg *myMsg, int channel)
{  
  char            label[MAX_LABEL_SIZE];
  struct          fetchkey_format fk;
  int             status;
  int             i;
  short     *     psInPtr;
  short           sInPtr_sav;
  REQOUTMSG *     pREQOUTMSGtRec;
  REQOUTMSG       REQOUTMSGtRec;
  
  sprintf(label,"N%dH00REQOUT_IN_PTR",channel);
  if(!get_fetchkey_status(label, &status, &fk)) {
    fprintf(stderr, "Error:  Couldn't get fetchkey for label %s\n",label);
    return false;      
  }
  psInPtr = (short *)FetchkeyToPointer(fk);
  
  sInPtr_sav = (*psInPtr)+1;
  if (sInPtr_sav>= REQOUTSTACKSTRU)
    sInPtr_sav = 0;
  
  sprintf(label,"N%dH00REQ_OUT_STACK(%d)",channel,sInPtr_sav);
  if(!get_fetchkey_status(label, &status, &fk)) {
    fprintf(stderr, "Error:  Couldn't get fetchkey for label %s\n",label);
    return false;
  }
  pREQOUTMSGtRec = (REQOUTMSG *)FetchkeyToPointer(fk);   
  
  printf("Copying a new Message to %s location %d\n", label, sInPtr_sav);
  
  REQOUTMSGtRec.sPaceTime = (unsigned short)(myMsg->getPaceTime());
  REQOUTMSGtRec.sMessageId = (unsigned short)(myMsg->getMsgID());
  REQOUTMSGtRec.sCatCount = (unsigned short)(myMsg->getCatCount());
  for(i = 0;i < myMsg->getVarCnt();i++)
    REQOUTMSGtRec.sVariable[i] = (unsigned short)*(myMsg->getVarData(i));
    
  memcpy(pREQOUTMSGtRec,&REQOUTMSGtRec,sizeof(REQOUTMSGtRec));  

  /*printf("Writing the following message to CDB %s;", label);
  printf("STACK MSG  ... %d\n",(*psInPtr) + 1);
  printf("Event Time ... %.4X\n",(*pREQOUTMSGtRec).sPaceTime);
  printf("Message ID ... %.4X\n",(*pREQOUTMSGtRec).sMessageId);
  printf("Var Count  ... %.4X\n",(*pREQOUTMSGtRec).sCatCount);
  for(i = 0;i < myMsg->getVarCnt();i++)
  printf("Var %d   ... %.4X\n",i,(*pREQOUTMSGtRec).sVariable[i]);*/
  
  (*psInPtr)++;
  if (*psInPtr >= REQOUTSTACKSTRU)
    *psInPtr = 0;
  
  return true;
}
#endif
