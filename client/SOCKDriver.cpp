#include "SOCKDriver.h"


int createMySocket(void)
{
    int sockfd;
    
    /************************************************/
    /* Socket Open Code (an Internet stream socket) */
    /************************************************/
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	printf("Client: can't open stream socket\n");
    
    return sockfd;
}


bool connectMySocket(int sockfd)
{
    struct sockaddr_in serv_addr;
    
    /*********************************************/
    /* Socket Configure Code                     */
    /*********************************************/
    
    /* 
     * Fill in the structure "serv_addr" with the 
     * address of the server we want to connect with.
     */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
    serv_addr.sin_port = htons(SERV_TCP_PORT);
    
    /*
     * Connect to the server.
     */
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    	return false;
        
    return true;
}

bool writeToMySocket(int sockfd, unsigned int data)
{
    if (writen(sockfd, data, sizeof(data)) <= 0)
	return false;
    return true;
}

int writen(int sockfd, unsigned int data, int nbytes)
{
    //int nwritten;
    int nRet;
    
    //nwritten = write(sockfd, (void *)&data, nbytes);
    nRet = send(sockfd, (char *)&data, nbytes, 0);
    
    return nRet;
}

void closeMySocket(int sockfd)
{
    close(sockfd);
    
    return;
}
