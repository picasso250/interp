#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

// function call: (func value)
// lambda: (lambda (name) body)
// let: (let (name value) body)
struct _expr_t
{
	// 0 function call, 1 int, 2 lambda, 3 let, 4 name, 5 value
	int type;
	int ivalue; // for int
	char *str; // for name

	// lambda or let
	struct _expr_t *name;
	struct _expr_t *func;
	struct _expr_t *value;
	struct _expr_t *body;
};

typedef struct _expr_t expr_t;

struct _list_t {
	int size; // -1 is atom
	struct _list_t *list;
	expr_t *atom;
};
typedef struct _list_t list_t;

char *readword(char **codep)
{
	char *str = malloc(100);
	int i = 0;
	char *code = *codep;
	for (;code[i] && code[i] != ')' && !isspace(code[i]); i++) {
		str[i] = *code;
	}
	str[i] = 0;
	*codep = code + i;
	return realloc(str, i+1);
}
expr_t *lexer_atom(char **codep)
{
	char *code = *codep;
	while (*code && isspace(*code)) code++;
	char *str;
	expr_t *e = malloc(sizeof(expr_t));
	str = readword(&code);
	printf("str: %s\n", str);
	e->str = str;
	*codep = code;
	return e;
}
list_t *lexer_list(char **codep, list_t *list)
{
	char *code = *codep;
	while (*code && isspace(*code)) code++;
	// printf("lexer_list '%s'\n", code);
	if (code[0] != '(') {
		// printf("atom\n");
		list->size = -1;
		list->atom = lexer_atom(&code);
		*codep = code;
		return list;
	}
	code++;
	list_t *items = malloc(sizeof(list_t)*100);
	int i = 0;
	while (1) {
		while (*code && isspace(*code)) code++;
		if (*code == ')') {
			code++;
			printf("end\n");
			break;
		}
		printf("child %d: ", i);
		lexer_list(&code, items+i);
		i++;
	}
	items = realloc(items, sizeof(list_t)*i);
	list->size = i;
	list->list = items;
	*codep = code;
}
list_t *lexer(char *code)
{
	list_t *list = malloc(sizeof(list_t));
	lexer_list(&code, list);
	return list;
}
void print_lexer_tree(list_t *list)
{
	if (list->size == -1) {
		printf("%s ", list->atom->str);
		return;
	}
	printf("(");
	for (int i = 0; i < list->size; ++i)
	{
		print_lexer_tree(list->list + i);
	}
	printf(")\n");
}

// void interp(expr_t *e, int env)
// {
// 	switch (e->type) {
// 		case INT:
// 			e->ivalue = atoi(e->code);
// 			break;
// 		case LAMBDA:
// 			break;
// 		case LET:

// 			break;
// 	}
// }
typedef struct _kv_t {
	char *key;
	char *value;
} kv_t;

int main(int argc, char const *argv[])
{
	char *code;
	// ----- examples -----
	code = "(+ 1 2)";
	list_t *list;
	list = lexer(code);
	printf("%s => %d\n", code, list->size);
	print_lexer_tree(list);
	// printf("%s => %d\n", code, lexer(code));
	// => 3

	code = "(* 2 3)";
	list = lexer(code);
	printf("%s => %d\n", code, list->size);
	print_lexer_tree(list);
	// printf("%s => %d\n", code, lexer(code));
	// => 6

	code = "(* (+ 1 2) (+ 3 4))";
	list = lexer(code);
	printf("%s => %d\n", code, list->size);
	print_lexer_tree(list);
	// printf("%s => %d\n", code, lexer(code));
	// => 21

	return 0;
}