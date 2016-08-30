#pragma once

#define MKID(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

#define ID_STAT MKID('S','T','A','T')
#define ID_LIST MKID('L','I','S','T')
#define ID_ULNK MKID('U','L','N','K')
#define ID_SEND MKID('S','E','N','D')
#define ID_RECV MKID('R','E','C','V')
#define ID_DENT MKID('D','E','N','T')
#define ID_DONE MKID('D','O','N','E')
#define ID_DATA MKID('D','A','T','A')
#define ID_OKAY MKID('O','K','A','Y')
#define ID_FAIL MKID('F','A','I','L')
#define ID_QUIT MKID('Q','U','I','T')

#define SWAP_WORD(x)  x = (x>>8) | (x<<8)
#define SWAP_DWORD(x)  x = (x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24)

#define TMP_SUFFIX ".fardroid";

#include "taskbarIcon.h"
#include "framebuffer.h"

typedef union {
	unsigned id;
	struct {
		unsigned id;
		unsigned namelen;
	} req;
	struct {
		unsigned id;
		unsigned mode;
		unsigned size;
		unsigned time;
	} stat;
	struct {
		unsigned id;
		unsigned mode;
		unsigned size;
		unsigned time;
		unsigned namelen;
	} dent;
	struct {
		unsigned id;
		unsigned size;
	} data;
	struct {
		unsigned id;
		unsigned msglen;
	} status;    
} syncmsg;

#define SYNC_DATA_MAX (64*1024)
struct syncsendbuf 
{
	unsigned id;
	unsigned size;
	char data[SYNC_DATA_MAX];
};

//чтение файла с проверкой результата
#define CHECKREADOK(rd_type) \
	res = ReadFile(hFile, &rd_type, sizeof(rd_type), &rd, NULL);\
	if (!res)\
	{\
		CloseHandle(hFile); \
		return INVALID_HANDLE_VALUE;\
	}\
	else if (rd != 0 && rd != sizeof(rd_type))\
	{\
		CloseHandle(hFile); \
		return INVALID_HANDLE_VALUE;\
	}\

//чтение файла с проверкой результата во вложенной функции
#define CHECKREADOK2(rd_type) \
	res = ReadFile(hFile, &rd_type, sizeof(rd_type), &rd, NULL);\
	if (res &&  rd == 0) \
		return ERROR_HANDLE_EOF;\
	else	if (!res)\
		return ERROR_READ_FAULT;\
	else if (rd != sizeof(rd_type))\
		return ERROR_BAD_FORMAT

//подготовка и запись строки в файл
#define WRITELINESTR(str1, str2)\
	PrepareInfoLine(str1, str2, line);\
	if (!WriteLine( stream, line, CodePage_ANSI)) goto exit

//подготовка и запись числа в файл
#define WRITELINEINT(str1, i)\
	PrepareInfoLine(str1, i, line, _T("%s%u\n"));\
	if (!WriteLine( stream, line, CodePage_ANSI)) goto exit

//подготовка и запись даты в файл
#define WRITELINEDATE64(str1, d)\
	PrepareInfoLineDate(str1, d, line, true);\
	if (!WriteLine( stream, line, CodePage_ANSI)) goto exit

//подготовка и запись даты в файл
#define WRITELINEDATE32(str1, d)\
	PrepareInfoLineDate(str1, d, line, false);\
	if (!WriteLine( stream, line, CodePage_ANSI)) goto exit


//запись в файл перевода строки
#define WRITELINEBRAKE\
	line = _T("\n");\
	if (!WriteLine( stream, line, CodePage_ANSI)) goto exit


struct CFileRecord
{
	CString filename;
	CString linkto;
	CString owner;
	CString grp;
	CString desc;
	time_t	time;
	UINT64	size;
	DWORD		attr;
};

typedef CSimpleArrayEx<CFileRecord*, CFileRecord*> CFileRecords;

struct CCopyRecord
{
  CString src;
  CString dst;
  UINT64	size;
};

typedef CSimpleArrayEx<CCopyRecord*, CCopyRecord*> CCopyRecords;

enum ProcessType { PS_COPY, PS_MOVE, PS_DELETE, PS_FB };

struct ProcessStruct
{
  HANDLE mutex;

  ProcessType pType;

  bool bSilent;
  bool bPrevState;

  CString from;
	CString to;
	CString title;

