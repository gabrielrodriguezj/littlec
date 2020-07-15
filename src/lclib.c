/****** Funciones de biblioteca internas *******/

/* Añada aquí las funciones propias. */


/*#include <conio.h>*/  /* si el compilador no admite este
                       archivo de cabecera, elimínelo */
#include <stdio.h>
#include <stdlib.h>

extern char *prog; /* apunta a la posición actual del programa */
extern char token[80]; /* guarda en forma de cadena el token */
extern char token_type; /* contiene el tipo de token */
extern char tok; /* guarda la representación interna del token */

enum tok_types {DELIMITER, IDENTIFIER, NUMBER, KEYWORD,
	            TEMP, STRING, BLOCK};

/*  Éstas son constantes utilizadas para llamar a sntx_err()
    cuando se produce un error de sintaxis. Y más si se quiere.
    NOTA: SYNTAX es un mensaje de error genérico utilizado
    cuando no hay nada más apropiado.
*/
enum error_msg
{   
    SYNTAX, UNBAL_PARENS, NO_EXP, EQUALS_EXPECTED,
    NOT_VAR, PARAM_ERR, SEMI_EXPECTED,
    UNBAL_BRACES, FUNC_UNDEF, TYPE_EXPECTED,
    NEST_FUNC, RET_NOCALL, PAREN_EXPECTED,
    WHILE_EXPECTED, QUOTE_EXPECTED, NOT_STRING,
    TOO_MANY_LVARS, DIV_BY_ZERO
};

int get_token(void);
void sntx_err(int error), eval_exp(int *result);
void putback(void);

/* Obtener un carácter de la consola. (Úsese getchar() si 
   el compilador no admite getche().) */
int call_getche(void)
{
	char ch;
	//ch = getche();
	ch = getchar();
	while (*prog != ')') prog++;
	prog++;   /* avanza al final de la línea */
	return ch;
}

/* Mostrar un carácter en la pantalla. */
int call_putch(void)
{
	int value;

	eval_exp(&value);
	printf("%c", value);
	return value;
}

/* Llamar a puts(). */
int call_puts(void)
{
	get_token();
	if (*token != '(') sntx_err(PAREN_EXPECTED);
	get_token();
	if (token_type != STRING) sntx_err(QUOTE_EXPECTED);
	puts(token);
	get_token();
	if (*token != ')') sntx_err(PAREN_EXPECTED);

	get_token();
	if (*token != ';') sntx_err(SEMI_EXPECTED);
	putback();
	return 0;
}

/* Una función de salida por consola incorporada. */
int print(void)
{
	int i;

	get_token();
	if (*token != '(')  sntx_err(PAREN_EXPECTED);

	get_token();
	if (token_type == STRING) { /* mostrar una cadena */
		printf("%s ", token);
	}
	else {  /* mostrar un número */
		putback();
		eval_exp(&i);
		printf("%d ", i);
	}

	get_token();

	if (*token != ')') sntx_err(PAREN_EXPECTED);

	get_token();
	if (*token != ';') sntx_err(SEMI_EXPECTED);
	putback();
	return 0;
}

/* Leer un entero desde el teclado. */
int getnum(void)
{
	char s[80];

	/* Si el compilador admite la función get usar esta sección de código */
	/*
    gets(s);
	while (*prog != ')') prog++;
    prog++;  /* avanza al final de la línea * /
	return atoi(s);
	*/
	
	/* Si el compilador no admite la función get usar esta sección de código */
	if (fgets(s, sizeof(s), stdin) != NULL) {
		while (*prog != ')') prog++;
		prog++;  /* avanza al final de la línea */
		return atoi(s);
	}
	else {
		return 0;
	}

	
}