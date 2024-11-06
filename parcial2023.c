/*
Por un pin del ADC del microcontrolador LPC1769 ingresa una tensión de rango dinámico 0 a 3,3[v] 
proveniente de un sensor de temperatura. Debido a la baja tasa de variación de la señal, se pide tomar una 
muestra cada 30[s]. Pasados los 2[min] se debe promediar las últimas 4 muestras y en función de este valor, 
tomar una decisión sobre una salida digital de la placa:
    ● Si el valor es <1 [V] colocar la salida en 0 (0[V]).
    ● Si el valor es >= 1[V] y <=2[V] modular una señal PWM con un Ciclo de trabajo que va desde el 50% 
    hasta el 90%  proporcional al valor de tensión, con un periodo de 20[KHz]. 
    ● Si el valor es > 2[V] colocar la salida en 1 (3,3[V])

*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"  // Librería de CMSIS para la familia LPC17xx
#endif

#ifdef __USE_MCUEXPRESSO
#include <cr_section_macros.h>  // Macros específicas para MCUXpresso
#endif

#include "lpc17xx_gpio.h"   // Librería para controlar GPIO
#include "lpc17xx_pinsel.h" // Librería para la configuración de pines
#include "lpc17xx_timer.h"  // Librería para controlar el Timer0
#include "lpc17xx_adc.h"
/* Definiciones de los pines */
#define SENSOR  (uint32_t)(1<<2)    // LED en el pin P0.2 CANAL 7 ADC
#define SALIDA  (uint32_t)(1<<28)  // MODULACION DE SALIDA  en el pin P0.28 MATCH 0 , CANAL 0 timer 0
#define CONT    (uint32_t)(1<<22) // TIMER PARA CONTROLAR CADA 30S LA MUESTRA Al pin 0.22 MATCH 1, CANAL 1 timer 1

#define OUTPUT 1  // Configuración de la dirección GPIO como salida
#define INPUT  0 // Configuracion de la direccion GPIO como entrada.

#define SEGUNDO 1000000 // 1 segundo en microsegundos
#define MUESTRA 30 // Frecuencia 30 s (1  muestra cada 30 por segundo)
#define ADC_FREQ 100000 // 100 kHz

static uint32_t adc_read_value = 0; // Variable para almacenar el valor del ADC
uint32_t CONT_PERIODO = 0; // Cuando esta variable llegue a 50 uS es un periodo completo de la señal PWM
uint32_t DUTY_CYCLE = 0; // Variable para controlar el ciclo de trabajo de la señal PWM
uint32_t CANT_MUESTRAS = 0; // Variable para contar la cantidad de muestras tomadas
uint32_t MUESTRAS [4] = {0,0,0,0}; // Arreglo para almacenar las últimas 4 muestras tomadas
uint32_t SIGNAL_PERIOD = 50; // Periodo de la señal PWM (20 kHz)

void start_timer(void)
{
    TIM_Cmd(LPC_TIM0, DISABLE); /* Enable the timer  Lo voy a habilitar para que arranque a contar desde que se detecta un match*/
    TIM_Cmd(LPC_TIM1, ENABLE); /* Enable the timer */ 

    NVIC_EnableIRQ(TIMER0_IRQn); // Habilitar la interrupción global para Timer0
    NVIC_SetPriority(TIMER0_IRQn, 1); // Establecer la prioridad de la interrupción de Timer0

    NVIC_EnableIRQ(TIMER1_IRQn); // Habilitar la interrupción global para Timer1
    NVIC_SetPriority(TIMER1_IRQn, 0); // Establecer la prioridad de la interrupción de Timer1
}

void configure_adc(void) 
{

    ADC_Init(LPC_ADC, ADC_FREQ); /* Initialize the ADC peripheral with a 100 kHz sampling frequency */
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_7, ENABLE); /* Enable ADC channel 7 */
    ADC_IntConfig(LPC_ADC, ADC_CHANNEL_7, ENABLE); /* Enable interrupt for ADC channel 7  se supone que aca habilita eso*/ 
    ADC_BurstCmd(LPC_ADC, DISABLE); /* DISABLE burst mode */

    NVIC_EnableIRQ(ADC_IRQn); // Habilitar la interrupción global para el ADC
}

