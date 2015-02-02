#pragma once

#ifndef BURNDVDA_H
#define BURNDVDA_H

//
typedef struct 
{
	UINT           uiAllTracks;    // общее количество треков
	UINT           uiCurrentGroup; // номер текущей группы
	UINT           uiCurrentTrack; // номер текущего трека
	UINT           uiGroupTrack;   // номер текушего трека относительно группы
	CProgressCtrl* progressStatus; //
	TRACKINFO***   trackInfo;      //
	CDialog*       dlgBurn;        //
} STRUCTURECONTEXT;

//
typedef struct
{
	CDialog*   dlgBurn;   //
	CFrameWnd* mainFrame; //
	PVOID*     pvGrabber; //
	CDVDAudio* dvdAudio;  //
} BURNTHREADPARAM;

//
typedef struct
{
	CDialog*       dlgBurn;        //
	CProgressCtrl* progressStatus; //
} BURNCONTEXT;

//
class CBurnDVDA : public CDialog
{
	DECLARE_DYNAMIC(CBurnDVDA)
	DECLARE_MESSAGE_MAP()

	//
	enum { IDD = IDD_BURNDVDA };

	public:

		// standard constructor
		CBurnDVDA(CWnd* pParent = NULL);

		//
		virtual ~CBurnDVDA();

	protected:

		CImageList*     m_pImageList;      //
		HANDLE          m_hThread;         //
		DWORD           m_dwThreadID;      //
		BURNTHREADPARAM m_burnThreadParam; //
		BOOL            m_bQuit;           //

	protected:

		// DDX/DDV support
		virtual VOID DoDataExchange(CDataExchange* pDX);

		//
		virtual BOOL OnInitDialog();

		//
		virtual void OnOK();

		//
		virtual void OnCancel();

		//
		afx_msg VOID OnPaint();

		//
		afx_msg void OnClose();

		//
		VOID SetEventMessage(_TCHAR* tcMessage);

		//
		static DWORD WINAPI BurnThread(LPVOID lpVoid);

		//
		static VOID __stdcall StructureCallBack(UINT uiMessageID, PVOID pvContext, PVOID pvSpecial);

		//
		static VOID __stdcall BurnCallBack(IN CALLBACK_NUMBER cbNumber, IN PVOID pvContext, 
			IN PVOID pvSpecial1, IN PVOID pvSpecial2);

	public:

		//
		VOID SetSize();

		//
		VOID SetQuit(BOOL bQuit) {m_bQuit = bQuit;}

		//
		BOOL GetQuit() {return m_bQuit;}
};

#endif