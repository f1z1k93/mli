#include <stdio.h> 
#include "tables.h"
#include <string.h>
#include "error.h"
#include "executer.h"

t_lex val_type(t_lex type)
{
	switch (type) {
	case LEX_NUM: return LEX_INT;
	case LEX_REALNUM:return LEX_REAL;
	case LEX_STRING: return LEX_STR;
	case LEX_SYMBOL: return LEX_CHAR;
	case LEX_TRUE:case LEX_FALSE: return LEX_BOOL;
	default:;
	}
	return LEX_NULL;
}

void runtime_error(t_cstr err)
{
	throw RuntimeError(err);
}

Executer::Executer(Table<Lex> *poliz, char *argv[])
	: argv(argv)
{
	poliz_index = 0;
	lex = *(*poliz)[0];
	argc = 2;
	this->poliz = poliz;
}

Executer::~Executer()
{
	local_memory.flush();
	global_memory.flush();
	func_addr.flush();
	exec_stack.flush();
}

exec_node::exec_node(t_lex type, t_value value, bool is_global, int index)
	: type(type), is_global(is_global), index(index)
{
	if (type == LEX_STR) {
		(this->value).str_val = new char [strlen(value.str_val) + 1];
		strcpy((this->value).str_val, value.str_val);
	} else {
		this->value = value;
	}
}

memory_node::memory_node(t_lex type, t_value value, Table <int> *size)
	:type(type), size(size)
{
	if (size) {
		this->size = size;
		int max_size = 1;
		for (int i = 0; i < size->get_num(); i++)
			max_size = max_size * (*(*size)[i]);
		for (int i = 0; i < max_size; i++)
			memory.add(new t_value(value));
	} else {
		this->value = value;
	}
}

func_node::func_node(Table<t_lex> *t_arg, int address)
	: address(address), t_arg(t_arg)
{}

int Executer::run()
{
	int num = poliz->get_num();
	while (poliz_index < num) {
		// проходим по полизу и выбираем команду
		switch (lex.type) {
		case LEX_INT: case LEX_BOOL: case LEX_REAL:
		case LEX_CHAR: case LEX_STR: case POLIZ_GLOBAL:
								new_variable(); break;
		case LEX_FUNC:			new_func(); break;
		case POLIZ_VAR:			push_var(); break;
		case POLIZ_CONST: 		push_const(); break;
		case POLIZ_GO: 			go(); break;
		case POLIZ_FGO:			fgo(); break;
		case POLIZ_CASEFGO:		case_fgo(); break;
		case LEX_FIN:			fin(); break;
		case LEX_WRITE:			write(); break;
		case LEX_READ:			read(); break;
		case LEX_RETURN:		return_(); break;
		case LEX_ASSIGN:		assign(); break;
		case LEX_EQ:			equal(); break;
		case LEX_NEQ:			nequal();break;
		case LEX_GTR:			gtr(); break;
		case LEX_GEQ:			geq(); break;
		case LEX_LSS:			lss(); break;
		case LEX_LEQ:			leq(); break;
		case LEX_PLUS:			add(); break;
		case LEX_MINUS:			sub(); break;
		case LEX_MUL:			mul(); break;
		case LEX_DIV:			div(); break;
		case LEX_NEG:			neg();break;
		case LEX_CALL:			call();break;
		case POLIZ_BOOLTGO:		booltgo(); break;
		case POLIZ_BOOLFGO:		boolfgo(); break;
		case LEX_NOT:			not_(); break;
		case LEX_MOD:			mod(); break;
		case LEX_MAIN:			start(); break;
		default:printf("unknown operator\n"); return 0;
		}
	}
	int result = (exec_stack.pop()->value).int_val;
	// оставшиеся число в стэке выполнения есть результат
	return result;
}

void Executer::print()
{
	int num = poliz->get_num();
	for (int i = 0; i < num; i++)
		(*poliz)[i]->print();
}

