#include "stdlib.h"
#include "stdio.h"
#include "time.h"
#include "assert.h"

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
    int edad;   // Entre 0 y 110
    enum Estado estado;
    double probabilidad_muerte; // Usado en caso estar infectado
    Tupla posicion;             //Posicion x e y dentro del mundo
    Tupla velocidad;            // Representa la direccion y la velocidad del movimiento de la persona
} Persona;

//##################################################

//######################### Constantes #########################

#define TAMANNO_MUNDO 10     //Tamaño de la matriz del mundo
#define TAMANNO_POBLACION 20 //Cantidad de personas en el mundo

#define VELOCIDAD_MAX 2 //Define la velocidad maxima a la que una persona podra viajar por el mundo. El numero indica el numero de "casillas"

//##################################################

void dibujar_mundo(Persona **mundo);
int comprobar_posicion(const Tupla posiciones_asignadas[TAMANNO_POBLACION], const Tupla posicion);
void inicializar_mundo(Persona **mundo);

int main(int argc, char const *argv[])
{
    assert(TAMANNO_MUNDO * TAMANNO_MUNDO > TAMANNO_POBLACION);

    srand(time(NULL));
    Persona **mundo = malloc(TAMANNO_MUNDO * TAMANNO_MUNDO * sizeof(Persona *));
    if (mundo == NULL)
    {
        printf("Error al asignar memoria al mundo");
        return 1;
    }
    inicializar_mundo(mundo);
    dibujar_mundo(mundo);

    //TODO hacer free de cada struct en el mundo
    free(mundo);
    return 0;
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
        persona_tmp->edad = rand() % 111;

       
        Tupla pos;

        int flag = 0; // 0 si no existe nadie en esa posicion, 1 en el caso contrario.
        do
        {
            pos.val1 = rand() % (TAMANNO_MUNDO); // 0-- 10
            pos.val2 = rand() % (TAMANNO_MUNDO);

            flag = comprobar_posicion(posiciones_asignadas, pos); //Comprobamos si existe algun otra persona en esa misma posicion.
        } while (flag != 0);

        posiciones_asignadas[k] = pos;
        k++;
        persona_tmp->posicion = pos;
        if (i == 0)
        {
            persona_tmp->estado = INFECTADO_SINTOMATICO;
            printf("Posicion infectado: (%d,%d)\n",persona_tmp->posicion.val1,persona_tmp->posicion.val2);
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
                    printf("1      ");
            }
            else
            {
                 printf("0      ");
            }
            
        }
        printf("\n");
    }
}
