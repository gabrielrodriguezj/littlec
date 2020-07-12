/* Programa 1 de demostración de littleC.

   Este programa demuestra todas las características
   de C que reconoce little C.
*/

int i, j;   /* variables globales */
char c;

int main()
{
    int i, j;   /* variables globales */

    puts("Programa de demostración de littleC.");

    print_alfa();

    do{
        puts("introduce un número (0 para salir): ");
        i = getnum();
        if(i < 0){
            puts("los números han de ser postivos, prueba de nuevo ");
        }
        else{
            for(j = 0; j < i; j=j+1){
                print(j);
                print("la suma es ");
                print(suma(j));
                puts("");
            }
        }
    } while(i!=0);

    return 0;
}

/* sumar los valores entre 0 y num. */
int suma(int num)
{
    int suma_parcial;

    suma_parcial = 0;

    while(num) {
        suma_parcial = suma_parcial + num;
        num = num - 1;
    }
    return suma_parcial;
}

/* Imprimir el alfabeto. */
int print_alfa()
{
    for(c = 'A'; c<='Z'; c = c + 1){
        putch(c);
    }
    puts("");

    return 0;
}