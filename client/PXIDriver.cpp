#include "PXIDriver.h"
#include "FILEDriver.h"
#include <NIDAQmx.h>


TaskHandle createMyTask(int *error,char *devport,char *clksource)
{
    TaskHandle	taskHandle = 0;	
    //int32 error = 0;
    /*********************************************/
    /* DAQmx Configure Code                      */
    /*********************************************/
    
    //if (DAQmxFailed(error = (DAQmxCreateTask("",&taskHandle))))
	//DAQmxError(error, taskHandle);
    //if (DAQmxFailed(error = (DAQmxCreateDIChan(taskHandle,"Dev2/port0:3","",DAQmx_Val_ChanForAllLines))))
    //DAQmxError(error, taskHandle);
    //if (DAQmxFailed(error = (DAQmxCfgSampClkTiming(taskHandle,"/Dev2/PFI2",17000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,1))))
	//DAQmxError(error, taskHandle);
    
//if (DAQmxFailed(error = (DAQmxCreateDIChan(taskHandle,"PXI1Slot4/port0:3","",DAQmx_Val_ChanForAllLines))))
    //	DAQmxError(error, taskHandle);
    //if (DAQmxFailed(error = (DAQmxCfgSampClkTiming(taskHandle,"/PXI1Slot4/PFI2",17000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,1))))
    //	DAQmxError(error, taskHandle);
    
    if ((DAQmxFailed(*error = (DAQmxCreateTask("",&taskHandle)))) ||
	(DAQmxFailed(*error = (DAQmxCreateDIChan(taskHandle,devport,"",DAQmx_Val_ChanForAllLines)))) ||	
//      (DAQmxFailed(*error = (DAQmxCreateDIChan(taskHandle,"Dev2/port0:3","",DAQmx_Val_ChanForAllLines)))) ||
/* Changed from sampling on rising edge of clock to falling edge of clock due to lost msg checksum from PACE
   Dan Gibson, March 29, 2007. 
   Switched back on same date, but wired to HUB BUSY control line - now picks up all of HUB Date Frame.*/
	(DAQmxFailed(*error = (DAQmxCfgSampClkTiming(taskHandle,clksource,17000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,1)))))
//	(DAQmxFailed(*error = (DAQmxCfgSampClkTiming(taskHandle,clksource,17000.0,DAQmx_Val_Falling,DAQmx_Val_ContSamps,1)))))
// 	(DAQmxFailed(*error = (DAQmxCfgSampClkTiming(taskHandle,"/Dev2/PFI2",17000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,1))))|| 
//	(DAQmxFailed(*error = (DAQmxCfgDigEdgeStartTrig(taskHandle,"/Dev2/PFI6",DAQmx_Val_Rising)))))
    {
	return taskHandle;
    }
    return taskHandle;
}

/*  Added by D.J. Gibson, Mar. 3, 2006 to synchronize two DIO boards */
TaskHandle createMyTriggerTask(int *error)
{
    TaskHandle	taskHandle = 0;

//  Create Trigger task and perform Signal routing.  D.J. Gibson, Mar. 3, 2006.    
    if ((DAQmxFailed(*error = (DAQmxCreateTask("",&taskHandle)))) ||
	(DAQmxFailed(*error = (DAQmxCreateDOChan(taskHandle,"/Dev2/port5/line3","",DAQmx_Val_ChanForAllLines)))) ||	
	(DAQmxFailed(*error = (DAQmxConnectTerms("/Dev2/PFI7","/Dev2/PXI_Trig1",DAQmx_Val_DoNotInvertPolarity))))|| 
	(DAQmxFailed(*error = (DAQmxConnectTerms("/Dev2/PXI_Trig1","/Dev2/PFI6",DAQmx_Val_DoNotInvertPolarity)))))

    {
	return taskHandle;
    }
    return taskHandle;
}

/*  Added by D.J. Gibson, June 2, 2006 to synchronize two DIO boards.  */
void exportClockSignal(int *error,TaskHandle MastertaskHandle,char *ClkDest)
{
    DAQmxFailed(*error = (DAQmxExportSignal(MastertaskHandle,DAQmx_Val_SampleClock,ClkDest)));

}


/*  Added by D.J. Gibson, May. 29, 2006 to synchronize two DIO boards by using a 3rd 6602 board */
void RouteClock(int *error,char *ClkSrc,char *ClkDest)
{
    DAQmxFailed(*error = (DAQmxConnectTerms(ClkSrc,ClkDest,DAQmx_Val_DoNotInvertPolarity)));

}

void startMyTask(TaskHandle taskHandle, int *error)
{
    //int32 error = 0;
    
    /*********************************************/
    /* DAQmx Start Code                          */
    /*********************************************/

    //if (DAQmxFailed(*error = (DAQmxStartTask(taskHandle))))
    //DAQmxError(*error, taskHandle);

    DAQmxFailed(*error = (DAQmxStartTask(taskHandle)));
    
    return;
}

/*  Added by D.J. Gibson, Mar. 3, 2006 to synchronize two DIO boards */
void startMyTriggerTask(TaskHandle taskHandle, int *error)
{
    //int32 error = 0;
    
    /*********************************************/
    /* DAQmx Start Code                          */
    /*********************************************/

    //if (DAQmxFailed(*error = (DAQmxWriteDigitalScalarU32(taskHandle))))
    //DAQmxError(*error, taskHandle);

    DAQmxFailed(*error = (DAQmxWriteDigitalScalarU32(taskHandle,TRUE,10.0,4,NULL)));
    
    return;
}


//void readFromMyTask(TaskHandle taskHandle, uInt32 *myData)
void readFromMyTask(TaskHandle taskHandle, unsigned int *myData, int *error)
{
    uInt32 data[1];
//    int32 error = 0;
    int32 sampsRead = 0;
      // static int32 totalRead = 0;
    
    /*********************************************/
    /* DAQmx Start Code                          */
    /*********************************************/
    
    DAQmxFailed(*error = (DAQmxReadDigitalU32(taskHandle,1,-1,DAQmx_Val_GroupByChannel,data,1,&sampsRead,NULL)));
    
	//DAQmxError(error, taskHandle);
    
    *myData = data[0];
    
    //if (sampsRead > 0) 
    //totalRead += sampsRead;

    //printf("Acquired %d samples. Total %d\r",sampsRead,totalRead);

    
    
    return;
}


bool hasDAQmxFailed(int error)
{
    return (DAQmxFailed(error));
}


char * DAQmxError(int error, TaskHandle taskHandle, char *myErrBuff)
{
    char errBuff[2048]={'\0'};
    
    DAQmxGetExtendedErrorInfo(errBuff,2048);
    if( taskHandle!=0 ) {
	/*********************************************/
	/* DAQmx Stop Code                           */
	/*********************************************/
	DAQmxStopTask(taskHandle);
	DAQmxClearTask(taskHandle);
    }
    if( DAQmxFailed(error) )
    {
	printf("DAQmx Error: %s\n",errBuff);
	sprintf(myErrBuff,"DAQmx Error: %s\n",errBuff);
    }
    return myErrBuff;
    //printf("shutting down program, press Enter key to quit\n");
    //getchar();
    //exit(0);
}
