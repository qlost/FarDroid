#pragma once

int _httoi(const TCHAR *value);

bool		TestExt(CString ext, CString extList);
CString ExtractName(CString Path, bool bRev = true);
CString ExtractPath(CString Path, bool bRev = true);//unix like
CString ExtractExt(CString Path);
void NormilizePath(CString &path);

void RegExTokenize(CString str, HANDLE hRegex, strvec& tokens);
void Tokenize(CString str,	strvec& tokens,	const CString& sep = " ", bool bLeaveSep = true, bool trim = true );
void TokensToParams(strvec& tokens,	strmap& params);
CString &Unquote(CString &str);
CString GetParam(strmap& params, CString opt);
bool ExistsParam(strmap& params, CString opt);

bool UnicodeToutf8(const wchar_t* wcs, size_t length, byte *outbuf, size_t outlength);
int  utf8ToUnicode ( const char* lpMultiByteStr, WCHAR* lpWideCharStr, int cmbChars );

void DelEndSlash(CString & str, bool bRev = false);
void AddEndSlash(CString & str, bool bRev = false);
void AddBeginSlash( CString & str );
void CleanFloat(CString& str, bool keepSize = true);

char * getAnsiString(LPCTSTR str);
char * getOemString(const CString& val);
int __cdecl my_wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count, UINT CodePage = CP_ACP);
int __cdecl my_mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count, UINT CodePage = CP_ACP);
void strcpyC(char * dst, const CString& src, bool Ansi);

CString WtoUTF8(LPCTSTR str, bool escape = true);
CString UTF8toW(LPCTSTR str, bool unescape = true);

CString FormatNumber(UINT64 str);
CString FormatSize(CString formatNum, CString formatText, UINT64 cb, bool clean=true);
CString FormatTime(int time);
CString CleanWindowsName(const CString& name);
CString EscapeCommand(const CString& cmd, bool quoted = false);
CString FormatFileNameTo(const CString& msg, const CString& file);
