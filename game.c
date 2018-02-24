#include "game.h"
#include "gusprite.h"

struct SGuSprite sGuSprite_Elements;
struct SGuSprite sGuSprite_Numbers;
struct SGuSprite sGuSprite_Pointer;
struct SGuSprite sGuSprite_Easy;
struct SGuSprite sGuSprite_Hard;
struct SGuSprite sGuSprite_Medium;

extern bool done;
static unsigned int __attribute__((aligned(16))) local_list[524288];

//игровое поле
struct SElement
{
 bool Mine;//true-в клетке мина
 bool Open;//true-клетка открыта
 int MineAmount;//сколько мин вокруг клетки
 bool Select;//true-клетка уже отмечена.(нужно для открывания поля, чтобы помечать, где уже рекурсивный алгоритм был)

 int Flag;//флаг поля:0-нет, 1- тут мина, 2- не знаю, что тут
 bool Explosion;//true-здесь произошёл взрыв

} sElement_MineMap[16][26];

//данные курсора
struct SCursor
{
 int X;//координаты курсора
 int Y;

 int MapWidth;//размер поля по X
 int MapHeight;//размер поля по Y
 int MapMineAmount;//количество мин на поле
 bool PressCross;//нажат крестик
 bool PressTriangle;//нажат треугольник
 bool PressSquare;//нажат квадрат

 bool Explosion;//true-произошёл взрыв

 float Time;//текущее время
 float dTime;//изменение времени

 int SetMine;//количество отмеченных мин
} sCursor;

