#ifndef _SFTPCLINET_GWI_
#define _SFTPCLINET_GWI_

#include ".\libssh2\include\libssh2_config.h"
#include ".\libssh2\include\libssh2.h"
#include ".\libssh2\include\libssh2_sftp.h"

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <Shlwapi.h>

#include <imagehlp.h>
#pragma comment( lib, "imagehlp.lib" )
#pragma comment( lib, ".\\libssh2\\lib\\libssh2.lib" )
#pragma comment( lib, ".\\libssh2\\lib\\libeay32.lib" )
#pragma comment( lib, ".\\libssh2\\lib\\zlib.lib" )
#ifndef MAXPATH 
#define MAXPATH 256
#endif



	/**
	* 函数名：SFTPDirExist
	* 说明  ：判断SFTP目录是否存在
	* 参数  ：1.sftp_session
	*           LIBSSH2_SFTP会话指针
	*       ：2.sftppath
	*           SFTP目录
	* 返回值：1指定目录存在；0指定目录不存在
	*/
	int SFTPDirExist(LIBSSH2_SFTP *sftp_session, const char *sftppath);
	/**
	* 函数名：makeSFTPDir
	* 说明  ：创建SFTP目录
	* 参数  ：1.sftp_session
	*           LIBSSH2_SFTP会话指针
	*       ：2.sftppath
	*           SFTP目录
	* 返回值：0成功；-1失败
	*/
	int makeSFTPDir(LIBSSH2_SFTP *sftp_session, const char *sftppath);
namespace GWI_SFTPClient{

	/************************************************************************/
	/* 实现根据错误码得到SFTP的错误信息                                                                     */
	/************************************************************************/
	typedef struct _GWI_SFTPClient_ErrorCode{
		int errorcode;
		char* errormsg;
	}GWI_SFTPClient_ErrorCode,*pGWI_SFTPClient_ErrorCode;

	void getGWI_SFTPClient_ErrorMsg_By_Errorcode(int errorcode,char* errormsg);
	/************************************************************************
	 * 函数名：sftpOpen 
	 * 说明  ：建立SFTP连接
	 * 参数  ：1.ip
	 *           SFTP服务器IP
	 *       ：2.port
	 *           SFTP服务器端口
	 *       ：3.username
	 *           SFTP用户名
	 *       ：4.password
	 *           SFTP用户密码
	 * 返回   ： 0 成功；1 其它错误
	/************************************************************************/
	int sftpOpen(const char *ip,int port,const char *username, const char *password);

	/************************************************************************
	 * 函数名 ： sftpClose
	 * 说明   ： 释放资源
	 * 参数   ： 无
	 * 返回   ： 0 成功；1 其它错误
	/************************************************************************/
	int sftpClose();

	/**
	* 函数名：uploadFile
	* 说明  ：上传文件
	* 参数 
	*       ：1.sftppath
	*           SFTP路径
	*       ：2.slocalfile
	*           要上传的本地文件全路径，包括文件名
	*       ：3.sftpfilename
	*           上传到SFTP服务器保存的文件名，可为空；如果为空，
	*           服务器端保存的文件名将与slocalfile中的文件名保持一致
	* 返回值：0成功；其它失败（）
	*/
	int uploadFile(const char *sftppath,
				   const char *slocalfile,
				   const char *sftpfilename = NULL);
	/**
	* 函数名：downloadFile
	* 说明  ：下载文件
	* 参数 
	*       ：1.sftpfile
	*           要下载的SFTP文件全路径，包括文件名
	*       ：2.slocalfile
	*           保存到本地的文件全路径，带文件名
	* 返回值：0成功；其它失败（）
	*/
	int downloadFile(
				   const char *sftpfile,
				   const char *slocalfile);

	/**
	* 函数名：getFileList
	* 说明  ：获取文件列表
	* 参数  
	*       ：1.sftpdir
	*           要获取文件列表的的SFTP路径
	*       ：2.ext
	*           文件名扩展名，可为"",表示获取所有文件
	*       ：3.retlist
	*           以格式如"['filename1','filename2',...]"返回文件列表
	* 返回值：0成功；其它失败（）
	*/
	int getFileList(
				   const char *sftpdir,
				   const char *ext,
				   /*char * retlist*/
				   CString &retlist);

	/************************************************************************/
	/* 函数名：sftpupdateFileDir
	/* 说明：实现本地文件夹上传到FTP，包括空文件    
	/* 参数：输入 
	const char *sftpdir ： 上传到SFTP的文件夹名
	const char *slocaldir：本地文件夹名
	         输出
	无
	/* 返回：0 成功  其它失败
	/************************************************************************/
	int sftpUpdateFileDir(
		const char *sftpdir,
		const char *slocaldir);

	/************************************************************************/
	/* 函数名：downloadFileDir
	/* 说明：实现SFTP文件夹下载到本地，包括空文件    
	/* 参数：输入 
	const char *sftpdir ： 上传到SFTP的文件夹名
	const char *slocaldir：本地文件夹名
	         输出
	无
	/* 返回：0 成功  其它失败
	/************************************************************************/
	int sftpDwnloadFileDir(
		const char *sftpdir,
		const char *slocaldir);
	/************************************************************************/
	/* 函数： sftpCd
	/* 说明： 创建sftp文件夹
	/* 参数： 输入
	const char* path：文件夹路径
	/************************************************************************/
	int sftpCd(const char* path);
}
#endif //#ifndef _SFTPCLINET_GWI_
