#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#define FUNC_CALL 0
#define INT       1
#define LAMBDA    2
#define LET       3
#define NAME      4
#define ARITH     5

struct _env_entry_t
{
	char *name;
	struct _expr_t *value;
};
typedef struct _env_entry_t env_entry_t;
struct _env_t
{
	int size;
	struct _env_entry_t *data;
};
typedef struct _env_t env_t;

// function call: (func value)
// lambda: (lambda (name) body)
// let: (let (name value) body)
// arithmetic: (name value value2)
struct _expr_t
{
	// 0 function call, 1 int, 2 lambda, 3 let, 4 name, 5 value, 6 arithmetic
	int type;
	int ivalue; // for int
	char *str;  // for name

	struct _expr_t *name;
	struct _expr_t *func;
	struct _expr_t *value;
	struct _expr_t *value2; // only for + - * /
	struct _expr_t *body;

	struct _env_t *env; // only for lambda
};

typedef struct _expr_t expr_t;

struct _list_t {
	int size; // -1 is atom
	struct _list_t *list;
	char *atom;
};
typedef struct _list_t list_t;

// ==== lexer ====

char *readword(char const **codep)
{
	char *str = malloc(100);
	int i = 0;
	const char *code = *codep;
	for (;code[i] && code[i] != ')' && !isspace(code[i]); i++) {
		str[i] = code[i];
	}
	str[i] = 0;
	*codep = code + i;
	return realloc(str, i+1);
}
char *lexer_atom(char const **codep)
{
	const char *code = *codep;
	while (*code && isspace(*code)) code++;
	char *str;
	str = readword(&code);
	// printf("str: %s\n", str);
	*codep = code;
	return str;
}
list_t *lexer_list(char const **codep, list_t *list)
{
	const char *code = *codep;
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
			// printf("end\n");
			break;
		}
		// printf("child %d: ", i);
		lexer_list(&code, items+i);
		i++;
	}
	items = realloc(items, sizeof(list_t)*i);
	list->size = i;
	list->list = items;
	*codep = code;
}
list_t *lexer(const char *code)
{
	list_t *list = malloc(sizeof(list_t));
	lexer_list(&code, list);
	return list;
}
void print_lexer_tree(list_t *list)
{
	if (list->size == -1) {
		printf("%s ", list->atom);
		return;
	}
	printf("(");
	for (int i = 0; i < list->size; ++i)
	{
		print_lexer_tree(list->list + i);
	}
	printf(")\n");
}

// ==== parser ====

expr_t *make_name(char *str)
{
	expr_t *atom = malloc(sizeof(expr_t));
	atom->type = NAME;
	atom->str = str;
	return atom;
}
expr_t *parse_atom(char *str)
{
	// printf("parse_atom %s\n", str);
	if (isdigit(str[0])) {
		expr_t *atom = malloc(sizeof(expr_t));
		atom->type = INT;
		atom->ivalue = atoi(str);
		// printf("int: %d\n", atom->ivalue);
		return atom;
	}
	return make_name(str);
}

expr_t *parse_func_call(list_t *list, expr_t *ret)
{
	
}
expr_t *parser(list_t *list)
{
	if (list->size == -1) {
		// printf("atom\n");
		return parse_atom(list->atom);
	}
	expr_t *ret   = malloc(sizeof(expr_t));
	expr_t *body  = malloc(sizeof(expr_t));
	expr_t *value = malloc(sizeof(expr_t));
	expr_t *func  = malloc(sizeof(expr_t));
	list_t first = list->list[0];
	// printf("first (%d)\n", first.size);
	if (first.size == -1) {
		// printf("list->atom %s\n", list->atom);
		if (strcmp(first.atom, "lambda") == 0) {
			// printf("lambda\n");
			ret->type = LAMBDA;
			ret->name = make_name(list->list[1].list[0].atom);
			// printf("name is %s\n", ret->name->str);
			ret->body = parser(list->list + 2);
			return ret;
		}
		if (strcmp(first.atom, "let") == 0) {
			// printf("let\n");
			ret->type = LET;
			ret->name = make_name(list->list[1].list[0].list[0].atom);
			ret->value = parser(list->list[1].list[0].list + 1);
			ret->body = parser(list->list + 2);
			return ret;
		}
		char op = first.atom[0];
		if (op == '+' || op == '-' || op == '*' || op == '/') {
			ret->type = ARITH;
			ret->func = parser(list->list);
			ret->value = parser(list->list + 1);
			ret->value2 = parser(list->list + 2);
			return ret;
		}
	}
	// printf("FUNC_CALL (%d)\n", list->size);
	ret->type = FUNC_CALL;
	ret->func = parser(list->list);
	ret->value = parser(list->list + 1);
	return ret;
}
void print_ast(expr_t *e)
{
	switch (e->type) {
		case INT:
			printf("%d", e->ivalue);
			break;
		case NAME:
			printf("%s:%d", e->str, e->type);
			break;
		case LAMBDA:
			printf("LD(lambda (%s) ", e->name->str);
			print_ast(e->body);
			printf(")\n");
			break;
		case LET:
			printf("LT(let ((%s ", e->name->str);
			print_ast(e->value);
			printf(")) ");
			print_ast(e->body);
			printf(")\n");
			break;
		case FUNC_CALL:
			printf("FC(");
			print_ast(e->func);
			printf(" ");
			print_ast(e->value);
			printf(")\n");
			break;
		case ARITH:
			printf("AR(");
			print_ast(e->func);
			printf(" ");
			print_ast(e->value);
			printf(" ");
			print_ast(e->value2);
			printf(")\n");
			break;
	}
}