// в lex новая лексема
void Executer::scan_lex()
{
	if (poliz_index < poliz->get_num() - 1)
		lex = *((*poliz)[++poliz_index]);
}

void Executer::new_variable()
{
	// определяем тип памяти для выделения под переменную
	memory = &local_memory; // стэк
	if (lex.type == POLIZ_GLOBAL) {
		memory = &global_memory; // глобальная
		scan_lex();
	}
	t_lex type = lex.type;
	scan_lex();
	// создаем значение и
	// заполняем в зависимости от типа переменной
	t_value value;
	switch (type) {
		case LEX_STR:
			if (lex.type == LEX_ARRAY) {
				value.str_val = new char [1];
				value.str_val[0] = '\0'; break;
			}
			if (lex.value.str_val) {
				value.str_val = new char [strlen(lex.value.str_val) + 1];
				strcpy(value.str_val, lex.value.str_val);
			} else {
				value.str_val = NULL;
			}
			break;
		case LEX_REAL:
			if (lex.type == LEX_ARRAY) {
				value.real_val = 0;break;
			}
			value.int_val = lex.value.int_val;
			break;
		default:
			if (lex.type == LEX_ARRAY) {
				value.int_val = 0; break;
			}
			value.int_val = lex.value.int_val;
	}
	if (lex.type != LEX_ARRAY) {
		// не массив, добавляем значение в память
		memory->add(new memory_node(type, value));
		scan_lex();
	} else {
		// заполняем массив размеров "матрицы"
		Table<int> *size = new Table<int>;
		scan_lex();
		while (lex.type == LEX_NUM) {
			size->add(new int(lex.value.int_val));
			scan_lex();
		}
		// и добавляем в память
		memory->add(new memory_node(type, value, size));
	}
	scan_lex();
}

void Executer::new_func()
{
	scan_lex();
	Table<t_lex> *arg_type = new Table<t_lex>;
	while (lex.type != LEX_NUM) {
		arg_type->add(new t_lex(lex.type));
		scan_lex();
	}
	int address = lex.value.int_val;
	func.add(new func_node(arg_type, address));
	scan_lex();scan_lex();
}

void Executer::push_const()
{
	// в стэке исполнения сохраняем значение константы
	scan_lex();
	exec_stack.add(new exec_node(val_type(lex.type), lex.value));
	if (poliz_index < poliz->get_num())
		scan_lex();	
}

void Executer::push_var()
{
	// в стэке исполнения сохраняем адрес переменной
	scan_lex();
	t_value value;
	memory = &local_memory;
	is_global = false;
	if (lex.type == POLIZ_GLOBAL) {
		memory = &global_memory;
		is_global = true;
		scan_lex();
	}

	int address = lex.value.int_val;
	if (!is_global)
		address = memory->get_num() - address - 1;
	scan_lex();
	if (lex.type == POLIZ_ADDR) {
		push_addr(address);
	} else {
		push_val(address);
	}
}

int Executer::get_index(Table<int> *size)
{
	// вытаскиваю значения из стэка исполнения
	// преобразую в индекс и возвращаю
	int tmp = 1, index = 0;
	int num = size->get_num() - 1;
	int bound, curr_i;
	for (int i = num; i >= 0; i--) {
		bound = *(*size)[i];
		curr_i = exec_stack.pop()->value.int_val;
		if (curr_i < 0 || curr_i >= bound)
			runtime_error("array index out of bound");
		index += curr_i * tmp;
		tmp *= bound;
	}
	return index;
}

void Executer::push_addr(int address)
{
	scan_lex();
	Table<int> *size = (*memory)[address]->size;
	t_value value; value.int_val = address;
	if (size != NULL) {
		int index = get_index(size);
		exec_stack.add(new exec_node(LEX_INT, value, is_global, index));
	} else
		exec_stack.add(new exec_node(LEX_INT, value, is_global));
}



