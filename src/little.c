/* Un intérprete pequeño de C */

#include <stdio.h>
#include <setjmp.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define NUM_FUNC        100
#define NUM_GLOBAL_VARS 100
#define NUM_LOCAL_VARS  200
#define NUM_BLOCK       100
#define ID_LEN          31
#define FUNC_CALLS      31
#define NUM_PARAMS      31
#define PROG_SIZE       10000
#define LOOP_NEST       31

enum tok_types {DELIMITER, IDENTIFIER, NUMBER, KEYWORD, 
                TEMP, STRING, BLOCK};

/*  puede añadir aquí tokens de palabras clave de C adicionales */
enum tokens {ARG, CHAR, INT, IF, ELSE, FOR, DO, WHILE,
            SWITCH, RETURN, EOL, FINISHED, END};

/*  puede añadir aquí operadores dobles adicionales (como ->) */
enum double_ops {LT=1, LE, GT, GE, EQ, NE};

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
    WHILE_EXPECTED, QUOTE_EXPECTED, NOT_TEMP,
    TOO_MANY_LVARS, DIV_BY_ZERO
};

char *prog; /* posición actual en el código fuente */
char *p_buf; /* apunta al principio del búfer de programa */
jmp_buf e_buf; /* guarda el entorno para longjmp() */

/*  Un array de estas estructuras contendrá la información
    asociada con variables globales.
*/
struct var_type {
    char var_name[ID_LEN];
    int v_type;
    int value;
} global_vars[NUM_GLOBAL_VARS];

struct var_type local_var_stack[NUM_LOCAL_VARS];

struct func_type {
    char func_name[ID_LEN];
    int ret_type;
    char *loc; /* posición del punto de entrada en el archivo */
} func_table[NUM_FUNC];

int call_stack[NUM_FUNC];

struct commands { /* tabla de consulta de palabras clave */
    char command[20];
    char tok;
} table [] = { /* Las órdenes se tienen que introducir */
    "if", IF, /* en minúsculas en esta tabla. */
    "else", ELSE,
    "for", FOR,
    "do", DO,
    "while", WHILE,
    "char", CHAR,
    "int", INT,
    "return", RETURN,
    "end", END,
    "", END /* marca el final de la tabla */
};

char token[80];
char token_type, tok;

int functos; /* índice al tope de la pila de llamadas a función */
int func_index; /* índice a la tabla de función */
int gvar_index; /* índice a la tabla de variables globales */
int lvartos; /* índice a la pila de variables locales */
int ret_value; /* valor devuelto por la función */

void print(void), prescan(void);
void decl_global(void), call(void), putback(void);
void decl_local(void), local_push(struct var_type i);
void eval_exp(int *value), sntx_err(int error);
void exec_if(void),find_eob(void), exec_for(void);
void get_params(void), get_args(void);
void exec_while(void), func_push(int i), exec_do(void);
void assign_var(char *var_name, int value);
int load_program(char *p, char *fname), find_var(char *s);
void interp_block(void), func_ret(void);
int func_pop(void), is_var(char *s), get_token(void);
char *find_func(char *name);

int main(int argc, char *argv[])
{
    if(argc != 2){
        printf("Uso: littlec <archivo>\n");
        exit(1);
    }

    /* asignar memoria para el programa */
    if((p_buf = (char *) malloc(PROG_SIZE)) == NULL){
        printf("Fallo de asingación");
        exit(1);
    }

    /* cargar el programa a ejecutar */
    if(!load_program(p_buf, argv[1])) exit(1);
    if(setjmp(e_buf)) exit(1); /* inicializa el búfer de longjmp */

    gvar_index = 0: /* inicializa el índice de variables globales */

    /* puntero del programa al principio del búfer del programa */
    prog = p_buf;
    prescan(); /* busca la posición de todas las funciones
                  y variables globales en el programa */

    lvartos = 0; /* inicializa el índice de pila de vars. locales */
    functos = 0; /* inicializa el índice de pila de llamadas */

    /* configurar la llamada a main() */
    prog = find_func("main"): /* buscar punto de entrada al programa */

    if(!prog){ /* función main() incorrecta o ausente */
        printf("main() no encontrada. \n");
        exit(1);
    }

    prog--; /* retroceder al "("" de apertura */
    strcpy(token, "main");
    call(); /* iniciar la interpretación */

    return 0;
}

