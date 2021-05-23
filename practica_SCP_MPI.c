#include <mpi.h>
#include "stdlib.h"
#include "stdio.h"
#include "time.h"
#include "assert.h"
#include "unistd.h"
#include "sys/resource.h"
#include "sys/time.h"
/**
 * Utilidad que simula la expansion del Virus T en la poblacion de Raccoon City. 
 * 
 * Desarrollado por la Corporacion Umbrella
 * 
 **/

//#define DEBUG 1 //Usado durante el desarrollo para diferenciar entre diferentes personas y activar varios printf
//######################### Definiciones de tipos #########################
//Definiciones globales de tipos de MPI
enum Estado
{
    SANO,
    INFECTADO_ASINTOMATICO,
    INFECTADO_SINTOMATICO,
    RECUPERADO,
    VACUNADO,
    FALLECIDO
};

const char *nombres_estados[] =
    {"SANO",
     "INFECTADO_ASINTOMATICO",
     "INFECTADO_SINTOMATICO",
     "RECUPERADO",
     "VACUNADO",
     "FALLECIDO"};
typedef struct
{
    long val1;
    long val2;
} Tupla;

typedef struct
{
    int edad; // Entre 0 y 110
    enum Estado estado;
    double probabilidad_muerte; // Usado en caso de estar infectado
    Tupla posicion;             //Posicion x e y dentro del mundo
    Tupla velocidad;            // Representa el movimiento a realizar en cada eje. X e Y en nuestro caso.
    int ciclos_desde_infeccion; //Esto es usado para controlar el periodo de incubacion de una persona infectada
    long se_ha_movido;
#ifdef DEBUG
    int identificador;
#endif
} Persona;

//##################################################

//######################### Constantes #########################

#define TAMANNO_MUNDO 100000L    //Tamaño de la matriz 2x2 del mundo
#define TAMANNO_POBLACION 40000L //Cantidad de personas en el mundo

#define VELOCIDAD_MAX 5           //Define la velocidad maxima a la que una persona podra viajar por el mundo. El numero indica el numero de "casillas"
#define RADIO 2                 //Define el radio de infeccion de una persona infectada
#define PORCENTAJE_INMUNE 70      //Define el porcentaje de población que queremos vacunar para el final de la ejecucion
#define PROBABILIDAD_CONTAGIO 50 //Define en porcentaje la probabilidad de infectar a alguien que este dentro del radio
#define COMIENZO_VACUNACION 2    //Define la iteración en la que queremos comenzar a vacunar a gente
#define PERIODO_INCUBACION 14     //Define cuantos ciclos debe durar la incubacion del virus, hasta que este sea capaz de infectar
#define PERIODO_RECUPERACION 20   // Define cuantos ciclos se tarda en recuperarse del virus

//Definiciones para la barra de progreso
#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60 // Anchura de la barra
//##################################################

int comprobar_posicion(const Tupla posiciones_asignadas[TAMANNO_POBLACION], const Tupla posicion);
void inicializar_mundo(Persona *mundo);
void simular_ciclo(Persona *mundo, int comenzar_vacunacion, int gente_a_vacunar, long iteracion, int personas_procesador, MPI_Datatype TuplaType);
int random_interval(int min, int max);
void mover_persona(Persona *mundo, int i);
void recoger_metricas(Persona *mundo, int personas_procesador);
void infectar(Persona *mundo, Tupla posicion_infectado, int personas_procesador);
int deberia_morir(Persona *mundo, int i);
int probabilidad_muerte(int edad);
void printProgress(double percentage);
void guardar_estado(Persona *mundo, long iteracion);
void crear_o_borrar_archivos();

int world_rank, world_size, personas_procesador;

