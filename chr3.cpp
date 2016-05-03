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

enum EXPR {
	FUNC_CALL,
	INT,
	LAMBDA,
	LET,
	DEFINE,
	NAME,
	ARITH,
	COND,
	BOOL,
	COMPARE,
};

struct expr_t;
struct env_entry_t;
struct env_t
{
	string name;
	expr_t *value;
	env_t *parent;

	env_t():parent(NULL),value(NULL) {}

	expr_t *lookup(string name) {
		assert(parent != this);
		// cout<<"lookup "<<name<<", compare "<<this->name<<endl;
		if (this->name == name) return value;
		if (parent == NULL) return NULL;
		return parent->lookup(name);
	}
	env_t *extend(string name, expr_t *value)
	{
		env_t *child = new env_t();
		child->name = name;
		child->value = value;
		child->parent = this;
		return child;
	}
};

// function call: (func value)
// lambda: (\ name . body)
// let: (let (pairs) body)
// arithmetic: (value name value2)
// compare: (value name value2)
// cond: (cond (pairs))
struct expr_t
{
	int type;
	int ivalue; // for int
	string str;  // for name

	expr_t *name;
	expr_t *func;
	expr_t *value;
	expr_t *value2; // only for + - * /
	expr_t *body;

	env_t *env; // only for lambda

	vector<pair<expr_t,expr_t>> pairs; // for cond and let

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
	expr_t *atom;
	if (str == "true" || str == "else") {
		atom = new expr_t(BOOL);
		atom->ivalue = 1;
		return atom;
	}
	if (str == "false") {
		atom = new expr_t(BOOL);
		atom->ivalue = 1;
		return atom;
	}
	atom = new expr_t(NAME);
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
		list_t first = list.list[0];
		list_t second = list.list[1];
		if (first.is_atom && first.atom == "cond") {
			assert(!second.is_atom);
			ret->type = EXPR::COND;
			for (auto t : second.list) {
				auto c = parser(t.list[0]);
				// c ? r
				t.list.erase(t.list.begin(), t.list.begin()+2);
				auto r = parser(t);
				ret->pairs.push_back(make_pair(*c, *r));
				delete c; delete r;
			}
			return ret;
		}
		// printf("FUNC_CALL (%d)\n", list.size);
		ret->type = EXPR::FUNC_CALL;
		ret->func = parser(list.list[0]);
		ret->value = parser(list.list[1]);
		return ret;
	} else if (size >= 3) {
		list_t first = list.list[0];
		list_t second = list.list[1];
		if (first.is_atom) {
			if (first.atom == "\\") {
				// cout<<"lambda"<<endl;
				ret->type = EXPR::LAMBDA;
				ret->name = make_name(list.list[1].atom);
				// printf("name is %s\n", ret->name.str);
				list.list.erase(list.list.begin(), list.list.begin()+3);
				ret->body = parser(list);
				return ret;
			}
			if (first.atom == "let") {
				ret->type = EXPR::LET;
				for (auto t : second.list) {
					assert(t.list[0].is_atom);
					auto a = make_name(t.list[0].atom);
					// a : e
					t.list.erase(t.list.begin(), t.list.begin()+2);
					auto e = parser(t);
					ret->pairs.push_back(make_pair(*a, *e));
					delete a; delete e;
				}
				list.list.erase(list.list.begin(), list.list.begin()+2);
				ret->body = parser(list);
				return ret;
			}
		}
		if (second.is_atom) {
			if (second.atom == ":") {
				// cout<<"="<<"LET"<<endl;
				ret->type = EXPR::DEFINE;
				ret->name = make_name(list.list[0].atom);
				list.list.erase(list.list.begin(), list.list.begin()+2);
				ret->value = parser(list);
				// cout<<"LET end!"<<endl;
				return ret;
			} else if (second.atom.size() == 1) {
				char op = second.atom[0];
				if (op == '+' || op == '-' || op == '*' || op == '/') {
					ret->type = EXPR::ARITH;
					ret->func = parser(list.list[1]);
					ret->value = parser(list.list[0]);
					ret->value2 = parser(list.list[2]);
					return ret;
				}
				if (op == '>' || op == '<' || op == '=') {
					ret->type = EXPR::COMPARE;
					ret->func = parser(list.list[1]);
					ret->value = parser(list.list[0]);
					ret->value2 = parser(list.list[2]);
					return ret;
				}

			}
		}
		
	}
	
	return ret;
}
void print_ast(expr_t &e)
{
	switch (e.type) {
		case EXPR::INT:
			printf("%d", e.ivalue);
			break;
		case EXPR::BOOL:
			printf("%s\n", e.ivalue ? "true" : "false");
			break;
		case EXPR::NAME:
			printf("%s:%d", e.str.c_str(), e.type);
			break;
		case EXPR::LAMBDA:
			printf("LD(lambda (%s) ", e.name->str.c_str());
			print_ast(*e.body);
			printf(")\n");
			break;
		case EXPR::DEFINE:
			printf("DF(%s ", e.name->str.c_str());
			print_ast(*e.value);
			printf(")\n");
			break;
		case EXPR::LET:
			printf("LT(\n");
			for (auto p : e.pairs) {
				printf("\t(");
				print_ast(p.first);
				printf(":\t");
				print_ast(p.second);
				printf(")\n");
			}
			print_ast(*e.body);
			printf(")\n");
			break;
		case EXPR::FUNC_CALL:
			printf("FC(");
			print_ast(*e.func);
			printf(" ");
			print_ast(*e.value);
			printf(")\n");
			break;
		case EXPR::ARITH:
			printf("AR(");
			print_ast(*e.func);
			printf(" ");
			print_ast(*e.value);
			printf(" ");
			print_ast(*e.value2);
			printf(")\n");
			break;
		case EXPR::COMPARE:
			printf("CP(");
			print_ast(*e.func);
			printf(" ");
			print_ast(*e.value);
			printf(" ");
			print_ast(*e.value2);
			printf(")\n");
			break;
		case EXPR::COND:
			printf("CD(\n");
			for (auto cp : e.pairs) {
				print_ast(cp.first);
				printf("?\t");
				print_ast(cp.second);
				printf("\n");
			}
			printf(")\n");
	}
}

