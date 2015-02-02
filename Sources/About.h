#pragma once 

#ifndef ABOUT_H
#define ABOUT_H

//
class CAboutDlg : public CDialog
{
	DECLARE_MESSAGE_MAP()

	//
	enum { IDD = IDD_ABOUTBOX };

	public:

		//
		CAboutDlg();

	protected:

		CBitmap m_bmpLogo;    //
		CFont   m_font15B;    //
		CFont   m_font25B;    //
		CFont   m_font15;     //

	protected:

		//
		virtual BOOL OnInitDialog();

		//
		afx_msg VOID OnPaint();

		//
		afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};

#endif