void GPIO_CONFIG(void)
{
    PINSEL_CFG_Type pin_cfg_struct; /* Create a variable to store the configuration of the pin */

    /* Configure GPIO */
    //SENSOR ADC
    pin_cfg_struct.Portnum = PINSEL_PORT_0;           /* Port number is 0 */
    pin_cfg_struct.Pinnum = PINSEL_PIN_2;            /* Pin number 2 */
    pin_cfg_struct.Funcnum = PINSEL_FUNC_2;           /* Function number is 2 (ADC) */
    pin_cfg_struct.Pinmode = PINSEL_PINMODE_PULLUP;   /* Pin mode is pull-up */
    pin_cfg_struct.OpenDrain = PINSEL_PINMODE_NORMAL; /* Normal mode */

    /* OUTPUT (señal) pin */
    PINSEL_ConfigPin(&pin_cfg_struct);
    pin_cfg_struct.Portnum = PINSEL_PORT_1
    pin_cfg_struct.Pinnum = PINSEL_PIN_28;            /* Pin number 28  TIMER 0*/
    pin_cfg_struct.Funcnum = PINSEL_FUNC_3;           /* Function number is 3 (MATCH) */
    pinsel_ConfigPin(&pin_cfg_struct);

    /* Configure Tiempo de muestreo */
    pin_cfg_struct.Pinnum = PINSEL_PIN_22;            /* Pin number 22 TIMER 1, canal 0*/
    pinsel_ConfigPin(&pin_cfg_struct);
   

    /* Set the LED pins as output */
    GPIO_SetDir(PINSEL_PORT_0, SENSOR , INPUT);
    GPIO_SetDir(PINSEL_PORT_1, SALIDA , OUTPUT);
    GPIO_SetDir(PINSEL_PORT_1, MUESTRA , OUTPUT); // Preguntar si es necesario ya que solo lo voy a utilizar para que avise cuando desborde.
}

void TIMER_MATCH_CONFIG(void)
{
    TIM_TIMERCFG_Type timer_cfg_struct; /* Create a variable to store the configuration of the timer */

    timer_cfg_struct.PrescaleOption = TIM_PRESCALE_TICKVALUE; /* Prescaler is in microseconds */
    timer_cfg_struct.PrescaleValue = (uint32_t) 0; /* Prescaler value is 0, giving a time resolution of ~10 ns */
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer_cfg_struct); /* Initialize Timer0 */

    timer_cfg_struct.PrescaleValue = (uint32_t) 100; /* Prescaler value is 100, giving a time resolution of ~1.01 µs */
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &timer_cfg_struct); /* Initialize Timer1 */

    // Configuracion de MATCHS

    TIM_MATCHCFG_Type match_cfg_struct;


    /*CONFIUGRACION MATCH TIMER 1 */
    match_cfg_struct.MatchChannel = 0; /* Match channel 0 */
    match_cfg_struct.IntOnMatch = ENABLE; /* Enable interrupt on match */
    match_cfg_struct.StopOnMatch = DISABLE; /* Do not stop the timer on match */
    match_cfg_struct.ResetOnMatch = ENABLE; /* Reset the timer on match */
    match_cfg_struct.ExtMatchOutputType = TIM_EXTMATCH_NOTHING; /* No external match output */
    match_cfg_struct.MatchValue = (uint32_t)(MUESTRA * SEGUNDO); /* Match value set for 30 seconds */
    TIM_ConfigMatch(LPC_TIM1, &match_cfg_struct); /* Configure the match */

    /*Configuracion  MATCH TIMER0  PENSAR */
     /* Create a variable to store the configuration of the match */
    match_cfg_struct.MatchChannel = 0; /* Match channel 0 */
    match_cfg_struct.IntOnMatch = ENABLE; /* Enable interrupt on match */
    match_cfg_struct.StopOnMatch = DISABLE; /* Do not stop the timer on match */
    match_cfg_struct.ResetOnMatch = ENABLE; /* Reset the timer on match */
    match_cfg_struct.ExtMatchOutputType = TIM_EXTMATCH_NOTHING; /* No external match output */
    match_cfg_struct.MatchValue = SIGNAL_PERIOD ; /*    // representa el 1% de la señal (500nS)  Periodo 20 Khz --> 50uS (100%) */  

    TIM_ConfigMatch(LPC_TIM0, &match_cfg_struct); /* Configure the match */
}

