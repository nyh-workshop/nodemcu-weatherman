unsigned char intrHappened = 0;

void intr() iv 0x0004 ics ICS_AUTO {
     INTCON.INTF = 0;
     intrHappened = 1;
}

void init() {
     // 31kHz LF.
     // internal oscillator block.
     // RA4 is input, pulled up.
     OSCCON = 0b00000010;
     TRISA = 0b00000100;
     TRISC = 0x00;
     ANSELA = 0x00;
     WPUA = 0b00000100;
}

void initIntr() {
     OPTION_REG.INTEDG = 0;
     INTCON = 0x00;
     INTCON.INTE = 1;
     INTCON.GIE = 1;
}

void main() {
     init();
     initIntr();
     LATA.RA5 = 1;
     
     while(1) {
         asm sleep;
         if(intrHappened) {
            INTCON.GIE = 0;
            intrHappened = 0;
            LATA.RA5 = 0;
            Delay_ms(500);
            LATA.RA5 = 1;
            
            Delay_ms(100);
            
            LATA.RA5 = 0;
            Delay_ms(500);
            LATA.RA5 = 1;
            
            Delay_ms(5000);
            
            INTCON.GIE = 1;
         }
     }
}