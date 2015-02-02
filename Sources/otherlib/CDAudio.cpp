#include "stdafx.h"
#include "CDAudio.h"
#include "..\StarBurn\StarBurn.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//=============================================================================
// Constructor
//=============================================================================
CCDAudio::CCDAudio(_TCHAR* tcLetter)
{ 
	CString strTmp;

	strTmp  = CString(_T("open "));
	strTmp += CString(tcLetter);
	strTmp += CString(_T(" type cdaudio alias ecdrom"));

	m_nErrorCode = mciSendString(strTmp, NULL, 0, NULL);	
}


//=============================================================================
// Destructor
//=============================================================================
CCDAudio::~CCDAudio()
{	
	if( IsReady() )
	{
		if( IsPaused() || IsPlaying() )
		{
			mciSendString( _T("stop ecdrom"), NULL, 0, NULL );				
		}
	}

	mciSendString( _T("close ecdrom"), NULL, 0, NULL ); 
}


//=============================================================================
// Check whether CD media is inserted
//=============================================================================
BOOL CCDAudio::IsMediaInsert()
{
	const INT nBuffSize = 16;
	_TCHAR szBuff[nBuffSize];

	m_nErrorCode = mciSendString(_T("status ecdrom media present"), szBuff, nBuffSize, NULL );    
	if (m_nErrorCode != 0)
	{
		return FALSE;
	}

	if (lstrcmp(szBuff, _T("true")) == 0) 
	{
		return TRUE;
	}

	return FALSE; 
}

//=============================================================================
// Check whether is paused mode
//=============================================================================
BOOL CCDAudio::IsPaused()
{
	const int nBuffSize = 64;
	TCHAR szBuff[nBuffSize];

	m_nErrorCode = mciSendString( _T("status ecdrom mode wait"), szBuff, nBuffSize, NULL );    
	if( m_nErrorCode != 0 )
	{
		return false;
	}

	if( lstrcmp( szBuff, _T("paused") ) == 0 ) 
		return true;
    return false; 
}

//=============================================================================
// Check whether is stopped mode
//=============================================================================
BOOL CCDAudio::IsStopped()
{
	const int nBuffSize = 64;
	TCHAR szBuff[nBuffSize];

	m_nErrorCode = mciSendString( _T("status ecdrom mode wait"), szBuff, nBuffSize, NULL );    
	if( m_nErrorCode != 0 )
	{
		return false;
	}

	if( lstrcmp( szBuff, _T("stopped") ) == 0 ) 
		return true;
    return false; 
}

//=============================================================================
// Check whether MCI is ready
//=============================================================================
BOOL CCDAudio::IsReady()
{
	const int nBuffSize = 64;
	TCHAR szBuff[nBuffSize];

	m_nErrorCode = mciSendString( _T("status ecdrom mode wait"), szBuff, nBuffSize, NULL );    
	if( m_nErrorCode != 0 )
	{
		return false;
	}

	if( lstrcmp( szBuff, _T("not ready") ) == 0 ) 
		return false;
    return true; 
}


//=============================================================================
// Check whether is play mode
//=============================================================================
BOOL CCDAudio::IsPlaying()
{
	const int nBuffSize = 64;
	TCHAR szBuff[nBuffSize];

	m_nErrorCode = mciSendString( _T("status ecdrom mode wait"), szBuff, nBuffSize, NULL );    
	if( m_nErrorCode != 0 )
	{
		return false;
	}

	if( lstrcmp( szBuff, _T("playing") ) == 0 ) 
		return true;
    return false; 
}


//=============================================================================
// Return the length of the given track
//=============================================================================
INT CCDAudio::GetTrackLength(const int nTrack)
{
	if(IsMediaInsert())
	{
		const int nBuffSize = 64;
		TCHAR szTrack[10];
		TCHAR szReqBuff[nBuffSize];
		TCHAR szBuff[nBuffSize];
		
		ITOA( nTrack, szTrack, 10 );
		lstrcpy( szReqBuff, _T("status ecdrom length track ") );
		lstrcat( szReqBuff, szTrack );

		m_nErrorCode = mciSendString( szReqBuff, szBuff, nBuffSize, NULL );    
		if( m_nErrorCode != 0 )
		{
			return 0;
		}

		TCHAR szMin[3], szSec[3];

		STRNCPY( szMin, szBuff, 2 );
		STRNCPY( szSec, (szBuff + 3), 2 );

		return ((ATOI(szMin) * 60) + ATOI(szSec));		
	}

	return 0;
}

