/*Un estacionamiento automatizado utiliza una barrera que se abre y cierra en función de la validación de un ticket de acceso
utilizando una LPC1769 Rev. D trabajando a una frecuencia de CCLK a 70 [MHz]
Cuando el sistema detecta que un automóvil se ha posicionado frente a la barrera, se debe activar un sensor conectado
al pin P2[10] mediante una interrupción externa (EINT). Una vez validado el ticket, el sistema activa un motor que abre
la barrera usando el pin P0[15]. El motor debe estar activado por X segundos y luego apagarse, utilizando el temporizador Systick
para contar el tiempo. Si el ticket es inválido, se encenderá un LED rojo conectado al pin P1[5].
Para gestionar el tiempo de apertura de la barrera, existe un switch conectado al pin P3[4] que dispone de una ventana
de configuración de 3 segundos gestionada por el temporizador Systick.
Durante dicha ventana, se debe contar cuantas veces se presiona el switch y en función de dicha cantidad, establecer el
tiempo de la barrera. */


#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#ifdef __USE_MCUEXPRESSO
#include <cr_section_macros.h>
#endif

#include <stdbool.h> //Para poder usar variables booleanas en C , de manera mas visual puesto que sino tengo que hacer un casteo o interpretación con 0 y 1

#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_systick.h"

/*PIN Definitions */
#define SENSOR     ((uint32_t)(1<<10)) // PUERTO 2 PIN 10
#define MOTOR      ((uint32_t)(1<<15)) // PUERTO 0 PIN 15
#define LED_ROJO 5 ((uint32_t)(1<<5) ) //  PUERTO 1 PIN 5
#define SWITCH 4   ((uint32_t)(1<<4) ) //  PUERTO 3 PIN 4

/*Variables globales */

#define TRUE 1   //Defino un valor booleano para TRUE
#define FALSE 0  //Defino un valor booleano para FALSE
#define INPUT 0  //Defino un valor para INPUT
#define OUTPUT 1 //Defino un valor para OUTPUT

#define DEFAULT_TIME 300; //Valor por default para el tiempo de la ventana de configuración
#define SYSTICK_TIME 10  // 10 mS la funcion recibe valores en mS
uint32_t  TIEMPO_VENTANA_CFG 300; //(3000mS) 300x10mS = 3s 
uint32_t contador = 0; //Variable que tiene que llegar a 300 para que se cumpla la ventana de tiempo
uint32_t contador_switch = 0; //Variable que cuenta la cantidad de veces que se presiona el switch
uint32_t Valores[4] = {5000, 1000, 2000, 3000}; //Valores posibles para el switch 
/*Configuración GPIO */


