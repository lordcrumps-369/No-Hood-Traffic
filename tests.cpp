#include "profile.h"
#include <stdio.h>

static int cleaned=0,built=0,finished=0,spawned=0;
static void __fastcall Clean(void*,void*) {++cleaned;}
static void __fastcall Build(void*,void*,uintptr_t,uintptr_t) {++built;}
static void __fastcall Finish(void*,void*) {++finished;}
static void __fastcall Spawn(void*,void*) {++spawned;}
static int failures=0;
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)

static void Relocate(unsigned char* code,size_t offset,void* target) {
 intptr_t relative=reinterpret_cast<unsigned char*>(target)-(code+offset+5);
 memcpy(code+offset+1,&relative,4);
}

int main() {
 CHECK(sizeof(void*)==4);
 unsigned char update[sizeof(kOriginalUpdate)],spawn[sizeof(kOriginalSpawnEntry)];
 char name[13];memcpy(update,kOriginalUpdate,sizeof(update));memcpy(spawn,kOriginalSpawnEntry,sizeof(spawn));memcpy(name,"nhoodRoadPath",13);
 CHECK(MatchCode(update,spawn,name));
 for(size_t i=0;i<sizeof(update);++i){update[i]^=1;CHECK(!MatchCode(update,spawn,name));update[i]^=1;}
 for(size_t i=0;i<sizeof(spawn);++i){spawn[i]^=1;CHECK(!MatchCode(update,spawn,name));spawn[i]^=1;}
 for(size_t i=0;i<sizeof(name);++i){name[i]^=1;CHECK(!MatchCode(update,spawn,name));name[i]^=1;}
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));CHECK(code);
 if(!code)return 1;
 memcpy(code,kOriginalUpdate,sizeof(kOriginalUpdate));
 // Execute the actual native update instructions. Only external callees are
 // replaced by local counters; the calling conventions and stack cleanup match.
 Relocate(code,0x10,reinterpret_cast<void*>(&Clean));
 Relocate(code,0x20,reinterpret_cast<void*>(&Build));
 Relocate(code,0x27,reinterpret_cast<void*>(&Finish));
 Relocate(code,0x40,reinterpret_cast<void*>(&Spawn));
 unsigned char fixtureTail[5];memcpy(fixtureTail,code+kTailOffset,5);
 using Update=void(__fastcall*)(void*,void*);
 auto run=reinterpret_cast<Update>(code);
 int cases=0;
 for(int mode=0;mode<3;++mode) {
  if(mode==1)CHECK(SetTail(code+kTailOffset,fixtureTail,kPatchedTail));
  if(mode==2)CHECK(SetTail(code+kTailOffset,kPatchedTail,fixtureTail));
  for(int dirty=0;dirty<2;++dirty)for(int enabled=0;enabled<2;++enabled)for(int occupied=0;occupied<2;++occupied) {
   alignas(4) unsigned char object[128]={};
   object[0x12]=static_cast<unsigned char>(dirty);object[0x11]=static_cast<unsigned char>(enabled);
   if(occupied){*reinterpret_cast<uint32_t*>(object+0x38)=0x1000;*reinterpret_cast<uint32_t*>(object+0x3c)=0x1004;}
   cleaned=built=finished=spawned=0;
   uintptr_t stackBefore=0,stackAfter=0;
   __asm mov stackBefore,esp
   run(object+8,nullptr);
   __asm mov stackAfter,esp
   CHECK(stackBefore==stackAfter);
   CHECK(cleaned==dirty && built==dirty && finished==dirty);
   CHECK(spawned==((mode!=1 && enabled && !occupied)?1:0));
   ++cases;
  }
 }
 // Mismatch must leave the executable bytes untouched.
 unsigned char preserved[5];memcpy(preserved,code+kTailOffset,5);
 CHECK(!SetTail(code+kTailOffset,kPatchedTail,kPatchedTail));
 CHECK(!memcmp(preserved,code+kTailOffset,5));
 VirtualFree(code,0,MEM_RELEASE);
 printf("%d native update cases (original/patched/restored), profile mismatches and stack checks: %s\n",cases,failures?"FAILED":"PASSED");
 return failures?1:0;
}
