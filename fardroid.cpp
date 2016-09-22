#include "StdAfx.h"
#include "fardroid.h"
#include "framebuffer.h"
#include <vector>
#include <ctime>

DWORD WINAPI ProcessThreadProc(LPVOID lpParam)
{
  auto android = static_cast<fardroid *>(lpParam);
  if (!android)
    return 0;

  while (true)
  {
    if (CheckForKey(VK_ESCAPE) && android->BreakProcessDialog()) 
      android->m_bForceBreak = true;

    if (android->m_bForceBreak)
      break;

    android->ShowProgressMessage();
    Sleep(100);
  }

  return 0;
}


fardroid::fardroid(void): lastError(S_OK), handleAdbServer(FALSE), InfoPanelLineArray(nullptr), m_bForceBreak(false)
{
  m_currentPath = _T("/");
  m_currentDevice.Empty();
}

fardroid::~fardroid(void)
{
  delete[] InfoPanelLineArray;

  if (conf.KillServer && handleAdbServer == TRUE)
    ExecuteCommandLine(_T("adb.exe"), conf.ADBPath, _T("kill-server"), false);

  DeleteRecords(records);
}

bool fardroid::CopyFilesDialog(CString& dest, const wchar_t* title)
{
  const auto width = 55;
  struct InitDialogItem InitItems[] = {
    /*00*/FDI_DOUBLEBOX(width - 4, 6, (farStr *)title),
    /*01*/FDI_LABEL(5, 2, (farStr *)MCopyDest),
    /*02*/FDI_EDIT(5, 3, width - 6, _F("fardroidDestinationDir")),
    /*03*/FDI_DEFCBUTTON(5,(farStr *)MOk),
    /*04*/FDI_CBUTTON(5,(farStr *)MCancel),
    /*--*/FDI_SEPARATOR(4,_F("")),
  };
  const int size = sizeof InitItems / sizeof InitItems[0];

  FarDialogItem DialogItemsC[size];
  InitDialogItems(InitItems, DialogItemsC, size);

  wchar_t* editbuf = static_cast<wchar_t *>(my_malloc(1024));
  lstrcpyW(editbuf, _C(dest));
  DialogItemsC[2].Data = editbuf;
  HANDLE hdlg;

  bool res = ShowDialog(width, 8, _F("CopyDialog"), DialogItemsC, size, hdlg) == 3;
  if (res)
    dest = GetItemData(hdlg, 2);

  fInfo.DialogFree(hdlg);
  my_free(editbuf);

  return res;
}

bool fardroid::CreateDirDialog(CString& dest)
{
  const auto width = 55;
  struct InitDialogItem InitItems[] = {
    /*00*/FDI_DOUBLEBOX(width - 4, 6,(farStr *)MCreateDir),
    /*01*/FDI_LABEL(5, 2, (farStr *)MDirName),
    /*02*/FDI_EDIT(5, 3,width - 6, _F("fardroidDirName")),
    /*03*/FDI_DEFCBUTTON(5,(farStr *)MOk),
    /*04*/FDI_CBUTTON(5,(farStr *)MCancel),
    /*--*/FDI_SEPARATOR(4,_F("")),
  };
  const int size = sizeof InitItems / sizeof InitItems[0];

  FarDialogItem DialogItems[size];
  InitDialogItems(InitItems, DialogItems, size);

  wchar_t* editbuf = static_cast<wchar_t *>(my_malloc(1024));
  lstrcpyW(editbuf, _C(dest));
  DialogItems[2].Data = editbuf;
  HANDLE hdlg;

  bool res = ShowDialog(width, 8, _F("CreateDirDialog"), DialogItems, size, hdlg) == 3;
  if (res)
    dest = GetItemData(hdlg, 2);

  fInfo.DialogFree(hdlg);
  my_free(editbuf);

  return res;
}

bool fardroid::DeviceNameDialog()
{
  return DeviceNameDialog(m_currentDevice, m_currentDeviceName);
}

bool fardroid::DeviceNameDialog(const CString &name, CString &alias)
{
  const auto width = 55;
  struct InitDialogItem InitItems[] = {
    /*00*/FDI_DOUBLEBOX(width - 4, 6, (farStr *)static_cast<const wchar_t*>(name)),
    /*01*/FDI_LABEL(5, 2, (farStr *)MRenameDeviceName),
    /*02*/FDI_EDIT(5, 3,width - 6, _F("")),
    /*03*/FDI_DEFCBUTTON(5,(farStr *)MOk),
    /*04*/FDI_CBUTTON(5,(farStr *)MCancel),
    /*--*/FDI_SEPARATOR(4,_F("")),
  };
  const int size = sizeof InitItems / sizeof InitItems[0];

  FarDialogItem DialogItems[size];
  InitDialogItems(InitItems, DialogItems, size);

  CString last;
  conf.GetSub(0, _T("names"), name, last, name);

  wchar_t* editbuf = static_cast<wchar_t *>(my_malloc(1024));
  lstrcpyW(editbuf, _C(last));
  DialogItems[2].Data = editbuf;
  HANDLE hdlg;

  bool res = ShowDialog(width, 8, _F(""), DialogItems, size, hdlg) == 3;
  if (res)
  {
    alias = GetItemData(hdlg, 2);
    conf.SetSub(0, _T("names"), name, alias);
  }

  fInfo.DialogFree(hdlg);
  my_free(editbuf);

  return res;
}

HANDLE fardroid::OpenFromMainMenu()
{
  if (DeviceTest())
  {
    if (conf.RemountSystem)
    {
      CString sRes;
      ADB_mount(_T("/system"), TRUE, sRes, false);
    }

    CString lastPath;
    conf.GetSub(0, _T("devices"), m_currentDevice, lastPath, _T("/"));
    if (ChangeDir(lastPath))
      return static_cast<HANDLE>(this);

    return INVALID_HANDLE_VALUE;
  }
  return INVALID_HANDLE_VALUE;
}

bool fardroid::DeleteFilesDialog()
{
  CString msg;
  msg.Format(L"%s\n%s\n%s\n%s", LOC(MDeleteTitle), LOC(MDeleteWarn), LOC(MYes), LOC(MNo));

  return ShowMessage(msg, 2, nullptr, true) == 0;
}

bool fardroid::BreakProcessDialog()
{
  if (m_procStruct.Hide())
  {
    CString msg;
    msg.Format(L"%s\n%s\n%s\n%s", m_procStruct.title, LOC(MBreakWarn), LOC(MYes), LOC(MNo));

    bool bOk = ShowMessage(msg, 2, nullptr, true) == 0;
    m_procStruct.Restore();

    return bOk;
  }
  return false;
}

int fardroid::FileExistsDialog(LPCTSTR sName)
{
  if (m_procStruct.Hide())
  {
    CString msg;
    CString msgexists;
    msg.Format(L"%s\n%s\n%s\n%s\n%s\n%s\n%s", LOC(MGetFile), LOC(MCopyWarnIfExists), LOC(MYes), LOC(MNo), LOC(MAlwaysYes), LOC(MAlwaysNo), LOC(MCancel));
    msgexists.Format(msg, sName);

    int res = ShowMessage(msgexists, 5, _F("warnifexists"), true);

    m_procStruct.Restore();
    return res;
  }
  return 4;
}

int fardroid::CopyErrorDialog(LPCTSTR sTitle, LPCTSTR sErr)
{
  auto ret = 2;
  if (m_procStruct.Hide())
  {
    CString errmsg;
    errmsg.Format(_T("%s\n%s\n\n%s\n\n%s\n%s\n%s"), sTitle, LOC(MCopyError), sErr, LOC(MYes), LOC(MNo), LOC(MCancel));
    ret = ShowMessage(errmsg, 3, _F("copyerror"), true);
    m_procStruct.Restore();
  }

  switch (ret)
  {
  case 0:
    return RETRY;
  case 1:
    return SKIP;
  default:
    return FALSE;
  }
}

int fardroid::CopyDeleteErrorDialog(LPCTSTR sTitle, LPCTSTR sName)
{
  if (m_procStruct.Hide())
  {
    CString msg;
    CString errmsg;
    msg.Format(_T("%s\n%s\n%s\n%s\n%s"), sTitle, LOC(MCopyDeleteError), LOC(MYes), LOC(MNo), LOC(MCancel));
    errmsg.Format(msg, sName);

    int ret = ShowMessage(errmsg, 3, _F("copyerror"), true);

    m_procStruct.Restore();
    return ret;
  }
  return 2;
}


void fardroid::ShowError(CString& error)
{
  CString msg;
  error.TrimRight();
  msg.Format(L"%s\n%s\n%s", LOC(MError), error, LOC(MOk));
  ShowMessage(msg, 1, nullptr, true);
}

HANDLE fardroid::OpenFromCommandLine(const CString& cmd)
{
  if (!DeviceTest())
    return INVALID_HANDLE_VALUE;

  CString sRes;
  if (conf.RemountSystem)
    ADB_mount(_T("/system"), TRUE, sRes, false);

  strvec tokens;
  Tokenize(cmd, tokens, _T(" -"));

  if (tokens.GetSize() == 0)
    return OpenFromMainMenu();

  TokensToParams(tokens, params);

  CString dir = GetParam(params, _T("filename"));
  bool havefile = !dir.IsEmpty();

  if (ExistsParam(params, _T("remount")))
  {
    CString fs = _T("/system");
    if (havefile) fs = dir;
    CString par = GetParam(params, _T("remount"));
    if (par.IsEmpty()) par = _T("rw");
    par.MakeLower();

    ADB_mount(fs, par == _T("rw"), sRes, false);
    return INVALID_HANDLE_VALUE;
  }

  if (havefile)
  {
    DelEndSlash(dir, true);
    if (ChangeDir(dir))
      return static_cast<HANDLE>(this);
    return OpenFromMainMenu();
  }
  return INVALID_HANDLE_VALUE;
}

