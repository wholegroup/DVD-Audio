/*
 * Copyright (C) 2015 Andrey Rychkov <wholegroup@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"
#include "resource.h"
#include "MainFrm.h"
#include "BurnDVDA.h"
#include <io.h>

IMPLEMENT_DYNAMIC(CBurnDVDA, CDialog)

BEGIN_MESSAGE_MAP(CBurnDVDA, CDialog)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BUTTONCLOSE, OnClose)
END_MESSAGE_MAP()

//
#define COLUMN_01  25
#define COLUMN_02  70
#define COLUMN_03 100

//
#define MAX_PROGRESS_STATUS 1000

//////////////////////////////////////////////////////////////////////////
// конструктор и деструктор
//
CBurnDVDA::CBurnDVDA(CWnd* pParent /*=NULL*/)
	: CDialog(CBurnDVDA::IDD, pParent)
{
	m_pImageList = NULL;
	m_hThread    = NULL;
	m_dwThreadID = 0;
	m_bQuit      = FALSE;
}

//
CBurnDVDA::~CBurnDVDA()
{
	if (NULL != m_pImageList)
	{
		m_pImageList->DeleteImageList();

		delete m_pImageList;
	}
}

//
void CBurnDVDA::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

//
BOOL CBurnDVDA::OnInitDialog()
{
	CDialog::OnInitDialog();

	CListCtrl* listCompilation = (CListCtrl*)GetDlgItem(IDC_LISTCOMPILATION);

	// добавление стиля списку сообщений
	DWORD dwExStyle = listCompilation->GetExtendedStyle();
	dwExStyle |= LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES ;
	listCompilation->SetExtendedStyle(dwExStyle);

	// создание колонок в списке треков
	listCompilation->InsertColumn(0, _T(""),      LVCFMT_CENTER, COLUMN_01);
	listCompilation->InsertColumn(1, _T("Time"),  LVCFMT_CENTER, COLUMN_02);
	listCompilation->InsertColumn(2, _T("Event"), LVCFMT_LEFT,   COLUMN_03);

	listCompilation->DeleteAllItems();

	// создание списка картинок
	HICON iconTmp;

	m_pImageList = new CImageList();
	m_pImageList->Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 3);

	iconTmp = AfxGetApp()->LoadIcon(IDI_ICONCD);
	m_pImageList->Add(iconTmp);	

	listCompilation->SetImageList(m_pImageList, LVSIL_SMALL);

	// инициализация прогрессбара
	CProgressCtrl* progressStatus = (CProgressCtrl*)GetDlgItem(IDC_PROGRESSTATUS);
	progressStatus->SetRange(0, MAX_PROGRESS_STATUS);
	progressStatus->SetPos(0);

	// создание треда для прожига
	m_burnThreadParam.dlgBurn   = this;
	m_burnThreadParam.mainFrame = GetParentFrame();
	m_hThread = CreateThread(NULL, 0, BurnThread, &m_burnThreadParam, 0, &m_dwThreadID);
	if (INVALID_HANDLE_VALUE == m_hThread)
	{
		MessageBox(_T("ERROR::CreateThread"));
		m_bQuit = TRUE;
		PostMessage(WM_QUIT);

		return TRUE;
	}

	return TRUE;
}

//
void CBurnDVDA::OnPaint()
{
	CPaintDC dc(this);

	// вывод разграничивающей полоски - диалог с тулбаром
	CPen  penTmp;
	CRect rectTmp;

	((CMainFrame*)GetParentFrame())->GetClientRect(&rectTmp);

	penTmp.DeleteObject();
	penTmp.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
	dc.SelectObject(&penTmp);

	dc.MoveTo(0, 2);
	dc.LineTo(rectTmp.Width(), 2);

	penTmp.DeleteObject();
	penTmp.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
	dc.SelectObject(&penTmp);

	dc.MoveTo(0, 3);
	dc.LineTo(rectTmp.Width(), 3);
}

