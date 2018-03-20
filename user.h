/******************************************************************************
 * Deve-se incluir nesse arquivo as bibliotecas e as definicoes de projeto 
 * necessarias a mais de um arquivo a ser utilizado no projeto. 
 ******************************************************************************/

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef USER_H
#define	USER_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdio.h> //bibliteca com as funcoes de entrada e saida padrao: printf())
#include <stdlib.h>		//Biblioteca Microchip C18 com funções conversão de dados: itoa()
#include <math.h>		//Biblioteca Microchip C18 com funções matemáticas: fabs()

#define _BOOTLOADER //Comente essa linha para usar o codigo no ISIS

//Indicacao do clock utilizado pelo uC para o compilador
#ifdef _BOOTLOADER  
    #define FCY			16e6		// Fclk = 32 MHz (configurada bo bootloader)
#else
    #define FCY			2e6		// Fclk = 4MHz (usar 4 MHz na simulação do ISIS)
#endif

#include <libpic30.h> //necessaria para utilizar-se as funcoes de delay

//Os bits de configuracao de clock nao permanecem nesse arquivo pois
//o compilador executa a compilacao de bibliotecas que usam a user.h
//separadamente e estariam redefinindo as configuracoes a cada biblioteca
//compilada. Essa operacao e considerada ilegal e os bits de configuracao
//foram transportados para o arquivo da funcao "main", para serem utilizados
//quando o firmware e compilado para ser simulado no ISIS.


#endif	/* USER_H */

