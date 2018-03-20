#include "user.h"

#ifndef _BOOTLOADER  
// CONFIG2
#pragma config POSCMOD = XT     // XT Oscillator mode selected
#pragma config OSCIOFNC = ON    // OSC2/CLKO/RC15 as as port I/O (RC15)
#pragma config FCKSM = CSDCMD   // Clock Switching and Monitor disabled
#pragma config FNOSC = PRI      // Primary Oscillator (XT, HS, EC)
#pragma config IESO = ON        // Int Ext Switch Over Mode enabled

// CONFIG1
#pragma config WDTPS = PS32768  // Watchdog Timer Postscaler (1:32,768)
#pragma config FWPSA = PR128    // WDT Prescaler (1:128)
#pragma config WINDIS = ON      // Watchdog Timer Window Mode disabled
#pragma config FWDTEN = OFF     // Watchdog Timer disabled
#pragma config ICS = PGx2       // Emulator/debugger uses EMUC2/EMUD2
#pragma config GWRP = OFF       // Writes to program memory allowed
#pragma config GCP = OFF        // Code protection is disabled
#pragma config JTAGEN = OFF     // JTAG port is disabled

#endif

#ifdef _BOOTLOADER  
    //Gravação no módulo físico
    #define PS3  1
    #define PS1  2
    #define Tad  1
    #define Ts   6
#else
    //Simulação no Proteus
    #define PS3  0
    #define PS1  1
    #define Tad  0
    #define Ts   2
#endif

#define NSENS    8
#define PADRAOP  0xFF

#define BUZZER_ON   OC1CON1bits.OCM = 0b101
#define BUZZER_OFF  OC1CON1bits.OCM = 0

enum ESTADOS {RETIRADA_DADOS = 1, 
             DETERMINACAO_ACIONAMENTO, 
             VERIFICACAO, 
             ALERTA};
             
volatile unsigned int DADOS[NSENS][60], SENSOR, IND_INSER = 0, NITENS = 0, *ADC16Ptr, BIP = 0, ESTADO = 0;
char MODO = 0;

