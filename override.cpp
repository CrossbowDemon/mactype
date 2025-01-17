// override.cpp - キレイなTextOut
// 2006/09/27

#include "override.h"
#include "ft.h"
#include "fteng.h"
#include "supinfo.h"
#include "undocAPI.h"
//#include "lrucache.hpp"

#include <malloc.h>		// _alloca
#include <mbctype.h>	// _getmbcp

#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "WindowsCodecs.lib")
#pragma comment (lib, "dwrite.lib")

#if defined(_DEBUG)
void Dbg_TractGetTextExtent(LPCSTR lpString, int cbString, LPSIZE lpSize);
void Dbg_TractGetTextExtent(LPCWSTR lpString, int cbString, LPSIZE lpSize);
void Dbg_TraceExtTextOutW(int nXStart, int nYStart, UINT fuOptions, LPCWSTR lpString, int cbString, const int* lpDx);
void Dbg_TraceScriptItemize(const WCHAR* pwcInChars, int cInChars);
void Dbg_TraceScriptShape(const WCHAR* pwcChars, int cChars, const SCRIPT_ANALYSIS* psa, const WORD* pwOutGlyphs, int cGlyphs);
#else
#define Dbg_TraceExtTextOutW(x,y,f,s,c,p)
#define Dbg_TractGetTextExtent(s,c,p)
#define Dbg_TraceScriptItemize(s,c)
#define Dbg_TraceScriptShape(s,c,p,g,o)
#endif
#define CCH_MAX_STACK	256

typedef HRESULT (WINAPI * __D2D1CreateFactory)(
										D2D1_FACTORY_TYPE factoryType,
										REFIID riid,
										const D2D1_FACTORY_OPTIONS *pFactoryOptions,
										void **ppIFactory);
typedef HRESULT (WINAPI* __DWriteCreateFactory)(
										__in DWRITE_FACTORY_TYPE factoryType,
										__in REFIID iid,
										__out IUnknown **factory
										);


CFontCache FontCache;
CDCArray DCArray;
wstring nullstring;
BOOL g_ccbRender = true;
BOOL g_ccbCache = true;
HFONT g_alterGUIFont = NULL;
HFONT g_alterSysFont = NULL;

//BOOL FreeTypeGetTextExtentPoint(HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize, const FREETYPE_PARAMS* params);

HFONT GetCurrentFont(HDC hdc)
{
	HFONT hCurFont = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
	if (!hCurFont) {
		// NULLの場合はSystemを設定しておく
		hCurFont = (HFONT)GetStockObject(SYSTEM_FONT);
	}
	return hCurFont;
}

//ﾅﾐｶﾏﾊﾇｷ�ﾊﾇﾓﾐﾐｧﾆﾕﾍｨﾏﾔﾊｾDC｣ｬﾓﾃﾓﾚﾌ�ｹ�ﾗﾖﾄｻ
BOOL IsValidDC(HDC hdc)	
{
	if (GetDeviceCaps(hdc, TECHNOLOGY) != DT_RASDISPLAY) {
		return FALSE;
	}
	return TRUE;

}

BOOL IsProcessExcluded()
{
//	if (GetModuleHandle(NULL) == GetModuleHandle(_T("gdi++.exe"))) {
//		return TRUE;
//	}

	const CGdippSettings* pSettings = CGdippSettings::GetInstanceNoInit();
	if (pSettings->IsInclude()) {
		if (pSettings->IsProcessIncluded()) {
			return FALSE;
		}
		return TRUE;
	}

	if (pSettings->IsProcessExcluded()) {
		return TRUE;
	}
	return FALSE;
}

BOOL IsProcessUnload()
{
//	if (GetModuleHandle(NULL) == GetModuleHandle(_T("gdi++.exe"))) {
//		return TRUE;
//	}

	const CGdippSettings* pSettings = CGdippSettings::GetInstanceNoInit();
	if (pSettings->IsInclude()) {
		if (pSettings->IsProcessIncluded()) {
			return FALSE;
		}
		return TRUE;
	}

	if (pSettings->IsProcessUnload()) {
		return TRUE;
	}
	return FALSE;
}

BOOL IsExeUnload(LPCTSTR lpApp)
{
	const CGdippSettings* pSettings = CGdippSettings::GetInstanceNoInit();
	if (pSettings->IsInclude()) {
		if (pSettings->IsExeInclude(lpApp)) {
			return FALSE;
		}
		return TRUE;
	}

	if (pSettings->IsExeUnload(lpApp)) {
		return TRUE;
	}
	return FALSE;
}

//切り上げ除算
int div_ceil(int a, int b)
{
	if(a % b)
		return (a>0)? (a/b+1): (a/b-1);
	return a / b;
}

template <typename _TCHAR>	//ﾐﾞｸﾄﾕ篋�ｺｯﾊ�｣ｬﾊｹﾋ�ﾔﾚﾊｧｰﾜｵﾄﾊｱｺ�ｷｵｻﾘfalse｣ｬﾓﾉｵ�ﾓﾃﾕﾟｸｺﾔ�ｵ�ﾓﾃWindowsﾔｭｺｯﾊ�｣ｬﾊｵﾏﾖﾏﾟｳﾌｰｲﾈｫ｡｣
BOOL _GetTextExtentPoint32AorW(HDC hdc, _TCHAR* lpString, int cbString, LPSIZE lpSize)
{
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	if (pSettings->WidthMode() == SETTING_WIDTHMODE_GDI32
		|| !IsValidDC(hdc) || !lpString || cbString <= 0) {
		return false;
	}

	FREETYPE_PARAMS params(0, hdc);

	if(FreeTypeGetTextExtentPoint(hdc, lpString, cbString, lpSize, &params)) {
		Dbg_TractGetTextExtent(lpString, cbString, lpSize);
		return TRUE;
	}
	else
		return false;
}

//firefox
/*
BOOL WINAPI IMPL_GetTextExtentPoint32A(HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize)
{
	//CThreadCounter __counter;
	//CCriticalSectionLock __lock;
	return _GetTextExtentPoint32AorW(hdc, lpString, cbString, lpSize) || ORIG_GetTextExtentPoint32A(hdc, lpString, cbString, lpSize);
}

BOOL WINAPI IMPL_GetTextExtentPoint32W(HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize)
{
	//CThreadCounter __counter;
	//CCriticalSectionLock __lock;
	return _GetTextExtentPoint32AorW(hdc, lpString, cbString, lpSize) || ORIG_GetTextExtentPoint32W(hdc, lpString, cbString, lpSize);
}

BOOL WINAPI IMPL_GetTextExtentPointA(HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize)
{
	//CThreadCounter __counter;
	//CCriticalSectionLock __lock;
	return _GetTextExtentPoint32AorW(hdc, lpString, cbString, lpSize) || ORIG_GetTextExtentPoint32A(hdc, lpString, cbString, lpSize);
}

BOOL WINAPI IMPL_GetTextExtentPointW(HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize)
{
	//CThreadCounter __counter;
	//CCriticalSectionLock __lock;
	return _GetTextExtentPoint32AorW(hdc, lpString, cbString, lpSize) || ORIG_GetTextExtentPoint32W(hdc, lpString, cbString, lpSize);
}

BOOL WINAPI IMPL_GetCharWidthW(HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer)
{
	//CThreadCounter __counter;
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	if (pSettings->WidthMode() == SETTING_WIDTHMODE_GDI32
		|| !IsValidDC(hdc) || !lpBuffer || !FreeTypeGetCharWidth(hdc, iFirstChar, iLastChar, lpBuffer)) {
		return ORIG_GetCharWidthW(hdc, iFirstChar, iLastChar, lpBuffer);
	}
	return TRUE;
}

BOOL WINAPI IMPL_GetCharWidth32W(HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer)
{
	//CThreadCounter __counter;
	TRACE(_T("GetCharWidth32W(%u, %u, {...})\n"), iFirstChar, iLastChar);
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	if (pSettings->WidthMode() == SETTING_WIDTHMODE_GDI32
		|| !IsValidDC(hdc) || !lpBuffer || !FreeTypeGetCharWidth(hdc, iFirstChar, iLastChar, lpBuffer)) {
		return ORIG_GetCharWidth32W(hdc, iFirstChar, iLastChar, lpBuffer);
	}
	return TRUE;
}*/
/*
extern PFNLdrGetProcedureAddress LdrGetProcedureAddress;
int MyGetProcAddress(HMODULE dll, LPSTR funcname)
{
	ANSI_STRING dumy;
	dumy.Length = strlen(funcname);
	dumy.MaximumLength = strlen(funcname);
	dumy.Buffer = funcname;
	int nRet;
	LdrGetProcedureAddress(dll,&dumy,0,(void**)&nRet);
	return nRet;
}*/


/*
LONG WINAPI IMPL_LdrLoadDll(IN PWCHAR PathToFile OPTIONAL, IN ULONG Flags OPTIONAL, IN UNICODE_STRING2* ModuleFileName, OUT HANDLE* ModuleHandle)
{
	static bool bFisrtTimeHook = GetModuleHandle(_T("d2d1.dll"))!=0;
	if (!bD2D1Loaded)
	{
		wstring filename = wstring(ModuleFileName->Buffer);
		int last_slash=filename.find_last_of('\\');
		if (last_slash!=-1)
			filename.erase(0,last_slash+1);	//ﾉｾｳ�ﾂｷｾｶ
		if (_wcsicmp(filename.c_str(), L"d2d1.dll")==0)	//ﾕ�ﾔﾚﾔﾘﾈ�d2d1.dll
		{
			bD2D1Loaded = true;
			LONG result = ORIG_LdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);
			HookD2D1((HMODULE)*ModuleHandle);
			return result;
		}
		if (bFisrtTimeHook)
		{
			bFisrtTimeHook = false;
			bD2D1Loaded = true;
			HookD2D1(GetModuleHandle(_T("d2d1.dll")));
		}
	}
	return ORIG_LdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);
}*/


