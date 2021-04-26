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

enum Direccion
{
    ARRIBA,
    ABAJO,
    IZQUIERDA,
    DERECHA,
    DIAGONAL_SUPERIOR_IZQUIERDA,
    DIAGONAL_SUPERIOR_DERECHA,
    DIAGONAL_INFERIOR_IZQUIERDA,
    DIAGONAL_INFERIOR_DERECHA,

}

typedef struct
{
    int val1;
    int val2;
} Tupla;

typedef struct
{
    int valido; //Usado para indicar si esta persona existe o no en el mundo.
    int edad;   // Entre 0 y 110
    enum Estado estado;
    double probabilidad_muerte; // Usado en caso estar infectado
    Tupla posicion;             //Posicion x e y dentro del mundo
    Tupla velocidad;            // Representa la direccion y la velocidad del movimiento de la persona
} Persona;

//##################################################

//######################### Constantes #########################

#define TAMANNO_MUNDO 10    //Tamaño de la matriz del mundo
#define TAMANNO_POBLACION 20 //Cantidad de personas en el mundo

#define VELOCIDAD_MAX 2 //Define la velocidad maxima a la que una persona podra viajar por el mundo. El numero indica el numero de "casillas"

//##################################################

void dibujar_mundo(const Persona *mundo);
int comprobar_posicion(const Tupla posiciones_asignadas[TAMANNO_POBLACION], const Tupla posicion);
void inicializar_mundo(Persona *mundo);

int main(int argc, char const *argv[])
{
    assert(TAMANNO_MUNDO * TAMANNO_MUNDO > TAMANNO_POBLACION);

    srand(time(NULL));
    Persona *mundo = (Persona *)malloc(TAMANNO_MUNDO * TAMANNO_MUNDO * sizeof(Persona));
    if (mundo == NULL)
    {
        printf("Error al asignar memoria al mundo");
        return 1;
    }
    inicializar_mundo(mundo);
    dibujar_mundo(mundo);

    free(mundo);
    return 0;
}

void inicializar_mundo(Persona *mundo)
{
    int i, j, k;
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            mundo[i + j * TAMANNO_MUNDO].valido = 0;
            mundo[i + j * TAMANNO_MUNDO].edad = 0;
            mundo[i + j * TAMANNO_MUNDO].posicion.val1 = 0;
            mundo[i + j * TAMANNO_MUNDO].posicion.val2 = 0;
            mundo[i + j * TAMANNO_MUNDO].velocidad.val1 = 0;
            mundo[i + j * TAMANNO_MUNDO].velocidad.val2 = 0;
            mundo[i + j * TAMANNO_MUNDO].estado = SANO;
            
        }
    }
    k = 0; //Usado para indexar el vector de las posiciones
    Tupla posiciones_asignadas[TAMANNO_POBLACION] = {{0,0}};
    for (i = 0; i < TAMANNO_POBLACION; i++)
    {
        Persona persona_tmp;

        persona_tmp.valido = 1;
        persona_tmp.edad = rand() % 111;

        //TODO Decidir el paciente 0

        if(i == 0)
        {
            persona_tmp.estado = INFECTADO_SINTOMATICO;
        }
        Tupla pos;

        int flag = 0; // 0 si no existe nadie en esa posicion, 1 en el caso contrario.
        do
        {
            pos.val1 = rand() % (TAMANNO_MUNDO + 1);
            pos.val2 = rand() % (TAMANNO_MUNDO + 1);

            flag = comprobar_posicion(posiciones_asignadas, pos); //Comprobamos si existe algun otra persona en esa misma posicion.
        } while (flag != 0);

        posiciones_asignadas[k] = pos;
        k++;
        persona_tmp.posicion = pos;

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

void dibujar_mundo(const Persona *mundo)
{
    int i, j;
    for (i = 0; i < TAMANNO_MUNDO; i++)
    {
        for (j = 0; j < TAMANNO_MUNDO; j++)
        {
            if(mundo[i + j * TAMANNO_MUNDO].estado == INFECTADO_SINTOMATICO && mundo[i + j * TAMANNO_MUNDO].valido == 1)
                printf("X      ");
            else
                printf("%d      ", mundo[i + j * TAMANNO_MUNDO].valido);
        }
        printf("\n");
    }
}
