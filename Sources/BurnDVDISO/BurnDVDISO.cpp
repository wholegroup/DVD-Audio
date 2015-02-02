#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "..\StarBurn\StarBurn.h"
#include "..\StarBurn\StarBurnKey.h"
#include "CallBack.h"
#include "BurnDVDISO.h"

// тестовый режим записи
// #define TEST_MODE

// включить проверку записанного
#define VERIFY_MODE

INT _tmain(INT iArgC, _TCHAR* tcArgV[])
{
	EXCEPTION_NUMBER        expNum = EN_SUCCESS;
	FINDCONTEXT             findContext;
	PVOID                   pvGrabber;
	ULONG                   ulStatus;
	CDB_FAILURE_INFORMATION cdbInf;
	LARGE_INTEGER           liLastPercent;

	_tprintf(_T("BurnDVDISO --- Test utility v1.0. (c) 2005, Whole Group\n\n"));

	// инициализация ядра StarBurn
	expNum = StarBurn_UpStartEx(&g__UCHAR__RegistrationKey, sizeof(g__UCHAR__RegistrationKey));
	if (EN_SUCCESS != expNum)
	{
		_tprintf(_T("ERROR::StarBurn_UpStartEx\n"));

		return S_FALSE;
	}

	// вывод существующих приводов для записи DVD, если параметры не указаны
	if (iArgC < 3)
	{
		DWORD dwTmp         = 0;
		DWORD dwCountDevice = 0;

		_tprintf(_T("Run:\n  BurnDVDISO.exe <drive number>|<drive letter> <filename.iso> [<write speed>]\n\n"));
		_tprintf(_T("Example:\n  BurnDVDISO.exe 2 dvd.iso\n  BurnDVDISO.exe F: dvd.iso 4\n\n"));
		_tprintf(_T("Select drive:\n"));

		dwCountDevice = StarBurn_FindDevice(SCSI_DEVICE_RO_DIRECT_ACCESS, FALSE, (PCALLBACK)(CallBackPrint), (PVOID)&dwTmp);
		if (!dwCountDevice)
		{
			_tprintf(_T("Drives ain't found\n"));

			StarBurn_DownShut();
			return S_FALSE;
		}

		StarBurn_DownShut();
		return S_OK;
	}

	//	проверка существования ISO
	FILE* fISO;
	fISO = _tfopen(tcArgV[2], _T("rb"));
	if (NULL ==  fISO)
	{
		_tprintf(_T("ERROR::_tfopen::%s\n"), tcArgV[2]);

		StarBurn_DownShut();
		return S_FALSE;
	}
	fclose(fISO);

	// поиск указанного привода
	RtlZeroMemory(&findContext, sizeof(findContext));

	findContext.dwNumDevice = _tstol(tcArgV[1]);
	if (!findContext.dwNumDevice)
	{
		_tcsupr(tcArgV[1]);
		RtlCopyMemory(&findContext.cLetter, "\\\\.\\", 4);
		sprintf(&findContext.cLetter[4], "%.2s", tcArgV[1]);

		findContext.bLetter = TRUE;
	}

	StarBurn_FindDevice(SCSI_DEVICE_RO_DIRECT_ACCESS, FALSE, (PCALLBACK)(CallBackFind), (PVOID)&findContext);
	if (!findContext.bFind)
	{
		_tprintf(_T("Selected drive %s is not found\n"), tcArgV[1]);

		StarBurn_DownShut();
		return S_FALSE;
	}

	// устройство найдено
	_tprintf(_T("Drive: [%ld:%ld:%ld:%ld] %s %s rev.%s (cache: %ld)\n\n"), 
		findContext.dwPortID, findContext.dwBusID, findContext.dwTargetID, findContext.dwLUN,
		findContext.cVendorID, findContext.cProductID, findContext.cProductRL, findContext.dwSizeBuf);

	// открытие найденного устройства
	expNum = StarBurn_CdvdBurnerGrabber_Create(&pvGrabber, NULL, 0, &ulStatus, &cdbInf, 
		(PCALLBACK)(CallBackBurn), &liLastPercent, 
		(UCHAR)findContext.dwPortID, 
		(UCHAR)findContext.dwBusID, 
		(UCHAR)findContext.dwTargetID, 
		(UCHAR)findContext.dwLUN, 1);

	// ожидание диска в приводе
	BOOLEAN bIsMediaPresent, bIsDoorOrTrayOpened;
	_tprintf(_T("Wait media present and door/tray closed... "));
	do {
		expNum = StarBurn_CdvdBurnerGrabber_GetMediaTrayStatus(pvGrabber, NULL, 0, &ulStatus, &cdbInf, 
			&bIsMediaPresent, &bIsDoorOrTrayOpened);
		if (EN_SUCCESS != expNum)
		{
			_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_GetMediaTrayStatus\n"));

			StarBurn_Destroy(&pvGrabber);
			StarBurn_DownShut();
			return S_FALSE;
		}

		if (!bIsMediaPresent || bIsDoorOrTrayOpened)
		{
			Sleep(1000);
		}
		else
		{
			break;
		}
	} while(TRUE);
	_tprintf(_T("OK\n"));

	// Try to test unit ready
	expNum = StarBurn_CdvdBurnerGrabber_TestUnitReady(pvGrabber, NULL, 0, &ulStatus, &cdbInf);
	if (EN_SUCCESS != expNum)
	{
		_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_TestUnitReady\n"));

		StarBurn_Destroy(&pvGrabber);
		StarBurn_DownShut();
		return S_FALSE;
	}

	// Получение информации о BUP (Buffer Underrun Protection)
	BOOLEAN bIsBUPEnabled, bIsBUPSupported;
	expNum = StarBurn_CdvdBurnerGrabber_GetBUP(pvGrabber, NULL, 0, &ulStatus, &cdbInf, 
		&bIsBUPEnabled, &bIsBUPSupported);
	if (EN_SUCCESS != expNum)
	{
		_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_GetBUP\n"));

		StarBurn_Destroy(&pvGrabber);
		StarBurn_DownShut();
		return S_FALSE;
	}

	// попытка включения BUP
	if (TRUE /*== bIsBUPSupported*/)
	{
		expNum = StarBurn_CdvdBurnerGrabber_SetBUP(pvGrabber, NULL, 0, &ulStatus, &cdbInf, TRUE);
		if (EN_SUCCESS != expNum)
		{
			_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_GetBUP\n"));

			StarBurn_Destroy(&pvGrabber);
			StarBurn_DownShut();
			return S_FALSE;
		}

	}

	// установка скорости 
	UINT uiSpeed =	CDVD_SPEED_IS_KBPS_MAXIMUM;
	if (iArgC > 3)
	{
		uiSpeed = _tstol(tcArgV[3]);
		if (uiSpeed)
		{
			uiSpeed = uiSpeed * DVD_SPEED_IN_KBPS_1X;
		}
		else
		{
			uiSpeed = CDVD_SPEED_IS_KBPS_MAXIMUM;
		}
	}

	expNum = StarBurn_CdvdBurnerGrabber_SetSpeeds(pvGrabber, NULL, 0, &ulStatus, &cdbInf, 
		CDVD_SPEED_IS_KBPS_MAXIMUM, uiSpeed);
	if (EN_SUCCESS != expNum)
	{
		_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_SetSpeeds\n"));

		StarBurn_Destroy(&pvGrabber);
		StarBurn_DownShut();
		return S_FALSE;
	}

	// получение информации о диске
	DISC_INFORMATION discInfo;
	expNum = StarBurn_CdvdBurnerGrabber_GetDiscInformation(pvGrabber, NULL, 0, &ulStatus, &cdbInf, &discInfo);
	if (EN_SUCCESS != expNum)
	{
		_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_GetDiscInformation\n"));

		StarBurn_Destroy(&pvGrabber);
		StarBurn_DownShut();
		return S_FALSE;
	}

	DISC_TYPE discType;
	discType = StarBurn_CdvdBurnerGrabber_GetInsertedDiscType(pvGrabber);
	if (DISC_TYPE_DVDPLUSR != discType && 
		DISC_TYPE_DVDPLUSR_DL != discType &&
		DISC_TYPE_DVDPLUSRW   != discType && 
		DISC_TYPE_DVDR        != discType &&
		DISC_TYPE_DVDRAM      != discType &&
		DISC_TYPE_DVDRWRO     != discType &&
		DISC_TYPE_DVDRWSR     != discType)
	{
		_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_GetInsertedDiscType::Media not writable\n"));

		StarBurn_Destroy(&pvGrabber);
		StarBurn_DownShut();
		return S_FALSE;
	}

	// получение информации о содержимом диска
	TRACK_INFORMATION trackInfo;
	expNum = StarBurn_CdvdBurnerGrabber_GetTrackInformation(pvGrabber, NULL, 0, &ulStatus, &cdbInf, 
		TRACK_NUMBER_INVISIBLE, (PTRACK_INFORMATION)(&trackInfo));
	if (EN_SUCCESS != expNum)
	{
		_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_GetTrackInformation\n"));

		StarBurn_Destroy(&pvGrabber);
		StarBurn_DownShut();
		return S_FALSE;
	}

	_tprintf(_T("Disc is blank... "));
	if (trackInfo.m__LONG__NextWritableAddress > 0)
	{
		_tprintf(_T("NO\n"));

		// если не пустой и перезаписываемый, то стираем
		if (!discInfo.m__BOOLEAN__IsErasable)
		{
			_tprintf(_T("ERROR::Media is not erasable\n"));

			StarBurn_Destroy(&pvGrabber);
			StarBurn_DownShut();
			return S_FALSE;
		}

		_tprintf(_T("Erase media, please wait... "));
		expNum = StarBurn_CdvdBurnerGrabber_Blank(pvGrabber, NULL, 0, &ulStatus, &cdbInf, ERASE_TYPE_BLANK_DISC_FAST);
		if (EN_SUCCESS != expNum)
		{
			_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_Blank\n"));

			StarBurn_Destroy(&pvGrabber);
			StarBurn_DownShut();
			return S_FALSE;
		}
		_tprintf(_T("OK\n"));
	}
	else
	{
		_tprintf(_T("YES\n"));
	}

	// посылаем OPC (Optimum Power Calibration)
#ifndef TEST_MODE
	_tprintf(_T("Send OPC... "));
	expNum = StarBurn_CdvdBurnerGrabber_SendOPC(pvGrabber, NULL, 0, &ulStatus, &cdbInf);
	if (EN_SUCCESS != expNum)
	{
		_tprintf(_T("\nERROR::StarBurn_CdvdBurnerGrabber_Blank\n"));

		StarBurn_Destroy(&pvGrabber);
		StarBurn_DownShut();
		return S_FALSE;
	}
	_tprintf(_T("OK\n"));
#endif

	// пишем
	_tprintf(_T("Burn:\n   "));

	liLastPercent.QuadPart = 0;

#ifdef TEST_MODE
	expNum = StarBurn_CdvdBurnerGrabber_TrackAtOnceFromFile(pvGrabber, NULL, 0, &ulStatus, &cdbInf,
		tcArgV[2], FALSE, TRUE, FALSE, WRITE_REPORT_DELAY_IN_SECONDS, BUFFER_STATUS_REPORT_DELAY_IN_SECONDS);
#else
	expNum = StarBurn_CdvdBurnerGrabber_TrackAtOnceFromFile(pvGrabber, NULL, 0, &ulStatus, &cdbInf,
		tcArgV[2], FALSE, FALSE, FALSE, WRITE_REPORT_DELAY_IN_SECONDS, BUFFER_STATUS_REPORT_DELAY_IN_SECONDS);
#endif

	if (EN_SUCCESS != expNum)
	{
		_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_TrackAtOnceFromFile\n"));

		StarBurn_Destroy(&pvGrabber);
		StarBurn_DownShut();
		return S_FALSE;
	}
	_tprintf(_T(" OK\n"));

	// закрываем сессию
#ifndef TEST_MODE
	expNum = StarBurn_CdvdBurnerGrabber_CloseSession(pvGrabber, NULL, 0, &ulStatus, &cdbInf);
	if (EN_SUCCESS != expNum)
	{
		_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_CloseSession\n"));

		StarBurn_Destroy(&pvGrabber);
		StarBurn_DownShut();
		return S_FALSE;
	}
#endif

	// проверяем запись
#ifndef TEST_MODE
#ifdef VERIFY_MODE
	_tprintf(_T("Verify:\n   "));
	liLastPercent.QuadPart = 0;
	LONG lFailedLBA;
	expNum = StarBurn_CdvdBurnerGrabber_VerifyFile(pvGrabber, NULL, 0, &ulStatus, &cdbInf,
		tcArgV[2], 0, &lFailedLBA, 1000);
	if (EN_SUCCESS != expNum)
	{
		_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_VerifyFile\n"));

		StarBurn_Destroy(&pvGrabber);
		StarBurn_DownShut();
		return S_FALSE;
	}

	// добиваем точками до 100
	DWORD dwOldPoint = (DWORD)liLastPercent.QuadPart;
	dwOldPoint = (DWORD)(dwOldPoint / 100.0 * 70.); 

	for (DWORD i = 0; i < (70 - dwOldPoint); i++)
	{
		_tprintf(_T("."));
	}

	_tprintf(_T(" OK\n"));
#endif
#endif

	// извлекаем диск
	expNum = StarBurn_CdvdBurnerGrabber_Eject(pvGrabber, NULL, 0, &ulStatus, &cdbInf);
	if (EN_SUCCESS != expNum)
	{
		_tprintf(_T("ERROR::StarBurn_CdvdBurnerGrabber_Eject\n"));

		StarBurn_Destroy(&pvGrabber);
		StarBurn_DownShut();
		return S_FALSE;
	}

	StarBurn_Destroy(&pvGrabber);
	StarBurn_DownShut();

	return S_FALSE;
}