void GPIO_Config(void)
{
    PINSEL_CFG_Type pin_cfg_struct; /* Create a variable to store the configuration of the pin */

    /* SENSOR    */
    pin_cfg_struct.Portnum = PINSEL_PORT_2;           /* The port number is 0 */
    pin_cfg_struct.Pinnum = PINSEL_PIN_22;            /* The pin number is 22 */
    pin_cfg_struct.Funcnum = PINSEL_FUNC_1;           /* The function number is 1 EINT0 */
    pin_cfg_struct.Pinmode = PINSEL_PINMODE_PULLUP;   /* The pin mode is pull-up */
    pin_cfg_struct.OpenDrain = PINSEL_PINMODE_NORMAL; /* The pin is in the normal mode */

    /* Configure the pin */
    PINSEL_ConfigPin(&pin_cfg_struct);

    //Configuración del pin de salida para el motor
    pin_cfg_struct.Portnum = PINSEL_PORT_0;           /* The port number is 0 */
    pin_cfg_struct.Pinnum = PINSEL_PIN_15;            /* The pin number is 15 */
    pin_cfg_struct.Funcnum = PINSEL_FUNC_0;           /* The function number is 0 GPIO */

    PINSEL_ConfigPin(&pin_cfg_struct);

    //Configuración del pin de salida para el led rojo
    pin_cfg_struct.Portnum = PINSEL_PORT_1;           /* The port number is 1 */
    pin_cfg_struct.Pinnum = PINSEL_PIN_5;             /* The pin number is 5 */
    pin_cfg_struct.Funcnum = PINSEL_FUNC_0;           /* The function number is 0 GPIO */

    PINSEL_ConfigPin(&pin_cfg_struct);

    //Configuración del pin de entrada para el switch
    pin_cfg_struct.Portnum = PINSEL_PORT_3;           /* The port number is 3 */
    pin_cfg_struct.Pinnum = PINSEL_PIN_4;             /* The pin number is 4 */
    pin_cfg_struct.Funcnum = PINSEL_FUNC_0;           /* The function number is 0 GPIO */

    PINSEL_ConfigPin(&pin_cfg_struct);


    /* Set the pins as input or output */
    GPIO_SetDir(PINSEL_PORT_2, SENSOR,    INPUT); /* Set the P0.22 pin as output */
    GPIO_SetDir(PINSEL_PORT_0, MOTOR_PIN, OUTPUT); /* Set the P0.15 pin as output */
    GPIO_SetDir(PINSEL_PORT_1, LED_PIN,   OUTPUT); /* Set the P1.5 pin as output */
    GPIO_SetDir(PINSEL_PORT_3, SWITCH_PIN, INPUT); /* Set the P3.4 pin as input */

    NVIC_EnableIRQ(EINT0_IRQn); /* Enable the EINT0 interrupt */
    NVIC_SetPriority(EINT0_IRQn, 0); /* Set the priority of the EINT0 interrupt to 0 */
}

void configure_eint (void) {
    EXTI_InitTypeDef exti_cfg;
	exti_cfg.EXTI_Line = EXTI_EINT0;
	exti_cfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
	exti_cfg.EXTI_polarity = EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;
	EXTI_Config(&exti_cfg);
}


void configure_systick (void){
    SYSTICK_InternalInit(SYSTICK_TIME); //Inicializo el systick con un tiempo de 10mS
    SYSTICK_IntCmd(ENABLE); //Habilito la interrupción del systick
}


void EINT0_Handler (void){
   
    EXTI_ClearEXTIFlag(EXTI_EINT0);

    ticket (); //Llamo a la función ticket
    
    /*Se supone que un auto aca se paro */
    



}

void Ticket (void){
    uint32_t ticket_valido = FALSE //Pongo por default el ticket en falso

    if (ticket_valido == TRUE){
        GPIO_SetValue(PINSEL_PORT_0, MOTOR_PIN); //Enciendo el motor
        SYSTICK_Cmd(ENABLE); //Habilito el systick
        while (contador < TIEMPO_VENTANA_CFG){
            if (GPIO_ReadValue(PINSEL_PORT_3) & SWITCH_PIN){ //Si el switch esta presionado
                contador_switch++; //Incremento el contador del switch
            }
        }
        contador_switch = 0; //Reseteo el contador del switch

         TIEMPO_VENTANA_CFG = Valores[contador_switch]; //Establezco el tiempo de la barrera en función de la cantidad de veces que se presiono el switch

        while (contador < TIEMPO_VENTANA_CFG){
            _nop (); //No hago nada
        }

        TIEMPO_VENTANA_CFG = DEFAULT_TIME; //Vuelvo al valor por default
        GPIO_ClearValue(PINSEL_PORT_0, MOTOR_PIN); //Apago el motor
    }
    else{
        GPIO_SetValue(PINSEL_PORT_1, LED_PIN); //Enciendo el led rojo
    }
}

void Systick_Handler (void){
    SYSTICK_ClearCounterFlag(); //Limpio la bandera de interrupción del systick
    static uint32_t tiempo = 0; //Variable para contar el tiempo

    
    
    if (tiempo < DEFAULT_TIME){
        tiempo++;
    }
    else{
        tiempo = 0;
        SYSTICK_Cmd(DISABLE); //Deshabilito el systick
        GPIO_ClearValue(PINSEL_PORT_0, MOTOR_PIN); //Apago el motor
    }

}




int main (void){

    SystemInit();
    GPIO_Config();
    configure_eint();
    config_systick();


    while (TRUE) {
        __WFI();
    }
}



return 0;

}