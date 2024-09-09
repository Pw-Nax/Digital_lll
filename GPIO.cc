/*
 * @file main.c
 * @brief Este proyecto demuestra cómo configurar los pines GPIO en la LPC1769 usando CMSIS.
 * Los pines GPIO se configuran como salida y entrada, y el LED conectado a P0.22 se alterna
 * en función del estado del pin de entrada P0.0.
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#ifdef __USE_MCUEXPRESSO
#include <cr_section_macros.h> /* Macros específicos de MCUXpresso */
#endif

#include "lpc17xx_gpio.h"   /* Manejo de GPIO */
#include "lpc17xx_pinsel.h" /* Selección de funciones del pin */

/* Definiciones de pines */
#define LED_PIN   ((uint32_t)(1 << 22)) /* P0.22 conectado al LED */
#define INPUT_PIN ((uint32_t)(1 << 0))  /* P0.0 conectado a la entrada */

/* Definiciones de la dirección GPIO */
#define INPUT  0  /* Entrada */
#define OUTPUT 1  /* Salida */

/* Valores booleanos */
#define TRUE  1   /* Verdadero */
#define FALSE 0   /* Falso */

/**
 * @brief Inicializar el periférico GPIO
 *
 */
void configure_port(void)
{
    PINSEL_CFG_Type led_pin_cfg; /* Crear una variable para almacenar la configuración del pin */

    /* Necesitamos configurar la estructura con la configuración deseada */
    led_pin_cfg.Portnum = PINSEL_PORT_0;           /* El número de puerto es 0 */
    led_pin_cfg.Pinnum = PINSEL_PIN_22;            /* El número de pin es 22 */
    led_pin_cfg.Funcnum = PINSEL_FUNC_0;           /* El número de función es 0 */
    led_pin_cfg.Pinmode = PINSEL_PINMODE_PULLUP;   /* El modo del pin es pull-up */
    led_pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL; /* El pin está en modo normal */

    /* Configurar el pin */
    PINSEL_ConfigPin(&led_pin_cfg);

    PINSEL_CFG_Type input_pin_cfg; /* Crear una variable para almacenar la configuración del pin */

    /* Necesitamos configurar la estructura con la configuración deseada */
    input_pin_cfg.Portnum = PINSEL_PORT_0;           /* El número de puerto es 0 */
    input_pin_cfg.Pinnum = PINSEL_PIN_0;             /* El número de pin es 0 */
    input_pin_cfg.Funcnum = PINSEL_FUNC_0;           /* El número de función es 0 */
    input_pin_cfg.Pinmode = PINSEL_PINMODE_PULLUP;   /* El modo del pin es pull-up */
    input_pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL; /* El pin está en modo normal */

    /**
     * ¡Consejo útil!
     * Podemos reutilizar la misma variable para configurar los pines, solo cambiamos los valores de la estructura.
     * PINSEL_CFG_Type pin_cfg = {
     * .Portnum = PINSEL_PORT_0,
     * .Pinnum = PINSEL_PIN_22,
     * .Funcnum = PINSEL_FUNC_0,
     * .Pinmode = PINSEL_PINMODE_PULLUP,
     * .OpenDrain = PINSEL_PINMODE_NORMAL
     * };
     * PINSEL_ConfigPin(&pin_cfg);
     * pin_cfg.Pinnum = PINSEL_PIN_0;
     * pin_cfg.Funcnum = PINSEL_FUNC_1;
     * PINSEL_ConfigPin(&pin_cfg);
     */

    /* Configurar los pines */
    PINSEL_ConfigPin(&led_pin_cfg);
    PINSEL_ConfigPin(&input_pin_cfg);

    /* Establecer los pines como entrada o salida */
    GPIO_SetDir(PINSEL_PORT_0, LED_PIN, OUTPUT);  /* Configurar el pin P0.22 como salida */
    GPIO_SetDir(PINSEL_PORT_0, INPUT_PIN, INPUT); /* Configurar el pin P0.0 como entrada */
}

/**
 * @brief Función principal.
 * Inicializa el sistema y alterna el LED en función del estado del pin de entrada.
 */
int main(void)
{
    SystemInit(); /* Inicializar el reloj del sistema (por defecto: 100 MHz) */

    configure_port(); /* Configurar los pines GPIO */

    while (TRUE)
    {
        /* Bucle de retardo simple */
        for (volatile int i = 0; i < 1000000; i++) /* Usar volatile para evitar la optimización */
        {
            __asm("nop"); /* No realizar ninguna operación */
        }

        /* Alternar el LED en función del estado del pin de entrada */
        if (GPIO_ReadValue(PINSEL_PORT_0) & INPUT_PIN)
        {
            GPIO_SetValue(PINSEL_PORT_0, LED_PIN); /* Encender el LED */
        }
        else
        {
            GPIO_ClearValue(PINSEL_PORT_0, LED_PIN); /* Apagar el LED */
        }
    }

    return 0; /* El programa nunca debería llegar a este punto */
}
