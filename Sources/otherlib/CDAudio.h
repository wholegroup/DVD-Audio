#ifndef CDAUDIO_H
#define CDAUDIO_H

// CDAudio.h: interface for the CCDAudio class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CDAUDIO1_H__FDE356E6_CB80_41FE_82BE_9F051620A4A0__INCLUDED_)
#define AFX_CDAUDIO1_H__FDE356E6_CB80_41FE_82BE_9F051620A4A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "mmsystem.h"

#ifdef _UNICODE
	#define	ATOI	_wtoi
	#define	ITOA	_itow	
	#define	STRNCPY	wcsncpy
#else
	#define ATOI	atoi
	#define	ITOA	_itoa
	#define	STRNCPY	strncpy
#endif


//=============================================================================
// A class wrapper of MCI API functions
//=============================================================================
class CCDAudio  
{
	public:
		CCDAudio(_TCHAR* tcLetter);
		virtual ~CCDAudio();
		
	protected:
		MCIERROR m_nErrorCode;                // MCI error code

	protected:
		inline void MCIError( MCIERROR MCIError ); // handle MCI errors

	public:
		MCIERROR Play();                      // Start playing CD Audio
		MCIERROR Play(const int nPos);        // Start playing CD Audio on given position
		MCIERROR Stop();                      // Stop playing CD Audio
		MCIERROR Pause();                     // Pause playing CD Audio
		MCIERROR Forward();                   // Move to the next track
		MCIERROR Backward();                  // Move to the previous track
		VOID EjectCDROM();                    // Eject the CDROM
		INT GetCurrentPos();                  // Return the current position in seconds
		INT GetCurrentTrack();                // Return the current track number
		INT GetLenghtAllTracks();             // Return length of all track in seconds
		INT GetTracksCount();                 // Return total tracks count
		INT GetTrackLength(const int nTrack); // Return length of given track
		INT GetTrackBeginTime( const int nTrack ); // Return begin time of given track
		BOOL IsMediaInsert();                 // check wheter CD media is inserted
		BOOL IsPaused();                      // is paused mode
		BOOL IsStopped();                     // is stopped mode
		BOOL IsReady();                       // the device is ready
		BOOL IsPlaying();                     // is playing mode
};

#endif // !defined(AFX_CDAUDIO1_H__FDE356E6_CB80_41FE_82BE_9F051620A4A0__INCLUDED_)

#endif
