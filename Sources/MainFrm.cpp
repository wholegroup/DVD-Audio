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
#include "mainfrm.h"
#include "StarBurn\StarBurnKey.h"
#include "io.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame
IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_MESSAGE(WM_GRABAUDIOCD, GrabAudioCD)
	ON_MESSAGE(WM_GRABAUDIOCD_CLOSE, GrabAudioCDClose)
	ON_MESSAGE(WM_BURNDVDA, BurnDVDA)
	ON_MESSAGE(WM_BURNDVDA_CLOSE, BurnDVDAClose)
	ON_CBN_SELCHANGE(IDC_COMBODRIVE, OnChangeDrive)
	ON_BN_CLICKED(IDC_TOOLBARNEW, OnNew)
	ON_BN_CLICKED(IDC_TOOLBARGRAB, OnGrabbing)
	ON_BN_CLICKED(IDC_TOOLBARDELETEGROUP, OnDeleteGroup)
	ON_BN_CLICKED(IDC_TOOLBARBURN, OnBurning)
	ON_BN_CLICKED(IDC_TOOLBARHELP, OnHelp)
	ON_BN_CLICKED(IDC_TOOLBARABOUT, OnAbout)
	ON_UPDATE_COMMAND_UI(IDC_TOOLBARNEW, UpdateToolbarNew)
	ON_UPDATE_COMMAND_UI(IDC_TOOLBARGRAB, UpdateToolbarNew)
	ON_UPDATE_COMMAND_UI(IDC_TOOLBARDELETEGROUP, UpdateToolbarNew)
	ON_UPDATE_COMMAND_UI(IDC_TOOLBARBURN, UpdateToolbarNew)
	ON_UPDATE_COMMAND_UI(ID_APP_EXIT, UpdateToolbarNew)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR // status line indicator
};

//////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction
//
CMainFrame::CMainFrame()
{
	m_dlgGrabAudio = NULL;
	m_dlgBurnDVDA  = NULL;
	m_hFileFlag    = NULL;
	m_uiCountGroup = 0;
	RtlZeroMemory(m_uiCountTrack, sizeof(m_uiCountTrack));
	RtlZeroMemory(m_trackInfo, sizeof(m_trackInfo));
}

//
CMainFrame::~CMainFrame()
{
	m_arrDevInfo.RemoveAll();

	for (UINT i = 0; i < m_uiCountGroup; i++)
	{
		for (UINT j = 0; j < m_uiCountTrack[i]; j++)
		{
			DeleteFile(m_trackInfo[i][j]->tcFileName);

			delete m_trackInfo[i][j];
		}

		delete [] m_trackInfo[i];
	}
}

