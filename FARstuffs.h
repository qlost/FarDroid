#pragma once

#define FDI_CONTROL( tp,x,y,x1,y1,f,s,fl,def,txt )			 { tp,            x,y,x1,y1,f,s,fl,def,txt }

#define FDI_DOUBLEBOX( x1,y1,txt )						FDI_CONTROL( DI_DOUBLEBOX,  3,1,x1,y1,0,0,0,0,txt )
#define FDI_SINGLEBOX( x,y, x1,y1,txt )				FDI_CONTROL( DI_SINGLEBOX,  x,y,x1,y1,0,0,0,0,txt )
#define FDI_COMBOBOX( x,y,x1,fl,txt )					FDI_CONTROL( DI_COMBOBOX,		x,y,x1, 0,0,0,fl,0,txt )
#define FDI_LISTBOX( x,y,x1,y1,fl,txt )				FDI_CONTROL( DI_LISTBOX,		x,y,x1,y1,0,0,fl|DIF_LISTWRAPMODE,0,txt )
#define FDI_SEPARATOR( y,txt )                FDI_CONTROL( DI_TEXT,       0,y, 0, 0,0,0,DIF_SEPARATOR|DIF_CENTERGROUP,0,txt )
#define FDI_LABEL( x,y,txt )                  FDI_CONTROL( DI_TEXT,       x,y, 0, 0,0,0,0,0,txt )
#define FDI_CLABEL( x,y,txt )                 FDI_CONTROL( DI_TEXT,       x,y, 0, 0,0,0,DIF_CENTERGROUP,0,txt )
#define FDI_FIXEDIT( x,y,x1,m )								FDI_CONTROL( DI_FIXEDIT,    x,y,x1, y,0,(INT_PTR)m,DIF_MASKEDIT, 0,NULL )
#define FDI_EDIT( x,y,x1,h )                  FDI_CONTROL( DI_EDIT,       x,y,x1, y,0,(INT_PTR)h,h?DIF_HISTORY|DIF_USELASTHISTORY:0,0,NULL )
#define FDI_PASS( x,y,x1 )										FDI_CONTROL( DI_PSWEDIT,    x,y,x1, y,0,0,DIF_MASKEDIT,0,NULL )
#define FDI_ROEDIT( x,y,x1,txt )              FDI_CONTROL( DI_EDIT,       x,y,x1, y,0,0,DIF_READONLY,0,txt )
#define FDI_DISEDEDIT( x,y,x1, txt )          FDI_CONTROL( DI_EDIT,       x,y,x1, y,0,0,DIF_DISABLE,0,txt )
#define FDI_CHECK( x,y,txt )                  FDI_CONTROL( DI_CHECKBOX,   x,y, 0, 0,0,0,0,0,txt )
#define FDI_RADIO( x,y,txt )                  FDI_CONTROL( DI_RADIOBUTTON,x,y, 0, 0,0,0,0,0,txt )
#define FDI_STARTRADIO( x,y,txt )             FDI_CONTROL( DI_RADIOBUTTON,x,y, 0, 0,0,0,DIF_GROUP,0,txt )
#define FDI_BUTTON( x,y,txt )                 FDI_CONTROL( DI_BUTTON,     x,y, 0, 0,0,0, 0,0,txt )
#define FDI_BUTTON2( x,y,txt )                FDI_CONTROL( DI_BUTTON,     x,y, 0, 0,0,0,DIF_NOBRACKETS,0,txt )
#define FDI_DEFBUTTON( x,y,txt )              FDI_CONTROL( DI_BUTTON,     x,y, 0, 0,0,0,0,1,txt )
#define FDI_CBUTTON( y,txt )									FDI_CONTROL( DI_BUTTON,     0,y, 0, 0,0,0,DIF_CENTERGROUP,0,txt )
#define FDI_DEFCBUTTON( y,txt )								FDI_CONTROL( DI_BUTTON,     0,y, 0, 0,0,0,DIF_CENTERGROUP,1,txt )

void InitDialogItems(InitDialogItem *Init, FarDialogItem *Item,	int ItemsNumber);
const farStr *LOC(int MsgId);
int ShowMessage(const farStr* msg, int buttons, const farStr* help, bool Warning = false);
void ShowMessageWait(const farStr *const*msg, int msgsize);
int ShowMenu(const farStr* title, const farStr* bottom, const farStr* help, const FarKey *breakKeys, intptr_t *breakCode, const FarMenuItem *Item, int ItemsNumber);

DWORD GetFileAttributes( bool another, bool selected, int i ) ;
CString GetFileName( bool another, bool selected, int i );
FarDialogItem * GetFarDialogItem(const HANDLE &hDlg, DWORD item);
int GetChecked(const HANDLE& hDlg, int item);
bool SetItemData( const HANDLE & hDlg, DWORD item, const CString &data);
bool SetItemSelected(const HANDLE& hDlg, DWORD item, int selected);
int GetItemSelected(const HANDLE& hDlg, DWORD item);
CString GetItemData(const HANDLE &hDlg, DWORD item );
int ShowDialog(int width, int height, const farStr * help, FarDialogItem * items, int count, HANDLE &hDlg, FARWINDOWPROC dlgProc = nullptr);

CString GetPanelPath( bool another = false );
CString GetCurrentFileName( bool another = false);