### Código Completo extraído de : https://github.com/scottellis/lpc17xx.cmsis.driver.library/blob/master/Examples/DAC/WaveGenerate/dac_wave_generate.c

Se agrego la opción de Dientes de Sierra.



```c
/**********************************************************************
* $Id$		dac_wave_generate.c				2010-07-16
*//**
* @file		dac_wave_generate.c
* @brief	Este ejemplo describe cómo usar el DAC para generar una onda
* 			sinusoidal, triangular, escalera o diente de sierra.
* @version	1.0
* @date		16. July. 2010
* @author	NXP MCU SW Application Team
*
* Copyright(C) 2010, NXP Semiconductor
* All rights reserved.
**********************************************************************/
#include "lpc17xx_dac.h"
#include "lpc17xx_libcfg.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "debug_frmwrk.h"

/* Configuraciones de DMA */
#define DMA_SIZE_SINE		60
#define NUM_SAMPLE_SINE		60
#define DMA_SIZE			64
#define NUM_SAMPLE			64

/* Parámetros de la señal */
#define SIGNAL_FREQ_IN_HZ	60
#define PCLK_DAC_IN_MHZ	25

/* Definiciones de las opciones de señal */
#define DAC_GENERATE_SINE		1
#define DAC_GENERATE_TRIANGLE	2
#define DAC_GENERATE_ESCALATOR	3
#define DAC_GENERATE_SAWTOOTH	4 // Nueva opción para diente de sierra
#define DAC_GENERATE_NONE		0

/* Variables privadas */
uint8_t menu1[] =
"********************************************************************************\n\r"
"Hello NXP Semiconductors \n\r"
" DAC generate signals demo \n\r"
"\t - MCU: LPC17xx \n\r"
"\t - Core: ARM Cortex-M3 \n\r"
"\t - Communicate via: UART0 - 115200 kbps \n\r"
" Use DAC to generate sine, triangle, escalator wave, frequency adjustable\n\r"
" Signal samples are transmitted to DAC by DMA memory to peripheral\n\r"
"********************************************************************************\n\r";

/* Prototipos de funciones privadas */
void print_menu(void);

/*********************************************************************//**
 * @brief		Imprime el menú de opciones
 * @param[in]	None
 * @return 		None
 **********************************************************************/
void print_menu(void)
{
	_DBG(menu1);
}

/*********************************************************************//**
 * @brief		Punto de entrada principal del programa DAC
 * @param[in]	None
 * @return 		int
 **********************************************************************/
int c_entry(void)
{
	PINSEL_CFG_Type PinCfg;
	DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;
	GPDMA_Channel_CFG_Type GPDMACfg;
	GPDMA_LLI_Type DMA_LLI_Struct;
	uint32_t tmp;
	uint8_t i,option;
	uint32_t sin_0_to_90_16_samples[16]={\
			0,1045,2079,3090,4067,\
			5000,5877,6691,7431,8090,\
			8660,9135,9510,9781,9945,10000\
	};
	uint32_t dac_lut[NUM_SAMPLE];

	/* Inicialización de pin DAC (P0.26) */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 26;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);

	/* Inicialización de debug (UART0) */
	debug_frmwrk_init();

	// Imprimir pantalla de bienvenida
	print_menu();

	while(1)
	{
		// Seleccionar opción
		_DBG("Please choose signal type:\n\r");
		_DBG("\t1) Sine wave.\n\r");
		_DBG("\t2) Triangle wave.\n\r");
		_DBG("\t3) Escalator wave.\n\r");
		_DBG("\t4) Sawtooth wave.\n\r"); // Nueva opción
		option = DAC_GENERATE_NONE;
		while(option == DAC_GENERATE_NONE)
		{
			switch(_DG)
			{
			case '1': option = DAC_GENERATE_SINE; break;
			case '2': option = DAC_GENERATE_TRIANGLE; break;
			case '3': option = DAC_GENERATE_ESCALATOR; break;
			case '4': option = DAC_GENERATE_SAWTOOTH; break; // Nueva opción
			default:
				_DBG("Wrong choice! Please select 1 or 2 or 3 or 4 only!\n\r");
				option = DAC_GENERATE_NONE;
				break;
			}
		}

		// Preparar tabla de búsqueda (look up table) del DAC
		switch(option)
		{
		case DAC_GENERATE_SINE:
			for(i=0;i<NUM_SAMPLE_SINE;i++)
			{
				if(i<=15) // Cuadrante 1 (0-90 grados)
				{
					dac_lut[i] = 512 + 512*sin_0_to_90_16_samples[i]/10000;
					if(i==15) dac_lut[i]= 1023;
				}
				else if(i<=30) // Cuadrante 2 (90-180 grados)
				{
					dac_lut[i] = 512 + 512*sin_0_to_90_16_samples[30-i]/10000;
				}
				else if(i<=45) // Cuadrante 3 (180-270 grados)
				{
					dac_lut[i] = 512 - 512*sin_0_to_90_16_samples[i-30]/10000;
				}
				else // Cuadrante 4 (270-360 grados)
				{
					dac_lut[i] = 512 - 512*sin_0_to_90_16_samples[60-i]/10000;
				}
				dac_lut[i] = (dac_lut[i]<<6); // Desplazamiento de bits para el registro DAC
			}
			break;
		case DAC_GENERATE_TRIANGLE:
			for(i=0;i<NUM_SAMPLE;i++)
			{
				if(i<32) dac_lut[i]= 32*i; // Rampa ascendente (de 0 a ~1023)
				else if (i==32) dac_lut[i]= 1023; // Valor pico (para 10 bits)
				else dac_lut[i] = 32*(NUM_SAMPLE-i); // Rampa descendente (de ~1023 a 0)
				dac_lut[i] = (dac_lut[i]<<6);
			}
			break;
		case DAC_GENERATE_ESCALATOR:
			for(i=0;i<NUM_SAMPLE;i++)
			{
				// La división entera (i/16) crea pasos discretos.
				// (1023/3) = 341. Los pasos son: 0, 341, 682, 1023.
				dac_lut[i] = (1023/3)*(i/16);
				dac_lut[i] = (dac_lut[i]<<6);
			}
			break;
		case DAC_GENERATE_SAWTOOTH: // Lógica para la onda de diente de sierra (rampa ascendente)
			for(i=0;i<NUM_SAMPLE;i++)
			{
				dac_lut[i] = (1023 * i) / NUM_SAMPLE;
				dac_lut[i] = (dac_lut[i]<<6);
			}
			break;
		default: break;
		}

		// Configuración del DMA (Lista de Enlace Enlazada LLI)
		DMA_LLI_Struct.SrcAddr= (uint32_t)dac_lut;
		DMA_LLI_Struct.DstAddr= (uint32_t)&(LPC_DAC->DACR);
		DMA_LLI_Struct.NextLLI= (uint32_t)&DMA_LLI_Struct;
		DMA_LLI_Struct.Control= ((option==DAC_GENERATE_SINE)?DMA_SIZE_SINE:DMA_SIZE)
								| (2<<18) // Ancho de fuente 32 bits
								| (2<<21) // Ancho de destino 32 bits
								| (1<<26) // Incremento de fuente
								;

		/* Inicialización del controlador GPDMA */
		GPDMA_Init();

		// Configuración del canal DMA 0
		GPDMACfg.ChannelNum = 0;
		GPDMACfg.SrcMemAddr = (uint32_t)(dac_lut);
		GPDMACfg.DstMemAddr = 0;
		GPDMACfg.TransferSize = ((option==DAC_GENERATE_SINE)?DMA_SIZE_SINE:DMA_SIZE);
		GPDMACfg.TransferWidth = 0;
		GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
		GPDMACfg.SrcConn = 0;
		GPDMACfg.DstConn = GPDMA_CONN_DAC;
		GPDMACfg.DMALLI = (uint32_t)&DMA_LLI_Struct;
		GPDMA_Setup(&GPDMACfg);

		DAC_ConverterConfigStruct.CNT_ENA =SET;
		DAC_ConverterConfigStruct.DMA_ENA = SET;
		DAC_Init(LPC_DAC);

		/* Calcular y configurar el tiempo de espera (timeout) del DAC para la frecuencia deseada */
		tmp = (PCLK_DAC_IN_MHZ*1000000)/(SIGNAL_FREQ_IN_HZ*((option==DAC_GENERATE_SINE)?NUM_SAMPLE_SINE:NUM_SAMPLE));
		DAC_SetDMATimeOut(LPC_DAC,tmp);
		DAC_ConfigDAConverterControl(LPC_DAC, &DAC_ConverterConfigStruct);

		// Habilitar canal DMA 0
		GPDMA_ChannelCmd(0, ENABLE);

		_DBG_("\n\rPreass ESC if you want to terminate!");
		while(_DG!=27);

		// Deshabilitar canal DMA 0
		GPDMA_ChannelCmd(0, DISABLE);
	}
	return 1;
}
int main(void)
{
    return c_entry();
}

#ifdef DEBUG
void check_failed(uint8_t *file, uint32_t line)
{
	while(1);
}
#endif
```

