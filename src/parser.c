/* Analizador recursivo descendente para expresiones enteras
   que puede incluir variables y llamadas a función.
*/

#include <setjmp.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NUM_FUNC        100
#define NUM_GLOBAL_VARS 100
#define NUM_LOCAL_VARS  200
#define ID_LEN          32
#define FUNC_CALLS      31
#define PROG_SIZE       10000
#define FOR_NEST        31

enum tok_types {DELIMITER, IDENTIFIER, NUMBER, KEYWORD,
	            TEMP, STRING, BLOCK
};

enum tokens {ARG, CHAR, INT, IF, ELSE, FOR, DO, WHILE,
	         SWITCH, RETURN, EOL, FINISHED, END};

enum double_ops {LT = 1, LE, GT, GE, EQ, NE};

/* Éstas son constantes utilizadas para llamar a sntx_err()
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

extern char *prog; /* posición actual en el código fuente */
extern char *p_buf; /* apunta al principio del búfer de programa */
extern jmp_buf e_buf; /* guarda el entorno para longjmp() */

/*  Un array de estas estructuras contendrá la información
    asociada con variables globales.
*/
extern struct var_type {
	char var_name[ID_LEN];
	int v_type;
	int value;
}  global_vars[NUM_GLOBAL_VARS];

/*  Esta es la pila de llamadas a función. */
extern struct func_type {
	char func_name[ID_LEN];
	int ret_type;
	char *loc;  /* punto de entrada a la función en el archivo */
} func_stack[NUM_FUNC];

/* Tabla de palabras clave */
extern struct commands {
	char command[20];
	char tok;
} table[];

/* Las funciones de "biblioteca estándar" se declaran aquí
   para que puedan ponerse en la tabla de funciones interna que
   vienea continuación.
 */
int call_getche(void), call_putch(void);
int call_puts(void), print(void), getnum(void);

struct intern_func_type {
	char *f_name; /* nombre de la función */
	int(*p)(void);   /* puntero a la función */
} intern_func[] = {
	"getche", call_getche,
	"putch", call_putch,
	"puts", call_puts,
	"print", print,
	"getnum", getnum,
	"", 0  /* nulo que termina la lista */
};

extern char token[80]; /* representación como cadena del token */
extern char token_type; /* contiene el tipo de token */
extern char tok; /* representación interna del token */

extern int ret_value; /* valor devuelto por la función */

void eval_exp0(int *value);
void eval_exp(int *value);
void eval_exp1(int *value);
void eval_exp2(int *value);
void eval_exp3(int *value);
void eval_exp4(int *value);
void eval_exp5(int *value);
void atom(int *value);
void sntx_err(int error), putback(void);
void assign_var(char *var_name, int value);
int isdelim(char c), look_up(char *s), iswhite(char c);
int find_var(char *s), get_token(void);
int internal_func(char *s);
int is_var(char *s);
char *find_func(char *name);
void call(void);

/* Punto de entrada al analizador. */
void eval_exp(int *value)
{
	get_token();
	if (!*token) {
		sntx_err(NO_EXP);
		return;
	}
	if (*token == ';') {
		*value = 0; /* expresión vacía */
		return;
	}
	eval_exp0(value);
	putback(); /* devuelve el último token leído
                  a la secuencia de entrada */
}

/* Procesar una expresión de asignación */
void eval_exp0(int *value)
{
	char temp[ID_LEN];  /* guarda el nombre de la variable
                           que recibe la asignación */
	register int temp_tok;

	if (token_type == IDENTIFIER) {
		if (is_var(token)) {  /* si es una variable, comprueba si hay asignación */
			strcpy(temp, token);
			temp_tok = token_type;
			get_token();
			if (*token == '=') {  /* es una asignación */
				get_token();
				eval_exp0(value);  /* obtiene el valor a asignar */
				assign_var(temp, *value);  /* asigna el valor*/
				return;
			}
			else {  /* no es una asignación */
				putback();  /* restaura el token original */
				strcpy(token, temp);
				token_type = temp_tok;
			}
		}
	}
	eval_exp1(value);
}

/* Procesar operadores relacionales. */
void eval_exp1(int *value)
{
	int partial_value;
	register char op;
	char relops[7] = {
	  LT, LE, GT, GE, EQ, NE, 0
	};

	eval_exp2(value);
	op = *token;
	if (strchr(relops, op)) {
		get_token();
		eval_exp2(&partial_value);
		switch (op) {  /* realizar la operación relacional */
		case LT:
			*value = *value < partial_value;
			break;
		case LE:
			*value = *value <= partial_value;
			break;
		case GT:
			*value = *value > partial_value;
			break;
		case GE:
			*value = *value >= partial_value;
			break;
		case EQ:
			*value = *value == partial_value;
			break;
		case NE:
			*value = *value != partial_value;
			break;
		}
	}
}

