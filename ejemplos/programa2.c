/* Programa 2 de demostración de littleC.
   
   Ejemplo de bucles anidados. 
*/

int main()
{
    int i, j, k;

    for(i = 0; i < 5; i = i +1){
        for(j = 0; j < 5; j = j +1){
            for(k = 0; k < 5; k = k +1){
                print(i);
                print(j);
                print(k);
                puts("");
            }
        }
    }
    puts("terminado");

    return 0;
}