int main(void) {
   
    unsigned int IND_RET = 0, PRIMEIRO[NSENS], AUX = 0, SENS_ATV = 0, SENS_DES = 0, PADRAO = 0;
    
    RPOR1bits.RP2R = 18;        //Pino RP2(RD8) como terminal do módulo Compare
    TRISDbits.TRISD8 = 0;
    RPINR18bits.U1RXR = 10;     //RX
    
    //Entrada ADC
    AD1PCFG = 0;
    AD1CON2bits.VCFG = 0;       //AVdd e AVss
    AD1CHSbits.CH0NA = 0;       //Vr-
    AD1CSSL = 0b110100111110;
    
    //Saída ADC
    AD1CON1bits.FORM = 0;       //Dados de saída no formato inteiro
    AD1CON2bits.BUFM = 1;       //Buffer de 8 words
    
    //Operação ADC
    AD1CON1bits.ASAM = 1;       //Amostragem automática
    AD1CON1bits.SSRC = 2;       //Timer 3 gera conversão
    AD1CON2bits.CSCNA = 1;      //Habilita scanning
    AD1CON2bits.SMPI = NSENS-1; //Interrompe ao preencher 3 buffers 
    AD1CON2bits.ALTS = 0;       //Sempre usar o MUXA
    
    //Temporização ADC
    AD1CON3bits.ADRC = 0;       //Clock derivado do sistema de clock
    AD1CON3bits.ADCS = Tad;
    AD1CON3bits.SAMC = Ts;      //Tempo de amostragem
    
    //Interrupção
    INTCON1bits.NSTDIS = 0;
    INTCON2bits.ALTIVT = 0;
    IFS0bits.AD1IF = 0;
    IPC3bits.AD1IP = 6;
    
    //Timer para ADC
    T3CON = 0;
    T3CONbits.TCKPS = PS3;
    PR3 = 50000;
    TMR3 = 0;
    
    //Compare Output
    T1CON = 0;
    T1CONbits.TCKPS = PS1;
    PR1 = 37499;                    //Ciclo de 150ms
    TMR1 = 0;
    
    OC1CON1 = 0;
    OC1CON2 = 0;
    OC1R = 12499;                   //Duty cycle de 100ms
    OC1RS = 37499;                  //Ciclo de 150ms
    OC1CON1bits.OCTSEL = 0b100;     //Compara com o Timer 1
    OC1CON2bits.SYNCSEL = 0b01011;  //TMR1 como fonte de sincronismo
    
    //UART
    U1MODE = 0;
    U1STA = 0;
    U1MODEbits.UEN = 0;         //U1TX e U1RX são os pinos usados
    U1MODEbits.RXINV = 0;       //Idle state é '1'
    U1MODEbits.BRGH = 1;        //Configura high speed (Fcy/4)
    U1MODEbits.PDSEL = 0;       //Sem bit de paridade 
    U1MODEbits.STSEL = 0;       //1 bit de stop
    U1STAbits.URXISEL = 0;      //Interrupção de recepção de dados após 1 dado recebido
    U1STAbits.UTXISEL0 = 0;     //Interrupção do TX após cada transmissão
    U1STAbits.UTXISEL1 = 0;     //Interrupção do TX após cada transmissão
    U1BRG = 415;                //U1BRG = [16M/(4*9600)]-1 
    
    //Interrupção UART
    IFS0bits.U1RXIF = 0;
    IPC2bits.U1RXIP = 7;
    
    //Enable
    AD1CON1bits.ADON = 0;
    IEC0bits.AD1IE = 1;
    T3CONbits.TON = 1;
    IEC0bits.OC1IE = 1;
    T1CONbits.TON = 1;
    BUZZER_OFF;
    IEC0bits.U1RXIE = 1;
    U1MODEbits.UARTEN = 1;
    
    while (1)
    {  
        switch (MODO) {
            case 'a': //Modo 1
                MODO = 1;
                AD1CON1bits.ADON = 1;
                ESTADO = RETIRADA_DADOS;
                break;
            case 'b':
                MODO = 2; //Modo 2
                AD1CON1bits.ADON = 1;
                ESTADO = RETIRADA_DADOS;
                break;
            case 'c': //Nenhum
                AD1CON1bits.ADON = 0;
                break;
            default:
                break;
        }
        
        switch (ESTADO) {
            
            //Realiza a cópia dos dados capturados no buffer circular
            case RETIRADA_DADOS:
                if (NITENS > 0) {
                    for (AUX = 0; AUX < NSENS; AUX++) {
                        PRIMEIRO[AUX] = DADOS[AUX][IND_RET];
                    }
                    IND_RET++; NITENS--;
                    if (NITENS < 0) {NITENS = 0;}           //Garante que o número de itens não seja negativo                
                    if (IND_RET > 60) {IND_RET = 0;}
                    ESTADO = DETERMINACAO_ACIONAMENTO;
                }
                break;

            //Atribui pesos para cada sensor acionado
            case DETERMINACAO_ACIONAMENTO:
                for (AUX = 0; AUX < NSENS; AUX++) {
                    if (PRIMEIRO[AUX] > 155) {              //155 = 500mV
                        switch (AUX) {
                            case 0:
                                SENS_ATV |= 0b1000;
                                break;
                            case 1:
                                SENS_ATV |= 0b10000;
                                break;
                            case 2:
                                SENS_ATV |= 0b100000;
                                break;
                            case 3:
                                SENS_ATV |= 0b1000000;
                                break;
                            case 4:
                                SENS_ATV |= 0b10000000;
                                break;
                            case 5:
                                SENS_ATV |= 0b10;
                                break;
                            case 6:
                                SENS_ATV |= 0b1;
                                break;
                            case 7:
                                SENS_ATV |= 0b100;
                                break;
                            default:
                                break;
                        }
                    }
                    
                    else {
                        SENS_DES++;                      //Incrementa quando algum sensor não é acionado
                    }
                }//end\For
                ESTADO = VERIFICACAO;
                break;
              
            //Verifica se o usuário retirou o pé do chão
            case VERIFICACAO:     
                if (SENS_DES == NSENS && SENS_ATV != 0) {
                    PADRAO = SENS_ATV;
                    SENS_ATV = 0;
                    SENS_DES = 0;
                    ESTADO = ALERTA;
                }
                else {
                    SENS_DES = 0;
                    ESTADO = RETIRADA_DADOS;
                }
                break;
            
            //Emite o alerta, caso a condição não seja satisfeita
            case ALERTA:
                if (PADRAO == PADRAOP && MODO == 1) {
                    PADRAO = 0;
                    BUZZER_ON;
                }
                else if (PADRAO != PADRAOP && MODO == 2) {
                    PADRAO = PADRAOP;
                    BUZZER_ON;
                }
                ESTADO = RETIRADA_DADOS;
            default:
                break;
        }
    }

    return -1;
}

//Realiza a aquisição de dados no buffer circular
void __attribute__((interrupt,no_auto_psv)) _ADC1Interrupt(void){
    if(IFS0bits.AD1IF){
        IFS0bits.AD1IF = 0;
        if(AD1CON2bits.BUFS){
            ADC16Ptr = &ADC1BUF0;
            for (SENSOR = 0; SENSOR < NSENS; SENSOR++) {
                DADOS[SENSOR][IND_INSER] = *ADC16Ptr;
                ADC16Ptr++;
            }
        }
        else {
            ADC16Ptr = &ADC1BUF8;
            for (SENSOR = 0; SENSOR < NSENS; SENSOR++) {
                DADOS[SENSOR][IND_INSER] = *ADC16Ptr;
                ADC16Ptr++;
            }
        }
        IND_INSER++; NITENS++;
        if (IND_INSER > 60) {IND_INSER = 0;}
    }
}

//Gera 3 pulsos
void __attribute__((interrupt,no_auto_psv)) _OC1Interrupt(void) {
    if (IFS0bits.OC1IF) {
        IFS0bits.OC1IF = 0;
        if (BIP < 2) {
            BIP++;
        }
        else {
            BIP = 0;
            BUZZER_OFF;
        }
    } 
}

void __attribute__((interrupt,no_auto_psv)) _U1RXInterrupt(void){
    if(IFS0bits.U1RXIF){    
        IFS0bits.U1RXIF = 0;
		MODO = U1RXREG;
    }
}