/* Sumar o restar dos términos. */
void eval_exp2(int *value)
{
	register char  op;
	int partial_value;

	eval_exp3(value);
	while ((op = *token) == '+' || op == '-') {
		get_token();
		eval_exp3(&partial_value);
		switch (op) { /* add or subtract */
		case '-':
			*value = *value - partial_value;
			break;
		case '+':
			*value = *value + partial_value;
			break;
		}
	}
}

/* Multiplicar o dividir dos factores. */
void eval_exp3(int *value)
{
	register char  op;
	int partial_value, t;

	eval_exp4(value);
	while ((op = *token) == '*' || op == '/' || op == '%') {
		get_token();
		eval_exp4(&partial_value);
		switch (op) { /* multiplicación, división o módulo */
		case '*':
			*value = *value * partial_value;
			break;
		case '/':
			if (partial_value == 0) sntx_err(DIV_BY_ZERO);
			*value = (*value) / partial_value;
			break;
		case '%':
			t = (*value) / partial_value;
			*value = *value - (t * partial_value);
			break;
		}
	}
}

/* Es un + o - monario */
void eval_exp4(int *value)
{
	register char  op;

	op = '\0';
	if (*token == '+' || *token == '-') {
		op = *token;
		get_token();
	}
	eval_exp5(value);
	if (op)
		if (op == '-') *value = -(*value);
}

/* Procesar expresión con paréntesis. */
void eval_exp5(int *value)
{
	if (*token == '(') {
		get_token();
		eval_exp0(value);   /* obtener subexpresión */
		if (*token != ')') sntx_err(PAREN_EXPECTED);
		get_token();
	}
	else
		atom(value);
}

/* Buscar el valor del número, variable o función. */
void atom(int *value)
{
	int i;

	switch (token_type) {
	case IDENTIFIER:
		i = internal_func(token);
		if (i != -1) {  /* llamar a función de "biblioteca estándar" */
			*value = (*intern_func[i].p)();
		}
		else if (find_func(token)) { /* llamada a función de usuario */
			call();
			*value = ret_value;
		}
		else *value = find_var(token); /* obtener valor de la variable */
		get_token();
		return;
	case NUMBER: /* es una constante  numérica */
		*value = atoi(token);
		get_token();
		return;
	case DELIMITER: /* ver si es una constante de caracter */
		if (*token == '\'') {
			*value = *prog;
			prog++;
			if (*prog != '\'') sntx_err(QUOTE_EXPECTED);
			prog++;
			get_token();
			return;
		}
		if (*token == ')') return; /* procesar una expresión vacía */
		else sntx_err(SYNTAX); /* error  de sintaxis*/
	default:
		sntx_err(SYNTAX); /* error  de sintaxis */
	}
}

/* Mostrar un mensaje de error. */
void sntx_err(int error)
{
	char *p, *temp;
	int linecount = 0;
	register int i;

	static char *e[] = {
	  "error de sintaxis",
	  "paréntesis no balanceados",
	  "no hay expresión presente",
	  "se esperaba el signo igual",
	  "no es una variable",
	  "error de parámetro",
	  "falta punto y coma",
	  "llaves no balanceadas",
	  "función no definida",
	  "falta especificador de tipo",
	  "demasiadas llamadas a función anidadas",
	  "return sin llamada",
	  "se esperaba paréntesis",
	  "se esperaba while",
	  "se esperaba la comilla de cierre",
	  "no es una cadena",
	  "demasiadas variables locales",
	  "división por cero"
	};
	printf("\n%s", e[error]);
	p = p_buf;
	while (p != prog && *p != '\0') {  /* buscar el número de línea del error */
		p++;
		if (*p == '\r') {
			linecount++;

			/* mejora para determinar saltos de líneas en unix y windoes para así
				obtener el número correcto de la línea donde se encuentra el error */
			if (p == prog) {
				break;
			}
			/* determinar si en salto de línea de Windows o Unix */
			p++;
			/* si en un salto unix, regresar el apuntador */
			if (*p != '\n') {
				p--;
			}
        }
		else if (*p == '\n') {
			linecount++;
		}
		else if (*p == '\0') {
			linecount++;
		}
	}
	printf(" en la línea %d\n", linecount);

	/* siguientes 2 líneas modificadas ligeramente */
	temp = p--;
	for (i = 0; i < 20 && p > p_buf && *p != '\n' && *p != '\r'; i++, p--);
	for (i = 0; i < 30 && p <= temp; i++, p++) printf("%c", *p);

	longjmp(e_buf, 1); /* volver a un lugar seguro */
}

