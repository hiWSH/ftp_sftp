// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once
#define _CSTDIO_
#define _CSTRING_
#define _CWCHAR_

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的


#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 从 Windows 头中排除极少使用的资料
#endif

#include <afx.h>
#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC 对 Internet Explorer 4 公共控件的支持
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC 对 Windows 公共控件的支持
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <iostream>

#include <wininet.h>
#pragma comment(lib,"wininet.lib")

#include <string>

#include <strsafe.h>
// TODO: 在此处引用程序需要的其他头文件
#include <stdexcept>

#define  GWI_PLUGIN_ERROR   "0100000004"
#define  GWI_PLUGIN_SUCCESS  "0000000000"
#define  GWI_PLUGIN_INVALID_FILE "0000000001"
#define  LEN_1024  1024

#define  RETURN_SUCCESS(ret,errno,data){\
	sprintf_s((ret),LEN_1024,"{\"success\": true,\"errno\":\"%s\",\"description\":\"operation success\",\"data\":\"%s\"}",(errno),(data));\
	return (ret);\
}

#define  RETURN_ERROR(ret,errno,descript,data){\
	sprintf_s((ret),LEN_1024,"{\"success\": false,\"errno\":\"%s\",\"description\":\"%s\",\"data\":\"%s\"}",(errno),(descript),(data));\
	return (ret);\
}

#define  ASSERT_PTR(jason,ret) \
	strcpy_s((ret),11,GWI_PLUGIN_ERROR);\
	if((jason) == nullptr)\
{\
	RETURN_ERROR((ret),GWI_PLUGIN_ERROR,"入参为空指针","");\
}

//实现释放内存
#define DELETE_ARRAY(ptr) \
	if(nullptr != ptr)\
	{\
	delete[] ptr;\
	ptr = nullptr;\
	}

//json解析 root根节点 item对应节点 var变量名 ret返回变量
#define GET_JASON_OBJECT(root,item,var,ret) \
	try{\
	(item) = cJSON_GetObjectItem((root), (var));\
	}catch(std::runtime_error e){\
	cJSON_Delete((root));\
	RETURN_ERROR((ret),GWI_PLUGIN_ERROR,"获取json节点值失败","");\
	}
