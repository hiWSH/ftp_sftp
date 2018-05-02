/*
 * 文件名：SFTPClient.cpp
 * 说明　：SFTP 客户端实现
 * 　　　　提供文件 和 文件夹 上传、下载功能
 * 作者  ：whish
 * 日期  ：2018.04.27
 */ 
#include "stdafx.h"
#include "SFTPClient.h"

static GWI_SFTPClient::GWI_SFTPClient_ErrorCode stuSftpError[] = {\
	{-1,"Can't open local file"},\
	{-2,"libssh2 initialization failed"},\
	{-3,"failed to connect!"},\
	{-4,"libssh2_session_init创建session失败"},\
	{-5,"libssh2_session_handshake Failure establishing SSH session"},\
	{-6,"Authentication failed"},\
	{-7,"Unable to init SFTP session"},\
	{-8,"创建SFTP目录失败"},\
	{-9,"Unable to open file with SFTP"}\
};
static int iGWI_SFTPClient_ErrorCode_len = sizeof(stuSftpError) / sizeof(stuSftpError[0]);



static const char *keyfile1="~/.ssh/id_rsa.pub";
static const char *keyfile2="~/.ssh/id_rsa";
static char gpassword[40]={'\0'};

static unsigned long hostaddr;
static int sock;
static struct sockaddr_in insockaddr;
static int iStatus = -1;
static const char *fingerprint;
static char *userauthlist = NULL;
static LIBSSH2_SESSION *session;


/**
* 函数名：SFTPDirExist
* 说明  ：判断SFTP目录是否存在
* 参数  ：1.sftp_session
*           LIBSSH2_SFTP会话指针
*       ：2.sftppath
*           SFTP目录
* 返回值：1指定目录存在；0指定目录不存在
*/
int SFTPDirExist(LIBSSH2_SFTP *sftp_session, const char *sftppath)
{
	LIBSSH2_SFTP_HANDLE *sftp_handle = libssh2_sftp_opendir(sftp_session, sftppath);
 
    if(sftp_handle)
	{
		libssh2_sftp_closedir(sftp_handle);
	}
	return sftp_handle>0;
}
/**
* 函数名：makeSFTPDir
* 说明  ：创建SFTP目录
* 参数  ：1.sftp_session
*           LIBSSH2_SFTP会话指针
*       ：2.sftppath
*           SFTP目录
* 返回值：0成功；-1失败
*/
int makeSFTPDir(LIBSSH2_SFTP *sftp_session, const char *sftppath)
{
	char tmppath[MAXPATH],tmp[MAXPATH];
	const char *seps = "/";
	char *token;
	memset(tmppath,0,MAXPATH);memset(tmp,0,MAXPATH);
	strncpy(tmp,sftppath,MAXPATH-1);
	token = strtok( tmp, seps );
	while( token != NULL )
	{
		sprintf_s(tmppath,MAXPATH,"%s/%s",tmppath,token);
		if(!SFTPDirExist(sftp_session, tmppath))
			if(libssh2_sftp_mkdir(sftp_session, tmppath,0))
				return -1;
		token = strtok( NULL, seps );
	}
	return 0;
}
static void kbd_sign_callback(const char *name, int name_len,
             const char *instruction, int instruction_len, int num_prompts,
             const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
             LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
             void **abstract)
{
    int i;
    size_t n;
    char buf[1024];
    (void)abstract;

    fwrite(name, 1, name_len, stdout);
    fwrite(instruction, 1, instruction_len, stdout);

    for (i = 0; i < num_prompts; i++) {
    
        fwrite(prompts[i].text, 1, prompts[i].length, stdout);
        strcpy_s(buf,1024,gpassword);
		//fgets(buf, sizeof(buf), stdin);
        n = strlen(buf);
        while (n > 0 && strchr("\r\n", buf[n - 1]))
          n--;
        buf[n] = 0;
 
        responses[i].text = strdup(buf);
        responses[i].length = n;
        fwrite(responses[i].text, 1, responses[i].length, stdout);
    }
}

void GWI_SFTPClient::getGWI_SFTPClient_ErrorMsg_By_Errorcode(int errorcode,char* errormsg)
{
	if (errormsg == nullptr) return;
	for (int i=0 ;i<iGWI_SFTPClient_ErrorCode_len; i++)
	{
		if (stuSftpError[i].errorcode == errorcode)
		{
			strcpy_s(errormsg,MAX_PATH,stuSftpError[i].errormsg);
			return;
		}
	}
	errormsg = "";
}
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
	************************************************************************/