int fardroid::GetItems(PluginPanelItem* PanelItem, int ItemsNumber, const CString& srcdir, const CString& dstdir, bool noPromt, bool ansYes, bool bSilent)
{
  CString sdir = srcdir;
  CString ddir = dstdir;
  AddEndSlash(sdir, true);
  AddEndSlash(ddir);

  CString sname;
  CString dname;
  CCopyRecords files;
  for (auto i = 0; i < ItemsNumber; i++)
  {
    if (m_bForceBreak)
      return ABORT;

    sname.Format(_T("%s%s"), sdir, PanelItem[i].FileName);
    dname.Format(_T("%s%s"), ddir, PanelItem[i].FileName);

    if (IsDirectory(PanelItem[i].FileAttributes))
    {
      ADBPullDirGetFiles(sname, dname, files);
    }
    else
    {
      auto rec = new CCopyRecord;
      rec->src = sname;
      rec->dst = dname;
      rec->size = PanelItem[i].FileSize;
      FileTimeToUnixTime(&PanelItem[i].LastWriteTime, &rec->time);
      files.Add(rec);
    }
  }

  UINT64 totalSize = 0;
  UINT64 totalTransmitted = 0;
  int filesSize = files.GetSize();
  for (auto i = 0; i < filesSize; i++)
    totalSize += files[i]->size;

  if (m_procStruct.Lock())
  {
    m_procStruct.nTotalStartTime = GetTickCount();
    m_procStruct.nTotalFileSize = totalSize;
    m_procStruct.nTotalFiles = filesSize;
    m_procStruct.Unlock();
  }

  int result = TRUE;
  for (auto i = 0; i < filesSize; i++)
  {
    if (m_bForceBreak)
      break;

    files[i]->dst.Replace(_T("\""), _T(""));
    if (m_procStruct.Lock())
    {
      totalTransmitted = m_procStruct.nTotalTransmitted;

      m_procStruct.nStartTime = GetTickCount();
      m_procStruct.nPosition = i;
      m_procStruct.from = files[i]->src;
      m_procStruct.to = files[i]->dst;
      m_procStruct.nTransmitted = 0;
      m_procStruct.nFileSize = files[i]->size;
      m_procStruct.Unlock();
    }

    auto exist = FileExists(files[i]->dst);
    if (exist)
    {
      if (!noPromt)
      {
        auto exResult = FileExistsDialog(files[i]->dst);
        if (exResult < 0 || exResult > 3)
        {
          m_bForceBreak = true;
          break;
        }

        ansYes = exResult == 0 || exResult == 2;
        noPromt = exResult == 2 || exResult == 3;
      }

      if (!ansYes)
      {
        if (m_procStruct.Lock())
        {
          m_procStruct.nTotalFileSize -= m_procStruct.nFileSize;
          m_procStruct.Unlock();
        }
        continue;
      }
    }

    CString tname = files[i]->dst + TMP_SUFFIX;
    do {
      if (m_procStruct.Lock())
      {
        m_procStruct.nTransmitted = 0;
        m_procStruct.nTotalTransmitted = totalTransmitted;
        m_procStruct.Unlock();
      }

      result = CopyFileFrom(files[i]->src, tname, bSilent, files[i]->time);
    } while (result == RETRY);


    if (result == FALSE || m_bForceBreak)
      break;

    if (result == SKIP)
    {
      result = TRUE;
      if (m_procStruct.Lock())
      {
        m_procStruct.nTotalTransmitted = totalTransmitted;
        m_procStruct.nTotalFileSize -= m_procStruct.nFileSize;
        m_procStruct.Unlock();
      }
      continue;
    }

    if (exist)
    {
      result = DeleteFileTo(files[i]->dst, false);
      if (result == FALSE)
      {
        DeleteFileTo(tname, true);
        break;
      }
    }

    result = MoveFile(tname, files[i]->dst) ? TRUE : FALSE;
    if (result == FALSE)
      break;
  }

  DeleteRecords(files);

  if (m_bForceBreak)
    return ABORT;

  return result;
}

int fardroid::PutItems(PluginPanelItem* PanelItem, int ItemsNumber, const CString& srcdir, const CString& dstdir, bool noPromt, bool ansYes, bool bSilent)
{
  CString sdir = srcdir;
  CString ddir = dstdir;
  AddEndSlash(sdir);
  AddEndSlash(ddir, true);

  CString sname;
  CString dname;
  CCopyRecords files;
  for (auto i = 0; i < ItemsNumber; i++)
  {
    if (m_bForceBreak)
      return ABORT;

    sname.Format(_T("%s%s"), sdir, PanelItem[i].FileName);
    dname.Format(_T("%s%s"), ddir, PanelItem[i].FileName);

    if (IsDirectory(PanelItem[i].FileAttributes))
    {
      ADBPushDirGetFiles(sname, dname, files);
    }
    else
    {
      auto rec = new CCopyRecord;
      rec->src = sname;
      rec->dst = dname;
      rec->size = PanelItem[i].FileSize;
      FileTimeToUnixTime(&PanelItem[i].LastWriteTime, &rec->time);
      files.Add(rec);
    }
  }

  UINT64 totalSize = 0;
  UINT64 totalTransmitted = 0;
  int filesSize = files.GetSize();
  for (auto i = 0; i < filesSize; i++)
    totalSize += files[i]->size;

  if (m_procStruct.Lock())
  {
    m_procStruct.nTotalStartTime = GetTickCount();
    m_procStruct.nTotalFileSize = totalSize;
    m_procStruct.nTotalFiles = filesSize;
    m_procStruct.Unlock();
  }

  auto result = TRUE;
  for (auto i = 0; i < filesSize; i++)
  {
    if (m_bForceBreak)
      break;

    if (m_procStruct.Lock())
    {
      totalTransmitted = m_procStruct.nTotalTransmitted;

      m_procStruct.nStartTime = GetTickCount();
      m_procStruct.nPosition = i;
      m_procStruct.from = files[i]->src;
      m_procStruct.to = files[i]->dst;
      m_procStruct.nTransmitted = 0;
      m_procStruct.nFileSize = files[i]->size;
      m_procStruct.Unlock();
    }

    auto permissions = GetPermissionsFile(files[i]->dst);
    auto exist = !permissions.IsEmpty();
    if (exist)
    {
      if (!noPromt)
      {
        auto exResult = FileExistsDialog(files[i]->dst);
        if (exResult < 0 || exResult > 3)
        {
          m_bForceBreak = true;
          break;
        }

        ansYes = exResult == 0 || exResult == 2;
        noPromt = exResult == 2 || exResult == 3;
      }

      if (!ansYes)
      {
        if (m_procStruct.Lock())
        {
          m_procStruct.nTotalFileSize -= m_procStruct.nFileSize;
          m_procStruct.Unlock();
        }
        continue;
      }
    }

    CString tname = files[i]->dst + TMP_SUFFIX;

    do {
      if (m_procStruct.Lock())
      {
        m_procStruct.nTransmitted = 0;
        m_procStruct.nTotalTransmitted = totalTransmitted;
        m_procStruct.Unlock();
      }

      result = CopyFileTo(files[i]->src, tname, permissions, bSilent);
    } while (result == RETRY);

    if (result == FALSE || m_bForceBreak)
      break;

    if (result == SKIP)
    {
      result = TRUE;
      if (m_procStruct.Lock())
      {
        m_procStruct.nTotalTransmitted = totalTransmitted;
        m_procStruct.nTotalFileSize -= m_procStruct.nFileSize;
        m_procStruct.Unlock();
      }
      continue;
    }

    if (exist)
    {
      result = DeleteFileFrom(files[i]->dst, false);
      if (result == FALSE)
      {
        DeleteFileFrom(tname, true);
        break;
      }
    }

    result = RenameFile(tname, files[i]->dst, false);
    if (result == FALSE)
      break;
  }

  DeleteRecords(files);
  Reread();

  if (m_bForceBreak)
    return ABORT;

  return result;
}

int fardroid::DeleteFileTo(const CString& name, bool bSilent)
{
  if (name.IsEmpty())
    return FALSE;

deltry:
  if (!DeleteFile(name))
  {
    if (bSilent)
      return FALSE;

    int ret = CopyDeleteErrorDialog(LOC(MDelFile), name);
    switch (ret)
    {
    case 0:
      goto deltry;
    case 1:
      return SKIP;
    default:
      return FALSE;
    }
  }

  return TRUE;
}

int fardroid::CopyFileFrom(const CString& src, const CString& dst, bool bSilent, const time_t& mtime)
{
  CString oldPermissions;
  auto useChmod = false;
  if (conf.WorkMode == WORKMODE_NATIVE && conf.SU && conf.UseExtendedAccess)
  {
    oldPermissions = GetPermissionsFile(src);
    if (!oldPermissions.IsEmpty() && oldPermissions.GetAt(oldPermissions.GetLength() - 3) != _T('r'))
    {
      auto newPermissions = oldPermissions;
      newPermissions.SetAt(newPermissions.GetLength() - 3, _T('r'));
      SetPermissionsFile(src, newPermissions);
      useChmod = true;
    }
  }

  CString sRes;
  auto res = ADB_pull(src, dst, sRes, bSilent, mtime);

  if (useChmod) 
    SetPermissionsFile(src, oldPermissions);

  if (!res)
  {
    if (!sRes.IsEmpty()) sRes += _T("\n\n");
    sRes += conf.WorkMode == WORKMODE_NATIVE ? (conf.SU ? LOC(MNeedFolderExePerm) : LOC(MNeedSuperuserPerm)) : LOC(MNeedNativeSuperuserPerm);
    return CopyErrorDialog(LOC(MGetFile), sRes);
  }

  return TRUE;
}

int fardroid::CopyFileTo(const CString& src, const CString& dst, const CString& oldPermissions, bool bSilent)
{
  CString parent = ExtractPath(dst);
  CString parentPermissions;

  auto useChmod = false;
  if (conf.WorkMode == WORKMODE_NATIVE && conf.SU && conf.UseExtendedAccess)
  {
    parentPermissions = GetPermissionsFile(parent);
    if (!parentPermissions.IsEmpty() && (
      parentPermissions.GetAt(parentPermissions.GetLength() - 2) != _T('w') ||
      parentPermissions.GetAt(parentPermissions.GetLength() - 1) != _T('x')))
    {
      auto newPermissions = parentPermissions;
      newPermissions.SetAt(newPermissions.GetLength() - 2, _T('w'));
      newPermissions.SetAt(newPermissions.GetLength() - 1, _T('x'));
      SetPermissionsFile(parent, newPermissions);
      useChmod = true;
    }
  }

  CString sRes;
  auto res = ADB_push(src, dst, sRes, bSilent);

  if (useChmod)
    SetPermissionsFile(parent, parentPermissions);

  if (!res)
  {
    if (!sRes.IsEmpty()) sRes += _T("\n\n");
    sRes += conf.WorkMode == WORKMODE_NATIVE ? (conf.SU ? LOC(MNeedFolderExePerm) : LOC(MNeedSuperuserPerm)) : LOC(MNeedNativeSuperuserPerm);
    return CopyErrorDialog(LOC(MPutFile), sRes);
  }

  if (!oldPermissions.IsEmpty())
    SetPermissionsFile(dst, oldPermissions);

  return TRUE;
}

int fardroid::DeleteFileFrom(const CString& src, bool bSilent)
{
  CString sRes;

deltry:
  BOOL res = ADB_rm(src, sRes, bSilent);
  if (!res)
  {
    if (bSilent)
      return FALSE;

    sRes.TrimRight();
    int ret = CopyDeleteErrorDialog(LOC(MDelFile), sRes);
    switch (ret)
    {
    case 0:
      sRes.Empty();
      goto deltry;
    case 1:
      return SKIP;
    default:
      return FALSE;
    }
  }

  return TRUE;
}

void fardroid::DeleteRecords(CFileRecords& recs)
{
  for (auto i = 0; i < recs.GetSize(); i++)
  {
    delete recs[i];
  }
  recs.RemoveAll();
}

void fardroid::DeleteRecords(CCopyRecords& recs)
{
  for (auto i = 0; i < recs.GetSize(); i++)
  {
    delete recs[i];
  }
  recs.RemoveAll();
}

int fardroid::GetFindData(struct PluginPanelItem** pPanelItem, size_t* pItemsNumber, OPERATION_MODES OpMode)
{
  *pPanelItem = nullptr;
  *pItemsNumber = 0;

  int items = records.GetSize();

  PluginPanelItem* NewPanelItem = static_cast<PluginPanelItem *>(my_malloc(sizeof(PluginPanelItem) * items));
  *pPanelItem = NewPanelItem;

  if (NewPanelItem == nullptr)
    return FALSE;

  CFileRecord* item;

  for (int i = 0; i < items; i++)
  {
    my_memset(&NewPanelItem[i], 0, sizeof(PluginPanelItem));

    item = records[i];
    NewPanelItem[i].FileAttributes = item->attr;
    NewPanelItem[i].UserData.Data = &i;
    NewPanelItem[i].FileSize = item->size;
    NewPanelItem[i].CreationTime = NewPanelItem[i].ChangeTime = NewPanelItem[i].LastAccessTime = NewPanelItem[i].LastWriteTime =
      UnixTimeToFileTime(item->time);

    NewPanelItem[i].FileName = my_strdupW(item->filename);
    NewPanelItem[i].Owner = my_strdupW(item->owner);
    NewPanelItem[i].Description = my_strdupW(item->desc);

    NewPanelItem[i].CustomColumnNumber = 0;
  }
  *pItemsNumber = items;

  return TRUE;
}

