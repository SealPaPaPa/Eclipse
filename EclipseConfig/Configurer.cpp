#include"..\Eclipse\Eclipse.h"

int main(int argc, char**argv) {
	char banner[] = "!@#$%^_CONFIG_^%$#@!";
	if (argc != 4) {
		printf("%s [Host] [Port] [SleepTime]\n", argv[0]);
		return 0;
	}
	SettingDataStruct settingDataStruct;
	memset(&settingDataStruct, 0, sizeof(SettingDataStruct));
	strcpy_s(settingDataStruct.host, 100, argv[1]);
	sscanf_s(argv[2], "%d", &settingDataStruct.dwPort);
	sscanf_s(argv[3], "%d", &settingDataStruct.dwSleepTime);

	char cFileName[300] = "Eclipse.exe";
	char cOutputFileName[300] = "Output.exe";
	HANDLE hFile = CreateFileA(cFileName, GENERIC_ALL, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("Can't open %s.\n", cFileName);
		return 0;
	}
	DWORD dwFileSize = GetFileSize(hFile, 0), dwReaded;
	char* buf = (char*)malloc(dwFileSize);
	ReadFile(hFile, buf, dwFileSize, &dwReaded, 0);
	CloseHandle(hFile);

	char* ptr = 0;
	for (int n = 0; n < dwFileSize; n++) {
		ptr = strstr(buf + n, banner);
		if (ptr)break;
	}
	if (!ptr) {
		printf("Can't find Banner.\n");
	}
	memcpy_s(ptr, sizeof(SettingDataStruct), &settingDataStruct, sizeof(SettingDataStruct));
	hFile = CreateFileA(cOutputFileName, GENERIC_ALL, FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("Can't create %s.\n", cOutputFileName);
		return 0;
	}
	WriteFile(hFile, buf, dwFileSize, &dwReaded, 0);
	CloseHandle(hFile);
	free(buf);

	printf("Host: %s\n", settingDataStruct.host);
	printf("Port: %d\n", settingDataStruct.dwPort);
	printf("Sleep Time: %d\n", settingDataStruct.dwSleepTime);

	return 0;
}