/*  Interpretar una instrucción individual o bloque de código.
    Cuando interp_block() vuelve de su llamada inicial, es que
    se ha encontrado la llave final (o un return) en main().
*/
void interp_block(void)
{
    int value;
    char block = 0;

    do{
        token_type = get_token();
        
        /*  Si se interpreta una instrucción individual, volver en
            el primer punto y coma.
        */

        /* ver el tipo de token */
        if(token_type == IDENTIFIER){
            /* No es palabra clave, por lo que se procesa expresión. */
            putback(); /* restaura el token a la secuencia de entrada
                          para un procesamiento posterior con eval_exp() */
            eval_exp(&value); /* procesa la expresión */
            if(*token != ';') sntx_err(SEMI_EXPECTED);
        }
        else if(token_type == BLOCK){ /* si es delimitador de bloque */
            if(*token == '{') /* es un bloque */
                block = 1; /* interpretar un bloque, no una instrucción */
            else return; /* es "}", por lo que ejecuta return */

        }
        else /* es palabra clave */
            switch(tok){
                case CHAR:
                case INT:       /* declara variables locales */
                    putback();
                    decl_local();
                    break;
                case RETURN:    /* vuelve de la llamada a una función */
                    func_ret();
                    return;
                case IF:        /* procesa una instrucción if */
                    exec_if();
                    break;
                case ELSE:      /* procesa una instrucción else */
                    find_eob(); /* busca el final del bloque else
                                   y continúa la ejecución */
                    break;
                case WHILE:     /* procesa un bucle while */
                    exec_while();
                    break;
                case DO:        /* procesa un bucle do-while */
                    exec_do();
                    break;
                case FOR:       /* procesa un bucle for */
                    exec_for();
                    break;
                case END:
                    exit(0);
            }
    } while (tok != FINISHED && block);
}

/* Cargar un programa. */
int load_program(char *p, char *fname)
{
    FILE *fp;
    int i = 0;

    if((fp = fopen(fname, "rb")) == NULL) return 0;

    i = 0;
    do{
        *p = getc(fp);
        p++; i++;
    } while(!feof(fp) && i < PROG_SIZE);

    if(*(p - 2) == 0x1a) * (p - 2) = '\0'; /* nulo final del programa */
    else *(p - 1) = '\0';
    fclose(fp);
    return 1;
}

/* Buscar la posición de todas las funciones en el programa y
   almacenar todas las variables globales. */
void prescan(void)
{
    char *p, *tp;
    char temp[32];
    int datatype;
    int brace = 0; /* cuando es 0, esta variable nos indica
                      que la posición del código fuente
                      actual está fuera de cualquier función. */
    
    p = prog;
    func_index = 0;
    do{
        while(brace){ /* salta el código de las funciones */
            get_token();
            if(*token == '{') brace++;
            if(*token == '}') brace--;
        }

        tp = prog; /* guardar la posición actual */
        get_token();
        /* tipo de variable global o tipo de vuela de función */
        if(tok == CHAR || tok == INT){
            datatype = tok; /* guardar tipo de dato */
            get_token();
            if(token_type == IDENTIFIER){
                strcpy(temp, token);
                get_token();
                if(*token != '('){ /* tiene que ser variable globarl */
                    prog = tp; /* volver al principio de la declaración */
                    decl_global();
                }
                else if(*token == '('){ /* tiene que ser función */
                    func_table[func_index].loc = prog;
                    func_table[func_index].ret_type = datatype;
                    strcpy(func_table[func_index].func_name, temp);
                    func_index++;
                    while(*prog != ')') prog++;
                    prog++;
                    /* ahora prog apunta a la llave de apertura
                       de la función */
                }
                else putback();
            }
        }
        else if(*token == '{') brace++;
    }while(tok != FINISHED);
    prog = p;
}

/* Devolver el punto de entrada de la función especificada.
   Si no lo encuentra devuelve NULL.
*/
char *find_func(char *name)
{
    register int i;

	for (i = 0; i < func_index; i++)
		if (!strcmp(name, func_table[i].func_name))
			return func_table[i].loc;

	return NULL;
}

