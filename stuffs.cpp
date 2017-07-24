#include "StdAfx.h"
#include "stuffs.h"

void MakeDirs(CString path)
{
  LPTSTR DestPath = path.GetBuffer(0);

  for (LPTSTR ChPtr = DestPath; *ChPtr != 0; ChPtr++)
  {
    if (*ChPtr == _T('\\'))
    {
      *ChPtr = 0;
      CreateDirectory(DestPath, nullptr);
      *ChPtr = _T('\\');
    }
  }
  CreateDirectory(DestPath, nullptr);
  path.ReleaseBuffer();
}

bool FileExists(const CString& path)
{
  return (GetFileAttributes(path) != 0xFFFFFFFF) ? true : false;
}

DWORD GetFileSizeS(const CString& path)
{
  HANDLE h = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h != INVALID_HANDLE_VALUE)
  {
    DWORD ret = GetFileSize(h, nullptr);
    CloseHandle(h);
    return ret;
  }
  return 0;
}

CString GetVersionString()
{
  CString ver;
  ver.Format(_T("FARDroid %u.%u.%u.%u"), MAJORVERSION, MINORVERSION, REVISION, BUILDNUMBER);
  return ver;
}

bool WriteLine(HANDLE stream, const CString& line, int CodePage)
{
#ifdef _UNICODE
  if (stream != INVALID_HANDLE_VALUE)
  {
    DWORD wr = 0;
    switch (CodePage)
    {
    case CodePage_Unicode:
      return (WriteFile(stream, static_cast<LPCTSTR>(line), line.GetAllocLength(), &wr, nullptr) && wr == line.GetAllocLength());
    case CodePage_OEM:
      {
        char* oem = getOemString(line);
        int len = lstrlenA(oem);
        auto res = WriteFile(stream, oem, len, &wr, nullptr);
        my_free(oem);
        return res && wr == len;
      }
    case CodePage_ANSI:
      {
        char* ansi = getAnsiString(line);
        int len = lstrlenA(ansi);
        auto res = WriteFile(stream, ansi, len, &wr, nullptr);
        my_free(ansi);
        return res && wr == len;
      }
    }
  }
#else
	#error TODO!!!
#endif
  return false;
}

bool CheckForKey(const int key)
{
  bool ExitCode = false;
  while (true)
  {
    INPUT_RECORD rec;
    HANDLE hConInp = GetStdHandle(STD_INPUT_HANDLE);
    DWORD ReadCount;
    PeekConsoleInput(hConInp, &rec, 1, &ReadCount);
    if (ReadCount == 0)
      break;
    ReadConsoleInput(hConInp, &rec, 1, &ReadCount);
    if (rec.EventType == KEY_EVENT)
      if (rec.Event.KeyEvent.wVirtualKeyCode == key &&
        rec.Event.KeyEvent.bKeyDown)
        ExitCode = true;
  }
  return ExitCode;
}

int rnd(int maxRnd)
{
  //сделаем более случайный выбор
  int rnds[10];
  for (int i = 0; i < 10; i++)
    rnds[i] = rand() % maxRnd;

  return (rnds[rand() % 10]);
}

bool IsDirectoryMode(int attr)
{
  return IsDirectory(ModeToAttr(attr));
}

