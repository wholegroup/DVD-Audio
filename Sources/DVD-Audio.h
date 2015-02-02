#pragma once

#ifndef DVDAUDIO_H
#define DVDAUDIO_H

//
class CDVDAudioApp : public CWinApp
{
	public:

		//
		CDVDAudioApp();

	public:

		//
		virtual BOOL InitInstance();

	public:

		//
		afx_msg void OnAppAbout();
		//
		DECLARE_MESSAGE_MAP()
};

extern CDVDAudioApp theApp;

#endif
