#ifndef CDVDAUDIO_H
#define CDVDAUDIO_H

#define SIZEWAVHEADER    44             // размер заголовка WAV
#define MAXGROUP         9              // максимальное количество групп
#define MAXTRACK         99             // максимальное количество треков
#define MAXPSP           524272         // максимальное количество пакетов в одном AOB
#define SIZEPSP          2048           // размер PSP пакета
#define SIZEINPUTBUFFER  2048 * 1024    // размер входного буффера
#define SIZEOUTPUTBUFFER SIZEPSP * 1024 // размер выходного буффера, должен быть кратен SIZEPSP 


// указатель на CallBack-функцию 
typedef VOID (__stdcall *PDVDCALLBACK)(UINT uiMessageID, PVOID pvContext, PVOID pvSpecial);

// MessageID для CallBack-функции
#define MID_NEW_GROUP     0x01
#define MID_NEW_TRACK     0x02
#define MID_PROCESS_TRACK 0x03

// структура параметров для MID_PROCESS_TRACK
typedef struct {
	UINT32 uiNumBytes;
	UINT32 uiProcBytes;
	UINT32 uiOldBytes;
} SPROCTRACK;

// класс описания трека
class CTrackInfo
{
	// конструктор и деструктор
	public:
		CTrackInfo();
		virtual ~CTrackInfo();

	// свойства класса
	protected:
		_TCHAR* m_tcFileName;        // имя файла
		FILE*   m_fHandle;           // хендл открытого сигнала
		UINT16  m_uiChannels;        // количество каналов
		UINT32  m_uiSamplesRate;     // частота сигнала, Гц
		UINT16  m_uiSampleUnitsSize; // размер отсчета * кол-во каналов
		UINT16  m_uiBitsPerSample;   // размер отсчета в битах
		UINT32  m_uiNumBytes;        // размер сигнала в байтах
		UINT64  m_uiPTSFirst;        // 
		UINT64  m_uiPTSLength;       // 
		UINT32  m_uiFirstSector;     //
		UINT32  m_uiLastSector;      //

	// методы класса
	public:
		UINT    ReadFile(_TCHAR* tcFileName);                   // считывает указанный файл
		BOOL    IsOpen();                                       // файл открыт ?
		UINT    Open(BOOL bPassHeader = FALSE);                 // открытие файла
		UINT    Close();                                        // закрытие файла
		UINT    Read(UCHAR* ucBuffer, UINT uiSize);             // считываем запрошенное кол-во байт
		UINT16  GetNumberChannels() {return m_uiChannels;};     // возврщает число каналов
		UINT32  GetSamplesRate()    {return m_uiSamplesRate;};  // возвращает частоту сигнала
		UINT16  GetBitsPerSample()  {return m_uiBitsPerSample;} // возвращает разрядность
		UINT64  GetPTSFirst()       {return m_uiPTSFirst;}      // 
		VOID    SetPTSFirst(UINT64 uiPTSFirst)                  // 
			{m_uiPTSFirst = uiPTSFirst;}
		UINT64  GetPTSLength()      {return m_uiPTSLength;}     //
		UINT32  GetFirstSector()    {return m_uiFirstSector;}   //
		VOID    SetFirstSector(UINT32 uiFirstSector)            //
			{m_uiFirstSector = uiFirstSector;}
		UINT32  GetLastSector()     {return m_uiLastSector;}    //
		VOID    SetLastSector(UINT32 uiLastSector)              //
			{m_uiLastSector = uiLastSector;}
		_TCHAR* GetFileName() {return m_tcFileName;}            //
		UINT32  GetNumBytes() {return m_uiNumBytes;}            //
};


class CDVDAudio
{
	// конструктор и деструктор
	public:
		CDVDAudio(UINT uiFreq, UINT uiBits);
		virtual ~CDVDAudio();

	// свойства класса
	protected:
		PDVDCALLBACK CallBack;          // указатель на CallBack функцию 
		PVOID        pContext;          // контекст для CallBack функции
		UINT         m_uiFreq;          // частота, Гц
		UINT         m_uiBits;          // разрядность, в битах
		UINT         m_uiBytesPerFrame; // размер одного фрейма в байтах
		UINT         m_uiBytesPerSecond;
		UINT         m_uiGroups;        // количество групп
		UINT         m_uiTrackCount[9]; // количество треков в каждой группе
		CTrackInfo** m_trackInfo[9];    // информация о треках
		UCHAR*       m_ucInputBuffer;   // входной буфер
		UCHAR*       m_ucOutputBuffer;  // выходной буфер
		UINT         m_uiSizeInBuffer;  // текущий размер входного буфера
		UINT         m_uiSizeOutBuffer; // текущий размер выходного буфера
		UINT         m_uiStartInBuffer; // точка отсчета во входном буфере
		UINT         m_uiNumberGroup;   // номер обрабатываемой группы
		UINT         m_uiNumberTrack;   // номер обрабатываемого трека
		UINT64       m_uiPTS;           // Presentation Timestamp
		UINT64       m_uiSCR;           // System Clock Reference (SCR)
		UCHAR        m_ucLPCMCounter;   // Continuity Counter - counts from 0x00 to 0x1f and then wraps to 0x00
		UINT         m_uiNumberPSP;     // число записанных PSP пакетов в группе
		UINT64       m_uiBytesWrite;    // количество записанных байт относительно одной группы
		UINT32       m_uiProcBytes;     // количество обработанных байт относительно одного трека
		BOOL         m_bTerminate;      // флаг для прерывания работы

	// методы класса
	public:
		VOID SetCallBack(PDVDCALLBACK pDVDCallBack, PVOID pContext);   // установка указателя на CallBack функцию
		UINT AddTrack(UINT uiGroup, _TCHAR* tcFileName);               // добавить трек в группу uiGroup
		UINT CreateDVDAudio(_TCHAR* tcDirPath);                        // создать структуру диска DVDAudio
		VOID Terminate() {m_bTerminate = TRUE;}                        // прервать процесс

	private:
		UINT CreateAOB(_TCHAR* tcDirPath);                              // создать AOB файл i-ой группы
		UINT CreateATS(_TCHAR* tcDirPath);                              // создать Audio Titleset Information file
		UINT CreateAMG(_TCHAR* tcDirPath);                              // создать Audio Manager (AUDIO_TS.IFO)
		UINT CreateSAMG(_TCHAR* tcDirPath);                             // создать Simple Audio Manager (AUDIO_PP.IFO)
		UINT MakePSP(UCHAR* ucPSP, BOOL& bLastPSP, BOOL& bFirstPSP);    // формирование PSP пакета
		UINT MakePackStartCode(UCHAR* ucBuffer);                        // формирование pack_start_code
		UINT MakeSystemHeader(UCHAR* ucBuffer);                         // формирование system_header
		UINT MakePrivateStreamHeader(UCHAR* ucBuffer, BOOL& bFirstPSP); // формирование private_stream_header
		UINT MakeLPCMHeader(UCHAR* ucBuffer, UINT uiAudioBytes);        // формирование LPCM заголовка
		UINT CopyAudioData(UCHAR* ucBuffer, UINT uiSize, BOOL& bEnd);   // копирование аудиоданных
		UINT CalculateSCR();                                            // вычисление System Clock Reference (SCR)
		UINT CalculatePTS();                                            // вычисление Presentation Timestamp (PTS)
};

#endif


/*
ToDo:
	-- прилепить padding_stream в конец PSP
	-- Convert little-endian WAV samples to big-endian сделано только для 16 бит
	-- сделать правильный расчет SCR
	-- сделать правильный расчет PTS
*/