int GWI_SFTPClient::sftpOpen(const char *ip,int port,const char *username, const char *password)
{
	int ret = 0;
	int auth_pw = 0;
	int rc;
	iStatus = 0;
	strcpy_s(gpassword,40,password);

#ifdef WIN32
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2,0), &wsadata);
#endif

	iStatus |= 1;
	hostaddr = inet_addr(ip);
    rc = libssh2_init (0);
    if (rc != 0) {
        return -2;
    }
	iStatus |= 2;
    /*
     * The application code is responsible for creating the socket
     * and establishing the connection
     */ 
    sock = socket(AF_INET, SOCK_STREAM, 0);
    iStatus |= 4;
    insockaddr.sin_family = AF_INET;
    insockaddr.sin_port = htons(port);
    insockaddr.sin_addr.s_addr = hostaddr;

    if (connect(sock, (struct sockaddr*)(&insockaddr),
                sizeof(struct sockaddr_in)) != 0) {
		libssh2_exit();
        return -3;
    }
    /* Create a session instance
     */ 
    session = libssh2_session_init();

    if(!session)
	{
        return -4;
	}
    /* Since we have set non-blocking, tell libssh2 we are blocking */ 
    libssh2_session_set_blocking(session, 1);

    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers
     */ 
   while ((rc = libssh2_session_handshake(session, sock)) == LIBSSH2_ERROR_EAGAIN);
    if (rc) {
        return -5;
    }
	iStatus |= 8;
    /* At this point we havn't yet authenticated.  The first thing to do
     * is check the hostkey's fingerprint against our known hosts Your app
     * may have it hard coded, may go to a file, may present it to the
     * user, that's your call
     */ 
    fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
	//for(i = 0; i < 20; i++) {
    //   fprintf(stderr, "%02X ", (unsigned char)fingerprint[i]);
    //}
    //fprintf(stderr, "\n");
 
	/* check what authentication methods are available */ 
    userauthlist = libssh2_userauth_list(session, username, strlen(username));

	if (strstr(userauthlist, "password") != NULL) {
        auth_pw |= 1;
    }
    if (strstr(userauthlist, "keyboard-interactive") != NULL) {
        auth_pw |= 2;
    }
    if (strstr(userauthlist, "publickey") != NULL) {
        auth_pw |= 4;
    }

	if(auth_pw & 1)
	{
		while ((rc = libssh2_userauth_password(session, username, password)) == LIBSSH2_ERROR_EAGAIN);
		
	}else if(auth_pw & 2)
	{ 
		while ((rc = libssh2_userauth_keyboard_interactive(session, username, &kbd_sign_callback)) == LIBSSH2_ERROR_EAGAIN);
	}else if(auth_pw & 4)
	{ 
		while ((rc = libssh2_userauth_publickey_fromfile(session, username, keyfile1, keyfile2, password)) == LIBSSH2_ERROR_EAGAIN);
	}
	if (rc) {
        ret = -6;
    }
	return 0;
}

/************************************************************************
	 * 函数名 ： sftpClose
	 * 说明   ： 释放资源
	 * 参数   ： 无
	 * 返回   ： 0 成功；1 其它错误
	/************************************************************************/

