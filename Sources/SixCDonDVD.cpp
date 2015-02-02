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
#include "sixcdondvd.h"

IMPLEMENT_DYNCREATE(CSixCDonDVD, CDialog)

BEGIN_MESSAGE_MAP(CSixCDonDVD, CDialog)
	ON_WM_PAINT()
	ON_CBN_SELCHANGE(IDC_FORMATDVD, OnCbnSelchangeFormatdvd)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREEGROUPS, OnTvnSelchangedTreegroups)
	ON_BN_CLICKED(IDC_BUTTONGRABCDA, GrabAudioCD)
	ON_BN_CLICKED(IDC_BUTTONBURN, BurnDVDAudio)
END_MESSAGE_MAP()

#define ONE_STICK_SIZE 250  // размер одного деления в Мб
#define DVD5_SIZE      4483 // размер однослойного ДВД в Мб
#define DVD9_SIZE      8152 // размер двухслойного ДВД в Мб
#define BLUE_START     0xB0 // 
#define BLUE_STOP      0xE0 //
#define RED_START      0xE0 //
#define RED_STOP       0xB0 //

//
CSixCDonDVD::CSixCDonDVD()
	: CDialog(CSixCDonDVD::IDD)
{
}

//
CSixCDonDVD::~CSixCDonDVD()
{
}

//
void CSixCDonDVD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREEGROUPS, m_treeGroups);
	DDX_Control(pDX, IDC_TREETRACKS, m_treeTracks);
}

//////////////////////////////////////////////////////////////////////////
// Инициализация диалога
//
BOOL CSixCDonDVD::OnInitDialog()
{
	CDialog::OnInitDialog();

	// инициализация переменных
	m_uiSizeData = 0;

	// очистка индикатора размера
	GetDlgItem(IDC_INDICATORSIZE)->SetWindowText(_T(""));

	// устанавливаем значение комбобокса - размер ДВД
	CString strTmp;
	m_uiFormatDVD = 0;
	CComboBox* wndFormatDVD = (CComboBox*)GetDlgItem(IDC_FORMATDVD);
	wndFormatDVD->ResetContent();
	strTmp.Format(_T("DVD5 (%iMB)"), DVD5_SIZE);
	wndFormatDVD->AddString(strTmp);
	strTmp.Format(_T("DVD9 (%iMB)"), DVD9_SIZE);
	wndFormatDVD->AddString(strTmp);
	wndFormatDVD->SetCurSel(m_uiFormatDVD);

	// создание списка изображений для дерева дисков
	HICON iconTmp;

	m_imgListGroups.Create(24, 24, ILC_COLOR32 | ILC_MASK, 0, 3);

	iconTmp = AfxGetApp()->LoadIcon(IDI_ICONCD);
	m_imgListGroups.Add(iconTmp);	

	iconTmp = AfxGetApp()->LoadIcon(IDI_ICONDVDR);
	m_imgListGroups.Add(iconTmp);	

	m_treeGroups.SetImageList(&m_imgListGroups, LVSIL_NORMAL);

	// создание списка изображений для дерева треков
	m_imgListTracks.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 3);

	iconTmp = AfxGetApp()->LoadIcon(IDI_ICONMUSIC);
	m_imgListTracks.Add(iconTmp);	

	iconTmp = AfxGetApp()->LoadIcon(IDI_ICONCD);
	m_imgListTracks.Add(iconTmp);	

	m_treeTracks.SetImageList(&m_imgListTracks, LVSIL_NORMAL);

	SetTreeData();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// Изменение размеров диалога и положения его элементов
