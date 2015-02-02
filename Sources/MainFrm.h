#pragma once

#ifndef MAINFRM_H
#define MAINFRM_H

//
#define PROGRAM_NAME "Audio SixPack"

//
#define WM_GRABAUDIOCD       WM_USER + 1
#define WM_GRABAUDIOCD_CLOSE WM_USER + 2

//
#define WM_BURNDVDA          WM_USER + 3
#define WM_BURNDVDA_CLOSE    WM_USER + 4

//
#define MAX_GROUP 9
#define MAX_TRACK 99

// информация об устройстве
typedef struct 
{
	_TCHAR tcLetter[3]; // Drive letter
	_TCHAR tcName[100]; // Drive name
	UCHAR  ucPortID;    // Port Id - Logical SCSI adapter ID if ASPI is used
	UCHAR  ucBusID;     // Bus ID - 0 if ASPI is used
	UCHAR  ucTargetID;  // Target Id
	UCHAR  ucLUN;       // LUN (Logical Unit Number)
} DEVICEINFO;

// информация о треке
typedef struct
{
	UINT   uiNumber;             // номер
//	_TCHAR tcArtist[200];        // артист
//	_TCHAR tcTitle[200];         // название песни
	_TCHAR tcFileName[MAX_PATH]; //
	DWORD  dwSize;               //
} TRACKINFO;

//
typedef CArray<DEVICEINFO, DEVICEINFO> CArrayDevInfo;

#include "otherlib\md5.h"
#include "otherlib\WGButton.h"
#include "otherlib\CDAudio.h"
#include "otherlib\CDVDAudio.h"
#include "StarBurn\StarBurn.h"
#include "DVD-Audio.h"
#include "SixCDonDVD.h"
#include "GrabAudio.h"
#include "BurnDVDA.h"
#include "GrabProgress.h"
#include "About.h"

//
class CMainFrame : public CFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
	DECLARE_MESSAGE_MAP()

	public:

		//
		CMainFrame();

		//
		virtual ~CMainFrame();

	protected: 

		CStatusBar    m_wndStatusBar;            //
		CReBar        m_wndReBar;                //
		CDialogBar    m_wndDlgBar;               //
		CSixCDonDVD   m_dlgSixCDonDVD;           //
		CGrabAudio*   m_dlgGrabAudio;            //
		CBurnDVDA*    m_dlgBurnDVDA;             //
		CArrayDevInfo m_arrDevInfo;              //
		_TCHAR        m_tcDirPath[MAX_PATH];     //
		HANDLE        m_hFileFlag;               // хендл файла флага во временном каталоге
		UINT          m_uiCountGroup;            // количество групп
		UINT          m_uiCountTrack[MAX_GROUP]; // количество треков в группе
		TRACKINFO**   m_trackInfo[MAX_GROUP];    // информация о треках

		WGButton  m_btnNew;         // 
		WGButton  m_btnGrab;        // 
		WGButton  m_btnDeleteGroup; // 
		WGButton  m_btnBurn;        // 
		WGButton  m_btnHelp;        //
		WGButton  m_btnAbout;       //

	protected:

		//
		virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

		//
		static VOID __stdcall CallBackPrint(IN CALLBACK_NUMBER cbNumber, IN PVOID pvContext, 
			IN PVOID pvSpecial1, IN PVOID pvSpecial2);

		//
		VOID RemoveDirectoryRecursive(CString* strDirPath);

		//
		VOID RemoveTemp();

		//
		afx_msg INT OnCreate(LPCREATESTRUCT lpCreateStruct);

		//
		afx_msg VOID OnSetFocus(CWnd *pOldWnd);

		//
		afx_msg VOID OnSize(UINT nType, int cx, int cy);

		//
		afx_msg VOID OnClose();

		//
		afx_msg VOID OnChangeDrive();

		//
		afx_msg void OnNew();
		
		//
		afx_msg void OnGrabbing();

		//
		afx_msg void OnDeleteGroup();

		//
		afx_msg void OnBurning();

		//
		afx_msg void OnHelp();

		//
		afx_msg void OnAbout();

		//
		afx_msg LRESULT GrabAudioCD(WPARAM wParam, LPARAM lParam);

		//
		afx_msg LRESULT GrabAudioCDClose(WPARAM wParam, LPARAM lParam);

		//
		afx_msg LRESULT BurnDVDA(WPARAM wParam, LPARAM lParam);

		//
		afx_msg LRESULT BurnDVDAClose(WPARAM wParam, LPARAM lParam);

		//
		VOID UpdateToolbarNew(CCmdUI* pCmdUI);

	public:

		// возвращает координаты клиентской области
		CRect GetClientSize();

		// возвращает информацию о выбранном в списке устройстве
		DEVICEINFO GetDeviceInfo();

		// возвращает путь к временному каталогу
		_TCHAR* GetDirPath() {return m_tcDirPath;}

		// возвращает число групп
		UINT GetCountGroup() {return m_uiCountGroup;}

		// устанавливает число групп
		VOID SetCountGroup(UINT uiCountGroup) {m_uiCountGroup = uiCountGroup;}

		// возвращает указатель на массив с информацией о числе треков в каждой группе
		UINT* GetCountTrack() {return m_uiCountTrack;}

		// возвращает указатель на массив структуры DVD-A
		TRACKINFO*** GetTrackInfo() {return m_trackInfo;}
};

#endif
