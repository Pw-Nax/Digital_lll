#include <stdio.h>

int main() {
    int numero;
    
    printf("Ingresa un número del 1 al 7: ");
    scanf("%d", &numero);

    switch(numero) {
        case 1:
            printf("1 - Montaña de la Mesa (Sudáfrica)\n");
            break;
        case 2:
            printf("2 - Cataratas del Iguazú (Argentina/Brasil)\n");
            break;
        case 3:
            printf("3 - Amazonas (Sudamérica)\n");
            break;
        case 4:
            printf("4 - Bahía de Ha Long (Vietnam)\n");
            break;
        case 5:
            printf("5 - Parque Nacional de Komodo (Indonesia)\n");
            break;
        case 6:
            printf("6 - Río subterráneo de Puerto Princesa (Filipinas)\n");
            break;
        case 7:
            printf("7 - Monte Everest (Nepal/China)\n");
            break;
        default:
            printf("Número fuera de rango. Ingresa un número entre 1 y 7.\n");
    }

    return 0;
}
