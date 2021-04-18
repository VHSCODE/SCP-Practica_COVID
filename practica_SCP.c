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

struct Tupla
{
    int val1;
    int val2;
};

struct Persona
{
    int valido; //Usado para indicar si esta persona existe o no en el mundo.
    int edad;   // Entre 0 y 110
    enum Estado estado;
    double probabilidad_muerte; // Usado en caso estar infectado
    struct Tupla posicion;      //Posicion x e y dentro del mundo
    struct Tupla velocidad;     // Representa la direccion y la velocidad del movimiento de la persona
};

//##################################################

//######################### Constantes #########################

#define TAMANNO_MUNDO 100    //Tamaño de la matriz del mundo
#define TAMANNO_POBLACION 15 //Cantidad de personas en el mundo

#define VELOCIDAD_MAX 2 //Define la velocidad maxima a la que una persona podra viajar por el mundo. El numero indica el numero de "casillas"

//##################################################



void dibujar_mundo(const struct Persona *mundo[TAMANNO_MUNDO][TAMANNO_MUNDO]);
int comprobar_posicion(const struct Tupla *posiciones_asignadas[TAMANNO_POBLACION], const struct Tupla *posicion);

int main(int argc, char const *argv[])
{
    assert(TAMANNO_MUNDO > TAMANNO_POBLACION);

    srand(time(NULL));
    struct Persona mundo[TAMANNO_MUNDO][TAMANNO_MUNDO] = {{0}};

    inicializar_mundo(&mundo);

    printf("%d\n", mundo[0][0]);
    //dibujar_mundo(&mundo);

    return 0;
}

void inicializar_mundo(struct Persona *mundo[TAMANNO_MUNDO][TAMANNO_MUNDO])
{
    int i, j, k;
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            mundo[i][j]->valido = 0;
            mundo[i][j]->edad = 0;
            mundo[i][j]->posicion.val1 = 0;
            mundo[i][j]->posicion.val2 = 0;
            mundo[i][j]->velocidad.val1 = 0;
            mundo[i][j]->velocidad.val2 = 0;

        }
    }

    k = 0; //Usado para indexar el vector de las posiciones
    struct Tupla posiciones_asignadas[TAMANNO_POBLACION];
    for (i = 0; i < TAMANNO_POBLACION; i++)
    {
        for (j = 0; j < TAMANNO_POBLACION; j++)
        {

            struct Persona persona_tmp;
            persona_tmp.valido = 1;
            persona_tmp.edad = rand() % 111;

            //TODO Decidir el paciente 0
            struct Tupla pos;

            int flag = 0; // 0 si no existe nadie en esa posicion, 1 en el caso contrario.
            do
            {
                pos.val1 = rand() % (TAMANNO_MUNDO + 1);
                pos.val2 = rand() % (TAMANNO_MUNDO + 1);

                flag = comprobar_posicion(&posiciones_asignadas, &pos); //Comprobamos si existe algun otra persona en esa misma posicion.
            } while (flag != 0);

            persona_tmp.posicion = pos;

            *mundo[persona_tmp.posicion.val1][persona_tmp.posicion.val2] = persona_tmp;
        }
    }
}

int comprobar_posicion(const struct Tupla *posiciones_asignadas[TAMANNO_POBLACION], const struct Tupla *posicion)
{

    int i;

    for (i = 0; i < TAMANNO_POBLACION; i++)
    {
        if (posiciones_asignadas[i]->val1 == posicion->val1 && posiciones_asignadas[i]->val2 == posicion->val2)
        {
            return 1;
        }
    }
    return 0;
}

void dibujar_mundo(const struct Persona *mundo[TAMANNO_MUNDO][TAMANNO_MUNDO])
{
    int i, j;
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            if (mundo[i][j]->valido == 1)
            {
                printf("%d      ", mundo[i][j]->valido);
            }

            printf("\n");
        }
    }
}