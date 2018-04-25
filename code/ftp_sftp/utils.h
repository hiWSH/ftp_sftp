#include "stdafx.h"
#include "shlwapi.h"

#pragma comment(lib,"shlwapi.lib")


const wchar_t* wcstrrchr(const wchar_t* str, const wchar_t wc)  
{  
	const wchar_t* pwc = NULL;  
	for (int i=wcslen(str)-1;i>=0;i--)  
	{  
		if (str[i] == wc)  
		{  
			pwc = str + i;  
			break;  
		}  
	}  
	return pwc;  
}  

bool createMultiDir(const wchar_t* path)  
{  
	if (path == NULL) return false;  
	const wchar_t* pwcStrrchr = wcstrrchr(path,L'\\');  
	if (!pwcStrrchr) return false;  
	if (PathIsDirectory(path)) return true;  

	wchar_t wsSubPath[MAX_PATH] = {};  
	memset(wsSubPath,0,sizeof(wsSubPath));  
	for (int i=0; i<pwcStrrchr-path; i++)  
		wsSubPath[i] = *(path+i);  
	createMultiDir(wsSubPath);  
	if(CreateDirectory(path,NULL)) return true;  
	return false;  
} 

bool createFtpMultiDir(HINTERNET hconn,const wchar_t* path)  
{  
	if (path == NULL) return false;  
	const wchar_t* pwcStrrchr = wcstrrchr(path,L'\\');  
	if (!pwcStrrchr) return false;  

	wchar_t wsSubPath[MAX_PATH] = {};  
	memset(wsSubPath,0,sizeof(wsSubPath));  
	for (int i=0; i<pwcStrrchr-path; i++)  
		wsSubPath[i] = *(path+i);  
	createMultiDir(wsSubPath);  
	if(FtpCreateDirectory(hconn,path)) return true;  
	return false;  
} 