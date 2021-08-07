//OLED SSD1306 COUNTER PIC16F628A
//MPLAB X v3.10 xc8 v1.45
//moty22.co.uk

#include <htc.h>
#include "oled_font.c"

#pragma config WDTE=OFF, MCLRE=OFF, BOREN=OFF, FOSC=HS, CP=OFF, CPD=OFF, LVP=OFF

#define SCL RA3 //pin2
#define SDA RA2 //pin1
#define _XTAL_FREQ 8000000

// prototypes
void command( unsigned char comm);
void oled_init();
void clrScreen();
void sendData(unsigned char dataB);
void startBit(void);
void stopBit(void);
void clock(void);
void drawChar2(char fig, unsigned char y, unsigned char x);

unsigned char addr=0b1111000;  //0x78

void main(void) {
   	unsigned long total;
	unsigned char timebase,nz,i,d[7];
	unsigned int freq2;
    
    	// PIC I/O init
    CMCON = 0b111;		//comparator off
    TRISA = 0b110011;   //SCL, SDA outputs
	//TRISB = 0;		 //
    
    OPTION_REG = 0b11111000;	//tmr0 1:1
    T1CON=0;		//timer OFF,
	CCP1CON=0b1011;		//1011 = Compare mode, trigger special event (CCP1IF bit is set; CCP1 resets TMR1.
	CCPR1H=0x9c; CCPR1L=0x40;	//CCP in compare mode sets TMR1 to a period of 40 ms  , 9C40=40000
	
    SCL=1;
    SDA=1;
    
    __delay_ms(1000);
    oled_init();   // oled init
    __delay_ms(1000);
    clrScreen();       // clear screen

    while (1) {
 
         //Frequency Counter
        nz=1;
        freq2 = 0;	//clear timers
        timebase=50;
        TMR1L=0;  TMR1H=0;			
        TMR1ON = 1;	//start count
        TMR0 = 0;
        TMR0IF = 0;

        while(timebase){		//1 sec 
            if(TMR0IF){++freq2; TMR0IF = 0;}
            if(CCP1IF){CCP1IF=0; --timebase;}
        }
        TMR1ON = 0;	//stop count

        total=(unsigned long)TMR0 + (unsigned long)freq2 * 256;
 
        //convert to 7 decimal digits
        d[6]=total/1000000;		//1MHz digit
        d[5]=(total/100000) %10;	//100KHz digit
        d[4]=(total/10000) %10;
        d[3]=(total/1000) %10;
        d[2]=(total/100) %10;
        d[1]=(total/10) %10;		//10Hz digit	
        d[0]=total %10;

        for(i=7;i>1;i--){
            if(!d[i-1] && nz){drawChar2(10, 1, 7-i);} //if leading 0 display blank
            if(d[i-1] && nz){nz=0; }
            if(!nz){drawChar2(d[i-1], 1, 7-i);}
        }
        drawChar2(d[0], 1, 6);	//display digit 0

        drawChar2(11, 1, 8); //H 0x48
        drawChar2(15, 1, 9); //z

        nz=1;
        if(total<1000){
            total=1000000/total; 
            drawChar2(14, 5, 8); //u
        }else{
            total=1000000000/total;
            drawChar2(13, 5, 8);}    //n

        //convert to 7 decimal digits
        d[6]=total/1000000;		//1MHz digit
        d[5]=(total/100000) %10;	//100KHz digit
        d[4]=(total/10000) %10;
        d[3]=(total/1000) %10;
        d[2]=(total/100) %10;
        d[1]=(total/10) %10;		//10Hz digit	
        d[0]=total %10;

        for(i=7;i>1;i--){
            if(!d[i-1] && nz){drawChar2(10, 5, 7-i);} //if leading 0 display blank
            if(d[i-1] && nz){nz=0; }
            if(!nz){drawChar2(d[i-1], 5, 7-i);}
        }	
         drawChar2(d[0], 5, 6);	//display digit 0

         drawChar2(12, 5, 9); //S

         __delay_ms(2000);

	}
}
    //size 2 chars