int GWI_SFTPClient::sftpClose()
{
	if (iStatus&8)
	{
		libssh2_session_disconnect(session, "Normal Shutdown");
		libssh2_session_free(session);
	}

	if (iStatus&4)
	{
#ifdef WIN32
		closesocket(sock);
#else
		close(sock);
#endif
	}
	
	if (iStatus&2)
	{
		libssh2_exit();
	}

	if (iStatus&1)
	{
#ifdef WIN32
		WSACleanup( );
#endif
	}
	iStatus = 0;
	return 0;
}
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
int GWI_SFTPClient::uploadFile(
			   const char *sftppath,
			   const char *slocalfile,
			   const char *sftpfilename/* = NULL*/)
{
	int ret = 0;
	int auth_pw = 0;
    int rc;
    LIBSSH2_SFTP *sftp_session;
    LIBSSH2_SFTP_HANDLE *sftp_handle;

	char sftp_filefullpath[MAX_PATH];
	FILE *hlocalfile;
    char mem[1024*10];
    size_t nread;
    char *ptr;

	hlocalfile = fopen(slocalfile, "rb");
    if (!hlocalfile) {
        return -1;
    }

	sftp_session = libssh2_sftp_init(session);
	if (!sftp_session) {
		return -7;
	}
		 
	/* 上传文件 */
	rc = 0;
	if(!SFTPDirExist(sftp_session, sftppath))
		rc = makeSFTPDir(sftp_session, sftppath);
	if(rc != 0){
		return -8;
	}
	/* FTP文件全路径，包括文件名 */
	sprintf_s(sftp_filefullpath,MAX_PATH,
		sftppath[strlen(sftppath)-1]=='/'?"%s%s":"%s/%s",
		sftppath,
		sftpfilename==NULL?PathFindFileNameA(slocalfile):sftpfilename);

	sftp_handle = libssh2_sftp_open(sftp_session, sftp_filefullpath,
						LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC,
						LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
						LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH);
			 
	if (!sftp_handle) {
		return -9;
	}
	do {
		nread = fread(mem, 1, sizeof(mem), hlocalfile);
		if (nread <= 0) {
			/* end of file */ 
			break;
		}
		ptr = mem;
				 
		do {
			/* write data in a loop until we block */ 
			rc = libssh2_sftp_write(sftp_handle, ptr, nread);
			if(rc < 0)
				break;
			ptr += rc;
			nread -= rc;
		} while (nread);
				 
	} while (rc > 0);
	libssh2_sftp_close(sftp_handle);
	libssh2_sftp_shutdown(sftp_session);
	return 0;
}

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
int GWI_SFTPClient::downloadFile(
			   const char *sftpfile,
			   const char *slocalfile)
{
	int ret = 0;
    int auth_pw = 0;
    int rc;
    LIBSSH2_SFTP *sftp_session;
    LIBSSH2_SFTP_HANDLE *sftp_handle;
	FILE *hlocalfile;
    char mem[1024*10];
 
	sftp_session = libssh2_sftp_init(session);
	if (!sftp_session) {
		return -7;
	}
	/* 打开SFTP文件 */
	sftp_handle = libssh2_sftp_open(sftp_session, sftpfile, LIBSSH2_FXF_READ,
							LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
							LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH);
	if (!sftp_handle) {
		libssh2_sftp_shutdown(sftp_session);
		return -9;
	}
	/* 此API可以直接送带文件名的路径 */
	MakeSureDirectoryPathExists(slocalfile);
	hlocalfile = fopen(slocalfile, "wb+");
	if (!hlocalfile) {
		libssh2_sftp_shutdown(sftp_session);
		return -1;
	}
	do {
		rc = libssh2_sftp_read(sftp_handle, mem, sizeof(mem));	 
		if (rc > 0) {
			fwrite(mem, 1, rc, hlocalfile);
		} else {
			break;
		}
	} while (1);
	if (hlocalfile)
		fclose(hlocalfile);
	libssh2_sftp_close(sftp_handle);
	libssh2_sftp_shutdown(sftp_session);
	return 0;
}
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
int GWI_SFTPClient::getFileList(
			   const char *sftpdir,
			   const char *ext,
			   /*char * retlist*/
			   CString &retlist)
{
	int ret = 0;
    int auth_pw = 0;
    int rc;
    LIBSSH2_SFTP *sftp_session;
    LIBSSH2_SFTP_HANDLE *sftp_handle;
	char tmpbuffer[512];
	char longentry[512];
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	retlist.Empty();
		
	sftp_session = libssh2_sftp_init(session);

	if (!sftp_session) {
		return -7;
	}
	/* 打开SFTP目录 */
	sftp_handle = libssh2_sftp_opendir(sftp_session, sftpdir); 
	if (!sftp_handle) {
		return -8;
	}
				//strcat(retlist,"[");
	retlist.AppendChar('[');
	do {
		wchar_t tmp[256];
		memset(&attrs,0,sizeof(LIBSSH2_SFTP_ATTRIBUTES));
		memset(tmpbuffer,0,sizeof(tmpbuffer));
		memset(longentry,0,sizeof(longentry));
		rc = libssh2_sftp_readdir_ex(sftp_handle, tmpbuffer, sizeof(tmpbuffer),longentry, sizeof(longentry), &attrs);
		if(rc > 0) {
			// 过滤掉当前目录及上一级目录名
			if(strcmp(tmpbuffer,".")==0||strcmp(tmpbuffer,"..")==0)
				continue;
			if(ext&& strlen(ext)>0)
			{
				// 文件类型过滤
				char *pos = strstr(tmpbuffer,ext);
				if(!pos)
					continue;
				if(strcmp(pos,ext)!=0)
					continue;
			}
			MultiByteToWideChar(CP_ACP,0,tmpbuffer,strlen(tmpbuffer)+1,tmp,sizeof(tmp)/sizeof(tmp[0]));
			retlist.Append(tmp);
			retlist.AppendChar(',');
			//strcat(retlist,tmp);
			//strcat(retlist,",");
		} 
		else
			break;
	} while (1);
	if(retlist.ReverseFind(',')==retlist.GetLength()-1)
		retlist.SetAt(retlist.GetLength()-1,'\0');
	retlist.AppendChar(']');
	//if(retlist[strlen(retlist)-1]==',')
	//   retlist[strlen(retlist)-1]='\0';
	//strcat(retlist,"]");
	libssh2_sftp_closedir(sftp_handle);
	libssh2_sftp_shutdown(sftp_session);
		
	return ret;
}

