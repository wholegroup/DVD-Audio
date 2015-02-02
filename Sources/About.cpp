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
#include "About.h"

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

//
CAboutDlg::CAboutDlg()
	:CDialog(CAboutDlg::IDD)
{
}

//
BOOL CAboutDlg::OnInitDialog()
{
	// загрузка лого
	m_bmpLogo.LoadBitmap(IDB_LOGOTYPE);

	// заголовок программы
	CString dlgName;
	
	dlgName = _T("About ");
	dlgName += _T(PROGRAM_NAME);
	SetWindowText(dlgName);

	// установка шрифтов
	LOGFONT lf15B;
	memset(&lf15B, 0, sizeof(LOGFONT));     // Clear out structure.
	lf15B.lfHeight = 15;                    // Request a 15-pixel-high font	
	lf15B.lfWeight = FW_BOLD;		          // with Bold and
	_tcscpy(lf15B.lfFaceName, _T("Arial")); // with face name "Arial".
	m_font15B.CreateFontIndirect(&lf15B);   // Create the font.

	GetDlgItem(IDC_STATICABOUT01)->SetFont(&m_font15B);
	GetDlgItem(IDC_STATICABOUT02)->SetFont(&m_font15B);

	GetDlgItem(IDC_STATICABOUT04)->SetFont(&m_font15B);
	GetDlgItem(IDC_STATICABOUT05)->SetFont(&m_font15B);

	LOGFONT lf25B;
	memset(&lf25B, 0, sizeof(LOGFONT));     // Clear out structure.
	lf25B.lfHeight = 25;                    // Request a 25-pixel-high font	
	lf25B.lfWeight = FW_BOLD;		          // with Bold and
	_tcscpy(lf25B.lfFaceName, _T("Arial")); // with face name "Arial".
	m_font25B.CreateFontIndirect(&lf25B);   // Create the font.

	GetDlgItem(IDC_STATICABOUT03)->SetWindowText(_T(PROGRAM_NAME));
	GetDlgItem(IDC_STATICABOUT03)->SetFont(&m_font25B);

	LOGFONT lf15;
	memset(&lf15, 0, sizeof(LOGFONT));     // Clear out structure.
	lf15.lfHeight = 15;                    // Request a 15-pixel-high font	
	_tcscpy(lf15.lfFaceName, _T("Arial")); // with face name "Arial".
	m_font15.CreateFontIndirect(&lf15);    // Create the font.

	GetDlgItem(IDC_STATICABOUT07)->SetFont(&m_font15);
	GetDlgItem(IDC_STATICABOUT08)->SetFont(&m_font15);

	return CDialog::OnInitDialog();
}

//
VOID CAboutDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CDC      dcImage;
	BITMAP   bm;
	CBitmap* pOldBitmap;

	if (dcImage.CreateCompatibleDC(&dc))
	{
		m_bmpLogo.GetBitmap(&bm);

		pOldBitmap = dcImage.SelectObject(&m_bmpLogo);

		TransparentBlt(dc, 10, 10, bm.bmWidth, bm.bmHeight, dcImage.GetSafeHdc(), 0, 0, 
			bm.bmWidth, bm.bmHeight, RGB(0x69, 0x61, 0xC7));

		dcImage.SelectObject(pOldBitmap);
	}
}

//
HBRUSH CAboutDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetSafeHwnd() == GetDlgItem(IDC_STATICABOUT03)->GetSafeHwnd())
	{
		pDC->SetTextColor(RGB(0, 0, 255));
	}
	else

	if (pWnd->GetSafeHwnd() == GetDlgItem(IDC_STATICABOUT08)->GetSafeHwnd())
	{
		pDC->SetTextColor(RGB(255, 0, 0));
	}

	return hbr;
}