void Executer::push_val(int address)
{
	Table<int> *size = (*memory)[address]->size;
	t_lex type = ((*memory)[address])->type;
	t_value value;
	if (size != NULL) {
		t_value tmp = *((*memory)[address]->memory[get_index(size)]);
		if (type == LEX_STR) {
			value.str_val = new char [strlen(tmp.str_val) + 1];
			strcpy(value.str_val, tmp.str_val);
		} else
			value = tmp;
	} else {
		t_value tmp = ((*memory)[address])->value;
		if (type == LEX_STR) {
			value.str_val = new char [strlen(tmp.str_val) + 1];
			strcpy(value.str_val, tmp.str_val);
		} else
			value = tmp;
	}
	exec_stack.add(new exec_node(type, value));
}	

void Executer::go()
{
	// прыжок по адресу полиза
	scan_lex();
	poliz_index = lex.value.int_val;
	lex = *((*poliz)[poliz_index]);
}

void Executer::fgo()
{	
	// прыжок по адресу полиза по лжи
	scan_lex();
	int is_go = !((exec_stack.pop()->value).int_val);
	if (is_go) {
		poliz_index = lex.value.int_val;
		lex = *((*poliz)[poliz_index]);
	} else
		scan_lex();
}

void Executer::case_fgo()
{
	// для switcha, чтобы не убирать значение со стэка
	scan_lex();
	int case_value = (exec_stack.pop()->value).int_val;
	int top = exec_stack.get_num() - 1;
	if (case_value != (exec_stack[top]->value).int_val) {
		poliz_index = lex.value.int_val;
		lex = *((*poliz)[poliz_index]);
	} else
		scan_lex();
}

void Executer::return_()
{
	scan_lex();
	int num = lex.value.int_val;
	memory_node *tmp;
	// высвобождаем то что заняли	
	for (int i = 0; i < num; i++) {
		tmp = local_memory.pop();
		if (tmp->type == LEX_STR) delete tmp;
	}
	// уходим
	if (func_addr.get_num() > 0) {
		poliz_index = *(func_addr.pop());
		lex = *(*poliz)[poliz_index];
	} else
		poliz_index = poliz->get_num();
}

void Executer::write()
{
	scan_lex();
	exec_node *tmp = exec_stack.pop();
	switch (tmp->type) {
		case LEX_INT:
			printf("%d", (tmp->value).int_val);
			break;
		case LEX_CHAR:
			printf("%c", (tmp->value).int_val);
			break;
		case LEX_REAL:
			printf("%.6g", (tmp->value).real_val);
			break;
		case LEX_STR:
			printf("%s", (tmp->value).str_val);
			break;
		default:;
	}
	delete tmp;
}

char *get_str()
{
	// чтение строки из полиза
	char buf[80];
	int i = 0;
	char *str = new char [1];
	str[0] = '\0';
	int c = getchar();
	while (c != EOF && c != ' ' && c != '\t' && c != '\r' && c != '\n') {
		buf[i++] = c;
		if (i == 80) {
			char *tmp = str;
			str = new char [strlen(tmp) + 80];
			strcpy(str, tmp); strncat(str, buf, 79);
			delete tmp;
			i = 0;
		}
		c = getchar();
	}
	buf[i] = '\0';
	if (i != 0) {
		char *tmp = str;
		str = new char [strlen(tmp) + i + 1];
		strcpy(str, tmp); strncat(str, buf, 79);
		delete tmp;
	}
	return str;
}

int get_int()
{
	// чтение целого
	int integer = 0;
	int c = getchar();
	while ('0' <= c && c <= '9') {
		integer = integer * 10 + (c - '0');
		c = getchar();
	}
	return integer;
}

double get_real()
{
	int integer = 0;
	double real = 0;
	int c = getchar();
	while ('0' <= c && c <= '9') {
		integer = integer * 10 + (c - '0');
		c = getchar();
	}
	if (c == '.') {
		double a = 10;
		c = getchar();
		while ('0' <= c && c <= '9') {
			real = real + (c - '0') / a;
			a *= 10;
			c = getchar();
		}
	}
	return integer + real;
}