void fardroid::FreeFindData(struct PluginPanelItem* PanelItem, int ItemsNumber)
{
  if (PanelItem)
  {
    for (auto I = 0; I < ItemsNumber; I++)
    {
      if (PanelItem[I].FileName)
        my_free(static_cast<void*>(const_cast<wchar_t*>(PanelItem[I].FileName)));
      if (PanelItem[I].Owner)
        my_free(static_cast<void*>(const_cast<wchar_t*>(PanelItem[I].Owner)));
      if (PanelItem[I].Description)
        my_free(static_cast<void*>(const_cast<wchar_t*>(PanelItem[I].Description)));
    }
    my_free(PanelItem);
    // ReSharper disable once CppAssignedValueIsNeverUsed
    PanelItem = nullptr;
  }
}

int fardroid::GetFiles(PluginPanelItem* PanelItem, int ItemsNumber, CString& DestPath, BOOL Move, OPERATION_MODES OpMode)
{
  CString srcdir = m_currentPath;
  AddEndSlash(srcdir, true);

  bool bSilent = IS_FLAG(OpMode, OPM_SILENT) || IS_FLAG(OpMode, OPM_FIND);
  bool noPromt = bSilent;

  if (IS_FLAG(OpMode, OPM_VIEW) || IS_FLAG(OpMode, OPM_QUICKVIEW) || IS_FLAG(OpMode, OPM_EDIT))
    bSilent = false;

  if (!bSilent && !IS_FLAG(OpMode, OPM_QUICKVIEW) && !IS_FLAG(OpMode, OPM_VIEW) && !IS_FLAG(OpMode, OPM_EDIT))
  {
    if (!CopyFilesDialog(DestPath, Move ? LOC(MMoveFile) : LOC(MGetFile)))
      return ABORT;
  }

  m_bForceBreak = false;
  if (m_procStruct.Lock())
  {
    m_procStruct.title = Move ? LOC(MMoveFile) : LOC(MGetFile);
    m_procStruct.pType = Move ? PS_MOVE : PS_COPY;
    m_procStruct.bSilent = false;
    m_procStruct.nTransmitted = 0;
    m_procStruct.nTotalTransmitted = 0;
    m_procStruct.nFileSize = 0;
    m_procStruct.nTotalFileSize = 0;
    m_procStruct.Unlock();
  }

  DWORD threadID = 0;
  HANDLE hThread = CreateThread(nullptr, 0, ProcessThreadProc, this, 0, &threadID);

  auto result = GetItems(PanelItem, ItemsNumber, srcdir, DestPath, noPromt, noPromt, bSilent);

  m_bForceBreak = true;
  CloseHandle(hThread);
  taskbarIcon.SetState(taskbarIcon.S_NO_PROGRESS);

  return result;
}

int fardroid::UpdateInfoLines()
{
  lines.RemoveAll();
  infoSize.RemoveAll();

  CPanelLine pl;
  pl.text = GetVersionString();
  pl.separator = TRUE;
  lines.Add(pl);

  GetDeviceInfo();

  conf.SU = conf.UseSU;
  auto res = GetMemoryInfo();
  if (conf.SU && !res)
  { 
    conf.SU = FALSE;
    GetMemoryInfo();
  }

  GetPartitionsInfo();

  return lines.GetSize();
}

void fardroid::PreparePanel(OpenPanelInfo* Info)
{
  panelTitle.Format(_T("%s%s"), m_currentDeviceName, m_currentPath);

  Info->PanelTitle = _C(panelTitle);
  Info->CurDir = _C(m_currentPath);

  if (m_currentPath != "/")
    Info->Flags |= OPIF_ADDDOTS;

  if (InfoPanelLineArray)
  {
    delete InfoPanelLineArray;
    InfoPanelLineArray = nullptr;
  }

  auto len = lines.GetSize();
  if (len > 0)
  {
    InfoPanelLineArray = new InfoPanelLine[len];
    for (auto i = 0; i < len; i++)
    {
      InfoPanelLineArray[i].Text = lines[i].text;
      InfoPanelLineArray[i].Data = lines[i].data;
      InfoPanelLineArray[i].Flags = lines[i].separator ? IPLFLAGS_SEPARATOR : 0;
    }
  }

  auto size = 0ULL;
  auto infoLen = infoSize.GetSize();
  if (infoLen > 0)
  {
    for (auto i = 0; i < infoLen; i++)
    {
      if (m_currentPath.Find(infoSize[i].path) == 0)
        size = infoSize[i].free;
    }
  }

  Info->FreeSize = size;
  Info->InfoLines = InfoPanelLineArray;
  Info->InfoLinesNumber = len;
}

void fardroid::ChangePermissionsDialog()
{
  if (conf.WorkMode != WORKMODE_NATIVE)
  {
    CString msg;
    msg.Format(L"%s\n%s\n%s", LOC(MWarningTitle), LOC(MOnlyNative), LOC(MOk));
    ShowMessage(msg, 1, nullptr, true);
    return;
  }

  CString sdir = m_currentPath;
  AddEndSlash(sdir, true);

  CString fileName = ExtractName(GetCurrentFileName(false));
  CString sname;
  if (fileName == "..") {
    fileName = ExtractName(sdir);
    sname = sdir;
  }
  else
  {
    sname.Format(_T("%s%s"), sdir, fileName);
  }

  CString permissions = GetPermissionsFile(sname);
  if (permissions.IsEmpty())
    return;

  const auto width = 45;
  struct InitDialogItem InitItems[] = {
    /*00*/FDI_DOUBLEBOX (width - 4, 11, (farStr *)MPermTitle),
    /*01*/FDI_LABEL ( 5, 2, (farStr *)MPermFileName),
    /*02*/FDI_LABEL ( 5, 3, (farStr *)MPermFileAttr),
    /*03*/FDI_LABEL ( 14, 5, _T("R   W   X")),
    /*04*/FDI_LABEL ( 5, 6, _T("Owner")),
    /*05*/FDI_LABEL ( 5, 7, _T("Group")),
    /*06*/FDI_LABEL ( 5, 8, _T("Others")),
    /*07*/FDI_CHECK ( 13, 6, _T("") ),
    /*08*/FDI_CHECK ( 13, 7, _T("") ),
    /*09*/FDI_CHECK ( 13, 8, _T("") ),
    /*10*/FDI_CHECK ( 17, 6, _T("") ),
    /*11*/FDI_CHECK ( 17, 7, _T("") ),
    /*12*/FDI_CHECK ( 17, 8, _T("") ),
    /*13*/FDI_CHECK ( 21, 6, _T("") ),
    /*14*/FDI_CHECK ( 21, 7, _T("") ),
    /*15*/FDI_CHECK ( 21, 8, _T("") ),
    /*16*/FDI_DEFCBUTTON (10, (farStr *)MOk),
    /*17*/FDI_CBUTTON (10,(farStr *)MCancel),
    /*--*/FDI_SEPARATOR(4,_F("")),
    /*--*/FDI_SEPARATOR(9,_F("")),
  };
  const int size = sizeof InitItems / sizeof InitItems[0];

  FarDialogItem DialogItems[size];
  InitDialogItems(InitItems, DialogItems, size);

  CString LabelTxt1 = LOC(MPermFileName) + fileName;
  DialogItems[1].Data = LabelTxt1;
  CString LabelTxt2 = LOC(MPermFileAttr) + permissions;
  DialogItems[2].Data = LabelTxt2;

  DialogItems[7].Selected = (permissions[1] != _T('-'));
  DialogItems[10].Selected = (permissions[2] != _T('-'));
  DialogItems[13].Selected = (permissions[3] != _T('-'));

  DialogItems[8].Selected = (permissions[4] != _T('-'));
  DialogItems[11].Selected = (permissions[5] != _T('-'));
  DialogItems[14].Selected = (permissions[6] != _T('-'));

  DialogItems[9].Selected = (permissions[7] != _T('-'));
  DialogItems[12].Selected = (permissions[8] != _T('-'));
  DialogItems[15].Selected = (permissions[9] != _T('-'));

  HANDLE hdlg;

  int res = ShowDialog(width, 13, nullptr, DialogItems, size, hdlg);
  if (res == 16)
  {
    permissions.SetAt(1, GetItemSelected(hdlg, 7) ? _T('r') : _T('-'));
    permissions.SetAt(2, GetItemSelected(hdlg, 10) ? _T('w') : _T('-'));
    permissions.SetAt(3, GetItemSelected(hdlg, 13) ? _T('x') : _T('-'));

    permissions.SetAt(4, GetItemSelected(hdlg, 8) ? _T('r') : _T('-'));
    permissions.SetAt(5, GetItemSelected(hdlg, 11) ? _T('w') : _T('-'));
    permissions.SetAt(6, GetItemSelected(hdlg, 14) ? _T('x') : _T('-'));

    permissions.SetAt(7, GetItemSelected(hdlg, 9) ? _T('r') : _T('-'));
    permissions.SetAt(8, GetItemSelected(hdlg, 12) ? _T('w') : _T('-'));
    permissions.SetAt(9, GetItemSelected(hdlg, 15) ? _T('x') : _T('-'));

    SetPermissionsFile(sname, permissions);

    CString permissions_chk = GetPermissionsFile(sname);

    if (permissions_chk != permissions)
    {
      CString msg;
      msg.Format(L"%s\n%s\n%s", LOC(MWarningTitle), LOC(MSetPermFail), LOC(MOk));
      ShowMessage(msg, 1, nullptr, true);
    }
  }
  fInfo.DialogFree(hdlg);
}

CString fardroid::GetDeviceName(const CString& device)
{
  strvec name;
  Tokenize(device, name, _T("\t"));
  return name[0];
}

CString fardroid::GetDeviceAliasName(const CString& device)
{
  CString name;
  conf.GetSub(0, _T("names"), device, name, device);
  return name;
}

CString fardroid::GetDeviceCaption(const CString& device)
{
  CString caption;
  CString name = GetDeviceName(device);
  CString alias = GetDeviceAliasName(name);
  if (alias.Compare(name) == 0)
    caption = alias;
  else
    caption.Format(L"%s (%s)", alias, name);
  return  caption;
}

int fardroid::DeviceMenu(CString& text)
{
  strvec devices;
  std::vector<FarMenuItem> items;

  Tokenize(text, devices, _T("\n"));

  auto size = devices.GetSize();
  if (size == 0)
  {
    return FALSE;
  }
  if (size == 1)
  {
    m_currentDevice = GetDeviceName(devices[0]);
    m_currentDeviceName = GetDeviceAliasName(m_currentDevice);
    return TRUE;
  }

  for (auto i = 0; i < size; i++)
  {
    FarMenuItem item;
    ::ZeroMemory(&item, sizeof(item));
    SetItemText(&item, GetDeviceCaption(devices[i]));
    items.push_back(item);
  }

  int res;
  FarKey pBreakKeys[] = {{ VK_F4,0 }, { VK_DELETE,0 } };
  intptr_t nBreakCode;
  while (true)
  {
    res = ShowMenu(LOC(MSelectDevice), _F("F4,Del"), _F(""), pBreakKeys, &nBreakCode, items.data(), static_cast<int>(items.size()));
    if (nBreakCode == -1)
    {
      if (res < 0)
        return ABORT;
      break;
    }

    CString name = GetDeviceName(devices[res]);
    CString caption = GetDeviceCaption(name);

    switch (pBreakKeys[nBreakCode].VirtualKeyCode)
    {
    case VK_F4:
      DeviceNameDialog(name, caption);
      SetItemText(&items[res], GetDeviceCaption(name));
      break;
    case VK_DELETE:
      conf.SetSub(0, _T("names"), name, name);
      SetItemText(&items[res], GetDeviceCaption(name));
      break;
    }
    SetItemSelected(items, res);
  }

  m_currentDevice = GetDeviceName(devices[res]);
  m_currentDeviceName = GetDeviceAliasName(m_currentDevice);
  return TRUE;
}

