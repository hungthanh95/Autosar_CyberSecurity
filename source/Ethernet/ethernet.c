/********************************************************************************************************/
/************************************************INCULDES************************************************/
/********************************************************************************************************/

#include "ethernet.h"
#include "SecOC_Debug.h"
#include "SecOC_Lcfg.h"
#include "CanTP.h"
#include "PduR_CanIf.h"
#include "SoAd.h"
#ifdef SCHEDULER_ON
    #include <pthread.h>
#endif
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib") /* Link with Winsock library */
#else
    #include <fcntl.h>
    #include <errno.h>
    #include <unistd.h>
#endif

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static uint8 ip_address_send[16] = "127.0.0.1"; /* Increased size to accommodate null terminator */
extern SecOC_PduCollection PdusCollections[];
#ifdef SCHEDULER_ON
    pthread_mutex_t lock;
#endif 

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void ethernet_init(void) 
{
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            #ifdef ETHERNET_DEBUG
                printf("WSAStartup failed: %d\n", WSAGetLastError());
            #endif
            return;
        }
    #endif

    uint8 ip_address_read[16];
    /* Open the file containing the IP address */
    FILE* fp = fopen("./source/Ethernet/ip_address.txt", "r");
    if (fp == NULL) 
    {
        #ifdef ETHERNET_DEBUG
            printf("Error opening file\n");
        #endif
        #ifdef _WIN32
            WSACleanup();
        #endif
        return;
    }
    
    /* Read the IP address from the file */
    if (fgets(ip_address_read, sizeof(ip_address_read), fp) == NULL) {
        #ifdef ETHERNET_DEBUG
            printf("Error reading IP address from file\n");
        #endif
        fclose(fp);
        #ifdef _WIN32
            WSACleanup();
        #endif
        return;
    }

    /* Close the file */
    fclose(fp);

    #ifdef ETHERNET_DEBUG
        printf("IP is %s\n", ip_address_read);
    #endif

    /* Copy the IP address to the global variable */
    if (strlen(ip_address_read) > 0) 
    {
        ip_address_read[strcspn(ip_address_read, "\n")] = 0; /* Remove newline */
        strncpy(ip_address_send, ip_address_read, sizeof(ip_address_send) - 1);
        ip_address_send[sizeof(ip_address_send) - 1] = '\0'; /* Ensure null termination */
    }

    #ifdef _WIN32
        /* WSACleanup not needed here as init is done once */
    #endif
}

Std_ReturnType ethernet_send(unsigned short id, unsigned char* data, unsigned char dataLen) {
    #ifdef ETHERNET_DEBUG
        printf("######## in Sent Ethernet\n");
    #endif

    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            #ifdef ETHERNET_DEBUG
                printf("WSAStartup failed: %d\n", WSAGetLastError());
            #endif
            return E_NOT_OK;
        }
    #endif

    /* Create a socket */
    SOCKET network_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (network_socket == INVALID_SOCKET) {
        #ifdef ETHERNET_DEBUG
            printf("Create Socket Error: %d\n", 
                   #ifdef _WIN32
                       WSAGetLastError()
                   #else
                       errno
                   #endif
            );
        #endif
        #ifdef _WIN32
            WSACleanup();
        #endif
        return E_NOT_OK;
    }

    /* Specify an address for the socket */
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address)); /* Clear structure */
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT_NUMBER);
    server_address.sin_addr.s_addr = inet_addr(ip_address_send);

    if (server_address.sin_addr.s_addr == INADDR_NONE) {
        #ifdef ETHERNET_DEBUG
            printf("Invalid IP address: %s\n", ip_address_send);
        #endif
        CLOSE_SOCKET(network_socket);
        #ifdef _WIN32
            WSACleanup();
        #endif
        return E_NOT_OK;
    }

    int connection_status = connect(network_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    if (connection_status == SOCKET_ERROR) {
        #ifdef ETHERNET_DEBUG
            printf("Connection Error: %d\n", 
                   #ifdef _WIN32
                       WSAGetLastError()
                   #else
                       errno
                   #endif
            );
        #endif
        CLOSE_SOCKET(network_socket);
        #ifdef _WIN32
            WSACleanup();
        #endif
        return E_NOT_OK;
    }

    /* Prepare data for sending */
    uint8 sendData[BUS_LENGTH_RECEIVE + sizeof(id)] = {0};
    memcpy(sendData, data, dataLen);
    for (unsigned char indx = 0; indx < sizeof(id); indx++) {
        sendData[BUS_LENGTH_RECEIVE + indx] = (id >> (8 * indx)) & 0xFF;
    }

    #ifdef ETHERNET_DEBUG
        for (int j = 0; j < BUS_LENGTH_RECEIVE + sizeof(id); j++)
            printf("%d\t", sendData[j]);
        printf("\n");
    #endif

    /* Send data */
    if (send(network_socket, (const char*)sendData, BUS_LENGTH_RECEIVE + sizeof(id), 0) == SOCKET_ERROR) {
        #ifdef ETHERNET_DEBUG
            printf("Send Error: %d\n", 
                   #ifdef _WIN32
                       WSAGetLastError()
                   #else
                       errno
                   #endif
            );
        #endif
        CLOSE_SOCKET(network_socket);
        #ifdef _WIN32
            WSACleanup();
        #endif
        return E_NOT_OK;
    }

    /* Close the connection */
    CLOSE_SOCKET(network_socket);
    #ifdef _WIN32
        WSACleanup();
    #endif
    return E_OK;
}