// ==== interpreter ====

env_t *env0;

expr_t *interp(expr_t *e, env_t *env)
{
	env_t *new_env;
	expr_t *ret;
	expr_t *v1, *v2;
	int v;
	switch (e->type) {
		case EXPR::NAME:
			// printf("name of '%s' in %lu\n", e->str.c_str(), &env);
			ret = env->lookup(e->str);
			if (!ret) {
				// printf("lookup %s in env0\n", e->str.c_str());
				ret = env0->lookup(e->str);
				if (!ret) {
					printf("undefined variable %s\n", e->str.c_str());
					exit(1);
				}
			}
			return ret;
		case EXPR::BOOL:
		case EXPR::INT:
			// printf("int %d\n", e->ivalue);
			return e;
		case EXPR::LAMBDA:
			// printf("lambda (%s)\n", e->name->str);
			e->env = env;
			return e;
		case EXPR::DEFINE:
			// printf("let (%s)\n", e->name->str.c_str());
			v1 = interp(e->value, env);
			env0 = env->extend(e->name->str, v1);
			return v1;
		case EXPR::LET:
			new_env = env;
			for (auto p : e->pairs) {
				v1 = interp(&p.second, env);
				// printf("extend %s\n", p.first.str.c_str());
				new_env = new_env->extend(p.first.str, v1);
			}
			return interp(e->body, new_env);
		case EXPR::ARITH:
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
		case EXPR::COMPARE:
			v1 = interp(e->value, env);
			v2 = interp(e->value2, env);
			ret = new expr_t(BOOL);
			assert(e->func->type == NAME);
			switch (e->func->str[0]) {
				case '=':
					v = v1->ivalue == v2->ivalue;
					break;
				case '<':
					v = v1->ivalue < v2->ivalue;
					break;
				case '>':
					v = v1->ivalue > v2->ivalue;
					break;
			}
			ret->ivalue = v;
			return ret;
		case EXPR::FUNC_CALL:
			// printf("FUNC_CALL (:%d :%d) \n", e->func->type, e->value->type);
			v2 = interp(e->value, env);
			v1 = interp(e->func, env);
			new_env = v1->env->extend(v1->name->str, v2);
			ret = interp(v1->body, new_env);
			delete new_env;
			return ret;
		case EXPR::COND:
			for (auto cp : e->pairs) {
				v1 = interp(&cp.first, env);
				if (v1->ivalue) {
					return interp(&cp.second, env);
				}
				delete v1;
			}
			printf("cond all false\n");
			exit(1);
			break;
	}
}

int main(int argc, char const *argv[])
{
	string code;
	list_t list;
	expr_t *e;
	ifstream ifs("test.chr3", ifstream::in);
	env0 = new env_t();
	while (ifs.good()) {
		// 补全括号规则
		// 如果某一行最后一个字符是左括号
		// 将接下来的每一行都用括号括起来
		// 直到遇见一个行首即为右括号且行末不是左括号的行
		getline(ifs, code);
		if (!code.empty() && code[code.size()-1] == '(') {
			string more_code;
			while (ifs.good()) {
				getline(ifs, more_code);
				if (more_code.empty())
				{
					continue;
				}
				if (more_code[0] == ')' && more_code[more_code.size()-1] == '(') {
					code += more_code;
				} else if (!more_code.empty() && more_code[0] == ')') {
					code += more_code;
					break;
				} else {
					code += ("(" + more_code + ")");
				}
			}
		}
		cout << "> " << code << endl;
		list = lexer(code.c_str());
		if (!list.is_atom && list.list.empty())
			continue;
		// print_lexer_tree(list);
		e = parser(list);
		// print_ast(*e);
		e = interp(e, env0);
		if (e->type == INT) {
			printf("=> %d", e->ivalue);
		} else if (e->type == BOOL) {
			printf("=> %s\n", e->ivalue ? "true" : "false");
		} else if (e->type == LET) {
			printf("bind %s\n", e->name->str);
		}
		printf("\n");
	}
}