void fardroid::SetItemText(FarMenuItem* item, const CString& text)
{
  size_t len = text.GetLength() + 1;
  wchar_t* buf = new wchar_t[len];
  wcscpy(buf, text);
  delete[] item->Text;
  item->Text = buf;
}

void fardroid::SetItemSelected(std::vector<FarMenuItem> &items, int sel)
{
  for (auto i = 0; i < static_cast<int>(items.size()); i++)
  {
    if (i == sel)
      items[i].Flags |= MIF_SELECTED;
    else
      items[i].Flags &= ~MIF_SELECTED;
  }
}

void fardroid::Reread()
{
  CString p = m_currentPath;
  ChangeDir(p, OPM_NONE, true);
}

int fardroid::ChangeDir(LPCTSTR sDir, OPERATION_MODES OpMode, bool updateInfo)
{
  bool bSilent = IS_FLAG(OpMode, OPM_SILENT) || IS_FLAG(OpMode, OPM_FIND);

  CString s = sDir;
  CFileRecord* item = GetFileRecord(sDir);
  if (s != ".." && item && conf.WorkMode != WORKMODE_SAFE  && OpMode == 0 && IsLink(item->attr))
    s = item->linkto;

  CString tempPath;
  if (s == "\\") 
    s = "/";
  if (s[0] == '/')
    tempPath = s;
  else
    tempPath.Format(_T("%s/%s"), m_currentPath, s);
  NormilizePath(tempPath);

  if (OpenPanel(tempPath, updateInfo, bSilent))
    return TRUE;

  if (OpMode != 0 || lastError != S_OK)
    return FALSE;

  return OpenPanel(m_currentPath, updateInfo, bSilent);
}

void fardroid::ADBSyncQuit(SOCKET sockADB)
{
  syncmsg msg;

  msg.req.id = ID_QUIT;
  msg.req.namelen = 0;

  SendADBPacket(sockADB, &msg.req, sizeof(msg.req));
}

bool fardroid::ADBReadMode(SOCKET sockADB, LPCTSTR path, int& mode)
{
  syncmsg msg;
  CString file = WtoUTF8(path, false);
  int len = lstrlen(file);

  msg.req.id = ID_STAT;
  msg.req.namelen = len;

  bool bOk = false;
  if (SendADBPacket(sockADB, &msg.req, sizeof(msg.req)))
  {
    char* buf = getAnsiString(file);
    bOk = SendADBPacket(sockADB, buf, len);
    my_free(buf);
  }

  if ((ReadADBPacket(sockADB, &msg.stat, sizeof(msg.stat)) <= 0) || (msg.stat.id != ID_STAT))
    bOk = false;

  mode = msg.stat.mode;
  return bOk;
}

