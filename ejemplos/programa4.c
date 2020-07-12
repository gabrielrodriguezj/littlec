/* Programa 4 de demostración de littleC.

   Este programa demuestra las funciones recursivas.
*/

/* devolver el factorial de i */
int factr(int i)
{
    if(i<2) {
        return 1;
    }
    else {
        return i * factr(i-1);
    }
}

int main()
{
    print("El factorial de 4 es: ");
    print(factr(4));

    return 0;
}