//
INT CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE); 
	
	// создание главного рабочего диалога
	m_dlgSixCDonDVD.Create(IDD_SIXCDONDVD, this);
	m_dlgSixCDonDVD.ShowWindow(TRUE);

	if (!m_wndDlgBar.Create(this, IDD_TOOLBAR, 
		CBRS_ALIGN_TOP, AFX_IDW_DIALOGBAR))
	{
		return -1;
	}

	// установка картинок на кнопки тулбара
	m_btnNew.SubclassWindow(m_wndDlgBar.GetDlgItem(IDC_TOOLBARNEW)->GetSafeHwnd());
	m_btnNew.SetImages(IDI_TOOLBARNEW);
	m_btnNew.SetWindowText(_T(""));
	m_btnNew.SetStyleFlat(TRUE);

	m_btnGrab.SubclassWindow(m_wndDlgBar.GetDlgItem(IDC_TOOLBARGRAB)->GetSafeHwnd());
	m_btnGrab.SetImages(IDI_TOOLBARGRAB);
	m_btnGrab.SetWindowText(_T(""));
	m_btnGrab.SetStyleFlat(TRUE);

	m_btnDeleteGroup.SubclassWindow(m_wndDlgBar.GetDlgItem(IDC_TOOLBARDELETEGROUP)->GetSafeHwnd());
	m_btnDeleteGroup.SetImages(IDI_TOOLBARDELETEGROUP);
	m_btnDeleteGroup.SetWindowText(_T(""));
	m_btnDeleteGroup.SetStyleFlat(TRUE);

	m_btnBurn.SubclassWindow(m_wndDlgBar.GetDlgItem(IDC_TOOLBARBURN)->GetSafeHwnd());
	m_btnBurn.SetImages(IDI_TOOLBARBURN);
	m_btnBurn.SetWindowText(_T(""));
	m_btnBurn.SetStyleFlat(TRUE);

	m_btnHelp.SubclassWindow(m_wndDlgBar.GetDlgItem(IDC_TOOLBARHELP)->GetSafeHwnd());
	m_btnHelp.SetImages(IDI_TOOLBARHELP);
	m_btnHelp.SetWindowText(_T(""));
	m_btnHelp.SetStyleFlat(TRUE);

	m_btnAbout.SubclassWindow(m_wndDlgBar.GetDlgItem(IDC_TOOLBARABOUT)->GetSafeHwnd());
	m_btnAbout.SetImages(IDI_TOOLBARABOUT);
	m_btnAbout.SetWindowText(_T(""));
	m_btnAbout.SetStyleFlat(TRUE);

	if (!m_wndReBar.Create(this) ||
		!m_wndReBar.AddBar(&m_wndDlgBar))
	{
		return -1;
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		return -1;
	}

	// инициализация ядра StarBurn
	EXCEPTION_NUMBER expNum = EN_SUCCESS;

	expNum = StarBurn_UpStartEx(&g__UCHAR__RegistrationKey, sizeof(g__UCHAR__RegistrationKey));
	if (EN_SUCCESS != expNum)
	{
		MessageBox(_T("StarBurn initialization error"), _T("Error"), MB_ICONERROR);
		PostMessage(WM_CLOSE);
		return 0;
	}

	// поиск существующих приводов
	DWORD dwCountDevice;

	dwCountDevice = StarBurn_FindDevice(SCSI_DEVICE_RO_DIRECT_ACCESS, FALSE, 
		(PCALLBACK)(CallBackPrint), (PVOID)&m_arrDevInfo);

	if (!m_arrDevInfo.GetSize())
	{
		MessageBox(_T("Not find devices"), _T("Error"), MB_ICONERROR);
		PostMessage(WM_CLOSE);
		return 0;
	}

	// заполняем комбо бокс: список драйвов
	CComboBox* cbTmp = (CComboBox*)m_wndDlgBar.GetDlgItem(IDC_COMBODRIVE);

	for (INT i = 0; i < m_arrDevInfo.GetSize(); i++)
	{
		DEVICEINFO devInfo = m_arrDevInfo.ElementAt(i);

		cbTmp->AddString(CString(devInfo.tcLetter) + CString(_T("  ")) + CString(devInfo.tcName));
	}
	cbTmp->SetCurSel(0);

	// удаление старых временных файлов
	RemoveTemp();

	// создание временного каталога
	DWORD dwDirSize = sizeof(m_tcDirPath);
	GetTempPath(dwDirSize, m_tcDirPath);

	if (_T('\\') == m_tcDirPath[_tcslen(m_tcDirPath) - 1])
	{
		_tcscat(m_tcDirPath, _T(PROGRAM_NAME));
	}
	else
	{
		_tcscat(m_tcDirPath, _T("\\"));
		_tcscat(m_tcDirPath, _T(PROGRAM_NAME));
	}

	CreateDirectory(m_tcDirPath, NULL);

	CString strTickCount;
	strTickCount.Format(_T("%Iu"), GetTickCount());

	_tcscat(m_tcDirPath, _T("\\"));
	_tcscat(m_tcDirPath, strTickCount);

	if (!CreateDirectory(m_tcDirPath, NULL))
	{
		MessageBox(_T("ERROR::CreateDirectory::TEMP"));

		PostMessage(WM_CLOSE);
	}

	// создание файла флага
	m_hFileFlag = CreateFile(CString(m_tcDirPath) + CString(_T("\\flag")), 
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (m_hFileFlag == INVALID_HANDLE_VALUE) 
	{ 
		MessageBox(_T("ERROR::CreateFile::FLAG"));

		PostMessage(WM_CLOSE);
	}

	SetFocus();

	return 0;
}

//
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// обработка закрытия программы
//
VOID CMainFrame::OnClose()
{
	if (NULL != m_dlgBurnDVDA)
	{
		return;
	}

	if (NULL != m_dlgGrabAudio)
	{
		delete m_dlgGrabAudio;
	}

	StarBurn_DownShut();

	CloseHandle(m_hFileFlag);

	// удаление старых временных файлов
	RemoveTemp();

	CFrameWnd::OnClose();
}

