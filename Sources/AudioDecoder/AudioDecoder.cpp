#include <Windows.h>
#include <tchar.h>
#include <assert.h>
#include <stdio.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#define MP3_BLOCK_SIZE   522
#define READ_BUFFER_SIZE 65536

INT _tmain(INT iArgC, _TCHAR* argv[])
{
	DWORD                dwACMVersion     = 0;
	MMRESULT             mmResult         = MMSYSERR_NOERROR;
	HACMSTREAM           hStream          = NULL;
	MPEGLAYER3WAVEFORMAT srcFmt;
	WAVEFORMATEX         dstFmt;
	HANDLE               hfIn             = NULL;
	HANDLE               hfOut            = NULL;
	ULONG                ulBufSize        = 0;
	LPBYTE               lpbMp3Buf        = NULL;
	LPBYTE               lpbWavBuf        = NULL;
	ACMSTREAMHEADER      mp3StreamHeader;
	DWORD                dwReadByte       = 0;
	LPBYTE               lpbReadBuf       = NULL;
	DWORD                dwReadBufSize    = 0;
	BOOL                 bFirstBlock      = TRUE;
	BOOL                 bEndBlock        = FALSE;
	DWORD                dwBlockSize      = 0;
	DWORD                dwWriteByte      = 0;
	DWORD                dwReadStart      = 0;
	DWORD                dwTemp           = 0;

	_tprintf(_T("AudioDecoder --- Test utility v1.0. (c) 2005, Whole Group\n\n"));

	// получение и вывод версии ACM
	dwACMVersion = acmGetVersion();
	_tprintf(_T("ACM version is %u.%.02u\n\n"), HIWORD(dwACMVersion) >> 8, HIWORD(dwACMVersion) & 0x00FF);

	__try
	{
		// вывод подсказки, если не указано имя файла
		if (iArgC < 2)
		{
			_tprintf(_T("Run:\n  AudioDecoder.exe <audio filename>\n\n"));
			_tprintf(_T("Example:\n  AudioDecoder.exe audio.wma\n  AudioDecoder.exe audio.mp3\n\n"));

			__leave;
		}

		// определение входного формата
		srcFmt.wfx.cbSize          = MPEGLAYER3_WFX_EXTRA_BYTES;
		srcFmt.wfx.wFormatTag      = WAVE_FORMAT_MPEGLAYER3;
		srcFmt.wfx.nChannels       = 2;
		srcFmt.wfx.nAvgBytesPerSec = 128 * (1024 / 8);   // not really used but must be one of 64, 96, 112, 128, 160kbps
		srcFmt.wfx.wBitsPerSample  = 0;                  // MUST BE ZERO
		srcFmt.wfx.nBlockAlign     = 1;                  // MUST BE ONE
		srcFmt.wfx.nSamplesPerSec  = 44100;              // 44.1kHz
		srcFmt.fdwFlags            = MPEGLAYER3_FLAG_PADDING_OFF;
		srcFmt.nBlockSize          = MP3_BLOCK_SIZE;     // voodoo value #1
		srcFmt.nFramesPerBlock     = 1;                  // MUST BE ONE
		srcFmt.nCodecDelay         = 1393;               // voodoo value #2
		srcFmt.wID                 = MPEGLAYER3_ID_MPEG;

		// определение выходного формата
		dstFmt.cbSize              = 0;
		dstFmt.wFormatTag          = WAVE_FORMAT_PCM;
		dstFmt.nChannels           = 2;
		dstFmt.nSamplesPerSec      = 44100;
		dstFmt.nAvgBytesPerSec     = 4 * 44100;
		dstFmt.nBlockAlign         = 4;
		dstFmt.wBitsPerSample      = 16;

		// создаем поток
		assert(NULL == hStream);
		mmResult = acmStreamOpen(&hStream, NULL, (LPWAVEFORMATEX)&srcFmt, &dstFmt, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME);
		if (MMSYSERR_NOERROR != mmResult)
		{
			_tprintf(_T("ERROR::acmStreamOpen::%d::%d\n"), mmResult, ACMERR_NOTPOSSIBLE);

			__leave;
		}

		// расчет размера буфера выходного потока
		mmResult = acmStreamSize(hStream, MP3_BLOCK_SIZE, &ulBufSize, ACM_STREAMSIZEF_SOURCE);
		if (MMSYSERR_NOERROR != mmResult)
		{
			_tprintf(_T("ERROR::acmStreamSize\n"));

			__leave;
		}

		// выделение памяти для буфферов
		lpbMp3Buf  = (LPBYTE)LocalAlloc(LPTR, MP3_BLOCK_SIZE);
		lpbWavBuf  = (LPBYTE)LocalAlloc(LPTR, ulBufSize);
		lpbReadBuf = (LPBYTE)LocalAlloc(LPTR, READ_BUFFER_SIZE);

		// подготовка заголовка к преобразованию
		RtlZeroMemory(&mp3StreamHeader, sizeof(mp3StreamHeader));
		mp3StreamHeader.cbStruct    = sizeof(mp3StreamHeader);
		mp3StreamHeader.pbSrc       = lpbMp3Buf;
		mp3StreamHeader.cbSrcLength = MP3_BLOCK_SIZE;
		mp3StreamHeader.pbDst       = lpbWavBuf;
		mp3StreamHeader.cbDstLength = ulBufSize;

		mmResult = acmStreamPrepareHeader(hStream, &mp3StreamHeader, 0);

		// открываем исходный файл
		assert(NULL == hfIn);
		hfIn = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hfIn)
		{
			_tprintf(_T("ERROR::CreateFile::%s\n"), argv[1]);

			__leave;
		}

		// создаем файл назначения
		assert(NULL == hfOut);
		hfOut = CreateFile(_T("out.wav"), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hfOut)
		{
			_tprintf(_T("ERROR::CreateFile::%s\n"), _T("out.wav"));

			__leave;
		}

		// цикл конвертирования
		while (TRUE != bEndBlock)
		{
			// чтение файла
			if (MP3_BLOCK_SIZE > (dwReadBufSize - dwReadStart))
			{
				// перенос в начало оставшейся информации
				if ((dwReadBufSize - dwReadStart) > 0)
				{
					RtlCopyMemory(lpbReadBuf, &lpbReadBuf[dwReadStart], dwReadBufSize - dwReadStart);
					dwTemp = dwReadBufSize - dwReadStart;
				}
				else
				{
					dwTemp = 0;
				}
				
				ReadFile(hfIn, &lpbReadBuf[dwTemp], READ_BUFFER_SIZE - dwTemp, &dwReadBufSize, NULL);

				if (0 == dwReadBufSize)
				{
					bEndBlock = TRUE;
				}

				dwReadBufSize = dwReadBufSize + dwTemp;
				dwReadStart   = 0;
			}

			// заполнение входного буфера
			if (MP3_BLOCK_SIZE > (dwReadBufSize - dwReadStart))
			{
				RtlZeroMemory(lpbMp3Buf, MP3_BLOCK_SIZE);
			}

			RtlCopyMemory(lpbMp3Buf, &lpbReadBuf[dwReadStart], MP3_BLOCK_SIZE);

			// преобразование
			if (bFirstBlock)
			{
				bFirstBlock = FALSE;
				mmResult = acmStreamConvert(hStream, &mp3StreamHeader, ACM_STREAMCONVERTF_START);
			}
			else
			{
				if (bEndBlock)
				{
					mmResult = acmStreamConvert(hStream, &mp3StreamHeader, ACM_STREAMCONVERTF_END);
				}
				else
				{
					mmResult = acmStreamConvert(hStream, &mp3StreamHeader, ACM_STREAMCONVERTF_BLOCKALIGN);
				}
			}

			// проверка результата
			if (MMSYSERR_NOERROR != mmResult)
			{
				_tprintf(_T("ERROR::acmStreamConvert\n"));

				__leave;
			}

			// запись в файл
			if (mp3StreamHeader.cbSrcLengthUsed > 0)
			{
				WriteFile(hfOut, lpbWavBuf, mp3StreamHeader.cbDstLengthUsed, &dwWriteByte, NULL);
/*				if (dwWriteByte != mp3StreamHeader.cbDstLengthUsed)
				{
					_tprintf(_T("ERROR::WriteFile\n"));

					__leave;
				}*/

				dwReadStart += mp3StreamHeader.cbSrcLengthUsed;
			}
			else
			{
				_tprintf(_T("ERROR::Small MP3_BLOCK_SIZE\n"));
			}
		}
	}

	__finally
	{
		if ((NULL != hfOut) && (INVALID_HANDLE_VALUE != hfOut))
		{
			CloseHandle(hfOut);
		}

		if ((NULL != hfIn) && (INVALID_HANDLE_VALUE != hfIn))
		{
			CloseHandle(hfIn);
		}

		if (NULL != hStream)
		{
			acmStreamUnprepareHeader(hStream, &mp3StreamHeader, 0);
		}

		if (NULL != lpbReadBuf)
		{
			LocalFree(lpbReadBuf);
		}

		if (NULL != lpbMp3Buf)
		{
			LocalFree(lpbMp3Buf);
		}

		if (NULL != lpbWavBuf)
		{
			LocalFree(lpbWavBuf);
		}

		if (NULL != hStream)
		{
			acmStreamClose(hStream, 0);
		}
	}

	return S_OK;
}