/*
BOOL WINAPI IMPL_CreateProcessInternalW( HANDLE hToken, LPCTSTR lpApplicationName, LPTSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, \
										DWORD dwCreationFlags, LPVOID lpEnvironment, LPCTSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation , PHANDLE hNewToken)
{
	//CThreadCounter __counter;
	CCriticalSectionLock __lock;
	return _CreateProcessInternalW(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation, hNewToken, ORIG_CreateProcessInternalW);
}*/

/*
BOOL WINAPI IMPL_nCreateProcessA(LPCSTR lpApp, LPSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, LPCSTR lpDir, LPSTARTUPINFOA psi, LPPROCESS_INFORMATION ppi)
{
	//CThreadCounter __counter;
	CCriticalSectionLock __lock;
	TRACE(_T("CreateProcessA(\"%hs\", \"%hs\", ...)\n"), lpApp, lpCmd);
	return _CreateProcessAorW(lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi, ORIG_nCreateProcessA);
}

BOOL WINAPI IMPL_nCreateProcessW(LPCWSTR lpApp, LPWSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, LPCWSTR lpDir, LPSTARTUPINFOW psi, LPPROCESS_INFORMATION ppi)
{
	//CThreadCounter __counter;
	CCriticalSectionLock __lock;
	TRACE(_T("CreateProcessW(\"%ls\", \"%ls\", ...)\n"), lpApp, lpCmd);
	return _CreateProcessAorW(lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi, ORIG_nCreateProcessW);
}


BOOL WINAPI IMPL_CreateProcessAsUserA(HANDLE hToken, LPCSTR lpApp, LPSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, LPCSTR lpDir, LPSTARTUPINFOA psi, LPPROCESS_INFORMATION ppi)
{
	//CThreadCounter __counter;
	CCriticalSectionLock __lock;
	TRACE(_T("CreateProcessA(\"%hs\", \"%hs\", ...)\n"), lpApp, lpCmd);
	return _CreateProcessAsUserAorW(hToken, lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi, ORIG_CreateProcessAsUserA);
}

BOOL WINAPI IMPL_CreateProcessAsUserW(HANDLE hToken, LPCWSTR lpApp, LPWSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, LPCWSTR lpDir, LPSTARTUPINFOW psi, LPPROCESS_INFORMATION ppi)
{
	//CThreadCounter __counter;
	CCriticalSectionLock __lock;
	TRACE(_T("CreateProcessW(\"%ls\", \"%ls\", ...)\n"), lpApp, lpCmd);
	return _CreateProcessAsUserAorW(hToken, lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi, ORIG_CreateProcessAsUserW);
}*/

/*
HOOK_DEFINE(user32.dll, DWORD, GetTabbedTextExtentA, (HDC hdc, LPCSTR lpString, int nCount, int nTabPositions, CONST LPINT lpnTabStopPositions), (hdc, lpString, nCount, nTabPositions, lpnTabStopPositions))
HOOK_DEFINE(user32.dll, DWORD, GetTabbedTextExtentW, (HDC hdc, LPCWSTR lpString, int nCount, int nTabPositions, CONST LPINT lpnTabStopPositions), (hdc, lpString, nCount, nTabPositions, lpnTabStopPositions))
HOOK_DEFINE(gdi32.dll, BOOL, GetTextExtentExPointA, (HDC hdc, LPCSTR lpszStr, int cchString, int nMaxExtent, LPINT lpnFit, LPINT lpDx, LPSIZE lpSize), (hdc, lpszStr, cchString, nMaxExtent, lpnFit, lpDx, lpSize))
HOOK_DEFINE(gdi32.dll, BOOL, GetTextExtentExPointW, (HDC hdc, LPCWSTR lpszStr, int cchString, int nMaxExtent, LPINT lpnFit, LPINT lpDx, LPSIZE lpSize), (hdc, lpszStr, cchString, nMaxExtent, lpnFit, lpDx, lpSize))
HOOK_DEFINE(gdi32.dll, BOOL, GetTextExtentExPointI, (HDC hdc, LPWORD pgiIn, int cgi, int nMaxExtent, LPINT lpnFit, LPINT lpDx, LPSIZE lpSize), (hdc, pgiIn, cgi, nMaxExtent, lpnFit, lpDx, lpSize))
HOOK_DEFINE(gdi32.dll, BOOL, GetTextExtentPointA, (HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize), (hdc, lpString, cbString, lpSize))
HOOK_DEFINE(gdi32.dll, BOOL, GetTextExtentPointW, (HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize), (hdc, lpString, cbString, lpSize))
HOOK_DEFINE(gdi32.dll, BOOL, GetTextExtentPointI, (HDC hdc, LPWORD pgiIn, int cgi, LPSIZE lpSize), (hdc, pgiIn, cgi, lpSize))
*/


/*
HFONT WINAPI IMPL_CreateFontA(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCSTR  lpszFace)
{
	LOGFONTA lf = {
		nHeight,
		nWidth,
		nEscapement,
		nOrientation,
		fnWeight,
		!!fdwItalic,
		!!fdwUnderline,
		!!fdwStrikeOut,
		(BYTE)fdwCharSet,
		(BYTE)fdwOutputPrecision,
		(BYTE)fdwClipPrecision,
		(BYTE)fdwQuality,
		(BYTE)fdwPitchAndFamily,
	};
	if (lpszFace)
		strncpy(lf.lfFaceName, lpszFace, LF_FACESIZE - 1);
	return IMPL_CreateFontIndirectA(&lf);
}

HFONT WINAPI IMPL_CreateFontW(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCWSTR lpszFace)
{
	LOGFONTW lf = {
		nHeight,
		nWidth,
		nEscapement,
		nOrientation,
		fnWeight,
		!!fdwItalic,
		!!fdwUnderline,
		!!fdwStrikeOut,
		(BYTE)fdwCharSet,
		(BYTE)fdwOutputPrecision,
		(BYTE)fdwClipPrecision,
		(BYTE)fdwQuality,
		(BYTE)fdwPitchAndFamily,
	};
	if (lpszFace)
		wcsncpy(lf.lfFaceName, lpszFace, LF_FACESIZE - 1);
	return IMPL_CreateFontIndirectW(&lf);
}

HFONT WINAPI IMPL_CreateFontIndirectA(CONST LOGFONTA *lplfA)
{
	if(!lplfA) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	if (pSettings->IsFontExcluded(lplfA->lfFaceName)) {
		return ORIG_CreateFontIndirectA(lplfA);
	}

	LOGFONT lf = {
		lplfA->lfHeight,
		lplfA->lfWidth,
		lplfA->lfEscapement,
		lplfA->lfOrientation,
		lplfA->lfWeight,
		lplfA->lfItalic,
		lplfA->lfUnderline,
		lplfA->lfStrikeOut,
		lplfA->lfCharSet,
		lplfA->lfOutPrecision,
		lplfA->lfClipPrecision,
		lplfA->lfQuality,
		lplfA->lfPitchAndFamily,
	};
	MultiByteToWideChar(CP_ACP, 0, lplfA->lfFaceName, LF_FACESIZE, lf.lfFaceName, LF_FACESIZE);

	LOGFONT* lplf = &lf;
	if (pSettings->CopyForceFont(lf, lf)) {
		lplf = &lf;
	}
	HFONT hf = IMPL_CreateFontIndirectW(lplf);
//	if(hf && lplf && !pSettings->LoadOnDemand()) {
//		AddFontToFT(lplf->lfFaceName, lplf->lfWeight, !!lplf->lfItalic);
//	}
	return hf;
}
*/

//Snowie!!
LPCWSTR GetCachedFont(HFONT lFont)
{
	CFontCache::const_iterator it= FontCache.find(lFont);
	if (it==FontCache.end())
		return NULL;
	else
		return it->second->lpRealName;
}

LPCWSTR GetCachedFontLocale(HFONT lFont)
{
	CFontCache::const_iterator it= FontCache.find(lFont);
	if (it==FontCache.end())
		return NULL;
	else
		return it->second->lpGDIName;
}

void AddToCachedFont(HFONT lfont, LPWSTR lpFaceName, LPWSTR lpGDIName)
{
	if (!lfont) return;	//ｲｻｿﾉﾒﾔﾌ�ｼﾓｿﾕｽﾚｵ�
	CCriticalSectionLock __lock(CCriticalSectionLock::CS_CACHEDFONT);
	if (GetCachedFont(lfont)) return;	//ﾒﾑｾｭｴ贇ﾚｵﾄﾗﾖﾌ�
	FontCache[lfont] = new CFontSubResult(lpFaceName, lpGDIName);
}

void DeleteCachedFont(HFONT lfont)
{
	if (!lfont) return;	//ｲｻｿﾉﾒﾔﾉｾｳ�ﾍｷｽ盞�
	CCriticalSectionLock __lock(CCriticalSectionLock::CS_CACHEDFONT);
	CFontCache::iterator it= FontCache.find(lfont);
	if (it!=FontCache.end())
	{
		delete it->second;
		FontCache.erase(it);
	}
}

int WINAPI IMPL_GetTextFaceAliasW(HDC hdc, int nLen, LPWSTR lpAliasW)
{
	//CThreadCounter __counter;
	int bResult = ORIG_GetTextFaceAliasW(hdc, nLen, lpAliasW);
	//LOGFONT lf, lf2;
	//StringCchCopy(lf.lfFaceName, LF_FACESIZE, lpAliasW);
	//LOGFONT * lplf = &lf;
	LPCWSTR fontcache=GetCachedFont(GetCurrentFont(hdc));
	if (fontcache){
		if (lpAliasW)
			StringCchCopy(lpAliasW, LF_FACESIZE, fontcache);
		bResult = wcslen(fontcache)+1;
	}
	return bResult;
}

// Won't get any better for clipbox, obsolete.
/*
cache::lru_cache<HFONT, int> FontHeightCache(200);	// cache 200 most frequently used fonts' height
const WCHAR TEST_ALPHABET_SEQUENCE[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

int GetFontMaxAlphabetHeight(HDC dc, MAT2 *lpmt2) {
	HFONT ft = GetCurrentFont(dc);
	if (FontHeightCache.exists(ft))
		return FontHeightCache.get(ft);
	GLYPHMETRICS lppm = { 0 };
	int nHeight = 0;
	for (int i = 0; i < 26; ++i) {
		ORIG_GetGlyphOutlineW(dc, TEST_ALPHABET_SEQUENCE[i], GGO_METRICS, &lppm, 0, 0, lpmt2);
		if (lppm.gmptGlyphOrigin.y>nHeight)
			nHeight = lppm.gmptGlyphOrigin.y;
	}
	FontHeightCache.put(ft, nHeight);
	return nHeight;
}*/