  int nPosition;
  int nTotalFiles;

  UINT64 nFileSize;
  UINT64 nTotalFileSize;
  UINT64 nTransmitted;
  UINT64 nTotalTransmitted;

  DWORD nStartTime;
  DWORD nTotalStartTime;

	ProcessStruct(): pType(), bSilent(false), bPrevState(false), nPosition(0), nTotalFiles(0), nFileSize(0), nTotalFileSize(0), nTransmitted(0), nTotalTransmitted(0), nStartTime(0), nTotalStartTime(0)
	{
	  mutex = CreateMutex(nullptr, FALSE, nullptr);
	}

  ~ProcessStruct()
	{
		CloseHandle(mutex);
		mutex = nullptr;
	}
	bool Lock() const
	{
		if (mutex)
		{
			switch (WaitForSingleObject(mutex, 1000))
			{
			case WAIT_OBJECT_0:
				return true;
			}
		}
		return false;
	}
	bool Unlock() const
	{
		if (mutex)
		{
			ReleaseMutex(mutex);
			return true;
		}
		return false;
	}
	bool Hide()
	{
		if (Lock())
		{
			bPrevState = bSilent;
			bSilent = true;
			return true;
		}
		return false;
	}
	void Restore()
	{
		bSilent = bPrevState;
		Unlock();
	}
};

struct CPanelLine
{
	CString data;
	CString text;
	BOOL separator;
};

typedef CSimpleArrayEx<CPanelLine, CPanelLine> InfoPanelLines;

class fardroid
{
private:
	int lastError;
  CFileRecords  records;
	ProcessStruct m_procStruct;
	InfoPanelLine * InfoPanelLineArray;
	InfoPanelLines lines;

	CFileRecord * GetFileRecord(LPCTSTR sFileName);
//	DWORD ParseSection (HANDLE hFile, BYTE sectionID);
//	bool ParseSectionBuf (BYTE * buf, int &bufPos, int bufSize, CFileRecords * record);
	CString m_currentPath;
  CString m_currentDevice;
  void ShowADBExecError(CString err, bool bSilent);
  static void DrawProgress(CString& sProgress, int size, double pc);
  static void DrawProgress(CString& sProgress, int size, LPCTSTR current, LPCTSTR total);
  static SOCKET	CreateADBSocket();
	SOCKET	PrepareADBSocket();
  static bool		SendADBPacket(SOCKET sockADB, void * packet, int size);
  static bool		SendADBCommand(SOCKET sockADB, LPCTSTR sCMD);
  static int			ReadADBPacket(SOCKET sockADB, void * packet, int size);
  static bool		CheckADBResponse(SOCKET sockADB);
  static bool		ReadADBSocket(SOCKET sockADB, char * buf, int bufSize);
	BOOL		ADBShellExecute(LPCTSTR sCMD, CString & sRes, bool bSilent);
  int ADBReadFramebuffer(struct fb* fb);
  static void		ADBSyncQuit(SOCKET sockADB);
	bool		ADBTransmitFile(SOCKET sockADB, LPCTSTR sFileName, time_t & mtime);
	bool		ADBSendFile(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString & sRes, int mode);
  static bool		ADBReadMode(SOCKET sockADB, LPCTSTR path, int &mode);
	BOOL		ADBPushFile(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString & sRes);
	bool		ADBPushDir(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString &sRes);
  void ADBPushDirGetFiles(LPCTSTR sSrc, LPCTSTR sDst, CCopyRecords& files);
  BOOL		ADBPullFile(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString & sRes);
	bool		ADBPullDir(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString & sRes);
  void ADBPullDirGetFiles(LPCTSTR sSrc, LPCTSTR sDst, CCopyRecords& files);
  static void		CloseADBSocket(SOCKET sockADB);

	bool DeviceTest();
	//BOOL ADB_execute(LPCTSTR sCMD, CString & sRes, bool bSilent);
	BOOL ADB_ls(LPCTSTR sDir, CFileRecords & files, CString & sRes, bool bSilent);
	BOOL ADB_rm(LPCTSTR sDir, CString & sRes, bool bSilent);
	BOOL ADB_mkdir(LPCTSTR sDir, CString & sRes, bool bSilent);
  BOOL ADB_rename(LPCTSTR sSource, LPCTSTR sDest, CString& sRes);
  BOOL ADB_pull(LPCTSTR sSrc, LPCTSTR sDst, CString & sRes, bool bSilent);
	BOOL ADB_push(LPCTSTR sSrc, LPCTSTR sDst, CString & sRes, bool bSilent);
	BOOL ADB_findmount( LPCTSTR sFS, strvec &fs_params, CString & sRes, bool bSilent );
	BOOL ADB_mount(LPCTSTR sFS, BOOL bAsRW, CString & sRes, bool bSilent);

