#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cassert>
#include <string>
#include <vector>
#include <istream>
#include <fstream>
#include <iostream>

using namespace std;

#define FUNC_CALL 0
#define INT       1
#define LAMBDA    2
#define LET       3
#define NAME      4
#define ARITH     5

struct expr_t;
struct env_entry_t;
typedef vector<env_entry_t> env_t;
struct env_entry_t
{
	string name;
	expr_t *value;
};

// function call: (func value)
// lambda: (lambda (name) body)
// let: (let (name value) body)
// arithmetic: (name value value2)
struct expr_t
{
	// 0 function call, 1 int, 2 lambda, 3 let, 4 name, 5 value, 6 arithmetic
	int type;
	int ivalue; // for int
	string str;  // for name

	expr_t *name;
	expr_t *func;
	expr_t *value;
	expr_t *value2; // only for + - * /
	expr_t *body;

	env_t *env; // only for lambda

	expr_t() {}
	expr_t(int type)
	{
		this->type = type;
	}
};

struct list_t {
	bool is_atom;
	vector<list_t> list; // array
	string atom;

	list_t():is_atom(false) {}
	void set_atom(string s) {
		is_atom = true;
		atom = s;
	}
};

// ==== lexer ====

char const *_g_code;

// a list is a line or (...)
// the end is ')' or '\n'
// operator is a word or a one length char ~!@#$%^&*()_+-={}|[]\:";'<>?,./
list_t lexer_list()
{
	list_t list;
	string word;
	list_t child;
	while (1) {
		if (isalnum(*_g_code)) {
			word += *_g_code;
			_g_code++;
		} else {
			if (!word.empty()) {
				// cout<<"word:"<<word<<endl;
				child.set_atom(word);
				list.list.push_back(child);
				word.clear();
			}
			if (*_g_code && isspace(*_g_code)) {
				_g_code++;
			} else if (*_g_code == 0 || *_g_code == ')') {
				_g_code++;
				break;
			} else if (*_g_code == '(') {
				_g_code++;
				list.list.push_back(lexer_list());
			} else { // operators
				word += *_g_code;
				// cout<<"oprt:"<<word<<endl;
				child.set_atom(word);
				list.list.push_back(child);
				word.clear();
				_g_code++;
			}
		}
	}
	// cout<<"	list end!"<<endl;
	return list;
}

list_t lexer(char const *code)
{
	_g_code = code;
	return lexer_list();
}
void print_lexer_tree(list_t list)
{
	if (list.is_atom) {
		printf("%s ", list.atom.c_str());
		return;
	}
	printf("(");
	for (auto i : list.list)
	{
		print_lexer_tree(i);
	}
	printf(")\n");
}

// ==== parser ====

expr_t *make_name(string str)
{
	expr_t *atom = new expr_t(NAME);
	atom->str = str;
	return atom;
}
expr_t *parse_atom(string str)
{
	// printf("parse_atom %s\n", str.c_str());
	if (isdigit(str[0])) {
		expr_t *atom = new expr_t(INT);
		atom->ivalue = stoi(str);
		// printf("int: %d\n", atom->ivalue);
		return atom;
	}
	return make_name(str);
}

expr_t *parser(list_t list)
{
	if (list.is_atom) {
		// printf("atom\n");
		return parse_atom(list.atom);
	}
	expr_t *ret   = new expr_t();
	int size = list.list.size();
	// printf("size (%d)\n", size);
	if (size == 1) {
		return parser(list.list[0]);
	} else if (size == 2) {
		// printf("FUNC_CALL (%d)\n", list.size);
		ret->type = FUNC_CALL;
		ret->func = parser(list.list[0]);
		ret->value = parser(list.list[1]);
		return ret;
	} else if (size >= 3) {
		list_t second = list.list[1];
		if (second.is_atom) {
			if (second.atom == "=") {
				// cout<<"="<<"LET"<<endl;
				ret->type = LET;
				ret->name = make_name(list.list[0].atom);
				list.list.erase(list.list.begin(), list.list.begin()+2);
				ret->value = parser(list);
				// cout<<"LET end!"<<endl;
				return ret;
			} else if (second.atom.size() == 1) {
				char op = second.atom[0];
				if (op == '+' || op == '-' || op == '*' || op == '/') {
					ret->type = ARITH;
					ret->func = parser(list.list[1]);
					ret->value = parser(list.list[0]);
					ret->value2 = parser(list.list[2]);
					return ret;
				}
			}
		}
		list_t first = list.list[0];
		if (first.is_atom && first.atom == "\\") {
			// cout<<"lambda"<<endl;
			ret->type = LAMBDA;
			ret->name = make_name(list.list[1].atom);
			// printf("name is %s\n", ret->name.str);
			list.list.erase(list.list.begin(), list.list.begin()+3);
			ret->body = parser(list);
			return ret;
		}
	}
	return ret;
}
void print_ast(expr_t *e)
{
	switch (e->type) {
		case INT:
			printf("%d", e->ivalue);
			break;
		case NAME:
			printf("%s:%d", e->str.c_str(), e->type);
			break;
		case LAMBDA:
			printf("LD(lambda (%s) ", e->name->str.c_str());
			print_ast(e->body);
			printf(")\n");
			break;
		case LET:
			printf("LT(let %s ", e->name->str.c_str());
			print_ast(e->value);
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

env_t *ext_env(string name, expr_t *e, env_t &env)
{
	env_t *new_env = new env_t();
	*new_env = env;
	new_env->push_back({name, e});
	return new_env;
}
expr_t *lookup(string name, env_t &env)
{
	// cout<<"lookup "<<name <<" in size("<<env.size()<<")"<<endl;
	for (int i = env.size() - 1; i >= 0; --i) {
		// cout<<"compare "<<name<<(env[i].name)<<endl;
		if (name == env[i].name) {
			return env[i].value;
		}
	}
	return NULL;
}

env_t env0;

expr_t *interp(expr_t *e, env_t &env)
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
			e->env = new env_t();
			*(e->env) = env;
			return e;
		case LET:
			// printf("let (%s)\n", e->name->str.c_str());
			v1 = interp(e->value, env);
			env0 = *ext_env(e->name->str, v1, env);
			return v1;
		case ARITH:
			// printf("v1:%d, v2:%d in env(%ld)\n",
			// 	e->value->type, e->value2->type, env);
			v1 = interp(e->value, env);
			v2 = interp(e->value2, env);
			ret = new expr_t(INT);
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
			new_env = ext_env(v1->name->str, v2, *(v1->env));
			return interp(v1->body, *new_env);
	}
}
expr_t *chr2(expr_t *e)
{
	return interp(e, env0);
}

int main(int argc, char const *argv[])
{
	string code;
	list_t list;
	expr_t *e;
	ifstream ifs("test.chr2", ifstream::in);
	while (ifs.good()) {
		getline(ifs, code);
		cout << "> " << code << endl;
		list = lexer(code.c_str());
		if (!list.is_atom && list.list.empty())
			continue;
		// print_lexer_tree(list);
		e = parser(list);
		// print_ast(e);
		e = chr2(e);
		if (e->type == INT) {
			printf("=> %d", e->ivalue);
		}
		printf("\n");
	}
}
