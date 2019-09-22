#include <cstring>
#include <iostream>
#include "error.h"
#include "scanner.h"
t_lex const Scanner::lex_keyword_table[] =
{
	LEX_MAIN,LEX_FUNC, LEX_ARRAY, LEX_OF,
	LEX_INT, LEX_BOOL, LEX_STR, LEX_REAL, LEX_CHAR,
	LEX_IF, LEX_ELSE, LEX_WHILE, LEX_DO, LEX_FOR, 
	LEX_SWITCH, LEX_CASE, LEX_DEFAULT,
	LEX_NOT, LEX_AND, LEX_OR,
	LEX_FALSE, LEX_TRUE,
	LEX_READ, LEX_WRITE, LEX_CALL,
	LEX_RETURN, LEX_BREAK, LEX_CONTINUE,
	LEX_NULL
};

t_cstr const Scanner::str_keyword_table[] =
{
	"main", "func", "array", "of",
	"int", "bool", "str", "real", "char",
	"if", "else", "while", "do", "for",
	"switch", "case", "default",
	"not", "and", "or",
	"false", "true",
	"scan", "print", "call",
	"return", "break", "continue"
};

t_lex const Scanner::lex_token_table[] =
{
	LEX_FIN, LEX_COMMA, LEX_LPAREN, LEX_RPAREN,
	LEX_LBRACKET, LEX_RBRACKET,
	LEX_BEGIN, LEX_END, LEX_COLON,
	LEX_ASSIGN, LEX_PLUS, LEX_MINUS,
	LEX_MUL, LEX_DIV, LEX_MOD,
	LEX_GTR, LEX_LSS, LEX_GEQ, LEX_NEQ, LEX_LEQ, LEX_EQ, 
	LEX_NULL
};

t_cstr const Scanner::str_token_table[] = 
{
	";", ",", "(", ")", "[", "]",
	"{", "}", ":",
	"=", "+", "-", "*", "/", "%",
	">","<", ">=", "!=", "<=", "=="
};

void Scanner::scan_char()
{
	if (c == '\n') {
		line++;
		column = 0;
	}
	if ((c = fgetc(file)) == -1) {
		c = '\0';
	}
	if (c != '\0')
		column++;
	return;
}

Scanner::Scanner(t_cstr name_file)
{
	if (name_file == NULL)
		throw OtherError("must be input file");
	file = fopen(name_file, "r");
	if (file == NULL)
		throw OtherError("file can't be open");
	line = 0;
	c = '\n';
	scan_char();
	return;
}

Scanner::~Scanner() 
{ 
	fclose(file);
}

// поиск ключивого слова
t_lex Scanner::find_keyword(char *buf)
{
	for (int i = 0;lex_keyword_table[i] != LEX_NULL; i++) {
		if (strcmp(str_keyword_table[i], buf) == 0)
			return lex_keyword_table[i];
	}
	return LEX_NULL;
}

// поиск символа
t_lex Scanner::find_token(char *buf)
{
	for (int i = 0; lex_token_table[i] != LEX_NULL; i++) {
		if (strcmp(str_token_table[i], buf) == 0)
			return lex_token_table[i];
	}
	return LEX_NULL;
}


char *Scanner::scan_word()
{
	// current_state == word
	char buf[80];
	buf[0] = c;
	scan_char();
	int i = 1;
	while (isalnum(c) || c == '_') {
		//current_state = word
		buf[i++] = c;
		if (i == 80)
			error("very long identifier");
		scan_char();
	}
	//current_state = start
	buf[i] = '\0';
	char *str = new char [strlen(buf) + 1];
	strcpy(str, buf);
	return str;
}

const Lex Scanner::lex(t_lex type, t_value value)
{
	Lex lex(type, value);
	return lex;
}

