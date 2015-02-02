#pragma once

#ifndef GRABPROGRESS_H
#define GRABPROGRESS_H

//
typedef struct 
{
	LARGE_INTEGER liLastReadenPercent; // last readen percent
	CDialog*      dlgProgress;         //
	UINT          uiCountTrack;        //
	UINT          uiCurrentTrack;      //
} GRABCONTEXT;

//
typedef struct
{
	CDialog* dlgProgress; // 
} GRABTHREADPARAM;

//
class CGrabProgress : public CDialog
{
	DECLARE_DYNAMIC(CGrabProgress)
	DECLARE_MESSAGE_MAP()

	//
	enum { IDD = IDD_GRABPROGRESS };

	public:

		// standard constructor
		CGrabProgress(CWnd* pParent = NULL);   

		//
		virtual ~CGrabProgress();

	protected:

		HANDLE          m_hThread;         //
		DWORD           m_dwThreadID;      //
		GRABTHREADPARAM m_grabThreadParam; //
		BOOL            m_bQuit;           //
		DEVICEINFO*     m_devInfo;         //
		UINT            m_uiCountTrack;    //
		GRABTRACKINFO*  m_trackInfo;       //
		_TCHAR*         m_tcDirPath;       //
		PVOID           m_pvGrabber;       //
		GRABCONTEXT     m_grabContext;     //

	protected:

		// DDX/DDV support
		virtual void DoDataExchange(CDataExchange* pDX);    

		//
		virtual BOOL OnInitDialog();

		//
		virtual BOOL DestroyWindow();

		//
		afx_msg VOID OnCancel();

		//
		afx_msg VOID OnOK();

		//
		static DWORD WINAPI GrabThread(LPVOID lpVoid);

		//
		static VOID __stdcall CallBackGrab(IN CALLBACK_NUMBER cbNumber, IN PVOID pvContext, 
			IN PVOID pvSpecial1, IN PVOID pvSpecial2);

	public:

		//
		VOID SetQuit(BOOL bQuit) {m_bQuit = bQuit;}

		//
		BOOL GetQuit() {return m_bQuit;}

		//
		VOID SetDeviceInfo(DEVICEINFO* devInfo) {m_devInfo = devInfo;}

		//
		DEVICEINFO* GetDevInfo() {return m_devInfo;}

		//
		VOID SetTrackInfo(GRABTRACKINFO* trackInfo, UINT uiCountTrack) 
			{m_trackInfo = trackInfo; m_uiCountTrack = uiCountTrack;}

		//
		UINT GetTrackInfo(GRABTRACKINFO** trackInfo)
			{*trackInfo = m_trackInfo; return m_uiCountTrack; }

		//
		VOID SetDirPath(_TCHAR* tcDirPath) {m_tcDirPath = tcDirPath;}

		//
		_TCHAR* GetDirPath() {return m_tcDirPath;}

		//
		PVOID GetGrabPointer() {return m_pvGrabber;}

		//
		GRABCONTEXT* GetGrabContext() {return &m_grabContext;}
};

#endif