DWORD WINAPI IMPL_GetGlyphOutlineW(__in HDC hdc, __in UINT uChar, __in UINT fuFormat, __out LPGLYPHMETRICS lpgm, __in DWORD cjBuffer, __out_bcount_opt(cjBuffer) LPVOID pvBuffer, __in CONST MAT2 *lpmat2)
{
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	DWORD ret= ORIG_GetGlyphOutlineW(hdc, uChar, fuFormat, lpgm, cjBuffer, pvBuffer, lpmat2);
	if (pSettings->EnableClipBoxFix() && (!cjBuffer || !pvBuffer)) {
		if (!(fuFormat & (GGO_BITMAP | GGO_GRAY2_BITMAP | GGO_GRAY4_BITMAP | GGO_GRAY8_BITMAP | GGO_NATIVE))) {
			//lpgm->gmptGlyphOrigin.x -= 1;
			//lpgm->gmptGlyphOrigin.y += 1;
			//lpgm->gmBlackBoxX += 3;
			//lpgm->gmBlackBoxY += 2;

			static int n = (int)floor(1.5*pSettings->ScreenDpi() / 96);
			int nDeltaY = n, nDeltaBlackY = n;
			TEXTMETRIC tm = { 0 };
			GetTextMetrics(hdc, &tm);
			if (lpgm->gmptGlyphOrigin.y < tm.tmAscent) {	// origin out of the top of the box
				if (lpgm->gmptGlyphOrigin.y + nDeltaY>tm.tmAscent) {
					nDeltaY = tm.tmAscent - lpgm->gmptGlyphOrigin.y;	// limit the top position of the origin
				}
			}
			else nDeltaY = 0;
			lpgm->gmptGlyphOrigin.y += nDeltaY;
			/*if (lpgm->gmptGlyphOrigin.x > 0)
				lpgm->gmBlackBoxX += n; // increase blackbox width if it's not a ligature
			if (lpgm->gmBlackBoxX > tm.tmMaxCharWidth) {
				lpgm->gmBlackBoxX = tm.tmMaxCharWidth;
			}*/
			lpgm->gmBlackBoxY += nDeltaY;
			if (tm.tmAscent - lpgm->gmptGlyphOrigin.y + lpgm->gmBlackBoxY - 1 < tm.tmHeight)	// still has some room to scale up
			{
				if (tm.tmAscent - lpgm->gmptGlyphOrigin.y + lpgm->gmBlackBoxY + 1 + nDeltaBlackY > tm.tmHeight)
					lpgm->gmBlackBoxY = tm.tmHeight - tm.tmAscent + lpgm->gmptGlyphOrigin.y + 1;
				else
					lpgm->gmBlackBoxY += nDeltaBlackY;
			}
		}
	}
// 	TEXTMETRIC tm;
// 	GetTextMetrics(hdc, &tm);

	return ret;
}

DWORD WINAPI IMPL_GetGlyphOutlineA(__in HDC hdc, __in UINT uChar, __in UINT fuFormat, __out LPGLYPHMETRICS lpgm, __in DWORD cjBuffer, __out_bcount_opt(cjBuffer) LPVOID pvBuffer, __in CONST MAT2 *lpmat2)
{
	//fuFormat |= GGO_UNHINTED;
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	DWORD ret=  ORIG_GetGlyphOutlineA(hdc, uChar, fuFormat, lpgm, cjBuffer, pvBuffer, lpmat2);
// 	if (pSettings->EnableClipBoxFix())
// 	{
// 		lpgm->gmptGlyphOrigin.y+=1;
// 		lpgm->gmBlackBoxY+=1;
// 	}
	if (pSettings->EnableClipBoxFix() && (!cjBuffer || !pvBuffer)) {
		if (!(fuFormat & (GGO_BITMAP | GGO_GRAY2_BITMAP | GGO_GRAY4_BITMAP | GGO_GRAY8_BITMAP | GGO_NATIVE))) {
			static int n = (int)floor(1.5*pSettings->ScreenDpi() / 96);
			int nDeltaY = n, nDeltaBlackY = n;
			TEXTMETRIC tm = { 0 };
			GetTextMetrics(hdc, &tm);
			if (lpgm->gmptGlyphOrigin.y < tm.tmAscent) {	// origin out of the top of the box
				if (lpgm->gmptGlyphOrigin.y + nDeltaY>tm.tmAscent) {
					nDeltaY = tm.tmAscent - lpgm->gmptGlyphOrigin.y;	// limit the top position of the origin
				}
			}
			else nDeltaY = 0;
			/*if (lpgm->gmptGlyphOrigin.x > 0)
				lpgm->gmBlackBoxX += n; // increase blackbox width if it's not a ligature
			if (lpgm->gmBlackBoxX > tm.tmMaxCharWidth) {
				lpgm->gmBlackBoxX = tm.tmMaxCharWidth;
			}*/
			lpgm->gmptGlyphOrigin.y += nDeltaY;	

			lpgm->gmBlackBoxY += nDeltaY;
			if (tm.tmAscent - lpgm->gmptGlyphOrigin.y + lpgm->gmBlackBoxY - 1 < tm.tmHeight)
			{
				if (tm.tmAscent - lpgm->gmptGlyphOrigin.y + lpgm->gmBlackBoxY + 1 + nDeltaBlackY > tm.tmHeight)
					lpgm->gmBlackBoxY = tm.tmHeight - tm.tmAscent + lpgm->gmptGlyphOrigin.y + 1;
				else
					lpgm->gmBlackBoxY += nDeltaBlackY;
			}
		}
	}
	return ret;
}



int WINAPI IMPL_GetTextFaceW( __in HDC hdc, __in int c, __out_ecount_part_opt(c, return)  LPWSTR lpName)
{
	int nSize = ORIG_GetTextFaceW(hdc, c, lpName);
	LPCWSTR fontcache=GetCachedFontLocale(GetCurrentFont(hdc));
	
	if (fontcache){
		if (lpName) {
			int len = Min(LF_FACESIZE, c);
			StringCchCopy(lpName, len, fontcache);
			nSize = (int)wcslen(fontcache) > len ? len : wcslen(fontcache) + 1;
		}
		else {
			// a request for the size of font
			nSize = Min(LF_FACESIZE, (int)wcslen(fontcache)+1);
		}
	}
	return nSize;
}

int WINAPI IMPL_GetTextFaceA( __in HDC hdc, __in int c, __out_ecount_part_opt(c, return)  LPSTR lpName)
{
	int nSize = ORIG_GetTextFaceA(hdc, c, lpName);
	LPCWSTR fontcache=GetCachedFontLocale(GetCurrentFont(hdc));
	if (fontcache){
		//int len=Min(LF_FACESIZE, c);
		size_t _Dsize = 2 * wcslen(fontcache) + 1;
		char *_Dest = new char[_Dsize];
		memset(_Dest,0,_Dsize);
		int len =wcstombs(_Dest, fontcache, _Dsize);
		if (lpName)
			StringCchCopyA(lpName, LF_FACESIZE, _Dest);
		delete[] _Dest;
		nSize = len+1;
	}
	return nSize;
}

HGDIOBJ WINAPI IMPL_GetStockObject(__in int i)
{
	switch (i)
	{
		case DEFAULT_GUI_FONT:
			{
				static const CGdippSettings* pSetting = CGdippSettings::GetInstance();
				if (g_alterGUIFont)
					return g_alterGUIFont;
				else
					return ORIG_GetStockObject(i);
			}
/*
		case SYSTEM_FONT:
			{
				if (g_alterSysFont)
					return g_alterSysFont;
				break;
			}*/

		default: return ORIG_GetStockObject(i);
	}
	return ORIG_GetStockObject(i);

}

BOOL WINAPI IMPL_BeginPath(HDC hdc)
{
	//CThreadCounter __counter;
	BOOL ret=ORIG_BeginPath(hdc);
	if (ret)
	{
		DCArray.insert(hdc);
	}
	return ret;
}

BOOL WINAPI IMPL_EndPath(HDC hdc)
{
	//CThreadCounter __counter;
	BOOL ret=ORIG_EndPath(hdc);
	if (ret)
	{
		DCArray.erase(hdc);
	}
	return ret;
}

BOOL WINAPI IMPL_AbortPath(HDC hdc)
{
	//CThreadCounter __counter;
	BOOL ret=ORIG_AbortPath(hdc);
	if (ret)
	{
		DCArray.erase(hdc);
	}
	return ret;
}

int WINAPI IMPL_GetObjectA(__in HANDLE h, __in int c, __out_bcount_opt(c) LPVOID pv)
{
	int ret = ORIG_GetObjectA(h, c, pv);
	if (ret==sizeof(LOGFONTA))
	{
		LPCWSTR fontcache = GetCachedFont((HFONT)h);
		if (fontcache && pv)
		{
			size_t _Dsize = 2 * wcslen(fontcache) + 1;
			char *_Dest = new char[_Dsize];
			memset(_Dest,0,_Dsize);
			wcstombs(_Dest,fontcache,_Dsize);
			StringCchCopyA(((LOGFONTA*)pv)->lfFaceName, LF_FACESIZE, _Dest);
			delete []_Dest;
		}
	}
	return ret;
}

int WINAPI IMPL_GetObjectW(__in HANDLE h, __in int c, __out_bcount_opt(c) LPVOID pv)
{
	int ret = ORIG_GetObjectW(h, c, pv);
	if (ret==sizeof(LOGFONTW))
	{
		LPCWSTR fontcache = GetCachedFont((HFONT)h);
		if (fontcache && pv)
		{
			StringCchCopyW(((LOGFONTW*)pv)->lfFaceName, LF_FACESIZE, fontcache);
		}
	}
	return ret;
}

