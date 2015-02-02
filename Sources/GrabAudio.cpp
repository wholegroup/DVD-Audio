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
#include "GrabAudio.h"

IMPLEMENT_DYNCREATE(CGrabAudio, CDialog)

BEGIN_MESSAGE_MAP(CGrabAudio, CDialog)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BUTTONREFRESH, OnRefreshList)
	ON_BN_CLICKED(IDC_BUTTONSELECTALL, OnSelectAll)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LISTTRACK, OnListChange)
	ON_BN_CLICKED(IDC_BUTTONOPEN, OnPlayerOpen)
	ON_BN_CLICKED(IDC_BUTTONPLAY, OnPlayerPlay)
	ON_BN_CLICKED(IDC_BUTTONSTOP, OnPlayerStop)
	ON_BN_CLICKED(IDC_BUTTONNEXT, OnPlayerNext)
	ON_BN_CLICKED(IDC_BUTTONPREV, OnPlayerPrev)
	ON_BN_CLICKED(IDC_BUTTONBACK, OnPlayerBack)
	ON_BN_CLICKED(IDC_BUTTONFORWARD, OnPlayerForward)
	ON_WM_TIMER()
	ON_WM_HSCROLL()
	ON_WM_THEMECHANGED()
	ON_BN_CLICKED(IDC_BUTTONGO, OnGO)
	ON_BN_CLICKED(IDC_BUTTONCLOSE, OnClose)
END_MESSAGE_MAP()

//
#define TIMER_PERIOD 500

//
#define MAX_PROGRESS_PLAYER 10000

//
#define MAX_SLIDER_PLAYER  10000
#define PAGE_SLIDER_PLAYER MAX_SLIDER_PLAYER / 10

//////////////////////////////////////////////////////////////////////////
// конструктор и деструктор
//
CGrabAudio::CGrabAudio(CWnd* pParent /*=NULL*/)
	: CDialog(CGrabAudio::IDD, pParent)
{
	m_trackInfo = NULL;
	m_cdAudio   = NULL;
	m_pvGrabber = NULL;
	m_iTimer    = 0;
	m_hMixer    = 0;

	if (InitMixer() && m_mixerDetails.paDetails)
	{
		GetMixerValue();
		m_dwOldLeft  = m_mixerValue[0].dwValue;
		m_dwOldRight = m_mixerValue[1].dwValue;
	}
	else
	{
		m_dwOldLeft  = 0;
		m_dwOldRight = 0;
	}
}

//
CGrabAudio::~CGrabAudio()
{
	StopTimer();

	if (NULL != m_trackInfo)
	{
		delete [] m_trackInfo;
	}

	if ((NULL != m_cdAudio) && (m_cdAudio->IsPlaying()))
	{
		m_cdAudio->Stop();

		delete m_cdAudio;
	}

	if (NULL != m_pvGrabber)
	{
		StarBurn_Destroy(&m_pvGrabber);
	}

	if (m_hMixer)
	{
		if (m_mixerDetails.paDetails)
		{
			m_mixerValue[0].dwValue = m_dwOldLeft;
			m_mixerValue[1].dwValue = m_dwOldRight;

			SetMixerValue();

			delete [] m_mixerDetails.paDetails;
		}

		mixerClose(m_hMixer);
	}
}

//
void CGrabAudio::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

