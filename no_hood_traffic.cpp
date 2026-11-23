#include "profile.h"

static void WriteStatus(HMODULE module,const char* message) {
 wchar_t path[MAX_PATH];
 DWORD n=GetModuleFileNameW(module,path,MAX_PATH);
 if(!n || n>=MAX_PATH-5)return;
 wchar_t* dot=wcsrchr(path,L'.');
 if(!dot)return;
 wcscpy_s(dot,static_cast<size_t>(MAX_PATH-(dot-path)),L".log");
 HANDLE file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE)return;
 DWORD written=0;WriteFile(file,message,static_cast<DWORD>(strlen(message)),&written,nullptr);CloseHandle(file);
}

BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {
 if(reason!=DLL_PROCESS_ATTACH)return TRUE;
 DisableThreadLibraryCalls(module);
 auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
 auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
 if(!base || dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>0x100000) {
  WriteStatus(module,"INACTIVE: unsupported executable header.\r\n");return TRUE;
 }
 auto nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
 if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 ||
    nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
    nt->OptionalHeader.SizeOfImage!=kImageSize || nt->FileHeader.TimeDateStamp!=kTimestamp) {
  WriteStatus(module,"INACTIVE: executable does not match supported UC EP9 profile.\r\n");return TRUE;
 }
 auto update=base+kUpdateRva;
 if(!MatchCode(update,base+kSpawnRva,reinterpret_cast<char*>(base+kNameRva))) {
  WriteStatus(module,"INACTIVE: native traffic code/name differs; no patch applied.\r\n");return TRUE;
 }
 if(!SetTail(update+kTailOffset,kOriginalUpdate+kTailOffset,kPatchedTail)) {
  WriteStatus(module,"INACTIVE: unable to apply guarded traffic-tail patch.\r\n");return TRUE;
 }
 WriteStatus(module,"ACTIVE Test01: native neighbourhood traffic update returns before creating nhoodRoadPath effects.\r\nDay/night visibility and lot vehicle behaviour still require in-game validation.\r\n");
 return TRUE;
}
