#ifndef INCLUDE_GUIINTERFACE_H_
#define INCLUDE_GUIINTERFACE_H_

/********************************************************************************************************/
/************************************************INCULDES************************************************/
/********************************************************************************************************/

#include <inttypes.h>


/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void GUIInterface_init();

char* GUIInterface_authenticate(uint8_t configId, uint8_t *data, uint8_t len);
char* GUIInterface_verify(uint8_t configId);

char* GUIInterface_getSecuredPDU(uint8_t configId, uint8_t *len);
char* GUIInterface_getSecuredRxPDU(uint8_t configId, uint8_t *len , uint8_t * Securedlen);
char* GUIInterface_getAuthPdu(uint8_t configId, uint8_t *len);

void GUIInterface_alterFreshness(uint8_t configId);
void GUIInterface_alterAuthenticator(uint8_t configId);

char* GUIInterface_transmit(uint8_t configId);
char* GUIInterface_receive(uint8_t* rxId , uint8_t* finalRxLen);


// External function declarations
extern void ethernet_init(void);
extern Std_ReturnType FVM_IncreaseCounter(uint16 SecOCFreshnessValueID);
extern Std_ReturnType PduR_SecOCTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
extern void CanTp_MainFunctionTx(void);
extern void SoAd_MainFunctionTx(void);
extern void PduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
extern void CanTp_RxIndication (PduIdType RxPduId, const PduInfoType* PduInfoPtr);
extern void CanTp_MainFunctionRx(void);

#endif
