#pragma once

#ifndef SIXCDONDVD_H
#define SIXCDONDVD_H

//
class CSixCDonDVD : public CDialog
{
	DECLARE_DYNCREATE(CSixCDonDVD)
	DECLARE_MESSAGE_MAP()

	//
	enum { IDD = IDD_SIXCDONDVD };

	public:

		//
		CSixCDonDVD();

		//
		virtual ~CSixCDonDVD();

	protected:

		UINT       m_uiFormatDVD;   // ИД списка размер диска
		UINT       m_uiSizeData;    // размер записываемых данных
		CTreeCtrl  m_treeGroups;    //
		CTreeCtrl  m_treeTracks;    //
		CImageList m_imgListGroups; //
		CImageList m_imgListTracks; // 

	protected:

		//
		virtual VOID DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		//
		virtual VOID OnCancel();

		//
		virtual VOID OnOK();

		//
		virtual BOOL OnInitDialog();

		//
		virtual VOID PaintIndicatorSize(CPaintDC& dc);

		//
		afx_msg VOID OnCbnSelchangeFormatdvd();

		//
		afx_msg VOID OnTvnSelchangedTreegroups(NMHDR *pNMHDR, LRESULT *pResult);

		//
		afx_msg VOID GrabAudioCD();

		//
		afx_msg VOID OnPaint();

		//
		afx_msg VOID BurnDVDAudio();

	public:

		//
		VOID SetSize();

		//
		VOID SetSizeData();

		//
		VOID SetTreeData();

		//
		UINT GetSelectedGroup();
};

#endif