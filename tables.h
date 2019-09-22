#include <cstdio>
#pragma once
typedef const char* t_cstr;
enum t_lex
{
	LEX_MAIN, LEX_FUNC, LEX_ARRAY, LEX_OF,
	LEX_BEGIN, LEX_END, 
	LEX_INT, LEX_BOOL, LEX_STR, LEX_REAL, LEX_CHAR,
	LEX_IF, LEX_ELSE,
	LEX_WHILE, LEX_DO, LEX_FOR, 
	LEX_SWITCH, LEX_CASE, LEX_DEFAULT,
	LEX_NOT, LEX_AND, LEX_OR,
	LEX_FALSE, LEX_TRUE,
	LEX_PLUS, LEX_MINUS, LEX_MUL, LEX_DIV, LEX_MOD, LEX_NEG,
	LEX_ID, LEX_NUM, LEX_STRING, LEX_REALNUM, LEX_SYMBOL,
	LEX_FIN, LEX_COMMA, LEX_COLON,
	LEX_ASSIGN, LEX_LPAREN, LEX_RPAREN, LEX_EQ,
	LEX_LSS, LEX_GTR,
	LEX_LEQ, LEX_NEQ, LEX_GEQ, LEX_READ, LEX_WRITE,
	LEX_LBRACKET, LEX_RBRACKET, LEX_CALL,
	LEX_BREAK, LEX_RETURN, LEX_CONTINUE,
	LEX_NULL, LEX_EVERY,
	POLIZ_VAR, POLIZ_CONST, POLIZ_ADDR, POLIZ_FGO, POLIZ_GO,
	POLIZ_BOOLTGO, POLIZ_BOOLFGO, POLIZ_CASEFGO, POLIZ_GLOBAL
};

enum t_id
{
	ID_FUNC, ID_ARRAY, ID_VAR
};

// значение
union t_value {
	int int_val;
	double real_val;
	char *str_val;
	t_value() {}
	t_value(int a) {int_val = a;}
	t_value(double a) {real_val = a;}
	t_value(char *a) {str_val = a;}
};

// лексема
struct Lex {
	t_lex type;
	t_value value;
	Lex(t_lex type = LEX_NULL, t_value value = 0) 
		:type(type), value(value) {}
	void print();
};

// шаблонный класс "Таблица"
template <typename t_node>
class Table
{
public:
	Table();
	~Table();
	int get_num() {return num;}
	void add(t_node *new_node);
	void flush();
	// можно использовать как стэк
	t_node *pop();
	t_node *operator [](int);
private:
	t_node **table;
	int num;
};

// шаблонный методы
template <typename t_node>
Table<t_node>::Table()
{
	table = NULL;
	num = 0;
}

template <typename t_node>
Table<t_node>::~Table() {}

template <typename t_node>
void Table<t_node>::add(t_node *new_node)
{
	t_node **tmp = new t_node *[num + 1];
	// copy old table
	for (int i = 0; i < num; i++)
		tmp[i] = table[i];
	tmp[num] = new_node;
	// delete old table
	if (table) delete table;
	table = tmp;
	num++;
	return;
}


template <typename t_node>
t_node* Table<t_node>::operator[](int i)
{
	return table[i];
}

template <typename t_node>
void Table<t_node>::flush()
{
	for (int i = 0; i < num; i++)
		delete table[i];
	delete table;
	table = 0;
	num = 0;
}

template <typename t_node>
t_node* Table<t_node>::pop()
{
	num--;
	return table[num];
}

struct Ident {
	Ident(t_lex type, char *name, t_id id = ID_VAR, Table<int> *arr = 0);
	t_lex type;
	t_id id;
	char *name;
	Table<int> arr;
};

typedef Table<Lex> Poliz;