/* INTERRUPT HANDLER FUNCTIONS */

void TIMER0_IRQHandler(void)
{
    // Comprobar y limpiar la interrupción por coincidencia para cada canal

    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) // Si se produjo una coincidencia en el canal 0
    {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT); // Limpiar la bandera de interrupción
        CONT_PERIODO++; // Incrementar el contador de periodo de la señal, si llega a 10 es un periodo completo
        if (CONT_PERIODO == 10) // Si se completó un periodo de la señal
        {
            CONT_PERIODO = 0; // Reiniciar el contador de periodo
        }
    }
    
}


void TIMER1_IRQHandler(void)
{
    // Comprobar y limpiar la interrupción por coincidencia para cada canal
    

    if (TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT)) // Si se produjo una coincidencia en el canal 0
    {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT); // Limpiar la bandera de interrupción
         ADC_StartCmd(LPC_ADC, ADC_START_NOW); /* Aca empieza a convertir */
    }
    
}

void ADC_IRQHandler() // Se ejecuta cuando se completa la conversión
{
    //Falto limpiar bandera
    uint32_t promedio = 0; // Variable para almacenar el promedio de las últimas 4 muestras
    adc_read_value = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_7); /* Read the ADC value 0-3.3 V  -> 0-4095*/
    bool READY = false; // Variable para indicar si el promedio está listo para ser procesado

    if (cant_muestras < 4)
    {
        MUESTRAS[CANT_MUESTRAS] = adc_read_value; // Almacenar la muestra en el arreglo
        CANT_MUESTRAS++; // Incrementar la cantidad de muestras tomadas
        READY = false;
    }
    else
    {
        for (int i = 0 ; i < 4 ; i++)
        {
           promedio += MUESTRAS[i]; // Sumar las últimas 4 muestras
        }
        promedio = promedio / 4; // Calcular el promedio de las últimas 4 muestras
        READY = true; 
        cant_muestras = 0; // Reiniciar la cantidad de muestras tomadas
    }

    if (READY)
    {
        uint32_t voltage = promedio * 3.3 / 4095; // Convertir el valor del ADC a voltaje (0-3.3 V)
        if (voltage < 1)
        {
            GPIO_ClearValue(PINSEL_PORT_1, SALIDA); // Colocar la salida en 0
        }
        else if (voltage >= 1 && voltage <= 2)
        {
            PWM_function(voltage); // Modulación de la señal PWM
        }
        else
        {
            GPIO_SetValue(PINSEL_PORT_1, SALIDA); // Colocar la salida en 1
        }
    }
    

  
    

}

void PWM_function (int voltage){
    uint32_t duty_cycle_time = 50 + (voltage - 1) * 40; // Calculo del duty EC. recta y2-y1 = m(x2-x1) + b 
    uint32_t on_time = (duty_cycle_time * SIGNAL_PERIOD) / 100;    // Tiempo en microsegundos que la señal estará en HIGH (activo)
    uint32_t off_time = SIGNAL_PERIOD - on_time; 
    TIM_Cmd(LPC_TIM0, ENABLE); // Desde aca empieza a contar.
    
    while (CONT_PERIODO <= 10){ // 10 veces de interrupcion del TIMER0 equivale a un periodo completo de la señal.
        if(CONT_PERIODO <= on_time){
            GPIO_SetValue(PINSEL_PORT_1, SALIDA); // Encender la señal
        }
        else{
            GPIO_ClearValue(PINSEL_PORT_1, SALIDA); // Apagar la señal
        }
        
    }
    TIM_Cmd(LPC_TIM0, DISABLE); // Lo apago para tener control absoluto de la señal.


}











int main (){
    SystemInit();
    GPIO_CONFIG();
    TIMER_MATCH_CONFIG();
    start_timer();

    while (true){



    }




    return 0;
}