//
VOID CSixCDonDVD::SetSize()
{
	CWnd  *wndTmp, *wndGroups, *wndTracks;
	CRect rectTmp, rectGroups, rectTracks;
	CRect rectOld;
	CRect rectNew = ((CMainFrame*)GetParentFrame())->GetClientSize();

	// получение текущих координат
	GetWindowRect(&rectOld);
	GetParentFrame()->ScreenToClient(&rectOld);

	// перемещение - статик: индикатор места на диске
	wndTmp = GetDlgItem(IDC_INDICATORSIZE);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectTmp.left,
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectNew.Width() - rectOld.Width() + rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - комбобокс: выбор размера DVD
	wndTmp = GetDlgItem(IDC_FORMATDVD);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.Width() - rectOld.Width() + rectTmp.left, 
		rectNew.Height() - rectOld.Height() + rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение - группбоксов: список групп и список треков
	wndGroups = GetDlgItem(IDC_FRAMEGROUPS);
	wndGroups->GetWindowRect(&rectGroups);
	ScreenToClient(&rectGroups);

	wndTracks = GetDlgItem(IDC_FRAMETRACKS);
	wndTracks->GetWindowRect(&rectTracks);
	ScreenToClient(&rectTracks);

	UINT uiWidthOld, uiWidthNew, uiStep;
	uiWidthOld = rectTracks.right - rectGroups.left;
	uiWidthNew = rectNew.Width() - rectGroups.left - (rectOld.Width() - rectTracks.right);
	uiStep     = (rectTracks.left - rectGroups.right) / 2;

	wndGroups->SetWindowPos(&wndBottom, 
		rectGroups.left, 
		rectGroups.top, 
		uiWidthNew / 2 - uiStep,
		rectNew.Height() - rectOld.Height() + rectGroups.Height(), 0);

	wndTracks->SetWindowPos(&wndBottom, 
		rectGroups.left + uiWidthNew / 2 + uiStep, 
		rectTracks.top, 
		rectNew.Width() - (rectGroups.left + uiWidthNew / 2 + uiStep) - (rectOld.Width() - rectTracks.right),
		rectNew.Height() - rectOld.Height() + rectTracks.Height(), 0);

	// перемещение TreeView: группы
	wndTmp = GetDlgItem(IDC_TREEGROUPS);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);

	wndTmp->SetWindowPos(&wndBottom,
		rectTmp.left,
		rectTmp.top,
		uiWidthNew / 2 - uiStep - (rectGroups.Width() - rectTmp.Width()),
		rectNew.Height() - rectOld.Height() + rectGroups.Height() - (rectGroups.Height() - rectTmp.Height()),
		0);

	// перемещение TreeView: треки
	wndTmp = GetDlgItem(IDC_TREETRACKS);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);

	wndTmp->SetWindowPos(&wndBottom, 
		rectGroups.left + uiWidthNew / 2 + uiStep + (rectTmp.left - rectTracks.left), 
		rectTmp.top, 
		rectNew.Width() - (rectGroups.left + uiWidthNew / 2 + uiStep) - (rectOld.Width() - rectTracks.right) - (rectTracks.Width() - rectTmp.Width()),
		rectNew.Height() - rectOld.Height() + rectTracks.Height() - (rectTracks.Height() - rectTmp.Height()),
		0);

	// перемещение группбокса: шаг первый
	wndTmp = GetDlgItem(IDC_FRAMESTEP1);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.right - rectTmp.Width() - (rectOld.right - rectTmp.right),
		rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение статика: инфа о первом шаге
	wndTmp = GetDlgItem(IDC_STATICSTEPONE);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.right - rectTmp.Width() - (rectOld.right - rectTmp.right),
		rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение кнопки: грабит аудио-кд
	wndTmp = GetDlgItem(IDC_BUTTONGRABCDA);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.right - rectTmp.Width() - (rectOld.right - rectTmp.right),
		rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение группбокса: шаг второй
	wndTmp = GetDlgItem(IDC_FRAMESTEP2);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.right - rectTmp.Width() - (rectOld.right - rectTmp.right),
		rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение статика: инфа о втором шаге
	wndTmp = GetDlgItem(IDC_STATICSTEPTWO);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.right - rectTmp.Width() - (rectOld.right - rectTmp.right),
		rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// перемещение кнопки: прожиг ДВД
	wndTmp = GetDlgItem(IDC_BUTTONBURN);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	wndTmp->SetWindowPos(&wndBottom, 
		rectNew.right - rectTmp.Width() - (rectOld.right - rectTmp.right),
		rectTmp.top, 
		rectTmp.Width(),
		rectTmp.Height(), 0);

	// установка новых размеров окна
	SetWindowPos(&wndBottom, rectNew.left, rectNew.top, rectNew.right - rectNew.left, 
		rectNew.bottom - rectNew.top, 0);

	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
// Перекрытие нажатия "Отмена", чтобы диалог не закрывался
// 
VOID CSixCDonDVD::OnCancel()
{
	return;

	CDialog::OnCancel();
}

//////////////////////////////////////////////////////////////////////////
// Перекрытие нажатия "ОК", чтобы диалог не закрывался
// 
VOID CSixCDonDVD::OnOK()
{
	return;

	CDialog::OnOK();
}

