#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Comunicar entre dos hospitales los datos de un paciente

 typedef struct{
	
	int year;
	int month ;
	int day;
	
} Birth;

typedef struct{
	
	int year;
	int month ;
	int day;
	
}Death;


// Usamos esta struct como mensajes, se podría tambien utilizar otra e invocar a esta.
typedef struct {
	
	char name[20] ;
	int age;
	int visits;
	Birth BirthDate ;
	Death DeathDate ;
	
}Data;




// 2 Funciones para la creación del txt
// SEND - Recieve 
//Pensar si hacemos la codificación





void Send(Data *Message){
    FILE *txt;
    char txtData[200];
    
   txt = fopen("data.txt","w");
   if(txt == NULL){
       perror("Error al abrir el archivo\n");
       return;
   }
   // Formatea los datos de la estructura en una cadena de texto
   // Formatea los datos de la estructura en una cadena de texto
snprintf(txtData, sizeof(txtData), "Name: %s\nAge: %d\nVisits: %d\nBirth Date: %d/%d/%d\nDeath Date: %d/%d/%d\n",
         Message->name,
         Message->age,
         Message->visits,
         Message->BirthDate.year, Message->BirthDate.month, Message->BirthDate.day,
         Message->DeathDate.year, Message->DeathDate.month, Message->DeathDate.day);
    //Escribe la cadena en el archivo de texto         
    fprintf(txt, "%s", txtData);
    //cierro el archivo
    fclose(txt);
    printf("Message sent and logged successfully.\n");

}




/*

// "Hacemos la simulación de recibido de la base de datos del otro hospital y lo agregamos en la interna nuestra, para eso simularemos copiando el arreglo elemento por elemento 
	//en uno nuevo"

*/
void Recieve (Data *Patients , int numPatients){

    Data *CheckMesagge = (Data *)malloc(numPatients * sizeof(Data)); // Allocate memory for CheckMesagge
    if (CheckMesagge == NULL) {
        printf("Memory allocation failed.\n");
        return;
    }

    for (int i = 0 ; i < numPatients ; i++){
        
        strcpy(CheckMesagge[i].name, Patients[i].name); 
		CheckMesagge[i].age = Patients[i].age; 
		CheckMesagge[i].visits = Patients[i].visits; 
		CheckMesagge[i].BirthDate.year = Patients[i].BirthDate.year; 
        CheckMesagge[i].BirthDate.month = Patients[i].BirthDate.month;
        CheckMesagge[i].BirthDate.day = Patients[i].BirthDate.day;
        CheckMesagge[i].DeathDate.year = Patients[i].DeathDate.year;
        CheckMesagge[i].DeathDate.month = Patients[i].DeathDate.month;
        CheckMesagge[i].DeathDate.day = Patients[i].DeathDate.day;

       Send(CheckMesagge);
	}
	


	
	
	
	
	
}







void DataFill(Data *Patients , int numPatients) { //Pequeña base de datos para cargar a mano para enviar 
    
    int verify;

    for (int i = 0; i < numPatients; i++) {

        // Nombre del paciente
        printf("Patient's name:\n");
        scanf("%s", Patients[i].name);

        // Edad del paciente
        printf("Patient's age:\n");
        scanf("%d", &Patients[i].age);

        // Cantidad de visitas
        printf("Number of visits:\n");
        scanf("%d", &Patients[i].visits);

        // Fecha de nacimiento
        printf("Date of birth:\n");
        printf("Year: ");
        scanf("%d", &Patients[i].BirthDate.year);
        printf("Month: ");
        scanf("%d", &Patients[i].BirthDate.month);
        printf("day: ");
        scanf("%d", &Patients[i].BirthDate.day);

        // Fecha de defunción (si aplica)
        printf("Date of death (enter 0 if still alive):\n");
		scanf ("%d", &verify);
        if ( verify != 0) {
		printf("Year: ");
        scanf("%d", &Patients[i].DeathDate.year);
        printf("Month: ");
        scanf("%d", &Patients[i].DeathDate.month);
        printf("Day:\n");
        scanf("%d", &Patients[i].DeathDate.day);
        } else {
		Patients[i].DeathDate.year = 0;
        Patients[i].DeathDate.month = 0;
        Patients[i].DeathDate.day = 0;
        }

        printf("\n"); // Separador entre pacientes
    }
}



void Test (Data *Patients , int numPatients){
    //Mostrar los datos de la estructura Data
    for (int i = 0; i < numPatients; i++) {
        printf("Patient %d:\n", i+1);
        printf("Name: %s\n", Patients[i].name);
        printf("Age: %d\n", Patients[i].age);
        printf("Visits: %d\n", Patients[i].visits);
        printf("Birth Date: %d/%d/%d\n", Patients[i].BirthDate.year, Patients[i].BirthDate.month,Patients[i].BirthDate.day);


        if (Patients[i].DeathDate.month != 0) {
            printf("Death Date: %d/%d/%d\n", Patients[i].DeathDate.year, Patients[i].DeathDate.month, Patients[i].DeathDate.day);
        } else {
            printf("Still alive\n");
        }
        printf("\n");
    }
}


	













int main()
{
	Data *Patients; // Ver de agregar el arreglo para enviar distintas cosas.
	int numPatients;


	printf("Enter the number of patients you want to send:\n");
    scanf("%d", &numPatients);
	
	// Reservar memoria dinámica para los pacientes
    Data *patientsArray = (Data *)malloc(numPatients * sizeof(Data));
    if (patientsArray == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    // Llamar a la función DataFill para rellenar los datos de los pacientes
    DataFill(patientsArray, numPatients);
	
    Patients = patientsArray;
    Test(Patients, numPatients);
    
	Recieve(Patients,numPatients);

    // Liberar la memoria después de usarla
    free(patientsArray);
	


	// CHECK

    
}