int RND(int max)
{
 return((int)((float)(max)*((float)rand())/((float)RAND_MAX)));
}
//создание поля
void CreateMap(void)
{
 u64 current_tick;
 sceRtcGetCurrentTick(&current_tick);
 int value=current_tick&0xFFFFFFFF;
 srand(value);
 int n,x,y;
 //очищаем поле
 for(x=0;x<sCursor.MapWidth;x++)
  for(y=0;y<sCursor.MapHeight;y++)
  {
   sElement_MineMap[y][x].Mine=false;
   sElement_MineMap[y][x].Open=false;
   sElement_MineMap[y][x].Explosion=false;
   sElement_MineMap[y][x].Flag=0;
  }
 //расставляем мины
 for(n=0;n<sCursor.MapMineAmount;n++)
 {
  while(done==false)
  {
   x=RND(sCursor.MapWidth);
   y=RND(sCursor.MapHeight);
   if (x>=sCursor.MapWidth) x=sCursor.MapWidth-1;
   if (y>=sCursor.MapHeight) x=sCursor.MapHeight-1;
   if (sElement_MineMap[y][x].Mine==true) continue;
   sElement_MineMap[y][x].Mine=true;
   break;
  }
 }
 //считаем, сколько мин вокруг клетки
 for(x=0;x<sCursor.MapWidth;x++)
  for(y=0;y<sCursor.MapHeight;y++)
  {
   sElement_MineMap[y][x].MineAmount=0;
   int i,j;
   for(i=x-1;i<=x+1;i++)
    for(j=y-1;j<=y+1;j++)
    {
     if (i<0 || j<0 || i>=sCursor.MapWidth || j>=sCursor.MapHeight) continue;
     if (sElement_MineMap[j][i].Mine==true) sElement_MineMap[y][x].MineAmount++;
    }
  }
}
//открытие поля
void OpenMap(int x,int y)
{
 if (x<0 || y<0) return;//вне поля
 if (x>=sCursor.MapWidth || y>=sCursor.MapHeight) return;//вне поля
 struct SElement *sElement=&sElement_MineMap[y][x];
 if (sElement->Select==true) return;
 if (sElement->Flag!=0) return;
 sElement->Open=true;
 sElement->Select=true;
 if (sElement->MineAmount!=0) return;//достигли границы
 OpenMap(x+1,y);
 OpenMap(x-1,y);
 OpenMap(x,y+1);
 OpenMap(x,y-1);
 OpenMap(x+1,y+1);
 OpenMap(x+1,y-1);
 OpenMap(x-1,y+1);
 OpenMap(x-1,y-1);
}
//открыть клетку
void OpenItem(int x,int y)
{
 struct SElement *sElement=&sElement_MineMap[y][x];
 if (sElement->Open==true) return;//уже открыто
 if (sElement->Flag==1) return;//эта клетка помечена как мина
 if (sElement->Mine==true)
 {
  sElement->Explosion=true;//здесь произошёл взрыв
  sCursor.Explosion=true;//и вообще, мы подорвались
  sCursor.dTime=0;//время останавливается
  return;//нарвались на мину
 }
 sElement->Open=true;//открываем клетку
 sElement->Flag=0;
 //открываем поле, если это возможно
 int xm;
 int ym;
 for(xm=0;xm<sCursor.MapWidth;xm++)
  for(ym=0;ym<sCursor.MapHeight;ym++)
  {
   sElement_MineMap[ym][xm].Select=false;
  }
 OpenMap(x,y);
}
//открытие клеток
void OnSquare(void)
{
 if (sCursor.Explosion==true) return;//игра неактивна
 int x=sCursor.X-64;
 int y=sCursor.Y-16;
 if (x<0 || y<0) return;//вне поля
 if (x>=16*sCursor.MapWidth || y>=16*sCursor.MapHeight) return;//вне поля
 x/=16;
 y/=16;
 struct SElement *sElement=&sElement_MineMap[y][x];
 if (sElement->Open==true)
 {
  int value=sElement->MineAmount;//сколько мин вокруг клетки
  //считаем, сколько флажков вокруг клетки
  int x1=x-1;
  int x2=x+1;
  int y1=y-1;
  int y2=y+1;
  if (x1<0) x1=0;
  if (y1<0) y1=0;
  if (x2>=sCursor.MapWidth) x2=sCursor.MapWidth-1;
  if (y2>=sCursor.MapHeight) y2=sCursor.MapHeight-1;
  int flag=0;
  int xc,yc;
  for(xc=x1;xc<=x2;xc++)
   for(yc=y1;yc<=y2;yc++)
   {
    if (xc==x && yc==y) continue;
    sElement=&sElement_MineMap[yc][xc];
    if (sElement->Flag==1 && sElement->Open==false) flag++;
   }
  if (value==flag)//все мины отмечены
  {
   //открываем квадрат
   for(xc=x1;xc<=x2;xc++)
    for(yc=y1;yc<=y2;yc++)
    {
     if (xc==x && yc==y) continue;
     OpenItem(xc,yc);
    }
  }
 }
}
//выбор клетки
void OnCross(void)
{
 if (sCursor.X>=64 && sCursor.X<64+sGuSprite_Easy.Width && sCursor.Y>=0 && sCursor.Y<16)//новичок
 {
  sCursor.MapWidth=9;
  sCursor.MapHeight=9;
  sCursor.MapMineAmount=10;
  sCursor.SetMine=0;
  sCursor.Time=0;
  sCursor.dTime=0;
  sCursor.PressCross=false;
  sCursor.PressTriangle=false;
  sCursor.Explosion=false;
  //генерируем поле
  CreateMap();
  return;
 }
 if (sCursor.X>=164 && sCursor.X<164+sGuSprite_Medium.Width && sCursor.Y>=0 && sCursor.Y<16)//любитель
 {
  sCursor.MapWidth=16;
  sCursor.MapHeight=16;
  sCursor.MapMineAmount=40;
  sCursor.SetMine=0;
  sCursor.Time=0;
  sCursor.dTime=0;
  sCursor.PressCross=false;
  sCursor.PressTriangle=false;
  sCursor.Explosion=false;
  //генерируем поле
  CreateMap();
  return;
 }
 if (sCursor.X>=264 && sCursor.X<264+sGuSprite_Hard.Width && sCursor.Y>=0 && sCursor.Y<16)//профессионал
 {
  sCursor.MapWidth=26;
  sCursor.MapHeight=16;
  sCursor.MapMineAmount=99;
  sCursor.SetMine=0;
  sCursor.Time=0;
  sCursor.dTime=0;
  sCursor.PressCross=false;
  sCursor.PressTriangle=false;
  sCursor.Explosion=false;
  //генерируем поле
  CreateMap();
  return;
 }
 if (sCursor.Explosion==true) return;//игра неактивна
 int x=sCursor.X-64;
 int y=sCursor.Y-16;
 if (x<0 || y<0) return;//вне поля
 if (x>=16*sCursor.MapWidth || y>=16*sCursor.MapHeight) return;//вне поля
 x/=16;
 y/=16;
 sCursor.dTime=1.0f/60.0f;
 OpenItem(x,y);
}