static int preSftpDownloadFileDir(const char sftpDir[MAX_PATH],const char localDir[MAX_PATH],const char extendDir[MAX_PATH])
{
	int rc = -1;
	int iSt = 0;
	LIBSSH2_SFTP *sftp_session;
	LIBSSH2_SFTP_HANDLE *sftp_handle;
	char tmpbuffer[512];
	char longentry[512];
	LIBSSH2_SFTP_ATTRIBUTES attrs;

	char csCreateLocalDir[MAX_PATH] = {};
	sprintf_s(csCreateLocalDir,MAX_PATH,"%s%s",localDir,extendDir);
	CreateDirectoryA(csCreateLocalDir,NULL);
	sftp_session = libssh2_sftp_init(session);

	if (!sftp_session) {
		return -7;
	}
	/* 打开SFTP目录 */
	sftp_handle = libssh2_sftp_opendir(sftp_session, sftpDir); 
	if (!sftp_handle) {
		libssh2_sftp_shutdown(sftp_session);
		return -8;
	}

	do {
		memset(&attrs,0,sizeof(LIBSSH2_SFTP_ATTRIBUTES));
		memset(tmpbuffer,0,sizeof(tmpbuffer));
		memset(longentry,0,sizeof(longentry));
		rc = libssh2_sftp_readdir_ex(sftp_handle, tmpbuffer, sizeof(tmpbuffer),longentry, sizeof(longentry), &attrs);
		if(rc > 0) {
			// 过滤掉当前目录及上一级目录名
			if(strcmp(tmpbuffer,".")==0||strcmp(tmpbuffer,"..")==0)
				continue;
		    //add 判断是文件夹 还是文件 通过末尾的'.' 和 是否含有下级目录 
			char* pcTmp = strrchr(tmpbuffer,'.');
			if (!pcTmp)//不是带后缀的文件
			{
				char sftpDirAdd[MAX_PATH] = {};
				sprintf_s(sftpDirAdd,MAX_PATH,"%s/%s",sftpDir,tmpbuffer);
				LIBSSH2_SFTP_HANDLE* sftp_handle = libssh2_sftp_open(sftp_session, sftpDirAdd, LIBSSH2_FXF_READ,
					LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
					LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH);
				if (sftp_handle) { //没有后缀的文件
					libssh2_sftp_close(sftp_handle);
					char localDirAdd[MAX_PATH] = {};
					sprintf_s(localDirAdd,MAX_PATH,"%s%s\\\\%s",localDir,extendDir,tmpbuffer);
					iSt = GWI_SFTPClient::downloadFile(sftpDirAdd,localDirAdd);			
				}
				else{  //文件夹
					char externDirAdd[MAX_PATH] = {};
					sprintf_s(externDirAdd,"%s\\\\%s",extendDir,tmpbuffer);
					iSt = preSftpDownloadFileDir(sftpDirAdd,localDir,externDirAdd);
				}
			}
			else //带后缀的文件
			{
				char SftpFilePath[MAX_PATH] = {};
				sprintf_s(SftpFilePath,MAX_PATH,"%s/%s",sftpDir,tmpbuffer);
				char localDirAdd[MAX_PATH] = {};
				sprintf_s(localDirAdd,MAX_PATH,"%s%s\\\\%s",localDir,extendDir,tmpbuffer);
				iSt = GWI_SFTPClient::downloadFile(SftpFilePath,localDirAdd);
			}
			if (iSt != 0) //递归错误 直接结束上层调用
			{
				break;
			}
		} 
		else
			break;
	
	} while (1);
	//释放资源
	libssh2_sftp_closedir(sftp_handle);
	libssh2_sftp_shutdown(sftp_session);
	return iSt;
}
/************************************************************************
函数名：sftpdDwnloadFileDir
说明：实现SFTP文件夹下载到本地，包括空文件    
	参数：输入 
const char *sftpdir ： 上传到SFTP的文件夹名
const char *slocaldir：本地文件夹名
	        输出
无
返回：0 成功  其它失败
************************************************************************/
int GWI_SFTPClient::sftpDwnloadFileDir(const char *sftpdir, const char *slocaldir)
{
	return preSftpDownloadFileDir(sftpdir,slocaldir,"");
}