bool fardroid::ADBTransmitFile(SOCKET sockADB, LPCTSTR sFileName, time_t& mtime)
{
  HANDLE hFile = CreateFile(sFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (hFile == INVALID_HANDLE_VALUE)
    return false;

  FILETIME ft;
  FILETIME lt;
  GetFileTime(hFile, nullptr, nullptr, &ft);
  LocalFileTimeToFileTime(&ft, &lt);
  FileTimeToUnixTime(&lt, &mtime);

  syncsendbuf* sbuf = new syncsendbuf;
  sbuf->id = ID_DATA;

  auto result = true;
  while (true)
  {
    DWORD readed = 0;
    if (!ReadFile(hFile, sbuf->data, SYNC_DATA_MAX, &readed, nullptr))
    {
      result = false;
      break;
    }

    if (readed == 0)
      break;

    sbuf->size = readed;
    if (!SendADBPacket(sockADB, sbuf, sizeof(unsigned) * 2 + readed))
    {
      result = false;
      break;
    }

    if (m_procStruct.Lock())
    {
      m_procStruct.nTransmitted += readed;
      m_procStruct.nTotalTransmitted += readed;
      m_procStruct.Unlock();
    }

    if (m_bForceBreak)
      break;
  }

  delete sbuf;
  CloseHandle(hFile);
  return result;
}

void fardroid::ReadError(SOCKET sockADB, unsigned id, unsigned len, CString& sRes)
{
  if (id != ID_FAIL)
  {
    sRes = _T("unknown reason");
    return;
  }

  auto buf = new char[len];
  auto res = ReadADBPacket(sockADB, buf, len);
  if (res > 0)
  {
    sRes = CString(buf, res);
  }
  delete[] buf;
}

bool fardroid::ADBSendFile(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString& sRes, int mode)
{
  syncmsg msg;
  int len;

  CString sData;
  sData.Format(_T("%s,%d"), sDst, mode);

  len = lstrlen(sDst);
  if (len > 1024) return false;

  msg.req.id = ID_SEND;
  msg.req.namelen = sData.GetLength();

  if (SendADBPacket(sockADB, &msg.req, sizeof(msg.req)))
  {
    char* buf = getAnsiString(sData);
    bool bOK = SendADBPacket(sockADB, buf, sData.GetLength());
    my_free(buf);
    if (!bOK) return false;
  }

  time_t mtime;
  if (!ADBTransmitFile(sockADB, sSrc, mtime))
    return false;

  msg.data.id = ID_DONE;
  msg.data.size = static_cast<unsigned>(mtime);
  if (!SendADBPacket(sockADB, &msg.data, sizeof(msg.data)))
    return false;

  if (ReadADBPacket(sockADB, &msg.status, sizeof(msg.status)) <= 0)
    return false;

  if (msg.status.id != ID_OKAY)
  {
    ReadError(sockADB, msg.status.id, msg.status.msglen, sRes);
    return false;
  }

  return true;
}

bool fardroid::ADBPushDir(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString& sRes)
{
  if (!ADB_mkdir(sDst, sRes, true))
    return false;

  CString ddir = sDst;
  CString sdir = sSrc;
  AddEndSlash(sdir);
  AddEndSlash(ddir, true);
  CString ssaved = sdir;
  CString dsaved = ddir;

  sdir += _T("*.*");

  WIN32_FIND_DATA fd;
  HANDLE h = FindFirstFile(sdir, &fd);
  if (h == INVALID_HANDLE_VALUE) 
    return false;

  CString sname, dname;
  do {
    if (m_bForceBreak)
      break;

    if (lstrcmp(fd.cFileName, _T(".")) == 0 || lstrcmp(fd.cFileName, _T("..")) == 0)
      continue;

    sname.Format(_T("%s%s"), ssaved, fd.cFileName);
    dname.Format(_T("%s%s"), dsaved, fd.cFileName);
    if (IsDirectory(fd.dwFileAttributes))
      ADBPushDir(sockADB, sname, dname, sRes);
    else
      ADBPushFile(sockADB, sname, dname, sRes);
  } while (FindNextFile(h, &fd) != 0);

  FindClose(h);
  return true;
}

void fardroid::ADBPushDirGetFiles(LPCTSTR sSrc, LPCTSTR sDst, CCopyRecords& files)
{
  if (m_procStruct.Lock())
  {
    m_procStruct.from = sSrc;
    m_procStruct.Unlock();
  }

  CString sdir = sSrc;
  CString ddir = sDst;
  AddEndSlash(sdir);
  AddEndSlash(ddir, true);

  WIN32_FIND_DATA fd;
  HANDLE h = FindFirstFile(sdir + _T("*.*"), &fd);
  if (h == INVALID_HANDLE_VALUE)
    return;

  CString sname;
  CString dname;
  do {
    if (m_bForceBreak)
      break;

    if (lstrcmp(fd.cFileName, _T(".")) == 0 || lstrcmp(fd.cFileName, _T("..")) == 0)
      continue;

    sname.Format(_T("%s%s"), sdir, fd.cFileName);
    dname.Format(_T("%s%s"), ddir, fd.cFileName);

    if (IsDirectory(fd.dwFileAttributes))
    {
      ADBPushDirGetFiles(sname, dname, files);
    } 
    else
    {
      auto rec = new CCopyRecord;
      rec->src = sname;
      rec->dst = dname;
      rec->size = fd.nFileSizeHigh * (MAXDWORD + UINT64(1)) + fd.nFileSizeLow;
      FileTimeToUnixTime(&fd.ftLastWriteTime, &rec->time);
      files.Add(rec);
    }

  } while (FindNextFile(h, &fd) != 0);

  FindClose(h);
}

BOOL fardroid::ADBPushFile(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString& sRes)
{
  CString dest = WtoUTF8(sDst, false);
  int mode = 0;
  if (!ADBReadMode(sockADB, dest, mode))
    return FALSE;

  if ((mode != 0) && IS_FLAG(mode, S_IFDIR))
  {
    CString name = ExtractName(sSrc, false);
    AddEndSlash(dest, true);
    dest += name;
  }

  if (!ADBSendFile(sockADB, sSrc, dest, sRes, mode))
    return FALSE;
  if (m_bForceBreak)
    DeleteFileFrom(sDst, true);

  if (conf.WorkMode == WORKMODE_NATIVE && conf.SU && conf.UseExtendedAccess)
    SetPermissionsFile(sDst, "-rw-rw-rw-");
  return TRUE;
}

BOOL fardroid::ADB_push(LPCTSTR sSrc, LPCTSTR sDst, CString& sRes, bool bSilent)
{
  SOCKET sock = PrepareADBSocket();

  if (!SendADBCommand(sock, _T("sync:")))
  {
    CloseADBSocket(sock);
    return FALSE;
  }

  if (IsDirectory(sSrc))
  {
    if (!ADBPushDir(sock, sSrc, sDst, sRes))
    {
      CloseADBSocket(sock);
      return FALSE;
    }
    ADBSyncQuit(sock);
  }
  else
  {
    if (!ADBPushFile(sock, sSrc, sDst, sRes))
    {
      CloseADBSocket(sock);
      return FALSE;
    }
    ADBSyncQuit(sock);
  }

  CloseADBSocket(sock);
  return TRUE;
}

bool fardroid::ADBPullDir(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString& sRes)
{
  MakeDirs(sDst);

  CString ddir = sDst;
  CString sdir = sSrc;
  sRes.Empty();
  AddEndSlash(sdir, true);
  AddEndSlash(ddir);
  CString ssaved = sdir;
  CString dsaved = ddir;

  CFileRecords recs;
  if (ADB_ls(sSrc, recs, sRes, true))
  {
    CString sname, dname;
    for (int i = 0; i < recs.GetSize(); i++)
    {
      sname.Format(_T("%s%s"), ssaved, recs[i]->filename);
      dname.Format(_T("%s%s"), dsaved, recs[i]->filename);
      dname.Replace(_T("\""), _T(""));

      if (!IsDirectory(recs[i]->attr))
      {
        if (m_procStruct.Lock())
        {
          m_procStruct.nStartTime = GetTickCount();
          m_procStruct.from = sname;
          m_procStruct.to = dname;
          m_procStruct.nTransmitted = 0;
          m_procStruct.nFileSize = recs[i]->size;
          m_procStruct.Unlock();
        }

        ADBPullFile(sockADB, sname, dname, sRes, recs[i]->time);
      }
      else
        ADBPullDir(sockADB, sname, dname, sRes);
    }
  }

  DeleteRecords(recs);
  return true;
}

void fardroid::ADBPullDirGetFiles(LPCTSTR sSrc, LPCTSTR sDst, CCopyRecords& files)
{
  if (m_procStruct.Lock())
  {
    m_procStruct.from = sSrc;
    m_procStruct.Unlock();
  }

  CString sdir = sSrc;
  CString ddir = sDst;
  AddEndSlash(sdir, true);
  AddEndSlash(ddir);

  CString sRes;
  CFileRecords recs;
  if (ADB_ls(sSrc, recs, sRes, true))
  {
    CString sname;
    CString dname;
    for (auto i = 0; i < recs.GetSize(); i++)
    {
      if (m_bForceBreak)
        break;

      sname.Format(_T("%s%s"), sdir, recs[i]->filename);
      dname.Format(_T("%s%s"), ddir, recs[i]->filename);

      if (IsDirectory(recs[i]->attr))
      {
        sname.Format(_T("%s%s"), sdir, recs[i]->filename);
        ADBPullDirGetFiles(sname, dname, files);
      }
      else
      {
        auto rec = new CCopyRecord;
        rec->src = sname;
        rec->dst = dname;
        rec->size = recs[i]->size;
        rec->time = recs[i]->time;
        files.Add(rec);
      }
    }
  }
  DeleteRecords(recs);
}

BOOL fardroid::ADBPullFile(SOCKET sockADB, LPCTSTR sSrc, LPCTSTR sDst, CString& sRes, const time_t& mtime)
{
  syncmsg msg;
  int len;
  unsigned id;

  CString file = WtoUTF8(sSrc, false);
  auto ft = UnixTimeToFileTime(mtime);

  len = lstrlen(file);
  if (len > 1024) return FALSE;

  msg.req.id = ID_RECV;
  msg.req.namelen = len;
  if (SendADBPacket(sockADB, &msg.req, sizeof(msg.req)))
  {
    char* buf = getAnsiString(file);
    bool bOK = SendADBPacket(sockADB, buf, len);
    my_free(buf);
    if (!bOK) return FALSE;
  }

  if (ReadADBPacket(sockADB, &msg.data, sizeof(msg.data)) <= 0)
    return FALSE;

  id = msg.data.id;
  if ((id != ID_DATA) && (id != ID_DONE))
  {
    ReadError(sockADB, id, msg.data.size, sRes);
    return FALSE;
  }

  if (FileExists(sDst))
    DeleteFileTo(sDst, false);
  MakeDirs(ExtractPath(sDst, false));

  HANDLE hFile = CreateFile(sDst, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE)
    return FALSE;

  SetFileTime(hFile, &ft, &ft, &ft);

  char* buffer = new char[SYNC_DATA_MAX];
  DWORD written = 0;
  bool bFirst = true;
  while (true)
  {
    if (!bFirst)
    {
      if (ReadADBPacket(sockADB, &msg.data, sizeof(msg.data)) <= 0)
      {
        delete [] buffer;
        CloseHandle(hFile);
        return FALSE;
      }
      id = msg.data.id;
    }

    len = msg.data.size;
    if (id == ID_DONE) break;
    if (id != ID_DATA)
    {
      delete [] buffer;
      CloseHandle(hFile);
      ReadError(sockADB, id, len, sRes);
      return FALSE;
    }

    if (len > SYNC_DATA_MAX)
    {
      delete [] buffer;
      CloseHandle(hFile);
      return FALSE;
    }

    if (ReadADBPacket(sockADB, buffer, len) <= 0)
    {
      delete [] buffer;
      CloseHandle(hFile);
      return FALSE;
    }

    if (!WriteFile(hFile, buffer, len, &written, nullptr))
    {
      delete [] buffer;
      CloseHandle(hFile);
      return FALSE;
    }

    if (m_procStruct.Lock())
    {
      m_procStruct.nTransmitted += written;
      m_procStruct.nTotalTransmitted += written;
      m_procStruct.Unlock();
    }

    if (m_bForceBreak)
    {
      delete [] buffer;
      CloseHandle(hFile);
      DeleteFileTo(sDst, true);
      return TRUE;
    }
    bFirst = false;
  }

  delete [] buffer;
  CloseHandle(hFile);
  return TRUE;
}

BOOL fardroid::ADB_pull(LPCTSTR sSrc, LPCTSTR sDst, CString& sRes, bool bSilent, const time_t& mtime)
{
  int mode;
  CString dest = sDst;

  SOCKET sock = PrepareADBSocket();

  if (!SendADBCommand(sock, _T("sync:")))
  {
    CloseADBSocket(sock);
    return FALSE;
  }

  if (!ADBReadMode(sock, sSrc, mode))
  {
    CloseADBSocket(sock);
    return FALSE;
  }
  if (IS_FLAG(mode, S_IFREG) || IS_FLAG(mode, S_IFCHR) || IS_FLAG(mode, S_IFBLK))
  {
    if (IsDirectory(dest))
    {
      AddEndSlash(dest);
      dest += ExtractName(sSrc);
    }

    dest.Replace(_T("\""), _T(""));
    if (!ADBPullFile(sock, sSrc, dest, sRes, mtime))
    {
      CloseADBSocket(sock);
      return FALSE;
    }
    ADBSyncQuit(sock);
  }
  else if (IS_FLAG(mode, S_IFDIR))
  {
    if (!ADBPullDir(sock, sSrc, dest, sRes))
    {
      CloseADBSocket(sock);
      return FALSE;
    }
    ADBSyncQuit(sock);
  }
  else
  {
    CloseADBSocket(sock);
    return FALSE;
  }

  CloseADBSocket(sock);
  return TRUE;
}

BOOL fardroid::ADB_ls(LPCTSTR sDir, CFileRecords& files, CString& sRes, bool bSilent)
{
  DeleteRecords(files);

  if (conf.WorkMode != WORKMODE_SAFE)
  {
    CString s;
    switch (conf.WorkMode)
    {
    case WORKMODE_NATIVE:
      s.Format(_T("ls -l -a \"%s\""), WtoUTF8(sDir));
      break;
    case WORKMODE_BUSYBOX:
      s.Format(_T("busybox ls -lAe%s --color=never \"%s\""), conf.LinksAsDir() ? _T("") : _T("L"), WtoUTF8(sDir));
      break;
    }

    if (ADBShellExecute(s, sRes, bSilent))
      return ReadFileList(sRes, files, bSilent);
  }
  else
  {
    SOCKET sockADB = PrepareADBSocket();

    if (SendADBCommand(sockADB, _T("sync:")))
    {
      syncmsg msg;
      char buf[257];
      int len;

      CString file = WtoUTF8(sDir, false);
      len = lstrlen(file);
      if (len > 1024) return FALSE;

      msg.req.id = ID_LIST;
      msg.req.namelen = len;

      if (SendADBPacket(sockADB, &msg.req, sizeof(msg.req)))
      {
        char* dir = getAnsiString(file);
        if (SendADBPacket(sockADB, dir, len))
        {
          for (;;)
          {
            if (ReadADBPacket(sockADB, &msg.dent, sizeof(msg.dent)) <= 0) break;
            if (msg.dent.id == ID_DONE)
            {
              my_free(dir);
              return TRUE;
            }
            if (msg.dent.id != ID_DENT) break;
            len = msg.dent.namelen;
            if (len > 256) break;

            if (ReadADBPacket(sockADB, buf, len) <= 0)
            {
              break;
            }

            buf[len] = 0;

            if (lstrcmpA(buf, ".") != 0 && lstrcmpA(buf, "..") != 0)
            {
              CFileRecord* rec = new CFileRecord;
              rec->attr = ModeToAttr(msg.dent.mode);
              rec->size = msg.dent.size;
              rec->time = msg.dent.time;

              CString s = buf;
              rec->filename = UTF8toW(s);

              files.Add(rec);
            }
          }
        }
        my_free(dir);
      }
    }
  }

  return FALSE;
}

BOOL fardroid::ADB_rm(LPCTSTR sDir, CString& sRes, bool bSilent)
{
  CString s;
  s.Format(_T("rm -r \"%s\""), WtoUTF8(sDir));
  ADBShellExecute(s, sRes, bSilent);
  return sRes.GetLength() == 0;
}

BOOL fardroid::ADB_mkdir(LPCTSTR sDir, CString& sRes, bool bSilent)
{
  CString s;
  s.Format(_T("mkdir \"%s\""), WtoUTF8(sDir));
  ADBShellExecute(s, sRes, bSilent);
  return sRes.GetLength() == 0;
}

BOOL fardroid::ADB_rename(LPCTSTR sSource, LPCTSTR sDest, CString& sRes)
{
  CString s;
  s.Format(_T("mv \"%s\" \"%s\""), WtoUTF8(sSource), WtoUTF8(sDest));
  ADBShellExecute(s, sRes, false);
  return sRes.GetLength() == 0;
}

BOOL fardroid::ReadFileList(CString& sFileList, CFileRecords& files, bool bSilent) const
{
  DeleteRecords(files);
  strvec list;

  Tokenize(sFileList, list, _T("\n"), true, false);

  auto size = list.GetSize();
  for (auto i = 0; i < size; i++)
  {
    switch (conf.WorkMode)
    {
    case WORKMODE_BUSYBOX:
      ParseFileLineBB(list[i], files);
      break;
    case WORKMODE_NATIVE:
      ParseFileLine(list[i], files);
      break;
    }
  }

  if (size == 1 && files.GetSize() == 0)
  {
    if (!bSilent)
      ShowError(sFileList);
    return FALSE;
  }

  return TRUE;
}

bool fardroid::ParseFileLine(CString& sLine, CFileRecords& files) const
{
  strvec tokens;
  CString regex;
  CFileRecord* rec = nullptr;
  if (sLine.IsEmpty())
    return true;
  if (sLine.Left(5) == _T("total") || sLine.Left(3) == _T("ls:"))
    return true;
  if (sLine.Right(1) == _T("\r"))
    sLine.Delete(sLine.GetLength()-1, 1);

  switch (sLine[0])
  {
  case 'd': //directory
    regex = _T("/^([\\w-]+)\\s+(?:\\d+\\s+)?(\\w+)\\s+(\\w+)\\s+(?:\\d+\\s+)?([\\w-]+)\\s+([\\w:]+)\\s(.+)$/");
    RegExTokenize(sLine, regex, tokens);
    if (tokens.GetSize() == 6)
    {
      rec = new CFileRecord;
      rec->attr = StringToAttr(tokens[0]);
      rec->owner = tokens[1];
      rec->grp = tokens[2];
      rec->time = StringTimeToUnixTime(tokens[3], tokens[4]);
      rec->size = 0;
      rec->filename = UTF8toW(tokens[5]);
    }
    break;
  case 'l': //symlink
    regex = _T("/^([\\w-]+)\\s+(?:\\d+\\s+)?(\\w+)\\s+(\\w+)\\s+(?:\\d+\\s+)?([\\w-]+)\\s+([\\w:]+)\\s(.+(?=\\s->))\\s->\\s(.+)$/");
    RegExTokenize(sLine, regex, tokens);
    if (tokens.GetSize() == 7)
    {
      rec = new CFileRecord;
      rec->attr = StringToAttr(tokens[0]);
      rec->owner = tokens[1];
      rec->grp = tokens[2];
      rec->time = StringTimeToUnixTime(tokens[3], tokens[4]);
      rec->size = 0;
      rec->filename = ExtractName(UTF8toW(tokens[5]));
      rec->linkto = UTF8toW(tokens[6]);
      rec->desc.Format(_T("-> %s"), UTF8toW(tokens[6]));
    }
    break;
  case 'c': //device
  case 'b':
  case 's': //socket
    regex = _T("/^([\\w-]+)\\s+(?:\\d+\\s+)?(\\w+)\\s+(\\w+)\\s+([\\w,]+\\s+\\w+)\\s+([\\w-]+)\\s+([\\w:]+)\\s(.+)$/");
    RegExTokenize(sLine, regex, tokens);
    if (tokens.GetSize() == 7)
    {
      rec = new CFileRecord;
      rec->attr = StringToAttr(tokens[0]);
      rec->owner = tokens[1];
      rec->grp = tokens[2];
      rec->size = 0;
      rec->desc = tokens[3];
      rec->time = StringTimeToUnixTime(tokens[4], tokens[5]);
      rec->filename = UTF8toW(tokens[6]);
    }
    else
    {
    case '-': //file
    case 'p': //FIFO	
      regex = _T("/^([\\w-]+)\\s+(?:\\d+\\s+)?(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+([\\w-]+)\\s+([\\w:]+)\\s(.+)$/");
      RegExTokenize(sLine, regex, tokens);
      if (tokens.GetSize() == 7)
      {
        rec = new CFileRecord;
        rec->attr = StringToAttr(tokens[0]);
        rec->owner = tokens[1];
        rec->grp = tokens[2];
        rec->size = _ttoi(tokens[3]);
        rec->time = StringTimeToUnixTime(tokens[4], tokens[5]);
        rec->filename = UTF8toW(tokens[6]);
      }
      else
      {
        regex = _T("/^([\\w-]+)\\s+(?:\\d+\\s+)?(\\w+)\\s+(\\w+)\\s+(?:\\d+\\s+)?([\\w-]+)\\s+([\\w:]+)\\s(.+)$/");
        RegExTokenize(sLine, regex, tokens);
        if (tokens.GetSize() == 6)
        {
          rec = new CFileRecord;
          rec->attr = StringToAttr(tokens[0]);
          rec->owner = tokens[1];
          rec->grp = tokens[2];
          rec->size = 0;
          rec->time = StringTimeToUnixTime(tokens[3], tokens[4]);
          rec->filename = UTF8toW(tokens[5]);
        }
      }
    }
    break;
  }

  if (rec && rec->filename != "." && rec->filename != "..")
  {
    files.Add(rec);
    return true;
  }

  return false;
}

bool fardroid::ParseFileLineBB(CString& sLine, CFileRecords& files) const
{
  strvec tokens;
  CString regex;
  CFileRecord* rec = nullptr;
  if (sLine.IsEmpty())
    return true;
  if (sLine.Left(5) == _T("total") || sLine.Left(3) == _T("ls:"))
    return true;
  if (sLine.Right(1) == _T("\r"))
    sLine.Delete(sLine.GetLength() - 1, 1);

  switch (sLine[0])
  {
  case 'l': //symlink
    regex = _T("/^([\\w-]+)\\s+(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+([\\w:]+)\\s(\\w+)\\s(.+(?=\\s->))\\s->\\s(.+)$/");
    RegExTokenize(sLine, regex, tokens);
    if (tokens.GetSize() == 12)
    {
      rec = new CFileRecord;
      rec->attr = StringToAttr(tokens[0]);
      rec->owner = tokens[2];
      rec->grp = tokens[3];
      rec->size = _ttoi(tokens[4]);
      rec->time = StringTimeToUnixTime(tokens[7], tokens[6], tokens[9], tokens[8]);
      rec->filename = ExtractName(UTF8toW(tokens[10]));
      rec->linkto = UTF8toW(tokens[11]);
      rec->desc.Format(_T("-> %s"), UTF8toW(tokens[11]));
    }
    break;
  case 'd': //directory
  case '-': //file
  case 'p': //FIFO
    regex = _T("/^([\\w-]+)\\s+(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+([\\w:]+)\\s(\\w+)\\s(.+)$/");
    RegExTokenize(sLine, regex, tokens);
    if (tokens.GetSize() == 11)
    {
      rec = new CFileRecord;
      rec->attr = StringToAttr(tokens[0]);
      //tokens[1] - links count
      rec->owner = tokens[2];
      rec->grp = tokens[3];
      rec->size = _ttoi(tokens[4]);
      //tokens[5] - day of week
      rec->time = StringTimeToUnixTime(tokens[7], tokens[6], tokens[9], tokens[8]);
      rec->filename = UTF8toW(tokens[10]);
    }
    break;
  case 'c': //device
  case 'b':
  case 's':
    regex = _T("/^([\\w-]+)\\s+(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+([\\w,]+\\s+\\w+)\\s+(\\w+)\\s+(\\w+)\\s+(\\w+)\\s+([\\w:]+)\\s(\\w+)\\s(.+)$/");
    RegExTokenize(sLine, regex, tokens);
    if (tokens.GetSize() == 11)
    {
      rec = new CFileRecord;
      rec->attr = StringToAttr(tokens[0]);
      rec->owner = tokens[2];
      rec->grp = tokens[3];
      rec->size = 0;
      rec->desc = tokens[4];
      rec->time = StringTimeToUnixTime(tokens[7], tokens[6], tokens[9], tokens[8]);
      rec->filename = UTF8toW(tokens[10]);
    }
    break;
  }

  if (rec && rec->filename != "." && rec->filename != "..")
  {
    files.Add(rec);
    return true;
  }

  return false;
}

BOOL fardroid::OpenPanel(LPCTSTR sPath, bool updateInfo, bool bSilent)
{
  BOOL bOK = FALSE;
  CString sDir = sPath;

  SOCKET sockADB = PrepareADBSocket();
  if (sockADB)
  {
    CloseADBSocket(sockADB);

    if (updateInfo || lines.GetSize() == 0)
      UpdateInfoLines();

    CString sRes;
    bOK = ADB_ls(sPath, records, sRes, bSilent);
    if (bOK && conf.WorkMode != WORKMODE_SAFE && records.GetSize() == 1)
    {
      auto file = records[0];
      if (IsLink(file->attr) && file->filename.Compare(ExtractName(sDir)) == 0)
      {
        CString tempPath;
        if (file->linkto[0] == '/')
          tempPath = file->linkto;
        else
          tempPath.Format(_T("%s/%s"), ExtractPath(sDir), file->linkto);
        NormilizePath(tempPath);
        return OpenPanel(tempPath, updateInfo, bSilent);
      }
    }
  }

  if (bOK)
  {
    m_currentPath = sDir;
    conf.SetSub(0, _T("devices"), m_currentDevice, m_currentPath);
  }

  return bOK;
}

void fardroid::ShowADBExecError(CString err, bool bSilent)
{
  if (bSilent)
    return;

  if (m_procStruct.Hide())
  {
    CString msg;
    if (err.IsEmpty())
      err = LOC(MADBExecError);

    err.TrimLeft();
    err.TrimRight();
    msg.Format(_T("%s\n%s\n%s"), LOC(MTitle), err, LOC(MOk));
    ShowMessage(msg, 1, nullptr, true);

    m_procStruct.Restore();
  }
}

void fardroid::DrawProgress(CString& sProgress, int size, double pc)
{
  if (pc > 1)
    pc = 1;
  if (pc < 0)
    pc = 0;

  int fn = static_cast<int>(pc * size);
  int en = size - fn;
  wchar_t buf[512];
  wchar_t *bp = buf;
  for (auto i = 0; i < fn; i++)
    *bp++ = 0x2588; //'█'
  for (auto i = 0; i < en; i++)
    *bp++ = 0x2591; //'░'
  // ReSharper disable once CppAssignedValueIsNeverUsed
  *bp++ = 0x0;
  sProgress.Format(_T("%s %3d%%"), buf, static_cast<int>(pc * 100));
}

void fardroid::DrawProgress(CString& sProgress, int size, LPCTSTR current, LPCTSTR total)
{
  CString left = sProgress;
  CString right, center;
  right.Format(_T("%s / %s"), current, total);
  for (auto i = 0; i < size - (left.GetLength() + right.GetLength()); i++)
  {
    center += " ";
  }
  sProgress.Format(_T("%s%s%s"), left, center, right);
}

void fardroid::ShowProgressMessage()
{
  if (m_procStruct.Lock())
  {
    static DWORD time = 0;

    if (m_procStruct.bSilent || GetTickCount() - time < 500)
    {
      m_procStruct.Unlock();
      return;
    }

    time = GetTickCount();
    if (m_procStruct.pType == PS_COPY || m_procStruct.pType == PS_MOVE)
    {
      CString mFrom = m_procStruct.nTotalFileSize == 0 ? LOC(MScanDirectory) : LOC(MFrom);
      CString mTo = m_procStruct.nTotalFileSize == 0 ? _T("") : LOC(MTo);
      CString mTotal;
      mTotal.Format(_T("\x1%s"), LOC(MTotal));

      static CString sInfo;
      int elapsed = m_procStruct.nTotalFileSize == 0 ? 0 : (time - m_procStruct.nTotalStartTime) / 1000;
      auto speed = 0;
      auto remain = 0;
      if (elapsed > 0)
        speed = static_cast<int>(m_procStruct.nTotalTransmitted / elapsed);
      if (speed > 0)
        remain = static_cast<int>((m_procStruct.nTotalFileSize - m_procStruct.nTotalTransmitted) / speed);
      sInfo.Format(LOC(MProgress), FormatTime(elapsed), FormatTime(remain), FormatSize("%9.2f", "%s %s/s", speed, false));

      int size = sInfo.GetLength() - 5;
      CString sFrom = m_procStruct.from;
      if (m_procStruct.from.GetLength() >= size)
      {
        sFrom.Delete(0, m_procStruct.from.GetLength() - size - 2);
        sFrom = "..." + sFrom;
      }
      CString sTo = m_procStruct.to;
      if (m_procStruct.to.GetLength() >= size)
      {
        sTo.Delete(0, m_procStruct.to.GetLength() - size - 2);
        sTo = "..." + sTo;
      }

      CString sFiles = LOC(MFiles);
      CString sBytes = LOC(MBytes);
      DrawProgress(sFiles, size, FormatNumber(m_procStruct.nPosition), FormatNumber(m_procStruct.nTotalFiles));
      DrawProgress(sBytes, size, FormatNumber(m_procStruct.nTotalTransmitted), FormatNumber(m_procStruct.nTotalFileSize));

      double pc = m_procStruct.nFileSize > 0 ? static_cast<double>(m_procStruct.nTransmitted) / static_cast<double>(m_procStruct.nFileSize) : 0;
      double tpc = m_procStruct.nTotalFileSize > 0 ? static_cast<double>(m_procStruct.nTotalTransmitted) / static_cast<double>(m_procStruct.nTotalFileSize) : 0;

      static CString sProgress;
      DrawProgress(sProgress, size, pc);

      if (m_procStruct.nTotalFiles > 1)
      {
        static CString sTotalProgress;
        DrawProgress(sTotalProgress, size, tpc);
        const farStr* MsgItems[] = { m_procStruct.title, mFrom, sFrom, mTo, sTo, sProgress, mTotal, sFiles, sBytes, sTotalProgress, _T("\x1"), sInfo };
        ShowMessageWait(MsgItems, sizeof(MsgItems) / sizeof(MsgItems[0]));
      }
      else
      {
        const farStr* MsgItems[] = { m_procStruct.title, mFrom, sFrom, mTo, sTo, sProgress, mTotal, sFiles, sBytes, _T("\x1"), sInfo };
        ShowMessageWait(MsgItems, sizeof(MsgItems) / sizeof(MsgItems[0]));
      }
      taskbarIcon.SetState(taskbarIcon.S_PROGRESS, tpc);
    }
    else if (m_procStruct.pType == PS_DELETE)
    {
      int size = 50;

      double pc = static_cast<double>(m_procStruct.nPosition) / static_cast<double>(m_procStruct.nTotalFiles);
      static CString sProgress;
      DrawProgress(sProgress, size, pc);

      const farStr* MsgItems[] = { m_procStruct.title, m_procStruct.from, sProgress };
      ShowMessageWait(MsgItems, sizeof(MsgItems) / sizeof(MsgItems[0]));
      taskbarIcon.SetState(taskbarIcon.S_PROGRESS, pc);
    }
    else if (m_procStruct.pType == PS_FB)
    {
      CString mFrom = LOC(MScreenshotComplete);
      int size = mFrom.GetLength() - 5;

      double pc = m_procStruct.nFileSize > 0 ? static_cast<double>(m_procStruct.nTransmitted) / static_cast<double>(m_procStruct.nFileSize) : 0;
      static CString sProgress;
      DrawProgress(sProgress, size, pc);

      const farStr* MsgItems[] = { m_procStruct.title, sProgress };
      ShowMessageWait(MsgItems, sizeof(MsgItems) / sizeof(MsgItems[0]));
      taskbarIcon.SetState(taskbarIcon.S_PROGRESS, pc);
    }

    m_procStruct.Unlock();
  }
}

int fardroid::DelItems(PluginPanelItem* PanelItem, int ItemsNumber, bool noPromt, bool ansYes, bool bSilent)
{
  CString sdir = m_currentPath;
  AddEndSlash(sdir, true);

  CString sname;
  auto result = TRUE;
  for (auto i = 0; i < ItemsNumber; i++)
  {
    if (m_bForceBreak)
      break;

    sname.Format(_T("%s%s"), sdir, PanelItem[i].FileName);
    if (m_procStruct.Lock())
    {
      m_procStruct.from = sname;
      m_procStruct.nPosition = i;
      m_procStruct.Unlock();
    }

    result = DeleteFileFrom(sname, bSilent);
    if (result == FALSE)
      break;
  }

  Reread();

  if (m_bForceBreak)
    return ABORT;

  return result;
}

int fardroid::DeleteFiles(PluginPanelItem* PanelItem, int ItemsNumber, OPERATION_MODES OpMode)
{
  bool bSilent = IS_FLAG(OpMode, OPM_SILENT) || IS_FLAG(OpMode, OPM_FIND);
  bool noPromt = bSilent;

  if (!bSilent && !IS_FLAG(OpMode, OPM_QUICKVIEW) && !IS_FLAG(OpMode, OPM_VIEW) && !IS_FLAG(OpMode, OPM_EDIT))
  {
    if (!DeleteFilesDialog())
      return ABORT;
  }

  m_bForceBreak = false;
  if (m_procStruct.Lock())
  {
    m_procStruct.title = LOC(MDelFile);
    m_procStruct.pType = PS_DELETE;
    m_procStruct.bSilent = false;
    m_procStruct.nTotalFiles = ItemsNumber;
    m_procStruct.Unlock();
  }

  DWORD threadID = 0;
  HANDLE hThread = CreateThread(nullptr, 0, ProcessThreadProc, this, 0, &threadID);

  auto result = DelItems(PanelItem, ItemsNumber, noPromt, noPromt, bSilent);

  m_bForceBreak = true;
  CloseHandle(hThread);
  taskbarIcon.SetState(taskbarIcon.S_NO_PROGRESS);

  return result;
}

int fardroid::PutFiles(PluginPanelItem* PanelItem, int ItemsNumber, CString SrcPath, BOOL Move, OPERATION_MODES OpMode)
{
  CString srcdir = SrcPath;
  CString path = m_currentPath;

  bool bSilent = IS_FLAG(OpMode, OPM_SILENT) || IS_FLAG(OpMode, OPM_FIND);
  bool noPromt = bSilent;

  if (IS_FLAG(OpMode, OPM_VIEW) || IS_FLAG(OpMode, OPM_QUICKVIEW) || IS_FLAG(OpMode, OPM_EDIT))
    bSilent = false;

  if (IS_FLAG(OpMode, OPM_EDIT))
    noPromt = true;

  m_bForceBreak = false;
  if (m_procStruct.Lock())
  {
    m_procStruct.title = Move ? LOC(MMoveFile) : LOC(MGetFile);
    m_procStruct.pType = Move ? PS_MOVE : PS_COPY;
    m_procStruct.bSilent = false;
    m_procStruct.nTransmitted = 0;
    m_procStruct.nTotalTransmitted = 0;
    m_procStruct.nFileSize = 0;
    m_procStruct.nTotalFileSize = 0;
    m_procStruct.Unlock();
  }

  DWORD threadID = 0;
  HANDLE hThread = CreateThread(nullptr, 0, ProcessThreadProc, this, 0, &threadID);

  auto result = PutItems(PanelItem, ItemsNumber, srcdir, path, noPromt, noPromt, bSilent);

  m_bForceBreak = true;
  CloseHandle(hThread);
  taskbarIcon.SetState(taskbarIcon.S_NO_PROGRESS);

  return result;
}

int fardroid::CreateDir(CString& DestPath, OPERATION_MODES OpMode)
{
  CString path;
  CString srcdir = m_currentPath;
  AddEndSlash(srcdir, true);

  bool bSilent = IS_FLAG(OpMode, OPM_SILENT) || IS_FLAG(OpMode, OPM_FIND);

  if (IS_FLAG(OpMode, OPM_VIEW) || IS_FLAG(OpMode, OPM_QUICKVIEW) || IS_FLAG(OpMode, OPM_EDIT))
    bSilent = false;

  if (!bSilent && !IS_FLAG(OpMode, OPM_QUICKVIEW) && !IS_FLAG(OpMode, OPM_VIEW) && !IS_FLAG(OpMode, OPM_EDIT))
  {
    if (!CreateDirDialog(path))
      return ABORT;

    DestPath = path;
  }

  srcdir += path;

  CString sRes;
  if (ADB_mkdir(srcdir, sRes, bSilent))
  {
    Reread();
  }
  else
  {
    ShowError(sRes);
  }
  return TRUE;
}

int fardroid::Rename(CString& DestPath)
{
  CString srcdir = m_currentPath;
  AddEndSlash(srcdir, true);

  CString src = srcdir + DestPath;
  if (!CopyFilesDialog(DestPath, LOC(MRenameFile)))
    return ABORT;
  CString dst = srcdir + DestPath;

  CString sRes;
  if (ADB_rename(src, dst, sRes))
  {
    Reread();
  }
  else
  {
    ShowError(sRes);
  }
  return TRUE;
}


int fardroid::RenameFile(const CString& src, const CString& dst, bool bSilent)
{
  CString sRes;
  if (ADB_rename(src, dst, sRes))
    return TRUE;

  if (!bSilent && m_procStruct.Hide())
  {
    ShowError(sRes);
    m_procStruct.Restore();
  }

  return FALSE;
}


int fardroid::GetFramebuffer()
{
  m_bForceBreak = false;
  if (m_procStruct.Lock())
  {
    m_procStruct.title = LOC(MScreenshot);
    m_procStruct.pType = PS_FB;
    m_procStruct.bSilent = false;
    m_procStruct.nTransmitted = 0;
    m_procStruct.nTotalFiles = 1;
    m_procStruct.Unlock();
  }

  DWORD threadID = 0;
  HANDLE hThread = CreateThread(nullptr, 0, ProcessThreadProc, this, 0, &threadID);

  fb fb;
  auto result = ADBReadFramebuffer(&fb);
  if (result == TRUE)
  {
    result = SaveToClipboard(&fb);
  }

  m_bForceBreak = true;
  CloseHandle(hThread);
  taskbarIcon.SetState(taskbarIcon.S_NO_PROGRESS);

  if (result == TRUE)
  {
    CString msg;
    msg.Format(L"%s\n%s\n%s", LOC(MScreenshot), LOC(MScreenshotComplete), LOC(MOk));
    ShowMessage(msg, 1, nullptr);
  }

  if (fb.data) my_free(fb.data);
  return result;
}



CFileRecord* fardroid::GetFileRecord(LPCTSTR sFileName)
{
  for (int i = 0; i < records.GetSize(); i++)
  {
    if (records[i]->filename.Compare(sFileName) == 0)
      return records[i];
  }
  return nullptr;
}

unsigned long long fardroid::ParseSizeInfo(CString s)
{
  strvec tokens;
  CString regex = _T("/([\\d.]+)(.*)/");
  RegExTokenize(s, regex, tokens);

  auto res = 0ULL;
  if (tokens.GetSize() > 0)
  {
    char* buf = getAnsiString(tokens[0]);
    auto size = atof(buf);

    if (tokens.GetSize() > 1) {
      if (tokens[1].Find('P') != -1 || tokens[1].Find('p') != -1)
        res = static_cast<unsigned long long>(size * SIZE_PB);
      else if (tokens[1].Find('T') != -1 || tokens[1].Find('t') != -1)
        res = static_cast<unsigned long long>(size * SIZE_TB);
      else if (tokens[1].Find('G') != -1 || tokens[1].Find('g') != -1)
        res = static_cast<unsigned long long>(size * SIZE_GB);
      else if (tokens[1].Find('M') != -1 || tokens[1].Find('m') != -1)
        res = static_cast<unsigned long long>(size * SIZE_MB);
      else if (tokens[1].Find('K') != -1 || tokens[1].Find('k') != -1)
        res = static_cast<unsigned long long>(size * SIZE_KB);
      else
        res = atoll(buf);
    }

    my_free(buf);
  }

  return res;
}

void fardroid::GetDeviceInfo()
{
  CString sRes;
  ADBShellExecute(_T("getprop"), sRes, true, true);

  strvec str;
  Tokenize(sRes, str, _T("\n"));

  strvec tokens;
  CString regex = _T("/^\\[([\\w.]+)\\]:\\s*\\[(.*)\\]$/");
  CString manufacturer = "Unknown", model, version;

  auto size = str.GetSize();
  for (auto i = 0; i < size; i++)
  {
    RegExTokenize(str[i], regex, tokens);
    if (tokens.GetSize() == 2)
    {
      if (tokens[0] == "ro.product.manufacturer")
        manufacturer = tokens[1];
      else if (tokens[0] == "ro.product.model")
        model = tokens[1];
      else if (tokens[0] == "ro.build.version.release")
        version = tokens[1];
    }
  }

  CPanelLine pl;
  pl.separator = FALSE;
  pl.text.Format(L"%s %s", manufacturer, model);
  pl.data.Format(L"%s", version);
  lines.Add(pl);
}


void fardroid::ParseMemoryInfo(CString s)
{
  strvec tokens;

  CPanelLine pl;
  pl.separator = FALSE;

  CString regex = _T("/(\\w+):\\s+(.+)$/");
  RegExTokenize(s, regex, tokens);
  if (tokens.GetSize() > 1)
  {
    pl.text = tokens[0];
    pl.data = FormatSize("%.2f","%s %s", ParseSizeInfo(tokens[1]));
    lines.Add(pl);
  }
}

bool fardroid::GetMemoryInfo()
{
  const static auto showSize = 7;

  CString sRes;
  ADBShellExecute(_T("cat /proc/meminfo"), sRes, true);

  strvec str;
  Tokenize(sRes, str, _T("\n"));

  auto size = str.GetSize();
  if (size < showSize)
    return false;

  CPanelLine pl;
  pl.separator = TRUE;
  pl.text = LOC(MMemoryInfo);
  lines.Add(pl);

  for (auto i = 0; i < showSize; i++)
  {
    ParseMemoryInfo(str[i]);
  }

  return true;
}

void fardroid::ParsePartitionInfo(CString s)
{
  strvec tokens;

  CPanelLine pl;
  CInfoSize fs;
  const CString formatNum = "%7.2f";
  const CString formatText = "%s%s";
  pl.separator = FALSE;

  CString path;
  unsigned long long total = 0, free = 0, used = 0;

  CString regex = _T("/(.*(?=:)):\\W+(\\w+(?=\\stotal)).+,\\s(\\w+(?=\\savailable))/");
  RegExTokenize(s, regex, tokens);
  if (tokens.GetSize() == 3)
  {
    path = tokens[0];
    total = ParseSizeInfo(tokens[1]);
    free = ParseSizeInfo(tokens[2]);
    used = total - free;
  }
  else 
  {
    regex = _T("/^(\\S+)\\s+([\\d.]+\\S*)\\s+([\\d.]+\\S*)\\s+([\\d.]+\\S*)\\s+(?:[\\d.]+%\\s+)?(\\S+)/");
    RegExTokenize(s, regex, tokens);
    if (tokens.GetSize() == 5)
    {
      if (tokens[4][0] == '/')
      {
        path = tokens[4];
        total = ParseSizeInfo(tokens[1]) * 1024ULL;
        used = ParseSizeInfo(tokens[2]) * 1024ULL;
        free = ParseSizeInfo(tokens[3]) * 1024ULL;
      }
      else {
        path = tokens[0];
        total = ParseSizeInfo(tokens[1]);
        used = ParseSizeInfo(tokens[2]);
        free = ParseSizeInfo(tokens[3]);
      }
    }
  }

  if (path.GetLength() > 0) {

    pl.text = path;
    pl.data.Format(_T("%s%s%s"), FormatSize(formatNum, formatText, total), FormatSize(formatNum, formatText, used), FormatSize(formatNum, formatText, free));
    lines.Add(pl);

    fs.path = path;
    fs.total = total;
    fs.used = used;
    fs.free = free;
    infoSize.Add(fs);

    if (fs.path.Find(L"emulated") > 0)
    {
      fs.path = L"/sdcard";
      infoSize.Add(fs);
      fs.path = L"/mnt/sdcard";
      infoSize.Add(fs);
    }
  }
}

void fardroid::GetPartitionsInfo()
{
  CString sRes;

  ADBShellExecute(_T("df"), sRes, true);

  strvec str;
  Tokenize(sRes, str, _T("\n"));

  CPanelLine pl;
  pl.separator = TRUE;
  pl.text = LOC(MPartitionsInfo);
  lines.Add(pl);
  pl.separator = FALSE;
  pl.text.Empty();
  pl.data = "Total     Used     Free";
  lines.Add(pl);

  auto size = str.GetSize();
  for (auto i = 0; i < size; i++)
    ParsePartitionInfo(str[i]);
}

CString fardroid::GetPermissionsFile(const CString& FullFileName)
{
  CString permissions;
  CString s;
  CString sRes;
  s.Format(_T("ls -l -a -d \"%s\""), WtoUTF8(FullFileName));
  if (ADBShellExecute(s, sRes, false))
  {
    strvec perm;
    Tokenize(sRes, perm, _T(" "));

    if (!sRes.IsEmpty() &&
      sRes.Find(_T("No such file or directory")) == -1 &&
      perm[0].GetLength() == 10)
    {
      permissions = perm[0];
    }
  }
  return permissions;
}

CString fardroid::PermissionsFileToMask(CString Permission)
{
  CString permission_mask, tmp_str;

  Permission.Replace(_T('-'),_T('0'));
  Permission.Replace(_T('r'),_T('1'));
  Permission.Replace(_T('w'),_T('1'));
  Permission.Replace(_T('x'),_T('1'));

  permission_mask.Format(_T("%d"),_tcstoul(Permission.Mid(1, 3).GetBuffer(11), nullptr, 2));
  tmp_str.Format(_T("%d"),_tcstoul(Permission.Mid(4, 3).GetBuffer(11), nullptr, 2));
  permission_mask += tmp_str;
  tmp_str.Format(_T("%d"),_tcstoul(Permission.Mid(7, 3).GetBuffer(11), nullptr, 2));
  permission_mask += tmp_str;

  return permission_mask;
}

bool fardroid::SetPermissionsFile(const CString& FullFileName, const CString& PermissionsFile)
{
  CString s;
  CString sRes;
  s.Format(_T("chmod %s \"%s\""), PermissionsFileToMask(PermissionsFile), WtoUTF8(FullFileName));
  return ADBShellExecute(s, sRes, false) != FALSE;
}

bool fardroid::DeviceTest()
{
  SOCKET sock = PrepareADBSocket();
  if (sock)
  {
    CloseADBSocket(sock);
    return true;
  }

  return false;
}

SOCKET fardroid::CreateADBSocket()
{
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2,2), &wsaData);

  SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock)
  {
    sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(5037);
    if (inet_addr("127.0.0.1") != INADDR_NONE)
    {
      dest_addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
      if (connect(sock, reinterpret_cast<sockaddr *>(&dest_addr), sizeof(dest_addr)) == 0)
        return sock;
    }
    closesocket(sock);
  }
  WSACleanup();

  return 0;
}

