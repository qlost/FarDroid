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

#define SIZE_PB 1125899906842624ULL
#define SIZE_TB 1099511627776ULL
#define SIZE_GB 1073741824ULL
#define SIZE_MB 1048576ULL
#define SIZE_KB 1024ULL


#include "taskbarIcon.h"
#include "framebuffer.h"
#include <memory>
#include <vector>
#include <map>

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
  int	mode;
};

typedef CSimpleArrayEx<CFileRecord*, CFileRecord*> CFileRecords;

struct CCopyRecord
{
  CString src;
  CString dst;
  UINT64	size;
  time_t	time;
};

typedef CSimpleArrayEx<CCopyRecord*, CCopyRecord*> CCopyRecords;

const enum ProcessType { PS_COPY, PS_MOVE, PS_DELETE, PS_FB };

const enum PermissionID
{
  IDPRM_Owner = 4,
  IDPRM_Group = 6,
  IDPRM_ChownSelected = 9,
  IDPRM_RUSR = 13,
  IDPRM_WUSR,
  IDPRM_XUSR,
  IDPRM_RGRP,
  IDPRM_WGRP,
  IDPRM_XGRP,
  IDPRM_ROTH,
  IDPRM_WOTH,
  IDPRM_XOTH,
  IDPRM_SUID,
  IDPRM_SGID,
  IDPRM_SVTX,
  IDPRM_Octal = 26,
  IDPRM_None,
  IDPRM_All,
  IDPRM_ChmodSelected,
  IDPRM_Ok,
  IDPRM_Cancel,
};

static std::map<int,int> PermissionMap = {
  { IDPRM_RUSR, S_IRUSR },
  { IDPRM_WUSR, S_IWUSR },
  { IDPRM_XUSR, S_IXUSR },
  { IDPRM_RGRP, S_IRGRP },
  { IDPRM_WGRP, S_IWGRP },
  { IDPRM_XGRP, S_IXGRP },
  { IDPRM_ROTH, S_IROTH },
  { IDPRM_WOTH, S_IWOTH },
  { IDPRM_XOTH, S_IXOTH },
  { IDPRM_SUID, S_ISUID },
  { IDPRM_SGID, S_ISGID },
  { IDPRM_SVTX, S_ISVTX },
};

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
			switch (WaitForSingleObject(mutex, 10000))
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

struct CInfoSize
{
  CString path;
  unsigned long long total;
  unsigned long long used;
  unsigned long long free;
};

typedef CSimpleArrayEx<CPanelLine, CPanelLine> InfoPanelLines;
typedef CSimpleArrayEx<CInfoSize, CInfoSize> InfoSize;

class fardroid
{
private:
  int lastError;
  int handleAdbServer;

  CFileRecords  records;
  ProcessStruct m_procStruct;
  InfoPanelLine * InfoPanelLineArray;
  InfoPanelLines lines;
  InfoSize infoSize;

  CString m_currentPath;
  CString m_currentDevice;
  CString m_currentDeviceName;

  CFileRecord * GetFileRecord(LPCTSTR sFileName);
  static unsigned long long ParseSizeInfo(CString s);
  void GetDeviceInfo();
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
  BOOL ADBShellExecute(LPCTSTR sCMD, CString & sRes, bool bSilent);
  int ADBReadFramebuffer(struct fb* fb);
  static void		ADBSyncQuit(SOCKET sockADB);
  bool		ADBTransmitFile(SOCKET sockADB, LPCTSTR sFileName, time_t & mtime);
  static void ReadError(SOCKET sockADB, unsigned id, unsigned len, CString& sRes);
  bool		ADBSendFile(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString & sRes, int mode);
  static bool		ADBReadMode(SOCKET sockADB, LPCTSTR path, int &mode);
  BOOL		ADBPushFile(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString & sRes);
  bool		ADBPushDir(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString &sRes);
  void ADBPushDirGetFiles(LPCTSTR sSrc, LPCTSTR sDst, CCopyRecords& files);
  BOOL		ADBPullFile(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString & sRes, const time_t &mtime);
  void ADBPullDirGetFiles(LPCTSTR sSrc, LPCTSTR sDst, CCopyRecords& files);
  static void		CloseADBSocket(SOCKET sockADB);

