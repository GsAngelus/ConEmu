
/*
Copyright (c) 2009-2010 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#pragma once

#include <windows.h>
#include <wchar.h>
#include "common.hpp"


// WinAPI wrappers
BOOL apiSetForegroundWindow(HWND ahWnd);
BOOL apiShowWindow(HWND ahWnd, int anCmdShow);
BOOL apiShowWindowAsync(HWND ahWnd, int anCmdShow);
#ifdef _DEBUG
void getWindowInfo(HWND ahWnd, wchar_t* rsInfo/*[1024]*/);
#endif

// Some WinAPI related functions
wchar_t* GetShortFileNameEx(LPCWSTR asLong);
BOOL FileExists(LPCWSTR asFilePath);
BOOL IsFilePath(LPCWSTR asFilePath);
BOOL IsUserAdmin();
BOOL IsWindows64(BOOL *pbIsWow64Process = NULL);


//------------------------------------------------------------------------
///| Section |////////////////////////////////////////////////////////////
//------------------------------------------------------------------------
class MSectionLock;

class MSection
{
protected:
	CRITICAL_SECTION m_cs;
	DWORD mn_TID; // ��������������� ������ ����� EnterCriticalSection
#ifdef _DEBUG
	DWORD mn_UnlockedExclusiveTID;
#endif
	int mn_Locked;
	BOOL mb_Exclusive;
	HANDLE mh_ReleaseEvent;
	friend class MSectionLock;
	DWORD mn_LockedTID[10];
	int   mn_LockedCount[10];
public:
	MSection();
	~MSection();
public:
	void ThreadTerminated(DWORD dwTID);
protected:
	void AddRef(DWORD dwTID);
	int ReleaseRef(DWORD dwTID);
	void WaitUnlocked(DWORD dwTID, DWORD anTimeout);
	bool MyEnterCriticalSection(DWORD anTimeout);
	BOOL Lock(BOOL abExclusive, DWORD anTimeout=-1/*, BOOL abRelockExclusive=FALSE*/);
	void Unlock(BOOL abExclusive);
};

class MSectionThread
{
protected:
	MSection* mp_S;
	DWORD mn_TID;
public:
	MSectionThread(MSection* apS);
	~MSectionThread();
};

class MSectionLock
{
protected:
	MSection* mp_S;
	BOOL mb_Locked, mb_Exclusive;
public:
	BOOL Lock(MSection* apS, BOOL abExclusive=FALSE, DWORD anTimeout=-1);
	BOOL RelockExclusive(DWORD anTimeout=-1);
	void Unlock();
	BOOL isLocked();
public:
	MSectionLock();
	~MSectionLock();
};


/* Console Handles */
class MConHandle
{
private:
	wchar_t   ms_Name[10];
	HANDLE    mh_Handle;
	MSection  mcs_Handle;
	BOOL      mb_OpenFailed;
	DWORD     mn_LastError;
	DWORD     mn_StdMode;

public:
	operator const HANDLE();

public:
	void Close();

public:
	MConHandle(LPCWSTR asName);
	~MConHandle();
};


