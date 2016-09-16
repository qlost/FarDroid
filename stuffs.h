#pragma once

#pragma warning (disable : 4005)
#define 	S_IFMT		61440
#define 	S_IFLNK   40960
#define 	S_IFREG   32768
#define 	S_IFBLK   24576
#define 	S_IFDIR   16384
#define 	S_IFCHR   8192
#define 	S_IFIFO   4096
#define 	S_IFSOCK  49152
#define 	S_IRUSR   0x00400
#define 	S_IWUSR   0x00200
#define 	S_IXUSR   0x00100
#define 	S_IEXEC   S_IXUSR
#define 	S_IWRITE   S_IWUSR
#define 	S_IREAD   S_IRUSR

#define   ABORT     -1
#define   SKIP     2
#define   RETRY     3

enum
{
	CodePage_Unicode,
	CodePage_ANSI,
	CodePage_OEM
};

void		MakeDirs(CString path);
bool		FileExists(const CString &path);
DWORD		GetFileSizeS(const CString &path);
CString GetVersionString();
bool WriteLine( HANDLE stream, const CString &line, int CodePage);
bool CheckForKey(const int key);
int  rnd(int maxRnd);
bool IsDevice(DWORD attr);
bool IsLink(DWORD attr);
bool IsDirectory(DWORD attr);
bool IsDirectory(uintptr_t attr);
bool IsDirectory(bool another, bool selected, int i);
bool IsDirectory( LPCTSTR sPath);
void PrepareInfoLine(const wchar_t * str, void *ansi, CString& line, CString format = _T("%s%s\n"));
void PrepareInfoLineDate(const wchar_t * str, time_t * time, CString& line, bool b64);

FILETIME UnixTimeToFileTime(time_t time);
time_t StringTimeToUnixTime( CString sDay, CString sMonth, CString sYear, CString sTime );
time_t StringTimeToUnixTime( CString sData, CString sTime );
void FileTimeToUnixTime(LPFILETIME pft, time_t* pt);
time_t *SystemTimeToUnixTime(LPSYSTEMTIME pst, time_t* pt);
CString SystemTimeToString(LPSYSTEMTIME pst);

DWORD StringToAttr( CString sAttr );
DWORD ModeToAttr( int mode );

BOOL DeletePanelItems(CString &sPath, struct PluginPanelItem *PanelItem,int ItemsNumber);

BOOL ExecuteCommandLine(const CString & command, const CString& path, const CString & parameters, bool wait);