HFONT WINAPI IMPL_CreateFontIndirectExW(CONST ENUMLOGFONTEXDV *penumlfex)
{
	if (!penumlfex) return NULL;
	TRACE(L"Creating font \"%s\"\n", penumlfex->elfEnumLogfontEx.elfLogFont.lfFaceName);
	{
		if (penumlfex->elfEnumLogfontEx.elfLogFont.lfClipPrecision == FONT_MAGIC_NUMBER)
		{		
			TRACE(L"Engine font, Ignored, ");
			((ENUMLOGFONTEXDV *)penumlfex)->elfEnumLogfontEx.elfLogFont.lfClipPrecision = 0;
			return ORIG_CreateFontIndirectExW(penumlfex);
		}
	}

	/*
		GetEnvironmentVariableW(L"MACTYPE_FONTSUBSTITUTES_ENV", NULL, 0);
			if (GetLastError()!=ERROR_ENVVAR_NOT_FOUND)
				return ORIG_CreateFontIndirectExW(penumlfex);*/
		

	if(!penumlfex) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return NULL;
		}
		const CGdippSettings* pSettings = CGdippSettings::GetInstance();
		if (pSettings->IsFontExcluded(penumlfex->elfEnumLogfontEx.elfLogFont.lfFaceName)) {
			TRACE(L"Font exception! ");
			return ORIG_CreateFontIndirectExW(penumlfex);
			//TRACE(L"Handle = %x\n" , (int)h);
		}


		ENUMLOGFONTEXDV mfont(*penumlfex);
		LOGFONT& lf = mfont.elfEnumLogfontEx.elfLogFont;
		LOGFONT lfOrg(lf);
		BOOL bForced = false;
		if (bForced = pSettings->CopyForceFont(lf, lfOrg)) {
/*
			mfont->elfEnumLogfontEx.elfLogFont.lfHeight			= lf.lfHeight;
	//			mfont->elfEnumLogfontEx.elfLogFont.lfWidth			= lfOrg.lfWidth;
			mfont->elfEnumLogfontEx.elfLogFont.lfWidth			= lf.lfWidth;
			mfont->elfEnumLogfontEx.elfLogFont.lfEscapement		= lf.lfEscapement;
			mfont->elfEnumLogfontEx.elfLogFont.lfOrientation	= lf.lfOrientation;
			mfont->elfEnumLogfontEx.elfLogFont.lfWeight			= lf.lfWeight;
			mfont->elfEnumLogfontEx.elfLogFont.lfItalic			= lf.lfItalic;
			mfont->elfEnumLogfontEx.elfLogFont.lfUnderline		= lf.lfUnderline;
			mfont->elfEnumLogfontEx.elfLogFont.lfStrikeOut		= lf.lfStrikeOut;
			mfont->elfEnumLogfontEx.elfLogFont.lfCharSet		= lf.lfCharSet;
			mfont->elfEnumLogfontEx.elfLogFont.lfOutPrecision	= 0;
			mfont->elfEnumLogfontEx.elfLogFont.lfClipPrecision	= 0;
			mfont->elfEnumLogfontEx.elfLogFont.lfQuality		= 0;
			mfont->elfEnumLogfontEx.elfLogFont.lfPitchAndFamily	= 0;
			StringCchCopy(mfont->elfEnumLogfontEx.elfLogFont.lfFaceName, LF_FACESIZE, lf.lfFaceName);*/

			TRACE(L"Font substitutes to \"%s\"\n", lf.lfFaceName);
		}
		else
			TRACE(L"Normal font\n");
		//bypass = true;
		HFONT hf = ORIG_CreateFontIndirectExW(&mfont);
		//ORIG_CreateFontIndirectExW(
		//if(hf && lplf && !pSettings->LoadOnDemand()) {
		//	AddFontToFT(lplf->lfFaceName, lplf->lfWeight, !!lplf->lfItalic);
		//}
		if (hf && bForced) {	
			AddToCachedFont(hf, (WCHAR*)penumlfex->elfEnumLogfontEx.elfLogFont.lfFaceName, (WCHAR *)lfOrg.lfFaceName);
		}
		//bypass = false;
		TRACE(L"Create font %s handle %x\n", lfOrg.lfFaceName, (int)hf);
		return hf;
}

BOOL WINAPI IMPL_DeleteObject(HGDIOBJ hObject)
{
	//CThreadCounter __counter;
	if (hObject == g_alterGUIFont)	//ﾎﾒｵﾄﾏｵﾍｳﾗﾖﾌ螢ｬｲｻｿﾉﾒﾔﾊﾍｷﾅｵ�
		return true;
	BOOL bResult = ORIG_DeleteObject(hObject);
	if (bResult) DeleteCachedFont((HFONT)hObject);
	return bResult;
}


HFONT WINAPI IMPL_CreateFontIndirectW(CONST LOGFONTW *lplf)	//ﾖﾘﾐﾂﾆ�ﾓﾃﾕ篋�hook｣ｬﾖｻﾎｪｼ貶ﾝﾋﾑｹｷﾊ菠�ｷｨ
{
	ENUMLOGFONTEXDVW envlf = {0};
	memcpy(&envlf.elfEnumLogfontEx.elfLogFont, lplf, sizeof(LOGFONTW));
	return IMPL_CreateFontIndirectExW(&envlf);
}


/*
BOOL WINAPI IMPL_DrawStateA(HDC hdc, HBRUSH hbr, DRAWSTATEPROC lpOutputFunc, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT uFlags)
{
	//CThreadCounter __counter;
	int cchW;
	LPWSTR lpStringW;

	if(lData && uFlags & DSS_DISABLED) {
		switch(uFlags & 0x0f) {
		case DST_TEXT:
		case DST_PREFIXTEXT:
			lpStringW = _StrDupAtoW((LPCSTR)lData, wData ? wData : -1, &cchW);
			if(lpStringW) {
				BOOL ret = IMPL_DrawStateW(hdc, hbr, lpOutputFunc, (LPARAM)lpStringW, cchW, x, y, cx, cy, uFlags);
				free(lpStringW);
				return ret;
			}
			break;
		}
	}
	return ORIG_DrawStateA(hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy, uFlags);
}

//灰色描画をDrawTextへ投げる
//DrawTextは内部でExtTextOutしてくれるので問題なし
BOOL WINAPI IMPL_DrawStateW(HDC hdc, HBRUSH hbr, DRAWSTATEPROC lpOutputFunc, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT uFlags)
{
	//CThreadCounter __counter;
	if(lData && uFlags & DSS_DISABLED) {
		switch(uFlags & 0x0f) {
		case DST_TEXT:
		case DST_PREFIXTEXT:
			{
			//wData==0の時に文字数自動計算
			//他のAPIと違って-1の時ではない
			if(wData == 0) {
				wData = wcslen((LPCWSTR)lData);
			}
			RECT rect = { x, y, x + 10000, y + 10000 };
			//どうやら3DHighLightの上に1pxずらして3DShadowを重ねてるらしい
			int oldbm = SetBkMode(hdc, TRANSPARENT);
			COLORREF oldfg = SetTextColor(hdc, GetSysColor(COLOR_3DHIGHLIGHT));
			//DrawStateとDrawTextではprefixのフラグが逆(APIの設計ダメすぎ)
			const UINT uDTFlags = DT_SINGLELINE | DT_NOCLIP | (uFlags & DST_PREFIXTEXT ? 0 : DT_NOPREFIX);

			OffsetRect(&rect, 1, 1);
			DrawTextW(hdc, (LPCWSTR)lData, wData, &rect, uDTFlags);
			SetTextColor(hdc, GetSysColor(COLOR_3DSHADOW));
			OffsetRect(&rect, -1, -1);
			DrawTextW(hdc, (LPCWSTR)lData, wData, &rect, uDTFlags);
			SetTextColor(hdc, oldfg);
			SetBkMode(hdc, oldbm);
			}
			return TRUE;
		}
	}
	return ORIG_DrawStateW(hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy, uFlags);
}*/


class FreeTypeFontEngine;
extern FreeTypeFontEngine* g_pFTEngine;


BOOL __stdcall IMPL_RemoveFontResourceExW(__in LPCWSTR name, __in DWORD fl, __reserved PVOID pdv)
{
	g_pFTEngine->RemoveFont(name);
	return ORIG_RemoveFontResourceExW(name, fl, pdv);
}

/*
BOOL __stdcall IMPL_RemoveFontResourceW(__in LPCWSTR lpFileName)
{
	g_pFTEngine->RemoveFont(lpFileName);
	return ORIG_RemoveFontResourceW(lpFileName);
}*/

BOOL WINAPI IMPL_TextOutA(HDC hdc, int nXStart, int nYStart, LPCSTR lpString, int cbString)
{
	//CThreadCounter __counter;
	return IMPL_ExtTextOutA(hdc, nXStart, nYStart, NULL, NULL, lpString, cbString, NULL);
}



BOOL WINAPI IMPL_TextOutW(HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cbString)
{
	//CThreadCounter __counter;
	return IMPL_ExtTextOutW(hdc, nXStart, nYStart, NULL, NULL, lpString, cbString, NULL);
}




void AnsiDxToUnicodeDx(LPCSTR lpStringA, int cbString, const int* lpDxA, int* lpDxW, int ACP)
{
	LPCSTR lpEndA = lpStringA + cbString;
	while(lpStringA < lpEndA) {
		*lpDxW = *lpDxA++;
		if(IsDBCSLeadByteEx(ACP, *lpStringA)) {
			*lpDxW += *lpDxA++;
			lpStringA++;
		}
		lpDxW++;
		lpStringA++;
	}
}

// ANSI->Unicodeに変換してExtTextOutWに投げるExtTextOutA

