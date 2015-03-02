!include "MUI.nsh"

!define VERSION "1.0.1"
!define BUILD "875"

Var STARTMENU_FOLDER

CRCCheck on

SetCompress   force
SetCompressor lzma

!include "mui.nsh"

#!include "${NSISDIR}\Contrib\System\system.nsh"

Name "Audio SixPack v.${VERSION} Build ${BUILD}"
OutFile "audiosixpack${BUILD}.exe"

XPStyle on
ShowInstDetails show
BrandingText /TRIMLEFT "www.wholegroup.com"
InstallDir "$PROGRAMFILES\Audio SixPack"

!define MUI_ABORTWARNING

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "Documents\header.bmp"

!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\win-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\win-uninstall.ico"

!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_FINISHPAGE_RUN "$INSTDIR\DVD-Audio.exe"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\audiosixpack.chm"
!define MUI_FINISHPAGE_LINK "Visit our site for the latest news, FAQs and support" 
!define MUI_FINISHPAGE_LINK_LOCATION "http://www.wholegroup.com/our_products.php"

!define MUI_COMPONENTSPAGE_NODESC

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "Documents\License.rtf"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!define MUI_PAGE_CUSTOMFUNCTION_PRE preStartMenu
!insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Function .onInit
	FindWindow $0 "" "Audio SixPack"
	IntCmp $0 0 notwindow
		MessageBox MB_OK "Audio SixPack is running. To run Audio SixPack Install you need to first close Audio SixPack."
	notwindow:

	FindWindow $0 "" "Audio SixPack"
	IntCmp $0 0 notwindow2
		Quit
	notwindow2:

	InitPluginsDir

	File /oname=$EXEDIR\splash.bmp "Documents\splashsrc.bmp"
	advsplash::show 1000 600 400 0x6961C7 splash
	Delete $EXEDIR\splash.bmp
FunctionEnd

Function un.onInit
	FindWindow $0 "" "Audio SixPack"
	IntCmp $0 0 notwindow
		MessageBox MB_OK "Audio SixPack is running. To run Audio SixPack Uninstall you need to first close Audio SixPack."
	notwindow:

	FindWindow $0 "" "Audio SixPack"
	IntCmp $0 0 notwindow2
		Quit
	notwindow2:
FunctionEnd

Function preStartMenu
	StrCpy $STARTMENU_FOLDER "Audio SixPack"
FunctionEnd

Section "Audio SixPack (required)"
	SectionIn RO

	SetOutPath "$INSTDIR"

	File "Sources\vs2010\Release\DVD-Audio.exe"
	File "Sources\StarBurn\WnASPI32.dll"
#	File "..\mfc71.dll"
#	File "..\mfc71u.dll"
#	File "..\msvcp71.dll"
#	File "..\msvcr71.dll"
	File "Documents\AudioSixPack.chm"
	File "Documents\License.rtf"

	CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
	CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER\Readme"

	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Audio SixPack Uninstall.lnk" "$INSTDIR\Uninstall.exe"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Audio SixPack.lnk" "$INSTDIR\DVD-Audio.exe"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Audio SixPack Documentation.lnk" "$INSTDIR\AudioSixPack.chm"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Readme\License.lnk" "$INSTDIR\license.rtf"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Audio SixPack" "DisplayName" "Audio SixPack (remove only)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Audio SixPack" "UninstallString" '"$INSTDIR\uninstall.exe"'

	WriteUninstaller "$INSTDIR\Uninstall.exe"

	ClearErrors
SectionEnd

Section "Create Shortcut on Desktop"
	CreateShortCut "$DESKTOP\Audio SixPack.lnk" "$INSTDIR\DVD-Audio.exe"
SectionEnd

Section "Create a Quick Launch icon"
	CreateShortCut "$QUICKLAUNCH\Audio SixPack.lnk" "$INSTDIR\DVD-Audio.exe"
SectionEnd

Section "Uninstall"
	ClearErrors
	
	RMDir /r "$INSTDIR"

	SetOutPath "$SMPROGRAMS\$STARTMENU_FOLDER"
	SetOutPath ".."
	RMDir /r "$SMPROGRAMS\$STARTMENU_FOLDER"
	Delete "$QUICKLAUNCH\Audio SixPack.lnk"
	Delete "$DESKTOP\Audio SixPack.lnk"

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Audio SixPack"
	DeleteRegKey HKCU "Software\Whole Group\Audio SixPack"
SectionEnd


 