void Executer::read()
{
	scan_lex();
	exec_node *tmp = exec_stack.pop();
	Table<memory_node> &memory = local_memory;
	int top = (tmp->value).int_val;
	switch (memory[top]->type) {
	case LEX_INT:
		memory[top]->value.int_val = get_int();
		break;
	case LEX_CHAR:
		memory[top]->value.int_val = getchar();
		break;
	case LEX_REAL:
		memory[top]->value.real_val = get_real();
		break;
	case LEX_STR:
		delete memory[top]->value.str_val;
		memory[top]->value.str_val = get_str();
		break;
	default:break;
	}
	delete tmp;
}

void Executer::assign()
{
	// присваивание (lvalue хранит адрес в памяти)
	exec_node *rvalue = exec_stack.pop();
	exec_node *lvalue = exec_stack.pop();
	memory = &local_memory;
	char *str;
	if (lvalue->is_global)
		memory = &global_memory;

	t_value value;
	int address = (lvalue->value).int_val;
	switch (rvalue->type) {
		case LEX_STR:
			str = (rvalue->value).str_val;
			value.str_val = str;
			break;
		case LEX_INT : case LEX_BOOL : case LEX_CHAR:
			value.int_val = (rvalue->value).int_val;
			break;
		case LEX_REAL:
			value.real_val = (rvalue->value).real_val;
		default:;
	}
	int index = lvalue->index;
	if (index >= 0) {
		*((*memory)[address]->memory[index]) = value;
	} else
		(*memory)[address]->value = value;
	delete lvalue;
	exec_stack.add(rvalue);
	scan_lex();
}

void Executer::fin()
{
	exec_node *tmp = exec_stack.pop();
	delete tmp;
	scan_lex();
}

// арифметические операции

// вытащить из стэка исполнения провести
// операции, результат поместить в стэк
void Executer::add()
{

	exec_node *right = exec_stack.pop();
	exec_node *left = exec_stack.pop();
	t_lex type;
	t_value value;

	if (right->type == LEX_STR) {
		char *r_str = (right->value).str_val;
		char *l_str = (left->value).str_val;
		char *res = new char [strlen(r_str) + strlen(r_str) + 1];
		strcpy(res, l_str);
		strcat(res, r_str);
		delete r_str; delete l_str;
		type = LEX_STR; value.str_val = res;
	} else if (right->type == left->type && right->type == LEX_INT) {
		type = LEX_INT;
		int a = (left->value).int_val; 
		int b = (right->value).int_val;
		value.int_val = a + b;
	} else {
		type = LEX_REAL;
		double a = (left->type == LEX_INT) ?
				(left->value).int_val : (left->value).real_val;
		double b = (right->type == LEX_INT) ?
				(right->value).int_val : (right->value).real_val;
		value.real_val = a + b;
	}
	delete right; delete left;
	exec_stack.add(new exec_node(type, value));
	scan_lex();
}

void Executer::sub()
{
	exec_node *right = exec_stack.pop();
	exec_node *left = exec_stack.pop();
	t_lex type;
	t_value value;
	if (right->type == left->type && right->type == LEX_INT) {
		type = LEX_INT;
		int a = (left->value).int_val; 
		int b = (right->value).int_val;
		value.int_val = a - b;
	} else {
		type = LEX_REAL;
		double a = (left->type == LEX_INT) ?
				(left->value).int_val : (left->value).real_val;
		double b = (right->type == LEX_INT) ?
				(right->value).int_val : (right->value).real_val;
		value.real_val = a - b;
	}
	delete right; delete left;
	exec_stack.add(new exec_node(type, value));
	scan_lex();
}

void Executer::mul()
{

	exec_node *right = exec_stack.pop();
	exec_node *left = exec_stack.pop();
	t_lex type;
	t_value value;
	if (right->type == left->type && right->type == LEX_INT) {
		type = LEX_INT;
		int a = (left->value).int_val; 
		int b = (right->value).int_val;
		value.int_val = a * b;
	} else {
		type = LEX_REAL;
		double a = (left->type == LEX_INT) ?
				(left->value).int_val : (left->value).real_val;
		double b = (right->type == LEX_INT) ?
				(right->value).int_val : (right->value).real_val;
		value.real_val = a * b;
	}
	delete right; delete left;
	exec_stack.add(new exec_node(type, value));
	scan_lex();
}