//=============================================================================
// Eject the CDROM
//=============================================================================
void CCDAudio::EjectCDROM()
{
	if(IsMediaInsert())
	{
		m_nErrorCode = mciSendString(_T("Set ecdrom Door Open wait"), NULL, 0, NULL);
	}
	else
	{
		DWORD dwStart;
		DWORD dwStop;

		dwStart = GetTickCount();
		m_nErrorCode = mciSendString(_T("Set ecdrom Door Closed wait"), NULL, 0, NULL); 
		dwStop  = GetTickCount();

		if ((dwStop - dwStart) < 500)
		{
			m_nErrorCode = mciSendString(_T("Set ecdrom Door Open wait"), NULL, 0, NULL);
		}
	}
}

//=============================================================================
// handle MCI errors
//=============================================================================
void CCDAudio::MCIError( MCIERROR m_nErrorCode )
{
	TCHAR szBuff[128];
	memset(szBuff, 0, sizeof(szBuff));

	if( !mciGetErrorString( m_nErrorCode, szBuff, sizeof(szBuff) ) )
	{
        lstrcpy( szBuff, _T("Unknown error") );
	}

	::MessageBox( NULL, szBuff, _T("MCI Error"), MB_OK | MB_ICONERROR );

	m_nErrorCode = 0;
}

//=============================================================================
// Start playing CD Audio
//=============================================================================
MCIERROR CCDAudio::Play()
{	
	m_nErrorCode = mciSendString( _T("play ecdrom"), NULL, 0, NULL );
	if( m_nErrorCode != 0 ){ 
		MCIError( m_nErrorCode ); 
		return m_nErrorCode;
	}
	
	return m_nErrorCode;
}

//=============================================================================
// Start playing CD Audio on given position
//=============================================================================
MCIERROR CCDAudio::Play(const int nPos)
{
	TCHAR szBuff[128];

	// form the connection string
	/*
	_stprintf( szBuff, _T("play ecdrom from %02d:%02d:%03d"), 
		(nPos / 6000), 
		(nPos % 6000) / 100, 
		(nPos % 6000) % 100);
	*/
	_stprintf( szBuff, _T("play ecdrom from %02d:%02d:30"), 
		(nPos / 6000), 
		(nPos % 6000) / 100);

	// send the command string
	m_nErrorCode = mciSendString( szBuff, NULL, 0, NULL );
	if( m_nErrorCode != 0 ){ 
		MCIError( m_nErrorCode ); 
		return m_nErrorCode;
	}

	return m_nErrorCode;
}

//=============================================================================
// Stop playing CD Audio
//=============================================================================
MCIERROR CCDAudio::Stop()
{
    m_nErrorCode = mciSendString( _T("stop ecdrom"), NULL, 0, NULL );
	// handle error
	if( m_nErrorCode != 0 ){
		MCIError( m_nErrorCode );
		return m_nErrorCode;
	}

	m_nErrorCode = mciSendString( _T("seek ecdrom to start"), NULL, 0, NULL );
	if( m_nErrorCode != 0 ){ 
		MCIError( m_nErrorCode ); 
		return m_nErrorCode;
	}

	return m_nErrorCode;
}

//=============================================================================
// Pause playing CD Audio
//=============================================================================
MCIERROR CCDAudio::Pause()
{  
    m_nErrorCode = mciSendString( _T("pause ecdrom"), NULL, 0, NULL );
	if( m_nErrorCode != 0 ){ 
		MCIError( m_nErrorCode ); 
		return m_nErrorCode;
	}

	return m_nErrorCode;
}

//=============================================================================
// Move to the next track
//=============================================================================
MCIERROR CCDAudio::Forward()
{   		
    TCHAR szBuff2[128], szBuff3[32];
	
    int nCurrTrack = GetCurrentTrack();
	const int nLastTrack = GetTracksCount();
	
	// now the current track is the last track
	if( (nCurrTrack == nLastTrack) || (nLastTrack == 0) ) 
		return 0;

	// increment track position	
    nCurrTrack++;

	// form the command string
	_stprintf( szBuff2, _T("status ecdrom position track %d"), nCurrTrack ); 

	// send the command string
    m_nErrorCode = mciSendString( szBuff2, szBuff3, sizeof(szBuff3), NULL );
	if( m_nErrorCode != 0 ){ 
		MCIError( m_nErrorCode ); 
		return m_nErrorCode;
	}

	memset(szBuff2, 0, sizeof(szBuff2));
	
	// form the command string
    _stprintf( szBuff2, _T("play ecdrom from %s"), szBuff3 );

	// send the command string
	m_nErrorCode = mciSendString( szBuff2, NULL, 0, NULL );
	if( m_nErrorCode != 0 ){ 
		MCIError( m_nErrorCode ); 
		return m_nErrorCode;
	}

	return m_nErrorCode;
}