BOOL WINAPI IMPL_ExtTextOutA(HDC hdc, int nXStart, int nYStart, UINT fuOptions, CONST RECT *lprc, LPCSTR lpString, UINT cbString, CONST INT *lpDx)
{
	//CThreadCounter __counter;
	if (!hdc || !lpString || !cbString || !g_ccbRender || !(fuOptions & ETO_IGNORELANGUAGE)) {
		return ORIG_ExtTextOutA(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
	}

	//However, if the ANSI version of ExtTextOut is called with ETO_GLYPH_INDEX,
	//the function returns TRUE even though the function does nothing.
	//とりあえずオリジナルに飛ばす
	if (fuOptions & ETO_GLYPH_INDEX)
		return ORIG_ExtTextOutA(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);

	//HDBENCHチート
//	if (lpString && cbString == 7 && pSettings->IsHDBench() && (memcmp(lpString, "HDBENCH", 7) == 0 || memcmp(lpString, "       ", 7) == 0))
//		return ORIG_ExtTextOutA(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);

	LPWSTR lpszUnicode;
	int bufferLength;
	BOOL result;
	WCHAR szStack[CCH_MAX_STACK];
	int dxStack[CCH_MAX_STACK];
	int nACP = GdiGetCodePage(hdc);//CP_ACP;
	//DWORD nCharset = GetTextCharsetInfo(hdc, NULL, 0);

	/*
	switch (nCharset)
		{
		case 0:
			{
				break;
			}
		case SYMBOL_CHARSET:
			{
				nACP = CP_SYMBOL;
				break;
			}
		case MAC_CHARSET:
			{
				nACP = CP_MACCP;
				break;
			}
		case OEM_CHARSET:
			{
				nACP = CP_OEMCP;
				break;
			}
		default:
			{
				CHARSETINFO Cs={0,CP_ACP,0};
				TranslateCharsetInfo((DWORD*)nCharset, &Cs, TCI_SRCCHARSET);
				nACP = Cs.ciACP;
			}
		}*/
	

	lpszUnicode = _StrDupExAtoW(lpString, cbString, szStack, CCH_MAX_STACK, &bufferLength, nACP);
	if(!lpszUnicode) {
		//メモリ不足: 一応オリジナルに投げとく
		return ORIG_ExtTextOutA(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
	}

	int* lpDxW = NULL;
	result = FALSE;
	if(lpDx && cbString && _getmbcp()) {
		if (cbString < CCH_MAX_STACK) {
			lpDxW = dxStack;
			ZeroMemory(lpDxW, sizeof(int) * cbString);
		} else {
			lpDxW = (int*)calloc(sizeof(int), cbString);
			if (!lpDxW) {
				goto CleanUp;
			}
		}
		if (nACP!=CP_SYMBOL)
		{
			AnsiDxToUnicodeDx(lpString, cbString, lpDx, lpDxW, nACP);
			lpDx = lpDxW;
		}
	}

	result = IMPL_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, (LPCWSTR)lpszUnicode, bufferLength, lpDx);

CleanUp:
	if (lpszUnicode != szStack)
		free(lpszUnicode);
	if (lpDxW != dxStack)
		free(lpDxW);
	return result;
}




typedef enum {
	ETOE_OK				= 0,
	ETOE_CREATEDC		= 1,
	ETOE_SETFONT		= 2,
	ETOE_CREATEDIB		= 3,
	ETOE_FREETYPE		= 4,
	ETOE_INVALIDARG		= 11,
	ETOE_ROTATION		= 12,
	ETOE_LARGESIZE		= 13,
	ETOE_INVALIDHDC		= 14,
	ETOE_ROTATEFONT		= 15,
	ETOE_NOAREA			= 16,
	ETOE_GETTEXTEXTENT	= 17,
	ETOE_MONO			= 18,
	ETOE_GENERAL		= 19,
} ExtTextOut_ErrorCode;

//例外モドキ
#define ETO_TRY()		ExtTextOut_ErrorCode error = ETOE_OK; {
#define ETO_THROW(code)	error = (code); goto _EXCEPTION_THRU
#define ETO_CATCH()		} _EXCEPTION_THRU:

/*
BOOL FreeTypeGetTextExtentPoint(HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize, const FREETYPE_PARAMS* params)
{
	WCHAR szStack[CCH_MAX_STACK];

	int cchStringW;
	LPWSTR lpStringW = _StrDupExAtoW(lpString, cbString, szStack, CCH_MAX_STACK, &cchStringW);
	if(!lpStringW) {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	BOOL ret = FreeTypeGetTextExtentPoint(hdc, lpStringW, cchStringW, lpSize, params);
	if (lpStringW != szStack)
		free(lpStringW);
	return ret;
}*/


class CETOBitmap
{
private:
	CBitmapCache&	m_cache;
	HDC				m_hdc;
	HBITMAP			m_hPrevBmp;
	HBITMAP			m_hBmp;
	BYTE*			m_lpPixels;

public:
	CETOBitmap(CBitmapCache& cache)
		: m_cache(cache)
		, m_hdc(NULL)
		, m_hPrevBmp(NULL)
		, m_hBmp(NULL)
		, m_lpPixels(NULL)
	{
	}
	HDC CreateDC(HDC dc)
	{
		m_hdc = m_cache.CreateDC(dc);
		return m_hdc;
	}
	bool CreateDIBandSelect(int cx, int cy)
	{
		m_hBmp = m_cache.CreateDIB(cx, cy, &m_lpPixels);
		if (!m_hBmp) {
			return false;
		}
		m_hPrevBmp = SelectBitmap(m_hdc, m_hBmp);
		return true;
	}
	void RestoreBitmap()
	{
		if (m_hPrevBmp) {
			SelectBitmap(m_hdc, m_hPrevBmp);
			m_hPrevBmp = NULL;
		}
	}
};

extern ControlIder CID;
// ﾈ｡ｴ�WindowsｵﾄExtTextOutW
BOOL WINAPI IMPL_ExtTextOutW(HDC hdc, int nXStart, int nYStart, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpString, UINT cbString, CONST INT *SyslpDx)
{
	//CThreadCounter __counter;		//ﾓﾃﾓﾚｰｲﾈｫﾍﾋｳ�ｵﾄｼﾆﾊ�ﾆ�
	INT* lpDx = const_cast<INT*>(SyslpDx);

	if (!hdc || !lpString || !cbString || !g_ccbRender || cbString>8192) {		//no valid param or rendering is disabled from control center.
		return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
	}
	if (!(fuOptions & ETO_GLYPH_INDEX) && cbString==1 && *lpString==32)	//ｿﾕｸ�
		return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions | ETO_IGNORELANGUAGE, lprc, lpString, cbString, lpDx);	//ｿﾕｸ�ｾﾍｲｻﾓﾃｴｦﾀ�ﾁﾋ｡｣｡｣｡｣ｷｴﾕ�ｶｼﾒｻﾑ�

	CThreadLocalInfo* pTLInfo = g_TLInfo.GetPtr();		
	if(!pTLInfo) {
		return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
	}

	if (DCArray.find(hdc)!=DCArray.end())
		return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
	CAutoVectorPtr<INT> newdx;
	if (!lpDx) {
		newdx.Allocate(cbString);
		SIZE p = { 0 };
		BOOL r = false;
		if (fuOptions & ETO_GLYPH_INDEX)
			r = GetTextExtentExPointI(hdc, (LPWORD)lpString, cbString, 0, NULL, newdx, &p);
		else
			r = GetTextExtentExPointW(hdc, lpString, cbString, 0, NULL, newdx, &p);
		if (r) {
			for (int i = cbString - 1; i > 0; --i) {
				newdx[i] -= newdx[i - 1];
			}
			lpDx = newdx;
		}
		else{
			newdx.Free();
		}
	}

	if (!(fuOptions & ETO_GLYPH_INDEX) && !(fuOptions & ETO_IGNORELANGUAGE) && !lpDx && CID.myiscomplexscript(lpString,cbString))		//complex script
		return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
	const CGdippSettings* pSettings = CGdippSettings::GetInstance(); //ｻ�ｵﾃﾒｻｸ�ﾅ葷ﾃﾎﾄｼ�ﾊｵﾀ�

/*

#ifndef _DEBUG		//debugﾄ｣ﾊｽﾏﾂｴﾋｲﾎﾊ�ﾓﾐﾎﾊﾌ�
	if (pSettings->FontLoader()==SETTING_FONTLOADER_WIN32)
	{
		if (!(fuOptions & ETO_GLYPH_INDEX) 	//ｸｴﾔﾓﾎﾄｼ�｣ｬｲｻｽ�ﾐﾐ葷ﾈｾ
		&& !(fuOptions & ETO_IGNORELANGUAGE) && ScriptIsComplex(lpString, cbString, SIC_COMPLEX) == S_OK) {
			return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
		}
	}
	else
		if (!(fuOptions & ETO_GLYPH_INDEX) && / *iswcntrl(lpString[0])* /CID.myiswcntrl(lpString[0])) {
			return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
		}
#endif

*/


	
	if (pTLInfo->InExtTextOut()) {	//ﾊﾇﾒ�ｳ｣ﾖｮｺ�ｵﾄﾗﾔｶｯｻｹﾔｭﾖｴﾐﾐ
		return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
	}

	XFORM xfm;
	static XFORM stdXfm = {1.0,0,0,1.0,0,0};
	bool bZoomedDC = false;
	CDCTransformer* DCTrans = NULL;
	if (GetTransform)
	{
		GetTransform(hdc, GT_WORLD_TO_DEVICE, &xfm);
		if (memcmp(&xfm, &stdXfm, sizeof(XFORM)-sizeof(FLOAT)*2)) //(xfm.eM11!=1.0 || xfm.eM22!=1.0)	//ﾈ郢�ｴ贇ﾚﾗ�ｱ�ﾗｪｻｻ
		{
			bool bZoomInOut = (xfm.eM12==0 && xfm.eM21==0 && xfm.eM11>0 && xfm.eM22>0);	//ﾖｻﾊﾇﾋ�ｷﾅ,ﾇﾒﾊﾇﾕ�ﾊ�ﾋ�ｷﾅ
			if (!bZoomInOut)
				return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);	//ｷﾅﾆ�葷ﾈｾ
			else
			{
				bZoomedDC = true;
				DCTrans = new CDCTransformer;
				DCTrans->init(xfm);
			}
		}
	}