void Executer::div()
{
	exec_node *right = exec_stack.pop();
	exec_node *left = exec_stack.pop();
	t_lex type;
	t_value value;
	if (right->type == left->type && right->type == LEX_INT) {
		type = LEX_INT;
		int a = (left->value).int_val; 
		int b = (right->value).int_val;
		if (b == 0)
			runtime_error("division by zerro");
		value.int_val = (int) a / b;
	} else {
		type = LEX_REAL;
		double a = (left->type == LEX_INT) ?
				(left->value).int_val : (left->value).real_val;
		double b = (right->type == LEX_INT) ?
				(right->value).int_val : (right->value).real_val;
		if (b == 0)
			runtime_error("division by zerro");
		value.real_val = a / b;
	}
	delete right; delete left;
	exec_stack.add(new exec_node(type, value));
	scan_lex();
}

void Executer::mod()
{
	exec_node *right = exec_stack.pop();
	exec_node *left = exec_stack.pop();
	t_value value;
	int a = (left->value).int_val; 
	int b = (right->value).int_val;
	value.int_val = a % b;
	delete right; delete left;
	exec_stack.add(new exec_node(LEX_INT, value));
	scan_lex();
}

void Executer::neg()
{
	int num = exec_stack.get_num() - 1;
	t_value &value = exec_stack[num]->value;
	if (exec_stack[num]->type == LEX_INT)
		value.int_val = -value.int_val;
	else
		value.real_val = -value.real_val;
	scan_lex();
}

int abs(double a)
{
	if (a < 0) a = -a;
	return a;
}

// операции сравнения
void Executer::equal()
{
	exec_node *right = exec_stack.pop();
	exec_node *left = exec_stack.pop();
	t_lex type = LEX_BOOL;
	t_value value;
	if (right->type == LEX_STR) {
		char *r_str = (right->value).str_val;
		char *l_str = (left->value).str_val;
		value.int_val = strcmp(r_str, l_str) ? 0 : 1;
		delete r_str; delete l_str;
	} else if (right->type == left->type && right->type == LEX_INT) {
		int a = (left->value).int_val; 
		int b = (right->value).int_val;
		value.int_val = (a == b) ? 1 : 0;
	} else {
		type = LEX_REAL;
		double a = (left->type == LEX_INT) ?
				(left->value).int_val : (left->value).real_val;
		double b = (right->type == LEX_INT) ?
				(right->value).int_val : (right->value).real_val;
		value.int_val = (abs(a - b) < 0.001) ? 1 : 0;
	}
	delete right; delete left;
	exec_stack.add(new exec_node(type, value));
	scan_lex();
}

void Executer::nequal()
{
	exec_node *right = exec_stack.pop();
	exec_node *left = exec_stack.pop();
	t_lex type = LEX_BOOL;
	t_value value;
	if (right->type == LEX_STR) {
		char *r_str = (right->value).str_val;
		char *l_str = (left->value).str_val;
		value.int_val = strcmp(r_str, l_str) ? 1 : 0;
		delete r_str; delete l_str;
	} else if (right->type == left->type && right->type == LEX_INT) {
		int a = (right->value).int_val; 
		int b = (left->value).int_val;
		value.int_val = (a != b) ? 1 : 0;
	} else {
		type = LEX_REAL;
		double a = (left->type == LEX_INT) ?
				(left->value).int_val : (left->value).real_val;
		double b = (left->type == LEX_INT) ?
				(right->value).int_val : (right->value).real_val;
		value.int_val = (abs(a - b) >= 0.001) ? 1 : 0;
	}
	delete right; delete left;
	exec_stack.add(new exec_node(type, value));
	scan_lex();
}

