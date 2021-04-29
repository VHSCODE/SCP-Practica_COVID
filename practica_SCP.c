#include "stdlib.h"
#include "stdio.h"
#include "time.h"
#include "assert.h"

/**
 * Utilidad que simula la expansion del Virus T en la poblacion de Raccoon City. 
 * 
 * Desarrollado por la Corporacion Umbrella
 * 
 **/



//#define DEBUG 1 //Usado durante el desarrollo para diferenciar entre diferentes personas.


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
#ifdef DEBUG
    int identificador;
#endif
} Persona;

//##################################################

//######################### Constantes #########################

#define TAMANNO_MUNDO 500    //Tamaño de la matriz del mundo
#define TAMANNO_POBLACION 1000 //Cantidad de personas en el mundo

#define VELOCIDAD_MAX 5 //Define la velocidad maxima a la que una persona podra viajar por el mundo. El numero indica el numero de "casillas"
#define RADIO 25        //Define el radio de infeccion de una persona infectada
//##################################################

void dibujar_mundo(Persona **mundo);
int comprobar_posicion(const Tupla posiciones_asignadas[TAMANNO_POBLACION], const Tupla posicion);
void inicializar_mundo(Persona **mundo);
void simular_ciclo(Persona **mundo);
int random_interval(int min, int max);
Tupla mover_persona(Persona **mundo, Persona *persona);
void recoger_metricas(Persona** mundo);
void infectar(Persona **mundo, Tupla posicion_persona);

int main(int argc, char const *argv[])
{
    assert(TAMANNO_MUNDO * TAMANNO_MUNDO > TAMANNO_POBLACION);

    srand(time(NULL));

    if (argc < 2)
    {
        printf("Uso: ./practica_scp <numero_de_iteraciones>\n");
        return 0;
    }

    long iteraciones = atol(argv[1]); //Numero de iteraciones a realizar.

    Persona **mundo = malloc(TAMANNO_MUNDO * TAMANNO_MUNDO * sizeof(Persona *)); // "Tablero" que representara el mundo en una matriz 2D
    if (mundo == NULL)
    {
        printf("Error al asignar memoria al mundo\n");
        return 1;
    }

    inicializar_mundo(mundo);

    #ifdef DEBUG
    printf("Situacion inicial\n");

    dibujar_mundo(mundo);
    #endif
    int iteraciones_realizadas = 0;

    for (iteraciones_realizadas; iteraciones_realizadas < iteraciones; iteraciones_realizadas++)
    {
        simular_ciclo(mundo);

        #ifdef DEBUG
        printf("Iteracion %d: \n", iteraciones_realizadas + 1);
        
        dibujar_mundo(mundo);
        #endif
    }

    //TODO Recoger metricas de la simulacion: Cantidad de muertes, supervivientes, posiciones de los mismos, etc...

    recoger_metricas(mundo);
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
    return 0;
}

/**
 * Esta funcion se encarga simular un ciclo en la simulacion: Mover a las personas, infectar a los demas, muertes, etc...
 **/
void simular_ciclo(Persona **mundo)
{
    int i, j;
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            if (mundo[i + j * TAMANNO_MUNDO] != NULL) //FIXME esta busqueda se puede optimizar si guardamos las posiciones de las personas que estan en la matriz... :)
            {
                Persona *persona_tmp = mundo[i + j * TAMANNO_MUNDO];

                Tupla pos_persona = mover_persona(mundo, persona_tmp); //Movemos la persona en el mundo

                if (persona_tmp->estado == INFECTADO_ASINTOMATICO || persona_tmp->estado == INFECTADO_SINTOMATICO)
                    infectar(mundo, pos_persona);
            }
        }
    }
}


void recoger_metricas(Persona** mundo)
{
    int cantidad_infectados = 0;
    int i, j;
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            if (mundo[i + j * TAMANNO_MUNDO] != NULL) //FIXME esta busqueda se puede optimizar si guardamos las posiciones de las personas que estan en la matriz... :)
            {
                if(mundo[i + j * TAMANNO_MUNDO]->estado == INFECTADO_ASINTOMATICO || mundo[i + j * TAMANNO_MUNDO]->estado == INFECTADO_SINTOMATICO)
                {
                    cantidad_infectados++;
                }

            }
        }
    }

    printf("Tamanno Poblacion : %d, Numero de infectados: %d\n",TAMANNO_POBLACION, cantidad_infectados);
    if(TAMANNO_POBLACION == cantidad_infectados)
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
                if(mundo[posible_persona.val1 + posible_persona.val2 * TAMANNO_MUNDO] != NULL)
                {
                    //TODO tener en cuenta la gente vacunada e immune
                    mundo[posible_persona.val1 + posible_persona.val2 * TAMANNO_MUNDO]->estado = INFECTADO_SINTOMATICO;
                }
            }
            
        }
    }
}
Tupla mover_persona(Persona **mundo, Persona *persona)
{
    int flag = 0;
    do //Este bucle se encarga de que cada persona se mueva, si no es capaz de moverse, se le asignara una nueva velocidad, para que este pueda moverse por el mundo.
    {
        Tupla nueva_pos = persona->posicion;
        Tupla vieja_pos = persona->posicion;

        //Asignamos una velocidad aleatoria entre -VELOCIDAD_MAX y VELOCIDAD_MAX
        persona->velocidad.val1 = random_interval(-VELOCIDAD_MAX, VELOCIDAD_MAX);
        persona->velocidad.val2 = random_interval(-VELOCIDAD_MAX, VELOCIDAD_MAX);

        //Calculamos la nueva posicion de la persona
        nueva_pos.val1 = nueva_pos.val1 + persona->velocidad.val1;
        nueva_pos.val2 = nueva_pos.val2 + persona->velocidad.val2;

        //Comprobamos si el movimiento es posible
        if ((nueva_pos.val1 >= 0 && nueva_pos.val1 < TAMANNO_MUNDO) && (nueva_pos.val2 >= 0 && nueva_pos.val2 < TAMANNO_MUNDO)) //Primero comprobamos que la posicion sea valida dentro de la matriz 2D
        {
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

        if (i == 0) //Elegimos el paciente 0
        {
            persona_tmp->estado = INFECTADO_SINTOMATICO;
            printf("Posicion infectado: (%d,%d)\n", persona_tmp->posicion.val1, persona_tmp->posicion.val2);
        }
        mundo[pos.val1 + pos.val2 * TAMANNO_MUNDO] = persona_tmp;
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
                    printf("X      ");
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

//Si has llegado hasta aqui, te has ganado una galleta 🍪