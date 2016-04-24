#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int calc_iter(char *code, char **end)
{
	char buf[100];
	if (code[0] != '(') {
		int i = 0;
		for (; code[i] && isdigit(code[i]); i++) {
			buf[i] = code[i];
		}
		buf[i] = 0;
		*end = code + i;
		return atoi(buf);
	}
	int v1, v2;
	char *end1, *end2;
	v1 = calc_iter(code+3, &end1);
	v2 = calc_iter(end1+1, &end2);
	*end = end2 + 1;
	switch(code[1]) {
		case '+':
			return v1 + v2;
		case '-':
			return v1 - v2;
		case '*':
			return v1 * v2;
		case '/':
			return v1 / v2;		
	}
}

int calc(char *code)
{
	char *end;
	return calc_iter(code, &end);
}

int main(int argc, char const *argv[])
{
	char *code;

	// ----- examples -----
	code = "(+ 1 2)";
	printf("%s => %d\n", code, calc(code));
	// => 3

	code = "(* 2 3)";
	printf("%s => %d\n", code, calc(code));
	// => 6

	code = "(* (+ 1 2) (+ 3 4))";
	printf("%s => %d\n", code, calc(code));
	// => 21
	return 0;
}