//
BOOL CGrabAudio::OnInitDialog()
{
	CDialog::OnInitDialog();

	CListCtrl* listTrack = (CListCtrl*)GetDlgItem(IDC_LISTTRACK);

	// добавление стиля списку треков: выделение всей строки
	DWORD dwExStyle = listTrack->GetExtendedStyle();
	dwExStyle |= LVS_EX_FULLROWSELECT;
	listTrack->SetExtendedStyle(dwExStyle);

	// создание колонок в списке треков
	listTrack->InsertColumn(0, _T("N"),      LVCFMT_CENTER, 25);
	listTrack->InsertColumn(1, _T("Start"),  LVCFMT_CENTER, 70);
	listTrack->InsertColumn(2, _T("Stop"),   LVCFMT_CENTER, 70);
	listTrack->InsertColumn(3, _T("Length"), LVCFMT_CENTER, 70);
	listTrack->InsertColumn(4, _T("Size"),   LVCFMT_CENTER, 70);

	// чтение треков
	ReadTracks();

	// установка картинок на кнопки проигрывателя
	m_btnPrev.SubclassWindow(GetDlgItem(IDC_BUTTONPREV)->GetSafeHwnd());
	m_btnPrev.SetImages(IDI_PLAYERPREV);
	m_btnPrev.SetWindowText(_T(""));

	m_btnBack.SubclassWindow(GetDlgItem(IDC_BUTTONBACK)->GetSafeHwnd());
	m_btnBack.SetImages(IDI_PLAYERBACK);
	m_btnBack.SetWindowText(_T(""));

	m_btnForward.SubclassWindow(GetDlgItem(IDC_BUTTONFORWARD)->GetSafeHwnd());
	m_btnForward.SetImages(IDI_PLAYERFORWARD);
	m_btnForward.SetWindowText(_T(""));

	m_btnNext.SubclassWindow(GetDlgItem(IDC_BUTTONNEXT)->GetSafeHwnd());
	m_btnNext.SetImages(IDI_PLAYERNEXT);
	m_btnNext.SetWindowText(_T(""));

	m_btnStop.SubclassWindow(GetDlgItem(IDC_BUTTONSTOP)->GetSafeHwnd());
	m_btnStop.SetImages(IDI_PLAYERSTOP);
	m_btnStop.SetWindowText(_T(""));

	m_btnPlay.SubclassWindow(GetDlgItem(IDC_BUTTONPLAY)->GetSafeHwnd());
	m_btnPlay.SetImages(IDI_PLAYERPLAY);
	m_btnPlay.SetWindowText(_T(""));

	m_btnOpen.SubclassWindow(GetDlgItem(IDC_BUTTONOPEN)->GetSafeHwnd());
	m_btnOpen.SetImages(IDI_PLAYEROPEN);
	m_btnOpen.SetWindowText(_T(""));

	// установка интервала прогрессбара
	CProgressCtrl* progressPosition = (CProgressCtrl*)GetDlgItem(IDC_PROGRESSPOSITION);
	progressPosition->SetRange(0, MAX_PROGRESS_PLAYER);

	// установка интервала слайдера
	CSliderCtrl* sliderPosition = (CSliderCtrl*)GetDlgItem(IDC_SLIDERPOSITION);
	sliderPosition->SetRange(0, MAX_SLIDER_PLAYER);
	sliderPosition->SetPageSize(PAGE_SLIDER_PLAYER);

	// установка интервала и значения слайдера громкости
	CSliderCtrl* sliderVolume = (CSliderCtrl*)GetDlgItem(IDC_SLIDERVOLUME);
	sliderVolume->SetRange(m_mixerControl.Bounds.dwMinimum, m_mixerControl.Bounds.dwMaximum);
	GetMixerValue();
	sliderVolume->SetPos(max(m_mixerValue[0].dwValue, m_mixerValue[1].dwValue));

	// установка инфы СД плеера
	SetInfoPlayer();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

//
void CGrabAudio::OnOK()
{	
	return;

	CDialog::OnOK();
}

//
void CGrabAudio::OnCancel()
{
	return;

	CDialog::OnCancel();
}

//////////////////////////////////////////////////////////////////////////
// Установка положения и размеров контролов диалога
//
void CGrabAudio::SetSize()
{
	CWnd* wndTmp;
	CRect rectTmp;
	CRect rectOld;
	CRect rectNew;

	// получение новых координат
	rectNew = ((CMainFrame*)GetParentFrame())->GetClientSize();

	// получение текущих координат
	GetWindowRect(&rectOld);
	GetParentFrame()->ScreenToClient(&rectOld);

	// перемещение группбокс: список треков
	wndTmp = GetDlgItem(IDC_STATICTRACKLIST);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left,
		rectTmp.top, 
		rectNew.Width() - rectOld.Width() + rectTmp.Width(),
		rectNew.Height() - rectOld.Height() + rectTmp.Height(), 0);

	// перемещение списка треков
	wndTmp = GetDlgItem(IDC_LISTTRACK);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left,
		rectTmp.top, 
		rectNew.Width() - rectOld.Width() + rectTmp.Width(),
		rectNew.Height() - rectOld.Height() + rectTmp.Height(), 0);

	// перемещение - кнопки: refresh
	wndTmp = GetDlgItem(IDC_BUTTONREFRESH);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - кнопки: select all
	wndTmp = GetDlgItem(IDC_BUTTONSELECTALL);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.Width() - rectOld.Width() + rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - статик: надпись Total
	wndTmp = GetDlgItem(IDC_STATICTOTAL);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.Width() / 2 - (rectOld.Width() / 2 - rectTmp.left), 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - статик: инфа о выбранных треках
	wndTmp = GetDlgItem(IDC_STATICSELECT);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.Width() / 2 - (rectOld.Width() / 2 - rectTmp.left), 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение группбокс: audio player controls
	wndTmp = GetDlgItem(IDC_STATICPLAYER);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left,
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectNew.Width() - rectOld.Width() + rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - кнопки: prev
	wndTmp = GetDlgItem(IDC_BUTTONPREV);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - кнопки: back
	wndTmp = GetDlgItem(IDC_BUTTONBACK);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - кнопки: forward
	wndTmp = GetDlgItem(IDC_BUTTONFORWARD);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - кнопки: next
	wndTmp = GetDlgItem(IDC_BUTTONNEXT);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - кнопки: stop
	wndTmp = GetDlgItem(IDC_BUTTONSTOP);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - кнопки: play
	wndTmp = GetDlgItem(IDC_BUTTONPLAY);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - кнопки: open
	wndTmp = GetDlgItem(IDC_BUTTONOPEN);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - статика: Position
	wndTmp = GetDlgItem(IDC_STATICPOSITION);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - статика: Volume
	wndTmp = GetDlgItem(IDC_STATICVOLUME);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - статика: Info
	wndTmp = GetDlgItem(IDC_STATICINFO);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectNew.Width() - rectOld.Width() + rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - слайдера: Position
	wndTmp = GetDlgItem(IDC_SLIDERPOSITION);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectNew.Width() - rectOld.Width() + rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - слайдера: Volume
	wndTmp = GetDlgItem(IDC_SLIDERVOLUME);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectNew.Width() - rectOld.Width() + rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - прогресс бара: Position
	wndTmp = GetDlgItem(IDC_PROGRESSPOSITION);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - кнопки: GO
	wndTmp = GetDlgItem(IDC_BUTTONGO);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.Width() - rectOld.Width() + rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - кнопки: CLOSE
	wndTmp = GetDlgItem(IDC_BUTTONCLOSE);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.Width() - rectOld.Width() + rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// установка новых размеров окна
	SetWindowPos(&wndBottom, rectNew.left, rectNew.top, rectNew.right - rectNew.left, 
		rectNew.bottom - rectNew.top, 0);

	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