bool isblank(char c)
{
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

void Scanner::skip_blank()
{
	while (isblank(c)) {
		//current state = blank
		scan_char();
	}
	//current_state = start:
	return;
}

void Scanner::skip_comment()
{
	//current_state == com
	scan_char();
	char c1 = c;
	while (!((c1 == '*' && c == '/') || c == '\0')) {
		c1 = c;
		scan_char();
	}
	
	if (c != '\0') // current_state == start
		scan_char();
	return;
}

const Lex Scanner::get_word_lex()
{
	t_lex type;
	// current_state == word
	char * buf = scan_word();
	// current_state == start
	if ((type = find_keyword(buf)) != LEX_NULL) {
		delete buf;
		return lex(type);
	}
	return lex(LEX_ID, buf);
}

const Lex Scanner::get_num_lex()
{
	//current_state == num
	int integer = c - '0';
	double real = -1;
	scan_char();
	while (isdigit(c)) {
		integer = integer * 10 + (c - '0'); 
		scan_char();
	}
	if (c == '.') {
		//current_state == real
		real = 0;
		double a = 10;
		scan_char();
		while (isdigit(c)) {
			real = real + (c - '0') / a;
			a *= 10;
			scan_char();
			//current_state = start
		}
	} // else current_state == start
	if (c == '_' || isalpha(c))
		//else current_state == stop
		error("wrong format of number");
	if (real != -1)
		return lex(LEX_REALNUM, (double) integer + real);
	return lex(LEX_NUM, integer);
}

const Lex Scanner::get_string_lex()
{
	//current_state == string
	scan_char();
	char *buf = new char [255];
	int i = 0;
	while (c != '"') {
		//current_state = string
		if (c == '\0' || c == '\n' || i == 254)
			error("wrong string format");
		if (c == '\\') {
			scan_char();
			switch (c) {
			case '\\': buf[i++] = '\\'; break;
			case 'n': buf[i++] = '\n'; break;
			case 't': buf[i++] = '\t'; break;
			case '0': buf[i++] = '\0'; break;
			case '\"': buf[i++] = '\"'; break;
			case '\0':
				error("wrong string format");
			}
		} else {
			buf[i++] = c;
		}
		scan_char();
	}
	//current_state = end_string
	buf[i] = '\0';
	scan_char();
	//current_state = start
	return lex(LEX_STRING, buf);
	
}

const Lex Scanner::get_char_lex()
{
	//current_state == symbol
	scan_char();
	//current_state = scan_sym
	int ch = c;
	scan_char();
	if (c != '\'')
		//current_state = err
		error("missing '");
	//else current_symbol = end_char
	scan_char();
	if (c == '\\') {
		scan_char();
		switch (c) {
		case '\\':	ch = '\\'; break;
		case 'n': 	ch = '\n'; break;
		case 't': 	ch = '\t'; break;
		case '0': 	ch = '\0'; break;
		case '\'':	ch = '\''; break;
		case '\0': error("wrong format of constant symbol");
		}
	} else {
		ch = c;
	}
	scan_char();
	if (c != '\'')
		error("wrong format of constant symbol");
	//current_state = start
	return lex(LEX_SYMBOL, ch);
}
	
const Lex Scanner::get_onechar_lex()
{	
	//current_state == onechar
	char buf[2];
	buf[0] = c; buf[1] = '\0';
	scan_char();
	//current_state = start
	return lex(find_token(buf));
}

const Lex Scanner::get_other_lex()
{
	//current_state == other
	char buf[3];
	buf[0] = c;
	scan_char();
	if (c == '=') {
		//current_state = twochars
		buf[1] = '='; buf[2] = '\0';
		scan_char();
		//current_state = start
	} else buf[1] = '\0'; //current_state = start
	return lex(find_token(buf));
}

bool is_onechar_lex(char c)
{
	return (c ==';'  || c == ',' || c == '(' || c ==')'  ||
			c == '{' || c == '}' || c == '+' || c == '-' || 
			c == '*' || c == '%' || c == '[' ||
			c == ']' || c == ':'
	);
}

// главная функция лексического анализа
// по запросу выдаёт новую лексему
const Lex Scanner::get_lex()
{
	//current_state == start
	skip_blank();
	while (c == '/') {
		scan_char();
		if (c != '*') //current_state = start
			return lex(LEX_DIV);
		else //current_state = com
			skip_comment();
		skip_blank(); //current_state = start
	}
	if (c == '\0') //current_state = stop
		return lex(LEX_NULL);
	else if (isalpha(c) || c == '_') //current_state = word
		return get_word_lex();
	else if (isdigit(c)) //current_state = num
		return get_num_lex();
	else if (c == '"') //current_state = string
		return get_string_lex();
	else if (c == '\'') //current_state = symbol
		return get_char_lex();
	else if (is_onechar_lex(c)) //current_state = onechar
		return get_onechar_lex();
	else if (c == '=' || c== '<' || c == '>' || c == '!')
		return get_other_lex(); //current_state = other
	else error("unknown symbol"); //current_state = err
	return lex();
}

void Scanner::error(t_cstr name)
{
	throw SyntaxError(name, line);
}