bool fardroid::SendADBCommand(SOCKET sockADB, LPCTSTR sCMD)
{
  if (sockADB == 0)
    return false;

  CString sCommand;
  sCommand.Format(_T("%04X%s"), lstrlen(sCMD), sCMD);
  char* buf = getAnsiString(sCommand);

  SendADBPacket(sockADB, buf, sCommand.GetLength());
  my_free(buf);

  return CheckADBResponse(sockADB);
}

void fardroid::CloseADBSocket(SOCKET sockADB)
{
  if (sockADB)
    closesocket(sockADB);
  WSACleanup();
}

bool fardroid::CheckADBResponse(SOCKET sockADB)
{
  long msg;
  ReadADBPacket(sockADB, &msg, sizeof(msg));
  return msg == ID_OKAY;
}

bool fardroid::ReadADBSocket(SOCKET sockADB, char* buf, int bufSize)
{
  int nsize;
  int received = 0;
  while (received < bufSize)
  {
    nsize = recv(sockADB, buf, bufSize, 0);
    if (nsize == SOCKET_ERROR)//ошибко
      return false;

    received += nsize;

    if (nsize > 0)
      buf[nsize] = 0;

    if (nsize == 0 || nsize == WSAECONNRESET)
      break;
  }

  return true;
}

SOCKET fardroid::PrepareADBSocket()
{
  lastError = S_OK;
  SOCKET sock = CreateADBSocket();
  if (sock)
  {
    if (m_currentDevice.IsEmpty())
    {
      if (SendADBCommand(sock, _T("host:devices")))
      {
        CString devices = "";
        auto buf = new char[4097];
        ReadADBPacket(sock, buf, 4);
        while (true)
        {
          auto len = ReadADBPacket(sock, buf, 4096);
          if (len <= 0)
            break;

          buf[len] = 0;
          devices += buf;
        }

        CloseADBSocket(sock);
        sock = 0;

        switch (DeviceMenu(devices))
        {
        case TRUE:
          return PrepareADBSocket();
        case ABORT:
          lastError = S_OK;
          break;
        default:
          lastError = ERROR_DEV_NOT_EXIST;
          break;
        }
      }
      else
      {
        CloseADBSocket(sock);
        sock = 0;
        lastError = ERROR_DEV_NOT_EXIST;
      }
    }
    else
    {
      if (!SendADBCommand(sock, _T("host:transport:") + m_currentDevice))
      {
        CloseADBSocket(sock);
        sock = 0;
        lastError = ERROR_DEV_NOT_EXIST;
      }
    }
  }
  else
  {
    if (handleAdbServer == FALSE)
    {
      handleAdbServer = ExecuteCommandLine(_T("adb.exe"), conf.ADBPath, _T("start-server"), true);
      if (handleAdbServer)
        return PrepareADBSocket();
      handleAdbServer = ABORT;
    }
    else
    {
      lastError = ERROR_DEV_NOT_EXIST;
    }
  }

  if (lastError == ERROR_DEV_NOT_EXIST)
    ShowADBExecError(LOC(MDeviceNotFound), false);

  return sock;
}