//////////////////////////////////////////////////////////////////////////
// обработка WM_PAINT
//
VOID CSixCDonDVD::OnPaint()
{
	CPaintDC dc(this);

	// индикатор места
	PaintIndicatorSize(dc);

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
// Рисование индикатора места
// 
VOID CSixCDonDVD::PaintIndicatorSize(CPaintDC& dc)
{
	CPen penTmp;

	// получаем координаты области для вывода индикатора
	CWnd*    wndTmp;
	CRect    rectTmp;

	wndTmp = GetDlgItem(IDC_INDICATORSIZE);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);
	rectTmp.left   += 2;
	rectTmp.right  -= 2;
	rectTmp.top    += 1;
	rectTmp.bottom -= 2;

	CPaintDC dcI(wndTmp);

	// расчет количества палок - 1 палка 250Mb
	UINT uiN250 = 0;
	if (0 == m_uiFormatDVD)
	{
		uiN250 = DVD5_SIZE / ONE_STICK_SIZE + 2;
	}
	else
	if (1 == m_uiFormatDVD)
	{
		uiN250 = DVD9_SIZE / ONE_STICK_SIZE + 2;
	}

	// смещение каждой палки в пикселах
	UINT uiStep = 0;
	uiStep = rectTmp.Width() / uiN250;

	// высота палки
	UINT uiHeight = rectTmp.Height() / 2;

	// установка нового шрифта
	CFont fontNew;
	CFont* pfontOld;

	fontNew.CreateFont(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Arial"));

	pfontOld = dc.SelectObject(&fontNew);

	// максимальная длина строки с цифрами
	CString strTmp = _T("0000MB");
	CSize   sizeStr = dc.GetTextExtent(strTmp);

	// вывод палок
	INT uiDiff = -sizeStr.cx * 2;
	for (UINT i = 0; ((uiStep * i) < (UINT)rectTmp.Width()); i++)
	{
		BOOL bViewNumber = (uiStep * i - uiDiff) > (sizeStr.cx * 1.5);

		// черная полоска
		penTmp.DeleteObject();
		penTmp.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
		dc.SelectObject(&penTmp);

		dc.MoveTo(rectTmp.left + uiStep * i, rectTmp.top);
		dc.LineTo(rectTmp.left + uiStep * i, rectTmp.top + (bViewNumber ? uiHeight : uiHeight / 2));

		// белая полоска
		penTmp.DeleteObject();
		penTmp.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
		dc.SelectObject(&penTmp);

		dc.MoveTo(rectTmp.left + uiStep * i - 1, rectTmp.top);
		dc.LineTo(rectTmp.left + uiStep * i - 1, rectTmp.top + (bViewNumber ? uiHeight : uiHeight / 2) - 1);

		// вывод числа - Мб
		if (bViewNumber && (((int)uiStep * (int)i - sizeStr.cx / 2) > 0) && 
			((uiStep * i + sizeStr.cx / 2) < (UINT)rectTmp.Width()))
		{
			CString strTmp;
			CSize   sizeTmp;

			strTmp.Format(_T("%iMB"), i * ONE_STICK_SIZE);
			sizeTmp = dc.GetTextExtent(strTmp);

			// вывод текста
			dc.TextOut(rectTmp.left + uiStep * i - sizeTmp.cx / 2, rectTmp.top + uiHeight, strTmp);
		}

		if (bViewNumber)
		{
			uiDiff = rectTmp.left + uiStep * i;
		}
	}

	// установка старого шрифта 
	dc.SelectObject(pfontOld);

	// вывод красной-желтой ограничивающей полоски
	UINT uiMaxSize;

	if (0 == m_uiFormatDVD)
	{
		uiMaxSize = (UINT)((float)DVD5_SIZE / ONE_STICK_SIZE * uiStep);
	}
	else
	if (1 == m_uiFormatDVD)
	{
		uiMaxSize = (UINT)((float)DVD9_SIZE / ONE_STICK_SIZE * uiStep);
	}

	penTmp.DeleteObject();
	penTmp.CreatePen(PS_SOLID, 1, RGB(0xFF, 0x00, 0x00));
	dc.SelectObject(&penTmp);

	dc.MoveTo(uiMaxSize, rectTmp.top);
	dc.LineTo(uiMaxSize, rectTmp.bottom);

	penTmp.DeleteObject();
	penTmp.CreatePen(PS_SOLID, 1, RGB(0xFF, 0xFF, 0x00));
	dc.SelectObject(&penTmp);

	dc.MoveTo(uiMaxSize - 1, rectTmp.top);
	dc.LineTo(uiMaxSize - 1, rectTmp.bottom);

	// вывод занятого размера
	if (m_uiSizeData)
	{
		UINT uiSize;

		if (0 == m_uiFormatDVD)
		{
			uiSize = (UINT)((float)m_uiSizeData / ONE_STICK_SIZE * uiStep);
		}
		else
		if (1 == m_uiFormatDVD)
		{
			uiSize = (UINT)((float)m_uiSizeData / ONE_STICK_SIZE * uiStep);
		}

		if (uiSize > (UINT)(rectTmp.Width() - 3))
		{
			uiSize = (rectTmp.Width() - 3);
		}

		penTmp.DeleteObject();
		penTmp.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
		dc.SelectObject(&penTmp);

		dc.MoveTo(rectTmp.left, rectTmp.top + 2);
		dc.LineTo(rectTmp.left + uiSize + 1, rectTmp.top + 2);

		for (UINT i = 0; i <= uiSize; i++)
		{
			if (i < (uiMaxSize - rectTmp.left))
			{
				penTmp.DeleteObject();
				penTmp.CreatePen(PS_SOLID, 1, RGB(0x00, 0x00, BLUE_START + (BLUE_STOP - BLUE_START) * ((float)i / uiSize)));
				dc.SelectObject(&penTmp);
			}
			else
			{
				penTmp.DeleteObject();
				penTmp.CreatePen(PS_SOLID, 1, RGB(RED_START + (RED_STOP - RED_START) * 
					((float)(i - (uiMaxSize - rectTmp.left)) / (uiSize - (uiMaxSize - rectTmp.left))), 0x00, 0x00));
				dc.SelectObject(&penTmp);
			}

			dc.MoveTo(rectTmp.left + i, rectTmp.top + 3);
			dc.LineTo(rectTmp.left + i, rectTmp.top + uiHeight - 4);
		}

		penTmp.DeleteObject();
		penTmp.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
		dc.SelectObject(&penTmp);

		dc.MoveTo(rectTmp.left + 2, rectTmp.top + uiHeight - 4);
		dc.LineTo(uiSize + 4, rectTmp.top + uiHeight - 4);

		dc.MoveTo(rectTmp.left + 2, rectTmp.top + uiHeight - 3);
		dc.LineTo(uiSize + 4, rectTmp.top + uiHeight - 3);

		dc.MoveTo(rectTmp.left + uiSize, rectTmp.top + 3);
		dc.LineTo(rectTmp.left + uiSize, rectTmp.top + uiHeight - 4);
	
		dc.MoveTo(rectTmp.left + uiSize + 1, rectTmp.top + 3);
		dc.LineTo(rectTmp.left + uiSize + 1, rectTmp.top + uiHeight - 4);
	}
}

//////////////////////////////////////////////////////////////////////////
// Обработка смены формата ДВД
//
void CSixCDonDVD::OnCbnSelchangeFormatdvd()
{
	m_uiFormatDVD = ((CComboBox*)GetDlgItem(IDC_FORMATDVD))->GetCurSel();

	CWnd* wndTmp;
	CRect rectTmp;
	
	wndTmp = GetDlgItem(IDC_INDICATORSIZE);
	wndTmp->GetWindowRect(&rectTmp);
	ScreenToClient(&rectTmp);

	InvalidateRect(&rectTmp);
}

//////////////////////////////////////////////////////////////////////////
// установка размера данных
//
void CSixCDonDVD::SetSizeData()
{
	CMainFrame*  mainFrame = (CMainFrame*)GetParentFrame();
	UINT         uiSize    = 0;
	TRACKINFO*** trackInfo = mainFrame->GetTrackInfo();

	for (UINT i = 0; i < mainFrame->GetCountGroup(); i++)
	{
		for (UINT j = 0; j < (mainFrame->GetCountTrack())[i]; j++)
		{
			uiSize += trackInfo[i][j]->dwSize / 1000000;
		}
	}

	m_uiSizeData = uiSize;
}

//////////////////////////////////////////////////////////////////////////
// обработка выбора диска в дереве
//
VOID CSixCDonDVD::OnTvnSelchangedTreegroups(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	CMainFrame*  mainFrame   = (CMainFrame*)GetParentFrame();

	if (0 == mainFrame->GetCountGroup())
	{
		m_treeTracks.SetRedraw(FALSE);
		m_treeTracks.DeleteAllItems();
		m_treeTracks.InsertItem(_T("No track"), 1, 1);
		m_treeTracks.SetRedraw(TRUE);

		return;
	}

	HTREEITEM hItem = m_treeGroups.GetSelectedItem();
	HTREEITEM hRootItem = m_treeGroups.GetRootItem();

	if (hItem == hRootItem)
	{
		HTREEITEM hChildItem = m_treeGroups.GetChildItem(hItem);
		if (hChildItem)
		{
			m_treeGroups.SelectItem(hChildItem);
		}
		
		return;
	}

	if (NULL != hItem)
	{
		m_treeTracks.SetRedraw(FALSE);
		m_treeTracks.DeleteAllItems();

		HTREEITEM treeItemHead = m_treeTracks.InsertItem(m_treeGroups.GetItemText(hItem), 1, 1);

		// узнаем номер группы
		UINT         uiGroup    = 0;
		HTREEITEM    hChildItem = m_treeGroups.GetChildItem(m_treeGroups.GetRootItem());
		CString      strNameTrack;
		TRACKINFO*** trackInfo  = mainFrame->GetTrackInfo();

		while ((hChildItem != hItem) && (hChildItem = m_treeGroups.GetNextItem(hChildItem, TVGN_NEXT)))
		{
			uiGroup++;
		}

		for (UINT i = 0; i < (mainFrame->GetCountTrack())[uiGroup]; i++)
		{
			strNameTrack.Format(_T("Track %02i"), trackInfo[uiGroup][i]->uiNumber);

			m_treeTracks.InsertItem(strNameTrack, treeItemHead);
		}

		m_treeTracks.Expand(treeItemHead, TVE_EXPAND);

		m_treeTracks.SetRedraw(TRUE);
	}

	*pResult = 0;
}

//////////////////////////////////////////////////////////////////////////
// вызов диалога граббинга аудиокд
//
VOID CSixCDonDVD::GrabAudioCD()
{
	GetParentFrame()->PostMessage(WM_GRABAUDIOCD);
}

//////////////////////////////////////////////////////////////////////////
// вызов диалога прожига DVD
//
VOID CSixCDonDVD::BurnDVDAudio()
{
	GetParentFrame()->PostMessage(WM_BURNDVDA);
}

//////////////////////////////////////////////////////////////////////////
// вывод групп
//
VOID CSixCDonDVD::SetTreeData()
{
	CMainFrame* mainFrame    = (CMainFrame*)GetParentFrame();
	UINT        uiCountGroup = mainFrame->GetCountGroup();
	HTREEITEM   treeItemHead;
	CString     strNameGroup;

	m_treeGroups.SetRedraw(FALSE);
	m_treeTracks.SetRedraw(FALSE);

	m_treeGroups.DeleteAllItems();
	m_treeTracks.DeleteAllItems();

	if (uiCountGroup)
	{
		treeItemHead = m_treeGroups.InsertItem(_T("DVD-Audio"), 1, 1);
	}
	else
	{
		treeItemHead = m_treeGroups.InsertItem(_T("No disc"), 1, 1);
		m_treeTracks.InsertItem(_T("No track"), 1, 1);
	}

	for (UINT i = 0; i < uiCountGroup; i++)
	{
		strNameGroup.Format(_T("Disc %i"), i + 1);
		m_treeGroups.InsertItem(strNameGroup, treeItemHead);
	}

	m_treeGroups.Expand(treeItemHead, TVE_EXPAND);

	m_treeTracks.SetRedraw(TRUE);
	m_treeGroups.SetRedraw(TRUE);

	HTREEITEM hChildItem = m_treeGroups.GetChildItem(treeItemHead);
	m_treeGroups.SelectItem(hChildItem);
}

//////////////////////////////////////////////////////////////////////////
// Возвращает номер выделенной группы начиная с 0
//
UINT CSixCDonDVD::GetSelectedGroup()
{
	HTREEITEM hItem = m_treeGroups.GetSelectedItem();
	UINT      uiGroup    = 0;
	HTREEITEM hChildItem = m_treeGroups.GetChildItem(m_treeGroups.GetRootItem());

	while ((hChildItem != hItem) && (hChildItem = m_treeGroups.GetNextItem(hChildItem, TVGN_NEXT)))
	{
		uiGroup++;
	}

	return uiGroup;
}