/* Obtener un token. */
int get_token(void)
{

	register char *temp;

	token_type = 0; tok = 0;

	temp = token;
	*temp = '\0';

	/* ignorar espacion en blanco */
	while (iswhite(*prog) && *prog) ++prog;

	/* Manejar saltos de línea Windows y Mac */
	if (*prog == '\r') {
		++prog;
		/* Ignorar \n sólo si existe (si no, significa que se está ejecutando es UNIX) */
		if (*prog == '\n') {
			++prog;
		}
		/* ignorar espacios en blanco */
		while (iswhite(*prog) && *prog) ++prog;
	}

	/* Manejo de saltos de línea en Unix */
	if (*prog == '\n') {
		++prog;
		/* ignorar espacios en blanco */
		while (iswhite(*prog) && *prog) ++prog;
	}

	if (*prog == '\0') { /* final del archivo */
		*token = '\0';
		tok = FINISHED;
		return (token_type = DELIMITER);
	}

	if (strchr("{}", *prog)) { /* delimitadores de bloque */
		*temp = *prog;
		temp++;
		*temp = '\0';
		prog++;
		return (token_type = BLOCK);
	}

	/* buscar comentarios */
	if (*prog == '/')
		if (*(prog + 1) == '*') { /* es un comentario */
			prog += 2;
			do { /* buscar el final del comentario */
				while (*prog != '*' && *prog != '\0') prog++;
				if (*prog == '\0') {
					prog--;
					break;
				}
				prog++;
			} while (*prog != '/');
			prog++;
		}

	if (strchr("!<>=", *prog)) { /* es o podría ser
								   un operator relational */
		switch (*prog) {
            case '=': 
                if (*(prog + 1) == '=') {
                    prog++; prog++;
                    *temp = EQ;
                    temp++; *temp = EQ; temp++;
                    *temp = '\0';
                }
            break;
            case '!': 
                if (*(prog + 1) == '=') {
                    prog++; prog++;
                    *temp = NE;
                    temp++; *temp = NE; temp++;
                    *temp = '\0';
                }
                break;
            case '<': 
                if (*(prog + 1) == '=') {
                    prog++; prog++;
                    *temp = LE; temp++; *temp = LE;
                }
                else {
                    prog++;
                    *temp = LT;
                }
                temp++;
                *temp = '\0';
                break;
            case '>': 
                if (*(prog + 1) == '=') {
                    prog++; prog++;
                    *temp = GE; temp++; *temp = GE;
                }
                else {
                    prog++;
                    *temp = GT;
                }
                temp++;
                *temp = '\0';
                break;
        }
		if (*token) return(token_type = DELIMITER);
	}

	if (strchr("+-*^/%=;(),'", *prog)) { /* delimitador */
		*temp = *prog;
		prog++; /* avanzar a la siguiente posición */
		temp++;
		*temp = '\0';
		return (token_type = DELIMITER);
	}

	if (*prog == '"') { /* cadena con comillas */
		prog++;
		while (*prog != '"' && *prog != '\r' && *prog != '\n') *temp++ = *prog++;
		if (*prog == '\r' || *prog == '\n') sntx_err(SYNTAX);
		prog++; *temp = '\0';
		return (token_type = STRING);
	}

	if (isdigit(*prog)) { /* número */
		while (!isdelim(*prog)) *temp++ = *prog++;
		*temp = '\0';
		return (token_type = NUMBER);
	}

	if (isalpha(*prog)) { /* variable u orden */
		while (!isdelim(*prog)) *temp++ = *prog++;
		token_type = TEMP;
	}

	*temp = '\0';

	/* comprobar si una cadena es una orden o una variable */
	if (token_type == TEMP) {
		tok = look_up(token); /* convertir a su representación interna */
		if (tok) token_type = KEYWORD; /* es palabra clave */
		else token_type = IDENTIFIER;
	}
	return token_type;
}

/* Devolver un token a la secuencia de entrada. */
void putback(void)
{
	char *t;

	t = token;
	for (; *t; t++) prog--;
}

/* Buscar la representación interna de un token en la tabla
   de tokens.
*/
int look_up(char *s)
{
	register int i;
	char *p;

	/* convertir a minúsculas */
	p = s;
	while (*p) { *p = tolower(*p); p++; }

	/* ver si el token está en la tabla */
	for (i = 0; *table[i].command; i++) {
		if (!strcmp(table[i].command, s)) return table[i].tok;
	}
	return 0; /* orden desconocida */
}

/* Devolver el índice de la función de biblioteca interna o
   -1 si no se encuentra.
*/
int internal_func(char *s)
{
	int i;

	for (i = 0; intern_func[i].f_name[0]; i++) {
		if (!strcmp(intern_func[i].f_name, s))  return i;
	}
	return -1;
}

/* Devolver cierto(true) si el parámetro c es un delimitador. */
int isdelim(char c)
{
	/* aquí se hace una pequeña adaptación al código original
	   para considerar al salto de línea UNIX como un separador */
	if (strchr(" !;,+-<>'/*%^=()", c) || c == 9 ||
		c == '\r' || c == '\n' || c == 0) return 1;
	return 0;
}

/* Devolver 1 si el parámetro c es espacio o tabulador. */
int iswhite(char c)
{
	if (c == ' ' || c == '\t') return 1;
	else return 0;
}
