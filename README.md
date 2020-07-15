# littlec interprete de C (Herbert Schildt) 
Pequeño interprete de C, tomado del libro: Manual de referencia C 4ta. edición de Herbert Schildt.

littlec es un subconjunto del lenguaje de programación C, escrito como una herramienta de aprendizaje por Herbert Schildt para su libro del lenguaje de programación C.

little C no es interprete más eficiente ni el mejor, pero cumple con su propósito de ser una herramienta de enseñanza, el cual puede ampliarse.

## Características de littlec

* Funciones con parameters y variables locales.
* Recursión.
* 'if', 'do-while', 'while', y 'for'.
* variables 'int' y 'char'.
* variables globales.
* constantes string (limitadas).
* sentencia 'return', con o sin un valor.
* biblioteca estándar limitada que se incluye con el código.
* operadores y operaciones con +, -, *, /, %, <, >, <=, >=, ==, !=, unario -, y unario +.
* funciones que pueden devolver 'int' (se puede usar el tipo 'char' pero siempre se promueve(transforma) a 'int').
* comentarios de estilo C como '/* */' 

## Posibles mejoras
* comentarios de estilo C++ como '//'
* Implementación de palabras clave brake y continue
* Si las llaves están juntas a una palabra clave la considera como variable: "do{" vs "do {"
* Implementar nuevos tipos de datos, incluyendo el uso del void

## Referencia.
https://www.drdobbs.com/cpp/building-your-own-c-interpreter/184408184