//
VOID CBurnDVDA::SetSize()
{
	CWnd  *wndTmp;
	CRect rectTmp;
	CRect rectOld;
	CRect rectNew = ((CMainFrame*)GetParentFrame())->GetClientSize();

	// получение текущих координат
	GetWindowRect(&rectOld);
	GetParentFrame()->ScreenToClient(&rectOld);

	// перемещение группбокса: compilation
	wndTmp = GetDlgItem(IDC_FRAMECOMPILATION);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left,
		rectTmp.top, 
		rectNew.Width() - rectOld.Width() + rectTmp.Width(),
		rectNew.Height() - rectOld.Height() + rectTmp.Height(),
		0);

	// перемещение листбокса: compilation
	wndTmp = GetDlgItem(IDC_LISTCOMPILATION);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left,
		rectTmp.top, 
		rectNew.Width() - rectOld.Width() + rectTmp.Width(),
		rectNew.Height() - rectOld.Height() + rectTmp.Height(),
		0);

	// установка размеров колонок
	CListCtrl* listCompilation = (CListCtrl*)GetDlgItem(IDC_LISTCOMPILATION);
	listCompilation->SetColumnWidth(0, COLUMN_01);
	listCompilation->SetColumnWidth(1, COLUMN_02);
	listCompilation->SetColumnWidth(2, rectNew.Width() - COLUMN_01 - COLUMN_02);

	// перемещение группбокса: status
	wndTmp = GetDlgItem(IDC_FRAMESTATUS);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left,
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectNew.Width() - rectOld.Width() + rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение группбокса: status
	wndTmp = GetDlgItem(IDC_PROGRESSTATUS);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left,
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectNew.Width() - rectOld.Width() + rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение кнопки: close
	wndTmp = GetDlgItem(IDC_BUTTONCLOSE);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.right - rectTmp.Width() - (rectOld.right - rectTmp.right),
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// установка новых размеров окна
	SetWindowPos(&wndBottom, rectNew.left, rectNew.top, 
		rectNew.right - rectNew.left, rectNew.bottom - rectNew.top, 0);

	Invalidate();
}

//
void CBurnDVDA::OnClose()
{
	if (m_bQuit)
	{
		WaitForSingleObject(m_hThread, 20000);
		TerminateThread (m_hThread, 1);
		CloseHandle(m_hThread);
		m_hThread = NULL;

		// прибиваем MessageBox
		CWnd* dlgMB = FindWindow(NULL, _T(PROGRAM_NAME));
		if (dlgMB)
		{
			((CDialog*)dlgMB)->EndDialog(IDNO);
		}

		GetParentFrame()->PostMessage(WM_BURNDVDA_CLOSE);

		return;
	}
	else
	{
		if (IDYES == MessageBox(_T("Do you want to abort the burning process?"), 
			_T(PROGRAM_NAME), MB_YESNO | MB_ICONQUESTION))
		{
			if (m_hThread)
			{
				m_bQuit = TRUE;

				if (NULL != m_burnThreadParam.dvdAudio)
				{
					m_burnThreadParam.dvdAudio->Terminate();
				}
				else
				{
					StarBurn_CdvdBurnerGrabber_Cancel(*m_burnThreadParam.pvGrabber);
				}

				WaitForSingleObject(m_hThread, 20000);
				TerminateThread (m_hThread, 1 );

				CloseHandle(m_hThread);
				m_hThread = NULL;
			}

			GetParentFrame()->PostMessage(WM_BURNDVDA_CLOSE);

			return;
		}
	}
}

//
VOID CBurnDVDA::SetEventMessage(_TCHAR* tcMessage)
{
	if (m_bQuit)
	{
		return;
	}

	CListCtrl* listCompilation = (CListCtrl*)GetDlgItem(IDC_LISTCOMPILATION);

	_TCHAR strTime[200];
	GetTimeFormat(NULL, TIME_FORCE24HOURFORMAT, NULL, _T("hh':'mm':'ss tt"), strTime, sizeof(strTime));

	INT iItem = listCompilation->InsertItem(LVIF_IMAGE, listCompilation->GetItemCount(), NULL, 0	, 0, 0, 0);
	listCompilation->SetItem(iItem, 1, LVIF_TEXT, strTime, 0, 0, 0, 0);
	listCompilation->SetItem(iItem, 2, LVIF_TEXT, tcMessage, 0, 0, 0, 0);

	listCompilation->EnsureVisible(listCompilation->GetItemCount() - 1, FALSE);
}