//
VOID CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// forward focus to the view window
	// m_dlgSixCDonDVD.SetFocus();
}

//////////////////////////////////////////////////////////////////////////
// Функция возвращает координаты в клиентской области (для вывода диалога)
//
CRect CMainFrame::GetClientSize()
{
	CRect clientSize;
	CRect reBarSize;
	CRect statusBarSize;

	this->GetClientRect(&clientSize);
	m_wndReBar.GetWindowRect(&reBarSize);
	m_wndDlgBar.GetWindowRect(&reBarSize);
	m_wndStatusBar.GetWindowRect(&statusBarSize);

	clientSize.top    += (reBarSize.bottom - reBarSize.top);
	clientSize.bottom -= (statusBarSize.bottom - statusBarSize.top);

	return clientSize;
}

//////////////////////////////////////////////////////////////////////////
// Обработчик изменения размеров главного фрейма
// 
void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	if (SIZE_MINIMIZED != nType)
	{
		if (NULL != m_dlgGrabAudio)
		{
			m_dlgGrabAudio->SetSize();
		}
		else

		if (NULL != m_dlgBurnDVDA)
		{
			m_dlgBurnDVDA->SetSize();
		}

		else
		{
			m_dlgSixCDonDVD.SetSize();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Создания диалога для граббинга дисков
//
LRESULT CMainFrame::GrabAudioCD(WPARAM wParam, LPARAM lParam)
{
	// скрываем кнопку закрытия окна
	HMENU hMenu = ::GetSystemMenu(this->m_hWnd, 0);
	::EnableMenuItem(hMenu, SC_CLOSE, MF_GRAYED | MF_BYCOMMAND);

	// сокрытие главного диалога
	m_dlgSixCDonDVD.ShowWindow(FALSE);

	// вызов диалога для граббинга
	if (NULL == m_dlgGrabAudio)
	{
		m_dlgGrabAudio = new CGrabAudio();

		m_dlgGrabAudio->Create(IDD_GRABAUDIO, this);
	}

	m_dlgGrabAudio->ShowWindow(TRUE);
	m_dlgGrabAudio->SetForegroundWindow();

	m_dlgGrabAudio->SetSize();

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// CallBack для поиска существующих приводов
//
VOID __stdcall CMainFrame::CallBackPrint(IN CALLBACK_NUMBER cbNumber, 
	IN PVOID pvContext, IN PVOID pvSpecial1, IN PVOID pvSpecial2)
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
					__leave;
				}

				DEVICEINFO devNew;
				devNew.ucBusID    = pDevice->m__UCHAR__BusID;
				devNew.ucLUN      = pDevice->m__UCHAR__LUN;
				devNew.ucPortID   = pDevice->m__UCHAR__PortID;
				devNew.ucTargetID = pDevice->m__UCHAR__TargetID;
				_stprintf(devNew.tcName, _T("%s %s"), cVendorID, cProductID);
				_stprintf(devNew.tcLetter, _T("%s"), &cDeviceName[4]);

				CArrayDevInfo* arrDevInfo;
				arrDevInfo = (CArrayDevInfo*)pvContext;
				arrDevInfo->Add(devNew);
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
// Получить информацию о выбранном девайсе
//
DEVICEINFO CMainFrame::GetDeviceInfo()
{
	CComboBox* cbTmp = (CComboBox*)m_wndDlgBar.GetDlgItem(IDC_COMBODRIVE);

	return m_arrDevInfo.ElementAt(cbTmp->GetCurSel());
}

//////////////////////////////////////////////////////////////////////////
// Обработка изменения драйва
//
void CMainFrame::OnChangeDrive()
{
	if (NULL != m_dlgGrabAudio)
	{
		m_dlgGrabAudio->OnRefreshList();
	}
}

//////////////////////////////////////////////////////////////////////////
// Обработка закрытия диалога граббинга CD-A
//
LRESULT CMainFrame::GrabAudioCDClose(WPARAM wParam, LPARAM lParam)
{
	if (NULL != m_dlgGrabAudio)
	{
      // закрываем диалог граббинга
		m_dlgGrabAudio->CloseWindow();
		delete m_dlgGrabAudio;
		m_dlgGrabAudio = NULL;

		m_dlgSixCDonDVD.SetSizeData();
		m_dlgSixCDonDVD.SetTreeData();

		m_dlgSixCDonDVD.ShowWindow(TRUE);
		m_dlgSixCDonDVD.SetSize();
	}

	HMENU hMenu = ::GetSystemMenu(this->m_hWnd, 0);
	::EnableMenuItem(hMenu, SC_CLOSE, MF_ENABLED | MF_BYCOMMAND);

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// Удаление директории вместе со всем содержимым
//
VOID CMainFrame::RemoveDirectoryRecursive(CString* strDirPath)
{
	intptr_t     iSearch;
	_tfinddata_t findData;
	CString      strPath;

	strPath = *strDirPath + CString(_T("\\*.*"));
	iSearch = _tfindfirst(strPath, &findData);

	if (-1 == iSearch)
	{
		return;
	}

	do
	{
		if ((0 == _tcscmp(findData.name, _T("."))) || (0 == _tcscmp(findData.name, _T(".."))))
		{
			continue;
		}

		CString strDelPath;
		strDelPath = *strDirPath + CString(_T("\\")) + CString(findData.name);

		if (findData.attrib & _A_SUBDIR)
		{
			RemoveDirectoryRecursive(&strDelPath);
		}
		else
		{
			DeleteFile(strDelPath);
		}

	} while (!_tfindnext(iSearch, &findData));

	_findclose(iSearch);

	RemoveDirectory(*strDirPath);
}

//////////////////////////////////////////////////////////////////////////
// Удаление временных файлов
//
VOID CMainFrame::RemoveTemp()
{
	DWORD dwDirSize = sizeof(m_tcDirPath);
	GetTempPath(dwDirSize, m_tcDirPath);

	if (_T('\\') == m_tcDirPath[_tcslen(m_tcDirPath) - 1])
	{
		_tcscat(m_tcDirPath, _T(PROGRAM_NAME));
	}
	else
	{
		_tcscat(m_tcDirPath, _T("\\"));
		_tcscat(m_tcDirPath, _T(PROGRAM_NAME));
	}

	intptr_t     iSearch;
	_tfinddata_t findData;
	CString      strPath;

	strPath = CString(m_tcDirPath) + CString(_T("\\*.*"));
	iSearch = _tfindfirst(strPath, &findData);

	if (-1 == iSearch)
	{
		return;
	}

	do
	{
		if ((0 == _tcscmp(findData.name, _T("."))) || (0 == _tcscmp(findData.name, _T(".."))))
		{
			continue;
		}

		CString strDelPath;
		strDelPath = CString(m_tcDirPath) + CString(_T("\\")) + CString(findData.name);

		if (findData.attrib & _A_SUBDIR)
		{
			m_hFileFlag = CreateFile(strDelPath + CString(_T("\\flag")),
				GENERIC_WRITE,
				0,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);

			if (INVALID_HANDLE_VALUE != m_hFileFlag)
			{
				CloseHandle(m_hFileFlag);

				RemoveDirectoryRecursive(&strDelPath);
			}
		}
		else
		{
			DeleteFile(strDelPath);
		}

	} while (!_tfindnext(iSearch, &findData));

	_findclose(iSearch);

	RemoveDirectory(m_tcDirPath);
}

//////////////////////////////////////////////////////////////////////////
// Очистка структуры DVD-A
//
void CMainFrame::OnNew()
{
	if (IDYES == MessageBox(_T("Create the new compilation and destroy old?"), 
		NULL, MB_ICONQUESTION | MB_YESNO))
	{
		for (UINT i = 0; i < m_uiCountGroup; i++)
		{
			for (UINT j = 0; j < m_uiCountTrack[i]; j++)
			{
				DeleteFile(m_trackInfo[i][j]->tcFileName);

				delete m_trackInfo[i][j];
			}
			m_uiCountTrack[i] = 0;

			delete [] m_trackInfo[i];
		}
		m_uiCountGroup = 0;

		m_dlgSixCDonDVD.SetSizeData();
		m_dlgSixCDonDVD.SetTreeData();
	}
}

//////////////////////////////////////////////////////////////////////////
// Создание диалога прожига ДВДА
//
LRESULT CMainFrame::BurnDVDA(WPARAM wParam, LPARAM lParam)
{
	// диалог установки скорости записи
	CDialog dlgTmp(IDD_SETWRITESPEED);
	if (IDOK != dlgTmp.DoModal())
	{
		return FALSE;
	}

	// скрываем кнопку выхода из программы
	HMENU hMenu = ::GetSystemMenu(this->m_hWnd, 0);
	::EnableMenuItem(hMenu, SC_CLOSE, MF_GRAYED | MF_BYCOMMAND);

	// скрываем главный диалог
	m_dlgSixCDonDVD.ShowWindow(FALSE);

	// открываем диалог записи DVD-A
	if (NULL == m_dlgBurnDVDA)
	{
		m_dlgBurnDVDA = new CBurnDVDA();

		m_dlgBurnDVDA->Create(IDD_BURNDVDA, this);
	}

	m_dlgBurnDVDA->ShowWindow(TRUE);
	m_dlgBurnDVDA->SetForegroundWindow();

	m_dlgBurnDVDA->SetSize();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// Закрытие диалога прожига ДВДА
//
LRESULT CMainFrame::BurnDVDAClose(WPARAM wParam, LPARAM lParam)
{
	if (NULL != m_dlgBurnDVDA)
	{
		// закрываем диалог граббинга
		m_dlgBurnDVDA->CloseWindow();
		delete m_dlgBurnDVDA;
		m_dlgBurnDVDA = NULL;

		m_dlgSixCDonDVD.SetSizeData();
		m_dlgSixCDonDVD.SetTreeData();

		m_dlgSixCDonDVD.ShowWindow(TRUE);
		m_dlgSixCDonDVD.SetSize();
	}

	HMENU hMenu = ::GetSystemMenu(this->m_hWnd, 0);
	::EnableMenuItem(hMenu, SC_CLOSE, MF_ENABLED | MF_BYCOMMAND);

	return 0;
}

//
void CMainFrame::OnGrabbing()
{
	PostMessage(WM_GRABAUDIOCD);
}

//////////////////////////////////////////////////////////////////////////
// Удаление выбранной группы
//
void CMainFrame::OnDeleteGroup()
{
	if (0 == m_uiCountGroup)
	{
		return;
	}

	if (IDYES == MessageBox(_T("Are you sure that you want to remove the selected group?"), 
		_T(PROGRAM_NAME), MB_YESNO | MB_ICONQUESTION))
	{
		UINT uiGroup = m_dlgSixCDonDVD.GetSelectedGroup();

		for (UINT i = 0; i < m_uiCountTrack[uiGroup]; i++)
		{
			DeleteFile(m_trackInfo[uiGroup][i]->tcFileName);

			delete m_trackInfo[uiGroup][i];
		}
		delete [] m_trackInfo[uiGroup];

		for (UINT i = uiGroup + 1; i < m_uiCountGroup; i++)
		{
			m_trackInfo[i - 1] = m_trackInfo[i];
		}
		m_uiCountTrack[m_uiCountGroup - 1] = 0;

		m_uiCountGroup--;

		m_dlgSixCDonDVD.SetSizeData();
		m_dlgSixCDonDVD.SetTreeData();
	}
}

//////////////////////////////////////////////////////////////////////////
// Обработка нажатия кнопки прожига на тулбаре
//
void CMainFrame::OnBurning()
{
	PostMessage(WM_BURNDVDA);
}

//////////////////////////////////////////////////////////////////////////
// Вызов справки
//
void CMainFrame::OnHelp()
{
	TCHAR buff[MAX_PATH];
	CString strPathCHM;

	GetModuleFileName(AfxGetApp()->m_hInstance, buff, MAX_PATH);
	while (_tcslen(buff) && ('\\' != buff[_tcslen(buff) - 1]))
	{
		buff[_tcslen(buff) - 1] = 0;
	}

	strPathCHM = buff;
	strPathCHM += _T("AudioSixPack.chm");

	::HtmlHelp(GetDesktopWindow()->m_hWnd, strPathCHM, HH_DISPLAY_TOC, 0);
}

//////////////////////////////////////////////////////////////////////////
// Вызов диалога "О программе..."
//
void CMainFrame::OnAbout()
{
	CAboutDlg dlg;

	dlg.DoModal();
}

//
VOID CMainFrame::UpdateToolbarNew(CCmdUI* pCmdUI)
{
	if (NULL != m_dlgGrabAudio)
	{
		pCmdUI->Enable(FALSE);
	}
	else

	if (NULL != m_dlgBurnDVDA)
	{
		pCmdUI->Enable(FALSE);
	}

	else
	{
		if (!m_uiCountGroup && (pCmdUI->m_nID != ID_APP_EXIT))
		{
			pCmdUI->Enable(FALSE);
		}
		else
		{
			pCmdUI->Enable(TRUE);
		}
	}
}