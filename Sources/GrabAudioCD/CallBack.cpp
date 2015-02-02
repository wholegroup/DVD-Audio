#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "..\StarBurn\StarBurn.h"
#include "GrabAudioCD.h"
#include "CallBack.h"

//////////////////////////////////////////////////////////////////////////
// CallBack для обнаружения устройств и вывода информации о них на консоль
// 
// cbNumber   - номер CallBack'а
// pvContext  - указатель на DWORD - количество найденных устройств
// pvSpecial1 - указатель на структуру с информацией об устройстве
// pvSpecial2 - ...
//
VOID __stdcall CallBackPrint(IN CALLBACK_NUMBER cbNumber, IN PVOID pvContext, IN PVOID pvSpecial1, IN PVOID pvSpecial2)
{
	EXCEPTION_NUMBER        expNum = EN_SUCCESS;  //
	PSCSI_DEVICE_ADDRESS    pDevice;              // инфа о девайсе
	PVOID                   pvGrabber = NULL;     // 
	ULONG                   ulStatus;             //
	CDB_FAILURE_INFORMATION cdbInf;               //
	CHAR                    cVendorID[1024];      // производитель
	CHAR                    cProductID[1024];     // продукт 
	CHAR                    cProductRL[1024];     // номер
	ULONG                   ulSizeBuf;            // размер кэша
	CHAR                    cDeviceName[1024];    // буква устройства

	switch(cbNumber)
	{
		// найдено устройство
		case CN_FIND_DEVICE:
		{
			pDevice = (PSCSI_DEVICE_ADDRESS)pvSpecial1;

			__try	
			{
				// открытие девайса
				expNum = StarBurn_CdvdBurnerGrabber_Create(&pvGrabber, 
					NULL, 0, &ulStatus, &cdbInf, NULL, NULL, 
					pDevice->m__UCHAR__PortID, 
					pDevice->m__UCHAR__BusID,
					pDevice->m__UCHAR__TargetID,
					pDevice->m__UCHAR__LUN, 1);

				if (EN_SUCCESS != expNum)
				{
					_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_Create\n"));

					__leave;
				}

				// получение информации о девайсе
				RtlZeroMemory(&cVendorID,  sizeof(cVendorID));
				RtlZeroMemory(&cProductID, sizeof(cProductID));
				RtlZeroMemory(&cProductRL, sizeof(cProductRL));

				StarBurn_CdvdBurnerGrabber_GetDeviceInformation(pvGrabber, 
					(PCHAR)&cVendorID, (PCHAR)&cProductID, (PCHAR)&cProductRL, &ulSizeBuf);

				while (strlen(cVendorID) && (' ' == cVendorID[strlen(cVendorID) - 1]))
				{
					cVendorID[strlen(cVendorID) - 1] = 0;
				}

				while (strlen(cProductID) && (' ' == cProductID[strlen(cProductID) - 1]))
				{
					cProductID[strlen(cProductID) - 1] = 0;
				}

				while (strlen(cProductRL) && (' ' == cProductRL[strlen(cProductRL) - 1]))
				{
					cProductRL[strlen(cProductRL) - 1] = 0;
				}

				RtlZeroMemory(&cDeviceName, sizeof(cDeviceName));

				// получение буквы устройства
				expNum = StarBurn_GetDeviceNameByDeviceAddress(
					pDevice->m__UCHAR__PortID, pDevice->m__UCHAR__BusID, pDevice->m__UCHAR__TargetID, pDevice->m__UCHAR__LUN,
					(PCHAR)&cDeviceName);

				if (EN_SUCCESS != expNum)
				{
					_tprintf(_T("ERROR::StarBurn_GetDeviceNameByDeviceAddress\n"));

					__leave;
				}

				(*(DWORD*)pvContext)++; // количество обнаруженных устройств

				_tprintf(_T("  %d. %s [%ld:%ld:%ld:%ld] %s %s rev.%s (cache: %ld)\n"), *(DWORD*)pvContext, &cDeviceName[4],
					pDevice->m__UCHAR__PortID, pDevice->m__UCHAR__BusID, pDevice->m__UCHAR__TargetID, pDevice->m__UCHAR__LUN,
					cVendorID, cProductID, cProductRL, ulSizeBuf);
			}
			
			__finally
			{
				if (NULL != pvGrabber)
				{
					StarBurn_Destroy(&pvGrabber);
				}
			}
		}
		break;
	}
}


