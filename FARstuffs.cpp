#include "stdafx.h"

#include "FARstuffs.h"

const farStr* LOC(int MsgId)
{
  return fInfo.GetMsg(&MainGuid, MsgId);
}

int ShowMessage(const farStr* msg, int buttons, const farStr* help, bool Warning /*= false*/)
{
  int res = fInfo.Message(&MainGuid, &MsgGuid, (Warning ? FMSG_WARNING : 0) | FMSG_ALLINONE, help, reinterpret_cast<const farStr * const *>(msg), 0, buttons);
  return res;
}

void ShowMessageWait(const farStr* const* msg, int msgsize)
{
  fInfo.Message(&MainGuid, &MsgWaitGuid, FMSG_LEFTALIGN, nullptr, msg, msgsize, 0);
}

int ShowDialog(int width, int height, const farStr* help, FarDialogItem* items, int count, HANDLE& hDlg)
{
  hDlg = fInfo.DialogInit(&MainGuid, &DialogGuid, -1, -1, width, height, help, items, count, 0, 0, nullptr, nullptr);
  int res = fInfo.DialogRun(hDlg);
  return res;
}

int ShowMenu(const farStr* title, const farStr* bottom, const farStr* help, const FarMenuItem* items, int count)
{
  int res = fInfo.Menu(&MainGuid, &MenuGuid, -1, -1, 0, FMENU_WRAPMODE, title, bottom, help, nullptr, nullptr, items, count);
  return res;
}

CString GetPanelPath(bool another /*= false*/)
{
  CString ret;

  int Size = static_cast<int>(fInfo.PanelControl(another ? PANEL_PASSIVE : PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, 0, nullptr));
  FarPanelDirectory* dirInfo = static_cast<FarPanelDirectory*>(my_malloc(Size));
  dirInfo->StructSize = sizeof(FarPanelDirectory);
  fInfo.PanelControl(another ? PANEL_PASSIVE : PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, Size, dirInfo);
  ret = dirInfo->Name;
  my_free(dirInfo);
  return ret;
}

FarDialogItem* GetFarDialogItem(const HANDLE& hDlg, DWORD item)
{
  if (hDlg)
  {
    struct FarGetDialogItem DialogItem = {sizeof(FarGetDialogItem)};
    DialogItem.Item = static_cast<FarDialogItem *>(my_malloc(DialogItem.Size = fInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, item, nullptr)));
    if (DialogItem.Item)
    {
      fInfo.SendDlgMessage(hDlg, DM_GETDLGITEM, item, &DialogItem);
      return DialogItem.Item;
    }
  }
  return nullptr;
}

BOOL GetItemSelected(const HANDLE& hDlg, DWORD item)
{
  FarDialogItem* DialogItem = GetFarDialogItem(hDlg, item);
  if (DialogItem)
  {
    BOOL s = DialogItem->Selected;
    my_free(DialogItem);
    return s;
  }
  return FALSE;
}

CString GetItemData(const HANDLE& hDlg, DWORD item)
{
  FarDialogItem* DialogItem = GetFarDialogItem(hDlg, item);
  if (DialogItem)
  {
    CString str = DialogItem->Data;
    my_free(DialogItem);
    return str;
  }
  return _F("");
}

bool SetItemData(const HANDLE& hDlg, DWORD item, const CString& data)
{
  if (!hDlg)
    return false;

  try
  {
    FarDialogItem* DialogItem = GetFarDialogItem(hDlg, item);
    if (DialogItem)
    {
      DialogItem->Data = _C(data);
      fInfo.SendDlgMessage(hDlg, DM_SETDLGITEM, item, static_cast<void *>(DialogItem));
      my_free(DialogItem);
      return true;
    }
  }
  catch (...)
  {
  }
  return false;
}

void InitDialogItems(struct InitDialogItem* Init, struct FarDialogItem* Item, int ItemsNumber)
{
  int I;
  struct FarDialogItem* PItem = Item;
  struct InitDialogItem* PInit = Init;
  for (I = 0; I < ItemsNumber; I++ , PItem++ , PInit++)
  {
    memset(static_cast<void *>(PItem), 0, sizeof(*PItem));
    PItem->Type = static_cast<FARDIALOGITEMTYPES>(PInit->Type);
    PItem->X1 = PInit->X1;
    PItem->Y1 = PInit->Y1;
    PItem->X2 = PInit->X2;
    PItem->Y2 = PInit->Y2;
    PItem->Flags = PInit->Flags;
    if (PInit->DefaultButton)
      PItem->Flags |= DIF_DEFAULTBUTTON;
    PItem->MaxLength = 0;
    PItem->History = reinterpret_cast<const farStr *>(PInit->Selected);
    if (static_cast<unsigned int>(reinterpret_cast<DWORD_PTR>(PInit->Data)) < 2000)
      PItem->Data = LOC(static_cast<unsigned int>(reinterpret_cast<DWORD_PTR>(PInit->Data)));
    else
      PItem->Data = PInit->Data;
  }
}

CString GetFileName(bool another, bool selected, int i)
{
  int size = fInfo.PanelControl(another ? PANEL_PASSIVE : PANEL_ACTIVE, selected ? FCTL_GETSELECTEDPANELITEM : FCTL_GETPANELITEM, i, nullptr);
  if (size > 0)
  {
    struct FarGetPluginPanelItem item;
    item.StructSize = sizeof(item);
    item.Size = size;
    item.Item = static_cast<PluginPanelItem *>(my_malloc(size));
    fInfo.PanelControl(another ? PANEL_PASSIVE : PANEL_ACTIVE, selected ? FCTL_GETSELECTEDPANELITEM : FCTL_GETPANELITEM, i, &item);
    CString ret = item.Item->FileName;
    my_free(item.Item);
    return ret;
  }
  return _T("");
}

DWORD GetFileAttributes(bool another, bool selected, int i)
{
  intptr_t size = fInfo.PanelControl(another ? PANEL_PASSIVE : PANEL_ACTIVE, selected ? FCTL_GETSELECTEDPANELITEM : FCTL_GETPANELITEM, i, nullptr);
  if (size > 0)
  {
    struct FarGetPluginPanelItem item;
    item.StructSize = sizeof(item);
    item.Size = size;
    item.Item = static_cast<PluginPanelItem *>(my_malloc(size));
    fInfo.PanelControl(another ? PANEL_PASSIVE : PANEL_ACTIVE, selected ? FCTL_GETSELECTEDPANELITEM : FCTL_GETPANELITEM, i, nullptr);
    DWORD ret = item.Item->FileAttributes;
    my_free(item.Item);
    return ret;
  }
  return 0;
}

CString GetCurrentFileName(bool another)
{
  intptr_t size = fInfo.PanelControl(another ? PANEL_PASSIVE : PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, nullptr);
  if (size > 0)
  {
    struct FarGetPluginPanelItem item;
    item.StructSize = sizeof(item);
    item.Size = size;
    item.Item = static_cast<PluginPanelItem *>(my_malloc(size));
    fInfo.PanelControl(another ? PANEL_PASSIVE : PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, &item);
    CString ret = item.Item->FileName;
    my_free(item.Item);
    return ret;
  }
  return _T("");
}