static int preSftpUpdateFileDir(const char localDir[MAX_PATH],const char sftpDir[MAX_PATH],const char externDir[MAX_PATH])      
{  
	WIN32_FIND_DATAA FindFileData;  
	HANDLE hFind=INVALID_HANDLE_VALUE;  
	char DirSpec[MAX_PATH];                  //定义要遍历的文件夹的目录  
	sprintf_s(DirSpec,MAX_PATH,"%s\\*",localDir);

	hFind=FindFirstFileA(DirSpec,&FindFileData);          //找到文件夹中的第一个文件  
	bool ret = 0;
	if(hFind==INVALID_HANDLE_VALUE)                               //如果hFind句柄创建失败，输出错误信息  
	{  
		return -1;    
	}  

	char csSftpFilePath[MAX_PATH] = {};
	sprintf_s(csSftpFilePath,MAX_PATH,"%s%s",sftpDir,externDir);
	LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
	if(!SFTPDirExist(sftp_session, csSftpFilePath))
		ret = makeSFTPDir(sftp_session, csSftpFilePath);
	libssh2_sftp_shutdown(sftp_session);
	if(ret != 0){
		return -8;
	}
	else   
	{   
		while(FindNextFileA(hFind,&FindFileData)!=0)                            //当文件或者文件夹存在时  
		{  
			if((FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0&&strcmp(FindFileData.cFileName,".")==0||strcmp(FindFileData.cFileName,"..")==0)        //判断是文件夹&&表示为"."||表示为"."  
			{  
				continue;  
			}  
			char DirAdd[MAX_PATH];  
			sprintf_s(DirAdd,MAX_PATH,"%s\\%s",localDir,FindFileData.cFileName);
			if((FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0)      //判断如果是文件夹  
			{  
				//拼接得到此文件夹的完整路径  
				char ExternDirAdd[MAX_PATH];
				sprintf_s(ExternDirAdd,MAX_PATH,"%s/%s",externDir,FindFileData.cFileName);
				ret = preSftpUpdateFileDir(DirAdd,sftpDir,ExternDirAdd);                                  //实现递归调用  
				if(ret)
				{
					FindClose(hFind);  
					return ret;
				}
			}  
			if((FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0)    //如果不是文件夹  
			{  
				//wcout<<Dir<<"\\"<<FindFileData.cFileName<<endl;            //输出完整路径  
				char sftpDirAdd[MAX_PATH] = {};
				sprintf_s(sftpDirAdd,MAX_PATH,"%s%s",sftpDir,externDir);
				
				char csLocalDirAdd[MAX_PATH] = {};
				sprintf_s(csLocalDirAdd,MAX_PATH,"%s\\%s",localDir,FindFileData.cFileName);

				ret = GWI_SFTPClient::uploadFile(sftpDirAdd,csLocalDirAdd);
				if(ret)
				{
					FindClose(hFind);  
					return ret;
				}
			}  
		}  
		
		FindClose(hFind);  
	} 
	return ret;
} 

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
int GWI_SFTPClient::sftpUpdateFileDir(const char *sftpdir,const char *slocaldir)
{
	return preSftpUpdateFileDir(slocaldir,sftpdir,"");
}

/************************************************************************/
/* 函数： sftpCd
/* 说明： 创建sftp文件夹
/* 参数： 输入
const char* path：文件夹路径
/************************************************************************/
int GWI_SFTPClient::sftpCd(const char* path)
{
	int ret = 0;
	LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
	if(!SFTPDirExist(sftp_session, path))
		ret = makeSFTPDir(sftp_session, path);
	libssh2_sftp_shutdown(sftp_session);
	if (ret)
	{
		return -8;
	}
	return ret;
}