Std_ReturnType ethernet_receive(unsigned char* data, unsigned char dataLen, unsigned short* id) {
    #ifdef ETHERNET_DEBUG
        printf("######## in Receive Ethernet\n");
    #endif

    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            #ifdef ETHERNET_DEBUG
                printf("WSAStartup failed: %d\n", WSAGetLastError());
            #endif
            return E_NOT_OK;
        }
    #endif

    /* Create a socket */
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        #ifdef ETHERNET_DEBUG
            printf("Create Socket Error: %d\n", 
                   #ifdef _WIN32
                       WSAGetLastError()
                   #else
                       errno
                   #endif
            );
        #endif
        #ifdef _WIN32
            WSACleanup();
        #endif
        return E_NOT_OK;
    }

    /* Specify an address for the socket */
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT_NUMBER);
    server_address.sin_addr.s_addr = INADDR_ANY;

    /* Set socket options */
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        #ifdef ETHERNET_DEBUG
            printf("setsockopt Error: %d\n", 
                   #ifdef _WIN32
                       WSAGetLastError()
                   #else
                       errno
                   #endif
            );
        #endif
        CLOSE_SOCKET(server_socket);
        #ifdef _WIN32
            WSACleanup();
        #endif
        return E_NOT_OK;
    }

    /* Bind the socket */
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        #ifdef ETHERNET_DEBUG
            printf("Bind Error: %d\n", 
                   #ifdef _WIN32
                       WSAGetLastError()
                   #else
                       errno
                   #endif
            );
        #endif
        CLOSE_SOCKET(server_socket);
        #ifdef _WIN32
            WSACleanup();
        #endif
        return E_NOT_OK;
    }

    /* Listen for connections */
    if (listen(server_socket, 5) == SOCKET_ERROR) {
        #ifdef ETHERNET_DEBUG
            printf("Listen Error: %d\n", 
                   #ifdef _WIN32
                       WSAGetLastError()
                   #else
                       errno
                   #endif
            );
        #endif
        CLOSE_SOCKET(server_socket);
        #ifdef _WIN32
            WSACleanup();
        #endif
        return E_NOT_OK;
    }

    /* Accept a connection */
    SOCKET client_socket = accept(server_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET) {
        #ifdef ETHERNET_DEBUG
            printf("Accept Error: %d\n", 
                   #ifdef _WIN32
                       WSAGetLastError()
                   #else
                       errno
                   #endif
            );
        #endif
        CLOSE_SOCKET(server_socket);
        #ifdef _WIN32
            WSACleanup();
        #endif
        return E_NOT_OK;
    }

    /* Receive data */
    unsigned char recData[dataLen + sizeof(unsigned short)];
    int bytes_received = recv(client_socket, (char*)recData, dataLen + sizeof(unsigned short), 0);
    if (bytes_received == SOCKET_ERROR || bytes_received < (int)(dataLen + sizeof(unsigned short))) {
        #ifdef ETHERNET_DEBUG
            printf("Receive Error: %d\n", 
                   #ifdef _WIN32
                       WSAGetLastError()
                   #else
                       errno
                   #endif
            );
        #endif
        CLOSE_SOCKET(client_socket);
        CLOSE_SOCKET(server_socket);
        #ifdef _WIN32
            WSACleanup();
        #endif
        return E_NOT_OK;
    }

    #ifdef ETHERNET_DEBUG
        printf("in Receive Ethernet \t");
        printf("Info Received: \n");
        for (int j = 0; j < (dataLen + sizeof(unsigned short)); j++) {
            printf("%d ", recData[j]);
        }
        printf("\n\n\n");
    #endif

    #ifdef SCHEDULER_ON
        pthread_mutex_lock(&lock);
    #endif 

    /* Copy received data */
    memcpy(id, recData + dataLen, sizeof(unsigned short));
    memcpy(data, recData, dataLen);

    #ifdef ETHERNET_DEBUG
        printf("id = %d\n", *id);
    #endif

    /* Close sockets */
    CLOSE_SOCKET(client_socket);
    CLOSE_SOCKET(server_socket);
    #ifdef _WIN32
        WSACleanup();
    #endif
    return E_OK;
}

