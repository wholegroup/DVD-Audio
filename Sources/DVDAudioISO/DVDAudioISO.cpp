#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include "..\StarBurn\StarBurn.h"
#include "..\StarBurn\StarBurnKey.h"
#include "CDVDAudio.h"
#include "Shlwapi.h"

#define PACKETSIZE 65536 // размер буфера при записи ISO образа

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


VOID PrintHelp()
{
	_tprintf(_T("Run:\n  DVDAudioISO.exe [-o <output.iso>] -l <files list> [-l <files list> ...]\n  DVDAudioISO.exe [-o <output.iso>] -f <files> [-f files ...]\n\n"));
	_tprintf(_T("Example:\n  DVDAudioISO.exe -o dvdaudio.iso -l fileslist1.txt -l fileslist2.txt\n  DVDAudioISO.exe -o dvdaudio.iso -f audio11.wav audio12.wav -f audio21.wav audio22.wav \n\n"));
}


VOID __stdcall StructureCallBack(UINT uiMessageID, PVOID pvContext, PVOID pvSpecial)
{
	switch(uiMessageID)
	{
		// новая группа
		case MID_NEW_GROUP:
			_tprintf(_T("\nCreate GROUP #%i\n"), *(UINT*)pvSpecial);
			break;

		// новый трек
		case MID_NEW_TRACK:
			_tprintf(_T("   %s\n   "), (_TCHAR*)pvSpecial);
			break;

		// обработка трека
		case MID_PROCESS_TRACK:
			SPROCTRACK* sProcTrack;
			sProcTrack = (SPROCTRACK*)pvSpecial;

			UINT32 uiNewPoint; 
			uiNewPoint = (UINT32)(sProcTrack->uiProcBytes / (DOUBLE)sProcTrack->uiNumBytes * 70.0);

			UINT32 uiOldPoint;
			uiOldPoint = (UINT32)(sProcTrack->uiOldBytes / (DOUBLE)sProcTrack->uiNumBytes * 70.0);

			for (UINT i = 0; i < (uiNewPoint - uiOldPoint); i++)
			{
				_tprintf(_T("."));
			}

			if (sProcTrack->uiProcBytes == sProcTrack->uiNumBytes)
			{
				_tprintf(_T(" [OK]\n"));
			}
			fflush( stdin );
			break;

		default:;
	}
}


