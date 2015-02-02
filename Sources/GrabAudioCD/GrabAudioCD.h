#pragma once

//
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
	BOOL  bFind;            // устройство найдено - TRUE
	BOOL  bLetter;          // искать по букве устройства
};

//
struct GRABCONTEXT
{
	LARGE_INTEGER liLastReadenPercent;   // last readen percent
	LARGE_INTEGER liNumberOfBadBlocks;   // number of bad blocks
	LARGE_INTEGER liNumberOfReadRetries; // number of read retries
};