void OnTriangle(void)
{
 if (sCursor.Explosion==true) return;//игра неактивна
 int x=sCursor.X-64;
 int y=sCursor.Y-16;
 if (x<0 || y<0) return;//вне поля
 if (x>=16*sCursor.MapWidth || y>=16*sCursor.MapHeight) return;//вне поля
 x/=16;
 y/=16;
 struct SElement *sElement=&sElement_MineMap[y][x];
 if (sElement->Open==true) return;//уже открыто
 if (sElement->Flag==1) sCursor.SetMine--;
 if (sElement->Flag==0) sCursor.SetMine++;
 sElement->Flag++;
 sElement->Flag%=3;
}
//управление от клавиш
void KeyboardControl(char *Path)
{
 SceCtrlData pad;
 //считываем положение аналогового джойстика
 sceCtrlReadBufferPositive(&pad, 1);
 int dx=(pad.Lx-127);
 int dy=(pad.Ly-127);
 int sdx=1;
 int sdy=1;
 if (dx<0) sdx=-1;
 if (dy<0) sdy=-1;
 if (fabs(dx)<10) dx=0;
 if (fabs(dy)<10) dy=0;
 dx*=0.02;
 dy*=0.02;
 sCursor.X+=dx;
 sCursor.Y+=dy;
 if (sCursor.X<0) sCursor.X=0;
 if (sCursor.X>=480) sCursor.X=479;
 if (sCursor.Y<0) sCursor.Y=0;
 if (sCursor.Y>=272) sCursor.Y=271;
 //смотрим на нажатия клавиш
 if ((pad.Buttons&PSP_CTRL_LTRIGGER) && (pad.Buttons&PSP_CTRL_RTRIGGER))//нажали правую и левую клавиши
 {
  //рестарт уровня
  sCursor.Time=0;
  sCursor.dTime=0;
  sCursor.PressCross=false;
  sCursor.PressTriangle=false;
  sCursor.Explosion=false;
  sCursor.SetMine=0;
  //генерируем поле
  CreateMap();
  return;
 }
 if (pad.Buttons&PSP_CTRL_CROSS)//нажали крестик
 {
  sCursor.PressCross=true;
  return;
 }
 else
 {
  if (sCursor.PressCross==true) OnCross();
  sCursor.PressCross=false;
 }
 if (pad.Buttons&PSP_CTRL_TRIANGLE)//нажали треугольник
 {
  sCursor.PressTriangle=true;
  return;
 }
 else
 {
  if (sCursor.PressTriangle==true) OnTriangle();
  sCursor.PressTriangle=false;
 }
 if (pad.Buttons&PSP_CTRL_SQUARE)//нажали квадрат
 {
  sCursor.PressSquare=true;
  OnSquare();
  return;
 }
 else
 {
  sCursor.PressSquare=false;
  return;
 }
}
void PutNumber(int x,int y,int number)
{
 char string[255];
 sprintf(string,"%i",number);
 int length=strlen(string);
 if (length>4) return;//только четырёхэлементные числа поддерживаем
 int n,m;
 for(n=0;n<4-length;n++) GuSprite_PutSpriteElement(x+n*13,y,0,23,13,23,&sGuSprite_Numbers);//рисуем пробел
 for(m=0;n<4;n++,m++)
 {
  unsigned char s=string[m];
  if (s==(unsigned char)'-') GuSprite_PutSpriteElement(x+n*13,y,0,0,13,23,&sGuSprite_Numbers);//рисуем минус
  if (s>=(unsigned char)'0' && s<=(unsigned char)'9') GuSprite_PutSpriteElement(x+n*13,y,0,23*(11-(s-(unsigned char)'0')),13,23,&sGuSprite_Numbers);//рисуем число
 }
}
void DrawMap(void)
{
 int x,y;
 //определим положение курсора
 int xc=sCursor.X-64;
 int yc=sCursor.Y-16;
 bool in_map=false;
 if (xc>=0 && yc>=0 && xc<16*sCursor.MapWidth && yc<16*sCursor.MapHeight) in_map=true;
 xc/=16;
 yc/=16;
 //выводим поле
 for(x=0;x<sCursor.MapWidth;x++)
  for(y=0;y<sCursor.MapHeight;y++)
  {
   struct SElement *sElement=&sElement_MineMap[y][x];
   //выводим блок, на который наведён курсор при нажатом квадрате
   bool put_box=false;
   if (x-xc>=-1 && x-xc<=1 && y-yc>=-1 && y-yc<=1 && sCursor.PressSquare==true && in_map==true && sCursor.Explosion==false) put_box=true;//если мы в блоке курсора
   if (put_box==true) GuSprite_PutSpriteElement(x*16+64,y*16+16,0,15*16,16,16,&sGuSprite_Elements);//рисуем пустой квадратик
   //выводим элементы
   if (sElement->Open==false)
   {
    if (put_box==false || sElement->Flag>0) GuSprite_PutSpriteElement(x*16+64,y*16+16,0,16*sElement->Flag,16,16,&sGuSprite_Elements);//клетка не открыта
   }
   else
   {
    if (sElement->Mine==false) GuSprite_PutSpriteElement(x*16+64,y*16+16,0,(15-sElement->MineAmount)*16,16,16,&sGuSprite_Elements);//рисуем количество мин вокруг клетки
   }
  }
 if (sCursor.Explosion==true)//произошёл взрыв
 {
  //выводим мины
  for(x=0;x<sCursor.MapWidth;x++)
   for(y=0;y<sCursor.MapHeight;y++)
   {
    struct SElement *sElement=&sElement_MineMap[y][x];
    if (sElement->Mine==true && sElement->Flag!=1) GuSprite_PutSpriteElement(x*16+64,y*16+16,0,5*16,16,16,&sGuSprite_Elements);//в клетке неотмеченная мина
    if (sElement->Mine==true && sElement->Flag==1) GuSprite_PutSpriteElement(x*16+64,y*16+16,0,4*16,16,16,&sGuSprite_Elements);//в клетке отмеченная мина
    if (sElement->Mine==true && sElement->Explosion==true) GuSprite_PutSpriteElement(x*16+64,y*16+16,0,3*16,16,16,&sGuSprite_Elements);//в клетке взорвавшаяся мина
   }
 }
 //выводим время
 PutNumber(0,24,(int)sCursor.Time);
 //выводим количество оставшихся мин
 PutNumber(0,54,(int)sCursor.MapMineAmount-sCursor.SetMine);
 //выводим пункты меню
 GuSprite_PutSprite(64,0,&sGuSprite_Easy);
 GuSprite_PutSprite(164,0,&sGuSprite_Medium);
 GuSprite_PutSprite(264,0,&sGuSprite_Hard);
}
void CheckFinish(void)
{
 if (sCursor.Explosion==true) return;//мы уже подорвались
 int x,y;
 if (sCursor.MapMineAmount!=sCursor.SetMine) return;//не всем мины отмечены
 //выводим поле
 for(x=0;x<sCursor.MapWidth;x++)
  for(y=0;y<sCursor.MapHeight;y++)
  {
   struct SElement *sElement=&sElement_MineMap[y][x];
   if (sElement->Open==false && sElement->Mine==false) return;//не все клетки открыты
  }
 //поле открыто полностью - вы выиграли
 sCursor.Explosion=true;//и вообще, мы подорвались
 sCursor.dTime=0;//время останавливается
}
//начинаем игру
void ActivateGame(char *Path)
{
 sCursor.X=0;
 sCursor.Y=0;

 sceCtrlSetSamplingCycle(0);
 sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

 sCursor.MapWidth=9;
 sCursor.MapHeight=9;
 sCursor.MapMineAmount=10;
 sCursor.SetMine=0;
 sCursor.Time=0;
 sCursor.dTime=0;
 sCursor.PressCross=false;
 sCursor.PressTriangle=false;
 sCursor.PressSquare=false;
 sCursor.Explosion=false;
 //генерируем поле
 CreateMap();

 while(done==false)
 {
  CheckFinish();
  KeyboardControl(Path);
  sceGuStart(GU_DIRECT,local_list);
  //очистим экран и буфер глубины
  sceGuClearColor(0);
  sceGuClearDepth(0);
  sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
  //инициализируем матрицы
  sceGumMatrixMode(GU_TEXTURE);
  sceGumLoadIdentity();
  sceGumMatrixMode(GU_VIEW);
  sceGumLoadIdentity();
  sceGumMatrixMode(GU_MODEL);
  sceGumLoadIdentity();
  sceGuEnable(GU_TEXTURE_2D);
  //рисуем сцену
  DrawMap();
  GuSprite_PutSprite(sCursor.X-7,sCursor.Y-1,&sGuSprite_Pointer);
  //и выводим изображение на экран
  sceGuFinish();
  //ждём, пока дисплейный список (у нас - контекст дисплея) не выполнится
  sceGuSync(0,GU_SYNC_DONE);
  sceDisplayWaitVblankStart();
  //делаем видимым буфер, в котором мы рисовали
  sceGuSwapBuffers();
  sCursor.Time+=sCursor.dTime;
  if (sCursor.Time>9999) sCursor.Time=9999;
 }
}
