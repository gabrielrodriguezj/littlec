/* Programa 5 de demostración de littleC.

   Un ejemplo más riguroso de argumentos de función.
*/

int f1(int a, int b)
{
    int cont;

    print("en f1");

    cont = a;
    do {
        print(cont);
    } while(cont=cont-1);

    print(a); print(b);
    print(a*b);
    return a*b;
}

int f2(int a, int x, int y)
{
    print(a); print(x);
    print(x / a);
    print(y*x);

    return 0;
}

int main()
{
    f2(10, f1(10, 20), 99);

    return 0;
}