/*
	int mm = GetMapMode(hdc);
	if (mm!=MM_TEXT)
	{
		SIZE size;
		GetWindowExtEx(hdc, &size);
		if (size.cx!=1 || size.cy!=1)
			return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
	}
	if (GetGraphicsMode(hdc)==GM_ADVANCED)
	{
		XFORM xfm;
		GetWorldTransform(hdc, &xfm);
		if (xfm.eM11!=1.0 || xfm.eM22!=1.0)
			return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
	}*/
	
	//if (GetROP2(hdc)!=13)
	//	return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
	//if (GetStretchBltMode(hdc)!=BLACKONWHITE)
	//	return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);

/*
	SIZE size;
	if (fuOptions & ETO_GLYPH_INDEX)
		GetTextExtentPointI(hdc,(LPWORD)lpString, cbString,&size);
	else
		GetTextExtentPoint(hdc, lpString, cbString, &size);
	if (!size.cx)
		return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);*/

	COwnedCriticalSectionLock __lock2(1, COwnedCriticalSectionLock::OCS_DC);	//ｻ�ﾈ｡ﾋ�ﾓﾐﾈｨ｣ｬｷﾀﾖｹｳ袁ｻ
	CBitmapCache& cache	= pTLInfo->BitmapCache();
	CETOBitmap bmp(cache);

	HDC		hCanvasDC		= NULL;
	HFONT	hPrevFont		= NULL;
	HFONT	hMyCurFont		= NULL;
	HFONT	hZoomedFont = NULL, hOldMDCFont = NULL;
	BOOL	bForceFont = false;
	int * outlpDx = NULL;
	FT_Referenced_Glyph* GlyphArray = new FT_Referenced_Glyph[cbString];
	FT_DRAW_STATE* ftDrawState = new FT_DRAW_STATE[cbString];
	memset(GlyphArray, 0, sizeof(FT_Referenced_Glyph)*cbString);
	memset(ftDrawState, 0, sizeof(FT_DRAW_STATE)*cbString);
	OUTLINETEXTMETRIC* otm = NULL;

ETO_TRY();
	//ﾉ靹ﾃｱ�ﾖｾ｣ｬ
	pTLInfo->InExtTextOut(true);

	POINT	curPos = { nXStart, nYStart };	//ｼﾇﾂｼｿｪﾊｼｵﾄﾎｻﾖﾃ
	POINT	destPos;
	SIZE	drawSize;

	HFONT	hCurFont		= NULL;
	BOOL	bShadow			= FALSE;

	UINT	align;
	SIZE	textSize;
	SIZE	realSize = { 0 };

//================ﾅﾐｶﾏﾊﾇｷ�ﾎｪﾓﾐﾐｧｵﾄDC=====================
	if (!IsValidDC(hdc)) {	
		ETO_THROW(ETOE_INVALIDHDC);	// hdc is invalid
	}

	int nSize=GetOutlineTextMetrics(hdc, 0, NULL);
	if (!nSize) {
		//nSize = sizeof(OUTLINETEXTMETRIC);
		ETO_THROW(ETOE_INVALIDHDC);
	}	//ｺﾄﾊｱ50-100ms

	otm = (OUTLINETEXTMETRIC*)malloc(nSize);
	memset(otm, 0, nSize);
	otm->otmSize = nSize;
	TEXTMETRIC& tm = otm->otmTextMetrics;
	LOGFONT	lf = { 0 };
	wstring strFamilyName;
	GetOutlineTextMetrics(hdc, nSize, otm);
/*
	if (!GetOutlineTextMetrics(hdc, nSize, otm))	//ｻ�ｵﾃﾕ賁ｵｵﾄﾗﾖﾌ衵ﾅﾏ｢
	{
		GetTextMetrics(hdc, &tm);	//ｻ�ｵﾃｱｸﾓﾃﾐﾅﾏ｢
		WCHAR buff[LF_FACESIZE];
		ORIG_GetTextFace(hdc, LF_FACESIZE, buff);
		strFamilyName = buff;
	}
	else*/

		strFamilyName = (LPWSTR)((DWORD_PTR)otm+(DWORD_PTR)otm->otmpFamilyName);	//ｻ�ｵﾃTTFﾐﾅﾏ｢

	const bool bVertical = pSettings->FontLoader()==SETTING_FONTLOADER_FREETYPE?  strFamilyName.c_str()[0]==L'@' :false;

	int nFontHeight = bZoomedDC ? DCTrans->TransformYAB(tm.tmHeight) : tm.tmHeight;
	if ((pSettings->MaxHeight() && nFontHeight > pSettings->MaxHeight()) || (pSettings->MinHeight() && nFontHeight < pSettings->MinHeight()))  {
		ETO_THROW(ETOE_INVALIDHDC);	//Font size too small or too big.
	}

	if (pSettings->IsFontExcluded(strFamilyName.c_str())) {	//ｱﾈｽﾏﾊﾇｷ�ﾊﾇﾅﾅｳ�ｵ�ｵﾄﾗﾖﾌ�
		ETO_THROW(ETOE_INVALIDHDC);
	}	//20-50ms

//====================end================================

	hCanvasDC = bmp.CreateDC(hdc);
	if(!hCanvasDC) {
		ETO_THROW(ETOE_CREATEDC);
	}	//0ms

	align = GetTextAlign(hdc);	//ｻ�ｵﾃｶﾔﾆ�ｷｽﾊｽ
	//if (pTLInfo->InUniscribe() && !(fuOptions & ETO_IGNORELANGUAGE))
	//	align &= ~TA_UPDATECP;
	if(align & TA_UPDATECP) {
		GetCurrentPositionEx(hdc, &curPos);
	}
