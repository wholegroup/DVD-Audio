#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "GrabAudioCD.h"
#include "..\StarBurn\StarBurn.h"
#include "..\StarBurn\StarBurnKey.h"
#include "CallBack.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

INT _tmain(INT iArgC, TCHAR* argv[], TCHAR* envp[])
{
	EXCEPTION_NUMBER        expNum = EN_SUCCESS; //
	DWORD                   dwCountDevice = 0;   // число устройств
	FINDCONTEXT             findContext;         // информация о выбранном устройстве
	PVOID                   pvGrabber = NULL;    // указатель на открытое устройство
	ULONG                   ulStatus;            //
	CDB_FAILURE_INFORMATION cdbInf;              // 
	DISC_TYPE               diskType;            // тип вставленного в устройство диска
	TOC_INFORMATION         tocInf;              // информация о дорожках на диске
	CHAR                    cFileName[MAX_PATH]; // имя файла
	GRABCONTEXT             grabContext;         // 
	INT                     iRetCode = 0;        // код завершения программы

	_tprintf(_T("GrabAudioCD --- Test utility v1.0. (c) 2005, Whole Group\n\n"));

	__try
	{
		// инициализация ядра StarBurn
		expNum = StarBurn_UpStartEx(&g__UCHAR__RegistrationKey, sizeof(g__UCHAR__RegistrationKey));
		if (EN_SUCCESS != expNum)
		{
			_tprintf(_T("ERROR::StarBurn_UpStartEx\n"));
			iRetCode = -1;

			__leave;
		}

		// вывод существующих приводов
		if (iArgC < 2)
		{
			DWORD dwTmp = 0;

			_tprintf(_T("Run:\n  GrabAudioCD.exe <number drive>|<letter drive> [<track number>]\n\n"));
			_tprintf(_T("Example:\n  GrabAudioCD.exe 2\n  GrabAudioCD.exe F: 2\n\n"));
			_tprintf(_T("Select drive:\n"));

			dwCountDevice = StarBurn_FindDevice(SCSI_DEVICE_RO_DIRECT_ACCESS, FALSE, (PCALLBACK)(CallBackPrint), (PVOID)&dwTmp);
			if (!dwCountDevice)
			{
				_tprintf(_T("Drives ain't found\n"));
				iRetCode = -1;
			}

			__leave;
		}

		// поиск указанного привода
		RtlZeroMemory(&findContext, sizeof(findContext));

		findContext.dwNumDevice = _tstol(argv[1]);
		if (!findContext.dwNumDevice)
		{
			_tcsupr(argv[1]);
			RtlCopyMemory(&findContext.cLetter, "\\\\.\\", 4);
			sprintf(&findContext.cLetter[4], "%.2s", argv[1]);

			findContext.bLetter = TRUE;
		}

		StarBurn_FindDevice(SCSI_DEVICE_RO_DIRECT_ACCESS, FALSE, (PCALLBACK)(CallBackFind), (PVOID)&findContext);
		if (!findContext.bFind)
		{
			_tprintf(_T("Select device not find\n"));
			iRetCode = -1;

			__leave;
		}

		// устройство найдено
		_tprintf(_T("Drive: [%ld:%ld:%ld:%ld] %s %s rev.%s (cache: %ld)\n\n"), 
			findContext.dwPortID, findContext.dwBusID, findContext.dwTargetID, findContext.dwLUN,
			findContext.cVendorID, findContext.cProductID, findContext.cProductRL, findContext.dwSizeBuf);

		__try
		{
			// открытие найденного устройства
			expNum = StarBurn_CdvdBurnerGrabber_Create(&pvGrabber, 
				NULL, 0, &ulStatus, &cdbInf, (PCALLBACK)(CallBackGrabbing), &grabContext, 
				(UCHAR)findContext.dwPortID, 
				(UCHAR)findContext.dwBusID,
				(UCHAR)findContext.dwTargetID,
				(UCHAR)findContext.dwLUN, 1);

			if (EN_SUCCESS != expNum)
			{
				_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_Create\n"));

				__leave;
			}

			// проверка на готовность
			expNum = StarBurn_CdvdBurnerGrabber_TestUnitReady(pvGrabber, NULL, 0, &ulStatus, &cdbInf);

			if (EN_SUCCESS != expNum)
			{
				_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_TestUnitReady\n"));

				__leave;
			}

			// установка максимальной скорости
			expNum = StarBurn_CdvdBurnerGrabber_SetSpeeds(pvGrabber, NULL, 0, &ulStatus, &cdbInf, 
				CDVD_SPEED_IS_KBPS_MAXIMUM, CDVD_SPEED_IS_KBPS_MAXIMUM);

			if (EN_SUCCESS != expNum)
			{
				_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_SetSpeeds\n"));

				__leave;
			}

			// получение информации о диске
			diskType = StarBurn_CdvdBurnerGrabber_GetInsertedDiscType(pvGrabber);

			if (DISC_TYPE_NO_MEDIA == diskType)
			{
				_tprintf(_T("No media is inserted to the disc drive\n"));

				__leave;
			}

			// вывод информации о дорожках на диске
			expNum = StarBurn_CdvdBurnerGrabber_GetTOCInformation(pvGrabber, NULL, 0, &ulStatus, &cdbInf, &tocInf);

			if (EN_SUCCESS != expNum)
			{
				_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_GetTOCInformation\n"));

				__leave;
			}

			for (DWORD i = 0; i < tocInf.m__UCHAR__NumberOfTracks; i++)
			{
				// проверка Audio дорожки
				if (!tocInf.m__TOC_ENTRY[i].m__BOOLEAN__IsAudio)
				{
					_tprintf(_T("%02ld. NO AUDIO TRACK\n"), i + 1);

					continue;
				}

				DWORD dwStart, dwStop, dwMin, dwSec, dwMSec;

				dwStart = tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[0] * 60 * 1000;
				dwStart += tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[1] * 1000;
				dwStart += tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[2];

				dwStop = tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[0] * 60 * 1000;
				dwStop += tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[1] * 1000;
				dwStop += tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[2];

				// вычисление длительности дорожки
				dwMin  = (dwStop - dwStart) / (60 * 1000);
				dwSec  = ((dwStop - dwStart) - dwMin * 60 * 1000) / 1000;
				dwMSec = ((dwStop - dwStart) - dwMin * 60 * 1000) - dwSec * 1000;

				_tprintf(_T("%02ld. Start - %02ld:%02ld:%03ld | Stop - %02ld:%02ld:%03ld | Duration - %02ld:%02ld:%03ld\n"), 
					i + 1, 
					tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[0], 
					tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[1], 
					tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[2],
					tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[0], 
					tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[1], 
					tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[2],
					dwMin, dwSec, dwMSec);

				// грабим дорожку на диск
				DWORD dwTmp = 0;
				
				if (iArgC > 2)
				{
					dwTmp = _tstol(argv[2]);
				}

				if (!dwTmp || (dwTmp && ((i + 1) == dwTmp)))
				{
					RtlZeroMemory(&grabContext, sizeof(grabContext));
					RtlZeroMemory(&cFileName, sizeof(cFileName));
					sprintf(cFileName, "%s%02ld.wav", "Audio", i + 1);

					_tprintf(_T("    "));

					expNum = StarBurn_CdvdBurnerGrabber_GrabTrack(pvGrabber, NULL, 0, &ulStatus, &cdbInf, 
						(UCHAR)(i + 1), cFileName, NUMBER_OF_READ_RETRIES, TRUE, FALSE, READ_REPORT_DELAY_IN_SECONDS);

					if (EN_SUCCESS != expNum)
					{
						_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_GrabTrack\n"));

						__leave;
					}

					DWORD dwOldPoint = (DWORD)grabContext.liLastReadenPercent.QuadPart;
					dwOldPoint = (DWORD)(dwOldPoint / 10.0 * 7.); 

					for (DWORD i = 0; i < (70 - dwOldPoint); i++)
					{
						_tprintf(_T("."));
					}
					_tprintf(_T(" [OK]\n"));
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

	__finally
	{
		StarBurn_DownShut();
	}

	return iRetCode;
}
