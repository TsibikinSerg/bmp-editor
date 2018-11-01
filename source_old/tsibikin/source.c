#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dir.h>
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include <math.h>

/* run this program using the console pauser or add your own getch, system("pause") or input loop */
#define usi   unsigned short int
#define si    signed int
#define ui    unsigned int
#define Byte  unsigned char

//  Максимально-допустимые пиксельные размеры растра
#define ROW_MX_RASTR  		256
#define COL_MX_RASTR  		256    //макс. число пикселей в строке
#define COLOR_MX_RASTR  	65536  //макс. число цветов в палитре
#define BYTE_MX_LINE            1024   //макс. число байт в строке

// Объявление структур, хранящих базовую инфу о файле BMP
//   ...заголовок
#pragma option -a1
typedef struct
{
     char   P_BMP[2];		// признак BMP файла
     ui     Len_fileB;		// длина файла в байтах
     usi    Free[2];		// свободные поля(не документированы)
     ui	    SM; 		// смещение до блока данных
}HEAD_BMP;
//    ... информационный блок
typedef struct
{
     ui	    Len_INFO; 		// размер информационного блока
     ui	    X_max; 		// длина строки растра в пикселах
     ui     Y_max; 		// количество строк растра
     usi    U1;
     usi    Bit_Pix; 		// число бит на пиксел(1,4,8,24)
     ui     P3; 		// признак сжатия
     ui	    Len_BlokB; 		// размер блока данных в байтах
     ui     Lx; 		// разрешение по горизонтали (пикс./метр)
     ui     Ly; 		// разрешение по вертикали (пикс./метр)
     ui	    Kol_C; 		// число используемых цветов палитры
     ui	    Kol_VC; 		// количество важных цветов палитры
}INFO_BMP;

//  ... цвет пикселя
typedef struct
{
unsigned char R;
unsigned char G;
unsigned char B;
unsigned char Q;
}RGBQ;
#pragma option -a4
//---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
char imf[MAXPATH]={'\0'};
HEAD_BMP  hb;
INFO_BMP  ib;
union {usi Col_16; Byte Col_8[2];} Col_Pix_16;
Byte Mask[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80}; //маска, задающая 8 единичных бит байта
Byte LineBMP[BYTE_MX_LINE];     //строка байт, определяющих индекс цвета пикселя в палитре
ui   Hist[COLOR_MX_RASTR];	//гистограмма цветов
RGBQ Pal[COLOR_MX_RASTR];       //палитра

ui    X,Y;                      //текущие координаты пикселей
ui    BitLineBMP;		//число бит в строке
ui    ByteLineBMP;		//число байт в строке
float Raster_Size_M;     	//обобщенный размер растра в метрах(задается пользователем)
ui    Raster_Size_MM;		//обобщенный размер растра в мм по обеим осям
float ResP;                     //размер пикселя по осям в мм
ui    CountPixGistSum;		//число пикселей, попавших в гистограмму
ui    CountColorPal;            //количество различных цветов в палитре
si    fin=-1;                   //дескриптор открываемого файла (условный номер канала связи проги с файлом)
float Sqr_MM=0.0;               //суммарная площадь всех пикселей в мм

    printf("    Calculation of the space occupied on a raster by pixels of the set color");
    printf("\n\n Input of the sizes of a raster in meters. I believe that a");
    printf("\n raster square to provide the equal sizes of pixel on both axes");

    printf("\n\n Name File: "); scanf("%s",imf);     //запросили у пользователя имя файла
    fin = open(imf, O_RDONLY | O_BINARY);  	    //открыть файл на чтение в бинарном виде
    if(fin == -1) perror(" Error Open file"); //сообщение об ошибке открытия файла          //
    else          printf(" File %s is open",imf);    //сообщение, что файл успешно открыт

//  Чтение базовых блоков BMP-файла
    read(fin,&hb, sizeof(hb));		//чтение блока HEADER BMP-файла
    read(fin,&ib, sizeof(ib));		//чтение блока INFO BMP-файла
//  Печать полей РАСТРОВЫЕ РАЗМЕРЫ и ГЛУБИНА ЦВЕТА РАСТРА (число бит кодировки цвета пикселя)
    printf("\n XmaxRastr= %lu\tYmaxRastr= %lu",ib.X_max,ib.Y_max);
    printf("\n BitsPerPixels= %u",ib.Bit_Pix);

