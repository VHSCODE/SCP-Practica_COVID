#include "stdlib.h"
#include "stdio.h"


//#################### Definiciones de tipos ##############################

enum Estado
{
    SANO,
    INFECTADO_ASINTOMATICO,
    INFECTADO_SINTOMATICO,
    RECUPERADO,
    VACUNADO,
    FALLECIDO
};

struct Persona
{
    int edad; // Entre 0 y 110
    enum Estado estado;
    double probabilidad_muerte; // Usado en caso estar infectado
    int posicion[2]; //Posicion x e y dentro del mundo
    int velocidad[2]; // Vector que representa la direccion y la velocidad del movimiento de la persona
};


//##################################################


int main(int argc, char const *argv[])
{
    return 0;
}