template <class T>
class MFileMapping
{
protected:
	HANDLE mh_Mapping;
	BOOL mb_WriteAllowed;
	int mn_Size;
	T* mp_Data; //WARNING!!! ������ ����� ���� ������ �� ������!
	wchar_t ms_MapName[MAX_PATH];
	DWORD mn_LastError;
	wchar_t ms_Error[MAX_PATH*2];
public:
	T* Ptr()
	{
		return mp_Data;
	};
	operator T*()
	{
		return mp_Data;
	};
	bool IsValid()
	{
		return (mp_Data!=NULL);
	};
	LPCWSTR GetErrorText()
	{
		return ms_Error;
	};
	bool SetFrom(const T* pSrc, int nSize=-1)
	{
		if (!IsValid() || !nSize) return false;
		if (nSize<0) nSize = sizeof(T);
		bool lbChanged = (memcmp(mp_Data, pSrc, nSize)!=0);
		memmove(mp_Data, pSrc, nSize);
		return lbChanged;
	}
	bool GetTo(T* pDst, int nSize=-1)
	{
		if (!IsValid() || !nSize) return false;
		if (nSize<0) nSize = sizeof(T);
		//������, ��� T ����� ���� ������ ��� const - (void*)
		memmove((void*)pDst, mp_Data, nSize);
		return true;
	}
public:
	void InitName(const wchar_t *aszTemplate,DWORD Parm1=0,DWORD Parm2=0)
	{
		if (mh_Mapping) CloseMap();
		wsprintfW(ms_MapName, aszTemplate, Parm1, Parm2);
	};
	void ClosePtr()
	{
		if (mp_Data)
		{
			UnmapViewOfFile(mp_Data);
			mp_Data = NULL;
		}
	};
	void CloseMap()
	{
		if (mp_Data) ClosePtr();
		if (mh_Mapping)
		{
			CloseHandle(mh_Mapping);
			mh_Mapping = NULL;
		}
		mh_Mapping = NULL; mb_WriteAllowed = FALSE; mp_Data = NULL; 
		mn_Size = -1; mn_LastError = 0;
	};
protected:
	// mn_Size � nSize ������������ ���������� ������ ��� CreateFileMapping ��� ��������� �����������
	T* InternalOpenCreate(BOOL abCreate,BOOL abReadWrite,int nSize)
	{
		if (mh_Mapping) CloseMap();
		mn_LastError = 0; ms_Error[0] = 0;
		_ASSERTE(mh_Mapping==NULL && mp_Data==NULL);
		_ASSERTE(nSize==-1 || nSize>=sizeof(T));

		if (ms_MapName[0] == 0)
		{
			_ASSERTE(ms_MapName[0]!=0);
			lstrcpyW (ms_Error, L"Internal error. Mapping file name was not specified.");
			return NULL;
		}
		else
		{
			mn_Size = (nSize<=0) ? sizeof(T) : nSize;
			mb_WriteAllowed = abCreate || abReadWrite;
			if (abCreate)
			{
				mh_Mapping = CreateFileMapping(INVALID_HANDLE_VALUE, 
					NullSecurity(), PAGE_READWRITE, 0, mn_Size, ms_MapName);
			}
			else
			{
				mh_Mapping = OpenFileMapping(FILE_MAP_READ, FALSE, ms_MapName);
			}
			if (!mh_Mapping)
			{
				mn_LastError = GetLastError();
				wsprintfW (ms_Error, L"Can't %s console data file mapping. ErrCode=0x%08X\n%s", 
						abCreate ? L"create" : L"open", mn_LastError, ms_MapName);
			}
			else
			{
				DWORD nFlags = mb_WriteAllowed ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
				mp_Data = (T*)MapViewOfFile(mh_Mapping, nFlags,0,0,0);
				if (!mp_Data)
				{
					mn_LastError = GetLastError();
					wsprintfW (ms_Error, L"Can't map console info (%s). ErrCode=0x%08X\n%s", 
							mb_WriteAllowed ? L"ReadWrite" : L"Read" ,mn_LastError, ms_MapName);
				}
			}
		}
		return mp_Data;
	};
public:
	T* Create(int nSize=-1)
	{
		_ASSERTE(nSize==-1 || nSize>=sizeof(T));
		return InternalOpenCreate(TRUE/*abCreate*/,TRUE/*abReadWrite*/,nSize);
	};
	T* Open(BOOL abReadWrite=FALSE/*FALSE - ������ Read*/,int nSize=-1)
	{
		_ASSERTE(nSize==-1 || nSize>=sizeof(T));
		return InternalOpenCreate(FALSE/*abCreate*/,abReadWrite,nSize);
	};
public:
	MFileMapping()
	{
		mh_Mapping = NULL; mb_WriteAllowed = FALSE; mp_Data = NULL; 
		mn_Size = -1; ms_MapName[0] = ms_Error[0] = 0; mn_LastError = 0;
	};
	~MFileMapping()
	{
		if (mh_Mapping) CloseMap();
	};
};

class MEvent
{
protected:
	WCHAR  ms_EventName[MAX_PATH];
	HANDLE mh_Event;
	DWORD  mn_LastError;
public:
	MEvent();
	~MEvent();
	void InitName(const wchar_t *aszTemplate, DWORD Parm1=0);
public:
	void   Close();
	HANDLE Open();
	DWORD  Wait(DWORD dwMilliseconds, BOOL abAutoOpen=TRUE);
	HANDLE GetHandle();
};