//  Ввод обобщенного размера картинки в метрах. Полагаю, что картинка квадратная,
//  поэтому размер один. Упрощение введено, чтобы вычисленный в дальнейшем по
//  этому параметру размер пикселя был одинаков по обеим осям
    printf("\n\n Enter Raster Size(in meters): "); scanf("%f",&Raster_Size_M);

//  Перевод обобщенного размера растра в миллиметры
    Raster_Size_MM=(ui)(Raster_Size_M*1000);
    printf(" Raster_Size_MM= %u",Raster_Size_MM);
    printf(" Raster_Sqwere_MM^2= %u",Raster_Size_MM*Raster_Size_MM);

//  Определяю размер пикселя по осям в мм
    ResP = (float)Raster_Size_MM/(float)ib.X_max; printf("\n ResP= %f",ResP);
    system("pause");
    BitLineBMP = ib.X_max* ib.Bit_Pix;       // число бит в строке
    ByteLineBMP = BitLineBMP >> 3;           // число байт в строке
    if(BitLineBMP %8) ByteLineBMP++;
    while(ByteLineBMP%4)ByteLineBMP++;       //удовлетворение требования, что
    				             //число байт в строке кратно 4
    CountColorPal = pow(2,ib.Bit_Pix);       //Количество различных цветов в палитре
    read(fin,Pal,CountColorPal*sizeof(RGBQ)); //Читаю цветовую палитру

//  Инициализация гистограммы пикселей растра
    for(ui i=0; i<CountColorPal; i++) Hist[i]=0;
//  Формирование гистограммы пикселей растра
    lseek(fin, hb.SM, SEEK_SET);	//сдвиг по файлу к началу цветовой матрицы
    for(Y=0; Y < ib.Y_max; Y++){ 	// построчно читаем файл
        read(fin, LineBMP, ByteLineBMP);
        switch(ib.Bit_Pix){
           case 1:
             for(X=0; X < ib.X_max; X++)
//               Последовательно "вырезаю" каждый из 8 бит байта, значения которых
//               определяют код цвета очередного пикселя при 1-битной кодировке цвета
                 for(usi i=0; i<8; i++) Hist[(LineBMP[X]&Mask[i])]++;
             break;
           case 4:
             for(X=0; X < ib.X_max; X++){
//               Последовательно "вырезаю" группы по 4 бита из байта, значения которых
//               определяют код цвета очередного пикселя при 4-битной кодировке цвета
Byte             c = LineBMP[X] & 0xF0; c>>=4; Hist[c]++;    //старшие 4 бита байта "опускаю" на 4 разряда вниз
                 c = LineBMP[X] & 0x0F;        Hist[c]++;
             }
             break;
           case 8: for(X=0; X < ib.X_max; X++) Hist[LineBMP[X]]++;  break;
           case 16:
             for(X=0; X < ib.X_max; X+=2){
//               Код цвета формируется двумя подряд расположенными байтами в строке
                 Col_Pix_16.Col_8[0]=LineBMP[X];
                 Col_Pix_16.Col_8[1]=LineBMP[X+1];
                 Hist[Col_Pix_16.Col_16]++;
             }
        }
    }
//  Блок верификации гистограммы
    CountPixGistSum=0;
    for(ui i=0; i<CountColorPal; i++) CountPixGistSum+=Hist[i];
    if(CountPixGistSum != (ib.X_max * ib.Y_max)) perror("Error Hist!");
    printf("\n Ok, Hist !!!");
//  Площадь, занимаемая пикселями каждого цвета
    printf("\nI================I==========I==========I");
    printf("\nI  Pixels Color  I  Sqwere  I   Sqwere I");
    printf("\nI                I   (pix)  I   (mm^2) I");
    printf("\nI================I==========I==========I");
    for(ui i=0; i<CountColorPal; i++){
        printf("\nI  %3u %3u %3u   I",Pal[i].R,Pal[i].G,Pal[i].B);
        printf(" %8u I",Hist[i]);
        printf(" %8.3f I",Hist[i]*(ResP*ResP));
        printf("\nI----------------I----------I----------I");
        Sqr_MM+= Hist[i]*(ResP*ResP);
    }
    printf("\nI       All      I %8u I %8.3f I",Raster_Size_MM*Raster_Size_MM,Sqr_MM);
    printf("\nI================I==========I==========I");
    system("pause");
    return 0;
}