	bool		ParseFileLineBB(CString & sLine, CFileRecords & files) const;
	bool		ParseFileLine(CString & sLine, CFileRecords & files) const;
	//bool		ParseFileLineSafe( CString & sLine );
	BOOL		ReadFileList(CString & sFileList, CFileRecords & files) const;
	BOOL		OpenPanel(LPCTSTR sPath, bool updateInfo);

	int		CopyFileFrom(const CString& src, const CString& dst, bool bSilent);
	int		CopyFileTo(const CString& src, const CString& dst, const CString& old_permissions, bool bSilent);
	int		DeleteFileFrom(const CString& src, bool bSilent);

	int GetItems(PluginPanelItem *PanelItem, int ItemsNumber, const CString& srcdir, const CString& dstdir, bool noPromt, bool ansYes, bool bSilent);
	int PutItems(PluginPanelItem *PanelItem, int ItemsNumber, const CString& srcdir, const CString& dstdir, bool noPromt, bool ansYes, bool bSilent);
	int DelItems(PluginPanelItem *PanelItem, int ItemsNumber, bool noPromt, bool ansYes, bool bSilent);

	void ParseMemoryInfo(CString s);
	void GetMemoryInfo();
	void ParsePartitionInfo(CString s);
	void GetPartitionsInfo();
	int UpdateInfoLines();
	CString GetPermissionsFile(const CString& FullFileName);
  static CString PermissionsFileToMask(CString Permission);
	bool SetPermissionsFile(const CString& FullFileName, const CString& PermissionsFile);
public:
	bool m_bForceBreak;
  TaskBarIcon taskbarIcon;

	CString fileUnderCursor;
	CString panelTitle;
	strmap params;
	fardroid(void);
	~fardroid(void);

	HANDLE	OpenFromMainMenu();
	HANDLE	OpenFromCommandLine(const CString &cmd);

	int			ChangeDir(LPCTSTR sDir, OPERATION_MODES OpMode = OPM_NONE, bool updateInfo = false);

  static bool		DeleteFilesDialog();
  static bool		CreateDirDialog(CString &dest);
  static bool		CopyFilesDialog(CString &dest, const wchar_t* title);
  static CString GetDeviceName(CString & device);
  int    DeviceMenu(CString &text);
  static void SetItemText(FarMenuItem* item, const CString& text);
  int		DeleteFileTo(const CString& name, bool bSilent);
  static void		DeleteRecords(CFileRecords & recs);
  static void DeleteRecords(CCopyRecords& recs);
  void		PreparePanel(struct OpenPanelInfo *Info);
	void		ChangePermissionsDialog();

	int GetFindData(struct PluginPanelItem **pPanelItem, size_t *pItemsNumber, OPERATION_MODES OpMode);
  static void FreeFindData(struct PluginPanelItem *PanelItem,int ItemsNumber);
	int GetFiles(PluginPanelItem *PanelItem, int ItemsNumber, CString &DestPath, BOOL Move, OPERATION_MODES OpMode);
	int PutFiles(PluginPanelItem *PanelItem, int ItemsNumber, CString SrcPath, BOOL Move, OPERATION_MODES OpMode);
	int DeleteFiles(PluginPanelItem *PanelItem, int ItemsNumber, OPERATION_MODES OpMode);
	int CreateDir(CString &DestPath, OPERATION_MODES OpMode);
  int Rename(CString& DestPath);
  int RenameFile(const CString& src, const CString& dst, bool bSilent);
  int GetFramebuffer();
  void Reread();

	void ShowProgressMessage();
  static CString FormatSpeed(int cb);
  static CString FormatTime(int time);
  bool BreakProcessDialog();
	int CopyErrorDialog(LPCTSTR sTitle, LPCTSTR sErr);
  int CopyDeleteErrorDialog(LPCTSTR sTitle, LPCTSTR sErr);
  static void ShowError(CString& error);
  int FileExistsDialog(LPCTSTR sName);
};