void drawChar2(char fig, unsigned char y, unsigned char x)
{
    unsigned char i, line, btm, top;    //
    
    command(0x20);    // vert mode
    command(0x01);

    command(0x21);     //col addr
    command(13 * x); //col start
    command(13 * x + 9);  //col end
    command(0x22);    //0x22
    command(y); // Page start
    command(y+1); // Page end
    
        startBit();
        sendData(addr);            // address
        sendData(0x40);
        
        for (i = 0; i < 5; i++){
            line=font[5*(fig)+i];
            btm=0; top=0;
                // expend char    
            if(line & 64) {btm +=192;}
            if(line & 32) {btm +=48;}
            if(line & 16) {btm +=12;}           
            if(line & 8) {btm +=3;}
            
            if(line & 4) {top +=192;}
            if(line & 2) {top +=48;}
            if(line & 1) {top +=12;}        

             sendData(top); //top page
             sendData(btm);  //second page
             sendData(top);
             sendData(btm);
        }
        stopBit();
        
    command(0x20);      // horizontal mode
    command(0x00);    
        
}

void clrScreen()    //fill screen with 0
{
    unsigned char y, i;
    
    for ( y = 0; y < 8; y++ ) {
        command(0x21);     //col addr
        command(0); //col start
        command(127);  //col end
        command(0x22);    //0x22
        command(y); // Page start
        command(y+1); // Page end    
        startBit();
        sendData(addr);            // address
        sendData(0x40);
        for (i = 0; i < 128; i++){
             sendData(0x00);
        }
        stopBit();
    }    
}

//Software I2C
void sendData(unsigned char dataB)
{
    for(unsigned char b=0;b<8;b++){
       SDA=(dataB >> (7-b)) % 2;
       clock();
    }
    TRISA2=1;   //SDA input
    clock();
    __delay_us(5);
    TRISA2=0;   //SDA output

}

void clock(void)
{
   __delay_us(1);
   SCL=1;
   __delay_us(5);
   SCL=0;
   __delay_us(1);
}

void startBit(void)
{
    SDA=0;
    __delay_us(5);
    SCL=0;

}

void stopBit(void)
{
    SCL=1;
    __delay_us(5);
    SDA=1;

}

void command( unsigned char comm){
    
    startBit();
    sendData(addr);            // address
    sendData(0x00);
    sendData(comm);             // command code
    stopBit();
}

void oled_init() {
    
    command(0xAE);   // DISPLAYOFF
    command(0x8D);         // CHARGEPUMP *
    command(0x14);     //0x14-pump on
    command(0x20);         // MEMORYMODE
    command(0x0);      //0x0=horizontal, 0x01=vertical, 0x02=page
    command(0xA1);        //SEGREMAP * A0/A1=top/bottom 
    command(0xC8);     //COMSCANDEC * C0/C8=left/right
    command(0xDA);         // SETCOMPINS *
    command(0x12);   //0x22=4rows, 0x12=8rows
    command(0x81);        // SETCONTRAST
    command(0x9F);     //0x8F
    //next settings are set by default
//    command(0xD5);  //SETDISPLAYCLOCKDIV 
//    command(0x80);  
//    command(0xA8);       // SETMULTIPLEX
//    command(0x3F);     //0x1F
//    command(0xD3);   // SETDISPLAYOFFSET
//    command(0x0);  
//    command(0x40); // SETSTARTLINE  
//    command(0xD9);       // SETPRECHARGE
//    command(0xF1);
//    command(0xDB);      // SETVCOMDETECT
//    command(0x40);
//    command(0xA4);     // DISPLAYALLON_RESUME
 //   command(0xA6);      // NORMALDISPLAY
    command(0xAF);          //DISPLAYON

}