template <class T_IN, class T_OUT>
class MPipe
{
protected:
	WCHAR ms_PipeName[MAX_PATH], ms_Module[32], ms_Error[MAX_PATH*2];
	HANDLE mh_Pipe, mh_Heap;
	T_IN m_In; // ��� �������...
	T_OUT* mp_Out; DWORD mn_OutSize, mn_MaxOutSize;
	T_OUT m_Tmp;
	//DWORD mdw_Timeout;
public:
	MPipe()
	{
		ms_PipeName[0] = ms_Module[0] = 0;
		mh_Pipe = NULL; memset(&m_In, 0, sizeof(m_In));
		mp_Out = NULL; mn_OutSize = mn_MaxOutSize = 0;
		mh_Heap = GetProcessHeap();
		_ASSERTE(mh_Heap!=NULL);
	};
	void SetTimeout(DWORD anTimeout)
	{
		//TODO: ���� anTimeout!=-1 - ��������� ���� � ��������� ������� � ���. ������� ���� �� ����� � ������� ��, ���� ������ Timeout
	}
	~MPipe()
	{
		if (mp_Out && mp_Out != &m_Tmp)
		{
			_ASSERTE(mh_Heap!=NULL);
			HeapFree(mh_Heap, 0, mp_Out);
			mp_Out = NULL;
		}
	};
	void Close()
	{
		if (mh_Pipe && mh_Pipe != INVALID_HANDLE_VALUE)
			CloseHandle(mh_Pipe);
		mh_Pipe = NULL;
	};
	void InitName(const wchar_t* asModule, const wchar_t *aszTemplate, const wchar_t *Parm1, DWORD Parm2)
	{
		//va_list args;
		//va_start( args, aszTemplate );
		//vswprintf_s(ms_PipeName, countof(ms_PipeName), aszTemplate, args);
		wsprintfW(ms_PipeName, aszTemplate, Parm1, Parm2);
		lstrcpynW(ms_Module, asModule, countof(ms_Module));
		if (mh_Pipe)
			Close();
	};
	BOOL Open()
	{
		if (mh_Pipe && mh_Pipe != INVALID_HANDLE_VALUE)
			return TRUE;
		mh_Pipe = ExecuteOpenPipe(ms_PipeName, ms_Error, ms_Module);
		if (mh_Pipe == INVALID_HANDLE_VALUE)
		{
			_ASSERTE(mh_Pipe != INVALID_HANDLE_VALUE);
			mh_Pipe = NULL;
		}
		return (mh_Pipe!=NULL);
	};
	BOOL Transact(const T_IN* apIn, DWORD anInSize, const T_OUT** rpOut, DWORD* rnOutSize)
	{
		ms_Error[0] = 0;
		_ASSERTE(apIn && rnOutSize);

		*rnOutSize = 0;
		*rpOut = &m_Tmp;

		if (!Open()) // ���� ����� ���� ��� ������, ������� ��� ���������
			return FALSE;

		_ASSERTE(sizeof(m_In) >= sizeof(CESERVER_REQ_HDR));
		_ASSERTE(sizeof(T_OUT) >= sizeof(CESERVER_REQ_HDR));

		BOOL fSuccess = FALSE;
		DWORD cbRead, dwErr, cbReadBuf;

		// ��� �������, ���������� � ��������� �������
		cbReadBuf = min(anInSize, sizeof(m_In));
		memmove(&m_In, apIn, cbReadBuf);

		// Send a message to the pipe server and read the response. 
		cbRead = 0; cbReadBuf = sizeof(m_Tmp);
		T_OUT* ptrOut = &m_Tmp;
		if (mp_Out && (mn_MaxOutSize > cbReadBuf))
		{
			ptrOut = mp_Out;
			cbReadBuf = mn_MaxOutSize;
			*rpOut = mp_Out;
		}
		WARNING("����������! ������ �������� ��� �������� �������");
		fSuccess = TransactNamedPipe( mh_Pipe, (LPVOID)apIn, anInSize, ptrOut, cbReadBuf, &cbRead, NULL);
		dwErr = fSuccess ? 0 : GetLastError();

		if (!fSuccess && dwErr == ERROR_BROKEN_PIPE)
		{
			// ������ �� ������ ������, �� ��������� �������
			Close(); // ��� ���� ������ - ��������� �����
			return TRUE;
		}
		
		if (!fSuccess && (dwErr != ERROR_MORE_DATA)) 
		{
			DEBUGSTR(L" - FAILED!\n");
			DWORD nCmd = 0;
			if (anInSize >= sizeof(CESERVER_REQ_HDR))
				nCmd = ((CESERVER_REQ_HDR*)apIn)->nCmd;
			wsprintfW(ms_Error, _T("%s: TransactNamedPipe failed, Cmd=%i, ErrCode = 0x%08X!"), ms_Module, nCmd, dwErr);
			Close(); // ��������� ��������� ����������� ������ - ���� ����� ������� (����� ����� �����������)
			return FALSE;
		}

		// ����� �������� ���������
		if (cbRead < sizeof(CESERVER_REQ_HDR))
		{
			_ASSERTE(cbRead >= sizeof(CESERVER_REQ_HDR));
			wsprintfW(ms_Error,
				_T("%s: Only %i bytes recieved, required %i bytes at least!"), 
				ms_Module, cbRead, (DWORD)sizeof(CESERVER_REQ_HDR));
			return FALSE;
		}
		if (((CESERVER_REQ_HDR*)apIn)->nCmd != ((CESERVER_REQ_HDR*)&m_In)->nCmd)
		{
			_ASSERTE(((CESERVER_REQ_HDR*)apIn)->nCmd == ((CESERVER_REQ_HDR*)&m_In)->nCmd);
			wsprintfW(ms_Error,
				_T("%s: Invalid CmdID=%i recieved, required CmdID=%i!"), 
				ms_Module, ((CESERVER_REQ_HDR*)apIn)->nCmd, ((CESERVER_REQ_HDR*)&m_In)->nCmd);
			return FALSE;
		}
		if (((CESERVER_REQ_HDR*)ptrOut)->nVersion != CESERVER_REQ_VER)
		{
			_ASSERTE(((CESERVER_REQ_HDR*)ptrOut)->nVersion == CESERVER_REQ_VER);
			wsprintfW(ms_Error,
				_T("%s: Old version packet recieved (%i), required (%i)!"), 
				ms_Module, ((CESERVER_REQ_HDR*)ptrOut)->nCmd, CESERVER_REQ_VER);
			return FALSE;
		}
		if (((CESERVER_REQ_HDR*)ptrOut)->cbSize < cbRead)
		{
			_ASSERTE(((CESERVER_REQ_HDR*)ptrOut)->cbSize >= cbRead);
			wsprintfW(ms_Error,
				_T("%s: Invalid packet size (%i), must be greater or equal to %i!"), 
				ms_Module, ((CESERVER_REQ_HDR*)ptrOut)->cbSize, cbRead);
			return FALSE;
		}

		if (dwErr == ERROR_MORE_DATA)
		{
			int nAllSize = ((CESERVER_REQ_HDR*)ptrOut)->cbSize;

			if (!mp_Out || (int)mn_MaxOutSize < nAllSize)
			{
				T_OUT* ptrNew = (T_OUT*)HeapAlloc(mh_Heap, 0, nAllSize);
				if (!ptrNew)
				{
					_ASSERTE(ptrNew!=NULL);
					wsprintfW(ms_Error, _T("%s: Can't allocate %u bytes!"), ms_Module, nAllSize);
					return FALSE;
				}
				memmove(ptrNew, ptrOut, cbRead);

				if (mp_Out) HeapFree(mh_Heap, 0, mp_Out);
				mn_MaxOutSize = nAllSize;
				mp_Out = ptrNew;

			}
			else if (ptrOut == &m_Tmp)
			{
				memmove(mp_Out, &m_Tmp, cbRead);
			}
			*rpOut = mp_Out;

			LPBYTE ptrData = ((LPBYTE)mp_Out)+cbRead;

			nAllSize -= cbRead;

			while(nAllSize>0)
			{ 
				// Read from the pipe if there is more data in the message.
				//WARNING: ���� � ������ ����� ������ ������ ��� nAllSize - ��������!
				fSuccess = ReadFile( mh_Pipe, ptrData, nAllSize, &cbRead, NULL);
				dwErr = fSuccess ? 0 : GetLastError();
				// Exit if an error other than ERROR_MORE_DATA occurs.
				if( !fSuccess && (dwErr != ERROR_MORE_DATA)) 
					break;
				ptrData += cbRead;
				nAllSize -= cbRead;
			}

			TODO("����� ���������� ASSERT, ���� ������� ���� ������� � �������� ������");
			if (nAllSize > 0)
			{
				_ASSERTE(nAllSize==0);
				wsprintfW(ms_Error, _T("%s: Can't read %u bytes!"), ms_Module, nAllSize);
				return FALSE;
			}

			if (dwErr == ERROR_MORE_DATA)
			{
				_ASSERTE(dwErr != ERROR_MORE_DATA);
				//	BYTE cbTemp[512];
				//	while (1) {
				//		fSuccess = ReadFile( mh_Pipe, cbTemp, 512, &cbRead, NULL);
				//		dwErr = GetLastError();
				//		// Exit if an error other than ERROR_MORE_DATA occurs.
				//		if( !fSuccess && (dwErr != ERROR_MORE_DATA)) 
				//			break;
				//	}
			}

			// ���� �� ���?
			fSuccess = FlushFileBuffers(mh_Pipe);
		}

		return TRUE;
	};
	LPCWSTR GetErrorText()
	{
		return ms_Error;
	};
};