//=============================================================================
// Move to the previous track
//=============================================================================
MCIERROR CCDAudio::Backward()
{			
    TCHAR szBuff2[128], szBuff3[32];

	int nCurrTrack = GetCurrentTrack();
	const int nLastTrack = GetTracksCount();

	if( (nCurrTrack == 0) || (nLastTrack == 0) || (nCurrTrack == 1) ) 
		return 0;
	
    nCurrTrack--;

	_stprintf( szBuff2, _T("status ecdrom position track %d"), nCurrTrack ); 

    m_nErrorCode = mciSendString( szBuff2, szBuff3, sizeof(szBuff3), NULL );
	if( m_nErrorCode != 0 ){ 
		MCIError( m_nErrorCode ); 
		return m_nErrorCode;
	}

	memset(szBuff2, 0, sizeof(szBuff2));
	
    _stprintf( szBuff2, _T("play ecdrom from %s"), szBuff3 );

	m_nErrorCode = mciSendString( szBuff2, NULL, 0, NULL );
	if( m_nErrorCode != 0 ){ 
		MCIError( m_nErrorCode ); 
		return m_nErrorCode;
	}

	return m_nErrorCode;
}

//=============================================================================
// Return the current track position in seconds
//=============================================================================
int CCDAudio::GetCurrentPos()
{
	if(IsMediaInsert())
	{
		TCHAR szBuff[16];
		TCHAR min[3], sec[3];
		m_nErrorCode = 0;

		m_nErrorCode = mciSendString( _T("status ecdrom position"), szBuff, sizeof(szBuff), NULL );
		if( m_nErrorCode != 0 ){ 
			MCIError( m_nErrorCode ); 
			return 0;
		}

		STRNCPY( min, szBuff, 2 );
		STRNCPY( sec, (szBuff + 3), 2 );

		return ((ATOI(min) * 60) + ATOI(sec));
	}

	return 0;
}

//=============================================================================
// Return the current track number
//=============================================================================
int CCDAudio::GetCurrentTrack()
{
	TCHAR szBuff[16];
   
	m_nErrorCode = mciSendString( _T("status ecdrom current track"), szBuff, sizeof(szBuff), NULL );
	if( m_nErrorCode != 0 ){ 
		MCIError( m_nErrorCode ); 
		return 0;
	}

	return ATOI(szBuff);
}

//=============================================================================
// Return the total tracks length
//=============================================================================
int CCDAudio::GetLenghtAllTracks()
{
	TCHAR szBuff[16];
	TCHAR min[3], sec[3];
    m_nErrorCode = 0;

	m_nErrorCode = mciSendString( _T("status ecdrom length"), szBuff, sizeof(szBuff), NULL );
	if( m_nErrorCode != 0 ){ 
		MCIError( m_nErrorCode ); 
		return 0;
	}

	STRNCPY( min, szBuff, 2 );
    STRNCPY( sec, (szBuff + 3), 2 );

	return ((ATOI(min) * 60) + ATOI(sec));
}

//=============================================================================
// Return the count of tracks
//=============================================================================
int CCDAudio::GetTracksCount()
{
	TCHAR szBuff[16];
	
	m_nErrorCode = mciSendString( _T("status ecdrom number of tracks"), szBuff, sizeof(szBuff), NULL );

	if( m_nErrorCode != 0 ){ 
		MCIError( m_nErrorCode ); 
		return 0;
	}

    return ATOI(szBuff); 
}

//=============================================================================
// Return the start time of the given track
//=============================================================================
int CCDAudio::GetTrackBeginTime( const int nTrack )
{
	TCHAR szBuff[64], szReq[64];
	_stprintf( szReq, _T("status ecdrom position track %d"), nTrack );

	m_nErrorCode = mciSendString( szReq, szBuff, sizeof(szBuff), NULL );
	if( m_nErrorCode != 0 ){ 
		return 0;
	}

	TCHAR szMin[3], szSec[3];

	STRNCPY( szMin, szBuff, 2 );
	szMin[2] = '\0';

	STRNCPY( szSec, (szBuff + 3), 2 );
	szSec[2] = '\0';

	return ((ATOI(szMin) * 60) + ATOI(szSec));		
}