void Executer::gtr()
{
	exec_node *right = exec_stack.pop();
	exec_node *left = exec_stack.pop();
	t_lex type = LEX_BOOL;
	t_value value;
	if (right->type == LEX_STR) {
		char *r_str = (right->value).str_val;
		char *l_str = (left->value).str_val;
		value.int_val = (strcmp(l_str, r_str) > 0) ? 1 : 0;
		delete r_str; delete l_str;
	} else if (right->type == left->type && right->type == LEX_INT) {
		int a = (left->value).int_val;
		int b = (right->value).int_val; 
		value.int_val = (a > b) ? 1 : 0;
	} else {
		type = LEX_REAL;
		double a = (left->type == LEX_INT) ?
				(left->value).int_val : (left->value).real_val;
		double b = (right->type == LEX_INT) ?
				(right->value).int_val : (right->value).real_val;
		value.int_val = (a > b) ? 1 : 0;
	}
	delete right; delete left;
	exec_stack.add(new exec_node(type, value));
	scan_lex();
}

void Executer::geq()
{
	exec_node *right = exec_stack.pop();
	exec_node *left = exec_stack.pop();
	t_lex type = LEX_BOOL;
	t_value value;
	if (right->type == LEX_STR) {
		char *r_str = (right->value).str_val;
		char *l_str = (left->value).str_val;
		value.int_val = (strcmp(l_str, r_str) >= 0) ? 1 : 0;
		delete r_str; delete l_str;
	} else if (right->type == left->type && right->type == LEX_INT) {
		int a = (left->value).int_val;
		int b = (right->value).int_val; 
		value.int_val = (a >= b) ? 1 : 0;
	} else {
		type = LEX_REAL;
		double a = (left->type == LEX_INT) ?
				(left->value).int_val : (left->value).real_val;
		double b = (right->type == LEX_INT) ?
				(right->value).int_val : (right->value).real_val;
		value.int_val = (a >= b) ? 1 : 0;
	}
	delete right; delete left;
	exec_stack.add(new exec_node(type, value));
	scan_lex();
}

void Executer::lss()
{
	exec_node *right = exec_stack.pop();
	exec_node *left = exec_stack.pop();
	t_lex type = LEX_BOOL;
	t_value value;
	if (right->type == LEX_STR) {
		char *r_str = (right->value).str_val;
		char *l_str = (left->value).str_val;
		value.int_val = (strcmp(l_str, r_str) < 0) ? 1 : 0;
		delete r_str; delete l_str;
	} else if (right->type == left->type && right->type == LEX_INT) {
		int a = (left->value).int_val;
		int b = (right->value).int_val; 
		value.int_val = (a < b) ? 1 : 0;
	} else {
		type = LEX_REAL;
		double a = (left->type == LEX_INT) ?
				(left->value).int_val : (left->value).real_val;
		double b = (right->type == LEX_INT) ?
				(right->value).int_val : (right->value).real_val;
		value.int_val = (a < b) ? 1 : 0;
	}
	delete right; delete left;
	exec_stack.add(new exec_node(type, value));
	scan_lex();
}

void Executer::leq()
{
	exec_node *right = exec_stack.pop();
	exec_node *left = exec_stack.pop();
	t_lex type = LEX_BOOL;
	t_value value;
	if (right->type == LEX_STR) {
		char *r_str = (right->value).str_val;
		char *l_str = (left->value).str_val;
		value.int_val = (strcmp(l_str, r_str) <= 0) ? 1 : 0;
		delete r_str; delete l_str;
	} else if (right->type == left->type && right->type == LEX_INT) {
		int a = (left->value).int_val;
		int b = (right->value).int_val; 
		value.int_val = (a <= b) ? 1 : 0;
	} else {
		type = LEX_REAL;
		double a = (left->type == LEX_INT) ?
				(left->value).int_val : (left->value).real_val;
		double b = (right->type == LEX_INT) ?
				(right->value).int_val : (right->value).real_val;
		value.int_val = (a <= b) ? 1 : 0;
	}
	delete right; delete left;
	exec_stack.add(new exec_node(type, value));
	scan_lex();
}

