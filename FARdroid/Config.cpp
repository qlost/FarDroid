#include "StdAfx.h"
#include "Config.h"

CConfig::CConfig(void)
{
  hHandle = nullptr;
}

CConfig::~CConfig(void)
{
  Save();
}

bool CConfig::InitHandle()
{
  if (hHandle)
    return true;
  struct FarSettingsCreate sc;
  sc.StructSize = sizeof(sc);
  sc.Guid = MainGuid;
  sc.Handle = INVALID_HANDLE_VALUE;
  fInfo.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, PSL_ROAMING, &sc);
  hHandle = (sc.Handle == INVALID_HANDLE_VALUE ? 0 : sc.Handle);
  return hHandle != nullptr;
}

void CConfig::FreeHandle()
{
  if (hHandle)
    fInfo.SettingsControl(hHandle, SCTL_FREE, 0, nullptr);
  hHandle = nullptr;
}

void CConfig::Set(size_t Root, const wchar_t* Name, int Value) const
{
  FarSettingsItem item = {sizeof(FarSettingsItem), Root, Name, FST_QWORD, {0}};
  item.Number = Value;
  fInfo.SettingsControl(hHandle, SCTL_SET, 0, &item);
}

void CConfig::Set(size_t Root, const wchar_t* Name, CString& Value) const
{
  FarSettingsItem item = {sizeof(FarSettingsItem), Root, Name, FST_STRING, {0}};
  item.String = Value;
  fInfo.SettingsControl(hHandle, SCTL_SET, 0, &item);
}

void CConfig::Get(size_t Root, const wchar_t* Name, int& Value, int Default) const
{
  FarSettingsItem item = {sizeof(FarSettingsItem), Root, Name, FST_QWORD, {0}};
  if (fInfo.SettingsControl(hHandle, SCTL_GET, 0, &item))
    Value = static_cast<int>(item.Number);
  else
    Value = Default;
}

void CConfig::Get(size_t Root, const wchar_t* Name, CString& Value, const wchar_t* Default) const
{
  FarSettingsItem item = {sizeof(FarSettingsItem), Root, Name, FST_STRING, {0}};
  if (fInfo.SettingsControl(hHandle, SCTL_GET, 0, &item))
    Value = item.String;
  else
    Value = Default;
}

int CConfig::CreateSubKey(size_t Root, const wchar_t *Name) const
{
  FarSettingsValue value = { sizeof(FarSettingsValue),Root,Name };
  return fInfo.SettingsControl(hHandle, SCTL_CREATESUBKEY, 0, &value);
}

int CConfig::OpenSubKey(size_t Root, const wchar_t *Name) const
{
  FarSettingsValue value = { sizeof(FarSettingsValue),Root,Name };
  return fInfo.SettingsControl(hHandle, SCTL_OPENSUBKEY, 0, &value);
}

bool CConfig::DeleteSubKey(size_t Root) const
{
  FarSettingsValue value = { sizeof(FarSettingsValue),Root,nullptr };
  return fInfo.SettingsControl(hHandle, SCTL_DELETE, 0, &value) ? true : false;
}

void CConfig::GetSub(size_t Root, const wchar_t* Sub, const wchar_t* Name, CString& Value, const wchar_t* Default)
{
  if (!InitHandle()) {
    Value = Default;
    return;
  }

  int key = OpenSubKey(Root, Sub);
  Get(key, Name, Value, Default);

  FreeHandle();
}

void CConfig::SetSub(size_t Root, const wchar_t* Sub, const wchar_t* Name, CString& Value)
{
  if (!InitHandle())
    return;

  int key = OpenSubKey(Root, Sub);
  if (key <= 0)
    key = CreateSubKey(Root, Sub);
  Set(key, Name, Value);

  FreeHandle();
}

bool CConfig::Load()
{
  if (!InitHandle())
    return false;

  Get(0, _T("PanelMode"), PanelMode, 1);
  Get(0, _T("SortMode"), SortMode, 1);
  Get(0, _T("SortOrder"), SortOrder, 0);
  Get(0, _T("WorkMode"), WorkMode, WORKMODE_SAFE);
  Get(0, _T("ShowLinksAsDir"), ShowLinksAsDir,FALSE);
  Get(0, _T("ShowAllPartitions"), ShowAllPartitions,FALSE);
  Get(0, _T("UseSU"), UseSU,FALSE);
  Get(0, _T("UseExtendedAccess"), UseExtendedAccess,FALSE);
  Get(0, _T("AddToDiskMenu"), AddToDiskMenu,FALSE);
  Get(0, _T("TimeOut"), TimeOut, 1000);
  Get(0, _T("RemountSystem"), RemountSystem,FALSE);
  Get(0, _T("Prefix"), Prefix,_T("fardroid"));
  Get(0, _T("ADBPath"), ADBPath,_T(""));

  FreeHandle();
  return true;
}

void CConfig::Save()
{
  if (!InitHandle())
    return;

  Set(0, _T("PanelMode"), PanelMode);
  Set(0, _T("SortMode"), SortMode);
  Set(0, _T("SortOrder"), SortOrder);
  Set(0, _T("Prefix"), Prefix);
  Set(0, _T("ADBPath"), ADBPath);
  Set(0, _T("WorkMode"), WorkMode);
  Set(0, _T("AddToDiskMenu"), AddToDiskMenu);
  Set(0, _T("ShowLinksAsDir"), ShowLinksAsDir);
  Set(0, _T("ShowAllPartitions"), ShowAllPartitions);
  Set(0, _T("UseSU"), UseSU);
  Set(0, _T("UseExtendedAccess"), UseExtendedAccess);
  Set(0, _T("TimeOut"), TimeOut);
  Set(0, _T("RemountSystem"), RemountSystem);

  FreeHandle();
}

BOOL CConfig::LinksAsDir() const
{
  switch (WorkMode)
  {
  case WORKMODE_BUSYBOX:
    return ShowLinksAsDir;
  case WORKMODE_SAFE:
    return TRUE;
  default:
    return FALSE;
  }
}