bool fardroid::SendADBPacket(SOCKET sockADB, void* packet, int size)
{
  char* p = static_cast<char*>(packet);
  int r;

  while (size > 0)
  {
    r = send(sockADB, p, size, 0);

    if (r > 0)
    {
      size -= r;
      p += r;
    }
    else if (r < 0) return false;
    else if (r == 0) return true;
  }
  return true;
}

int fardroid::ReadADBPacket(SOCKET sockADB, void* packet, int size)
{
  char* p = static_cast<char*>(packet);
  int r;
  int received = 0;

  while (size > 0)
  {
    r = recv(sockADB, p, size, 0);
    if (r > 0)
    {
      received += r;
      size -= r;
      p += r;
    }
    else if (r == 0) break;
    else return r;
  }

  return received;
}

BOOL fardroid::ADBShellExecute(LPCTSTR sCMD, CString& sRes, bool bSilent, bool disableSU)
{
  SOCKET sockADB = PrepareADBSocket();

  BOOL bOK = FALSE;
  CString cmd;
  if (!disableSU && conf.SU)
    cmd.Format(_T("shell:su -c \'%s\'"), sCMD);
  else
    cmd.Format(_T("shell:%s"), sCMD);
  if (SendADBCommand(sockADB, cmd))
  {
    char* buf = new char[4097];
    while (true)
    {
      int len = ReadADBPacket(sockADB, buf, 4096);
      if (len <= 0)
        break;

      buf[len] = 0;
      sRes += buf;
    }
    delete[] buf;
    bOK = TRUE;
  }

  CloseADBSocket(sockADB);
  return bOK;
}

