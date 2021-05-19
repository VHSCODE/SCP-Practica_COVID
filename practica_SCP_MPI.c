#include <mpi.h>
#include "stdlib.h"
#include "stdio.h"
#include "time.h"
#include "assert.h"
#include "unistd.h"
#include "sys/resource.h"
/**
 * Utilidad que simula la expansion del Virus T en la poblacion de Raccoon City. 
 * 
 * Desarrollado por la Corporacion Umbrella
 * 
 **/

#define DEBUG 1 //Usado durante el desarrollo para diferenciar entre diferentes personas y activar varios printf
//######################### Definiciones de tipos #########################

enum Estado
{
    SANO,
    INFECTADO_ASINTOMATICO,
    INFECTADO_SINTOMATICO,
    RECUPERADO,
    VACUNADO,
    FALLECIDO
};

const char* nombres_estados[] =
{   "SANO",
    "INFECTADO_ASINTOMATICO",
    "INFECTADO_SINTOMATICO",
    "RECUPERADO",
    "VACUNADO",
    "FALLECIDO"
};
typedef struct
{
    int val1;
    int val2;
} Tupla;

typedef struct
{
    int edad; // Entre 0 y 110
    enum Estado estado;
    double probabilidad_muerte; // Usado en caso de estar infectado
    Tupla posicion;             //Posicion x e y dentro del mundo
    Tupla velocidad;            // Representa el movimiento a realizar en cada eje. X e Y en nuestro caso.
    int ciclos_desde_infeccion; //Esto es usado para controlar el periodo de incubacion de una persona infectada
#ifdef DEBUG
    int identificador;
#endif
} Persona;

//##################################################

//######################### Constantes #########################

#define TAMANNO_MUNDO 5     //Tamaño de la matriz del mundo
#define TAMANNO_POBLACION 10 //Cantidad de personas en el mundo

#define VELOCIDAD_MAX 1 //Define la velocidad maxima a la que una persona podra viajar por el mundo. El numero indica el numero de "casillas"
#define RADIO 1         //Define el radio de infeccion de una persona infectada
#define PORCENTAJE_INMUNE 70    //Define el porcentaje de población que queremos vacunar para el final de la ejecucion
#define PROBABILIDAD_CONTAGIO 50 //Define en porcentaje la probabilidad de infectar a alguien que este dentro del radio
#define COMIENZO_VACUNACION 4    //Define la iteración en la que queremos comenzar a vacunar a gente
#define PERIODO_INCUBACION 14  //Define cuantos ciclos debe durar la incubacion del virus, hasta que este sea capaz de infectar
#define PERIODO_RECUPERACION 20 // Define cuantos ciclos se tarda en recuperarse del virus

//Definiciones para la barra de progreso
#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60 // Anchura de la barra
//##################################################

void dibujar_mundo(Persona **mundo);
int comprobar_posicion(const Tupla posiciones_asignadas[TAMANNO_POBLACION], const Tupla posicion);
void inicializar_mundo(Persona **mundo);
void simular_ciclo(Persona **mundo, int comenzar_vacunacion, int gente_a_vacunar);
int random_interval(int min, int max);
Tupla mover_persona(Persona **mundo, Persona *persona);
void recoger_metricas(Persona **mundo);
void infectar(Persona **mundo, Tupla posicion_persona);
void deberia_morir(Persona **mundo, Tupla posicion_persona);
void vacunar(Persona **mundo);
int probabilidad_muerte(int edad);
void printProgress(double percentage);
void guardar_estado(Persona **mundo, long iteracion);
void crear_o_borrar_archivos();


