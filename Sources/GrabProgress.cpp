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
#include "grabprogress.h"

IMPLEMENT_DYNAMIC(CGrabProgress, CDialog)

BEGIN_MESSAGE_MAP(CGrabProgress, CDialog)
END_MESSAGE_MAP()

//
#define MAX_PROGRESS_TOTAL   1000
#define MAX_PROGRESS_CURRENT 1000

//////////////////////////////////////////////////////////////////////////
//
//
CGrabProgress::CGrabProgress(CWnd* pParent /*=NULL*/)
	: CDialog(CGrabProgress::IDD, pParent)
{
	m_hThread      = NULL;
	m_dwThreadID   = 0;
	m_bQuit        = FALSE;
	m_devInfo      = NULL;
	m_uiCountTrack = 0;
	m_trackInfo    = NULL;
	m_pvGrabber    = NULL;
}

//
CGrabProgress::~CGrabProgress()
{
	if (NULL != m_pvGrabber)
	{
		StarBurn_Destroy(&m_pvGrabber);
	}
}

//
void CGrabProgress::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

//
void CGrabProgress::OnCancel()
{
	if (m_bQuit)
	{
		DWORD dwExitCode = 1;

		if (m_hThread)
		{
			WaitForSingleObject(m_hThread, INFINITE);
			GetExitCodeThread(m_hThread, &dwExitCode);
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}

		if (dwExitCode)
		{
			CDialog::OnCancel();
		}
		else
		{
			CDialog::OnOK();
		}
	}
	else
	{
		if (IDYES != MessageBox(_T("Are you sure you want to cancel?"), 
			_T(PROGRAM_NAME), MB_YESNO | MB_ICONQUESTION))
		{
			return;
		}

		if (m_hThread)
		{
			m_bQuit = TRUE;
			StarBurn_CdvdBurnerGrabber_Cancel(m_pvGrabber);
			WaitForSingleObject(m_hThread, INFINITE);
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}

		CDialog::OnCancel();
	}
}

//
void CGrabProgress::OnOK()
{
	return;

	CDialog::OnOK();
}

