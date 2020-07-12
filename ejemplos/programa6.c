/* Programa 6 de demostración de littleC.

   Las instrucciones de bucle. 
*/

int main()
{
    int a;
    char c;

    /* while */
    puts("Introduce un número: ");
    a = getnum();
    while(a) {
        print(a);
        print(a*a);
        puts("");
        a = a - 1;
    }

    /* do-while */
    puts("introduce caracteres, 't' para terminar");
    do {
        c = getche();
    } while(c != 't');

    /* for */
    for(a=0; a<10; a=a+1){
        print(a);
    }

    return 0;
}