/*
	if (!align && lpDx && !fuOptions)	//optimized
	{
	//	ETO_THROW(ETOE_FREETYPE);
	}//0ms*/


	hCurFont = GetCurrentFont(hdc);	//ｻ�ｵﾃｵｱﾇｰdcｵﾄﾗﾖﾌ螢ｬﾗ｢ﾒ籏ﾖﾌ蠹�ｳﾆｿﾉﾄﾜﾊﾇｴ�ﾎ�ｵﾄ
	if (!hCurFont) {		//ｻ�ｵﾃﾊｧｰﾜ
		ETO_THROW(ETOE_SETFONT);
	}
	TRACE(L"Draw text \"%s\", font=\"%s\", handle=%x\n", lpString, strFamilyName.c_str(), (int)hCurFont);
	if (!ORIG_GetObjectW(hCurFont, sizeof(LOGFONT), &lf)) {
		ETO_THROW(ETOE_SETFONT);
	}//30ms
	StringCchCopy(lf.lfFaceName, LF_FACESIZE, (LPWSTR)((DWORD_PTR)otm+(DWORD_PTR)otm->otmpFamilyName));	//ｰﾑﾕ�ﾈｷｵﾄﾗﾖﾌ蠹�ｳﾆｸｴﾖﾆｹ�ﾈ･
	if (lf.lfEscapement != 0) {
		ETO_THROW(ETOE_ROTATEFONT);// rotated font
	}
	hPrevFont = SelectFont(hCanvasDC, hCurFont);
	if (!hPrevFont)
	{
		hMyCurFont = CreateFontIndirect(&lf);
		hPrevFont = SelectFont(hCanvasDC, hMyCurFont);
	}

	if (lf.lfHeight >= 0) {
		// ﾈ郢�ﾃｻﾓﾐﾖｸｶｨｸﾟｶﾈｾﾍﾊｹﾓﾃtmﾖﾐｵﾄｸﾟｶﾈ
		lf.lfHeight = -(tm.tmHeight-tm.tmInternalLeading);	//optimized
	}

	if (bZoomedDC)
	{
		//DCTrans->SetSourceOffset(curPos.x, curPos.y);
		curPos.x = DCTrans->TransformXAB(curPos.x);
		curPos.y = DCTrans->TransformYAB(curPos.y);
		lf.lfHeight = DCTrans->TransformYAB(lf.lfHeight);
		lf.lfWidth = DCTrans->TransformXAB(lf.lfWidth);
		tm.tmHeight = abs(DCTrans->TransformYAB(tm.tmHeight));
		tm.tmInternalLeading = abs(DCTrans->TransformYAB(tm.tmInternalLeading));
		tm.tmAscent = DCTrans->TransformYAB(tm.tmAscent);
		tm.tmDescent = DCTrans->TransformYAB(tm.tmDescent);
		tm.tmAveCharWidth = DCTrans->TransformXAB(tm.tmAveCharWidth);
// 		if (!DCTrans->TransformMode() && !lf.lfWidth && DCTrans->MirrorX()) 
// 			lf.lfWidth = tm.tmAveCharWidth; 
		if (lpDx && cbString)	//firefoxﾊｹﾓﾃETO_PDYｵﾄyﾗ�ｱ�ﾗｪｻｻﾀｴﾉ�ｳﾉﾗﾝﾏ�ﾎﾄﾗﾖ
		{
			int szDx=fuOptions&ETO_PDY ? cbString*2:cbString;
			outlpDx = new int[szDx];
			DCTrans->TransformlpDx(lpDx, outlpDx, szDx);	//lpDx has a size of cbString -1
		}
	}
	FREETYPE_PARAMS params(fuOptions & ~ETO_OPAQUE, hdc, &lf, otm);
	if (bZoomedDC)
		params.charExtra = DCTrans->TransformXAB(params.charExtra);
	SetTextCharacterExtra(hCanvasDC, params.charExtra);
	BITMAP bm;
	HBITMAP hbmpSrc = (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP);

	if(hbmpSrc && ORIG_GetObjectW(hbmpSrc, sizeof(BITMAP), &bm) && bm.bmBitsPixel <= 16) {
		//ﾗﾔｶｯﾉ靹ﾃﾎｪｵ･ﾉｫ葷ﾈｾｷｽﾊｽ
		ETO_THROW(ETOE_MONO);	//ｲｻ葷ﾈｾｵ･ﾉｫﾎﾄﾗﾖ
		//params.ftOptions |= FTO_MONO;
	}

	if(!params.IsMono() && pSettings->EnableShadow()) {
		bShadow = true;
	}
	int xs = 0, ys=0;
	if (bShadow) {
		const int* shadow = pSettings->GetShadowParams();
		xs = shadow[0], ys = shadow[1];
		params.alpha = shadow[2] | shadow[3];
	}
	else
		params.alpha = 1;

	int width=0;

	FreeTypeDrawInfo FTInfo(params, bZoomedDC ? hCanvasDC : hdc, (LOGFONT*)&lf, &cache, bZoomedDC ? outlpDx : lpDx, cbString, xs, ys);

	lf.lfQuality = 0;	//magic number means this is a non scaled font;
	if (lf.lfWidth)
		++lf.lfQuality;
	if (bZoomedDC)
	{
		hZoomedFont = CreateFontIndirect(&lf);
		if (!DCTrans->TransformMode() || lf.lfWidth) 
			++lf.lfQuality;	//scaled font
		hOldMDCFont = SelectFont(hCanvasDC, hZoomedFont);
		SetGraphicsMode(hCanvasDC, GM_ADVANCED);
	}

	if (!FreeTypeGetGlyph(FTInfo, lpString, cbString, width, GlyphArray, ftDrawState))
	{
		ETO_THROW(ETOE_FREETYPE);
	}
	if (FTInfo.xBase<0)	//ﾈ郢�ﾓﾐｸｨﾒ�ｷ�ｺﾅ｣ｬｵ�ﾕ�ﾎﾄﾗﾖﾎｻﾖﾃ
	{
		width-=FTInfo.xBase;	//ﾔ�ｼﾓｻ賚ｼｿ�ｶﾈ
		FTInfo.x -= FTInfo.xBase;	//ﾔ�ｼﾓｹ箜�ｿ�ｶﾈ
		curPos.x+=FTInfo.xBase;	//ﾒﾆｶｯﾆ�ﾊｼｹ箜�
		for (int i=0;i<cbString;++i)
			FTInfo.Dx[i]-=FTInfo.xBase;	//ﾐﾞｸﾄﾎﾄﾗﾖｻ賚ｼｻ�ﾗｼﾎｻﾖﾃ
	}

	/*if (bZoomedDC && DCTrans->MirrorX())	//ﾗ�ﾓﾒｷｴﾏ�｣ｬRGBｺﾍBGRﾒｪﾏ犢ｴ
		for (int i=0; i<cbString; ++i)
		{
			switch (FTInfo.AAModes[i])
			{
			case 2:
				FTInfo.AAModes[i] = 3;
				break;
			case 3:
				FTInfo.AAModes[i] = 2;
				break;
			case 4:
				FTInfo.AAModes[i] = 5;
				break;
			case 5:
				FTInfo.AAModes[i] = 4;
				break;
			}
		}*/
	//POINT destSize;	//LPﾏﾂｵﾄｴ�ﾐ｡ｺﾍﾆ�ﾊｼﾎｻﾖﾃ
	/*
	if (bZoomedDC)
		{
			if (hOldMDCFont)
			{
				SelectFont(hCanvasDC, hOldMDCFont);
				DeleteFont(hZoomedFont);
				hZoomedFont = NULL;
				hOldMDCFont = NULL;
			}
		}*/
	
	textSize.cx = width;
	textSize.cy = FTInfo.y + tm.tmHeight;
	realSize.cx = FTInfo.x;
	realSize.cy = FTInfo.y + tm.tmHeight;

	//******************

	{
		RECT rc = { 0 };
		const UINT horiz = align & (TA_LEFT|TA_RIGHT|TA_CENTER);
		const UINT vert  = align & (TA_BASELINE|TA_TOP|TA_BOTTOM);

		switch (horiz) {
		case TA_CENTER:
			rc.left  = curPos.x - div_ceil(textSize.cx, 2);
			rc.right = curPos.x + div_ceil(textSize.cx, 2);
			//no move
			break;
		case TA_RIGHT:
			rc.left  = curPos.x - textSize.cx;
			rc.right = curPos.x;
			curPos.x -= realSize.cx;//move pos
			break;
		default:
			rc.left  = curPos.x;
			rc.right = curPos.x + textSize.cx;
			curPos.x += realSize.cx;//move pos
			break;
		}

		switch (vert) {
		case TA_BASELINE:
			rc.top = curPos.y - tm.tmAscent;
			rc.bottom = curPos.y + tm.tmDescent + FTInfo.y;
			//trace(L"ascent=%d descent=%d\n", metric.tmAscent, metric.tmDescent);
			break;
		case TA_BOTTOM:
			rc.top = curPos.y - textSize.cy;
			rc.bottom = curPos.y;
			break;
		default:
			rc.top = curPos.y;
			rc.bottom = curPos.y + textSize.cy;
			break;
		}
		//rc.bottom++;
		destPos.x = rc.left;
		destPos.y = rc.top;
		drawSize.cx = textSize.cx;
		drawSize.cy = rc.bottom - rc.top;
	}

	//trace(L"MovedCursor=%d %d\n", curPos.x, curPos.y);
	//trace(L"TargetRect=%d %d %d %d\n", rc.left, rc.top, rc.right, rc.bottom);
	//trace(L"DestPos=%dx%d Size=%dx%d\n", destPos.x, destPos.y, destSize.cx, destSize.cy);
	//trace(L"CanvasPos=%dx%d Size=%dx%d\n", canvasPos.x, canvasPos.y, canvasSize.cx, canvasSize.cy);

	if(drawSize.cx < 1 || drawSize.cy < 1) {
		ETO_THROW(ETOE_NOAREA); //throw no area
	}
	//drawSize.cx += tm.tmMaxCharWidth;	//ｼﾓﾉﾏﾒｻｸ�ﾗ�ｴ�ﾗﾖﾌ蠢�ｶﾈ

	//bitmap

	if (!bmp.CreateDIBandSelect(drawSize.cx+4, drawSize.cy+4)) {
		ETO_THROW(ETOE_CREATEDIB);
	}

	int xorg=0, yorg=0;
	if	(lprc && (fuOptions & ETO_CLIPPED)) {
		const RECT rcBlt = { destPos.x, destPos.y, destPos.x + drawSize.cx, destPos.y + drawSize.cy };
		RECT rcClip = { 0 };
		if (bZoomedDC)
		{
			RECT rcTrans;
			DCTrans->TransformRectAB(lprc, &rcTrans);
			IntersectRect(&rcClip, &rcBlt, &rcTrans);
		}
		else
			IntersectRect(&rcClip, &rcBlt, lprc);
		xorg = rcClip.left-destPos.x; //ｼﾆﾋ翦ｫﾒﾆ
		yorg = rcClip.top-destPos.y;
		destPos.x = rcClip.left;
		destPos.y = rcClip.top;
		drawSize.cx = rcClip.right-rcClip.left;
		drawSize.cy = rcClip.bottom-rcClip.top;
	}

	{
		const BOOL fillrect = (lprc && (fuOptions & ETO_OPAQUE));
		//clear bitmap

		if(fillrect || GetBkMode(hdc) == OPAQUE) {
			COLORREF  bgcolor = GetBkColor(hdc); //両方とも同じ背景色に
			//if ((bgcolor>>24)%2 || (bgcolor>>28)%2)
			//	bgcolor = 0;
			if ((bgcolor>>24)%2 || (bgcolor>>28)%2)
				bgcolor = GetPaletteColor(hdc, bgcolor);
			SetBkMode(hCanvasDC, OPAQUE);
			SetBkColor(hCanvasDC, bgcolor);

			RECT rc = { xorg, yorg, drawSize.cx+ xorg, drawSize.cy+ yorg };
			if (bZoomedDC)
			{
				rc.right+=2;
				rc.bottom+=2;
			}
			cache.FillSolidRect(bgcolor, &rc);

			if(fillrect) {
				//FillRect(hdc, lprc, (HBRUSH)GetCurrentObject(hdc, OBJ_BRUSH));
				ORIG_ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, lprc, NULL, 0, NULL);
			}
		}
		else
		{
			if (!bZoomedDC)
			{
				if (!(BitBlt(hCanvasDC, xorg, yorg, drawSize.cx, drawSize.cy, hdc, destPos.x, destPos.y, SRCCOPY)))
				{
					ETO_THROW(ETOE_FREETYPE);
				}
			}
			else
			{
				SetWorldTransform(hCanvasDC, DCTrans->GetTransform());
				if (!(BitBlt(hCanvasDC, DCTrans->TransformXCoordinateBA(xorg), DCTrans->TransformYCoordinateBA(yorg), 
					DCTrans->TransformXCoordinateBA(drawSize.cx+4), DCTrans->TransformYCoordinateBA(drawSize.cy+4), hdc, 
					DCTrans->TransformXCoordinateBA(destPos.x), DCTrans->TransformYCoordinateBA(destPos.y), SRCCOPY)))
				{
					SetWorldTransform(hCanvasDC, &stdXfm);
					ETO_THROW(ETOE_FREETYPE);
				}
				SetWorldTransform(hCanvasDC, &stdXfm);
			}
		}
	}

	//setup
	SetTextAlign(hCanvasDC, TA_LEFT | TA_TOP);
	//debug
	//Dbg_TraceExtTextOutW(nXStart, nYStart, fuOptions, lpString, cbString, lpDx);

	//textout
	SetTextColor(hCanvasDC, FTInfo.params->color);
	SetBkMode(hCanvasDC, TRANSPARENT);
	FTInfo.hdc = hCanvasDC;

	if (!FreeTypeTextOut(hCanvasDC, cache, lpString, cbString, FTInfo, GlyphArray, ftDrawState)) {
		ETO_THROW(ETOE_FREETYPE);
	}

	//blt + clipping
	/*
	if(lprc && (fuOptions & ETO_CLIPPED)) {
			//TRACE(_T("ClipRect={%d %d %d %d}, pos = (%d,%d)\n"), lprc->left, lprc->top, lprc->right, lprc->bottom,
			//	nXStart, nYStart);
			//trace(L"ClipRect=%d %d %d %d\n", lprc->left, lprc->top, lprc->right, lprc->bottom);
			const RECT rcBlt = { destPos.x, destPos.y, destPos.x + drawSize.cx, destPos.y + drawSize.cy };
			RECT rcClip = { 0 };
			IntersectRect(&rcClip, &rcBlt, lprc);
			BitBlt(hdc, rcClip.left, rcClip.top, rcClip.right - rcClip.left, rcClip.bottom - rcClip.top,
					hCanvasDC, rcClip.left - rcBlt.left, rcClip.top - rcBlt.top, SRCCOPY);
		} else {*/
	if (!bZoomedDC)
		BitBlt(hdc, destPos.x, destPos.y, drawSize.cx, drawSize.cy, hCanvasDC, xorg, yorg, SRCCOPY);
	else
	{
		SetWorldTransform(hCanvasDC, DCTrans->GetTransform());
		BitBlt(hdc, DCTrans->TransformXCoordinateBA(destPos.x), DCTrans->TransformYCoordinateBA(destPos.y), 
			DCTrans->TransformXCoordinateBA(drawSize.cx), DCTrans->TransformYCoordinateBA(drawSize.cy), hCanvasDC, 
			DCTrans->TransformXCoordinateBA(xorg), DCTrans->TransformYCoordinateBA(yorg), SRCCOPY);
		SetWorldTransform(hCanvasDC, &stdXfm);
	}

	//}
	//GdiFlush();
	if(align & TA_UPDATECP) {
		if (!bZoomedDC)
			MoveToEx(hdc, curPos.x, curPos.y, NULL);
		else
			MoveToEx(hdc, DCTrans->TransformXCoordinateBA(curPos.x), DCTrans->TransformYCoordinateBA(curPos.y), NULL);
	}

