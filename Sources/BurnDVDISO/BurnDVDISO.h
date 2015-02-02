#ifndef BURNDVDISO_H
#define BURNDVDISO_H

struct FINDCONTEXT
{
	DWORD dwPortID;         // Port Id - Logical SCSI adapter ID if ASPI is used
	DWORD dwBusID;          // Bus ID - 0 if ASPI is used
	DWORD dwTargetID;       // Target Id
	DWORD dwLUN;            // LUN (Logical Unit Number)
	CHAR  cVendorID[1024];  // 
	CHAR  cProductID[1024]; //
	CHAR  cProductRL[1024]; //
	DWORD dwSizeBuf;        //
	DWORD dwNumDevice;      // указанный пользователем номер из списка устройств
	CHAR  cLetter[7];       // указанная пользователем буква
	BOOL  bLetter;          // искать по букве устройства
	BOOL  bFind;            // устройство найдено - TRUE
};

#endif