void ethernet_ReceiveMainFunction(void)
{
    static uint8 dataReceive[BUS_LENGTH_RECEIVE];
    uint16 id;
    if (ethernet_receive(dataReceive, BUS_LENGTH_RECEIVE, &id) != E_OK) {
        #ifdef SCHEDULER_ON
            pthread_mutex_unlock(&lock); /* Unlock mutex on error */
        #endif
        return;
    }

    PduInfoType PduInfoPtr = {
        .SduDataPtr = dataReceive,
        .MetaDataPtr = &PdusCollections[id],
        .SduLength = BUS_LENGTH_RECEIVE,
    };

    switch (PdusCollections[id].Type) {
        case SECOC_SECURED_PDU_CANIF:
            #ifdef ETHERNET_DEBUG
                printf("here in Direct\n");
            #endif
            PduR_CanIfRxIndication(id, &PduInfoPtr);
            break;
        case SECOC_SECURED_PDU_CANTP:
            #ifdef ETHERNET_DEBUG
                printf("here in CANTP\n");
            #endif
            CanTp_RxIndication(id, &PduInfoPtr);
            break;
        case SECOC_SECURED_PDU_SOADTP:
            #ifdef ETHERNET_DEBUG
                printf("here in Ethernet SOADTP\n");
            #endif
            SoAdTp_RxIndication(id, &PduInfoPtr);
            break;
        case SECOC_SECURED_PDU_SOADIF:
            #ifdef ETHERNET_DEBUG
                printf("here in Ethernet SOADIF\n");
            #endif
            PduR_SoAdIfRxIndication(id, &PduInfoPtr);
            break;
        case SECOC_AUTH_COLLECTON_PDU:
            #ifdef ETHERNET_DEBUG
                printf("here in Direct - pdu collection - auth\n");
            #endif
            PduR_CanIfRxIndication(id, &PduInfoPtr);
            break;
        case SECOC_CRYPTO_COLLECTON_PDU:
            #ifdef ETHERNET_DEBUG
                printf("here in Direct - pdu collection - crypto\n");
            #endif
            PduR_CanIfRxIndication(id, &PduInfoPtr);
            break;
        default:
            #ifdef SCHEDULER_ON
                pthread_mutex_unlock(&lock);
            #endif
            #ifdef ETHERNET_DEBUG
                printf("This is no type like it for ID: %d type: %d\n", id, PdusCollections[id].Type);
            #endif
            break;
    }

    #ifdef SCHEDULER_ON
        pthread_mutex_unlock(&lock); /* Unlock mutex after successful processing */
    #endif
}