// обработка WM_PAINT
//
VOID CGrabAudio::OnPaint()
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

//////////////////////////////////////////////////////////////////////////
// Чтение треков с указанного в комбобоксе драйва
//
VOID CGrabAudio::ReadTracks()
{
	CMainFrame* mainFrame; //
	DEVICEINFO  devInfo;   //
	CListCtrl*  listTrack; //
	HCURSOR     hCursor;

	hCursor = SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

	// инициализация переменных
	mainFrame = (CMainFrame*)GetParentFrame();
	devInfo   = mainFrame->GetDeviceInfo();
	listTrack = (CListCtrl*)GetDlgItem(IDC_LISTTRACK);

	// очистка списка
	listTrack->DeleteAllItems();

	// дизаблим кнопку GO
	GetDlgItem(IDC_BUTTONGO)->EnableWindow(FALSE);

	EXCEPTION_NUMBER        expNum = EN_SUCCESS; //
	ULONG                   ulStatus;            //
	CDB_FAILURE_INFORMATION cdbInf;              // 
	TOC_INFORMATION         tocInf;              // информация о дорожках на диске
	INT                     iItem;               //
	CString                 strTmp;              //
	DWORD                   dwStart;             //
	DWORD                   dwStop;              //
	DWORD                   dwMin;               //
	DWORD                   dwSec;               //
	DWORD                   dwFrame;             //
	DWORD                   dwSize;              //

	// инициализация для проигрывателя CDAudio
	if (NULL != m_cdAudio)
	{
		delete m_cdAudio;
	}
	m_cdAudio = new CCDAudio(devInfo.tcLetter);

	// открытие устройства
	if (NULL != m_pvGrabber)
	{
		StarBurn_Destroy(&m_pvGrabber);
	}

	expNum = StarBurn_CdvdBurnerGrabber_Create(&m_pvGrabber, 
		NULL, 0, &ulStatus, &cdbInf, NULL, NULL, 
		devInfo.ucPortID, devInfo.ucBusID, devInfo.ucTargetID, devInfo.ucLUN, 1);

	if (EN_SUCCESS != expNum)
	{	
		m_pvGrabber = NULL;

		SetSelectInfo();

		SetCursor(hCursor);

		return;
	}

	// вывод информации о дорожках на диске
	StarBurn_SetFastReadTOC(TRUE); 
	expNum = StarBurn_CdvdBurnerGrabber_GetTOCInformation(m_pvGrabber, 
		NULL, 0, &ulStatus, &cdbInf, &tocInf);

	if (EN_SUCCESS != expNum)
	{	
		SetSelectInfo();

		SetCursor(hCursor);

		return;
	}

	// выделение памяти для храния инфы о дорожках
	if (NULL != m_trackInfo)
	{
		delete [] m_trackInfo;
	}
	m_trackInfo = new GRABTRACKINFO[tocInf.m__UCHAR__NumberOfTracks];
	RtlZeroMemory(m_trackInfo, sizeof(GRABTRACKINFO) * tocInf.m__UCHAR__NumberOfTracks);

	// вывод инфы о каждой дорожке
	for (INT i = 0; i < tocInf.m__UCHAR__NumberOfTracks; i++)
	{
		if (!tocInf.m__TOC_ENTRY[i].m__BOOLEAN__IsAudio)
		{
			continue;
		}

		// N
		strTmp.Format(_T("%i"), i + 1);
		iItem = listTrack->InsertItem(i, strTmp);

		// start
		strTmp.Format(_T("%02u:%02u.%02u"), 
			tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[0],
			tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[1],
			tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[2]);
		listTrack->SetItemText(iItem, 1, strTmp);

		// stop
		strTmp.Format(_T("%02u:%02u.%02u"), 
			tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[0],
			tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[1],
			tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[2]);
			listTrack->SetItemText(iItem, 2, strTmp);

		dwStart = tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[0] * 60 * 100;
		dwStart += tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[1] * 100;
		dwStart += tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[2];

		dwStop = tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[0] * 60 * 100;
		dwStop += tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[1] * 100;
		dwStop += tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[2];

		// length
		dwMin   = (dwStop - dwStart) / (60 * 100);
		dwSec   = ((dwStop - dwStart) - dwMin * 60 * 100) / 100;
		dwFrame = ((dwStop - dwStart) - dwMin * 60 * 100) - dwSec * 100;

		strTmp.Format(_T("%02ld:%02ld.%02ld"), 
			dwMin,
			dwSec,
			dwFrame);
		listTrack->SetItemText(iItem, 3, strTmp);

		// size
		dwSize = (tocInf.m__TOC_ENTRY[i].m__LONG__EndingLBA - tocInf.m__TOC_ENTRY[i].m__LONG__StartingLBA);
		dwSize *= tocInf.m__TOC_ENTRY[i].m__ULONG__LBSizeInUCHARs;

		strTmp.Format(_T("%d %03d KB"), (dwSize / (1024 * 1000)), (dwSize - (dwSize / (1024 * 1000)) * (1024 * 1000)) / 1024);
		listTrack->SetItemText(iItem, 4, strTmp);

		// сохранение нужной информации
		m_trackInfo[i].ucStartingMSF[0] = tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[0];
		m_trackInfo[i].ucStartingMSF[1] = tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[1];
		m_trackInfo[i].ucStartingMSF[2] = tocInf.m__TOC_ENTRY[i].m__UCHAR__StartingMSF[2];
		m_trackInfo[i].ucEndingMSF[0]   = tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[0];
		m_trackInfo[i].ucEndingMSF[1]   = tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[1];
		m_trackInfo[i].ucEndingMSF[2]   = tocInf.m__TOC_ENTRY[i].m__UCHAR__EndingMSF[2];
		m_trackInfo[i].uiLengthMSF[0]   = dwMin;
		m_trackInfo[i].uiLengthMSF[1]   = dwSec;
		m_trackInfo[i].uiLengthMSF[2]   = dwFrame;
		m_trackInfo[i].dwSize           = dwSize;
		m_trackInfo[i].uiNumber         = i + 1;
	}

	OnSelectAll();
	SetSelectInfo();
}