/* Declarar una variable global. */
void decl_global(void)
{
	int vartype;

	get_token();  /* obtiene el tipo */

	vartype = tok; /* guarda el tipo de variable */

	do { /* procesar una lista separada por comas */
		global_vars[gvar_index].v_type = vartype;
		global_vars[gvar_index].value = 0;  /* inicializa a 0 */
		get_token();  /* obtiene el nombre */
		strcpy(global_vars[gvar_index].var_name, token);
		get_token();
		gvar_index++;
	} while (*token == ',');
	if (*token != ';') sntx_err(SEMI_EXPECTED);
}

/* Declarar una variable local. */
void decl_local(void)
{
	struct var_type i;

	get_token();  /* obtiene el tipo */

	i.v_type = tok;
	i.value = 0;  /* inicializa a 0 */

	do { /* procesar una lista separada por comas */
		get_token(); /* obtiene el nombre de variable */
		strcpy(i.var_name, token);
		local_push(i);
		get_token();
	} while (*token == ',');
	if (*token != ';') sntx_err(SEMI_EXPECTED);
}

/* Llamar a una función. */
void call(void)
{
	char *loc, *temp;
	int lvartemp;

	loc = find_func(token); /* buscar el punto de entrada */
	if (loc == NULL)
		sntx_err(FUNC_UNDEF); /* funcion no definida */
	else {
		lvartemp = lvartos;  /* guardar índice pila variables locales (save local var stack index) */
		get_args();  /* obtener los argumentos de la función */
		temp = prog; /* guardar la posición de retorno */
		func_push(lvartemp);  /* guardar índice pila variables locales (save local var stack index) */
		prog = loc;  /* inicializar prog al principio de la función */
		get_params(); /* cargar los parámetros de la función con 
                         los valores de los argumentos */
		interp_block(); /* interpretar la función */
		prog = temp; /* reinicializar el puntero del programa */
		lvartos = func_pop(); /* reinicializar pila de variables locales */
	}
}

/* Poner los argumentos de la función en la pila
   de variables locales. */
void get_args(void)
{
	int value, count, temp[NUM_PARAMS];
	struct var_type i;

	count = 0;
	get_token();
	if (*token != '(') sntx_err(PAREN_EXPECTED);

	/* procesar una lista de valores separados por comas */
	do {
		eval_exp(&value);
		temp[count] = value;  /* lo guarda temporalmente */
		get_token();
		count++;
	} while (*token == ',');
	count--;
	/* ahora, meter en pila de variables locales en orden inverso */
	for (; count >= 0; count--) {
		i.value = temp[count];
		i.v_type = ARG;
		local_push(i);
	}
}

/* Obtener los parámetros de la función. */
void get_params(void)
{
	struct var_type *p;
	int i;

	i = lvartos - 1;
	do { /* procesar una lista de parámetros separados por comas */
		get_token();
		p = &local_var_stack[i];
		if (*token != ')') {
			if (tok != INT && tok != CHAR)
				sntx_err(TYPE_EXPECTED);

			p->v_type = token_type;
			get_token();

			/* vincular el nombre del parámetro con el argumento que
               ya está en la pila de variables locales */
			strcpy(p->var_name, token);
			get_token();
			i--;
		}
		else break;
	} while (*token == ',');
	if (*token != ')') sntx_err(PAREN_EXPECTED);
}

/* Volver de una función. */
void func_ret(void)
{
	int value;

	value = 0;
	/* obtener el valorde retorno, si existe */
	eval_exp(&value);

	ret_value = value;
}

/* Meter en la pila una variable local. */
void local_push(struct var_type i)
{
	if (lvartos >= NUM_LOCAL_VARS) {
		sntx_err(TOO_MANY_LVARS);
	}
	else {
		local_var_stack[lvartos] = i;
		lvartos++;
	}
}

/* Sacar el índice de la pila de variables locales. */
int func_pop(void)
{
	functos--;
	if (functos < 0) sntx_err(RET_NOCALL);
	return call_stack[functos];
}

/* Meter en la pila el índice de la pila de
   variables locales. */
void func_push(int i)
{
	if (functos > NUM_FUNC)
        sntx_err(NEST_FUNC);
	call_stack[functos] = i;
	functos++;
}

/* Asignar un valor a una variable. */
void assign_var(char *var_name, int value)
{
	register int i;

	/* primero, mirar a ver si es una variable local */
	for (i = lvartos - 1; i >= call_stack[functos - 1]; i--) {
		if (!strcmp(local_var_stack[i].var_name, var_name)) {
			local_var_stack[i].value = value;
			return;
		}
	}
	if (i < call_stack[functos - 1])
		/* si no es local, se prueba la tabla de variables locales */
		for (i = 0; i < NUM_GLOBAL_VARS; i++)
			if (!strcmp(global_vars[i].var_name, var_name)) {
				global_vars[i].value = value;
				return;
			}
	sntx_err(NOT_VAR); /* variable no encontrada */
}