  bool DeviceTest();
  //BOOL ADB_execute(LPCTSTR sCMD, CString & sRes, bool bSilent);
  BOOL ADB_ls(LPCTSTR sDir, CFileRecords & files, CString & sRes, bool bSilent);
  BOOL ADB_rm(LPCTSTR sDir, CString & sRes, bool bSilent);
  BOOL ADB_mkdir(LPCTSTR sDir, CString & sRes, bool bSilent);
  BOOL ADB_rename(LPCTSTR sSource, LPCTSTR sDest, CString& sRes);
  BOOL ADB_copy(LPCTSTR sSource, LPCTSTR sDest, CString& sRes);
  BOOL ADB_chmod(LPCTSTR sSource, LPCTSTR octal, CString& sRes);
  BOOL ADB_chown(LPCTSTR sSource, LPCTSTR user, LPCTSTR group, CString& sRes);
  BOOL ADB_pull(LPCTSTR sSrc, LPCTSTR sDst, CString & sRes, bool bSilent, const time_t& mtime);
  BOOL ADB_push(LPCTSTR sSrc, LPCTSTR sDst, CString & sRes, bool bSilent);
  BOOL ADB_findmount(LPCTSTR sFS, strvec &fs_params, CString & sRes, bool bSilent);
  BOOL ADB_mount(LPCTSTR sFS, BOOL bAsRW, CString & sRes, bool bSilent);

  CFileRecord* ParseFileLine(CString & sLine) const;
  BOOL		ReadFileList(CString & sFileList, CFileRecords & files, bool bSilent) const;
  BOOL		OpenPanel(LPCTSTR sPath, bool updateInfo, bool bSilent);

  int		DeleteFileFrom(const CString& src, bool bSilent);

  int GetItems(PluginPanelItem *PanelItem, int ItemsNumber, const CString& srcdir, const CString& dstdir, bool noPromt, bool ansYes, bool bSilent);
  int PutItems(PluginPanelItem *PanelItem, int ItemsNumber, const CString& srcdir, const CString& dstdir, bool noPromt, bool ansYes, bool bSilent);
  int DelItems(PluginPanelItem *PanelItem, int ItemsNumber, bool noPromt, bool ansYes, bool bSilent);

  void ParseMemoryInfo(CString s);
  bool GetMemoryInfo();
  void ParsePartitionInfo(CString s);
  void GetPartitionsInfo();
  int UpdateInfoLines();
  CFileRecord* ReadFileRecord(const CString& sSource);
  static CString PermissionsFileToMask(CString Permission);
public:
  bool m_bForceBreak;
  TaskBarIcon taskbarIcon;

  CString panelTitle;
  strmap params;

  fardroid(void);
  ~fardroid(void);

  HANDLE	OpenFromMainMenu();
  HANDLE	OpenFromCommandLine(const CString &cmd);

  int			ChangeDir(LPCTSTR sDir, OPERATION_MODES OpMode = OPM_NONE, bool updateInfo = false);

  static bool		DeleteFilesDialog();
  static bool		CreateDirDialog(CString &dest);
  bool DeviceNameDialog();
  static bool DeviceNameDialog(const CString& name, CString& alias);
  static bool		CopyFilesDialog(CString &dest, const wchar_t* title);
  static CString GetDeviceName(const CString & device);
  static CString GetDeviceAliasName(const CString& device);
  static CString GetDeviceCaption(const CString& device);
  int    DeviceMenu(CString &text);
  static void SetItemText(FarMenuItem* item, const CString& text);
  static void SetItemSelected(std::vector<FarMenuItem>& items, int sel);
  int		DeleteFileTo(const CString& name, bool bSilent);
  static void		DeleteRecords(CFileRecords & recs);
  static void DeleteRecords(CCopyRecords& recs);
  void		PreparePanel(struct OpenPanelInfo *Info);
  void		ChangePermissionsDialog(int selected);

  int GetFindData(struct PluginPanelItem **pPanelItem, size_t *pItemsNumber, OPERATION_MODES OpMode);
  static void FreeFindData(struct PluginPanelItem *PanelItem, int ItemsNumber);
  int GetFiles(PluginPanelItem *PanelItem, int ItemsNumber, CString &DestPath, BOOL Move, OPERATION_MODES OpMode);
  int PutFiles(PluginPanelItem *PanelItem, int ItemsNumber, CString SrcPath, BOOL Move, OPERATION_MODES OpMode);
  int DeleteFiles(PluginPanelItem *PanelItem, int ItemsNumber, OPERATION_MODES OpMode);
  int CreateDir(CString &DestPath, OPERATION_MODES OpMode);
  int Rename(CString& DestPath);
  int RenameFile(const CString& src, const CString& dst, CString& sRes);
  int GetFramebuffer();
  void Reread();

  void ShowProgressMessage();
  bool BreakProcessDialog();
  int CopyErrorDialog(LPCTSTR sTitle, CString sErr);
  int CopyDeleteErrorDialog(LPCTSTR sTitle, LPCTSTR sErr);
  static void ShowError(CString& error);
  int FileExistsDialog(LPCTSTR sName);
};