bool IsDirectory(DWORD attr)
{
  return (attr & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
}

bool IsDirectory(uintptr_t attr)
{
  return (attr & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
}

bool IsLinkMode(int attr)
{
  return IsLink(ModeToAttr(attr));
}

bool IsLink(DWORD attr)
{
  return (attr & FILE_ATTRIBUTE_REPARSE_POINT) ? true : false;
}

bool IsDevice(DWORD attr)
{
  return (attr & FILE_ATTRIBUTE_DEVICE) ? true : false;
}

bool IsDirectoryLocal(LPCTSTR sPath)
{
  DWORD attr = GetFileAttributes(sPath);
  return ((attr != -1) && (attr & FILE_ATTRIBUTE_DIRECTORY)) ? true : false;
}

void PrepareInfoLine(const wchar_t* str, void* ansi, CString& line, CString format)
{
  line.Format(format, str, ansi);
}

time_t StringTimeToUnixTime(CString sData)
{
  static const CString regexpDate1 = "/(\\d{4}-\\d{2}-\\d{2})\\s+(\\d{2}:\\d{2})/";
  static const CString regexpDate2 = "/(\\w{3})\\s+(\\d+)\\s+(\\d{4})/";
  static const CString regexpDate3 = "/(\\w{3})\\s+(\\d+)\\s+(\\d{2}:\\d{2})/";

  strvec a;
  RegExTokenize(sData, regexpDate1, a);
  if (a.GetSize() == 2)
    return StringTimeToUnixTime(a[0], a[1]);

  RegExTokenize(sData, regexpDate2, a);
  if (a.GetSize() == 3)
    return StringTimeToUnixTime(a[1], a[0], a[2], "");

  RegExTokenize(sData, regexpDate3, a);
  if (a.GetSize() == 3)
    return StringTimeToUnixTime(a[1], a[0], "", a[2]);

  return 0;
}

time_t StringTimeToUnixTime(CString sData, CString sTime)
{
  SYSTEMTIME time = {0};
  strvec a;
  Tokenize(sData, a, _T("-"), false);
  if (a.GetSize() == 3)
  {
    time.wYear = _ttoi(a[0]);
    time.wMonth = _ttoi(a[1]);
    time.wDay = _ttoi(a[2]);
  }

  Tokenize(sTime, a, _T(":"), false);
  if (a.GetSize() == 2)
  {
    time.wHour = _ttoi(a[0]);
    time.wMinute = _ttoi(a[1]);
  }

  time_t t = 0;
  SystemTimeToUnixTime(&time, &t);
  return t;
}

time_t StringTimeToUnixTime(CString sDay, CString sMonth, CString sYear, CString sTime)
{
  SYSTEMTIME time = {0};
  time.wDay = _ttoi(sDay);

  if (sMonth.CompareNoCase(_T("Jan")) == 0)
    time.wMonth = 1;
  else if (sMonth.CompareNoCase(_T("Feb")) == 0)
    time.wMonth = 2;
  else if (sMonth.CompareNoCase(_T("Mar")) == 0)
    time.wMonth = 3;
  else if (sMonth.CompareNoCase(_T("Apr")) == 0)
    time.wMonth = 4;
  else if (sMonth.CompareNoCase(_T("May")) == 0)
    time.wMonth = 5;
  else if (sMonth.CompareNoCase(_T("Jun")) == 0)
    time.wMonth = 6;
  else if (sMonth.CompareNoCase(_T("Jul")) == 0)
    time.wMonth = 7;
  else if (sMonth.CompareNoCase(_T("Aug")) == 0)
    time.wMonth = 8;
  else if (sMonth.CompareNoCase(_T("Sep")) == 0)
    time.wMonth = 9;
  else if (sMonth.CompareNoCase(_T("Oct")) == 0)
    time.wMonth = 10;
  else if (sMonth.CompareNoCase(_T("Nov")) == 0)
    time.wMonth = 11;
  else if (sMonth.CompareNoCase(_T("Dec")) == 0)
    time.wMonth = 12;

  if (sYear.IsEmpty())
  {
    SYSTEMTIME cstime;
    GetSystemTime(&cstime);
    time.wYear = cstime.wYear;
  }
  else
  {
    time.wYear = _ttoi(sYear);
  }

  strvec a;
  Tokenize(sTime, a, _T(":"), false);
  if (a.GetSize() >= 2)
  {
    time.wHour = _ttoi(a[0]);
    time.wMinute = _ttoi(a[1]);
    if (a.GetSize() == 3)
      time.wSecond = _ttoi(a[2]);
  }

  time_t t = 0;
  SystemTimeToUnixTime(&time, &t);
  return t;
}

time_t* SystemTimeToUnixTime(LPSYSTEMTIME pst, time_t* pt)
{
  FILETIME ft;
  FILETIME lft;
  SystemTimeToFileTime(pst, &lft);
  LocalFileTimeToFileTime(&lft, &ft);
  FileTimeToUnixTime(&ft, pt);
  return pt;
}

CString SystemTimeToString(LPSYSTEMTIME pst)
{
  CString str;
  str.Format(_T("%0d.%0d.%04d %0d:%0d:%0d"), pst->wDay, pst->wMonth, pst->wYear, pst->wHour, pst->wMinute, pst->wSecond);
  return str;
}

FILETIME UnixTimeToFileTime(time_t time)
{
  FILETIME ft;
  auto ticks = (static_cast<LONGLONG>(time) + EPOCH_DIFFERENCE) * TICKS_PER_SECOND;
  ft.dwLowDateTime = static_cast<DWORD>(ticks);
  ft.dwHighDateTime = static_cast<DWORD>(ticks >> 32);
  return ft;
}

void FileTimeToUnixTime(LPFILETIME pft, time_t* pt)
{
  auto ticks = (static_cast<LONGLONG>(pft->dwHighDateTime) << 32) + static_cast<LONGLONG>(pft->dwLowDateTime);
  auto sec = ticks / TICKS_PER_SECOND - EPOCH_DIFFERENCE;
  *pt = static_cast<time_t>(sec);
}

int StringToMode(const CString& sAttr)
{
  if (sAttr.GetLength() != 10)
    return -1;

  auto p = 0;
  switch (sAttr[0])
  {
  case 'd': //directory
    p |= S_IFDIR;
    break;
  case '-': //file
    p |= S_IFREG;
    break;
  case 'p': //FIFO
    p |= S_IFIFO;
    break;
  case 'c': //character device
    p |= S_IFCHR;
    break;
  case 'l': //symlink
    p |= S_IFLNK;
    break;
  case 'b': //block
    p |= S_IFBLK;
    break;
  case 's': //socket
    p |= S_IFSOCK;
    break;
  default:
    return -1;
  }

  if (sAttr[1] == 'r') p |= S_IRUSR;
  if (sAttr[2] == 'w') p |= S_IWUSR;
  if (sAttr[3] == 'x' || sAttr[3] == 's') p |= S_IXUSR;
  if (sAttr[3] == 's' || sAttr[3] == 'S') p |= S_ISUID;

  if (sAttr[4] == 'r') p |= S_IRGRP;
  if (sAttr[5] == 'w') p |= S_IWGRP;
  if (sAttr[6] == 'x' || sAttr[6] == 's') p |= S_IXGRP;
  if (sAttr[6] == 's' || sAttr[6] == 'S') p |= S_ISGID;

  if (sAttr[7] == 'r') p |= S_IROTH;
  if (sAttr[8] == 'w') p |= S_IWOTH;
  if (sAttr[9] == 'x' || sAttr[9] == 't') p |= S_IXOTH;
  if (sAttr[9] == 't' || sAttr[9] == 'T') p |= S_ISVTX;

  return p;
}

CString ModeToType(const int p)
{
  if (IS_FLAG(p, S_IFSOCK))
    return "Socket";
  if (IS_FLAG(p, S_IFLNK))
    return "Symbolic Link";
  if (IS_FLAG(p, S_IFREG))
    return "File";
  if (IS_FLAG(p, S_IFBLK))
    return "Block Device";
  if (IS_FLAG(p, S_IFDIR))
    return "Directory";
  if (IS_FLAG(p, S_IFCHR))
    return "Character Device";
  if (IS_FLAG(p, S_IFIFO))
    return "FIFO";
  return "Unknown";
}

int SgringOctalToMode(CString attr)
{
  auto n = _ttoi(attr);
  int res = 0, i = 0;
  while (n != 0)
  {
    int rem = n % 10;
    n /= 10;
    res += static_cast<int>(rem * pow(8, i));
    ++i;
  }
  return res;
}


DWORD StringToAttr(const CString& sAttr)
{
  if (sAttr.GetLength() != 10)
    return FILE_ATTRIBUTE_OFFLINE;
  return
    ((sAttr[0] == _T('d')) ? FILE_ATTRIBUTE_DIRECTORY : 0) | //directory flag
    ((sAttr[0] == _T('l')) ? FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT : 0) | //link
    ((sAttr[0] == _T('-')) ? FILE_ATTRIBUTE_ARCHIVE : 0) | //file
    ((sAttr[0] == _T('c')) ? FILE_ATTRIBUTE_DEVICE : 0) | //symbol device
    ((sAttr[0] == _T('b')) ? FILE_ATTRIBUTE_DEVICE : 0) | //block device
    ((sAttr[0] == _T('p')) ? FILE_ATTRIBUTE_READONLY : 0) | //FIFO device
    ((sAttr[0] == _T('s')) ? FILE_ATTRIBUTE_DEVICE : 0) | //socket device
    ((sAttr[5] != _T('w')) ? FILE_ATTRIBUTE_READONLY : 0);//writable flag
}

DWORD ModeToAttr(int p)
{
  if (p == -1)
    return FILE_ATTRIBUTE_OFFLINE;
  
  auto res = IS_FLAG(p, S_IWRITE) ? 0 : FILE_ATTRIBUTE_READONLY;
  if (IS_FLAG(p, S_IFSOCK))
    res |= FILE_ATTRIBUTE_DEVICE;
  else if (IS_FLAG(p, S_IFLNK))
    res |= FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT;
  else if (IS_FLAG(p, S_IFREG))
    res |= FILE_ATTRIBUTE_ARCHIVE;
  else if (IS_FLAG(p, S_IFBLK))
    res |= FILE_ATTRIBUTE_DEVICE;
  else if (IS_FLAG(p, S_IFDIR))
    res |= FILE_ATTRIBUTE_DIRECTORY;
  else if (IS_FLAG(p, S_IFCHR))
    res |= FILE_ATTRIBUTE_DEVICE;
  else if (IS_FLAG(p, S_IFIFO))
    res |= FILE_ATTRIBUTE_DEVICE;
  return res;
}

BOOL DeleteDir(LPCTSTR sSrc)
{
  CString sDir = sSrc;
  AddEndSlash(sDir);

  WIN32_FIND_DATA fd;
  HANDLE h = FindFirstFile(sDir + _T("*.*"), &fd);
  if (h == INVALID_HANDLE_VALUE)
    return FALSE;

  auto result = TRUE;
  CString sname;
  do
  {
    if (lstrcmp(fd.cFileName, _T(".")) == 0 || lstrcmp(fd.cFileName, _T("..")) == 0)
      continue;

    sname.Format(_T("%s%s"), sDir, fd.cFileName);
    if (IsDirectory(fd.dwFileAttributes))
    {
      if (!DeleteDir(sname))
      {
        result = FALSE;
        break;
      }
    }
    else
    {
      if (!DeleteFile(sname))
      {
        result = FALSE;
        break;
      }
    }
  } while (FindNextFile(h, &fd) != 0);

  FindClose(h);

  if (result)
    result = RemoveDirectory(sSrc);
  return result;
}

BOOL DeletePanelItems(CString& sPath, struct PluginPanelItem* PanelItem, int ItemsNumber)
{
  BOOL result = TRUE;
  AddEndSlash(sPath);
  CString sName;

  for (auto i = 0; i < ItemsNumber; i++)
  {
    sName.Format(_T("%s%s"), sPath, PanelItem[i].FileName);
    if (IsDirectory(PanelItem[i].FileAttributes))
    {
      if (!DeleteDir(sName))
      {
        result = FALSE;
        break;
      }
    }
    else
    {
      if (!DeleteFile(sName))
      {
        result = FALSE;
        break;
      }
    }
  }

  return result;
}

BOOL ExecuteCommandLine(const CString &command, const CString& path, const CString & parameters, bool wait)
{
  SHELLEXECUTEINFO ShExecInfo = { 0 };
  ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
  ShExecInfo.fMask = SEE_MASK_DOENVSUBST | (wait ? SEE_MASK_NOCLOSEPROCESS : SEE_MASK_FLAG_NO_UI);
  ShExecInfo.hwnd = nullptr;
  ShExecInfo.lpVerb = nullptr;
  ShExecInfo.lpFile = command;
  ShExecInfo.lpParameters = parameters;
  ShExecInfo.lpDirectory = path;
  ShExecInfo.nShow = SW_HIDE;
  ShExecInfo.hInstApp = nullptr;

  if (ShellExecuteEx(&ShExecInfo))
  {
    if (!wait)
      return TRUE;

    WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
    CloseHandle(ShExecInfo.hProcess);
    return TRUE;
  }

  return FALSE;
}
