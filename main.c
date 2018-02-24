#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdlib.h>
#include <string.h>

#include <pspgu.h>
#include <pspgum.h>
#include <math.h>
#include "vram.h"

#ifndef bool
#define bool char
#define true 1
#define false 0
#endif

#include "gusprite.h"
#include "game.h"

extern struct SGuSprite sGuSprite_Elements;
extern struct SGuSprite sGuSprite_Numbers;
extern struct SGuSprite sGuSprite_Pointer;
extern struct SGuSprite sGuSprite_Easy;
extern struct SGuSprite sGuSprite_Hard;
extern struct SGuSprite sGuSprite_Medium;

static unsigned int __attribute__((aligned(16))) list[262144];

PSP_MODULE_INFO("Mine",0,1,1);

PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER|THREAD_ATTR_VFPU);

bool done=false;

int exit_callback(int arg1,int arg2,void *common)
{
 done=true;
 return(0);
}
int CallbackThread(SceSize args, void *argp)
{
 int cbid;
 cbid=sceKernelCreateCallback("Exit Callback",exit_callback,NULL);
 sceKernelRegisterExitCallback(cbid);
 sceKernelSleepThreadCB();
 return(0);
}
int SetupCallbacks(void)
{
 int thid = 0;
 thid=sceKernelCreateThread("update_thread",CallbackThread,0x11,0xFA0,0,0);
 if(thid>=0) sceKernelStartThread(thid, 0, 0);
 return(thid);
}

void LoadSprite(char *Path,char *FileName,struct SGuSprite *sGuSprite)
{
 char *FullFileName=(char*)malloc(strlen(Path)+strlen(FileName)+100);
 sprintf(FullFileName,"%s%s",Path,FileName);
 GuSprite_LoadSprite(FullFileName,sGuSprite,0xFF);
 free(FullFileName);
}

void LoadSprites(char *Path)
{
 LoadSprite(Path,"sprites/elements.tga",&sGuSprite_Elements);
 LoadSprite(Path,"sprites/numbers.tga",&sGuSprite_Numbers);

 LoadSprite(Path,"sprites/easy.tga",&sGuSprite_Easy);
 LoadSprite(Path,"sprites/hard.tga",&sGuSprite_Hard);
 LoadSprite(Path,"sprites/medium.tga",&sGuSprite_Medium);

 LoadSprite(Path,"sprites/pointer.tga",&sGuSprite_Pointer);
 GuSprite_ReplaceAlpha(0,0,0,0,&sGuSprite_Pointer);
}
void DeleteSprites(void)
{
 GuSprite_DeleteSprite(&sGuSprite_Elements);
 GuSprite_DeleteSprite(&sGuSprite_Numbers);
 GuSprite_DeleteSprite(&sGuSprite_Pointer);

 GuSprite_DeleteSprite(&sGuSprite_Easy);
 GuSprite_DeleteSprite(&sGuSprite_Hard);
 GuSprite_DeleteSprite(&sGuSprite_Medium);
}

//�������� ���������
int main(int argc, char  **argv)
{
 int n;
 int argv_len=strlen(argv[0]);
 //��������� ��� ����� ������
 //���������� �� �����
 for(n=argv_len;n>0;n--)
 {
  if (argv[0][n-1]=='/')
  {
   argv[0][n]=0;//�������� ������
   break;
  }
 }
 //��������� �������
 LoadSprites(argv[0]);

 pspDebugScreenInit();
 SetupCallbacks();
 sceKernelDcacheWritebackAll();
 //�������� ��������� �� ������� ������

 void* fbp0=getStaticVramBuffer(512,272,GU_PSM_8888);
 void* fbp1=getStaticVramBuffer(512,272,GU_PSM_8888);
 void* zbp=getStaticVramBuffer(512,272,GU_PSM_4444);

 //�������������� ������� GU
 sceGuInit();
 //��������� �� ���������� ����� �������� ������� - �� ������ ����������� �����, �.�. GU_DIRECT
 sceGuStart(GU_DIRECT,list);
 //������������� ��������� ������ ���������- ������ �������, ��������� �� ������� �����������, ����� ������ (�����������, � �� ����������)
 sceGuDrawBuffer(GU_PSM_8888,fbp0,512);
 //������������� ��������� ������ ������ - ������ ������, ��������� �� �����������, ����� ������
 sceGuDispBuffer(480,272,fbp1,512);
 //������������� ��������� ������ �������- ��������� �� ������ ������ ������� � ����������� � ����� ������
 sceGuDepthBuffer(zbp,512);
 //������������� �������� ������ � ����� ������������ 4096x4096 (� PSP ����� ������ ������������ ������, ���� �� �����)
 sceGuOffset(2048-(480/2),2048-(272/2));//������ �� ������
 //����������� ������� ���� - ���� ���������- ���������� ������ � ������� ������
 sceGuViewport(2048,2048,480,272);
 //������������� �������� �������� ��� ������ ������� - �������� � ������ ��������� ��������� (����� ������������ � �������� �� 0 �� 65535 !)
 sceGuDepthRange(65535,0);
 //�������� ��������� ������� ������ �� �������� �������� �����
 sceGuScissor(0,0,480,272);
 sceGuEnable(GU_SCISSOR_TEST);
 sceGuEnable(GU_CLIP_PLANES);
 //�������� ������� �������������
 sceGumMatrixMode(GU_PROJECTION);
 sceGumLoadIdentity();
 sceGumPerspective(90.0f,16.0/9.0f,0.1f,1000.0f);
 //������� ���� �������
 sceGuDepthFunc(GU_LEQUAL);
 sceGuEnable(GU_DEPTH_TEST);
 //������� ����� ������� ������������ ����� ������
 sceGuShadeModel(GU_SMOOTH);
 //�������� ����� ��������� ������, ��������� �������� �������� � �����������
 sceGuFrontFace(GU_CW);
 sceGuDisable(GU_CULL_FACE);
 //��������� ������������
 sceGuEnable(GU_BLEND);
 sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
 //� ������� �����������
 sceGuFinish();
 //���, ���� ���������� ������ (� ��� - �������� �������) �� ����������
 sceGuSync(0,GU_SYNC_DONE);
 //�������� �������
 sceGuDisplay(GU_TRUE);
 sceDisplayWaitVblankStart();

 //��������� ����
 ActivateGame(argv[0]);

 sceGuTerm();
 //����������� ������ �� ���� ��������
 DeleteSprites();
 sceKernelExitGame();
 return(0);
}