---

### Explicación de las Formas de Onda Generadas

#### 1. Onda Sinusoidal (`DAC_GENERATE_SINE`)

**Propósito:** Generar una onda con forma de seno utilizando una tabla de búsqueda precalculada para el primer cuadrante y simetría para el resto de la onda.

**Lógica:** La onda completa de 360 grados se construye a partir de 60 muestras (la constante `NUM_SAMPLE_SINE`). El código divide la onda en cuatro cuadrantes de 15 muestras cada uno.

*   **Cuadrante 1 (muestras 0-15):** Usa el array `sin_0_to_90_16_samples` en orden ascendente (0 a 15), escalando de 0 a 1023.
*   **Cuadrante 2 (muestras 16-30):** Usa el array `sin_0_to_90_16_samples` en orden descendente (14 a 0) para crear la bajada del pico.
*   **Cuadrante 3 (muestras 31-45):** Usa el array `sin_0_to_90_16_samples` en orden ascendente (0 a 15) con un signo negativo (restado de 512) para crear el valle de la onda.
*   **Cuadrante 4 (muestras 46-60):** Usa el array `sin_0_to_90_16_samples` en orden descendente para volver a subir al punto central.

**Dibujo de la Onda Sinusoidal:**

```
^ Valor (Amplitud)
|       .-----------------. (Pico a 1023)
|      /                   \
|     /                     \
|----/----------+-----------\------> Tiempo (Muestras)
|   /           |            \
|  /            |             \
| .             |              . (Valle a 0)
+---------------v-------------------
    0   15  30  45  60
```

