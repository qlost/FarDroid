#pragma once

#pragma warning (disable : 4005)
/* Traditional mask definitions for st_mode. */
#define S_IFMT   0170000	/* type of file */
#define S_IFSOCK 0140000  /* socket */
#define S_IFLNK  0120000	/* symbolic link */
#define S_IFREG  0100000	/* regular */
#define S_IFBLK  0060000	/* block special */
#define S_IFDIR  0040000	/* directory */
#define S_IFCHR  0020000	/* character special */
#define S_IFIFO  0010000	/* this is a FIFO */

/* POSIX masks for st_mode. */
#define S_IRUSR   00400		/* owner:  r-------- */
#define S_IWUSR   00200		/* owner:  -w------- */
#define S_IXUSR   00100		/* owner:  --x------ */
#define S_IRWXU   (S_IRUSR | S_IWUSR | S_IXUSR)

#define S_IRGRP   00040		/* group:  ---r----- */
#define S_IWGRP   00020		/* group:  ----w---- */
#define S_IXGRP   00010		/* group:  -----x--- */
#define S_IRWXG   (S_IRGRP | S_IWGRP | S_IXGRP)

#define S_IROTH   00004		/* others: ------r-- */ 
#define S_IWOTH   00002		/* others: -------w- */
#define S_IXOTH   00001		/* others: --------x */
#define S_IRWXO   (S_IROTH | S_IWOTH | S_IXOTH)

#define S_ISUID  0004000	/* set user id on execution */
#define S_ISGID  0002000	/* set group id on execution */
#define S_ISVTX  0001000	/* save swapped text even after use */

#define S_ISRWX  (S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX)

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
bool IsLinkMode(int attr);
bool IsLink(DWORD attr);
bool IsDirectoryMode(int attr);
bool IsDirectory(DWORD attr);
bool IsDirectory(uintptr_t attr);
bool IsDirectoryLocal(LPCTSTR sPath);
void PrepareInfoLine(const wchar_t * str, void *ansi, CString& line, CString format = _T("%s%s\n"));
void PrepareInfoLineDate(const wchar_t * str, time_t * time, CString& line, bool b64);

FILETIME UnixTimeToFileTime(time_t time);
time_t StringTimeToUnixTime( CString sDay, CString sMonth, CString sYear, CString sTime );
time_t StringTimeToUnixTime( CString sData, CString sTime );
void FileTimeToUnixTime(LPFILETIME pft, time_t* pt);
time_t *SystemTimeToUnixTime(LPSYSTEMTIME pst, time_t* pt);
CString SystemTimeToString(LPSYSTEMTIME pst);

int StringToMode(const CString& sAttr);
CString ModeToType(const int p);
int SgringOctalToMode(CString attr);
DWORD StringToAttr(const CString& sAttr);
DWORD ModeToAttr( int mode );

BOOL DeletePanelItems(CString &sPath, struct PluginPanelItem *PanelItem,int ItemsNumber);

BOOL ExecuteCommandLine(const CString & command, const CString& path, const CString & parameters, bool wait);