// ==== interpreter ====

// return new env
env_t *env_dup(env_t *env)
{
	env_t *new_env = malloc(sizeof(env_t));
	assert(new_env != NULL);
	new_env->size = env->size;
	new_env->data = NULL;
	if (new_env->size) {
		new_env->data = malloc(sizeof(env_entry_t)*new_env->size);
		assert(new_env->data != NULL);
		memcpy(new_env->data, env->data,
			sizeof(env_entry_t) * env->size);
	}
	return new_env;
}
env_t *ext_env(char *name, expr_t *e, env_t *env)
{
	env_t *new_env = env_dup(env);
	int size = sizeof(env_entry_t) * (new_env->size+1);
	new_env->data = new_env->data ?
		realloc(new_env->data, size) : malloc(size);
	assert(new_env->data != NULL);
	new_env->data[new_env->size].name = name;
	new_env->data[new_env->size].value = e;
	new_env->size++;
}
expr_t *lookup(char *name, env_t *env)
{
	for (int i = env->size - 1; i >= 0; --i) {
		if (strcmp(name, env->data[i].name) == 0) {
			return env->data[i].value;
		}
	}
	return NULL;
}
expr_t *make_expr(int type)
{
	expr_t *e = malloc(sizeof(expr_t));
	e->type = type;
	return e;
}
expr_t *interp(expr_t *e, env_t *env)
{
	env_t *new_env;
	expr_t *ret;
	expr_t *v1, *v2;
	int v;
	switch (e->type) {
		case NAME:
			// printf("name of '%s'\n", e->str);
			ret = lookup(e->str, env);
			if (!ret) {
				printf("undefined variable %s\n", e->str);
				exit(1);
			}
			return ret;
		case INT:
			// printf("int %d\n", e->ivalue);
			return e;
		case LAMBDA:
			// printf("lambda (%s)\n", e->name->str);
			e->env = env_dup(env);
			return e;
		case LET:
			// printf("let (%s)\n", e->name->str);
			v1 = interp(e->value, env);
			new_env = ext_env(e->name->str, v1, env);
			return interp(e->body, new_env);
		case ARITH:
			// printf("v1:%d, v2:%d in env(%ld)\n",
			// 	e->value->type, e->value2->type, env);
			v1 = interp(e->value, env);
			v2 = interp(e->value2, env);
			ret = make_expr(INT);
			assert(e->func->type == NAME);
			switch (e->func->str[0]) {
				case '+':
					v = v1->ivalue + v2->ivalue;
					break;
				case '-':
					v = v1->ivalue - v2->ivalue;
					break;
				case '*':
					v = v1->ivalue * v2->ivalue;
					break;
				case '/':
					v = v1->ivalue / v2->ivalue;
					break;
			}
			ret->ivalue = v;
			return ret;
		case FUNC_CALL:
			// printf("FUNC_CALL (:%d :%d) \n", e->func->type, e->value->type);
			v1 = interp(e->func, env);
			v2 = interp(e->value, env);
			new_env = ext_env(v1->name->str, v2, v1->env);
			return interp(v1->body, new_env);
	}
}
expr_t *r2(expr_t *e)
{
	env_t env0;
	env0.size = 0;
	env0.data = NULL;
	return interp(e, &env0);
}

int main(int argc, char const *argv[])
{
	const char *code;
	list_t *list;
	expr_t *e;
	if (argc >= 2) {
		code = argv[1];
		list = lexer(code);
		printf("%s ==> ", code);
		print_lexer_tree(list);
		e = parser(list);
		print_ast(e);
		printf("%s => %d\n", code, r2(e)->ivalue);
		return 0;
	}
	// ----- examples -----
	code = "(+ 1 2)";
	list = lexer(code);
	e = parser(list);
	printf("%s => %d\n", code, r2(e)->ivalue);
	// => 3

	code = "(* 2 3)";
	list = lexer(code);
	e = parser(list);
	printf("%s => %d\n", code, r2(e)->ivalue);
	// => 6

	code = "(* 2 (+ 3 4))";
	list = lexer(code);
	e = parser(list);
	printf("%s => %d\n", code, r2(e)->ivalue);
	// => 14

	code = "(* (+ 1 2) (+ 3 4))";
	list = lexer(code);
	e = parser(list);
	printf("%s => %d\n", code, r2(e)->ivalue);
	// => 21

	code = "((lambda (x) (* 2 x)) 3)";
	list = lexer(code);
	e = parser(list);
	printf("%s => %d\n", code, r2(e)->ivalue);
	// => 6

	code = "(let ((x 2)) "
			" (let ((f (lambda (y) (* x y))))"
			" (f 3)))";
	list = lexer(code);
	e = parser(list);
	printf("%s => %d\n", code, r2(e)->ivalue);
	// => 6

	return 0;
}