//////////////////////////////////////////////////////////////////////////
// Refresh списка треков
//
VOID CGrabAudio::OnRefreshList()
{
	ReadTracks();
}

//////////////////////////////////////////////////////////////////////////
// Выделить все треки
// 
VOID CGrabAudio::OnSelectAll()
{
	CListCtrl* listTrack = (CListCtrl*)GetDlgItem(IDC_LISTTRACK);

	for (INT i = 0; i < listTrack->GetItemCount(); i++)
	{
		listTrack->SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	}
}

//////////////////////////////////////////////////////////////////////////
// Вывод суммарной информации о выделенных треках
//
VOID CGrabAudio::SetSelectInfo()
{
	CStatic*   selectInfo = (CStatic*)GetDlgItem(IDC_STATICSELECT);
	CListCtrl* listTrack = (CListCtrl*)GetDlgItem(IDC_LISTTRACK);
	CString    strTmp;
	POSITION   pos;
	DWORD      dwLength;
	DWORD      dwSize;
	INT        i;
	
	// если ни один трек не выбран
	pos = listTrack->GetFirstSelectedItemPosition();
	if (NULL == pos)
	{
		selectInfo->SetWindowText(_T("No tracks selected"));

		GetDlgItem(IDC_BUTTONGO)->EnableWindow(FALSE);

		return;
	}

	// поиск выделенных треков
	m_iItemSelected = 0;
	dwLength      = 0;
	dwSize        = 0;
	while (pos)
	{
		m_iItemSelected++;

		i = listTrack->GetNextSelectedItem(pos);

		dwLength += (m_trackInfo[i].uiLengthMSF[0] * 60 + m_trackInfo[i].uiLengthMSF[1]) * 100 + 
			m_trackInfo[i].uiLengthMSF[2];
		dwSize   += m_trackInfo[i].dwSize;
	}

	// вывод информации о выделенных треках
	if (m_iItemSelected)
	{
		GetDlgItem(IDC_BUTTONGO)->EnableWindow(TRUE);

		if (1 == m_iItemSelected)
		{
			strTmp.Format(_T("%i Track, %02ld:%02ld.%02ld (%d %03d KB)"), 
				m_iItemSelected,
				dwLength / 60000, 
				(dwLength - (dwLength / 6000) * 6000) / 100, 
				(dwLength - (dwLength / 6000) * 6000) - ((dwLength - (dwLength / 6000) * 6000) / 100) * 100,
				(dwSize / (1024 * 1000)), 
				(dwSize - (dwSize / (1024 * 1024)) * 1024 * 1024) / 1024);
		}
		else
		{
			strTmp.Format(_T("%i Tracks, %02ld:%02ld.%02ld (%d %03d KB)"), 
				m_iItemSelected,
				dwLength / 6000, 
				(dwLength - (dwLength / 6000) * 6000) / 100, 
				(dwLength - (dwLength / 6000) * 6000) - ((dwLength - (dwLength / 6000) * 6000) / 100) * 100,
				(dwSize / (1024 * 1000)), 
				(dwSize - (dwSize / (1024 * 1000)) * 1000 * 1024) / 1024);
		}

		selectInfo->SetWindowText(strTmp);
	}
	else
	{
		selectInfo->SetWindowText(_T("No tracks selected"));
	}
}