//
void CBurnDVDA::OnOK()
{
	return;

	CDialog::OnOK();
}

//
void CBurnDVDA::OnCancel()
{
	return;

	CDialog::OnCancel();
}

//
DWORD WINAPI CBurnDVDA::BurnThread(LPVOID lpVoid)
{
	CDVDAudio        DVDAudio(44100, 16);
	BURNTHREADPARAM* burnThreadParam = (BURNTHREADPARAM*)lpVoid;
	CBurnDVDA*       dlgBurn         = (CBurnDVDA*)burnThreadParam->dlgBurn;
	HWND             hwnd            = dlgBurn->m_hWnd;
	CMainFrame*      mainFrame       = (CMainFrame*)burnThreadParam->mainFrame;
	TRACKINFO***     trackInfo       = mainFrame->GetTrackInfo();
	UINT             uiCountGroup    = mainFrame->GetCountGroup();
	UINT*            uiCountTrack    = mainFrame->GetCountTrack();
	CString          strPath;	
	CString          strMsg;
	STRUCTURECONTEXT structureContext;

	burnThreadParam->dvdAudio = NULL;

	dlgBurn->SetEventMessage(_T("Start..."));

	// открытие устройства
	EXCEPTION_NUMBER        expNum    = EN_SUCCESS;
	ULONG                   ulStatus  = 0;
	CDB_FAILURE_INFORMATION cdbInf;
	DEVICEINFO              devInfo;
	PVOID                   pvGrabber;
	BURNCONTEXT             burnContext;

	devInfo = mainFrame->GetDeviceInfo();

	burnContext.dlgBurn        = dlgBurn;
	burnContext.progressStatus = (CProgressCtrl*)dlgBurn->GetDlgItem(IDC_PROGRESSTATUS);

	expNum = StarBurn_CdvdBurnerGrabber_Create(&pvGrabber, 
		NULL, 0, &ulStatus, &cdbInf, (PCALLBACK)BurnCallBack, &burnContext, 
		devInfo.ucPortID, devInfo.ucBusID, devInfo.ucTargetID, devInfo.ucLUN, 1);

	if ((EN_SUCCESS != expNum) || dlgBurn->GetQuit())
	{
		if (!dlgBurn->GetQuit())
		{
			dlgBurn->MessageBox(_T("ERROR::BurnThread::StarBurn_CdvdBurnerGrabber_Create"));

			dlgBurn->SetQuit(TRUE);
			dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
		}

		return 1;
	}

	burnThreadParam->pvGrabber = &pvGrabber;

	// ожидание диска в приводе
	BOOLEAN bIsMediaPresent, bIsDoorOrTrayOpened;

	do {
		dlgBurn->SetEventMessage(_T("Wait media present and door/tray closed... "));

		expNum = StarBurn_CdvdBurnerGrabber_GetMediaTrayStatus(pvGrabber, NULL, 0, &ulStatus, &cdbInf, 
			&bIsMediaPresent, &bIsDoorOrTrayOpened);
		if ((EN_SUCCESS != expNum) || dlgBurn->GetQuit())
		{
			if (!dlgBurn->GetQuit())
			{
				dlgBurn->MessageBox(_T("ERROR::BurnThread::StarBurn_CdvdBurnerGrabber_GetMediaTrayStatus"));

				dlgBurn->SetQuit(TRUE);
				dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
			}

			return 1;
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

	// блокирование привода
	dlgBurn->SetEventMessage(_T("Locking the media inside the DVD device"));
	StarBurn_CdvdBurnerGrabber_Lock(pvGrabber, NULL, 0, NULL, &cdbInf);

	// Try to test unit ready
	dlgBurn->SetEventMessage(_T("Try to test unit ready"));
	expNum = StarBurn_CdvdBurnerGrabber_TestUnitReady(pvGrabber, NULL, 0, &ulStatus, &cdbInf);
	if ((EN_SUCCESS != expNum) || dlgBurn->GetQuit())
	{
		if (!dlgBurn->GetQuit())
		{
			dlgBurn->MessageBox(_T("ERROR::BurnThread::StarBurn_CdvdBurnerGrabber_TestUnitReady"));

			dlgBurn->SetQuit(TRUE);
			dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
		}

		StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
		StarBurn_Destroy(&pvGrabber);

		return 1;
	}

	// попытка включения BUP
	dlgBurn->SetEventMessage(_T("Setting BUP (Buffer Underrun Protection) on DVD burner device"));
	expNum = StarBurn_CdvdBurnerGrabber_SetBUP(pvGrabber, NULL, 0, &ulStatus, &cdbInf, TRUE);

	// установка скорости 
	dlgBurn->SetEventMessage(_T("Setting maximum speed on DVD burner device"));

	UINT uiSpeed = CDVD_SPEED_IS_KBPS_MAXIMUM;
	//UINT uiSpeed = DVD_SPEED_IN_KBPS_2DOT4X;

	expNum = StarBurn_CdvdBurnerGrabber_SetSpeeds(pvGrabber, NULL, 0, &ulStatus, &cdbInf, 
		CDVD_SPEED_IS_KBPS_MAXIMUM, uiSpeed);
	if (EN_SUCCESS != expNum)
	{
		if ((!dlgBurn->GetQuit()) || dlgBurn->GetQuit())
		{
			dlgBurn->MessageBox(_T("ERROR::BurnThread::StarBurn_CdvdBurnerGrabber_SetSpeeds"));

			dlgBurn->SetQuit(TRUE);
			dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
		}

		StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
		StarBurn_Destroy(&pvGrabber);

		return 1;
	}

	// получение информации о диске
	dlgBurn->SetEventMessage(_T("Getting disc information of the currently inserted disc on DVD burner device"));

	DISC_INFORMATION discInfo;
	expNum = StarBurn_CdvdBurnerGrabber_GetDiscInformation(pvGrabber, NULL, 0, &ulStatus, &cdbInf, &discInfo);
	if (EN_SUCCESS != expNum)
	{
		if ((!dlgBurn->GetQuit()) || dlgBurn->GetQuit())
		{
			dlgBurn->MessageBox(_T("ERROR::BurnThread::StarBurn_CdvdBurnerGrabber_GetDiscInformation"));

			dlgBurn->SetQuit(TRUE);
			dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
		}

		StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
		StarBurn_Destroy(&pvGrabber);

		return 1;
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
		if ((!dlgBurn->GetQuit()) || dlgBurn->GetQuit())
		{
			dlgBurn->MessageBox(_T("ERROR::BurnThread::StarBurn_CdvdBurnerGrabber_GetInsertedDiscType::Media not writable"));

			dlgBurn->SetQuit(TRUE);
			dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
		}

		StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
		StarBurn_Destroy(&pvGrabber);

		return 1;
	}

	// получение информации о содержимом диска
	TRACK_INFORMATION trackInformation;
	expNum = StarBurn_CdvdBurnerGrabber_GetTrackInformation(pvGrabber, NULL, 0, &ulStatus, &cdbInf, 
		TRACK_NUMBER_INVISIBLE, (PTRACK_INFORMATION)(&trackInformation));
	if ((EN_SUCCESS != expNum) || dlgBurn->GetQuit())
	{
		if (!dlgBurn->GetQuit())
		{
			dlgBurn->MessageBox(_T("ERROR::BurnThread::StarBurn_CdvdBurnerGrabber_GetTrackInformation"));

			dlgBurn->SetQuit(TRUE);
			dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
		}

		StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
		StarBurn_Destroy(&pvGrabber);

		return 1;
	}

	// если не пустой и перезаписываемый, то стираем
	if (trackInformation.m__LONG__NextWritableAddress > 0)
	{
		if (!discInfo.m__BOOLEAN__IsErasable)
		{
			if (!dlgBurn->GetQuit())
			{
				dlgBurn->MessageBox(_T("ERROR::BurnThread::Media is not erasable"));

				dlgBurn->SetQuit(TRUE);
				dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
			}

			StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
			StarBurn_Destroy(&pvGrabber);

			return 1;
		}

		dlgBurn->SetEventMessage(_T("Erase media, please wait..."));
		expNum = StarBurn_CdvdBurnerGrabber_Blank(pvGrabber, NULL, 0, &ulStatus, &cdbInf, ERASE_TYPE_BLANK_DISC_FAST);
		if ((EN_SUCCESS != expNum) || dlgBurn->GetQuit())
		{
			if (!dlgBurn->GetQuit())
			{
				dlgBurn->MessageBox(_T("ERROR::BurnThread::StarBurn_CdvdBurnerGrabber_Blank"));

				dlgBurn->SetQuit(TRUE);
				dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
			}
	
			StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
			StarBurn_Destroy(&pvGrabber);

			return 1;
		}
	}

	// формирование структуры DVD-A
	dlgBurn->SetEventMessage(_T("Creating structure of DVD-Audio"));

	DVDAudio.SetCallBack(dlgBurn->StructureCallBack, &structureContext);

	strPath.Format(_T("%s\\%s"), mainFrame->GetDirPath(), _T("DVD-A"));
	CreateDirectory(strPath, NULL);

	structureContext.uiAllTracks    = 0;
	structureContext.uiCurrentGroup = 0;
	structureContext.uiCurrentTrack = 0;
	structureContext.progressStatus = (CProgressCtrl*)dlgBurn->GetDlgItem(IDC_PROGRESSTATUS);
	structureContext.progressStatus->SetPos(0);
	structureContext.trackInfo      = trackInfo;
	structureContext.dlgBurn        = dlgBurn;

	for (UINT i = 0; i < uiCountGroup; i++)
	{
		structureContext.uiAllTracks += uiCountTrack[i];

		for (UINT j = 0; j < uiCountTrack[i]; j++)
		{
			DVDAudio.AddTrack(i + 1, trackInfo[i][j]->tcFileName);
		}
	}

	burnThreadParam->dvdAudio = &DVDAudio;
	if (S_OK != DVDAudio.CreateDVDAudio(strPath.GetBuffer(0)))
	{
		if (!dlgBurn->GetQuit())
		{
			dlgBurn->MessageBox(_T("ERROR::BurnThread::CreateDVDAudio"));

			dlgBurn->SetQuit(TRUE);
			dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
		}

		StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
		StarBurn_Destroy(&pvGrabber);

		burnThreadParam->dvdAudio = NULL;

		return 1;
	}
	burnThreadParam->dvdAudio = NULL;

	/*
	dlgBurn->SetQuit(TRUE);
	dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);

	StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
	StarBurn_Destroy(&pvGrabber);

	return 1;
	*/

	// создание ISO 
	UDF_TREE_ITEM     udfDirectory[3];      // структура директорий в ISO образе
	UDF_TREE_ITEM     udfFile[100];         // структура файлов в ISO образе
	UINT              uiGUID;               // ID элементов структуры ISO образа
	_TCHAR            tcFilePath[MAX_PATH]; //
	UINT              uiCountFiles;         // количество файлов в ISO образе
	UDF_CONTROL_BLOCK udfControlBlock;      // 

	dlgBurn->SetEventMessage(_T("Creating UDF tree"));

	uiGUID       = 0;
	uiCountFiles = 0;

	StarBurn_UDF_FormatTreeItemAsDirectory(&udfDirectory[0], ++uiGUID, "", NULL);
	StarBurn_UDF_FormatTreeItemAsDirectory(&udfDirectory[1], ++uiGUID, "VIDEO_TS", &udfDirectory[0]);
	StarBurn_UDF_FormatTreeItemAsDirectory(&udfDirectory[2], ++uiGUID, "AUDIO_TS", &udfDirectory[0]);

	_stprintf(tcFilePath, _T("%s\\%s\\AUDIO_TS\\AUDIO_PP.IFO"), mainFrame->GetDirPath(), _T("DVD-A"));
	if (TRUE == PathFileExists(tcFilePath))
	{
		StarBurn_UDF_FormatTreeItemAsFile(&udfFile[uiCountFiles++], ++uiGUID, 
			"AUDIO_PP.IFO", tcFilePath, &udfDirectory[2]);
	}

	_stprintf(tcFilePath, _T("%s\\%s\\AUDIO_TS\\AUDIO_TS.IFO"), mainFrame->GetDirPath(), _T("DVD-A"));
	if (TRUE == PathFileExists(tcFilePath))
	{
		StarBurn_UDF_FormatTreeItemAsFile(&udfFile[uiCountFiles++], ++uiGUID, 
			"AUDIO_TS.IFO", tcFilePath, &udfDirectory[2]);
	}

	_stprintf(tcFilePath, _T("%s\\%s\\AUDIO_TS\\AUDIO_TS.BUP"), mainFrame->GetDirPath(), _T("DVD-A"));
	if (TRUE == PathFileExists(tcFilePath))
	{
		StarBurn_UDF_FormatTreeItemAsFile(&udfFile[uiCountFiles++], ++uiGUID, 
			"AUDIO_TS.BUP", tcFilePath, &udfDirectory[2]);
	}

	for (INT i = 1; i < 10; i++)
	{
		_TCHAR tcName[MAX_PATH];

		_stprintf(tcFilePath, _T("%s\\%s\\AUDIO_TS\\ATS_0%i_0.IFO"), mainFrame->GetDirPath(), _T("DVD-A"), i);
		if (TRUE == PathFileExists(tcFilePath))
		{
			_stprintf(tcName, _T("ATS_0%i_0.IFO"), i);
			StarBurn_UDF_FormatTreeItemAsFile(&udfFile[uiCountFiles++], ++uiGUID, tcName, tcFilePath, &udfDirectory[2]);
		}

		for (INT j = 1; j < 10; j++)
		{
			_stprintf(tcFilePath, _T("%s\\%s\\AUDIO_TS\\ATS_0%i_%i.AOB"), mainFrame->GetDirPath(), _T("DVD-A"), i, j);
			if (TRUE == PathFileExists(tcFilePath))
			{
				_stprintf(tcName, _T("ATS_0%i_%i.AOB"), i, j);
				StarBurn_UDF_FormatTreeItemAsFile(&udfFile[uiCountFiles++], ++uiGUID, tcName, tcFilePath, &udfDirectory[2]);
			}
		}

		_stprintf(tcFilePath, _T("%s\\%s\\AUDIO_TS\\ATS_0%i_0.BUP"), mainFrame->GetDirPath(), _T("DVD-A"), i);
		if (TRUE == PathFileExists(tcFilePath))
		{
			_stprintf(tcName, _T("ATS_0%i_0.BUP"), i);
			StarBurn_UDF_FormatTreeItemAsFile(&udfFile[uiCountFiles++], ++uiGUID, tcName, tcFilePath, &udfDirectory[2]);
		}
	}

	expNum = StarBurn_UDF_Create(&udfDirectory[0], &udfDirectory[1], &udfDirectory[2], 
		&udfControlBlock, NULL, 0, NULL, "WHOLEGROUP.COM");

	// посылаем OPC (Optimum Power Calibration)
	dlgBurn->SetEventMessage(_T("Sending OPC (Optimum Power Calibration) for current write speed")
		_T(" on DVD burner device and current DVD media."));
	expNum = StarBurn_CdvdBurnerGrabber_SendOPC(pvGrabber, NULL, 0, &ulStatus, &cdbInf);
	if ((EN_SUCCESS != expNum) || dlgBurn->GetQuit())
	{
		if (!dlgBurn->GetQuit())
		{
			dlgBurn->MessageBox(_T("ERROR::BurnThread::StarBurn_CdvdBurnerGrabber_SendOPC"));

			dlgBurn->SetQuit(TRUE);
			dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
		}

		StarBurn_UDF_CleanUp(&udfFile[0], 99);
		if (NULL != udfControlBlock.m__PVOID__Body)
		{
			StarBurn_UDF_Destroy(&udfDirectory[0], &udfControlBlock);
		}

		StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
		StarBurn_Destroy(&pvGrabber);

		return 1;
	}

	// запус прожига
	dlgBurn->SetEventMessage(_T("Burning..."));

	burnContext.progressStatus->SetPos(0);

	expNum = StarBurn_CdvdBurnerGrabber_TrackAtOnceFromTree(pvGrabber, NULL, 0, &ulStatus, &cdbInf,
		udfControlBlock.m__PVOID__Body, TRUE, FALSE, FALSE, WRITE_REPORT_DELAY_IN_SECONDS, 
		BUFFER_STATUS_REPORT_DELAY_IN_SECONDS);

	if ((EN_SUCCESS != expNum) || dlgBurn->GetQuit())
	{
		if (!dlgBurn->GetQuit())
		{
			dlgBurn->MessageBox(_T("ERROR::BurnThread::StarBurn_CdvdBurnerGrabber_TrackAtOnceFromFile"));

			dlgBurn->SetQuit(TRUE);
			dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
		}

		StarBurn_UDF_CleanUp(&udfFile[0], 99);
		if (NULL != udfControlBlock.m__PVOID__Body)
		{
			StarBurn_UDF_Destroy(&udfDirectory[0], &udfControlBlock);
		}

		StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
		StarBurn_Destroy(&pvGrabber);

		return 1;
	}

	// закрываем сессию
	expNum = StarBurn_CdvdBurnerGrabber_CloseSession(pvGrabber, NULL, 0, &ulStatus, &cdbInf);
	if ((EN_SUCCESS != expNum) || dlgBurn->GetQuit())
	{
		if (!dlgBurn->GetQuit())
		{
			dlgBurn->MessageBox(_T("::BurnThread::StarBurn_CdvdBurnerGrabber_CloseSession"));

			dlgBurn->SetQuit(TRUE);
			dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE, 0);
		}

		StarBurn_UDF_CleanUp(&udfFile[0], 99);
		if (NULL != udfControlBlock.m__PVOID__Body)
		{
			StarBurn_UDF_Destroy(&udfDirectory[0], &udfControlBlock);
		}

		StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
		StarBurn_Destroy(&pvGrabber);

		return 1;
	}

	StarBurn_UDF_CleanUp(&udfFile[0], 99);
	if (NULL != udfControlBlock.m__PVOID__Body)
	{
		StarBurn_UDF_Destroy(&udfDirectory[0], &udfControlBlock);
	}

	StarBurn_CdvdBurnerGrabber_Release(pvGrabber, NULL, 0, NULL, &cdbInf);
	StarBurn_CdvdBurnerGrabber_Eject(pvGrabber, NULL, 0, &ulStatus, &cdbInf);
	StarBurn_Destroy(&pvGrabber);

	dlgBurn->SetQuit(TRUE);
	dlgBurn->PostMessage(WM_COMMAND, IDC_BUTTONCLOSE);

	return 0;
}

//
VOID __stdcall CBurnDVDA::StructureCallBack(UINT uiMessageID, PVOID pvContext, PVOID pvSpecial)
{
	STRUCTURECONTEXT* structureContext = (STRUCTURECONTEXT*)pvContext;
	CBurnDVDA*        dlgBurn          = (CBurnDVDA*)structureContext->dlgBurn;
	CString           strMsg;

	switch(uiMessageID)
	{
		// новая группа
		case MID_NEW_GROUP:
			structureContext->uiGroupTrack = 0;
			structureContext->uiCurrentGroup++;
			strMsg.Format(_T("Creating group %d"), structureContext->uiCurrentGroup);
			((CBurnDVDA*)structureContext->dlgBurn)->SetEventMessage(strMsg.GetBuffer(0));
			break;

		// новый трек
		case MID_NEW_TRACK:
			structureContext->uiGroupTrack++;
			structureContext->uiCurrentTrack++;
			strMsg = _T("");
			strMsg.Format(_T("Processing %s"), structureContext->trackInfo[structureContext->uiCurrentGroup - 1][structureContext->uiGroupTrack - 1]->tcFileName);
			((CBurnDVDA*)structureContext->dlgBurn)->SetEventMessage(strMsg.GetBuffer(0));
			break;

		// обработка трека
		case MID_PROCESS_TRACK:
			SPROCTRACK* sProcTrack = (SPROCTRACK*)pvSpecial;
			DOUBLE      dPeriod;
			DOUBLE      dStep;

			dPeriod = (DOUBLE)MAX_PROGRESS_STATUS / structureContext->uiAllTracks;
			dStep   = dPeriod / sProcTrack->uiNumBytes * sProcTrack->uiProcBytes;

			structureContext->progressStatus->SetPos((INT)(dPeriod * (structureContext->uiCurrentTrack - 1) + dStep));

			break;
	}

}

//
VOID __stdcall CBurnDVDA::BurnCallBack(IN CALLBACK_NUMBER cbNumber, IN PVOID pvContext, 
	IN PVOID pvSpecial1, IN PVOID pvSpecial2)
{
	BURNCONTEXT* structureContext = (BURNCONTEXT*)pvContext;

	switch (cbNumber)
	{
		case CN_TARGET_FILE_ANALYZE_END:
			structureContext->progressStatus->SetPos((INT)(MAX_PROGRESS_STATUS / 70.));
			break;

		case CN_DVD_MEDIA_PADDING_SIZE:
			structureContext->progressStatus->SetPos((INT)(MAX_PROGRESS_STATUS / 70. * 2));
			break;

		case CN_WAIT_CACHE_FULL_BEGIN:
			structureContext->progressStatus->SetPos((INT)(MAX_PROGRESS_STATUS / 70. * 3));
			break;

		case CN_WAIT_CACHE_FULL_END:
			structureContext->progressStatus->SetPos((INT)(MAX_PROGRESS_STATUS / 70. * 4));
			break;

		case CN_CDVD_WRITE_PROGRESS:
			LARGE_INTEGER  liFileSizeInLBs;
			LARGE_INTEGER  liLBsWritten;
			LARGE_INTEGER  liCurrentPercent;

			liFileSizeInLBs = *(PLARGE_INTEGER)(pvSpecial1);
			liLBsWritten    = *(PLARGE_INTEGER)(pvSpecial2);

			liCurrentPercent.QuadPart = (liLBsWritten.QuadPart * 100) / liFileSizeInLBs.QuadPart;

			structureContext->progressStatus->SetPos((INT)(MAX_PROGRESS_STATUS / 70. * 
				(4 + 62 * liCurrentPercent.QuadPart / 100.)));
			break;

		case CN_DVD_MEDIA_PADDING_BEGIN:
			structureContext->progressStatus->SetPos((INT)(MAX_PROGRESS_STATUS / 70. * 67));
			break;

		case CN_DVD_MEDIA_PADDING_END:
			structureContext->progressStatus->SetPos((INT)(MAX_PROGRESS_STATUS / 70. * 68));
			break;

		case CN_SYNCHRONIZE_CACHE_BEGIN:
			structureContext->progressStatus->SetPos((INT)(MAX_PROGRESS_STATUS / 70. * 69));
			break;

		case CN_SYNCHRONIZE_CACHE_END:
			structureContext->progressStatus->SetPos(MAX_PROGRESS_STATUS);
			break;

		case CN_CDVD_VERIFY_PROGRESS:
			break;

		default:
			break;
	}
}
