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
#include "WGButton.h"

BEGIN_MESSAGE_MAP(WGButton, CButton)
	ON_WM_THEMECHANGED()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_CAPTURECHANGED()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
// конструктор
//
WGButton::WGButton()
{
	// инициализация переменных
	m_hModThemes = NULL;
	m_hTheme     = NULL;

	m_hImage     = NULL;
	m_hImageBW   = NULL;
	m_uTypeImage = 0;

	m_iImPos     = IMGPOS_LEFT;

	m_bMouseInside = FALSE;
	m_bPushed      = FALSE;
	m_bToggle      = FALSE;
	m_bFlat        = FALSE;
	m_bShowFocus   = TRUE;

	OpenThemeData                 = NULL;
	DrawThemeBackground           = NULL;
	CloseThemeData                = NULL;
	DrawThemeText                 = NULL;
	GetThemeBackgroundContentRect = NULL;
	GetThemeTextExtent            = NULL;

	// получение указателя на функцию TrackMouseEvent
	if (NULL != LoadLibrary(_T("user32.dll")))
	{
		TrackMouseEvent = (Type_TrackMouseEvent)GetProcAddress(
			GetModuleHandle(_T("user32.dll")), "TrackMouseEvent");
	}
	else
	{
		TrackMouseEvent = NULL;
	}

	// генерация таблицы преобразования цветов
	#define CONTRAST_MEDIAN 209

	INT b_offset = 25;
	INT c_offset = -40;
	INT value_offset;

	for (INT i = 0; i < 3; i++)
	{
		for (INT j = 0; j < 256; j++)
		{
			BGRTransTable[i][j] = j;

			// изменение яркости
			if (j + b_offset>255)
			{
				BGRTransTable[i][j] = 255;
			}
			else
			if (j + b_offset < 0)
			{
				BGRTransTable[i][j] = 0;
			}
			else
			{
				BGRTransTable[i][j] = j + b_offset;
			}

			// уменьшение контрастности
			if(BGRTransTable[i][j] < CONTRAST_MEDIAN)
			{
				INT value_offset = (CONTRAST_MEDIAN - BGRTransTable[i][j]) * c_offset / 128;
				if (BGRTransTable[i][j] - value_offset > CONTRAST_MEDIAN)
				{
					BGRTransTable[i][j] = CONTRAST_MEDIAN;
				}
				else
				{
					BGRTransTable[i][j] -= value_offset;
				}
			}
			else
			{
				value_offset = (BGRTransTable[i][j] - CONTRAST_MEDIAN) * c_offset / 128;
				if (BGRTransTable[i][j] + value_offset < CONTRAST_MEDIAN)
				{
					BGRTransTable[i][j] = CONTRAST_MEDIAN;
				}
				else
				{
					BGRTransTable[i][j] += value_offset;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// деструктор
//
WGButton::~WGButton()
{
	if (DST_ICON == m_uTypeImage)
	{
		if (m_hImage)
		{
			DestroyIcon((HICON)m_hImage);
		}
		if (m_hImageBW)
		{
			DestroyIcon((HICON)m_hImageBW);
		}
	}
	else
	{
		if (m_hImage)
		{
			DeleteObject(m_hImage);
		}
		if (m_hImageBW)
		{
			DeleteObject(m_hImageBW);
		}
	}

	if (NULL != m_hModThemes)
	{
		FreeLibrary(m_hModThemes);
	}
}

//////////////////////////////////////////////////////////////////////////
// подготовка к сбклаассингу: установка стиля BS_OWNERDRAW
//
VOID WGButton::PreSubclassWindow()
{
	ModifyStyle(0, BS_OWNERDRAW);

	CButton::PreSubclassWindow();
}

//////////////////////////////////////////////////////////////////////////
// обработка сабклассинга, открытие данных установленной темы
//
BOOL WGButton::SubclassWindow(HWND hwnd)
{
	BOOL bTmp = CButton::SubclassWindow(hwnd);

	InitThemes();

	if (NULL != m_hModThemes)
	{
		m_hTheme = OpenThemeData(m_hWnd, L"BUTTON");
	}

	return bTmp;
}

//////////////////////////////////////////////////////////////////////////
// Инициализация установленной темы и получение указателей на функции
//
VOID WGButton::InitThemes()
{
	m_hModThemes = LoadLibrary(_T("UXTHEME.DLL"));
	if (NULL != m_hModThemes)
	{
		OpenThemeData                 = (PFNOPENTHEMEDATA)GetProcAddress(m_hModThemes, "OpenThemeData");
		DrawThemeBackground           = (PFNDRAWTHEMEBACKGROUND)GetProcAddress(m_hModThemes, "DrawThemeBackground");
		CloseThemeData                = (PFNCLOSETHEMEDATA)GetProcAddress(m_hModThemes, "CloseThemeData");
		DrawThemeText                 = (PFNDRAWTHEMETEXT)GetProcAddress(m_hModThemes, "DrawThemeText");
		GetThemeBackgroundContentRect = (PFNGETTHEMEBACKGROUNDCONTENTRECT)GetProcAddress(m_hModThemes, "GetThemeBackgroundContentRect");
		GetThemeTextExtent            = (PFNGETTHEMETEXTEXTENT)GetProcAddress(m_hModThemes, "GetThemeTextExtent");

		if (!OpenThemeData || !DrawThemeBackground || !CloseThemeData || !DrawThemeText || !GetThemeBackgroundContentRect)
		{
			FreeLibrary(m_hModThemes);
			m_hModThemes = NULL;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Обработка изменения темы
//
LRESULT WGButton::OnThemeChanged()
{
	if (m_hTheme)
	{
		CloseThemeData(m_hTheme);
		FreeLibrary(m_hModThemes);
	}

	InitThemes();

	if (NULL != m_hModThemes)
	{
		m_hTheme = OpenThemeData(m_hWnd, L"BUTTON");
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// Отрисовка кнопки
//
VOID WGButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CDC*    pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CMyMemDC  memDC(pDC);
	CFont*  pFont = GetParent()->GetFont();
	RECT    rectTmp = lpDrawItemStruct->rcItem;
	RECT    rectCaption;
	CString strText;
	UINT    uiDisabled = DSS_NORMAL;
	BOOL    bPush = (lpDrawItemStruct->itemState & ODS_SELECTED) | m_bPushed;
	CPoint  ptOffsetT;   // left-top point of text
	CPoint  ptOffsetI;   // left-top point of image

	if (lpDrawItemStruct->itemState & ODS_DISABLED)
	{
		uiDisabled = DSS_DISABLED;
	}

	// установка шрифта
	if (pFont)
	{
		memDC.SelectObject(pFont);
	}

	// определим положение текста и изображения
	GetWindowText(strText);

	if (strText.IsEmpty())
	{
		ptOffsetI.x = (rectTmp.right  - m_iImWidht)  / 2;
		ptOffsetI.y = (rectTmp.bottom - m_iImHeight) / 2;
	}
	else
	{
		CSize	szTextSize;

		// вычисление размера текста
		if (m_hTheme)
		{
			MyGetThemeTextExtent(m_hTheme, memDC->m_hDC, BP_PUSHBUTTON, PBS_NORMAL, strText, 
				DT_SINGLELINE | DT_CALCRECT, &rectCaption);
			szTextSize.cx = rectCaption.right - rectCaption.left;
			szTextSize.cy = rectCaption.bottom - rectCaption.top;
		}
		else
		{
			szTextSize = pDC->GetTextExtent(strText);
		}

		// вычисление положения текста и изображения
		if (m_hImage)
		{
			if (IMGPOS_LEFT == m_iImPos)
			{
				ptOffsetI.x = (rectTmp.right  - szTextSize.cx - m_iImWidht - 3) / 2;
				ptOffsetI.y = (rectTmp.bottom - m_iImHeight) / 2;
				ptOffsetT.x = (rectTmp.right  - szTextSize.cx - m_iImWidht - 3) / 2 + m_iImWidht + 3;
				ptOffsetT.y = (rectTmp.bottom - szTextSize.cy) / 2;

				rectCaption.left = (rectTmp.right  - szTextSize.cx - m_iImWidht - 3) / 2 + m_iImWidht + 3;
			}
			else
			{
				ptOffsetI.x = (rectTmp.right  - m_iImWidht) / 2;
				ptOffsetI.y = (rectTmp.bottom - szTextSize.cy - m_iImHeight) / 2 + 1;
				ptOffsetT.x = (rectTmp.right  - szTextSize.cx) / 2;
				ptOffsetT.y = ptOffsetI.y + m_iImHeight + 1;

				rectCaption.top  = ptOffsetI.y + m_iImHeight + 1;
			}
		}
		else
		{
			ptOffsetT.x = (rectTmp.right  - szTextSize.cx) / 2;
			ptOffsetT.y = (rectTmp.bottom - szTextSize.cy) / 2;
		}

		// вычисление положения текста, если используется XP Style
		rectCaption.left   = ptOffsetT.x;
		rectCaption.right  = ptOffsetT.x + szTextSize.cx;
		rectCaption.top    = ptOffsetT.y;
		rectCaption.bottom = ptOffsetT.y + szTextSize.cy;
	}

	// вывод фона
	memDC.SetBkMode(TRANSPARENT);

	if (m_hTheme)
	{
		// если кнопка нажата (вывод фона и текста)
		if (bPush)
		{
			DrawThemeBackground(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, PBS_PRESSED, &rectTmp, NULL);
			MyDrawThemeText(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, PBS_NORMAL, strText, 
				DT_LEFT | DT_TOP | DT_SINGLELINE, 0, &rectCaption);
		}
		else
		{
			// если мышка на кнопке (вывод фона и текста)
			if ((m_bMouseInside) && (DSS_DISABLED != uiDisabled))
			{
				DrawThemeBackground(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, PBS_HOT, &rectTmp, NULL);
				MyDrawThemeText(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, PBS_HOT, strText, 
					DT_LEFT | DT_TOP | DT_SINGLELINE, 0, &rectCaption);
			}
			else
			{
				// если фокус на кнопке и разрешен показ фокуса (вывод фона и текста)
				if ((m_bShowFocus && lpDrawItemStruct->itemState & ODS_FOCUS) && (DSS_DISABLED != uiDisabled))
				{
					DrawThemeBackground(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, PBS_DEFAULTED, &rectTmp, NULL);
					MyDrawThemeText(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, PBS_DEFAULTED, strText, 
						DT_LEFT | DT_TOP | DT_SINGLELINE, 0, &rectCaption);
				}
				else
				{
					// если кнопка в стиле FLAT (вывод фона и текста)
					if (!m_bFlat)
					{
						if (DSS_DISABLED == uiDisabled)
						{
							DrawThemeBackground(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, PBS_DISABLED, &rectTmp, NULL);
							MyDrawThemeText(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, PBS_DISABLED, strText, 
								DT_LEFT | DT_TOP | DT_SINGLELINE, 0, &rectCaption);
						}
						else
						{
							DrawThemeBackground(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, PBS_NORMAL, &rectTmp, NULL);
							MyDrawThemeText(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, PBS_NORMAL, strText, 
								DT_LEFT | DT_TOP | DT_SINGLELINE, 0, &rectCaption);
						}
					}

					// все остальные случаи (вывод фона и текста)
					else
					{
						if (DSS_DISABLED == uiDisabled)
						{
							MyDrawThemeText(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, PBS_DISABLED, strText, 
								DT_LEFT | DT_TOP | DT_SINGLELINE, 0, &rectCaption);
						}
						else
						{
							MyDrawThemeText(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, PBS_DISABLED, strText, 
								DT_LEFT | DT_TOP | DT_SINGLELINE, 0, &rectCaption);
						}
					}
				}
			}
		}
	}
	else
	{
		// вывод фона 
		if( m_bToggle && (FALSE == m_bMouseInside) && bPush )
		{
			COLORREF	clrTextColor;

			clrTextColor = memDC.SetTextColor(GetSysColor(COLOR_3DHILIGHT));
			memDC.FillRect(&rectTmp, CDC::GetHalftoneBrush());
			memDC.SetTextColor(clrTextColor);
			memDC.SetBkMode(TRANSPARENT);
		}
		else
		{
			memDC.FillSolidRect(&rectTmp, GetSysColor(COLOR_3DFACE));
		}

		// 3D-рамка
		if((FALSE == m_bFlat) || (TRUE == bPush) || (m_bMouseInside && (DSS_NORMAL == uiDisabled)))
		{
			if (TRUE == m_bFlat) 
			{
				if (bPush)
				{
					memDC.Draw3dRect(&rectTmp, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHILIGHT));
				}
				else
				{
					memDC.Draw3dRect(&rectTmp, GetSysColor(COLOR_3DHILIGHT), GetSysColor(COLOR_3DSHADOW));
				}
			}
			else
			{
				if (bPush)
				{
					memDC.DrawEdge(&rectTmp, EDGE_SUNKEN, BF_RECT | BF_SOFT);
				}
				else
				{
					memDC.DrawEdge(&rectTmp, EDGE_RAISED, BF_RECT | BF_SOFT);
				}
			}
		}

		// смещение положения текста и изображения, когда кнопка нажата
		if (bPush)
		{
			ptOffsetT.x++; ptOffsetT.y++;
			ptOffsetI.x++; ptOffsetI.y++;
		}

		// вывод текста
		memDC.SetTextColor(GetSysColor(COLOR_BTNTEXT));
		memDC.SetBkColor(GetSysColor(COLOR_BTNFACE));
		memDC.DrawState(ptOffsetT, CSize(0, 0), strText, DST_TEXT | uiDisabled, FALSE, 0, (HBRUSH)NULL);
	}

	// вывод изображения 
	if ((DSS_DISABLED == uiDisabled) && m_hImageBW)
	{
		::DrawState(memDC.m_hDC, NULL, NULL, (LPARAM)m_hImageBW, 0, ptOffsetI.x, ptOffsetI.y, 
			m_iImWidht, m_iImHeight, m_uTypeImage);
	}
	else

	if (m_hImage)
	{
		::DrawState(memDC.m_hDC, NULL, NULL, (LPARAM)m_hImage, 0, ptOffsetI.x, ptOffsetI.y, 
			m_iImWidht, m_iImHeight, m_uTypeImage);
	}

	// вывод фокуса
	if( m_bShowFocus && (lpDrawItemStruct->itemState & ODS_FOCUS) && (DSS_NORMAL == uiDisabled))
	{
		RECT rectFocus = rectTmp;
		InflateRect(&rectFocus, -3, -3);
		memDC.DrawFocusRect(&rectFocus);
	}
}

//////////////////////////////////////////////////////////////////////////
//
//
VOID WGButton::OnMouseMove(UINT nFlags, CPoint point)
{
	if (NULL == TrackMouseEvent) 
	{
		if (FALSE == m_bMouseInside)
		{
			m_bMouseInside = TRUE;
			if(GetCapture() != this) 
			{
				SetCapture();
			}
			Invalidate(FALSE);
		}
		else
		{
			CRect rectClient;
			GetClientRect(&rectClient);
			if (!rectClient.PtInRect(point) && GetCapture() == this) 
			{
				ReleaseCapture();
			}
		}
	}
	else
	{
		if (FALSE == m_bMouseInside)
		{
			TRACKMOUSEEVENT TrackMEvent;

			TrackMEvent.cbSize    = sizeof(TrackMEvent);
			TrackMEvent.hwndTrack = m_hWnd;
			TrackMEvent.dwFlags   = TME_LEAVE;

			if (TrackMouseEvent(&TrackMEvent))
			{
				m_bMouseInside = TRUE;
				Invalidate(FALSE);
			}
		}
	}

	CButton::OnMouseMove(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
//
//
VOID WGButton::OnMouseLeave()
{
	m_bMouseInside = FALSE;

	Invalidate(FALSE);
}

//////////////////////////////////////////////////////////////////////////
//
//
VOID WGButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_bToggle)
	{
		m_bPushed = m_bPushed ? FALSE : TRUE;
	}

	CButton::OnLButtonDown(nFlags, point);
}

//////////////////////////////////////////////////////////////////////////
//
//
VOID WGButton::OnCaptureChanged(CWnd *pWnd)
{
	if (NULL == TrackMouseEvent) 
	{	
		if (pWnd && pWnd->m_hWnd == m_hWnd)
		{
			return;
		}
		
		OnMouseLeave();
	}

	CButton::OnCaptureChanged(pWnd);
}

//////////////////////////////////////////////////////////////////////////
//
//
VOID WGButton::ShowFocus(BOOL bShow)
{
	m_bShowFocus = bShow;
}

//////////////////////////////////////////////////////////////////////////
//
//
VOID WGButton::SetStyleFlat(BOOL bFlat)
{
	m_bFlat      = bFlat;
	m_bShowFocus = FALSE;

	if (m_hTheme)
	{
		CloseThemeData(m_hTheme);
		FreeLibrary(m_hModThemes);
	}

	InitThemes();

	if (NULL != m_hModThemes)
	{
		if (m_bFlat)
		{
			m_hTheme = OpenThemeData(m_hWnd, L"TOOLBAR");
		}
		else
		{
			m_hTheme = OpenThemeData(m_hWnd, L"BUTTON");
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//
//
VOID WGButton::SetStyleToggle(BOOL bToggle)
{
	m_bToggle = bToggle;
	m_bPushed = FALSE;
}

//////////////////////////////////////////////////////////////////////////
// Установка изображений для кнопки
//
BOOL WGButton::SetImages(DWORD dwResourceID, DWORD dwResourceID_BW, BOOL bHotImage, UINT uiType)
{
	if (DST_ICON == m_uTypeImage)
	{
		if (m_hImage)
		{
			DestroyIcon((HICON)m_hImage);
		}
		if (m_hImageBW)
		{
			DestroyIcon((HICON)m_hImageBW);
		}
	}
	else
	{
		if (m_hImage)
		{
			DeleteObject(m_hImage);
		}
		if (m_hImageBW)
		{
			DeleteObject(m_hImageBW);
		}
	}

	m_hImage   = NULL; 
	m_hImageBW = NULL;

	if (NULL == dwResourceID)
	{
		return TRUE;
	}

	if (IMAGE_ICON == uiType)
	{
		m_uTypeImage = DST_ICON;
	}
	else
	{
		m_uTypeImage = DST_BITMAP;
	}

	m_hImage = LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(dwResourceID), uiType, 0, 0, 
		(IMAGE_ICON == uiType) ? LR_DEFAULTCOLOR : LR_LOADMAP3DCOLORS);

	if (NULL == m_hImage)
	{
		return FALSE;
	}

	CDC					DisplayDC;
	BITMAPINFOHEADER	bmiHeader;

	DisplayDC.CreateDC(_T("DISPLAY"), NULL, NULL, NULL);

	memset(&bmiHeader, 0, sizeof(BITMAPINFOHEADER));

	bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
	bmiHeader.biBitCount = 0;

	if (IMAGE_ICON == uiType) 
	{
		ICONINFO iconInfo;

		GetIconInfo((HICON)m_hImage, &iconInfo);

		GetDIBits(DisplayDC.GetSafeHdc(), iconInfo.hbmColor, 0, 0, 
			NULL, (LPBITMAPINFO)&bmiHeader, DIB_RGB_COLORS );

		if (bHotImage)
		{
			if (dwResourceID_BW)
			{
				m_hImageBW = LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(dwResourceID_BW), 
					uiType, 0, 0, LR_DEFAULTCOLOR );
			}
			else
			{
				iconInfo.hbmColor = BWBitmap(iconInfo.hbmColor, bmiHeader);
				m_hImageBW = CreateIconIndirect(&iconInfo);
			}
		}
	}

	else

	if (IMAGE_BITMAP == uiType) 
	{
		GetDIBits(DisplayDC.GetSafeHdc(), (HBITMAP)m_hImage, 0, 0, 
			NULL, (LPBITMAPINFO)&bmiHeader, DIB_RGB_COLORS );

		if (bHotImage)
		{
			if (dwResourceID_BW)
			{
				m_hImageBW = LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(dwResourceID_BW), uiType, 
					0, 0, LR_LOADMAP3DCOLORS);
			}
			else
			{
				m_hImageBW = BWBitmap((HBITMAP)m_hImage, bmiHeader);
			}
		}
	}

	if (bHotImage && (NULL == m_hImageBW))
	{
		return FALSE;
	}

	m_iImWidht  = bmiHeader.biWidth;
	m_iImHeight = bmiHeader.biHeight;

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// Преобразует цветной bitmap в чёрно-белый
//
HBITMAP WGButton::BWBitmap(HBITMAP hColorBM, BITMAPINFOHEADER &BMInfo)
{
	INT         iWidht, iHeight, iWidthLine;
	CDC         dcDisplay;
	HBITMAP     hBWBitmap = hColorBM;
	LPBYTE      pBits = NULL;
	SBITMAPINFO bi;

	iWidht     = BMInfo.biWidth;
	iHeight    = BMInfo.biHeight;
	iWidthLine = (iWidht * BMInfo.biBitCount / 8 + 3) & ~3;

	dcDisplay.CreateDC(_T("DISPLAY"), NULL, NULL, NULL);

	pBits = new BYTE[iWidthLine * iHeight];

	memcpy(&bi, &BMInfo, sizeof(BITMAPINFOHEADER));

	if (BI_BITFIELDS == bi.bmiHeader.biCompression)
	{
		bi.bmiHeader.biCompression = 0;
	}

	// Извлечём биты и палитру
	GetDIBits(dcDisplay.GetSafeHdc(), hColorBM, 0, iHeight, 
		pBits, LPBITMAPINFO(bi), DIB_RGB_COLORS);

	switch (BMInfo.biBitCount)
	{
		case 1:
		case 4:
		case 8:
		{	
			INT iMaxPalSize = 1;

			iMaxPalSize <<= BMInfo.biBitCount;

			// Пройдёмся по палитре и заменим цветные элементы на ЧБ
			for (INT i = 0; i < iMaxPalSize; i++)
			{
				DWORD dwColor;

				if (dwColor = bi.bmiColors[i])
				{
					dwColor         = BWColor(dwColor & 0xFF, (dwColor >> 8) & 0xFF, (dwColor >> 16) & 0xFF);
					bi.bmiColors[i] = dwColor | (dwColor << 8) | (dwColor << 16);
				}
			}
		}
		break;

		case 16:
		{
			LPWORD pwBits = (LPWORD)pBits;

			iWidthLine /= 2;

			for (INT y = 0; y < iHeight; y++)
			{
				for (INT x = 0; x < iWidht; x++)
				{
					WORD wColor = pwBits[x + y * iWidthLine];

					wColor = (WORD)BWColor(wColor & 0x1F, (wColor >> 5) & 0x1F, (wColor >> 10) & 0x1F);
					pwBits[x + y * iWidthLine] = wColor | (wColor << 5) | (wColor << 10);
				}
			}
		}
		break;

		case 24:
		case 32:
		{
			INT step = 3;

			if (32 == BMInfo.biBitCount)
			{
				step = 4;
			}

			for (INT y = 0; y < iHeight; y++)
			{
				for (INT x = 0; x < iWidht * step; x += step)
				{
					BYTE btColor;

					btColor = (BYTE)BWColor(pBits[x + y * iWidthLine + 0], 
						pBits[x + y * iWidthLine + 1], pBits[x + y * iWidthLine + 2]);
					pBits[x + y * iWidthLine + 0] = btColor;
					pBits[x + y * iWidthLine + 1] = btColor;
					pBits[x + y * iWidthLine + 2] = btColor;
				}
			}
		}
		break;

		default:
			delete [] pBits;
			return hBWBitmap;
	}

	hBWBitmap = CreateDIBitmap(dcDisplay.GetSafeHdc(), 
		&BMInfo, CBM_INIT, 
		pBits, LPBITMAPINFO(bi), DIB_RGB_COLORS);

	delete [] pBits;
	
	return hBWBitmap;
}

//////////////////////////////////////////////////////////////////////////
// Преобразование пиксела в ЧБ
//
DWORD WGButton::BWColor(DWORD b, DWORD g, DWORD r)
{
	if (b || g || r)
	{
		b = BGRTransTable[0][b];
		g = BGRTransTable[1][g];
		r = BGRTransTable[2][r];
	}

	return (11 * b + 59 * g + 30 * r) / 100;
}

//////////////////////////////////////////////////////////////////////////
// Выводит текст в стиле XP
//
VOID WGButton::MyDrawThemeText(HTHEME hTheme, HDC hdc, INT iPartId, INT iStateId, 
	LPCTSTR lpszText, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect)
{
	HRESULT hr = S_OK;

	if (m_hTheme)
	{
		#ifdef _UNICODE

		hr = DrawThemeText(hTheme, hdc, iPartId, iStateId,
			lpszText, wcslen(lpszText),
			dwTextFlags, dwTextFlags2, pRect);

		#else

		INT nLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpszText, (INT)(_tcslen(lpszText) + 1), NULL, 0);
		nLen += 2;
		WCHAR * pszWide = new WCHAR[nLen];
		if (pszWide)
		{
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpszText, (INT)(_tcslen(lpszText) + 1), pszWide, nLen);
			hr = DrawThemeText(hTheme, hdc, iPartId, iStateId,
				pszWide, (INT)wcslen(pszWide),
				dwTextFlags, dwTextFlags2, pRect);
			delete pszWide;
		}
		
		#endif
	}
}

//////////////////////////////////////////////////////////////////////////
// Возвращает размеры области текста lpszText (в стиле XP)
//
VOID WGButton::MyGetThemeTextExtent(HTHEME hTheme, HDC hdc, int iPartId, 
												int iStateId, LPCTSTR lpszText,
												DWORD dwTextFlags, RECT *pExtentRect)
{
	HRESULT hr = S_OK;

	if (m_hTheme)
	{
		#ifdef _UNICODE

		hr = GetThemeTextExtent(hTheme, hdc, iPartId, iStateId,
			lpszText, wcslen(lpszText),
			dwTextFlags, NULL, pExtentRect);

		#else

		INT nLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpszText, (INT)(_tcslen(lpszText) + 1), NULL, 0);
		nLen += 2;
		WCHAR * pszWide = new WCHAR[nLen];
		if (pszWide)
		{
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpszText, (INT)(_tcslen(lpszText) + 1), pszWide, nLen);
			hr = GetThemeTextExtent(hTheme, hdc, iPartId, iStateId,
				pszWide, (INT)wcslen(pszWide),
				dwTextFlags, NULL, pExtentRect);
			delete pszWide;
		}

		#endif
	}
}

