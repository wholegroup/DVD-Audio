#pragma once

#ifndef GRABAUDIO_H
#define GRABAUDIO_H

//
typedef struct
{
	UINT   uiNumber;             // номер
	_TCHAR tcArtist[200];        // артист
	_TCHAR tcTitle[200];         // название песни
	_TCHAR tcFileName[MAX_PATH]; //
	UCHAR  ucStartingMSF[3];     //
	UCHAR  ucEndingMSF[3];       //
	UINT   uiLengthMSF[3];       //
	DWORD  dwSize;               //
} GRABTRACKINFO;

//
class CGrabAudio : public CDialog
{
	DECLARE_DYNCREATE(CGrabAudio)
	DECLARE_MESSAGE_MAP()

	//
	enum { IDD = IDD_GRABAUDIO};

	public:

		// standard constructor
		CGrabAudio(CWnd* pParent = NULL);   

		//
		virtual ~CGrabAudio();

	protected:

		INT                           m_iItemSelected; //
		GRABTRACKINFO*                m_trackInfo;     //
		PVOID                         m_pvGrabber;     //
		CCDAudio*                     m_cdAudio;       //
		INT                           m_iTrackPlay;    //
		UINT_PTR                      m_iTimer;        //
		HMIXER                        m_hMixer;        //
		MIXERCONTROL                  m_mixerControl;  //
		MIXERCONTROLDETAILS           m_mixerDetails;  //
		MIXERCONTROLDETAILS_UNSIGNED* m_mixerValue;    //
		DWORD                         m_dwOldLeft;     //
		DWORD                         m_dwOldRight;    //
		WGButton                      m_btnPrev;       //
		WGButton                      m_btnBack;       //
		WGButton                      m_btnForward;    //
		WGButton                      m_btnNext;       //
		WGButton                      m_btnStop;       //
		WGButton                      m_btnPlay;       //
		WGButton                      m_btnOpen;       //

	protected:

		// DDX/DDV support
		virtual void DoDataExchange(CDataExchange* pDX); 

		//
		virtual BOOL OnInitDialog();

		//
		VOID OnOK();

		//
		VOID OnCancel();

		//
		VOID StartTimer();

		//
		VOID StopTimer();

		//
		VOID SetInfoPlayer();

		//
		BOOL InitMixer();

		//
		VOID GetMixerValue();

		//
		VOID SetMixerValue();

	public:

		//
		VOID SetSize();

		//
		VOID ReadTracks();

		//
		VOID SetSelectInfo();

		//
		afx_msg VOID OnPaint(); 

		//
		afx_msg VOID OnRefreshList();

		//
		afx_msg VOID OnSelectAll();

		//
		afx_msg void OnListChange(NMHDR *pNMHDR, LRESULT *pResult);

		//
		afx_msg void OnPlayerOpen();

		//
		afx_msg void OnPlayerPlay();

		//
		afx_msg void OnPlayerStop();

		//
		afx_msg void OnPlayerNext();

		//
		afx_msg void OnPlayerPrev();

		//
		afx_msg void OnPlayerBack();

		//
		afx_msg void OnPlayerForward();

		//
		afx_msg void OnTimer(UINT nIDEvent);

		//
		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

		//
		LRESULT OnThemeChanged();

		//
		afx_msg void OnGO();

		//
		afx_msg void OnClose();
};

#endif