//////////////////////////////////////////////////////////////////////////
// Обработка изменения состояния списка треков
//
void CGrabAudio::OnListChange(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	SetSelectInfo();

	*pResult = 0;
}

//////////////////////////////////////////////////////////////////////////
// Открытие/закрытие лотка CDRom'а
//
void CGrabAudio::OnPlayerOpen()
{
	if (NULL != m_cdAudio)
	{
		m_cdAudio->EjectCDROM();

		ReadTracks();
	}
}

//////////////////////////////////////////////////////////////////////////
// Запуск проигрывания
//
void CGrabAudio::OnPlayerPlay()
{
	if (NULL != m_cdAudio)
	{
		CListCtrl* listTrack = (CListCtrl*)GetDlgItem(IDC_LISTTRACK);

		// если ни один трек не выбран
		POSITION pos = listTrack->GetFirstSelectedItemPosition();
		if (NULL == pos)
		{
			m_iTrackPlay = 0;
			m_cdAudio->Play();

			StartTimer();

			return;
		}

		m_iTrackPlay = listTrack->GetNextSelectedItem(pos);

		m_cdAudio->Play((m_trackInfo[m_iTrackPlay].ucStartingMSF[0] * 60 + 
			m_trackInfo[m_iTrackPlay].ucStartingMSF[1]) * 100 + m_trackInfo[m_iTrackPlay].ucStartingMSF[2]);

		StartTimer();
	}
}

//////////////////////////////////////////////////////////////////////////
// Остановка проигрывания
//
void CGrabAudio::OnPlayerStop()
{
	if ((NULL != m_cdAudio) && (m_cdAudio->IsPlaying()))
	{
		m_cdAudio->Stop();

		StopTimer();
	}	
}

