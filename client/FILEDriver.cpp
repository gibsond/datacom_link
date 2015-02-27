#include "FILEDriver.h"

FILE * createMyFile(char *argv)
{
    return fopen(argv, "r");
}


bool readFromMyFile(FILE *inFile, unsigned int *myData)
{
    char *	   ptr = NULL;
    char *	   line = NULL;
    char	   oneLine[600];
    char           dataStrSeg[5];
    char           statStrSeg[5];
    unsigned int   dataIntSeg;
    unsigned int   statIntSeg;	
    
    if(line = fgets(oneLine, sizeof(oneLine), inFile))
    {
	strncpy(statStrSeg, "\0", sizeof(statStrSeg));
	strncpy(dataStrSeg, "\0", sizeof(dataStrSeg));
	strncpy(statStrSeg, line,   4);
	strncpy(dataStrSeg, line+4, 4);
	
	dataIntSeg = strtol(dataStrSeg, &ptr, 16);
	statIntSeg = strtol(statStrSeg, &ptr, 16);
	
	statIntSeg = statIntSeg << 16;
	
	*myData = (statIntSeg | dataIntSeg);
	
	return false;
    }
    else
	return true;
}

bool fileExists(const char *path)
{
    /* exists and is readable */

    return (access(path, R_OK) == 0);
}


