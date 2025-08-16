#ifndef SENDER_H__
#define SENDER_H__

/********************************************************************************************************/
/************************************************INCULDES************************************************/
/********************************************************************************************************/

#include <stdlib.h>
#include "Std_Types.h"
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define SOCKET_ERROR_RETURN SOCKET_ERROR
    #define CLOSE_SOCKET(s) closesocket(s)
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define SOCKET_ERROR_RETURN -1
    #define CLOSE_SOCKET(s) close(s)
#endif

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define PORT_NUMBER 12345
#define BUS_LENGTH_RECEIVE 8


/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : ethernet_init                 *
 * Function_Index       :                               *
 * Function_File        :                               *
 * Function_Descripton  : Used to get the ip that       *
 * send data to                                         *
 *******************************************************/
void ethernet_init(void);


/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : ethernet_send                 *
 * Function_Index       :                               *
 * Function_File        :                               *
 * Function_Descripton  : Used to send the data using   *
 * Sockets                                              *
 *******************************************************/
Std_ReturnType ethernet_send(unsigned short id, unsigned char* data , unsigned char dataLen);


/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : ethernet_send                 *
 * Function_Index       :                               *
 * Function_File        :                               *
 * Function_Descripton  : Used to Receive the data using*
 * Sockets                                              *
 *******************************************************/
Std_ReturnType ethernet_receive(unsigned char* data , unsigned char dataLen, unsigned short* id);


/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : ethernet_ReceiveMainFunction  *
 * Function_Index       :                               *
 * Function_File        :                               *
 * Function_Descripton  : Used to route the data        *
 * Received to protocol                                 *
 *******************************************************/
void ethernet_ReceiveMainFunction(void);

#endif