ETO_CATCH();
	if (otm)
		free(otm);
	bmp.RestoreBitmap();
	if (hOldMDCFont)
	{
		SelectFont(hCanvasDC, hOldMDCFont);
		DeleteFont(hZoomedFont);
		hZoomedFont = NULL;
		hOldMDCFont = NULL;
	}
	if(hPrevFont) {
		SelectFont(hCanvasDC, hPrevFont);
	}
	if (hMyCurFont)
		DeleteFont(hMyCurFont);
	{
		//CCriticalSectionLock __lock;
		for (UINT i=0;i<cbString;i++)
		{
			if (GlyphArray[i])
			{
				FT_Done_Ref_Glyph(&GlyphArray[i]);
			}
		}
	}
	delete[] GlyphArray;
	delete[] ftDrawState;
	if (DCTrans)
		delete DCTrans;
	if (outlpDx)
		delete[] outlpDx;
	if(error == ETOE_OK) {
		pTLInfo->InExtTextOut(false);
		return TRUE;
	}
	int ret = ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
	pTLInfo->InExtTextOut(false);
	return ret;
}

BOOL WINAPI IMPL_MySetProcessMitigationPolicy(
	_In_ PROCESS_MITIGATION_POLICY MitigationPolicy,
	_In_ PVOID                     lpBuffer,
	_In_ SIZE_T                    dwLength
	)
{
	if (MitigationPolicy == ProcessDynamicCodePolicy) {
		PPROCESS_MITIGATION_DYNAMIC_CODE_POLICY(lpBuffer)->ProhibitDynamicCode = false;
	}
	return ORIG_MySetProcessMitigationPolicy(MitigationPolicy, lpBuffer, dwLength);
}

/*
HRESULT WINAPI IMPL_ScriptItemize(
  const WCHAR* pwcInChars, 
  int cInChars, 
  int cMaxItems, 
  const SCRIPT_CONTROL* psControl, 
  const SCRIPT_STATE* psState, 
  SCRIPT_ITEM* pItems, 
  int* pcItems 
) {
	//CThreadCounter __counter;
	HRESULT hr = ORIG_ScriptItemize(pwcInChars, cInChars, cMaxItems, psControl, psState, pItems, pcItems);
	if (FAILED(hr))
		return hr;
	Dbg_TraceScriptItemize(pwcInChars, cInChars);
	//異体字セレクタで始まる実行単位(ラン)を探す
	//最終実行単位の長さを示すための項目は*pcItemsの値に含まれないことに注意
	for (int i = 1; i < *pcItems; ++i) {
		//条件を満たさない実行単位はスキップ
		const int iCharPos = pItems[i].iCharPos;
		if (pItems[i + 1].iCharPos - iCharPos < 2)
			continue; //異体字セレクタの表現には少なくともUnicodeコードポイント2つが必要
		if (pwcInChars[iCharPos] != 0xDB40)
			continue; //上位サロゲートの値が範囲外
		if (pwcInChars[iCharPos + 1] - 0xDD00 >= 0xF0)
			continue; //下位サロゲートの値が範囲外
		//異体字セレクタを1つ前の実行単位に移動する
		pItems[i].iCharPos += 2;
		//長さが0になった場合は実行単位そのものを削除する
		if (pItems[i + 1].iCharPos <= pItems[i].iCharPos) {
			memmove(&pItems[i], &pItems[i + 1], (*pcItems - i) * sizeof SCRIPT_ITEM);
			--*pcItems;
			--i; //削除した分のつじつま合わせ
		}
	}
	return hr;
}*/

//HFONT dummy=NULL;
/*
int WINAPI IMPL_GdipCreateFontFamilyFromName(const WCHAR *name, void *fontCollection, void **FontFamily)
{
	LOGFONT lf={};
	memset(&lf, 0, sizeof lf);
	StringCchCopy(lf.lfFaceName, LF_FACESIZE, name);
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	if (pSettings->CopyForceFont(lf, lf))
	{
		//dummy = CreateFont(1,0,0,0,0,0,0,0,DEFAULT_CHARSET,0,0,0,0,lf.lfFaceName);
		return ORIG_GdipCreateFontFamilyFromName(lf.lfFaceName, fontCollection, FontFamily);
	}
	return ORIG_GdipCreateFontFamilyFromName(name, fontCollection, FontFamily);
}*/


/*
HRESULT WINAPI IMPL_ScriptShape(
  HDC hdc, 
  SCRIPT_CACHE* psc, 
  const WCHAR* pwcChars, 
  int cChars, 
  int cMaxGlyphs, 
  SCRIPT_ANALYSIS* psa, 
  WORD* pwOutGlyphs, 
  WORD* pwLogClust, 
  SCRIPT_VISATTR* psva, 
  int* pcGlyphs 
) {
	//CThreadCounter __counter;
	Dbg_TraceScriptShape(pwcChars, cChars, psa, NULL, 0);
	//実行単位の末尾が異体字シーケンスでなければ何もしない
	WORD vsindex;
	if (cChars < 3 || cMaxGlyphs < 1 || pwcChars[cChars - 2] != 0xDB40 || (vsindex = pwcChars[cChars - 1] - 0xDD00) >= 0xF0)
		return ORIG_ScriptShape(hdc, psc, pwcChars, cChars, cMaxGlyphs, psa, pwOutGlyphs, pwLogClust, psva, pcGlyphs);

	if (!hdc)
		return E_PENDING; //判定にはHDCが必要

	//異体字セレクタを削ってシェーピングエンジンに渡す
	HRESULT hr = ORIG_ScriptShape(hdc, psc, pwcChars, cChars - 2, cMaxGlyphs - 1, psa, pwOutGlyphs, pwLogClust, psva, pcGlyphs);
	if (FAILED(hr) || psa->fNoGlyphIndex)
		return hr;
	Dbg_TraceScriptShape(pwcChars, cChars, psa, pwOutGlyphs, *pcGlyphs);

	//最終グリフを置き換える
	WORD high;
	WORD low = pwcChars[cChars - 3] - 0xDC00;
	int baseChar;
	if (cChars >= 4 && (high = pwcChars[cChars - 4] - 0xD800) < 0x400 && low < 0x400)
		baseChar = ((high << 10) | low) + 0x10000;
	else
		baseChar = pwcChars[cChars - 3];
	if (*pcGlyphs > 0) {
		FreeTypeSubstGlyph(hdc, vsindex, baseChar, cChars, psa, pwOutGlyphs, pwLogClust, psva, pcGlyphs);
	}
	Dbg_TraceScriptShape(pwcChars, cChars, psa, pwOutGlyphs, *pcGlyphs);

	return hr;
}*/

/*

HRESULT WINAPI IMPL_ScriptTextOut(
  const HDC hdc, 
  SCRIPT_CACHE* psc, 
  int x, 
  int y, 
  UINT fuOptions, 
  const RECT* lprc, 
  const SCRIPT_ANALYSIS* psa, 
  const WCHAR* pwcReserved, 
  int iReserved, 
  const WORD* pwGlyphs, 
  int cGlyphs, 
  const int* piAdvance, 
  const int* piJustify, 
  const GOFFSET* pGoffset 
) {
	//CThreadCounter __counter;
	CThreadLocalInfo* pTLInfo = g_TLInfo.GetPtr();
	if (pTLInfo)
		pTLInfo->InUniTextOut(true);

	HRESULT hr = ORIG_ScriptTextOut(hdc, psc, x, y, fuOptions, lprc, psa, pwcReserved, iReserved,
		pwGlyphs, cGlyphs, piAdvance, piJustify, pGoffset);

	if (pTLInfo)
		pTLInfo->InUniTextOut(false);

	return hr;
}

HRESULT WINAPI IMPL_ScriptStringOut(
	__in  SCRIPT_STRING_ANALYSIS ssa,
	__in  int iX,
	__in  int iY,
	__in  UINT uOptions,
	__in  const RECT *prc,
	__in  int iMinSel,
	__in  int iMaxSel,
	__in  BOOL fDisabled

) {
  //CThreadCounter __counter;
  CThreadLocalInfo* pTLInfo = g_TLInfo.GetPtr();
  if (pTLInfo)
	  pTLInfo->InUniscribe(true);

  HRESULT hr = ORIG_ScriptStringOut(ssa, iX, iY, uOptions, prc, iMinSel, iMaxSel, fDisabled);

  if (pTLInfo)
	  pTLInfo->InUniscribe(false);

  return hr;
}
*/

/*
int WINAPI IMPL_GetTextFace(HDC hdc, int c, LPWSTR lpName)
{
	//CThreadCounter __counter;
	BOOL bResult = ORIG_GetTextFace(hdc, c, lpName);
	wstring fontcache=GetCachedFont(GetCurrentFont(hdc));
	if (fontcache.size()){
		StringCchCopy(lpName, c, fontcache.c_str());
	}
	return bResult;
}*/




//EOF
