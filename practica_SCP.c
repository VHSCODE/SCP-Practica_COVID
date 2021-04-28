#include "stdlib.h"
#include "stdio.h"
#include "time.h"
#include "assert.h"

#define DEBUG 1
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

#define TAMANNO_MUNDO 10     //Tamaño de la matriz del mundo
#define TAMANNO_POBLACION 20 //Cantidad de personas en el mundo

#define VELOCIDAD_MAX 5 //Define la velocidad maxima a la que una persona podra viajar por el mundo. El numero indica el numero de "casillas"

//##################################################

void dibujar_mundo(Persona **mundo);
int comprobar_posicion(const Tupla posiciones_asignadas[TAMANNO_POBLACION], const Tupla posicion);
void inicializar_mundo(Persona **mundo);
void simular_ciclo(Persona **mundo);
int random_interval(int min, int max);



int main(int argc, char const *argv[])
{
    assert(TAMANNO_MUNDO * TAMANNO_MUNDO > TAMANNO_POBLACION);

    srand(time(NULL));

    long iteraciones = atol(argv[1]);

    Persona **mundo = malloc(TAMANNO_MUNDO * TAMANNO_MUNDO * sizeof(Persona *));
    if (mundo == NULL)
    {
        printf("Error al asignar memoria al mundo");
        return 1;
    }

    inicializar_mundo(mundo);

    int iteraciones_realizadas = 0;

    for (iteraciones_realizadas; iteraciones_realizadas < iteraciones; iteraciones_realizadas++)
    {
        simular_ciclo(mundo);

        printf("Iteracion %d: \n",iteraciones_realizadas);
        dibujar_mundo(mundo);
    }
    
    //FIXME Investigar porque tenemos un double free,( Seguramente porque hacemos free en simular_ciclo)
    /**
    //Liberamos la memoria asignada
    int i, j;
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            if(mundo[i +j * TAMANNO_MUNDO] != NULL)
                free(mundo[i + j * TAMANNO_MUNDO]);
        }
    }
    **/
   
    free(mundo);
    return 0;
}

void simular_ciclo(Persona **mundo)
{
    int i = 0;
    int j = 0;

    for (i; i < TAMANNO_MUNDO; i++)
    {
        for (j; j < TAMANNO_MUNDO; j++)
        {
            if(mundo[i +j * TAMANNO_MUNDO] != NULL)
            {

                //Asignamos una velocidad aleatoria entre 1 y VELOCIDAD_MAX

                mundo[i +j * TAMANNO_MUNDO]->velocidad.val1 = random_interval(-VELOCIDAD_MAX, VELOCIDAD_MAX);
                mundo[i +j * TAMANNO_MUNDO]->velocidad.val2 = random_interval(-VELOCIDAD_MAX, VELOCIDAD_MAX);             
                Persona* persona_tmp =  mundo[i +j * TAMANNO_MUNDO];
                
                Tupla nueva_pos = persona_tmp->posicion;
                Tupla vieja_pos = persona_tmp->posicion;

                

                nueva_pos.val1 = nueva_pos.val1 +  mundo[i +j * TAMANNO_MUNDO]->velocidad.val1;
                nueva_pos.val2 = nueva_pos.val2 +  mundo[i +j * TAMANNO_MUNDO]->velocidad.val1;
                
                //Comprobamos si el movimiento es posible

                if((nueva_pos.val1 >= 0 && nueva_pos.val1 < TAMANNO_MUNDO) && (nueva_pos.val2 >= 0 && nueva_pos.val2 < TAMANNO_MUNDO)) //Primero comprobamos que la posicion sea valida dentro de la matriz 2D
                {
                    if(mundo[nueva_pos.val1 + nueva_pos.val2 * TAMANNO_MUNDO] == NULL) //Ahora comprobamos que la casilla a moverse este vacia
                    {
                        //mundo[nueva_pos.val1 + nueva_pos.val2 * TAMANNO_MUNDO] = malloc(sizeof(Persona));
                        mundo[nueva_pos.val1 + nueva_pos.val2 * TAMANNO_MUNDO] = persona_tmp;
                        //free(mundo[vieja_pos.val1 + vieja_pos.val2 * TAMANNO_MUNDO]);
                        mundo[vieja_pos.val1 + vieja_pos.val2 * TAMANNO_MUNDO] = NULL;
                        free(persona_tmp);
                    }

                    //TODO Quizas randomizar otra vez el vector de velocidad si el movimiento no es posible ???
                }
            }
        }
    }
    //TODO implementar radio de infeccion e infectar
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
        persona_tmp->edad = random_interval(0,110);

        Tupla pos;

        int flag = 0; // 0 si no existe nadie en esa posicion, 1 en el caso contrario.
        do
        {
            pos.val1 = random_interval(0,TAMANNO_MUNDO -1);
            pos.val2 = random_interval(0,TAMANNO_MUNDO -1);

            flag = comprobar_posicion(posiciones_asignadas, pos); //Comprobamos si existe algun otra persona en esa misma posicion.
        } while (flag != 0);

        posiciones_asignadas[k] = pos;
        k++;
        persona_tmp->posicion = pos;
        if (i == 0)
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
                if (mundo[i + j * TAMANNO_MUNDO]->estado == INFECTADO_SINTOMATICO)
                    printf("X      ");
                else
                {
                    #ifdef DEBUG
                    printf("%d      ",mundo[i + j * TAMANNO_MUNDO]->identificador);
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