//////////////////////////////////////////////////////////////////////////
// CallBack для поиска указанного пользователем устройства
// 
// cbNumber   - номер CallBack'а
// pvContext  - указатель на FINDCONTEXT
// pvSpecial1 - указатель на структуру с информацией об устройстве
// pvSpecial2 - ...
//
VOID __stdcall CallBackFind(IN CALLBACK_NUMBER cbNumber, IN PVOID pvContext, IN PVOID pvSpecial1, IN PVOID pvSpecial2)
{
	EXCEPTION_NUMBER        expNum = EN_SUCCESS;  //
	PSCSI_DEVICE_ADDRESS    pDevice;              // инфа о девайсе
	PVOID                   pvGrabber = NULL;     // 
	ULONG                   ulStatus;             //
	CDB_FAILURE_INFORMATION cdbInf;               //
	CHAR                    cDeviceName[1024];    // буква устройства
	FINDCONTEXT*            findContext = (FINDCONTEXT*)pvContext;

	// выходим, если устройство уже найдено
	if (findContext->bFind)
	{
		return;
	}

	switch(cbNumber)
	{
		// найдено устройство
		case CN_FIND_DEVICE:
		{
			pDevice = (PSCSI_DEVICE_ADDRESS)pvSpecial1;

			__try	
			{
				// открытие девайса
				expNum = StarBurn_CdvdBurnerGrabber_Create(&pvGrabber, 
					NULL, 0, &ulStatus, &cdbInf, NULL, NULL, 
					pDevice->m__UCHAR__PortID, 
					pDevice->m__UCHAR__BusID,
					pDevice->m__UCHAR__TargetID,
					pDevice->m__UCHAR__LUN, 1);

				if (EN_SUCCESS != expNum)
				{
					_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_Create\n"));

					__leave;
				}

				// проверка по номеру
				if (!findContext->bLetter)
				{
					findContext->dwNumDevice--;

					if (!findContext->dwNumDevice)
					{
						findContext->dwBusID    = pDevice->m__UCHAR__BusID;
						findContext->dwPortID   = pDevice->m__UCHAR__PortID;
						findContext->dwTargetID = pDevice->m__UCHAR__TargetID;
						findContext->dwLUN      = pDevice->m__UCHAR__LUN;

						findContext->bFind      = TRUE;

						// получение информации о девайсе
						RtlZeroMemory(&findContext->cVendorID,  sizeof(findContext->cVendorID));
						RtlZeroMemory(&findContext->cProductID, sizeof(findContext->cProductID));
						RtlZeroMemory(&findContext->cProductRL, sizeof(findContext->cProductRL));

						StarBurn_CdvdBurnerGrabber_GetDeviceInformation(pvGrabber, 
							(PCHAR)&findContext->cVendorID, (PCHAR)&findContext->cProductID, 
							(PCHAR)&findContext->cProductRL, &findContext->dwSizeBuf);

						while (strlen(findContext->cVendorID) && (' ' == findContext->cVendorID[strlen(findContext->cVendorID) - 1]))
						{
							findContext->cVendorID[strlen(findContext->cVendorID) - 1] = 0;
						}

						while (strlen(findContext->cProductID) && (' ' == findContext->cProductID[strlen(findContext->cProductID) - 1]))
						{
							findContext->cProductID[strlen(findContext->cProductID) - 1] = 0;
						}

						while (strlen(findContext->cProductRL) && (' ' == findContext->cProductRL[strlen(findContext->cProductRL) - 1]))
						{
							findContext->cProductRL[strlen(findContext->cProductRL) - 1] = 0;
						}
					}

					__leave;
				}

				// проверка по букве
				RtlZeroMemory(&cDeviceName, sizeof(cDeviceName));

				expNum = StarBurn_GetDeviceNameByDeviceAddress(
					pDevice->m__UCHAR__PortID, pDevice->m__UCHAR__BusID, pDevice->m__UCHAR__TargetID, pDevice->m__UCHAR__LUN,
					(PCHAR)&cDeviceName);

				if (EN_SUCCESS != expNum)
				{
					_tprintf(_T("ERROR::StarBurn_GetDeviceNameByDeviceAddress\n"));

					__leave;
				}

				if (!strcmp(cDeviceName, findContext->cLetter))
				{
					findContext->dwBusID    = pDevice->m__UCHAR__BusID;
					findContext->dwPortID   = pDevice->m__UCHAR__PortID;
					findContext->dwTargetID = pDevice->m__UCHAR__TargetID;
					findContext->dwLUN      = pDevice->m__UCHAR__LUN;

					findContext->bFind      = TRUE;

					// получение информации о девайсе
					RtlZeroMemory(&findContext->cVendorID,  sizeof(findContext->cVendorID));
					RtlZeroMemory(&findContext->cProductID, sizeof(findContext->cProductID));
					RtlZeroMemory(&findContext->cProductRL, sizeof(findContext->cProductRL));

					StarBurn_CdvdBurnerGrabber_GetDeviceInformation(pvGrabber, 
						(PCHAR)&findContext->cVendorID, (PCHAR)&findContext->cProductID, 
						(PCHAR)&findContext->cProductRL, &findContext->dwSizeBuf);

					while (strlen(findContext->cVendorID) && (' ' == findContext->cVendorID[strlen(findContext->cVendorID) - 1]))
					{
						findContext->cVendorID[strlen(findContext->cVendorID) - 1] = 0;
					}

					while (strlen(findContext->cProductID) && (' ' == findContext->cProductID[strlen(findContext->cProductID) - 1]))
					{
						findContext->cProductID[strlen(findContext->cProductID) - 1] = 0;
					}

					while (strlen(findContext->cProductRL) && (' ' == findContext->cProductRL[strlen(findContext->cProductRL) - 1]))
					{
						findContext->cProductRL[strlen(findContext->cProductRL) - 1] = 0;
					}
				}
			}

			__finally
			{
				if (NULL != pvGrabber)
				{
					StarBurn_Destroy(&pvGrabber);
				}
			}
		}
		break;
	}
}