void Executer::call() // вызов функции
{
	scan_lex();
	// сохраняем адрес возврата в стэке возвратов из функций
	func_addr.add(new int(poliz_index + 1));
	// создаем структуру, которая хранит все данный о функции
	func_node *func_tmp = func[lex.value.int_val];
	// выделяем таблицу аргументов этой функции
	Table<t_lex> *t_arg = func_tmp->t_arg;
	int num_arg = t_arg->get_num() - 1;
	int num_exec = exec_stack.get_num() - 1; 
	exec_node *tmp;
	// в цикле вынимаем параметры из функции и заносим в локальную
	// память функции (стэк)
	for (int i = num_arg; i >= 0; i--) {
		tmp = exec_stack[num_exec - i];
		local_memory.add(new memory_node(tmp->type, tmp->value));
		delete tmp;
	}
	for (int i = 0; i <= num_arg; i++)
		exec_stack.pop();
	// прыгаем на первую инструкцию функции
	poliz_index = func_tmp->address;
	lex = *((*poliz)[poliz_index]);
}

// переходы в ленивых вычислениях
void Executer::booltgo()
{
	scan_lex();
	int num = exec_stack.get_num() - 1;
	if (exec_stack[num]->value.int_val == 1) {
		poliz_index = lex.value.int_val;
		lex = *((*poliz)[poliz_index]);
	} else
		scan_lex();
}

void Executer::boolfgo()
{
	scan_lex();
	int num = exec_stack.get_num() - 1;
	if (exec_stack[num]->value.int_val == 0) {
		poliz_index = lex.value.int_val;
		lex = *((*poliz)[poliz_index]);
	} else
		scan_lex();
}

void Executer::not_()
{
	int num = exec_stack.get_num() - 1;
	t_value &value = exec_stack[num]->value;
	value.int_val = not value.int_val;
	scan_lex();
}


int str_to_int(char *str)
{
	int integer = 0;
	int i = 0, sign = 1;
	if (str[0] == '-') {
		sign = -1;
		i = 1;
	}
	int c = str[i++];
	
	while ('0' <= c && c <= '9') {
		integer = integer * 10 + (c - '0');
		c = str[i++];
	}
	if (c != '\0') {
		runtime_error("wrong format of shell argument (int)");
	}
	return sign * integer;
}

double str_to_real(char *str)
{
	int integer = 0;
	double real = 0;
	int i = 0, sign = 1;
	if (str[0] == '-') {
		sign = -1;
		i = 1;
	}
	int c = str[i++];

	while ('0' <= c && c <= '9') {
		integer = integer * 10 + (c - '0');
		c = str[i++];
	}
	if (c == '.') {
		double a = 10;
		c = getchar();
		while ('0' <= c && c <= '9') {
			real = real + (c - '0') / a;
			a *= 10;
			c = str[i++];
		}
	}
	if (c != '\0') {
		runtime_error("wrong format of shell argument (real)");
	}
	return sign * (integer + real);
}

void Executer::start()
{
	// выделение место на стэке для переменных из оболочки
	scan_lex();
	while (lex.type != LEX_FIN) {
		if	(argv[argc] == 0)
			runtime_error("wrong num of shell arguments");
		switch (lex.type) {
		case LEX_INT:	local_memory.add(new memory_node(LEX_INT, 
				str_to_int(argv[argc])));
			break;
		case LEX_REAL:	local_memory.add(new memory_node(LEX_REAL,
				str_to_real(argv[argc])));
			break;
		case LEX_CHAR: local_memory.add(new memory_node(LEX_CHAR,
				argv[argc][0]));
			break;
		case LEX_STR:	local_memory.add(new memory_node(LEX_STR,
				argv[argc]));
			break;
		default:;
		}
		scan_lex(); argc++;
	}
	if (argc[argv] != 0)
		runtime_error("wrong num of shell arguments");
	scan_lex();
}