//
BOOL CGrabProgress::OnInitDialog()
{
	CDialog::OnInitDialog();

	// инициализация контролов
	CProgressCtrl* progressTotal   = (CProgressCtrl*)GetDlgItem(IDC_PROGRESSTOTAL);
	CProgressCtrl* progressCurrent = (CProgressCtrl*)GetDlgItem(IDC_PROGRESSCURRENT);

	progressTotal->SetRange(0, MAX_PROGRESS_TOTAL);
	progressTotal->SetPos(0);
	progressCurrent->SetRange(0, MAX_PROGRESS_CURRENT);
	progressCurrent->SetPos(0);

	GetDlgItem(IDC_STATICNAME)->SetWindowText(_T(""));

	// открытие устройства
	EXCEPTION_NUMBER        expNum          = EN_SUCCESS;
	ULONG                   ulStatus        = 0;
	CDB_FAILURE_INFORMATION cdbInf;
	expNum = StarBurn_CdvdBurnerGrabber_Create(&m_pvGrabber, 
			NULL, 0, &ulStatus, &cdbInf, CallBackGrab, (PVOID)&m_grabContext, 
			m_devInfo->ucPortID, m_devInfo->ucBusID, m_devInfo->ucTargetID, m_devInfo->ucLUN, 1);

	if (EN_SUCCESS != expNum)
	{
		MessageBox(_T("ERROR::StarBurn_CdvdBurnerGrabber_Create"));
		m_bQuit = TRUE;
		PostMessage(WM_QUIT);

		return TRUE;
	}

	// создание треда для граббинга
	m_grabThreadParam.dlgProgress = this;
	m_hThread = CreateThread(NULL, 0, GrabThread, &m_grabThreadParam, 0, &m_dwThreadID);
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
BOOL CGrabProgress::DestroyWindow()
{
	return CDialog::DestroyWindow();
}

//
DWORD WINAPI CGrabProgress::GrabThread(LPVOID lpVoid)
{
	GRABTHREADPARAM*        grabThreadParam = (GRABTHREADPARAM*)lpVoid;
	CGrabProgress*          dlgProgress     = (CGrabProgress*)grabThreadParam->dlgProgress;
	HWND                    hwnd            = dlgProgress->m_hWnd;
	DEVICEINFO*             devInfo         = dlgProgress->GetDevInfo();
	UINT                    uiCountTrack    = 0;
	GRABTRACKINFO*          trackInfo       = NULL;
	_TCHAR*                 tcDirPath       = dlgProgress->GetDirPath();
	PVOID                   pvGrabber       = dlgProgress->GetGrabPointer();
	EXCEPTION_NUMBER        expNum          = EN_SUCCESS;
	ULONG                   ulStatus        = 0;
	CDB_FAILURE_INFORMATION cdbInf;
	GRABCONTEXT*            grabContext     = dlgProgress->GetGrabContext();

	// проверка информации о девайсе
	if (NULL == devInfo)
	{
		dlgProgress->MessageBox(_T("ERROR::GrabThread::devInfo::NULL"));

		dlgProgress->SetQuit(TRUE);
		::PostMessage(hwnd, WM_CLOSE, 0, 0);
		return 1;
	}

	// проверка информации о треках
	uiCountTrack = dlgProgress->GetTrackInfo(&trackInfo);
	if (0 == uiCountTrack)
	{
		dlgProgress->MessageBox(_T("ERROR::GrabThread::trackInfo::NULL"));

		dlgProgress->SetQuit(TRUE);
		::PostMessage(hwnd, WM_CLOSE, 0, 0);
		return 1;
	}

	// проверка на готовность
	expNum = StarBurn_CdvdBurnerGrabber_TestUnitReady(pvGrabber, NULL, 0, &ulStatus, &cdbInf);

	if (EN_SUCCESS != expNum)
	{
		dlgProgress->MessageBox(_T("ERROR::GrabThread::StarBurn_CdvdBurnerGrabber_TestUnitReady"));
		StarBurn_Destroy(&pvGrabber);

		dlgProgress->SetQuit(TRUE);
		::PostMessage(hwnd, WM_CLOSE, 0, 0);
		return 1;
	}

	// установка максимальной скорости
	expNum = StarBurn_CdvdBurnerGrabber_SetSpeeds(pvGrabber, NULL, 0, &ulStatus, &cdbInf, 
		CDVD_SPEED_IS_KBPS_MAXIMUM, CDVD_SPEED_IS_KBPS_MAXIMUM);

	if (EN_SUCCESS != expNum)
	{
		dlgProgress->MessageBox(_T("ERROR::GrabThread::StarBurn_CdvdBurnerGrabber_SetSpeeds"));
		StarBurn_Destroy(&pvGrabber);

		dlgProgress->SetQuit(TRUE);
		::PostMessage(hwnd, WM_CLOSE, 0, 0);
		return 1;
	}

	// грабим треки поочередно
	CHAR cFileName[MAX_PATH];

	grabContext->dlgProgress  = dlgProgress;
	grabContext->uiCountTrack = uiCountTrack;

	for (UINT i = 0; i < uiCountTrack; i++)
	{
		grabContext->uiCurrentTrack                 = i;
		grabContext->liLastReadenPercent.QuadPart   = 0;

		// формировании наименования трека
		CString strTrackName;
		strTrackName.Format(_T("%s %02i"), _T("Track"), trackInfo[i].uiNumber);
		dlgProgress->GetDlgItem(IDC_STATICNAME)->SetWindowText(strTrackName);

		// формируем имя очередного файла
		RtlZeroMemory(&cFileName, sizeof(cFileName));
		sprintf(cFileName, "%s\\%Iu%i.wav", tcDirPath, GetTickCount(), trackInfo[i].uiNumber);
		_stprintf(trackInfo[i].tcFileName, _T("%s"), cFileName);

		// грабим
		((CProgressCtrl*)(dlgProgress->GetDlgItem(IDC_PROGRESSCURRENT)))->SetPos(0);

		expNum = StarBurn_CdvdBurnerGrabber_GrabTrack(pvGrabber, NULL, 0, &ulStatus, &cdbInf, 
			(UCHAR)(trackInfo[i].uiNumber), cFileName, NUMBER_OF_READ_RETRIES + 5, TRUE, FALSE, READ_REPORT_DELAY_IN_SECONDS);

		if (dlgProgress->GetQuit())
		{
			dlgProgress->SetQuit(TRUE);
			::PostMessage(hwnd, WM_CLOSE, 0, 0);

			return 2;
		}

		if (EN_SUCCESS != expNum)
		{
			dlgProgress->MessageBox(_T("ERROR::StarBurn_CdvdBurnerGrabber_GrabTrack"));
		}

		((CProgressCtrl*)(dlgProgress->GetDlgItem(IDC_PROGRESSCURRENT)))->SetPos(MAX_PROGRESS_CURRENT);
		((CProgressCtrl*)(dlgProgress->GetDlgItem(IDC_PROGRESSCURRENT)))->SetPos((INT)((i + 1.) * MAX_PROGRESS_TOTAL / uiCountTrack));
	}

	dlgProgress->SetQuit(TRUE);
	::PostMessage(hwnd, WM_CLOSE, 0, 0);

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// CallBack при граббинге трека
//
VOID __stdcall CGrabProgress::CallBackGrab(IN CALLBACK_NUMBER cbNumber, 
	IN PVOID pvContext, IN PVOID pvSpecial1, IN PVOID pvSpecial2)
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
			grabContext->liLastReadenPercent.QuadPart = liCurrentPercent.QuadPart;

			CProgressCtrl* progressCurrent = (CProgressCtrl*)grabContext->dlgProgress->GetDlgItem(IDC_PROGRESSCURRENT);
			progressCurrent->SetPos((INT)(MAX_PROGRESS_CURRENT / 100. * grabContext->liLastReadenPercent.QuadPart));

			CProgressCtrl* progressTotal = (CProgressCtrl*)grabContext->dlgProgress->GetDlgItem(IDC_PROGRESSTOTAL);
			progressTotal->SetPos((INT)((double)grabContext->uiCurrentTrack * MAX_PROGRESS_TOTAL / grabContext->uiCountTrack) + 
				(INT)(MAX_PROGRESS_TOTAL / 100. / grabContext->uiCountTrack * grabContext->liLastReadenPercent.QuadPart));

			break;
		}
	}
}