int main(int argc, char const *argv[])
{

    int world_rank, world_size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    assert(TAMANNO_MUNDO * TAMANNO_MUNDO > TAMANNO_POBLACION);

    srand(time(NULL));

    if (argc != 3)
    {
        printf("Uso: ./run.sh \"practica_SCP_MPI <numero_de_iteraciones> <frecuencia_batches> \" numero_de_procesadores \n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    long iteraciones = atol(argv[1]); //Numero de iteraciones a realizar.
    long frecuencia_batches = atol(argv[2]); //Cada cuanto se debe guardar el estado del mundo


    if (COMIENZO_VACUNACION >= iteraciones)
    {
        printf("[Error] El numero de iteraciones debe ser mayor al umbral de comienzo de vacunacion y viceversa\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    else if(frecuencia_batches > iteraciones)
    {
        printf("[Error] La frecuencia de guardado de datos debe de ser menor a la cantidad de iteraciones a realizar\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }else if (TAMANNO_POBLACION % world_size != 0)
    {
        printf("[Error] El tamanno de la poblacion debe ser multiplo del numero de procesadores en los que se ejecuta\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

  if (world_rank == 0) 
  {

    int total_vacunados = (TAMANNO_POBLACION * PORCENTAJE_INMUNE) / 100;

    int gente_a_vacunar_por_iteracion = total_vacunados / (iteraciones - COMIENZO_VACUNACION); //Calculos necesarios para saber cuantas personas hay que vacunar en cada iteracion

    if (gente_a_vacunar_por_iteracion == 0) 
        gente_a_vacunar_por_iteracion = 1;

   
    Persona **mundo = malloc(TAMANNO_MUNDO * TAMANNO_MUNDO * sizeof(Persona *)); // "Tablero" que representara el mundo en una matriz 2D
    if (mundo == NULL)
    {
        printf("Error al asignar memoria al mundo\n");
        return 1;
    }

    crear_o_borrar_archivos();
    printf("Inicializando mundo...\n");
    inicializar_mundo(mundo);

#ifdef DEBUG
    printf("Situacion inicial\n");

    dibujar_mundo(mundo);
#endif
    long iteraciones_realizadas = 0;
    int comenzar_vacunacion = 0; 
    int gente_vacunada_en_ciclo = 0;
    double porcentaje_completado = 0.0;


    long deberia_guardar = frecuencia_batches;
 

    clock_t inicio, final;

    double tiempo_transcurrido;

   
    printf("Comienza la simulacion...\n");
    printProgress(porcentaje_completado);
    printf("\n");
    
    inicio = clock();
    for (iteraciones_realizadas; iteraciones_realizadas < iteraciones; iteraciones_realizadas++)
    {
        if (iteraciones_realizadas > COMIENZO_VACUNACION - 1) //Si llegamos al umbral, comenzamos el proceso de vacunacion
            comenzar_vacunacion = 1;
        
        simular_ciclo(mundo,comenzar_vacunacion,gente_a_vacunar_por_iteracion);


        deberia_guardar--;
        if(deberia_guardar == 0 || iteraciones_realizadas == 0)
        {
            guardar_estado(mundo,iteraciones_realizadas + 1);
            deberia_guardar = frecuencia_batches;
        }
        
        porcentaje_completado = (double) iteraciones_realizadas / (double )iteraciones;
        printProgress(porcentaje_completado);

#ifdef DEBUG
        printf("Iteracion %d: \n", iteraciones_realizadas + 1);

        dibujar_mundo(mundo);
#endif
    }
    final = clock();
    tiempo_transcurrido = ((double)final - (double)inicio) / CLOCKS_PER_SEC;
    printProgress(1.0);
    printf("\nTerminado!!!\n");

    recoger_metricas(mundo);

    printf("Tiempo transcurrido: %f segundos\n",tiempo_transcurrido);


    struct rusage r_usage;

    int ret = getrusage(RUSAGE_SELF,&r_usage);
    if(ret != 0)
        printf("Error obteniendo la cantidad de memoria usada\n");
    else
        printf("Uso de memoria: %ld kilobytes\n", r_usage.ru_maxrss);
    //Liberamos la memoria asignada
    int i, j;
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            if (mundo[i + j * TAMANNO_MUNDO] != NULL)
                free(mundo[i + j * TAMANNO_MUNDO]);
        }
    }

    free(mundo);
  }
  MPI_Finalize();

}


/**
 * Esta funcion se encarga simular un ciclo en la simulacion: Mover a las personas, infectar a los demas, muertes, etc...
 **/
void simular_ciclo(Persona **mundo, int comenzar_vacunacion, int gente_a_vacunar)
{
    int i, j;
    int gente_vacunada_en_ciclo = 0;
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            if (mundo[i + j * TAMANNO_MUNDO] != NULL) //FIXME esta busqueda se puede optimizar si guardamos las posiciones de las personas que estan en la matriz... :)
            {
                Persona *persona_tmp = mundo[i + j * TAMANNO_MUNDO];

                Tupla pos_persona = mover_persona(mundo, persona_tmp); //Movemos la persona en el mundo
                
                switch (persona_tmp->estado)
                {
                case INFECTADO_SINTOMATICO: //Solo los sintomaticos pueden infectar

                    if (persona_tmp->ciclos_desde_infeccion >= PERIODO_RECUPERACION)
                    {
                        mundo[pos_persona.val1 + pos_persona.val2 * TAMANNO_MUNDO]->estado = RECUPERADO;
                        mundo[pos_persona.val1 + pos_persona.val2 * TAMANNO_MUNDO]->ciclos_desde_infeccion = 0;
                    }
                    else
                    {
                        printf("posicion infectado x:%d, y:%d\n", pos_persona.val1, pos_persona.val2);
                        infectar(mundo, pos_persona);

                        deberia_morir(mundo, pos_persona);
                        mundo[pos_persona.val1 + pos_persona.val2 * TAMANNO_MUNDO]->ciclos_desde_infeccion++;
                    }

                    break;

                case INFECTADO_ASINTOMATICO:
                    if (persona_tmp->ciclos_desde_infeccion >= PERIODO_INCUBACION) //Si ha pasado el periodo de incubacion, lo convertimos en sintomatico
                    {
                        mundo[pos_persona.val1 + pos_persona.val2 * TAMANNO_MUNDO]->estado = INFECTADO_SINTOMATICO;
                        mundo[pos_persona.val1 + pos_persona.val2 * TAMANNO_MUNDO]->ciclos_desde_infeccion = 0;
                    }
                    else
                        mundo[pos_persona.val1 + pos_persona.val2 * TAMANNO_MUNDO]->ciclos_desde_infeccion++;
                    break;
                case SANO:
                    if(comenzar_vacunacion == 1 && gente_vacunada_en_ciclo < gente_a_vacunar)
                    {
                        mundo[pos_persona.val1 + pos_persona.val2 * TAMANNO_MUNDO]->estado = VACUNADO;
                        gente_vacunada_en_ciclo++;
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void recoger_metricas(Persona **mundo)
{
    int cantidad_infectados = 0;
    int cantidad_fallecidos = 0;
    int cantidad_vacunados = 0;
    int cantidad_recuperados = 0;
    int i, j;
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            if (mundo[i + j * TAMANNO_MUNDO] != NULL) //FIXME esta busqueda se puede optimizar si guardamos las posiciones de las personas que estan en la matriz... :)
            {
                if (mundo[i + j * TAMANNO_MUNDO]->estado == INFECTADO_ASINTOMATICO || mundo[i + j * TAMANNO_MUNDO]->estado == INFECTADO_SINTOMATICO)
                {
                    cantidad_infectados++;
                }
                else if (mundo[i + j * TAMANNO_MUNDO]->estado == FALLECIDO)
                {
                    cantidad_fallecidos++;
                }
                else if (mundo[i + j * TAMANNO_MUNDO]->estado == VACUNADO)
                {
                    cantidad_vacunados++;
                }
                 else if (mundo[i + j * TAMANNO_MUNDO]->estado == RECUPERADO)
                {
                    cantidad_recuperados++;
                }
            }
        }
    }
    
    //Dejamos los resultados finales de las metricas en el fichero "practica_SCP.metricas"
    FILE* fichero;
    fichero = fopen("practica_SCP.metricas", "wt");
    fprintf(fichero, "Tamanno Poblacion : %d, Infectados: %d, Fallecidos: %d, Vacunados: %d, Recuperados: %d\n", TAMANNO_POBLACION, cantidad_infectados, cantidad_fallecidos, cantidad_vacunados,cantidad_recuperados);
    fclose(fichero);

    printf("Tamanno Poblacion : %d, Infectados: %d, Fallecidos: %d, Vacunados: %d, Recuperados: %d\n", TAMANNO_POBLACION, cantidad_infectados, cantidad_fallecidos, cantidad_vacunados,cantidad_recuperados);
    if (TAMANNO_POBLACION == cantidad_fallecidos)
    {
        printf("Bye Bye Raccoon City\n");
    }
}
void infectar(Persona **mundo, Tupla posicion_persona)
{
    Persona *persona_tmp = mundo[posicion_persona.val1 + posicion_persona.val2 * TAMANNO_MUNDO];

    int x;
    int y;
    for (x = -RADIO; x <= RADIO; x++)
    {
        for (y = -RADIO; y <= RADIO; y++)
        {
            Tupla posible_persona = posicion_persona;

            posible_persona.val1 += x;
            posible_persona.val2 += y;

            if ((posible_persona.val1 >= 0 && posible_persona.val1 < TAMANNO_MUNDO) && (posible_persona.val2 >= 0 && posible_persona.val2 < TAMANNO_MUNDO))
            {
                if (mundo[posible_persona.val1 + posible_persona.val2 * TAMANNO_MUNDO] != NULL)
                {
                    if (mundo[posible_persona.val1 + posible_persona.val2 * TAMANNO_MUNDO]->estado == SANO)
                    {
                        int infectar = random_interval(0, 100);
                        if (infectar <= PROBABILIDAD_CONTAGIO)
                        {
                            mundo[posible_persona.val1 + posible_persona.val2 * TAMANNO_MUNDO]->estado = INFECTADO_ASINTOMATICO;
                            mundo[posible_persona.val1 + posible_persona.val2 * TAMANNO_MUNDO]->ciclos_desde_infeccion = 0;
                        }
                    }
                }
            }
        }
    }
}
void deberia_morir(Persona **mundo, Tupla posicion_persona)
{
    int muerto, prob_muerte;
    muerto = random_interval(0, 100000);
    prob_muerte = mundo[posicion_persona.val1 + posicion_persona.val2 * TAMANNO_MUNDO]->probabilidad_muerte;
    if (muerto < prob_muerte)
    {
        mundo[posicion_persona.val1 + posicion_persona.val2 * TAMANNO_MUNDO]->estado = FALLECIDO;
    }
}
Tupla mover_persona(Persona **mundo, Persona *persona)
{

    int flag = 0;
    Tupla vieja_pos = persona->posicion;   
    do //Este bucle se encarga de que cada persona se mueva, si no es capaz de moverse, se le asignara una nueva velocidad, para que este pueda moverse por el mundo.
    {
        Tupla nueva_pos = vieja_pos;
            
        //Asignamos una velocidad aleatoria entre -VELOCIDAD_MAX y VELOCIDAD_MAX
        persona->velocidad.val1 = random_interval(-VELOCIDAD_MAX, VELOCIDAD_MAX);
        persona->velocidad.val2 = random_interval(-VELOCIDAD_MAX, VELOCIDAD_MAX);

        //Calculamos la nueva posicion de la persona
        nueva_pos.val1 = nueva_pos.val1 + persona->velocidad.val1;
        nueva_pos.val2 = nueva_pos.val2 + persona->velocidad.val2;

        //Comprobamos si el movimiento es posible
        if ((nueva_pos.val1 >= 0 && nueva_pos.val1 < TAMANNO_MUNDO) && (nueva_pos.val2 >= 0 && nueva_pos.val2 < TAMANNO_MUNDO)) //Primero comprobamos que la posicion sea valida dentro de la matriz 2D
        {
            if ((nueva_pos.val1 == vieja_pos.val1) && (nueva_pos.val2 == vieja_pos.val2)) 
            {
                flag = 1;
                return nueva_pos;
            }

            if (mundo[nueva_pos.val1 + nueva_pos.val2 * TAMANNO_MUNDO] == NULL) //Ahora comprobamos que la casilla a moverse este vacia
            {
                persona->posicion = nueva_pos;
                mundo[nueva_pos.val1 + nueva_pos.val2 * TAMANNO_MUNDO] = persona;
                mundo[vieja_pos.val1 + vieja_pos.val2 * TAMANNO_MUNDO] = NULL;
                flag = 1;
                return nueva_pos;
            }
        }
    } while (flag == 0);
}
void inicializar_mundo(Persona **mundo)
{
    int i, j, k;
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            mundo[i + j * TAMANNO_MUNDO] = NULL;
        }
    }

    k = 0; //Usado para indexar el vector de las posiciones
    Tupla posiciones_asignadas[TAMANNO_POBLACION] = {{0, 0}};
    for (i = 0; i < TAMANNO_POBLACION; i++)
    {
        Persona *persona_tmp = malloc(sizeof(Persona));
        persona_tmp->edad = random_interval(0, 110);
        int prob = probabilidad_muerte(persona_tmp->edad);
        persona_tmp->probabilidad_muerte = prob;
        persona_tmp->ciclos_desde_infeccion = 0;

#ifdef DEBUG
        persona_tmp->identificador = i + 1;
#endif

        Tupla pos;

        int flag = 0; // 0 si no existe nadie en esa posicion, 1 en el caso contrario.
        do
        {
            pos.val1 = random_interval(0, TAMANNO_MUNDO - 1);
            pos.val2 = random_interval(0, TAMANNO_MUNDO - 1);

            flag = comprobar_posicion(posiciones_asignadas, pos); //Comprobamos si existe algun otra persona en esa misma posicion.
        } while (flag != 0);

        posiciones_asignadas[k] = pos;
        k++;
        persona_tmp->posicion = pos;

        Tupla vel;
        vel.val1 = 0;
        vel.val2 = 0;

        persona_tmp->velocidad = vel;


        if (i == 0) //Elegimos el paciente 0
        {
            persona_tmp->estado = INFECTADO_SINTOMATICO;
            #ifdef DEBUG
            printf("Posicion infectado: (%d,%d)\n", persona_tmp->posicion.val1, persona_tmp->posicion.val2);
            #endif
        }
        else
        {
            persona_tmp->estado = SANO;
        }
        mundo[pos.val1 + pos.val2 * TAMANNO_MUNDO] = persona_tmp;
#ifdef DEBUG
        printf("Posicion persona: (%d,%d), estado : %d\n", persona_tmp->posicion.val1, persona_tmp->posicion.val2, mundo[pos.val1 + pos.val2 * TAMANNO_MUNDO]->estado);
#endif   
    }
}

int comprobar_posicion(const Tupla posiciones_asignadas[TAMANNO_POBLACION], const Tupla posicion)
{

    int i;

    for (i = 0; i < TAMANNO_POBLACION; i++)
    {
        if (posiciones_asignadas[i].val1 == posicion.val1 && posiciones_asignadas[i].val2 == posicion.val2)
        {
            return 1;
        }
    }
    return 0;
}


void guardar_estado(Persona **mundo, long iteracion)
{
    int i, j;

    FILE* archivo = fopen("practica_SCP.pos","a");
    fprintf(archivo,"ITERACION %d TAMANNO %d POBLACION %d\n",iteracion, TAMANNO_MUNDO, TAMANNO_POBLACION);
    fflush(archivo);
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            if (mundo[i + j * TAMANNO_MUNDO] != NULL)
            {
                fprintf(archivo,"%d %d %s\n", mundo[i + j * TAMANNO_MUNDO]->posicion.val1, mundo[i + j * TAMANNO_MUNDO]->posicion.val2, nombres_estados[mundo[i + j * TAMANNO_MUNDO]->estado]);
                fflush(archivo);
            }
        }
    }

    fclose(archivo);
}

void crear_o_borrar_archivos()
{
    FILE* archivo = fopen("practica_SCP.pos","w");
    fprintf(archivo,"Posiciones y estados de las personas cada iteracion:\n");
    fflush(archivo);
    fclose(archivo);

    archivo = fopen("practica_SCP.metricas","w");
    fprintf(archivo,"Metricas finales de la simulacion:\n");
    fflush(archivo);
    fclose(archivo);
}
void dibujar_mundo(Persona **mundo)
{
    int i, j;
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            if (mundo[i + j * TAMANNO_MUNDO] != NULL)
            {
                if (mundo[i + j * TAMANNO_MUNDO]->estado == INFECTADO_SINTOMATICO || mundo[i + j * TAMANNO_MUNDO]->estado == INFECTADO_ASINTOMATICO)
                    printf("X%d     ",mundo[i + j * TAMANNO_MUNDO]->identificador);
                else if (mundo[i + j * TAMANNO_MUNDO]->estado == FALLECIDO)
                    printf("-%d     ",mundo[i + j * TAMANNO_MUNDO]->identificador);
                else if (mundo[i + j * TAMANNO_MUNDO]->estado == VACUNADO)
                    printf("V%d     ",mundo[i + j * TAMANNO_MUNDO]->identificador);
                else
                {
#ifdef DEBUG
                    printf("%d      ", mundo[i + j * TAMANNO_MUNDO]->identificador);
#endif

#ifndef DEBUG
                    printf("1      ");
#endif
                }
            }
            else
            {
                printf("0      ");
            }
        }
        printf("\n");
    }
}

int random_interval(int min, int max)
{
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

int probabilidad_muerte(int edad)
{
    if (edad < 10)
    {
        return 0;
    }
    else if (edad < 20)
    {
        return 2;
    }
    else if (edad < 30)
    {
        return 3;
    }
    else if (edad < 40)
    {
        return 4;
    }
    else if (edad < 50)
    {
        return 5;
    }
    else if (edad < 60)
    {
        return 13;
    }
    else if (edad < 70)
    {
        return 36;
    }
    else if (edad < 80)
    {
        return 80;
    }
    else if (edad < 90)
    {
        return 148;
    }
    else if (edad < 100)
    {
        return 296;
    }
    else
    {
        return 592;
    }
}

//Barra de progreso. Sacado de https://stackoverflow.com/questions/14539867/how-to-display-a-progress-indicator-in-pure-c-c-cout-printf
void printProgress(double percentage) {
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}
//Si has llegado hasta aqui, te has ganado una galleta 🍪