INT _tmain(INT iArgC, _TCHAR* tcArgV[])
{
	INT               i;                    // рабочая переменная
	UINT              uiGroup;              // номер текущей группы
	UINT              uiTracks;             // количество треков
	CDVDAudio         DVDAudio(44100, 16);  // объект формируемого DVD-A
	_TCHAR            tcNameISO[MAX_PATH];  // название выходного ISO образа
	EXCEPTION_NUMBER  expNum;               // код возврата для функций StarBurn
	UDF_TREE_ITEM     udfDirectory[4];      // структура директорий в ISO образе
	UDF_TREE_ITEM     udfFile[100];         // структура файлов в ISO образе
	UINT              uiGUID;               // ID элементов структуры ISO образа
	UINT              uiCountFiles;         // количество файлов в ISO образе
	intptr_t          iSearch;              // хендл для поиска файлов
	_tfinddata_t      findData;             // структура для поиска файлов
	_TCHAR            tcFilePath[MAX_PATH]; // 
	UDF_CONTROL_BLOCK udfControlBlock;      // 
	LARGE_INTEGER     liSizeISO;            // размер ISO образа
	UCHAR             ucBuffer[PACKETSIZE]; // буффера для записи ISO образа на диск

	expNum       = EN_SUCCESS;
	uiGUID       = 0;
	uiCountFiles = 0;

	RtlZeroMemory(udfDirectory, sizeof(udfDirectory));
	RtlZeroMemory(udfFile, sizeof(udfFile));
	RtlZeroMemory(tcFilePath, sizeof(tcFilePath));
	RtlZeroMemory(&udfControlBlock, sizeof(udfControlBlock));
	_stprintf(tcNameISO, _T("dvdaudio.iso"));

	DVDAudio.SetCallBack(StructureCallBack, NULL);

	// вывод описания к программе
	_tprintf(_T("DVDAudioISO --- Create your DVD-A ISO image v1.0. (c) 2005, Whole Group\n\n"));
	if (3 > iArgC)
	{
		PrintHelp();

		return S_OK;
	}

	uiGroup  = 0;
	uiTracks = 0;

	// цикл по всем параметрам
	for (i = 1; i < iArgC; i++)
	{
		// проверка - параметр или нет 
		if (_T('-') == tcArgV[i][0])
		{
			// -l fileslist.txt
			if (0 == _tcscmp(tcArgV[i], _T("-l")))
			{
				if (uiTracks || !uiGroup)
				{
					uiGroup  = uiGroup + 1;
					uiTracks = 0;
				}

				if (((i + 1) == iArgC) || (_T('-') == tcArgV[i + 1][0]))
				{
					_tprintf(_T("ERROR::Invalid parameter::%s\n\n"), tcArgV[i]);
					PrintHelp();

					return S_FALSE;
				}

				i++;

				// обработка списка файлов
				FILE*  fList = NULL;
				_TCHAR tcFileName[MAX_PATH];
            
				fList = _tfopen(tcArgV[i], _T("rt"));
				if (NULL == fList)
				{
					_tprintf(_T("ERROR::The file '%s' was not opened\n\n"), tcArgV[i]);
					PrintHelp();

					return S_FALSE;
				}

				// чтение листа и ввод треков
				while (NULL != _fgetts(tcFileName, MAX_PATH, fList))
				{
					UINT j = (UINT)_tcslen(tcFileName);
					if (10 == tcFileName[j - 1])
					{
						tcFileName[j - 1] = 0;
					}

					if (S_OK == DVDAudio.AddTrack(uiGroup, tcFileName))
					{
						uiTracks++;
					}
				}

				fclose(fList);
			}
			else

			// -f file1.wav file2.wav ...
			if (0 == _tcscmp(tcArgV[i], _T("-f")))
			{
				if (uiTracks || !uiGroup)
				{
					uiGroup  = uiGroup + 1;
					uiTracks = 0;
				}

				while (((i + 1) < iArgC) && (_T('-') != tcArgV[i + 1][0]))
				{
					i++;
					if (S_OK == DVDAudio.AddTrack(uiGroup, tcArgV[i]))
					{
						uiTracks++;
					}
				}
			}
			else

			// -o dvdaudio.iso
			if (0 == _tcscmp(tcArgV[i], _T("-o")))
			{
				if (((i + 1) == iArgC) || (_T('-') == tcArgV[i + 1][0]))
				{
					_tprintf(_T("ERROR::Invalid parameter::%s\n\n"), tcArgV[i]);
					PrintHelp();

					return S_FALSE;
				}

				i++;

				_stprintf(tcNameISO, _T("%s"), tcArgV[i]);
			}

			else
			{
				_tprintf(_T("ERROR::Invalid parameter::%s\n\n"), tcArgV[i]);
				PrintHelp();

				return S_FALSE;
			}
		}
		else
		{
			if (0 == uiGroup)
			{
				_tprintf(_T("ERROR::Not enough parameters\n\n"));
				PrintHelp();

				return S_FALSE;
			}
		}
	}

	if (!uiTracks)
	{
		uiGroup--;
	}

	if (!uiGroup)
	{
		_tprintf(_T("ERROR::Choose tracks\n\n"));
		PrintHelp();

		return S_FALSE;
	}

	// формируем структуру
	_tprintf(_T("Create structure DVD-A: \n"));
	CreateDirectory(_T("TEMPDVD"), NULL);
	if (S_OK != DVDAudio.CreateDVDAudio(_T("TEMPDVD")))
	{
		_tprintf(_T("ERROR::CreateDVDAudio\n\n"));

		return S_FALSE;
	}

///*
	// собираем ISO образ
	_tprintf(_T("\nSave ISO image (%s): \n   "), tcNameISO);

	expNum = StarBurn_UpStartEx(&g__UCHAR__RegistrationKey, sizeof(g__UCHAR__RegistrationKey));
	if (EN_SUCCESS != expNum)
	{
		_tprintf(_T("ERROR::StarBurn_UpStartEx\n\n"));

		return S_FALSE;
	}

	StarBurn_UDF_FormatTreeItemAsDirectory(&udfDirectory[1], ++uiGUID, "", NULL);
	StarBurn_UDF_FormatTreeItemAsDirectory(&udfDirectory[2], ++uiGUID, "VIDEO_TS", &udfDirectory[1]);
	StarBurn_UDF_FormatTreeItemAsDirectory(&udfDirectory[3], ++uiGUID, "AUDIO_TS", &udfDirectory[1]);

	_stprintf(tcFilePath, _T("TEMPDVD\\AUDIO_TS\\AUDIO_PP.IFO"));
	if (TRUE == PathFileExists(tcFilePath))
	{
		StarBurn_UDF_FormatTreeItemAsFile(&udfFile[++uiCountFiles], ++uiGUID, "AUDIO_PP.IFO", tcFilePath, &udfDirectory[3]);
	}

	_stprintf(tcFilePath, _T("TEMPDVD\\AUDIO_TS\\AUDIO_TS.IFO"));
	if (TRUE == PathFileExists(tcFilePath))
	{
		StarBurn_UDF_FormatTreeItemAsFile(&udfFile[++uiCountFiles], ++uiGUID, "AUDIO_TS.IFO", tcFilePath, &udfDirectory[3]);
	}

	_stprintf(tcFilePath, _T("TEMPDVD\\AUDIO_TS\\AUDIO_TS.BUP"));
	if (TRUE == PathFileExists(tcFilePath))
	{
		StarBurn_UDF_FormatTreeItemAsFile(&udfFile[++uiCountFiles], ++uiGUID, "AUDIO_TS.BUP", tcFilePath, &udfDirectory[3]);
	}

	for (i = 1; i < 10; i++)
	{
		_TCHAR tcName[MAX_PATH];

		_stprintf(tcFilePath, _T("TEMPDVD\\AUDIO_TS\\ATS_0%i_0.IFO"), i);
		if (TRUE == PathFileExists(tcFilePath))
		{
			_stprintf(tcName, _T("ATS_0%i_0.IFO"), i);
			StarBurn_UDF_FormatTreeItemAsFile(&udfFile[++uiCountFiles], ++uiGUID, tcName, tcFilePath, &udfDirectory[3]);
		}

		for (INT j = 1; j < 10; j++)
		{
			_stprintf(tcFilePath, _T("TEMPDVD\\AUDIO_TS\\ATS_0%i_%i.AOB"), i, j);
			_tprintf(tcFilePath);
			if (TRUE == PathFileExists(tcFilePath))
			{
				_tprintf(_T("OK"));
				_stprintf(tcName, _T("ATS_0%i_%i.AOB"), i, j);
				StarBurn_UDF_FormatTreeItemAsFile(&udfFile[++uiCountFiles], ++uiGUID, tcName, tcFilePath, &udfDirectory[3]);
			}
			_tprintf(_T("\n"));
		}

		_stprintf(tcFilePath, _T("TEMPDVD\\AUDIO_TS\\ATS_0%i_0.BUP"), i);
		if (TRUE == PathFileExists(tcFilePath))
		{
			_stprintf(tcName, _T("ATS_0%i_0.BUP"), i);
			StarBurn_UDF_FormatTreeItemAsFile(&udfFile[++uiCountFiles], ++uiGUID, tcName, tcFilePath, &udfDirectory[3]);
		}
	}

	expNum = StarBurn_UDF_Create(&udfDirectory[1], &udfDirectory[2], &udfDirectory[3], &udfControlBlock, NULL, 0, NULL, "WHOLEGROUP.COM");

	liSizeISO.LowPart = StarBurn_ISO9660JolietFileTree_GetSizeInUCHARs(udfControlBlock.m__PVOID__Body, &liSizeISO.HighPart);

	// запись ISO в файл tcNameISO
	UINT uiTransfer;
	FILE* fISO;

	fISO = _tfopen(tcNameISO, _T("wb"));
	if (NULL == fISO)
	{
		_tprintf(_T("ERROR::_tfopen::%s\n\n"), tcNameISO);
		StarBurn_DownShut();

		return S_FALSE;
	}

	LARGE_INTEGER liTotalSizeISO = liSizeISO;
	while (liSizeISO.QuadPart)
	{
		if (liSizeISO.QuadPart < PACKETSIZE)
		{
			uiTransfer = (UINT)liSizeISO.QuadPart;
		}
		else
		{
			uiTransfer = PACKETSIZE;
		}

		StarBurn_ISO9660JolietFileTree_Read(udfControlBlock.m__PVOID__Body, NULL, 0, NULL, uiTransfer, ucBuffer);

		if (fwrite(ucBuffer, uiTransfer, 1, fISO) != 1)
		{
			_tprintf(_T("ERROR::fwrite::dvdaudio.iso\n\n"));
			fclose(fISO);
			StarBurn_DownShut();

			return S_FALSE;
		}

		// вычисляем сколько должно быть выведено точек 10% = 7 точек
		LARGE_INTEGER liNewPoint; 
		liNewPoint.QuadPart = liTotalSizeISO.QuadPart - liSizeISO.QuadPart + uiTransfer;
		liNewPoint.QuadPart = (LONGLONG)(liNewPoint.QuadPart / (DOUBLE)liTotalSizeISO.QuadPart * 70.0);

		LARGE_INTEGER liOldPoint; 
		liOldPoint.QuadPart = liTotalSizeISO.QuadPart - liSizeISO.QuadPart;
		liOldPoint.QuadPart = (LONGLONG)(liOldPoint.QuadPart / (DOUBLE)liTotalSizeISO.QuadPart * 70.0);

		for (i = 0; i < (liNewPoint.QuadPart - liOldPoint.QuadPart); i++)
		{
			_tprintf(_T("."));
		}

		liSizeISO.QuadPart -= uiTransfer;
	}

	fclose(fISO);

	StarBurn_UDF_CleanUp(&udfFile[0], 100);
	StarBurn_UDF_CleanUp(&udfDirectory[0], 4);
	StarBurn_UDF_Destroy(&udfDirectory[1], &udfControlBlock);
	StarBurn_DownShut();

	_tprintf(_T(" [OK]\n"));

//	*/

	/*
	PVOID pvFileTree;
	ULONG ulStatus;
	ULONG ulTreeNodes;

	expNum = StarBurn_ISO9660JolietFileTree_Create(&pvFileTree, NULL, 0, &ulStatus, NULL, 
		&ulTreeNodes, TRUE, FALSE, FALSE, FILE_TREE_ISO9660);
	*/

	// удаление временных файлов
	iSearch = _tfindfirst(_T("TEMPDVD\\AUDIO_TS\\A*.*"), &findData);
	if (-1 == iSearch)
	{
		_tprintf(_T("ERROR::_tfindfirst\n\n"));

		return S_FALSE;
	}

	do
	{
		_stprintf(tcFilePath, _T("TEMPDVD\\AUDIO_TS\\%s"), findData.name);
		DeleteFile(tcFilePath);
	} while (!_tfindnext(iSearch, &findData));

	_findclose(iSearch);

	RemoveDirectory("TEMPDVD\\AUDIO_TS");
	RemoveDirectory("TEMPDVD\\VIDEO_TS");
	RemoveDirectory("TEMPDVD");


	return S_OK;
}