---

#### 2. Onda Triangular (`DAC_GENERATE_TRIANGLE`)

**Propósito:** Generar una onda con forma triangular, que sube linealmente hasta un pico y luego desciende linealmente de vuelta al punto inicial.

**Lógica:** La onda se divide en dos partes de 32 muestras cada una (total 64 muestras `NUM_SAMPLE`).

*   **Rampa Ascendente (muestras 0-31):** La fórmula `32 * i` crea una línea recta ascendente. La pendiente es `1024 / 32 = 32`. Aumenta 32 unidades por cada muestra. El valor máximo (cerca de 1023) se alcanza en `i=31` (`32*31 = 992`). El código establece `dac_lut[32]=1023` explícitamente para asegurar el pico.
*   **Rampa Descendente (muestras 33-63):** La fórmula `32 * (NUM_SAMPLE - i)` invierte la rampa. Por ejemplo, en `i=33`, el valor es `32 * (64 - 33) = 32 * 31 = 992`. En `i=63`, el valor es `32 * (64 - 63) = 32`.

**Dibujo de la Onda Triangular:**

```
^ Valor (Amplitud)
|      /\
|     /  \
|    /    \
|   /      \
|  /        \
| /          \
+---------------> Tiempo (Muestras)
    0   32  64
```

---

#### 3. Onda Escalera (`DAC_GENERATE_ESCALATOR`)

**Propósito:** Generar una onda con pasos discretos (escalones) en lugar de una transición continua.

**Lógica:** Utiliza la división entera `i / 16`. La división entera trunca el resultado, creando mesetas de valores constantes. El bucle se repite 64 veces (`NUM_SAMPLE`).

*   **Fórmula:** `dac_lut[i] = (1023/3)*(i/16);`
    *   **Paso 1 (muestras 0-15):** `i / 16 = 0`. Valor: `(1023/3) * 0 = 0`.
    *   **Paso 2 (muestras 16-31):** `i / 16 = 1`. Valor: `(1023/3) * 1 = 341`.
    *   **Paso 3 (muestras 32-47):** `i / 16 = 2`. Valor: `(1023/3) * 2 = 682`.
    *   **Paso 4 (muestras 48-63):** `i / 16 = 3`. Valor: `(1023/3) * 3 = 1023`.

**Dibujo de la Onda Escalera:**

```
^ Valor (Amplitud)
|   +-----------------------+  (Nivel 4: 1023)
|   |                       |
|   |                       |
|   |                       |  (Nivel 3: 682)
|   |                       |
|   |   +-------------------+
|   |   |                   |
|   |   |                   |  (Nivel 2: 341)
|   |   |                   |
|   |   |   +---------------+
|   |   |   |               |  (Nivel 1: 0)
+---+---+---+---+-----------+---> Tiempo (Muestras)
    0   16  32  48  64
```

---

#### 4. Onda Diente de Sierra (`DAC_GENERATE_SAWTOOTH`)

**Propósito:** Generar una onda con una rampa ascendente constante que luego cae abruptamente a cero.

**Lógica:** Implementada en el código. La rampa se genera mediante la fórmula `(1023 * i) / NUM_SAMPLE`. El valor aumenta linealmente con cada muestra.

*   **Fórmula:** `dac_lut[i] = (1023 * i) / NUM_SAMPLE;`
    *   `i = 0`: Valor = 0.
    *   `i = 32`: Valor = `(1023 * 32) / 64 = 1023 / 2 = 511`.
    *   `i = 63`: Valor = `(1023 * 63) / 64 = 1017`.

**Dibujo de la Onda Diente de Sierra:**

```
^ Valor (Amplitud)
| \
|  \
|   \
|    \
|     \
|      \
+-------+-------------------> Tiempo (Muestras)
    0   64
```



**Aplicación de la onda Diente de sierra en contra mano:**



```c
DAC_GENERATE_SAWTOOTH:
			for(i=0;i<NUM_SAMPLE;i++)
			{
				// Genera una rampa descendente.
				dac_lut[i] = (1023 * (NUM_SAMPLE - i)) / NUM_SAMPLE;
				dac_lut[i] = (dac_lut[i]<<6);
			}

```


```
Si quiero la rampa para el otro lado: 

^ Valor (Amplitud)
|
|
|   /|
|  / |
| /  |
|/   |
+----+-----------------------> Tiempo (Muestras)
    0   64
```