/* Busca el valor de  una variable. */
int find_var(char *s)
{
	register int i;

	/* primero, mirar a ver si es una variable local */
	for (i = lvartos - 1; i >= call_stack[functos - 1]; i--)
		if (!strcmp(local_var_stack[i].var_name, token))
			return local_var_stack[i].value;

	/* en otro caso, probar las variables globales */
	for (i = 0; i < NUM_GLOBAL_VARS; i++)
		if (!strcmp(global_vars[i].var_name, s))
			return global_vars[i].value;

	sntx_err(NOT_VAR); /* variable no encontrada */
	return -1;
}

/* Determinar si un identificador es una variabl. Devuelve 
   1 si se encuentra variable; 0 en otro caso.
*/
int is_var(char *s)
{
	register int i;

	/* primero, mirar a ver si es una variable local */
	for (i = lvartos - 1; i >= call_stack[functos - 1]; i--)
		if (!strcmp(local_var_stack[i].var_name, token))
			return 1;

	/* en cualquier otro caso, probar las variables globales */
	for (i = 0; i < NUM_GLOBAL_VARS; i++)
		if (!strcmp(global_vars[i].var_name, s))
			return 1;

	return 0;
}

/* Ejecutar una instrucción if. */
void exec_if(void)
{
	int cond;

	eval_exp(&cond); /* obtiene la expresión if */

	if (cond) { /* si es verdadera, procesa el objetivo(target) del IF */
		interp_block();
	}
	else { /* en otro caso, saltael bloque IF y
              procesa el ELSE, si existe */
		find_eob(); /* busca el inicio de la siguiente línea */
		get_token();

		if (tok != ELSE) {
			putback(); /* restaura el token si no hay ELSE */
			return;
		}
		interp_block();
	}
}

/* Ejecutar un bucle while. */
void exec_while(void)
{
	int cond;
	char *temp;

	putback();
	temp = prog; /* guarda la posición del principio del while */
	get_token();
	eval_exp(&cond); /* comprueba la expresión condicional */
	if (cond) interp_block(); /* si verdadero, interpreta */
	else {  /* en otro caso, salta el bucle */
		find_eob();
		return;
	}
	prog = temp;  /* retroceder al principio del bucle */
}

/* Ejecutar un bucle do. */
void exec_do(void)
{
	int cond;
	char *temp;

	putback();
	temp = prog;  /* guarda la posición del principio del bucle do */

	get_token(); /* obtiene el principio del bucle do */
	interp_block(); /* interpretar el bucle */
	get_token();
	if (tok != WHILE) sntx_err(WHILE_EXPECTED);
	eval_exp(&cond); /* comprueba la condición del bucle */
	if (cond) prog = temp; /* si es verdadera, itera;
							  en caso contrario, continúa */
}

/* Buscar el fin de un bloque. */
void find_eob(void)
{
	int brace;

	get_token();
	brace = 1;
	do {
		get_token();
		if (*token == '{') brace++;
		else if (*token == '}') brace--;
	} while (brace);
}

/* Ejecutar un bucle for. */
void exec_for(void)
{
	int cond;
	char *temp, *temp2;
	int brace;

	get_token();
	eval_exp(&cond);  /* expresión de inicialización */
	if (*token != ';') sntx_err(SEMI_EXPECTED);
	prog++; /* pasa el ; */
	temp = prog;
	for (;;) {
		eval_exp(&cond);  /* comprueba la condición */
		if (*token != ';') sntx_err(SEMI_EXPECTED);
		prog++; /* pasa el ; */
		temp2 = prog;

		/* buscar el principio del bloque for */
		brace = 1;
		while (brace) {
			get_token();
			if (*token == '(') brace++;
			if (*token == ')') brace--;
		}

		if (cond) interp_block();  /* si es verdadera interpreta */
		else {  /* en caso contrario salta el bucle */
			find_eob();
			return;
		}
		prog = temp2;
		eval_exp(&cond); /* incrementa */
		prog = temp;  /* retrocede al principio */
	}
}