//////////////////////////////////////////////////////////////////////////
// переключение на следующий трек
//
void CGrabAudio::OnPlayerNext()
{
	if ((NULL != m_cdAudio) && (m_cdAudio->IsPlaying()))
	{
		CListCtrl* listTrack = (CListCtrl*)GetDlgItem(IDC_LISTTRACK);
		
		if (m_iTrackPlay < (listTrack->GetItemCount() - 1))
		{
			m_iTrackPlay++;

			m_cdAudio->Play((m_trackInfo[m_iTrackPlay].ucStartingMSF[0] * 60 + 
				m_trackInfo[m_iTrackPlay].ucStartingMSF[1]) * 100 + m_trackInfo[m_iTrackPlay].ucStartingMSF[2]);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// переключение на предыдущий трек
//
void CGrabAudio::OnPlayerPrev()
{
	if ((NULL != m_cdAudio) && (m_cdAudio->IsPlaying()))
	{
		CListCtrl* listTrack = (CListCtrl*)GetDlgItem(IDC_LISTTRACK);

		if (0 < m_iTrackPlay)
		{
			m_iTrackPlay--;

			m_cdAudio->Play((m_trackInfo[m_iTrackPlay].ucStartingMSF[0] * 60 + 
				m_trackInfo[m_iTrackPlay].ucStartingMSF[1]) * 100 + m_trackInfo[m_iTrackPlay].ucStartingMSF[2]);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// отмотать трек назад на 20 секунд
//
void CGrabAudio::OnPlayerBack()
{
	if ((NULL != m_cdAudio) && (m_cdAudio->IsPlaying()))
	{
		INT iPos   = m_cdAudio->GetCurrentPos() * 100 - 2000;
		INT iStart = (m_trackInfo[m_iTrackPlay].ucStartingMSF[0] * 60 + 
			m_trackInfo[m_iTrackPlay].ucStartingMSF[1]) * 100 + m_trackInfo[m_iTrackPlay].ucStartingMSF[2];

		if (iPos< iStart)
		{
			iPos = iStart;
		}

		m_cdAudio->Play(iPos);
	}
}

//////////////////////////////////////////////////////////////////////////
// промотать трек вперед на 20 сек
//
void CGrabAudio::OnPlayerForward()
{
	if ((NULL != m_cdAudio) && (m_cdAudio->IsPlaying()))
	{
		INT iPos   = m_cdAudio->GetCurrentPos() * 100 + 2000;
		INT iStop  = (m_trackInfo[m_iTrackPlay].ucEndingMSF[0] * 60 + 
			m_trackInfo[m_iTrackPlay].ucEndingMSF[1]) * 100 + m_trackInfo[m_iTrackPlay].ucEndingMSF[2];

		if (iPos > iStop)
		{
			iPos = iStop;
		}

		m_cdAudio->Play(iPos);
	}
}

//////////////////////////////////////////////////////////////////////////
// Обработка таймера
//
void CGrabAudio::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == m_iTimer)
	{
		SetInfoPlayer();
	}

	CDialog::OnTimer(nIDEvent);
}

//////////////////////////////////////////////////////////////////////////
// Запуск таймера
//
VOID CGrabAudio::StartTimer()
{
	if (0 == m_iTimer)
	{
		m_iTimer = SetTimer(1, TIMER_PERIOD, 0);
	}
}

//////////////////////////////////////////////////////////////////////////
// Остановка таймера
//
VOID CGrabAudio::StopTimer()
{
	if (0 != m_iTimer)
	{
		KillTimer(m_iTimer);

		m_iTimer = 0;

		SetInfoPlayer();
	}
}

//////////////////////////////////////////////////////////////////////////
// Вывод и установка значений CD плеера
// 
VOID CGrabAudio::SetInfoPlayer()
{
	CSliderCtrl*   sliderPosition   = (CSliderCtrl*)GetDlgItem(IDC_SLIDERPOSITION);
	CProgressCtrl* progressPosition = (CProgressCtrl*)GetDlgItem(IDC_PROGRESSPOSITION);
	CStatic*       staticInfo       = (CStatic*)GetDlgItem(IDC_STATICINFO);

	if ((NULL != m_cdAudio) && (m_cdAudio->IsPlaying()))
	{
		INT iPos   = m_cdAudio->GetCurrentPos() * 100;
		INT iStop  = (m_trackInfo[m_iTrackPlay].ucEndingMSF[0] * 60 + 
			m_trackInfo[m_iTrackPlay].ucEndingMSF[1]) * 100 + m_trackInfo[m_iTrackPlay].ucEndingMSF[2];
		INT iStart = (m_trackInfo[m_iTrackPlay].ucStartingMSF[0] * 60 + 
			m_trackInfo[m_iTrackPlay].ucStartingMSF[1]) * 100 + m_trackInfo[m_iTrackPlay].ucStartingMSF[2];

		if (iPos >= iStop)
		{
			OnPlayerStop();

			staticInfo->SetWindowText(CString(_T("stopped")));
			progressPosition->SetPos(MAX_PROGRESS_PLAYER);
			sliderPosition->SetPos(MAX_SLIDER_PLAYER);
		}
		else
		{
			CString strTmp;
			strTmp.Format(_T("Track %02i Time %02i:%02i (%02i:%02i)"), m_iTrackPlay + 1, 
				(iPos - iStart) / 6000,
				((iPos - iStart) % 6000) / 100,
				iPos / 6000,
				(iPos % 6000) / 100
				);
			staticInfo->SetWindowText(strTmp);

			iPos  -= iStart;
			iStop -= iStart;
			iStart = 0;

			progressPosition->SetPos((INT)((double)MAX_PROGRESS_PLAYER / iStop * iPos));
			sliderPosition->SetPos((INT)((double)MAX_PROGRESS_PLAYER / iStop * iPos));
		}
	}
	else
	{
		staticInfo->SetWindowText(CString(_T("stopped")));
		progressPosition->SetPos(MAX_PROGRESS_PLAYER);
		sliderPosition->SetPos(MAX_SLIDER_PLAYER);
	}
}

//////////////////////////////////////////////////////////////////////////
// Обработка слайдеров
//
void CGrabAudio::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl* sliderPosition = (CSliderCtrl*)GetDlgItem(IDC_SLIDERPOSITION);
	CSliderCtrl* sliderVolume = (CSliderCtrl*)GetDlgItem(IDC_SLIDERVOLUME);

	if ((CWnd*)pScrollBar == (CWnd*)sliderPosition)
	{
		INT iCurPos = sliderPosition->GetPos();
		INT iMinPos;
		INT iMaxPos;

		sliderPosition->GetRange(iMinPos, iMaxPos);

		if (((SB_THUMBTRACK == nSBCode) || (SB_PAGELEFT == nSBCode) || (SB_PAGERIGHT == nSBCode)) && 
			((NULL != m_cdAudio) && (m_cdAudio->IsPlaying())))
		{
			INT iPos   = 0;
			INT iStart = (m_trackInfo[m_iTrackPlay].ucStartingMSF[0] * 60 + 
				m_trackInfo[m_iTrackPlay].ucStartingMSF[1]) * 100 + m_trackInfo[m_iTrackPlay].ucStartingMSF[2];
			INT iStop  = (m_trackInfo[m_iTrackPlay].ucEndingMSF[0] * 60 + 
				m_trackInfo[m_iTrackPlay].ucEndingMSF[1]) * 100 + m_trackInfo[m_iTrackPlay].ucEndingMSF[2];

			iPos = (INT)(iStart + (double)(iStop - iStart) / (double)(iMaxPos - iMinPos) * (double)(iCurPos - iMinPos));

			if (iPos > iStop)
			{
				iPos = iStop;
			}

			m_cdAudio->Play(iPos);
		}
		else
		{
			iCurPos = iMaxPos;
		}
	}
	else

	if ((CWnd*)pScrollBar == (CWnd*)sliderVolume)
	{
		INT iCurPos = sliderVolume->GetPos();

		m_mixerValue[0].dwValue = iCurPos;
		m_mixerValue[1].dwValue = iCurPos;

		SetMixerValue();
	}

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

//////////////////////////////////////////////////////////////////////////
// Инициализация миксера
//
BOOL CGrabAudio::InitMixer()
{
	if (m_hMixer)
	{
		mixerClose(m_hMixer);
		m_hMixer = NULL;
	}

	UINT uiMixerNum = mixerGetNumDevs();

	if (MMSYSERR_NOERROR != mixerOpen(&m_hMixer, 0, 0, 0, 0))
	{
		return FALSE;
	}

	MIXERLINE mixerLine;
	MIXERCAPS mixerCaps;
	INT       iDestin = -1;
	INT       iSource = -1;

	if (MMSYSERR_NOERROR != mixerGetDevCaps((UINT_PTR)m_hMixer, &mixerCaps, sizeof(MIXERCAPS)))
	{
		mixerClose(m_hMixer);
		m_hMixer = NULL;

		return FALSE;
	}

	for (INT i = 0; i < (INT)mixerCaps.cDestinations; i++)
	{
		mixerLine.cbStruct      = sizeof(MIXERLINE);
		mixerLine.dwSource      = 0;
		mixerLine.dwDestination = i;

		if (MMSYSERR_NOERROR != mixerGetLineInfo((HMIXEROBJ)m_hMixer, &mixerLine, MIXER_GETLINEINFOF_DESTINATION))
		{
			mixerClose(m_hMixer);
			m_hMixer = NULL;

			return FALSE;
		}

		if (MIXERLINE_COMPONENTTYPE_DST_SPEAKERS == mixerLine.dwComponentType)
		{
			iDestin = i;

			break;
		}
	}

	if (-1 == iDestin)
	{
		mixerClose(m_hMixer);
		m_hMixer = NULL;

		return FALSE;
	}

	DWORD dwDstIndex = mixerLine.dwDestination;
	INT   iConn      = (INT)mixerLine.cConnections;
	for (INT i = 0; i < iConn; i++)
	{
		mixerLine.cbStruct      = sizeof(MIXERLINE);
		mixerLine.dwSource      = i;
		mixerLine.dwDestination = dwDstIndex;

		if (MMSYSERR_NOERROR != mixerGetLineInfo((HMIXEROBJ)m_hMixer, &mixerLine, MIXER_GETLINEINFOF_SOURCE))
		{
			mixerClose(m_hMixer);
			m_hMixer = NULL;

			return FALSE;
		}

		if (MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC == mixerLine.dwComponentType)
		{
			iSource = i;

			break;
		}
	}

	if (-1 == iSource)
	{
		mixerClose(m_hMixer);
		m_hMixer = NULL;

		return FALSE;
	}

	MIXERLINECONTROLS mixerLineControl;

	mixerLineControl.cbStruct       = sizeof(MIXERLINECONTROLS);
	mixerLineControl.dwLineID       = mixerLine.dwLineID;
	mixerLineControl.dwControlType  = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mixerLineControl.cControls      = 1;
	mixerLineControl.cbmxctrl       = sizeof(MIXERCONTROL);
	mixerLineControl.pamxctrl       = &m_mixerControl;

	if (MMSYSERR_NOERROR != mixerGetLineControls((HMIXEROBJ)m_hMixer, &mixerLineControl, MIXER_GETLINECONTROLSF_ONEBYTYPE ))
	{
		mixerClose(m_hMixer);
		m_hMixer = NULL;

		return FALSE;
	}

	m_mixerDetails.cbStruct       = sizeof(MIXERCONTROLDETAILS);
	m_mixerDetails.dwControlID    = m_mixerControl.dwControlID;
	m_mixerDetails.cMultipleItems = m_mixerControl.cMultipleItems;
	m_mixerDetails.cChannels      = mixerLine.cChannels;
	m_mixerDetails.cbDetails      = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
	m_mixerDetails.paDetails      = new CHAR[m_mixerDetails.cChannels * m_mixerDetails.cbDetails];

	m_mixerValue = (MIXERCONTROLDETAILS_UNSIGNED*)m_mixerDetails.paDetails;

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// Получить значения миксера
//
VOID CGrabAudio::GetMixerValue()
{
	if (!m_hMixer)
	{
		return;
	}

	if (MMSYSERR_NOERROR != mixerGetControlDetails((HMIXEROBJ)m_hMixer, 
		&m_mixerDetails, MIXER_GETCONTROLDETAILSF_VALUE))
	{
		mixerClose(m_hMixer);
		m_hMixer = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
// Установить значения миксера
//
VOID CGrabAudio::SetMixerValue()
{
	if (!m_hMixer)
	{
		return;
	}

	if (MMSYSERR_NOERROR != mixerSetControlDetails((HMIXEROBJ)m_hMixer, 
		&m_mixerDetails, MIXER_SETCONTROLDETAILSF_VALUE))
	{
		mixerClose(m_hMixer);
		m_hMixer = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
// Обработка смены темы - остановка проигрывания
//
LRESULT CGrabAudio::OnThemeChanged()
{
	OnPlayerStop();

	return 1;
}

//////////////////////////////////////////////////////////////////////////
// Запуск грабинга
//
void CGrabAudio::OnGO()
{
	// если ни один трек не выбран
	CListCtrl* listTrack = (CListCtrl*)GetDlgItem(IDC_LISTTRACK);
	POSITION   pos;

	pos = listTrack->GetFirstSelectedItemPosition();
	if (NULL == pos)
	{
		return;
	}

	// останавливаем плеер
	OnPlayerStop();

	// диалог выбора скорости
	CDialog dlgTmp(IDD_SETREADSPEED);
	if (IDOK != dlgTmp.DoModal())
	{
		return;
	}

	// диалог процесса граббинга
	CMainFrame*   mainFrame;
	DEVICEINFO    devInfo;
	CGrabProgress dlgProgress(this);

	// установка инфы о выбранном устройстве, где будет происходить граббинг
	mainFrame = (CMainFrame*)GetParentFrame();
	devInfo   = mainFrame->GetDeviceInfo();

	dlgProgress.SetDeviceInfo(&devInfo);

	// установка инфы о треках для граббинга
	GRABTRACKINFO* grabTrackInfo;
	UINT           uiCountGrabTrack;

	uiCountGrabTrack = m_iItemSelected;
	grabTrackInfo    = new GRABTRACKINFO[uiCountGrabTrack];

	for (INT i = 0; pos; i++)
	{
		INT j = listTrack->GetNextSelectedItem(pos);
		grabTrackInfo[i] = m_trackInfo[j];
	}

	dlgProgress.SetTrackInfo(grabTrackInfo, uiCountGrabTrack);
	dlgProgress.SetDirPath(mainFrame->GetDirPath());

	if (IDOK == dlgProgress.DoModal())
	{
		TRACKINFO*** trackInfo    = mainFrame->GetTrackInfo();
		UINT         uiCountGroup = mainFrame->GetCountGroup();
		UINT*        uiCountTrack = mainFrame->GetCountTrack();

		if (MAX_GROUP <= uiCountGroup)
		{
			MessageBox(_T("It is impossible to add a new group. ")
				_T("The maximal amount of groups is already created."), NULL, MB_ICONERROR);
		}
		else
		{
			if ((0 == uiCountGrabTrack) || (uiCountGrabTrack > MAX_TRACK))
			{
				MessageBox(_T("Not correct amount of tracks."), NULL, MB_ICONERROR);
			}
			else
			{
				uiCountGroup++;

				trackInfo[uiCountGroup - 1] = new TRACKINFO* [MAX_TRACK];

				TRACKINFO* newTrackInfo;
				for (UINT i = 0; i < uiCountGrabTrack; i++)
				{
					newTrackInfo = new TRACKINFO;

					newTrackInfo->uiNumber = grabTrackInfo[i].uiNumber;
					RtlCopyMemory(newTrackInfo->tcFileName, grabTrackInfo[i].tcFileName, sizeof(newTrackInfo->tcFileName));
					newTrackInfo->dwSize   = grabTrackInfo[i].dwSize;

					uiCountTrack[uiCountGroup - 1]++;

					trackInfo[uiCountGroup - 1][uiCountTrack[uiCountGroup - 1] - 1] = newTrackInfo;
				}

				mainFrame->SetCountGroup(uiCountGroup);

				mainFrame->PostMessage(WM_GRABAUDIOCD_CLOSE);
			}
		}
	}
	else
	{
		for (UINT i = 0; i < uiCountGrabTrack; i++)
		{
			DeleteFile(grabTrackInfo[i].tcFileName);
		}
	}

	delete [] grabTrackInfo;
}

//////////////////////////////////////////////////////////////////////////
//
//
void CGrabAudio::OnClose()
{
	GetParentFrame()->PostMessage(WM_GRABAUDIOCD_CLOSE);
}