//////////////////////////////////////////////////////////////////////////
// CallBack для граббинга устройства
// 
// cbNumber   - номер CallBack'а
// pvContext  - указатель на GRABCONTEXT
// pvSpecial1 - размер дорожки
// pvSpecial2 - считанно с дорожки
//
VOID __stdcall CallBackGrabbing(IN CALLBACK_NUMBER cbNumber, IN PVOID pvContext, IN PVOID pvSpecial1, IN PVOID pvSpecial2)
{
	GRABCONTEXT*  grabContext = (GRABCONTEXT*)pvContext;
	LARGE_INTEGER liTrackSizeInLBs;
	LARGE_INTEGER liLBsReaden;
	LARGE_INTEGER liCurrentPercent;

	switch (cbNumber)
	{
		case CN_CDVD_READ_PROGRESS:
		{
			liTrackSizeInLBs = *(PLARGE_INTEGER)pvSpecial1;
			liLBsReaden      = *(PLARGE_INTEGER)pvSpecial2;

			liCurrentPercent.QuadPart = liLBsReaden.QuadPart * 100 / liTrackSizeInLBs.QuadPart;

			if (liCurrentPercent.QuadPart == grabContext->liLastReadenPercent.QuadPart)
			{
				// Nothing
			}
			else
			{
				// вычисляем сколько должно быть выведено точек 10% = 7 точек
				DWORD dwNewPoint = (DWORD)liCurrentPercent.QuadPart;
				dwNewPoint = (DWORD)(dwNewPoint / 10.0 * 7.);

				// вычисляем сколько уже было выведено точек
				DWORD dwOldPoint = (DWORD)grabContext->liLastReadenPercent.QuadPart;
				dwOldPoint = (DWORD)(dwOldPoint / 10.0 * 7.); 

				grabContext->liLastReadenPercent.QuadPart = liCurrentPercent.QuadPart;

				for (DWORD i = 0; i < (dwNewPoint - dwOldPoint); i++)
				{
					_tprintf(_T("."));
				}
			}
		}
		break;

		case CN_CDVD_READ_BAD_BLOCK_HIT:
		{
			// Update number of bad blocks hit
			grabContext->liNumberOfBadBlocks.QuadPart += *(PULONG)(pvSpecial2);
		}
		break;

		case CN_CDVD_READ_RETRY:
		{
			// Update number of read retries
			grabContext->liNumberOfReadRetries.QuadPart++;
		}
	}
}