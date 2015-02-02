#ifndef WGBUTTON_H
#define WGBUTTON_H

#include <uxtheme.h>

typedef HRESULT(__stdcall *PFNCLOSETHEMEDATA)(HTHEME hTheme);
typedef HRESULT(__stdcall *PFNDRAWTHEMEBACKGROUND)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect,  const RECT *pClipRect);
typedef HTHEME(__stdcall *PFNOPENTHEMEDATA)(HWND hwnd, LPCWSTR pszClassList);
typedef HRESULT (__stdcall *PFNDRAWTHEMETEXT)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect);
typedef HRESULT (__stdcall *PFNGETTHEMEBACKGROUNDCONTENTRECT)(HTHEME hTheme,  HDC hdc, int iPartId, int iStateId,  const RECT *pBoundingRect, RECT *pContentRect);
typedef HRESULT (__stdcall *PFNGETTHEMETEXTEXTENT)(HTHEME hTheme,  HDC hdc, int iPartId, int iStateId,  LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, const RECT *pBoundingRect, RECT *pContentRect);


typedef BOOL (FAR WINAPI *Type_TrackMouseEvent)(LPTRACKMOUSEEVENT);

struct SBITMAPINFO
{
	BITMAPINFOHEADER bmiHeader;
	DWORD            bmiColors[256];			

	operator BITMAPINFO()         {return *this;                    }
	operator LPBITMAPINFO()       {return (LPBITMAPINFO)this;       }
	operator LPBITMAPINFOHEADER() {return (LPBITMAPINFOHEADER)this; }	
};


//////////////////////////////////////////////////////////////////////////
//
//
class WGButton : public CButton
{
	DECLARE_MESSAGE_MAP()

	public:
		WGButton();
		virtual ~WGButton();

	protected:
		HMODULE m_hModThemes;   // хендл библиотеки тем
		HTHEME  m_hTheme;       // хендл текущей темы
		HANDLE  m_hImage;       // Цветное изображение
		HANDLE  m_hImageBW;     // ЧБ изображение
		UINT    m_uTypeImage;   // Тип изображения(Icon or Bitmap)
		INT     m_iImPos;       // Позиция изображения: над текстом или слева
		INT     m_iImHeight;    //
		INT     m_iImWidht;     //
		BOOL    m_bShowFocus;   // Показывать рамку, если фокус на кнопке
		BOOL    m_bFlat;        // стиль Flat
		BOOL    m_bToggle;      // стиль Toggle
		BOOL    m_bPushed;      // для Toggle button означает нажата ли клавиша
		BOOL    m_bMouseInside; // мышка внутри клавиши

		BYTE    BGRTransTable[3][256]; // таблица для изменения яркости/контрастности

		PFNOPENTHEMEDATA                 OpenThemeData;
		PFNDRAWTHEMEBACKGROUND           DrawThemeBackground;
		PFNCLOSETHEMEDATA                CloseThemeData;
		PFNDRAWTHEMETEXT                 DrawThemeText;
		PFNGETTHEMEBACKGROUNDCONTENTRECT GetThemeBackgroundContentRect;
		PFNGETTHEMETEXTEXTENT            GetThemeTextExtent;

		Type_TrackMouseEvent TrackMouseEvent;

	public:
		enum {IMGPOS_LEFT, IMGPOS_TOP};

	protected:
		virtual VOID PreSubclassWindow();
		virtual VOID DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
		virtual VOID InitThemes();
		afx_msg LRESULT OnThemeChanged();
		afx_msg VOID OnMouseMove(UINT nFlags, CPoint point);
		afx_msg VOID OnMouseLeave();
		afx_msg VOID OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg VOID OnCaptureChanged(CWnd *pWnd);
		VOID MyDrawThemeText(HTHEME hTheme, HDC hdc, INT iPartId, INT iStateId, 
			LPCTSTR lpszText, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect);
		VOID MyGetThemeTextExtent(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCTSTR lpszText,
			DWORD dwTextFlags, RECT *pExtentRect);
		HBITMAP BWBitmap( HBITMAP hColorBM, BITMAPINFOHEADER &BMInfo );
		DWORD BWColor(DWORD b, DWORD g, DWORD r);

	public:
		virtual BOOL SubclassWindow(HWND hwnd);
		VOID SetStyleFlat(BOOL bFlat = TRUE);
		VOID SetStyleToggle(BOOL bToggle = TRUE);
		VOID ShowFocus(BOOL bShow = TRUE);
		VOID SetImagePosition(INT iImPos) {m_iImPos = iImPos;}
		BOOL SetImages(DWORD dwResourceID, DWORD dwResourceID_BW = NULL, 
			BOOL bHotImage = TRUE, UINT uiType = IMAGE_ICON);
//		VOID SetEnabled(BOOL bEnabled = TRUE) {m_bEnabled = bEnabled;}
};


//////////////////////////////////////////////////////////////////////////
//
//
class CMyMemDC : public CDC
{
	private:	
		CBitmap  m_bitmap;    // Offscreen bitmap
		CBitmap* m_oldBitmap; // bitmap originally found in CMemDC
		CDC*     m_pDC;       // Saves CDC passed in constructor
		CRect    m_rect;      // Rectangle of drawing area.
		BOOL     m_bMemDC;    // TRUE if CDC really is a Memory DC.

	public:

	CMyMemDC(CDC* pDC, const CRect* pRect = NULL)
		:CDC()
	{
		ASSERT(pDC != NULL); 

		// Some initialization
		m_pDC       = pDC;
		m_oldBitmap = NULL;
		m_bMemDC    = !pDC->IsPrinting();

		// Get the rectangle to draw
		if (NULL == pRect)
		{
			pDC->GetClipBox(&m_rect);
		}
		else
		{
			m_rect = *pRect;
		}

		// Create a Memory DC
		if (m_bMemDC)
		{
			CreateCompatibleDC(pDC);
			pDC->LPtoDP(&m_rect);

			m_bitmap.CreateCompatibleBitmap(pDC, m_rect.Width(), m_rect.Height());
			m_oldBitmap = SelectObject(&m_bitmap);

			SetMapMode(pDC->GetMapMode());

			SetWindowExt(pDC->GetWindowExt());
			SetViewportExt(pDC->GetViewportExt());

			pDC->DPtoLP(&m_rect);
			SetWindowOrg(m_rect.left, m_rect.top);
		}

		// Make a copy of the relevent parts of the current DC for printing
		else
		{
			m_bPrinting = pDC->m_bPrinting;
			m_hDC       = pDC->m_hDC;
			m_hAttribDC = pDC->m_hAttribDC;
		}

		// Fill background 
		FillSolidRect(m_rect, pDC->GetBkColor());
	}

	virtual ~CMyMemDC()	
	{		
		// Copy the offscreen bitmap onto the screen.
		if (m_bMemDC)
		{
			m_pDC->BitBlt(m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(),
				this, m_rect.left, m_rect.top, SRCCOPY);			

			//Swap back the original bitmap.
			SelectObject(m_oldBitmap);		
		}
		else
		{
			m_hDC = m_hAttribDC = NULL;
		}	
	}

	// Allow usage as a pointer	
	CMyMemDC* operator->() 
	{
		return this;
	}	

	// Allow usage as a pointer	
	operator CMyMemDC*() 
	{
		return this;
	}
};

#endif