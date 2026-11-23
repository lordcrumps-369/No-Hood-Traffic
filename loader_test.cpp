#include <windows.h>
#include <stdio.h>
#include <string.h>
int main() {
 HMODULE module=LoadLibraryW(L".\\TS2-NoHoodTraffic.asi");
 if(!module){printf("ASI load failed: %lu\n",GetLastError());return 1;}
 FreeLibrary(module);
 HANDLE file=CreateFileW(L"TS2-NoHoodTraffic.log",GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
 if(file==INVALID_HANDLE_VALUE)return 2;
 char message[512]={};DWORD read=0;BOOL ok=ReadFile(file,message,sizeof(message)-1,&read,nullptr);CloseHandle(file);
 if(!ok || !strstr(message,"INACTIVE: executable does not match supported UC EP9 profile."))return 3;
 printf("Actual ASI loads in x86 process and rejects unsupported executable: PASSED\n");
 return 0;
}