int fardroid::ADBReadFramebuffer(struct fb* fb)
{
  auto result = TRUE;

  SOCKET sockADB = PrepareADBSocket();
  if (SendADBCommand(sockADB, _T("framebuffer:")))
  {
    auto buffer = new char[sizeof(struct fbinfo)];
    if (ReadADBPacket(sockADB, buffer, sizeof(struct fbinfo)) <= 0)
    {
      CloseADBSocket(sockADB);
      return FALSE;
    }

    const struct fbinfo* fbinfo = reinterpret_cast<struct fbinfo*>(buffer);
    memcpy(fb, &fbinfo->bpp, sizeof(struct fbinfo) - 4);

    if (m_procStruct.Lock())
    {
      m_procStruct.nTransmitted = 0;
      m_procStruct.nFileSize = fb->size;
      m_procStruct.Unlock();
    }

    fb->data = my_malloc(fb->size);
    int size = fb->size;
    auto position = static_cast<char*>(fb->data);
    int readed;
    while (size > 0)
    {
      readed = ReadADBPacket(sockADB, position, min(size, SYNC_DATA_MAX));
      if (readed == 0) break;

      position += readed;
      size -= readed;

      if (m_procStruct.Lock())
      {
        m_procStruct.nTransmitted += readed;
        m_procStruct.Unlock();
      }

      if (m_bForceBreak)
      {
        result = ABORT;
        break;
      }
    }
  }
  else
  {
    result = FALSE;
  }

  CloseADBSocket(sockADB);
  return result;
}

BOOL fardroid::ADB_findmount(LPCTSTR sFS, strvec& fs_params, CString& sRes, bool bSilent)
{
  sRes.Empty();
  if (ADBShellExecute(_T("mount"), sRes, bSilent))
  {
    strvec tokens;
    Tokenize(sRes, tokens, _T("\n"));
    for (int i = 0; i < tokens.GetSize(); i++)
    {
      fs_params.RemoveAll();
      Tokenize(tokens[i], fs_params, _T(" "));
      if (fs_params.GetSize() == 6 && fs_params[1] == sFS)
        return TRUE;
    }
  }
  return FALSE;
}

BOOL fardroid::ADB_mount(LPCTSTR sFS, BOOL bAsRW, CString& sRes, bool bSilent)
{
  strvec fs_params;
  if (ADB_findmount(sFS, fs_params, sRes, bSilent))
  {
    CString cmd;
    cmd.Format(_T("mount -o remount,%s -t %s %s %s"), bAsRW ? _T("rw") : _T("ro"), fs_params[2], fs_params[0], sFS);
    sRes.Empty();
    return ADBShellExecute(cmd, sRes, bSilent);
  }
  return FALSE;
}