class MSetter
{
protected:
	enum tag_MSETTERTYPE
	{
		st_BOOL,
		st_DWORD,
	} type;

	union
	{
		struct
		{
			// st_BOOL
			BOOL *mp_BoolVal;
		};
		struct
		{
			// st_DWORD
			DWORD *mdw_DwordVal; DWORD mdw_OldDwordValue, mdw_NewDwordValue;
		};
	};
public:
	MSetter(BOOL* st);
	MSetter(DWORD* st, DWORD setValue);
	~MSetter();

	void Unlock();
};

class MFileLog
{
protected:
	wchar_t* ms_FilePathName;
	HANDLE   mh_LogFile;
public:
	MFileLog(LPCWSTR asName, LPCWSTR asDir = NULL, DWORD anPID = 0);
	~MFileLog();
	HRESULT CreateLogFile(); // Returns 0 if succeeded, otherwise - GetLastError() code
	LPCWSTR GetLogFileName();
	void LogString(LPCSTR asText, BOOL abWriteTime = TRUE, LPCSTR asThreadName = NULL);
	void LogString(LPCWSTR asText, BOOL abWriteTime = TRUE, LPCWSTR asThreadName = NULL);
};

// ����� ���������� ����������� ��������� ���������.
class MWow64Disable
{
protected:
	typedef BOOL (WINAPI* Wow64DisableWow64FsRedirection_t)(PVOID* OldValue);
	typedef BOOL (WINAPI* Wow64RevertWow64FsRedirection_t)(PVOID OldValue);
	Wow64DisableWow64FsRedirection_t _Wow64DisableWow64FsRedirection;
	Wow64RevertWow64FsRedirection_t _Wow64RevertWow64FsRedirection;
	