int main(int argc, char *argv[])
{

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int i, j, myrank;
    MPI_Status status;
    srand(time(NULL) * world_rank);

    if (argc != 3)
    {
        printf("Uso: ./run.sh \"practica_SCP_MPI <numero_de_iteraciones> <frecuencia_batches> \" numero_de_procesadores \n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    long iteraciones = atol(argv[1]);        //Numero de iteraciones a realizar.
    long frecuencia_batches = atol(argv[2]); //Cada cuanto se debe guardar el estado del mundo
    if (COMIENZO_VACUNACION >= iteraciones)
    {
        printf("[Error] El numero de iteraciones debe ser mayor al umbral de comienzo de vacunacion y viceversa\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    else if (frecuencia_batches > iteraciones)
    {
        printf("[Error] La frecuencia de guardado de datos debe de ser menor a la cantidad de iteraciones a realizar\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    else if (TAMANNO_POBLACION % world_size != 0)
    {
        printf("[Error] El tamanno de la poblacion debe ser multiplo del numero de procesadores en los que se ejecuta\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    personas_procesador = TAMANNO_POBLACION / world_size;

    int total_vacunados = (TAMANNO_POBLACION * PORCENTAJE_INMUNE) / 100;
    int total_vacunados_por_procesador = total_vacunados / world_size;

    int gente_a_vacunar_por_iteracion = total_vacunados_por_procesador / (iteraciones - COMIENZO_VACUNACION); //Calculos necesarios para saber cuantas personas hay que vacunar en cada iteracion

    if (gente_a_vacunar_por_iteracion == 0)
        gente_a_vacunar_por_iteracion = 1;

    Persona *mundo;
    if (world_rank == 0)
    {
        mundo = malloc(TAMANNO_POBLACION * sizeof(Persona));
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

#endif
    }
    else
        mundo = malloc(personas_procesador * sizeof(Persona));

    //Creamos los tipos necesarios para que MPI los reconozca

    MPI_Datatype TuplaType;
    MPI_Datatype tupla_types[2] = {MPI_LONG, MPI_LONG};

    int blocklen_tupla[2] = {1, 1};

    Tupla tmp;
    MPI_Aint disp_tupla[2];

    MPI_Aint base_address;

    MPI_Get_address(&tmp, &base_address);
    MPI_Get_address(&tmp.val1, &disp_tupla[0]);
    MPI_Get_address(&tmp.val2, &disp_tupla[1]);

    disp_tupla[0] = MPI_Aint_diff(disp_tupla[0], base_address);
    disp_tupla[1] = MPI_Aint_diff(disp_tupla[1], base_address);

    MPI_Type_create_struct(2, blocklen_tupla, disp_tupla, tupla_types, &TuplaType);

    MPI_Type_commit(&TuplaType);

    MPI_Datatype PersonaType;

    MPI_Datatype Type[7] = {MPI_INT, MPI_INT, MPI_DOUBLE, TuplaType, TuplaType, MPI_INT, MPI_LONG};

    int blocklen_persona[7] = {1, 1, 1, 1, 1, 1, 1};
    Persona tmp2;
    MPI_Aint dis_persona[7];

    MPI_Get_address(&tmp2, &base_address);

    MPI_Get_address(&tmp2.edad, &dis_persona[0]);
    MPI_Get_address(&tmp2.estado, &dis_persona[1]);
    MPI_Get_address(&tmp2.probabilidad_muerte, &dis_persona[2]);
    MPI_Get_address(&tmp2.posicion, &dis_persona[3]);
    MPI_Get_address(&tmp2.velocidad, &dis_persona[4]);
    MPI_Get_address(&tmp2.ciclos_desde_infeccion, &dis_persona[5]);
    MPI_Get_address(&tmp2.se_ha_movido, &dis_persona[6]);

    dis_persona[0] = MPI_Aint_diff(dis_persona[0], base_address);
    dis_persona[1] = MPI_Aint_diff(dis_persona[1], base_address);
    dis_persona[2] = MPI_Aint_diff(dis_persona[2], base_address);
    dis_persona[3] = MPI_Aint_diff(dis_persona[3], base_address);
    dis_persona[4] = MPI_Aint_diff(dis_persona[4], base_address);
    dis_persona[5] = MPI_Aint_diff(dis_persona[5], base_address);
    dis_persona[6] = MPI_Aint_diff(dis_persona[6], base_address);

    MPI_Type_create_struct(7, blocklen_persona, dis_persona, Type, &PersonaType);

    MPI_Type_commit(&PersonaType);

    //Dividimos la personas entre los procesadores
    if (MPI_Scatter(mundo, personas_procesador, PersonaType, mundo, personas_procesador, PersonaType, 0, MPI_COMM_WORLD) != MPI_SUCCESS)
    {
        perror("Scatter Error");
        exit(1);
    }
    long iteraciones_realizadas = 0;
    int comenzar_vacunacion = 0;
    int gente_vacunada_en_ciclo = 0;
    double porcentaje_completado = 0.0;

    long deberia_guardar = frecuencia_batches;



    struct timeval  t1,t2;
    double tiempo_transcurrido;
    if (world_rank == 0)
    {
        printf("Comienza la simulacion...\n");
        printProgress(porcentaje_completado);
#ifdef DEBUG
        printf("\n");
#endif
    }

    gettimeofday(&t1,NULL);

    for (iteraciones_realizadas; iteraciones_realizadas < iteraciones; iteraciones_realizadas++)
    {
        if (iteraciones_realizadas > COMIENZO_VACUNACION - 1) //Si llegamos al umbral, comenzamos el proceso de vacunacion
            comenzar_vacunacion = 1;

        simular_ciclo(mundo, comenzar_vacunacion, gente_a_vacunar_por_iteracion, iteraciones_realizadas, personas_procesador, TuplaType);
        
        deberia_guardar--;
        if (deberia_guardar == 0 || iteraciones_realizadas == 0)
        {
            guardar_estado(mundo, iteraciones_realizadas + 1);
            deberia_guardar = frecuencia_batches;
        }
        porcentaje_completado = (double)iteraciones_realizadas / (double)iteraciones;

        if (world_rank == 0)
        {
            printProgress(porcentaje_completado);
            printf("Iteracion %d", iteraciones_realizadas + 1);

#ifdef DEBUG

#endif
        }
    }
    gettimeofday(&t2,NULL);


    double tiempo_real;
    tiempo_transcurrido = t2.tv_sec - t1.tv_sec;

    MPI_Reduce(&tiempo_transcurrido,&tiempo_real,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);


    if (world_rank == 0)
    {
        printProgress(1.0);
        printf("\nTerminado!!!\n");
    }
    recoger_metricas(mundo,personas_procesador);

    if (world_rank == 0)
    {

        printf("Tiempo transcurrido: %f segundos\n", tiempo_real);

        struct rusage r_usage;

        int ret = getrusage(RUSAGE_SELF, &r_usage);
        if (ret != 0)
            printf("Error obteniendo la cantidad de memoria usada\n");
        else
            printf("Uso de memoria: %ld kilobytes\n", r_usage.ru_maxrss);
    }


    free(mundo);

    MPI_Type_free(&PersonaType);
    MPI_Type_free(&TuplaType);
    MPI_Finalize();
}

/**
 * Esta funcion se encarga simular un ciclo en la simulacion: Mover a las personas, infectar a los demas, muertes, etc...
 **/
void simular_ciclo(Persona *mundo, int comenzar_vacunacion, int gente_a_vacunar, long iteracion, int personas_procesador, MPI_Datatype TuplaType)
{
    int i, j;
    int gente_vacunada_en_ciclo = 0;

    int tamaño_pos = 0;
    int index_posiciones = 0;
    int hay_que_infectar_procesador = 0;
    int hay_que_infectar_total = 0;
    for (i = 0; i < personas_procesador; i++)
    {
       
        if (mundo[i].se_ha_movido == iteracion)
        {
            mover_persona(mundo, i); //Movemos la persona en el mundo
            mundo[i].se_ha_movido++;

            switch (mundo[i].estado) //Ahora hacemos las acciones necesarias para cada estado
            {
            case INFECTADO_SINTOMATICO: //Solo los sintomaticos pueden infectar

                if (mundo[i].ciclos_desde_infeccion >= PERIODO_RECUPERACION)
                {
                    mundo[i].estado = RECUPERADO;
                    mundo[i].ciclos_desde_infeccion = 0;
                }
                else
                {
                    if (deberia_morir(mundo, i) == 1)
                    {

                        mundo[i].ciclos_desde_infeccion++;
                        hay_que_infectar_procesador = 1;
                        index_posiciones++;
                        tamaño_pos++;
                    }
                    else
                    {
                        mundo[i].estado = FALLECIDO;
                    }
                }
                break;

            case INFECTADO_ASINTOMATICO:
                if (mundo[i].ciclos_desde_infeccion >= PERIODO_INCUBACION) //Si ha pasado el periodo de incubacion, lo convertimos en sintomatico
                {
                    mundo[i].estado = INFECTADO_SINTOMATICO;
                    mundo[i].ciclos_desde_infeccion = 0;
                }
                else
                    mundo[i].ciclos_desde_infeccion++;
                break;

            case SANO:
                if (comenzar_vacunacion == 1 && gente_vacunada_en_ciclo < gente_a_vacunar)
                {
                    mundo[i].estado = VACUNADO;
                    gente_vacunada_en_ciclo++;
                }
                break;
            default:
                break;
            }
        }
        hay_que_infectar_total = 0;
        MPI_Allreduce(&hay_que_infectar_procesador, &hay_que_infectar_total, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD); //Comprobamos si es necesario infectar

        if (hay_que_infectar_total == 1)
        {
            infectar(mundo, mundo[i].posicion, personas_procesador);
            hay_que_infectar_total = 0;
            hay_que_infectar_procesador = 0;
        }
    }
}

void recoger_metricas(Persona *mundo, int personas_procesador)
{
    int recogida_metricas[4] = {0, 0, 0, 0};
    int i, j;
    for (i = 0; i < personas_procesador; i++)
    {
        if (mundo[i].estado == INFECTADO_ASINTOMATICO || mundo[i].estado == INFECTADO_SINTOMATICO)
        {
            recogida_metricas[0]++;
        }
        else if (mundo[i].estado == FALLECIDO)
        {
            recogida_metricas[1]++;
        }
        else if (mundo[i].estado == VACUNADO)
        {
            recogida_metricas[2]++;
        }
        else if (mundo[i].estado == RECUPERADO)
        {
            recogida_metricas[3]++;
        }
    }

    //El procesador 0 recoge los datos de los demás procesadores
    int metricas_totales[4];
    MPI_Reduce(recogida_metricas, metricas_totales, 4, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (world_rank == 0)
    {
        //Dejamos los resultados finales de las metricas en el fichero "practica_SCP.metricas"
        FILE *fichero;
        fichero = fopen("practica_SCP.metricas", "wt");
        fprintf(fichero, "Tamanno Poblacion : %d, Infectados: %d, Fallecidos: %d, Vacunados: %d, Recuperados: %d\n", TAMANNO_POBLACION, metricas_totales[0], metricas_totales[1], metricas_totales[2], metricas_totales[3]);
        fclose(fichero);

        printf("Tamanno Poblacion : %d, Infectados: %d, Fallecidos: %d, Vacunados: %d, Recuperados: %d\n", TAMANNO_POBLACION, metricas_totales[0], metricas_totales[1], metricas_totales[2], metricas_totales[3]);
        if (TAMANNO_POBLACION == metricas_totales[1])
        {
            printf("Bye Bye Raccoon City\n");
        }
    }
}

void infectar(Persona *mundo, Tupla posicion_infectado, int personas_procesador)
{

    int i, z;
    for (z = 0; z < personas_procesador; z++) //Cogemos cada persona del procesador
    {
        if ((mundo[z].posicion.val1 != posicion_infectado.val1) && (mundo[z].posicion.val2 != posicion_infectado.val1)) //Para evitar coger a la misma persona que infecta dentro del mundo
        {
            //Comprobamos si la persona esta dentro del radio de infeccion de la persona infectada
            if ((((mundo[z].posicion.val1 - posicion_infectado.val1) ^ 2) + ((mundo[z].posicion.val2 - posicion_infectado.val2) ^ 2)) < RADIO ^ 2) //Gracias pitagoras <3
            {
                if (mundo[z].estado == SANO)
                {
                    int infectar = random_interval(0, 100);
                    if (infectar <= PROBABILIDAD_CONTAGIO)
                    {
                        mundo[z].estado = INFECTADO_ASINTOMATICO;
                        mundo[z].ciclos_desde_infeccion = 0;
                    }
                }
            }
        }
    }
    //Ahora haremos una busqueda en todos los nodos
}
//Devuelve 0 si deberia morir
int deberia_morir(Persona *mundo, int i)
{
    int muerto, prob_muerte;
    muerto = random_interval(0, 100000);
    prob_muerte = mundo[i].probabilidad_muerte;
    if (muerto < prob_muerte)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
void mover_persona(Persona *mundo, int i)
{
    int flag = 0;
    Tupla vieja_pos = mundo[i].posicion;

    do //Este bucle se encarga de que cada persona se mueva, si no es capaz de moverse, se le asignara una nueva velocidad, para que este pueda moverse por el mundo.
    {
        Tupla nueva_pos = vieja_pos;

        //Asignamos una velocidad aleatoria entre -VELOCIDAD_MAX y VELOCIDAD_MAX
        mundo[i].velocidad.val1 = random_interval(-VELOCIDAD_MAX, VELOCIDAD_MAX);
        mundo[i].velocidad.val2 = random_interval(-VELOCIDAD_MAX, VELOCIDAD_MAX);

        //Calculamos la nueva posicion de la persona
        nueva_pos.val1 = nueva_pos.val1 + mundo[i].velocidad.val1;
        nueva_pos.val2 = nueva_pos.val2 + mundo[i].velocidad.val2;

        //Comprobamos si el movimiento es posible
        if ((nueva_pos.val1 >= 0 && nueva_pos.val1 < TAMANNO_MUNDO) && (nueva_pos.val2 >= 0 && nueva_pos.val2 < TAMANNO_MUNDO)) //Primero comprobamos que la posicion sea valida dentro de la matriz 2D
        {
            mundo[i].posicion = nueva_pos;
            flag = 1;
        }
    } while (flag == 0);
}
void inicializar_mundo(Persona *mundo)
{
    int i, j, k;

    k = 0; //Usado para indexar el vector de las posiciones
    Tupla posiciones_asignadas[TAMANNO_POBLACION] = {{0, 0}};
    for (i = 0; i < TAMANNO_POBLACION; i++)
    {
        mundo[i].edad = random_interval(0, 110);
        int prob = probabilidad_muerte(mundo[i].edad);
        mundo[i].probabilidad_muerte = prob;
        mundo[i].ciclos_desde_infeccion = 0;
        mundo[i].se_ha_movido = 0;

#ifdef DEBUG
        mundo[i].identificador = (personas_procesador * world_rank) + i + 1;
#endif

        Tupla pos;

        //En distintos procesadores puede haber personas en la misma posición.

        int flag = 0; // 0 si no existe nadie en esa posicion, 1 en el caso contrario.
        do
        {
            pos.val1 = random_interval(0, TAMANNO_MUNDO - 1);
            pos.val2 = random_interval(0, TAMANNO_MUNDO - 1);

            flag = comprobar_posicion(posiciones_asignadas, pos); //Comprobamos si existe algun otra persona en esa misma posicion.
        } while (flag != 0);

        posiciones_asignadas[k] = pos;
        k++;
        mundo[i].posicion = pos;

        Tupla vel;
        vel.val1 = 0;
        vel.val2 = 0;

        mundo[i].velocidad = vel;

        if (i == 0 && world_rank == 0) //Elegimos el paciente 0
        {
            mundo[i].estado = INFECTADO_SINTOMATICO;
#ifdef DEBUG
            printf("Posicion infectado: (%d,%d)\n", mundo[i].posicion.val1, mundo[i].posicion.val2);
#endif
        }
        else
        {
            mundo[i].estado = SANO;
        }
#ifdef DEBUG
        printf("Procesador %d, persona %d, posicion (%d,%d)\n", world_rank, mundo[i].identificador, mundo[i].posicion.val1, mundo[i].posicion.val2);
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

void guardar_estado(Persona *mundo, long iteracion)
{
    int i, j;

    FILE *archivo = fopen("practica_SCP.pos", "a");
    if (world_rank == 0)
    {
        fprintf(archivo, "ITERACION %d TAMANNO %d POBLACION %d\n", iteracion, TAMANNO_MUNDO, TAMANNO_POBLACION);
        fflush(archivo);
    }
    for (i = 0; i < personas_procesador; i++)
    {
        fprintf(archivo, "%d %d %s\n", mundo[i].posicion.val1, mundo[i].posicion.val2, nombres_estados[mundo[i].estado]);
        fflush(archivo);
    }

    fclose(archivo);
}

void crear_o_borrar_archivos()
{
    FILE *archivo = fopen("practica_SCP.pos", "w");
    fprintf(archivo, "Posiciones y estados de las personas cada iteracion:\n");
    fflush(archivo);
    fclose(archivo);

    archivo = fopen("practica_SCP.metricas", "w");
    fprintf(archivo, "Metricas finales de la simulacion:\n");
    fflush(archivo);
    fclose(archivo);
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
void printProgress(double percentage)
{
    int val = (int)(percentage * 100);
    int lpad = (int)(percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}
//Si has llegado hasta aqui, te has ganado una galleta 🍪