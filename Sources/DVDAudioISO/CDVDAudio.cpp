#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <math.h>
#include "CDVDAudio.h"

//////////////////////////////////////////////////////////////////////////
// Конструктор CTrackInfo
//
CTrackInfo::CTrackInfo()
{
	m_fHandle            = NULL;
	m_uiChannels         = 0;
	m_uiSamplesRate      = 0;
	m_uiSampleUnitsSize  = 0;
	m_uiBitsPerSample    = 0;
	m_uiNumBytes         = 0;
	m_tcFileName         = NULL;
	m_uiPTSFirst         = 0;
	m_uiPTSLength        = 0;
	m_uiFirstSector      = 0;
	m_uiLastSector       = 0;
}


//////////////////////////////////////////////////////////////////////////
// Деструктор CTrackInfo
// 
CTrackInfo::~CTrackInfo()
{
	Close();

	if (NULL != m_tcFileName)
	{
		delete [] m_tcFileName;
	}
}


//////////////////////////////////////////////////////////////////////////
//
//
BOOL CTrackInfo::IsOpen()
{
	if (NULL == m_fHandle)
	{
		return FALSE;
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//
//
UINT CTrackInfo::Open(BOOL bPassHeader)
{
	if (!IsOpen())
	{
		m_fHandle = _tfopen(m_tcFileName, "rb");
		if (NULL == m_fHandle)
		{
			return S_FALSE;
		}

		if (TRUE == bPassHeader)
		{
			fseek(m_fHandle, 44, SEEK_SET);
		}
	}
	else
	{
		fseek(m_fHandle, 0, SEEK_SET);
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//
//
UINT CTrackInfo::Close()
{
	if (IsOpen())
	{
		fclose(m_fHandle);
	}

	m_fHandle = NULL;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// Открытие WAV файла
//
UINT CTrackInfo::ReadFile(_TCHAR* tcFileName)
{
	UCHAR ucHeader[44];

	if (NULL == tcFileName)
	{
		return S_FALSE;
	}

	// сохраняем название файла
	if (NULL != m_tcFileName)
	{
		delete [] m_tcFileName;
	}
	m_tcFileName = new _TCHAR[_tcslen(tcFileName) + 1];
	RtlCopyMemory(m_tcFileName, tcFileName, _tcslen(tcFileName));
	m_tcFileName[_tcslen(tcFileName)] = 0;

	// открываем файл
	if (S_OK != Open())
	{
		return S_FALSE;
	}

	// считываем заголовок
	RtlZeroMemory(ucHeader, sizeof(ucHeader));
	if (SIZEWAVHEADER != fread(ucHeader, 1, sizeof(ucHeader), m_fHandle))
	{
		Close();

		return S_FALSE;
	}

	// проверяем тэги
	if ((0 == RtlEqualMemory(&ucHeader, "RIFF", 4)) || (0 == RtlEqualMemory(&ucHeader[8], "WAVEfmt", 7)))
	{
		Close();

		return S_FALSE;
	}

	// инициализируем свойства
	m_uiChannels        = *(UINT16*)&ucHeader[22];
	m_uiSamplesRate     = *(UINT32*)&ucHeader[24];
	m_uiSampleUnitsSize = *(UINT16*)&ucHeader[32];
	m_uiBitsPerSample   = *(UINT16*)&ucHeader[34];
	m_uiNumBytes        = *(UINT32*)&ucHeader[40];

	Close();

	// вычисление PTSLength
	UINT64 uiNumSamples;
	uiNumSamples  = m_uiNumBytes / (2 * m_uiBitsPerSample / 8);
	m_uiPTSLength = (90000 * uiNumSamples);
	m_uiPTSLength /= m_uiSamplesRate;
	
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// Считываем запрошенную порцию байт из файла
//
UINT CTrackInfo::Read(UCHAR* ucBuffer, UINT uiSize)
{
	if (IsOpen())
	{
		return (UINT)fread(ucBuffer, 1, uiSize, m_fHandle);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// Конструктор CDVDAudio
//
CDVDAudio::CDVDAudio(UINT uiFreq, UINT uiBits)
{
	// установка частоты сигнала
	switch(uiFreq)
	{
		case 44100:
			m_uiFreq = 44100;  // 44.1 kHz
			break;

		case 88200:
			m_uiFreq = 88200;  // 88.2 kHz
			break;

		case 176400:
			m_uiFreq = 176400; // 176.4 kHz
			break;

		case 48000:
			CDVDAudio::m_uiFreq = 48000;  // 48 kHz
			break;

		case 96000:
			CDVDAudio::m_uiFreq = 96000;  // 96 kHz
			break;

		case 192000:
			CDVDAudio::m_uiFreq = 192000; // 192 kHz
			break;

		default:
			CDVDAudio::m_uiFreq = 44100;  // 44.1 kHz, default CDAudio
	}

	// установка разрядности сигнала
	switch(uiBits)
	{
		case 16:
			CDVDAudio::m_uiBits = 16;
			break;

		case 20:
			CDVDAudio::m_uiBits = 20;
			break;

		case 24:
			CDVDAudio::m_uiBits = 24;
			break;

		default:
			CDVDAudio::m_uiBits = 16;
			break;
	}

	// инициализация свойств
	CallBack = NULL;
	pContext = NULL;

	m_ucInputBuffer  = new UCHAR[SIZEINPUTBUFFER];
	m_ucOutputBuffer = new UCHAR[SIZEOUTPUTBUFFER];

	RtlZeroMemory(m_uiTrackCount, sizeof(m_uiTrackCount));
	RtlZeroMemory(m_ucInputBuffer, SIZEINPUTBUFFER);
	RtlZeroMemory(m_ucOutputBuffer, SIZEOUTPUTBUFFER);

	m_uiGroups         = 0;
	m_uiSizeInBuffer   = 0;
	m_uiSizeOutBuffer  = 0;
	m_uiStartInBuffer  = 0;
	m_uiBytesPerSecond = m_uiFreq * m_uiBits * 2 / 8;

	if ((44100 == m_uiFreq) || (48000 == m_uiFreq))
	{
		m_uiBytesPerFrame = 5 * 2 * m_uiBits;
	}
	else
	if ((88200 == m_uiFreq) || (96000 == m_uiFreq))
	{
		m_uiBytesPerFrame = 10 * 2 * m_uiBits;
	}
	else
	if ((176400 == m_uiFreq) || (192000 == m_uiFreq))
	{
		m_uiBytesPerFrame = 20 * 2 * m_uiBits;
	}

	m_bTerminate = FALSE;
}


//////////////////////////////////////////////////////////////////////////
// Деструктор CDVDAudio
// 
CDVDAudio::~CDVDAudio()
{
	for (UINT i = 0; i < m_uiGroups; i++)
	{
		for (UINT j = 0; j < m_uiTrackCount[i]; j++)
		{
			delete m_trackInfo[i][j];
		}
		delete [] m_trackInfo[i];
	}

	delete [] m_ucInputBuffer;
	delete [] m_ucOutputBuffer;
}


//////////////////////////////////////////////////////////////////////////
// Установка указателя на CallBack функцию
//
VOID CDVDAudio::SetCallBack(PDVDCALLBACK pDVDCallBack, PVOID pContext)
{
	CallBack = pDVDCallBack;
	this->pContext = pContext;
}


//////////////////////////////////////////////////////////////////////////
// Добавление трека
//
UINT CDVDAudio::AddTrack(UINT uiGroup, _TCHAR* tcFileName)
{
	// группа должна быть от 1 до MAXGROUP
	if ((uiGroup < 1) || (uiGroup > MAXGROUP))
	{
		return S_FALSE;
	}

	// треки можно заносить только в текущую или следующую группу
	if ((uiGroup - m_uiGroups) > 1)
	{
		return S_FALSE;
	}

	// в указанной группе может быть максимум MAXTRACK треков
	if (m_uiTrackCount[uiGroup - 1] >= MAXTRACK)
	{
		return S_FALSE;
	}

	// создаем новый объект трека
	CTrackInfo* newTrack = new CTrackInfo();
	if (S_OK != newTrack->ReadFile(tcFileName) || 
		(m_uiFreq != newTrack->GetSamplesRate()) ||
		(m_uiBits != newTrack->GetBitsPerSample()) || 
		(2 != newTrack->GetNumberChannels()))
	{
		delete newTrack;

		return S_FALSE;
	}

	// если группа новая, выделяем для неё память
	if (uiGroup != m_uiGroups)
	{
		m_trackInfo[uiGroup - 1] = new CTrackInfo* [MAXTRACK];

		m_uiGroups = uiGroup;
	}

	m_trackInfo[m_uiGroups - 1][m_uiTrackCount[m_uiGroups - 1]] = newTrack;
	m_uiTrackCount[m_uiGroups - 1]++;

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// Создание структуры DVDAudio
//
UINT CDVDAudio::CreateDVDAudio(_TCHAR* tcDirPath)
{
	TCHAR tcTmp[MAX_PATH];
	
	// проверка наличия групп и треков
	if ((m_uiGroups < 1) || (m_uiTrackCount[0] < 1))
	{
		return S_FALSE;
	}

	// проверка отсутствия завершающего слэша
	if (tcDirPath[_tcslen(tcDirPath) - 1] == _T('\\'))
	{
		return S_FALSE;
	}

	// создание каталога AUDIO_TS и VIDEO_TS
	_stprintf(tcTmp, _T("%s\\AUDIO_TS"), tcDirPath);
	if (!CreateDirectory(tcTmp, NULL))
	{
		if (ERROR_ALREADY_EXISTS != GetLastError())
		{
			return S_FALSE;
		}
	}

	_stprintf(tcTmp, _T("%s\\VIDEO_TS"), tcDirPath);
	if (!CreateDirectory(tcTmp, NULL))
	{
		if (ERROR_ALREADY_EXISTS != GetLastError())
		{
			return S_FALSE;
		}
	}

	// цикл по группам
	for (UINT i = 0; i < m_uiGroups; i++)
	{
		if (m_uiTrackCount[i] > 0)
		{
			m_uiStartInBuffer = 0;
			m_uiSizeInBuffer  = 0;
			m_uiSizeOutBuffer = 0;
			m_uiNumberGroup   = i + 1;
			m_uiNumberTrack   = 0;
			m_uiSCR           = 0;
			m_ucLPCMCounter   = 0;
			m_uiProcBytes     = 0;
		
			// посылаем CallBack о формировании новой группы
			if (!m_bTerminate)
			{
				CallBack(MID_NEW_GROUP, pContext, (PVOID)&m_uiNumberGroup);
			}

			// формирование AOB
			if (S_OK != CreateAOB(tcDirPath))
			{
				return S_FALSE;
			}

			// формирование Audio Titleset Information file
			if (S_OK != CreateATS(tcDirPath))
			{
				return S_FALSE;
			}
		}
	}

	// формирование Audio Manager (AUDIO_TS.IFO)
	if (S_OK != CreateAMG(tcDirPath))
	{
		return S_FALSE;
	}

	// формирование Simple Audio Manager (AUDIO_PP.IFO)
	if (S_OK != CreateSAMG(tcDirPath))
	{
		return S_FALSE;
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// Создание AOB файла для i-ой группы
//
UINT CDVDAudio::CreateAOB(_TCHAR* tcDirPath)
{
	if ((m_uiNumberGroup < 1) || (m_uiNumberGroup > MAXGROUP))
	{
		return S_FALSE;
	}

	if ((m_uiTrackCount[m_uiNumberGroup - 1] < 1) || (m_uiTrackCount[m_uiNumberGroup - 1] > MAXTRACK))
	{
		return S_FALSE;
	}

	_TCHAR  tcTmp[MAX_PATH];
	FILE*   fAOB        = NULL;
	UINT    uiNumberAOB = 0;
	BOOL    bStartAOB   = TRUE;  // флаг начала AOB файла 
	BOOL    bFirstPSP   = TRUE;  // первоначальный пакет
	BOOL    bLastPSP    = FALSE; // флаг самого последнего пакет

	m_uiNumberPSP  = 0;     // число записанных PSP пакетов
	m_uiBytesWrite = 0;
	while (!bLastPSP)
	{
		// создаем новый AOB
		if (bStartAOB)
		{
			if (uiNumberAOB)
			{
				fclose(fAOB);
				fAOB = NULL;
			}

			uiNumberAOB++;

			_stprintf(tcTmp, _T("%s\\AUDIO_TS\\ATS_%02d_%d.AOB"), tcDirPath, m_uiNumberGroup, uiNumberAOB);
			fAOB = _tfopen(tcTmp, "wb");
			if ((NULL == fAOB) || m_bTerminate)
			{
				return S_FALSE;
			}

			bStartAOB      = FALSE;
		}

		// формирование PSP
		if ((S_OK != MakePSP(&m_ucOutputBuffer[m_uiSizeOutBuffer], bLastPSP, bFirstPSP)) || m_bTerminate)
		{
			fclose(fAOB);

			return S_FALSE;
		}
		m_uiSizeOutBuffer += SIZEPSP;

		// счетчик записанных PSP пакетов
		m_uiNumberPSP++;
		if (0 == (m_uiNumberPSP % MAXPSP))
		{
			bStartAOB      = TRUE;
		}

		// скидываем выходной буфер в файл
		if (bLastPSP || bStartAOB || (SIZEOUTPUTBUFFER <= m_uiSizeOutBuffer))
		{
			if ((m_uiSizeOutBuffer != fwrite(m_ucOutputBuffer, 1, m_uiSizeOutBuffer, fAOB)) || m_bTerminate)
			{
				fclose(fAOB);

				return S_FALSE;
			}

			m_uiSizeOutBuffer = 0;
		}
	}

	fclose(fAOB);

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// Формирование PSP пакета
//
UINT CDVDAudio::MakePSP(UCHAR* ucPSP, BOOL& bLastPSP, BOOL& bFirstPSP)
{
	UINT uiAudioBytes = SIZEPSP;

	CalculatePTS();
	CalculateSCR();

	// заголовок pack_start_code
	MakePackStartCode(&ucPSP[SIZEPSP - uiAudioBytes]);
	uiAudioBytes = uiAudioBytes - 14;

	// заголовок system_header
	if (bFirstPSP)
	{
		MakeSystemHeader(&ucPSP[SIZEPSP - uiAudioBytes]);
		uiAudioBytes = uiAudioBytes - 18;
	}
	
	// заголовок private_stream
	MakePrivateStreamHeader(&ucPSP[SIZEPSP - uiAudioBytes], bFirstPSP);
	if (bFirstPSP)
	{
		uiAudioBytes = uiAudioBytes - 17;
		bFirstPSP    = FALSE;
	}
	else
	{
		uiAudioBytes = uiAudioBytes - 14;
	}

	// заголовок LPCM
	uiAudioBytes -= MakeLPCMHeader(&ucPSP[SIZEPSP - uiAudioBytes], uiAudioBytes);

	// перенос аудио данных из буфера в PSP
	m_uiBytesWrite += uiAudioBytes;
	if (S_OK != CopyAudioData(&ucPSP[SIZEPSP - uiAudioBytes], uiAudioBytes, bLastPSP))
	{
		return S_FALSE;
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// Копирование аудиоданных
//
UINT CDVDAudio::CopyAudioData(UCHAR* ucBuffer, UINT uiSize, BOOL& bEnd)
{
	UINT uiOldBytes = m_uiProcBytes;

	// если во входном буффере недостаточно информации считываем её
	if (uiSize > m_uiSizeInBuffer)
	{
		// копирование остатков в начало
		if (m_uiSizeInBuffer)
		{
			RtlCopyMemory(m_ucInputBuffer, &m_ucInputBuffer[m_uiStartInBuffer], m_uiSizeInBuffer);
		}

		m_uiStartInBuffer = 0;

		// если файл закрыт открываем
		if (!m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->IsOpen())
		{
			m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->Open(TRUE);
			m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->SetPTSFirst(585);

			if (!m_bTerminate)
			{
				CallBack(MID_NEW_TRACK, pContext, m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->GetFileName());
			}

			m_uiProcBytes = 0;
		}

		// считываем очередную порцию в буффер
		m_uiSizeInBuffer += m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->Read(&m_ucInputBuffer[m_uiSizeInBuffer], SIZEINPUTBUFFER - m_uiSizeInBuffer);

		// если информации не хватает, считываем её из следующего файла
		while ((uiSize > m_uiSizeInBuffer) && (m_uiNumberTrack < (m_uiTrackCount[m_uiNumberGroup - 1] - 1)))
		{
			m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->Close();

			SPROCTRACK sProcTrack;
			sProcTrack.uiNumBytes  = m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->GetNumBytes();
			sProcTrack.uiProcBytes = sProcTrack.uiNumBytes;
			sProcTrack.uiOldBytes  = uiOldBytes;

			if (!m_bTerminate)
			{
				CallBack(MID_PROCESS_TRACK, pContext, &sProcTrack);
			}

			m_uiNumberTrack++;

			m_uiProcBytes = uiSize - m_uiSizeInBuffer;
			uiOldBytes    = 0;

			if (!m_bTerminate)
			{
				CallBack(MID_NEW_TRACK, pContext, m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->GetFileName());
			}

			// если файл закрыт открываем
			if (!m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->IsOpen())
			{
				DOUBLE dPTS;

				dPTS = ((m_uiBytesWrite * 90000.) / (m_uiBytesPerSecond * 1.0)) + 585.0;

				m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack - 1]->SetLastSector(m_uiNumberPSP);
				m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->Open(TRUE);
				m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->SetFirstSector(m_uiNumberPSP);
				m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->SetPTSFirst((UINT64)floor(dPTS));
				m_ucLPCMCounter = 0;
			}

			// считываем очередную порцию в буффер
			m_uiSizeInBuffer += m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->Read(&m_ucInputBuffer[m_uiSizeInBuffer], SIZEINPUTBUFFER - m_uiSizeInBuffer);
		}

		// если информации не хватает, добиваем её padding_stream
		if (uiSize > m_uiSizeInBuffer)
		{
			if ((uiSize - m_uiSizeInBuffer) > 6)
			{
				m_ucInputBuffer[m_uiSizeInBuffer + 0] = 0x00; // заголовок padding_stream
				m_ucInputBuffer[m_uiSizeInBuffer + 1] = 0x00;
				m_ucInputBuffer[m_uiSizeInBuffer + 2] = 0x01;
				m_ucInputBuffer[m_uiSizeInBuffer + 3] = 0xBE;

				m_ucInputBuffer[m_uiSizeInBuffer + 4] = ((uiSize - m_uiSizeInBuffer - 6) >> 8) & 0xFF;
				m_ucInputBuffer[m_uiSizeInBuffer + 5] = (uiSize - m_uiSizeInBuffer - 6) & 0xFF;

				memset(&m_ucInputBuffer[m_uiSizeInBuffer + 6], 0xFF, uiSize - m_uiSizeInBuffer - 6);
			}
			else
			{
				memset(&m_ucInputBuffer[m_uiSizeInBuffer], 0x00, uiSize - m_uiSizeInBuffer);
			}

			m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->Close();		
			m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->SetLastSector(m_uiNumberPSP);

			m_uiSizeInBuffer = uiSize;
			bEnd             = TRUE; 
			m_uiProcBytes    = m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->GetNumBytes();
		}
	}
	else
	{
		m_uiProcBytes += uiSize;
	}

	RtlCopyMemory(ucBuffer, &m_ucInputBuffer[m_uiStartInBuffer], uiSize);
	m_uiSizeInBuffer  -= uiSize;
	m_uiStartInBuffer += uiSize;

	// Convert little-endian WAV samples to big-endian MPEG LPCM samples
	if (16 == m_uiBits)
	{
		UCHAR ucTmp;
		for (UINT i = 0; i < uiSize; i += 2)
		{
			ucTmp           = ucBuffer[i + 1];
			ucBuffer[i + 1] = ucBuffer[i];
			ucBuffer[i]     = ucTmp;
		}
	}

	SPROCTRACK sProcTrack;
	sProcTrack.uiNumBytes  = m_trackInfo[m_uiNumberGroup - 1][m_uiNumberTrack]->GetNumBytes();
	sProcTrack.uiProcBytes = m_uiProcBytes;
	sProcTrack.uiOldBytes  = uiOldBytes;
	if (!m_bTerminate)
	{
		CallBack(MID_PROCESS_TRACK, pContext, (PVOID)&sProcTrack);
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// Формирование system_header
//
UINT CDVDAudio::MakeSystemHeader(UCHAR* ucBuffer)
{
	UCHAR ucSystemHeader[18];

	RtlZeroMemory(ucSystemHeader, sizeof(ucSystemHeader));

	ucSystemHeader[0]  = 0x00; // system_header_start_code 0x000001BB
	ucSystemHeader[1]  = 0x00; // 
	ucSystemHeader[2]  = 0x01; //
	ucSystemHeader[3]  = 0xBB; //
	ucSystemHeader[4]  = 0x00; // header_length
	ucSystemHeader[5]  = 0x0C; //
	ucSystemHeader[6]  = 0x80; // marker_bit(1) + rate_bound(22) + marker_bit(1)
	ucSystemHeader[7]  = 0xC4; //
	ucSystemHeader[8]  = 0xE1; //
	ucSystemHeader[9]  = 0x04; // audio_bound(6) + fixed_flag(1) + CSPS_flag(1)
	ucSystemHeader[10] = 0xA0; // system_audio_lock_flag(1) + system_video_lock_flag(1) +
										// marker_bit(1) + video_bound(5)
	ucSystemHeader[11] = 0x7F; // packet_rate_restriction_flag(1) + reserved_byte(7)
	ucSystemHeader[12] = 0xB8; // stream_id
	ucSystemHeader[13] = 0xC0; // 
	ucSystemHeader[14] = 0x40; // 
	ucSystemHeader[15] = 0xBD; // 
	ucSystemHeader[16] = 0xE0; // 
	ucSystemHeader[17] = 0x0A; // 

	RtlCopyMemory(ucBuffer, ucSystemHeader, sizeof(ucSystemHeader));

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// Формирование pack_start_code
//
UINT CDVDAudio::MakePackStartCode(UCHAR* ucBuffer)
{
	UCHAR  ucPackStartCode[14];
	UINT64 uiSCR;
	UINT64 uiSCRBase;
	UINT16 uiSCRExt;
	UINT64 uiHS, uiLS;

	RtlZeroMemory(ucPackStartCode, sizeof(ucPackStartCode));

	uiSCR     = m_uiSCR;
	uiSCRBase = uiSCR / 300;
	uiSCRExt  = (UINT16)(uiSCR % 300);

	uiHS = (uiSCRBase >> 32) & 0x01;
	uiLS = uiSCRBase & 0xFFFFFFFF;

	ucPackStartCode[0]  = 0x00; // pack_start_code 0x000001BA
	ucPackStartCode[1]  = 0x00;
	ucPackStartCode[2]  = 0x01;
	ucPackStartCode[3]  = 0xBA;

	// system_clock_reference_base; system_clock_reference_extension
	ucPackStartCode[4]  = (UCHAR)((0x01 << 6) |  (uiHS << 05) | ((uiLS >> 27) & 0x18) | 0x04 | ((uiLS >> 28) & 0x03));
	ucPackStartCode[5]  = (UCHAR)((uiLS & 0x0FF00000) >> 20);
	ucPackStartCode[6]  = (UCHAR)(((uiLS & 0x000F8000) >> 12) | 0x04 | ((uiLS & 0x00006000) >> 13));
	ucPackStartCode[7]  = (UCHAR)((uiLS & 0x00001FE0) >> 5);
	ucPackStartCode[8]  = (UCHAR)(((uiLS & 0x0000001F) << 3) | 0x04 | ((uiSCRExt & 0x00000180) >> 7));
	ucPackStartCode[9]  = (UCHAR)(((uiSCRExt & 0x0000007F) << 1) | 1);

	ucPackStartCode[10]  = 0x01; // program_mux_rate(22) + marker_bit(1) + marker_bit(1)
	ucPackStartCode[11]  = 0x89; //
	ucPackStartCode[12]  = 0xC3; //
	ucPackStartCode[13]  = 0xF8; // reserved(5) + pack_stuffing_length(3)

	RtlCopyMemory(ucBuffer, ucPackStartCode, sizeof(ucPackStartCode));

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// Формирование private_stream_header
//
UINT CDVDAudio::MakePrivateStreamHeader(UCHAR* ucBuffer, BOOL& bFirstPSP)
{
	UINT64 uiPTS = m_uiPTS;
	UCHAR  ucPSHeader[17];

	RtlZeroMemory(ucPSHeader, sizeof(ucPSHeader));

	ucPSHeader[0]  = 0x00;     // private_stream_1
	ucPSHeader[1]  = 0x00;
	ucPSHeader[2]  = 0x01;
	ucPSHeader[3]  = 0xBD;
	ucPSHeader[6]  = 0x81;     // various flags - original_or_copy = 1
	ucPSHeader[7]  = (2 << 6); // various flags - contains pts_dts_flags and extension_flav
	ucPSHeader[9]  = (UCHAR)(0x21 | ((((uiPTS & 0xFF000000) >> 24) & 0xC0) >> 5));    
	ucPSHeader[10] = (UCHAR)(((((uiPTS & 0xFF000000) >> 24) & 0x3F) << 2) | ((((uiPTS & 0x00FF0000) >> 16) & 0xC0) >> 6));
	ucPSHeader[11] = (UCHAR)(((((uiPTS & 0x00FF0000) >> 16) & 0x3F) << 2) | ((((uiPTS & 0x0000FF00) >> 8) & 0x80) >> 6) | 1);
	ucPSHeader[12] = (UCHAR)(((((uiPTS & 0x0000FF00) >> 8) & 0x7F) << 1) | (((uiPTS & 0x000000FF) & 0x80) >> 7));
	ucPSHeader[13] = (UCHAR)((((uiPTS & 0x000000FF) & 0x7F) << 1) | 1);
	if (bFirstPSP)
	{
		ucPSHeader[4]  = (2010 & 0xFF00) >> 8; // PES packet length 
		ucPSHeader[5]  = (2010 & 0x00FF);      //                   
	
		ucPSHeader[7] |= 1;
		ucPSHeader[8]  = 0x08; // PES_header_data_length

		ucPSHeader[14] = 0x1E; // PES_extension_flags
		ucPSHeader[15] = 0x60 | ((10 & 0x1F00) >> 8);
		ucPSHeader[16] = 10 & 0xFF;

		RtlCopyMemory(ucBuffer, ucPSHeader, 17);
	}
	else
	{
		ucPSHeader[4]  = (2028 & 0xFF00) >> 8; // PES packet length 
		ucPSHeader[5]  = (2028 & 0x00FF);      //                   

		ucPSHeader[8]  = 0x05; // PES_header_data_length

		RtlCopyMemory(ucBuffer, ucPSHeader, 14);
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// Формирование LPCM заголовка
//
UINT CDVDAudio::MakeLPCMHeader(UCHAR* ucBuffer, UINT uiAudioBytes)
{
	UCHAR ucLPCMHeader[32];
	UINT  uiSamples = (uiAudioBytes - 12) / (m_uiBits / 4);
	UINT  uiZero    = (uiAudioBytes - 12) - (m_uiBits / 4) * uiSamples;
	UINT  uiFrames  = (UINT)(m_uiBytesWrite / m_uiBytesPerFrame);
	UINT  uiOffset;

	// не понятно, откуда берется размер заголовка... ??? попробывать потом закоментировать
	if (0 != m_uiBytesWrite)
	{
		uiZero = 0x10 - 8;
	}

	if ((uiFrames * m_uiBytesPerFrame) < m_uiBytesWrite)
	{
		uiFrames++;
	}
	uiOffset = (UINT)((uiFrames * m_uiBytesPerFrame) - m_uiBytesWrite) + 8 + uiZero - 1;

	ucLPCMHeader[0] = 0xA0;                         // SubStreamID
	ucLPCMHeader[1] = m_ucLPCMCounter;              // Continuity Counter
	ucLPCMHeader[2] = ((8 + uiZero) & 0xFF00) >> 8; // Header Length
	ucLPCMHeader[3] = (8 + uiZero) & 0xFF;
	ucLPCMHeader[4] = (uiOffset & 0xFF00) >> 8;     // Byte pointer to start of first audio frame.
	ucLPCMHeader[5] = uiOffset & 0xFF;
	ucLPCMHeader[6] = 0x10;
	switch(m_uiBits)
	{
		case 16: ucLPCMHeader[7] = 0x0F; break;
		case 20: ucLPCMHeader[7] = 0x1F; break;
		case 24: ucLPCMHeader[7] = 0x2F; break;
	}
	switch(m_uiFreq)
	{
		case 48000 : ucLPCMHeader[8] = 0x0F; break;
		case 96000 : ucLPCMHeader[8] = 0x1F; break;
		case 192000: ucLPCMHeader[8] = 0x2F; break;
		case 44100 : ucLPCMHeader[8] = 0x8F; break;
		case 88200 : ucLPCMHeader[8] = 0x9F; break;
		case 176400: ucLPCMHeader[8] = 0xAF; break;
	}	
	ucLPCMHeader[9]  = 0x00; // Unknown - e.g. 0x00
	ucLPCMHeader[10] = 0x01; // Channel Group Assignment (see below).
	ucLPCMHeader[11] = 0x80; // Unknown - e.g. 0x80

	RtlZeroMemory(&ucLPCMHeader[12], uiZero);
	RtlCopyMemory(ucBuffer, ucLPCMHeader, 12 + uiZero);

	m_ucLPCMCounter++;
	if (m_ucLPCMCounter > 0x1F)
	{
		m_ucLPCMCounter = 0;
	}

	return (12 + uiZero);
}


//////////////////////////////////////////////////////////////////////////
// создать Audio Titleset Information file
//
UINT CDVDAudio::CreateATS(_TCHAR* tcDirPath)
{
	UCHAR ucATSI[2048 * 3];
	UINT  uiOffset;

	RtlZeroMemory(ucATSI, sizeof(ucATSI));
	RtlCopyMemory(&ucATSI[0], "DVDAUDIO-ATS", 12);

	ucATSI[32]  = 0x00; // DVD Specifications version
	ucATSI[33]  = 0x12;
	ucATSI[128] = 0x00; // End byte address of ATSI_MAT
	ucATSI[129] = 0x00;
	ucATSI[130] = 0x07;
	ucATSI[131] = 0xFF;
	ucATSI[204] = 0x00;
	ucATSI[205] = 0x00;
	ucATSI[206] = 0x00;
	ucATSI[207] = 0x01;

	uiOffset = 256;

	for (UINT i = 0; i < 1; i++)
	{
		ucATSI[uiOffset]     = 0x00;
		ucATSI[uiOffset + 1] = 0x00;

		uiOffset += 2;

		switch(m_uiBits)
		{
			case 16: ucATSI[uiOffset] = 0x0F; break;
			case 20: ucATSI[uiOffset] = 0x1F; break;
			case 24: ucATSI[uiOffset] = 0x2F; break;
		}

		uiOffset++;

		switch(m_uiFreq)
		{
			case 48000 : ucATSI[uiOffset] = 0x0F; break;
			case 96000 : ucATSI[uiOffset] = 0x1F; break;
			case 192000: ucATSI[uiOffset] = 0x2F; break;
			case 44100 : ucATSI[uiOffset] = 0x8F; break;
			case 88200 : ucATSI[uiOffset] = 0x9F; break;
			case 176400: ucATSI[uiOffset] = 0xAF; break;
		}	

		uiOffset++;
		ucATSI[uiOffset] = 0x01;
		uiOffset++;
		ucATSI[uiOffset] = 0x00;
		uiOffset++;
		uiOffset += 10;
	}

	// I have no idea what the following info is for:
	ucATSI[384] = 0x00;
	ucATSI[385] = 0x00;
	ucATSI[386] = 0x1E;
	ucATSI[387] = 0xFF;
	ucATSI[388] = 0xFF;
	ucATSI[389] = 0x1E;
	ucATSI[390] = 0x2D;
	ucATSI[391] = 0x2D;
	ucATSI[392] = 0x3C;
	ucATSI[393] = 0xFF;
	ucATSI[394] = 0xFF;
	ucATSI[395] = 0x3C;
	ucATSI[396] = 0x4B;
	ucATSI[397] = 0x4B;
	ucATSI[398] = 0x00;
	ucATSI[399] = 0x00;

	uiOffset = 0x800;
	ucATSI[uiOffset]     = (1 & 0xFF00) >> 8;
	ucATSI[uiOffset + 1] = (1 & 0xFF);
	uiOffset += 2; 
	
	uiOffset += 2; // Padding
	uiOffset += 4;

	for (int i = 0; i < 1; i++)
	{
		ucATSI[uiOffset]     = 0x80 + (i + 1);
		ucATSI[uiOffset + 1] = 0x00;
		uiOffset += 2;
		
		ucATSI[uiOffset]     = 0x00;
		ucATSI[uiOffset + 1] = 0x00;
		uiOffset += 2;

		uiOffset += 4; // To be filled later - pointer to a following table.
	}

	for (int i = 0; i < 1; i++)
	{
		ucATSI[0x808 + 8 * i + 4] = ((uiOffset - 0x800) & 0xFF000000) >> 24;
		ucATSI[0x808 + 8 * i + 5] = ((uiOffset - 0x800) & 0x00FF0000) >> 16;
		ucATSI[0x808 + 8 * i + 6] = ((uiOffset - 0x800) & 0x0000FF00) >> 8;
		ucATSI[0x808 + 8 * i + 7] = (uiOffset - 0x800) & 0x000000FF;

		ucATSI[uiOffset]     = 0x00; // Unknown
		ucATSI[uiOffset + 1] = 0x00;
		uiOffset += 2;

		ucATSI[uiOffset]     = m_uiTrackCount[m_uiNumberGroup - 1]; 
		ucATSI[uiOffset + 1] = m_uiTrackCount[m_uiNumberGroup - 1];
		uiOffset += 2;

		UINT64 uiTitleLength = 0;
		for (UINT j = 0; j < m_uiTrackCount[m_uiNumberGroup - 1]; j++)
		{
			 uiTitleLength +=	m_trackInfo[m_uiNumberGroup - 1][j]->GetPTSLength();
		}

		ucATSI[uiOffset]     = (UCHAR)((uiTitleLength & 0xFF000000) >> 24); 
		ucATSI[uiOffset + 1] = (UCHAR)((uiTitleLength & 0x00FF0000) >> 16);
		ucATSI[uiOffset + 2] = (UCHAR)((uiTitleLength & 0x0000FF00) >> 8);
		ucATSI[uiOffset + 3] = (UCHAR)((uiTitleLength & 0x000000FF));
		uiOffset += 4;

		ucATSI[uiOffset]     = 0x00; // Unknown
		ucATSI[uiOffset + 1] = 0x00;
		uiOffset += 2;

		ucATSI[uiOffset]     = 0x00; // Pointer to PTS table
		ucATSI[uiOffset + 1] = 0x10;
		uiOffset += 2;

		ucATSI[uiOffset]     = ((16 + 20 * m_uiTrackCount[m_uiNumberGroup - 1]) & 0xFF00) >> 8; // Pointer to sector table
		ucATSI[uiOffset + 1] = ((16 + 20 * m_uiTrackCount[m_uiNumberGroup - 1]) & 0x00FF);
		uiOffset += 2;

		ucATSI[uiOffset]     = 0x00; // ?? Pointer to a third table (present in commercial DVD-As)
		ucATSI[uiOffset + 1] = 0x00;
		uiOffset += 2;

		// Timestamp and sector records
		for (int j = 0; j < m_uiTrackCount[m_uiNumberGroup - 1]; j++)
		{
			if (0 == j)
			{
				ucATSI[uiOffset]     = 0xC0; 
			}
			else
			{
				ucATSI[uiOffset]     = 0x00; 
			}
			ucATSI[uiOffset + 1] = 0x10;
			uiOffset += 2;

			ucATSI[uiOffset]     = 0x00;
			ucATSI[uiOffset + 1] = 0x00;
			uiOffset += 2;

			ucATSI[uiOffset]     = j + 1;
			ucATSI[uiOffset + 1] = 0x00;
			uiOffset += 2;

			ucATSI[uiOffset]     = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetPTSFirst() & 0xFF000000) >> 24); 
			ucATSI[uiOffset + 1] = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetPTSFirst() & 0x00FF0000) >> 16);
			ucATSI[uiOffset + 2] = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetPTSFirst() & 0x0000FF00) >> 8);
			ucATSI[uiOffset + 3] = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetPTSFirst() & 0x000000FF));
			uiOffset += 4;

			ucATSI[uiOffset]     = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetPTSLength() & 0xFF000000) >> 24); 
			ucATSI[uiOffset + 1] = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetPTSLength() & 0x00FF0000) >> 16);
			ucATSI[uiOffset + 2] = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetPTSLength() & 0x0000FF00) >> 8);
			ucATSI[uiOffset + 3] = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetPTSLength() & 0x000000FF));
			uiOffset += 4;

			uiOffset += 6; // Padding
		}

		// Sector pointer records
		for (int j = 0; j < m_uiTrackCount[m_uiNumberGroup - 1]; j++)
		{
			ucATSI[uiOffset]     = 0x01;
			uiOffset += 4;

			ucATSI[uiOffset]     = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetFirstSector() & 0xFF000000) >> 24); 
			ucATSI[uiOffset + 1] = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetFirstSector() & 0x00FF0000) >> 16);
			ucATSI[uiOffset + 2] = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetFirstSector() & 0x0000FF00) >> 8);
			ucATSI[uiOffset + 3] = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetFirstSector() & 0x000000FF));
			uiOffset += 4;

			ucATSI[uiOffset]     = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetLastSector() & 0xFF000000) >> 24); 
			ucATSI[uiOffset + 1] = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetLastSector() & 0x00FF0000) >> 16);
			ucATSI[uiOffset + 2] = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetLastSector() & 0x0000FF00) >> 8);
			ucATSI[uiOffset + 3] = (UCHAR)((m_trackInfo[m_uiNumberGroup - 1][j]->GetLastSector() & 0x000000FF));
			uiOffset += 4;
		}
	}

	ucATSI[0x0804]     = ((uiOffset - 0x801) & 0xFF000000) >> 24;
	ucATSI[0x0804 + 1] = ((uiOffset - 0x801) & 0x00FF0000) >> 16;
	ucATSI[0x0804 + 2] = ((uiOffset - 0x801) & 0x0000FF00) >> 8;
	ucATSI[0x0804 + 3] = ((uiOffset - 0x801) & 0x000000FF);

	UINT uiATSISector = 0;

	if (uiOffset > 2048)
	{
		uiATSISector = 3;
	}
	else
	{
		uiATSISector = 2;
	}

	// Pointer to last sector in ATS (i.e. numsectors-1)
	UINT32 uiLastSector;
	uiLastSector = m_trackInfo[m_uiNumberGroup - 1][m_uiTrackCount[m_uiNumberGroup - 1] - 1]->GetLastSector();
	uiLastSector += uiATSISector * 2;
	ucATSI[12] = ((uiLastSector) & 0xFF000000) >> 24;
	ucATSI[13] = ((uiLastSector) & 0x00FF0000) >> 16;
	ucATSI[14] = ((uiLastSector) & 0x0000FF00) >> 8;
	ucATSI[15] = ((uiLastSector) & 0x000000FF);

	// Last sector in ATSI
	ucATSI[28] = 0x00;
	ucATSI[29] = 0x00;
	ucATSI[30] = 0x00;
	ucATSI[31] = uiATSISector - 1;

	// Start sector of ATST_AOBS
	ucATSI[196] = 0x00;
	ucATSI[197] = 0x00;
	ucATSI[198] = 0x00;
	ucATSI[199] = uiATSISector;

	_TCHAR tcTmp[MAX_PATH];
	FILE*  fOut;

	_stprintf(tcTmp, _T("%s\\AUDIO_TS\\ATS_%02d_0.IFO"), tcDirPath, m_uiNumberGroup);
	fOut = _tfopen(tcTmp, "wb");
	if (NULL == fOut)
	{
		return S_FALSE;
	}
	fwrite(ucATSI, 1, 2048 * uiATSISector, fOut);
	fclose(fOut);

	_stprintf(tcTmp, _T("%s\\AUDIO_TS\\ATS_%02d_0.BUP"), tcDirPath, m_uiNumberGroup);
	fOut = _tfopen(tcTmp, "wb");
	if (NULL == fOut)
	{
		return S_FALSE;
	}
	fwrite(ucATSI, 1, 2048 * uiATSISector, fOut);
	fclose(fOut);

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// создать Audio Manager (AUDIO_TS.IFO)
//
UINT CDVDAudio::CreateAMG(_TCHAR* tcDirPath)
{
	UCHAR ucAMG[2048 * 3];
	UINT  uiOffset;
	UINT  uiTitleSet;

	// подсчет общего количества треков во всех группах
	uiTitleSet = 0;
	for (UINT i = 0; i < m_uiGroups; i++)
	{
		uiTitleSet++;
	}

	RtlZeroMemory(ucAMG, sizeof(ucAMG));
	RtlCopyMemory(ucAMG, "DVDAUDIO-AMG", 12);

	ucAMG[12]   = 0x00; // Last sector in AMG
	ucAMG[13]   = 0x00; 
	ucAMG[14]   = 0x00; 
	ucAMG[15]   = 0x05; 
	ucAMG[28]   = 0x00; // Last sector in AMG
	ucAMG[29]   = 0x00; 
	ucAMG[30]   = 0x00; 
	ucAMG[31]   = 0x02; 
	ucAMG[32]   = 0x00; // DVD Specifications
	ucAMG[33]   = 0x12; 
	ucAMG[38]   = 0x00; // Number of Volumes
	ucAMG[39]   = 0x01; 
	ucAMG[40]   = 0x00; // Current Volume
	ucAMG[41]   = 0x01; 
	ucAMG[42]   = 0x01; // Disc Side
	ucAMG[47]   = 0x01; // ??? Maybe something to do with SV ??
	ucAMG[48]   = 0x00; // Sector Pointer to AUDIO_SV.IFO
	ucAMG[49]   = 0x00; 
	ucAMG[50]   = 0x00; 
	ucAMG[51]   = 0x00; 
	ucAMG[62]   = 0x00; // Number of video titlesets
	ucAMG[63]   = m_uiGroups; // Number of audio titlesets
	ucAMG[128]  = 0x00; 
	ucAMG[129]  = 0x00; 
	ucAMG[130]  = 0x07; 
	ucAMG[131]  = 0xFF; 
	ucAMG[0xC4] = 0x00; // Pointer to sector 2
	ucAMG[0xC5] = 0x00; 
	ucAMG[0xC6] = 0x00; 
	ucAMG[0xC7] = 0x01; 
	ucAMG[0xC8] = 0x00; // Pointer to sector 3
	ucAMG[0xC9] = 0x00; 
	ucAMG[0xCA] = 0x00; 
	ucAMG[0xCB] = 0x02; 

	// Sector 2

	uiOffset = 0x800;

	ucAMG[uiOffset]     = (uiTitleSet & 0xFF00) >> 8; 
	ucAMG[uiOffset + 1] = (uiTitleSet & 0x00FF); 
	uiOffset += 2;
	ucAMG[uiOffset]     = ((4 + 14 * uiTitleSet - 1) & 0xFF00) >> 8; 
	ucAMG[uiOffset +1 ] = ((4 + 14 * uiTitleSet - 1) & 0x00FF); 
	uiOffset += 2;

	UINT uiSectorOffset = 6; 
	for (int i = 0; i < m_uiNumberGroup; i++)
	{
		ucAMG[uiOffset] = 0x80 | (i + 1);
		uiOffset++;
		ucAMG[uiOffset] = m_uiTrackCount[i];
		uiOffset++;
		uiOffset += 2;

		UINT64 uiTitleLength = 0;
		for (UINT j = 0; j < m_uiTrackCount[i]; j++)
		{
			uiTitleLength +=	m_trackInfo[i][j]->GetPTSLength();
		}

		ucAMG[uiOffset]     = (UCHAR)((uiTitleLength & 0xFF000000) >> 24); 
		ucAMG[uiOffset + 1] = (UCHAR)((uiTitleLength & 0x00FF0000) >> 16);
		ucAMG[uiOffset + 2] = (UCHAR)((uiTitleLength & 0x0000FF00) >> 8);
		ucAMG[uiOffset + 3] = (UCHAR)((uiTitleLength & 0x000000FF));
		uiOffset += 4;
		ucAMG[uiOffset] = (i + 1);
		uiOffset ++;
		ucAMG[uiOffset] = 1;
		uiOffset ++;

		ucAMG[uiOffset]     = (UCHAR)((uiSectorOffset & 0xFF000000) >> 24); 
		ucAMG[uiOffset + 1] = (UCHAR)((uiSectorOffset & 0x00FF0000) >> 16);
		ucAMG[uiOffset + 2] = (UCHAR)((uiSectorOffset & 0x0000FF00) >> 8);
		ucAMG[uiOffset + 3] = (UCHAR)((uiSectorOffset & 0x000000FF));
		uiOffset += 4;

		uiSectorOffset += m_trackInfo[i][m_uiTrackCount[i] - 1]->GetLastSector() + 1 + 3 + 3;
	}

	// Sector 3

	RtlCopyMemory(&ucAMG[4096], &ucAMG[2048], 2048);

	_TCHAR tcTmp[MAX_PATH];
	FILE*  fOut;

	_stprintf(tcTmp, _T("%s\\AUDIO_TS\\AUDIO_TS.IFO"), tcDirPath);
	fOut = _tfopen(tcTmp, "wb");
	if (NULL == fOut)
	{
		return S_FALSE;
	}
	fwrite(ucAMG, 1, sizeof(ucAMG), fOut);
	fclose(fOut);

	_stprintf(tcTmp, _T("%s\\AUDIO_TS\\AUDIO_TS.BUP"), tcDirPath);
	fOut = _tfopen(tcTmp, "wb");
	if (NULL == fOut)
	{
		return S_FALSE;
	}
	fwrite(ucAMG, 1, sizeof(ucAMG), fOut);
	fclose(fOut);

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// создать Simple Audio Manager (AUDIO_PP.IFO)
//
UINT CDVDAudio::CreateSAMG(_TCHAR* tcDirPath)
{
	UCHAR ucSAMG[2048 * 8];
	UINT  uiSectorOffset;
	UINT  uiOffset;
	UINT  i, j;

	RtlZeroMemory(ucSAMG, sizeof(ucSAMG));
	RtlCopyMemory(ucSAMG, "DVDAUDIOSAPP", 12);

	// sizeof(AUDIO_PP.IFO) + sizeof(AUDIO_TS.IFO) + sizeof(AUDIO_TS.BUP) + sizeof(ATS_01_1.IFO)
	uiSectorOffset = 281 + 64 + 3 + 3 + 3;

	// Подсчет количества треков всех групп
	UINT uiCountTrack = 0;
	for (i = 0; i < m_uiGroups; i++)
	{
		uiCountTrack += m_uiTrackCount[i];
	}

	ucSAMG[12] = (uiCountTrack & 0xFF00) >> 8;
	ucSAMG[13] = uiCountTrack & 0xFF;
	ucSAMG[14] = 0x00;
	ucSAMG[15] = 0x12;

	uiOffset = 16;
	for (i = 0; i < m_uiGroups; i++)
	{
		for (j = 0; j < m_uiTrackCount[i]; j++)
		{
			uiOffset += 2;
			ucSAMG[uiOffset] = i + 1;
			uiOffset++;
			ucSAMG[uiOffset] = j + 1;
			uiOffset++;
			ucSAMG[uiOffset]     = (UCHAR)((m_trackInfo[i][j]->GetPTSFirst() & 0xFF000000) >> 24); 
			ucSAMG[uiOffset + 1] = (UCHAR)((m_trackInfo[i][j]->GetPTSFirst() & 0x00FF0000) >> 16);
			ucSAMG[uiOffset + 2] = (UCHAR)((m_trackInfo[i][j]->GetPTSFirst() & 0x0000FF00) >> 8);
			ucSAMG[uiOffset + 3] = (UCHAR)((m_trackInfo[i][j]->GetPTSFirst() & 0x000000FF));
			uiOffset += 4;
			ucSAMG[uiOffset]     = (UCHAR)((m_trackInfo[i][j]->GetPTSLength() & 0xFF000000) >> 24); 
			ucSAMG[uiOffset + 1] = (UCHAR)((m_trackInfo[i][j]->GetPTSLength() & 0x00FF0000) >> 16);
			ucSAMG[uiOffset + 2] = (UCHAR)((m_trackInfo[i][j]->GetPTSLength() & 0x0000FF00) >> 8);
			ucSAMG[uiOffset + 3] = (UCHAR)((m_trackInfo[i][j]->GetPTSLength() & 0x000000FF));
			uiOffset += 4;
			uiOffset += 4;
			if (0 == j)
			{
				ucSAMG[uiOffset] = 0xC8;
			}
			else
			{
				ucSAMG[uiOffset] = 0x48;
			}
			uiOffset++;
			switch (m_uiBits)
			{
				case 16: ucSAMG[uiOffset] = 0x0F; break;
				case 20: ucSAMG[uiOffset] = 0x1F; break;
				case 24: ucSAMG[uiOffset] = 0x2F; break;
			}
			uiOffset++;
			switch (m_uiFreq)
			{
				case 48000:  ucSAMG[uiOffset] = 0x0F; break;
				case 96000:  ucSAMG[uiOffset] = 0x1F; break;
				case 192000: ucSAMG[uiOffset] = 0x2F; break;
				case 44100:  ucSAMG[uiOffset] = 0x8F; break;
				case 88200:  ucSAMG[uiOffset] = 0x9F; break;
				case 176400: ucSAMG[uiOffset] = 0xAF; break;
			}
			uiOffset++;
			ucSAMG[uiOffset] = 1;
			uiOffset++;
			uiOffset += 20;
			ucSAMG[uiOffset]     = (UCHAR)(((uiSectorOffset + m_trackInfo[i][j]->GetFirstSector()) & 0xFF000000) >> 24); 
			ucSAMG[uiOffset + 1] = (UCHAR)(((uiSectorOffset + m_trackInfo[i][j]->GetFirstSector()) & 0x00FF0000) >> 16);
			ucSAMG[uiOffset + 2] = (UCHAR)(((uiSectorOffset + m_trackInfo[i][j]->GetFirstSector()) & 0x0000FF00) >> 8);
			ucSAMG[uiOffset + 3] = (UCHAR)(((uiSectorOffset + m_trackInfo[i][j]->GetFirstSector()) & 0x000000FF));
			uiOffset += 4;
			ucSAMG[uiOffset]     = (UCHAR)(((uiSectorOffset + m_trackInfo[i][j]->GetFirstSector()) & 0xFF000000) >> 24); 
			ucSAMG[uiOffset + 1] = (UCHAR)(((uiSectorOffset + m_trackInfo[i][j]->GetFirstSector()) & 0x00FF0000) >> 16);
			ucSAMG[uiOffset + 2] = (UCHAR)(((uiSectorOffset + m_trackInfo[i][j]->GetFirstSector()) & 0x0000FF00) >> 8);
			ucSAMG[uiOffset + 3] = (UCHAR)(((uiSectorOffset + m_trackInfo[i][j]->GetFirstSector()) & 0x000000FF));
			uiOffset += 4;
			ucSAMG[uiOffset]     = (UCHAR)(((uiSectorOffset + m_trackInfo[i][j]->GetLastSector()) & 0xFF000000) >> 24); 
			ucSAMG[uiOffset + 1] = (UCHAR)(((uiSectorOffset + m_trackInfo[i][j]->GetLastSector()) & 0x00FF0000) >> 16);
			ucSAMG[uiOffset + 2] = (UCHAR)(((uiSectorOffset + m_trackInfo[i][j]->GetLastSector()) & 0x0000FF00) >> 8);
			ucSAMG[uiOffset + 3] = (UCHAR)(((uiSectorOffset + m_trackInfo[i][j]->GetLastSector()) & 0x000000FF));
			uiOffset += 4;
		}

		uiSectorOffset += m_trackInfo[i][j - 1]->GetLastSector() + 1 + 3 + 3;
	}

	_TCHAR tcTmp[MAX_PATH];
	FILE*  fOut;

	_stprintf(tcTmp, _T("%s\\AUDIO_TS\\AUDIO_PP.IFO"), tcDirPath);
	fOut = _tfopen(tcTmp, "wb");
	if (NULL == fOut)
	{
		return S_FALSE;
	}
	for (i = 0; i < 8; i++)
	{
		fwrite(ucSAMG, 1, sizeof(ucSAMG), fOut);
	}
	fclose(fOut);

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// вычисление System Clock Reference (SCR)
//
UINT CDVDAudio::CalculateSCR()
{
	DOUBLE dSCR = 0.;

	if (m_uiNumberPSP <= 4)
	{
		dSCR = m_uiNumberPSP * (2048.0 * 8.0 * 90000.0 * 300.0) / 10080000.0;
	}
	else
	{
		dSCR = 4 * (2048.0 * 8.0 * 90000.0 * 300.0) / 10080000.0;
	}

	if (16 == m_uiBits)
	{
		if (m_uiNumberPSP >= 4)
		{
			dSCR += ((1984.0 * 300.0 * 90000.0) / m_uiBytesPerSecond) - 42.85;
		}
		if (m_uiNumberPSP >= 5)
		{
			dSCR += (m_uiNumberPSP - 4.0) * ((2000.0 * 300.0 * 90000.0) / m_uiBytesPerSecond);
		}
	}
	else
	{
		if (m_uiNumberPSP >= 4)
		{
			dSCR += ((1980.0 * 300.0 * 90000.0) / m_uiBytesPerSecond) - 42.85;
		}
		if (m_uiNumberPSP >= 5)
		{
			dSCR += (m_uiNumberPSP - 4.0) * ((2004.0 * 300.0 * 90000.0) / m_uiBytesPerSecond);
		}
	}

	dSCR    = floor(dSCR);
	m_uiSCR = (UINT64)dSCR;
   
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// вычисление Presentation Timestamp (PTS)
//
UINT CDVDAudio::CalculatePTS()
{
	DOUBLE dPTS;
	UINT   uiFrames = (UINT)(m_uiBytesWrite / m_uiBytesPerFrame);

	if (0 == m_uiNumberPSP)
	{
		dPTS = 585.0;
	}
	else
	{
		if ((uiFrames * m_uiBytesPerFrame) < m_uiBytesWrite)
		{
			uiFrames++;
		}

		dPTS = ((uiFrames * m_uiBytesPerFrame * 90000.0) / (m_uiBytesPerSecond * 1.0)) + 585.0;
	}

	dPTS    = floor(dPTS);
	m_uiPTS = (UINT64)dPTS;

	return S_OK;
}