	BOOL mb_Disabled;
	PVOID m_OldValue;
public:
	void Disable()
	{
		if (!mb_Disabled && _Wow64DisableWow64FsRedirection)
		{
			mb_Disabled = _Wow64DisableWow64FsRedirection(&m_OldValue);
		}
	};
	void Restore()
	{
		if (mb_Disabled)
		{
			mb_Disabled = FALSE;
			if (_Wow64RevertWow64FsRedirection)
				_Wow64RevertWow64FsRedirection (m_OldValue);
		}
	};
public:
	MWow64Disable()
	{
		mb_Disabled = FALSE; m_OldValue = NULL;
		HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
		if (hKernel)
		{
			_Wow64DisableWow64FsRedirection = (Wow64DisableWow64FsRedirection_t)GetProcAddress(hKernel, "Wow64DisableWow64FsRedirection");
			_Wow64RevertWow64FsRedirection = (Wow64RevertWow64FsRedirection_t)GetProcAddress(hKernel, "Wow64RevertWow64FsRedirection");
		}
		else
		{
			_Wow64DisableWow64FsRedirection = NULL;
			_Wow64RevertWow64FsRedirection = NULL;
		}
	};
	~MWow64Disable()
	{
		Restore();
	};
};

// ������� ��� �������
class CTimer
{
protected:
	HWND mh_Wnd;
	UINT mn_TimerId;
	UINT mn_Elapse;
	bool mb_Started;
public:
	bool IsStarted()
	{
		return mb_Started;
	};
	void Start(UINT anElapse = 0)
	{
		if (!mh_Wnd || mb_Started)
			return;
		mb_Started = true;
		SetTimer(mh_Wnd, mn_TimerId, anElapse ? anElapse : mn_Elapse, FALSE);
	};
	void Stop()
	{
		if (mb_Started)
		{
			mb_Started = false;
			KillTimer(mh_Wnd, mn_TimerId);
		}
	};
	void Restart(UINT anElapse = 0)
	{
		if (mb_Started)
			Stop();
		Start(anElapse);
	};
public:
	CTimer()
	{
		mh_Wnd = NULL;
		mn_TimerId = mn_Elapse = 0;
		mb_Started = false;
	};
	~CTimer()
	{
		Stop();
	};
	void Init(HWND ahWnd, UINT anTimerID, UINT anElapse)
	{
		Stop();
		mh_Wnd = ahWnd; mn_TimerId = anTimerID; mn_Elapse = anElapse;
	};
};
