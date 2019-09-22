#include <cstring>
#include "error.h"
#include "parser.h"

Parser::Parser(t_cstr name_file, Table<Lex> *poliz): scanner(name_file)
{
	this->poliz = poliz;
	name = NULL;
	num_table = 1;
	argc = 0;
	is_switch = is_cycle = false;
	is_func = is_const = false;
	num_break = num_continue = 0;
	lex = scanner.get_lex();
	return;
}

Parser::~Parser()
{
	type_stack.flush();
	break_table.flush();
	continue_table.flush();
	or_table.flush();
	and_table.flush();
	global_table.flush();
	local_table.flush();
}


void Parser::scan_lex(t_lex needed_lex = LEX_EVERY, t_cstr error_str = NULL)
{
	if (needed_lex != LEX_EVERY && lex.type != needed_lex)
		scanner.error(error_str);
	lex = scanner.get_lex();
	return;
}

bool Parser::is_type(t_lex lex_type)
{
	return (lex_type == LEX_INT || lex_type == LEX_STR ||
			lex_type == LEX_BOOL || lex_type == LEX_REAL || 
			lex.type == LEX_CHAR
	);
}

int Parser::find(Table<Ident> *table, char *name)
{
	int num = table->get_num();
	char *table_name;
	for (int i = 0; i < num; i++) {
		table_name = (*table)[i]->name;
		if (strcmp(table_name, name) == 0)
			return i;
	}
	return -1;
}

// все функции реализуют рекурсивный спуск
void Parser::parser()
{
	program();
	if (lex.type != LEX_NULL)
		scanner.error("expected end of file");
}

void Parser::program()
{
	global_declarations();
	scan_lex(LEX_MAIN, "expected 'main' token");
	ret_num = 0;
	main_arguments();
	char *func_name = new char [5];
	strcpy(func_name, "main");
	func_table.add(new Ident(LEX_INT, func_name, ID_FUNC, &arr));
	num_call_func = func_table.get_num() - 1;
	poliz->add(new Lex(LEX_MAIN));
	int arg_num = arr.get_num();
	t_lex arg_type;
	for (int i = 0; i < arg_num; i++) {
		arg_type = (t_lex) *arr[i];
		poliz->add(new Lex((arg_type)));
	}
	poliz->add(new Lex(LEX_FIN));
	body();
	poliz->add(new Lex(POLIZ_CONST));
	poliz->add(new Lex(LEX_NUM, 0));
	poliz->add(new Lex(LEX_RETURN));
	poliz->add(new Lex(LEX_NUM, ret_num));
	arr.flush();
}

void Parser::global_declarations()
{
	while (is_type(lex.type) || lex.type == LEX_ARRAY || 
		lex.type == LEX_FUNC
	) {
		global_declaration();
	}
}

void Parser::global_declaration()
{
	if (lex.type == LEX_FUNC) {
		scan_lex();
		function();
		num_table++;
		return;
	}
	int tmp = num_table; num_table = 0;
	poliz->add(new Lex(POLIZ_GLOBAL));
	if (lex.type == LEX_ARRAY) {
		scan_lex();
		scan_lex(LEX_OF, "expected 'of' after 'array'");
		array();
		num_table = tmp;
		scan_lex(LEX_FIN, "expected ';' after after declaration");
	} else {
		variable();
		scan_lex(LEX_FIN, "expected ';' after after declaration");
	}
	num_table = tmp;
}

void Parser::array()
{
	Table<Ident> *table = num_table ? &local_table : &global_table;
	type();
	array_size();
	while (true) {
		id();
		// имя в области видимости должно быть единственным
		if (find(table, name) != -1)
			scanner.error("name is used");
		table->add(new Ident(dtype, name, ID_ARRAY, &arr));
		poliz->add(new Lex(dtype));
		poliz->add(new Lex(LEX_ARRAY));
		int num = arr.get_num();
		for (int i = 0; i < num; i++)
			poliz->add(new Lex(LEX_NUM, *arr[i]));
		poliz->add(new Lex(LEX_FIN));
		if (lex.type != LEX_COMMA) break;
			scan_lex();
		ret_num++;
	}
	arr.flush();
}

void Parser::variable()
{
	type();
	while (true) {
		id();
		Table<Ident> *tmp_table = (num_table == 0) ?
			&global_table : &local_table;
		if (find(tmp_table, name) != -1)
			scanner.error("name is used");
		tmp_table->add(new Ident(dtype, name));
		init();
		poliz->add(new Lex(dtype));
		poliz->add(new Lex(dconst.type, dconst.value));
		poliz->add(new Lex(LEX_FIN));
		if (lex.type != LEX_COMMA) break;
		scan_lex();
	}
}

void Parser::declaration()
{
	if (lex.type == LEX_ARRAY) {
		scan_lex();
		scan_lex(LEX_OF, "expected 'of' after 'array'");
		array();	
	} else {
		variable();
	}
}

void Parser::array_size()
{
	scan_lex(LEX_LBRACKET, "expected '[' in array declaration");
	// формирование массива типов для функции
	while (true) {
		if (lex.type != LEX_NUM)
			scanner.error("array size must be const natural number");
		arr.add(new int(lex.value.int_val));
		scan_lex();
		if (lex.type != LEX_COMMA) break;
		scan_lex();
	}
	scan_lex(LEX_RBRACKET, "expected ']'in array declaration");
}

t_lex num_type(t_lex type)
{
	switch (type) {
	case LEX_INT: case LEX_BOOL: case LEX_CHAR:
		return LEX_NUM;
	case LEX_REAL: return LEX_REALNUM;
	case LEX_STR: return LEX_STRING;
	default:;
	}
	return LEX_NULL;
}

t_lex var_type(t_lex type)
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

// возвращает ноль определённого типа
t_value get_zerro(t_lex type)
{
	t_value value;
	switch (type) {
	case LEX_INT: case LEX_CHAR: case LEX_BOOL:
		value.int_val = 0; break;
	case LEX_REAL:
		value.real_val = 0; break;
	case LEX_STR:
		value.str_val = new char [1];
		value.str_val[0] = '\0';
		break;
	default:;
	}
	return value;
}


void Parser::init()
{
	if (lex.type == LEX_ASSIGN) {
		scan_lex();
		constant();
		// правильность типа присвоенной константы
		if (dtype != var_type(dconst.type))
			scanner.error("type missmatch in init");
	} else {
		dconst.type = num_type(dtype);
		dconst.value = get_zerro(dtype);
	}
	ret_num++;
}

void Parser::type()
{
	if (!is_type(lex.type))
		scanner.error("expected variable type");
	dtype = lex.type;
	scan_lex();
	return;
}

void Parser::function()
{
	type();
	id();
	t_lex func_type = dtype;
	char *func_name = new char [strlen(name) + 1];
	strcpy(func_name, name);
	if (find(&func_table, name) != -1)
		scanner.error("name is used");
	ret_num = 0;
	arguments();
	func_table.add(new Ident(func_type, func_name, ID_FUNC, &arr));
	num_call_func = func_table.get_num() - 1;
	// LEX_FUNC
	poliz->add(new Lex(LEX_FUNC));
	int arg_num = arr.get_num();
	t_lex arg_type;
	// список типов функции(нужен во время исполнения)
	for (int i = 0; i < arg_num; i++) {
		arg_type = (t_lex) *arr[i];
		poliz->add(new Lex((arg_type)));
	}
	int address = poliz->get_num();
	int addr_end_func = address + 3;
	// адрес входа в функцию
	poliz->add(new Lex(LEX_NUM, address + 4));
	poliz->add(new Lex(LEX_FIN));
	// прожок на послефункцию
	poliz->add(new Lex(POLIZ_GO));
	poliz->add(new Lex(LEX_NUM, 0));
	body();
	// генерация return <кол-во удал объектов>
	poliz->add(new Lex(POLIZ_CONST));
	poliz->add(new Lex(num_type(dtype), get_zerro(dtype)));
	poliz->add(new Lex(LEX_RETURN));
	poliz->add(new Lex(LEX_NUM, ret_num));
	(*poliz)[addr_end_func]->value.int_val =
		poliz->get_num();
	arr.flush();	
	local_table.flush();
}

void Parser::main_arguments()
{
	scan_lex(LEX_LPAREN, "expected '(' in arguments declaration");
	argc = 0;
	while (is_type(lex.type)) {
		if (lex.type != LEX_INT && lex.type != LEX_REAL && lex.type != LEX_STR)
			scanner.error("wrong main argument type");
		type();
		id();
		if (find(&local_table, name) != -1)
			scanner.error("name is used");	
		local_table.add(new Ident(dtype, name));
		arr.add(new int(dtype));
		ret_num++;
		if (lex.type != LEX_COMMA) break;
		scan_lex();
	}
	if (lex.type != LEX_RPAREN)
		scanner.error("expected ')' in arguments declaration");
	scan_lex();
}


void Parser::arguments()
{
	scan_lex(LEX_LPAREN, "expected '(' in arguments declaration");
	argc = 0;
	while (is_type(lex.type)) {
		type();
		id();
		if (find(&local_table, name) != -1)
			scanner.error("name is used");	
		local_table.add(new Ident(dtype, name));
		arr.add(new int(dtype));
		ret_num++;
		if (lex.type != LEX_COMMA) break;
		scan_lex();
	}
	if (lex.type != LEX_RPAREN)
		scanner.error("expected ')' in arguments declaration");
	scan_lex();
}

void Parser::body()
{
	scan_lex(LEX_BEGIN, "expected '{'");
	while (lex.type != LEX_END && lex.type != LEX_NULL) {
		body_operator();
	}
	scan_lex(LEX_END, "expected '}'");
}

bool is_begin_of_operator(t_lex lex)
{
	if (lex == LEX_IF || lex == LEX_WHILE || lex == LEX_DO || lex == LEX_FOR || 
		lex == LEX_SWITCH || lex == LEX_READ || lex == LEX_WRITE || lex == LEX_ID || 
		lex == LEX_BREAK || lex == LEX_CONTINUE || lex == LEX_RETURN
	)
		return true;
	return false;
}

void Parser::block()
{
	if (lex.type == LEX_FIN) {
		scan_lex();
	} else if (lex.type == LEX_BEGIN) {
		scan_lex();
		while (lex.type != LEX_END && lex.type != LEX_NULL)
			operator_();
		scan_lex(LEX_END, "expected '}'");
	} else {
		operator_();
	}
}

void Parser::body_operator()
{
	if (is_type(lex.type) || lex.type == LEX_ARRAY) {
		declaration();
		scan_lex(LEX_FIN, "declaration must be end by ';'");
	} else
		operator_();
}

// операторы
void Parser::operator_()
{
	switch (lex.type) {
	case LEX_FIN:
		scan_lex(); return;
	case LEX_IF: scan_lex();	
		operator_if();return;
	case LEX_WHILE: scan_lex();	
		operator_while();return;
	case LEX_DO: scan_lex();	
		operator_do();return;
	case LEX_FOR: scan_lex();	
		operator_for();return;
	case LEX_SWITCH: scan_lex();	
		operator_switch();return;
	case LEX_READ: scan_lex();	
		operator_read();break;
	case LEX_WRITE:	scan_lex();	
		operator_write();break;
	case LEX_BREAK: scan_lex();	
		operator_break();break;
	case LEX_CONTINUE: scan_lex();	
		operator_continue();break;
	case LEX_RETURN: scan_lex();	
		operator_return();break;
	case LEX_ID:
		operator_simple();break;
	default:
		scanner.error("wrong operator");
	}
	scan_lex(LEX_FIN, "this operator must ended by ';'");
}

void Parser::operator_simple()
{
	is_func = is_const = false;
	mono1();
	if (lex.type != LEX_FIN && lex.type != LEX_ASSIGN)
		scanner.error("wrong format of lvalue");
	if (lex.type == LEX_ASSIGN) {
		if (is_func || is_const)
			scanner.error("lvalue can't be function or constant");
		poliz->add(new Lex(POLIZ_ADDR));
		scan_lex();
		expression();
		poliz->add(new Lex(LEX_ASSIGN));
		check_types_assign();
	} else {
		if (!is_func)
			scanner.error("value must be used");
	}
	type_stack.pop();
	poliz->add(new Lex(LEX_FIN));
}

void Parser::operator_if()
{
	scan_lex(LEX_LPAREN, "expected '(' in operator 'if'");
	expression();
	scan_lex(LEX_RPAREN, "expected ')' after expression in operator 'if'");
	check_type_fgo();
	poliz->add(new Lex(POLIZ_FGO));
	poliz->add(new Lex(LEX_NUM, 0));
	int fgo_addr = poliz->get_num() - 1;
	block();
	int jmp_addr1 = poliz->get_num();
	if (lex.type == LEX_ELSE) {
		scan_lex();
		jmp_addr1 += 2;
		poliz->add(new Lex(POLIZ_GO));
		poliz->add(new Lex(LEX_NUM, 0));
		int go_addr = poliz->get_num() - 1;
		block();
		int jmp_addr2 = poliz->get_num() -1;
		(*poliz)[go_addr]->value.int_val = jmp_addr2;
	}
	(*poliz)[fgo_addr]->value.int_val = jmp_addr1;
}

void init_table(Table<int *>&table, int num, int num_jmp)
{
	for (int i = 0; i < num_jmp; i++) {
		**(table.pop()) = num;
	}
}

void Parser::operator_while()
{	
	scan_lex(LEX_LPAREN, "after 'while' expected  '('");
	int expr_addr = poliz->get_num();
	expression();
	scan_lex(LEX_RPAREN,"expected ')'after expression in operator 'while'");
	int fgo_addr = poliz->get_num();
	poliz->add(new Lex(POLIZ_FGO));
	poliz->add(new Lex(LEX_NUM, 0));
	int break_num_tmp = num_break;
	num_break = 0;
	int continue_num_tmp = num_continue;
	num_continue = 0;
	int tmp_is_cycle = is_cycle;
	is_cycle = true;
	block();
	is_cycle = tmp_is_cycle;
	poliz->add(new Lex(POLIZ_GO));
	poliz->add(new Lex(LEX_NUM, expr_addr));
	int exit_addr = poliz->get_num();
	(*poliz)[fgo_addr + 1]->value.int_val = exit_addr;
	init_table(break_table, exit_addr, num_break);
	init_table(continue_table, fgo_addr + 2, num_continue);
	num_break = break_num_tmp;
	num_continue = continue_num_tmp;	
}


void Parser::operator_do()
{
	int jmp_addr = poliz->get_num();
	int break_num_tmp = num_break;
	num_break = 0;
	int continue_num_tmp = num_continue;
	num_continue = 0;
	bool tmp_is_cycle = is_cycle;
	is_cycle = true;
	block();
	is_cycle = tmp_is_cycle;
	scan_lex(LEX_WHILE, "expected 'while' at end of operator 'do'");
	scan_lex(LEX_LPAREN, "expected '(' after 'while' in operator 'do'");
	expression();
	check_type_fgo();
	scan_lex(LEX_RPAREN, "expected ')' after expression in operator 'do'");
	int exit_addr = poliz->get_num() + 4;
	poliz->add(new Lex(POLIZ_FGO));
	poliz->add(new Lex(LEX_NUM, exit_addr));
	poliz->add(new Lex(POLIZ_GO));
	poliz->add(new Lex(LEX_NUM, jmp_addr));
	init_table(break_table, exit_addr, num_break);	
	init_table(continue_table, jmp_addr, num_continue);
	num_break = break_num_tmp;
	num_continue = continue_num_tmp;
}

bool is_mono(t_lex);
void Parser::operator_for()
{

	scan_lex(LEX_LPAREN, "after 'for' expected '('");
	if (is_mono(lex.type)) {
		operator_simple();
	}
	scan_lex(LEX_FIN, "expected ; in operator 'for'");

	int cmp_addr = poliz->get_num();
	if (is_mono(lex.type)) {
		expression();
		check_type_fgo();
	} else {
		poliz->add(new Lex(POLIZ_CONST));
		poliz->add(new Lex(LEX_TRUE));
	}
	scan_lex(LEX_FIN, "expected ; in operator 'for'");

	// if false goto exit
	int step_addr = poliz->get_num();
	poliz->add(new Lex(POLIZ_FGO));
	poliz->add(new Lex(LEX_NUM, 0));
	poliz->add(new Lex(POLIZ_GO));
	poliz->add(new Lex(LEX_NUM, 0));
	if (is_mono(lex.type)) {
		operator_simple();
	}
	scan_lex(LEX_RPAREN, "expected ')' after expression in operator 'for'");
		
	poliz->add(new Lex(POLIZ_GO));
	poliz->add(new Lex(LEX_NUM, cmp_addr));

	int block_addr = poliz->get_num();
	int break_num_tmp = num_break; num_break = 0;
	int continue_num_tmp = num_continue; num_continue = 0;
	bool tmp_is_cycle = is_cycle; is_cycle = true;
	block();
	is_cycle = tmp_is_cycle;
	poliz->add(new Lex(POLIZ_GO));
	poliz->add(new Lex(LEX_NUM, step_addr + 4));
	
	int exit_addr = poliz->get_num();
	(*poliz)[step_addr + 1]->value.int_val = exit_addr;
	(*poliz)[step_addr + 3]->value.int_val = block_addr;

	init_table(break_table, exit_addr, num_break);	
	init_table(continue_table, block_addr, num_continue);
	num_break = break_num_tmp;
	num_continue = continue_num_tmp;
}


void Parser::operator_switch()
{
	scan_lex(LEX_LPAREN, "after 'switch' expected '('");
	expression();
	scan_lex(LEX_RPAREN,"expected')'after expression in operator'switch'");
	scan_lex(LEX_BEGIN, "expected '{' in operator 'switch'");
	int last_index = type_stack.get_num() - 1;
	t_lex switch_type = *(type_stack[last_index]);
	if (switch_type != LEX_BOOL && switch_type != LEX_INT && 
		switch_type != LEX_CHAR)
		scanner.error("expr type must be enum type");
	int case_addr;
	int num_break_tmp = num_break;
	num_break = 0;
	bool is_switch_tmp = is_switch;
	is_switch = true;
	while (lex.type == LEX_CASE) {
		scan_lex();
		constant();
		t_lex case_type = var_type(dconst.type);
		if (case_type != switch_type)
			scanner.error("type mismatch in case");
		case_addr = poliz->get_num();
		poliz->add(new Lex(POLIZ_CONST));
		poliz->add(new Lex(LEX_NUM, dconst.value.int_val));
		poliz->add(new Lex(POLIZ_CASEFGO));
		poliz->add(new Lex(LEX_NUM, 0));
		
		scan_lex(LEX_COLON, "expected ':' after expression");
		block();
		(*poliz)[case_addr + 3]->value.int_val = poliz->get_num();
	}
	if (lex.type == LEX_DEFAULT) {
		scan_lex();
		scan_lex(LEX_COLON,"expected':'after default in operator'switch'");
		block();
	}
	int exit_addr = poliz->get_num();
	init_table(break_table, exit_addr, num_break);
	num_break = num_break_tmp;
	is_switch = is_switch_tmp;
	scan_lex(LEX_END, "expected '}' in operator 'switch'");
}

void Parser::operator_read()
{
	scan_lex(LEX_LPAREN, "after 'read' expected '('");
	t_lex print_type;
	for (;;) {
		if (lex.type != LEX_ID)
			scanner.error("read argument must be variable");
		mono();
		if (is_func || is_const)
			scanner.error("read argument must be variable");
		print_type = *(type_stack.pop());
		if (print_type == LEX_BOOL)
			scanner.error("type of read argument can't be 'bool'");
		poliz->add(new Lex(POLIZ_ADDR));
		poliz->add(new Lex(LEX_READ));
		if (lex.type != LEX_COMMA) break;
		scan_lex();
	};
	scan_lex(LEX_RPAREN, "expected ')' after arguments in operator 'read'");
}

void Parser::operator_write()
{
	scan_lex(LEX_LPAREN, "after 'write' expected '('");
	t_lex write_type;
	for (;;) {
		expression();
		write_type = *(type_stack.pop());
		if (write_type == LEX_BOOL)
			scanner.error("bool isn't write type");
		poliz->add(new Lex(LEX_WRITE));
		if (lex.type != LEX_COMMA) break;
		scan_lex();
	}
	scan_lex(LEX_RPAREN,"expected ')' after arguments in operator 'write'");
}

void Parser::operator_break()
{
	if (!is_cycle && !is_switch)
		scanner.error("wrong used 'break'");
	int num = poliz->get_num() + 1;
	poliz->add(new Lex(POLIZ_GO));
	poliz->add(new Lex(LEX_NUM, ret_num));
	
	int *tmp = &((*poliz)[num]->value.int_val);
	// таблица содержит адреса инструкции для изменения
	// адресов в этих инструкциях
	break_table.add(new (int *)(tmp));
	num_break++;
	return;
}

void Parser::operator_continue()
{
	if (!is_cycle)
		scanner.error("wrong used 'continue'");
	poliz->add(new Lex(POLIZ_GO));
	poliz->add(new Lex(LEX_NUM, 0));

	int num = poliz->get_num() - 1;
	int *tmp = &((*poliz)[num]->value.int_val);
	continue_table.add(new (int *)(tmp));
	num_continue++;
	return;
}

// ретурн похож на ассемблерный
void Parser::operator_return()
{
	int num = func_table.get_num() - 1;
	Ident ident = *(func_table[num]);
	expression();
	t_lex ret_type = *(type_stack.pop());
	if (ret_type != ident.type)
		scanner.error("type mismatch");
	poliz->add(new Lex(LEX_RETURN));
	poliz->add(new Lex(LEX_NUM, ret_num));
	return;
}

void Parser::index()
{
	int num = ((*table)[num_id]->arr).get_num();
	int i = 0;
	while (true) {
		if (!is_mono(lex.type)) break;
		expression();
		t_lex index_type = *(type_stack.pop());
		if (index_type != LEX_INT)
			scanner.error("type of index must be 'int'");
		i++;
		if (lex.type != LEX_COMMA) break;
		scan_lex();
	}
	scan_lex(LEX_RBRACKET,"expected ']' after index in array variable");
	if (i != num)
		scanner.error("wrong num of indexes");
}

void Parser::expression()
{
	mono_num = 0;
	bool tmp_func = is_func, tmp_const = is_const;
	is_func = is_const = false;
	value();
	if (lex.type == LEX_ASSIGN) {
		if (is_func || is_const)
			scanner.error("function or constant can't be left operand of assignment");
		if (mono_num > 1)
			scanner.error("wrong format of lvalue");
		poliz->add(new Lex(POLIZ_ADDR));
		scan_lex();
		value();
		check_types_assign();
		poliz->add(new Lex(LEX_ASSIGN));
	}
	is_func = tmp_func;
	is_const = tmp_const;
	return;
}

void Parser::value()
{
	int *tmp;
	int num, i = 0;
	or_value();
	while (lex.type == LEX_OR) {
		check_type_fgo();
		type_stack.add(new t_lex(LEX_BOOL));
		num = poliz->get_num() + 1;
		poliz->add(new Lex(POLIZ_BOOLTGO));
		poliz->add(new Lex(LEX_NUM, 0));
		poliz->add(new Lex(LEX_FIN));
		tmp = &((*poliz)[num]->value.int_val);
		or_table.add(new (int *)(tmp));
		i++;
		scan_lex();
		or_value();
	}
	num = poliz->get_num();
	init_table(or_table, num, i);
	return;
}

void Parser::or_value()
{
	int *tmp;
	int num; int i = 0;
	and_value();
	while (lex.type == LEX_AND) {
		check_type_fgo();
		type_stack.add(new t_lex(LEX_BOOL));
		num = poliz->get_num() + 1;
		poliz->add(new Lex(POLIZ_BOOLFGO));
		poliz->add(new Lex(LEX_NUM, 0));
		poliz->add(new Lex(LEX_FIN));
		tmp = &((*poliz)[num]->value.int_val);
		and_table.add(new (int *)(tmp));
		i++;
		scan_lex();
		and_value();
	}
	num = poliz->get_num();
	init_table(and_table, num, i);
	return;
}

void Parser::and_value()
{
	bool is_not = false;
	if (lex.type == LEX_NOT) {
		is_not = true;
		scan_lex();
	}
	bool_value();
	if (is_bool_operation(lex.type)) {
		t_lex bool_op = lex.type;
		bool_operation();
		bool_value();
		check_types_cmp();
		poliz->add(new Lex(bool_op));
	}
	if (is_not) {
		check_types_not();
		type_stack.add(new t_lex(LEX_BOOL));
		poliz->add(new Lex(LEX_NOT));
	}
	return;
}

void Parser::bool_value()
{
	t_lex op;
	add_value();
	while (lex.type == LEX_PLUS || lex.type == LEX_MINUS) {
		op = lex.type;
		scan_lex();
		add_value();
		if (op == LEX_PLUS)
			check_types_with_str();
		else
			check_types_math();
		poliz->add(new Lex(op));
	}
	return;
}

void Parser::add_value()
{
	t_lex op;
	mul_value();
	while (lex.type == LEX_MUL || lex.type == LEX_DIV || lex.type == LEX_MOD) {
		op = lex.type;
		scan_lex();
		mul_value();
		if (op == LEX_MOD)
			check_types_mod();
		else
			check_types_math();
		poliz->add(new Lex(op));
	}
	return;
}

void Parser::mul_value()
{
	bool is_sign = false;
	t_lex op;
	if (lex.type == LEX_PLUS || lex.type == LEX_MINUS) {
		is_sign = true;
		op = lex.type;
		scan_lex();
	}

	if (lex.type == LEX_LPAREN) {
		scan_lex();
		expression();
		scan_lex(LEX_RPAREN, "expected ')' after expression");
	} else {
		mono();
	}
	if (is_sign) {
		check_types_minus_plus();
		if (op == LEX_MINUS)
			poliz->add(new Lex(LEX_NEG));
	}
}

bool is_mono(t_lex lex)
{
	if (lex != LEX_ID && 
		lex != LEX_NUM && lex != LEX_REALNUM && lex != LEX_STRING &&
		lex != LEX_SYMBOL && lex != LEX_PLUS  &&
		lex != LEX_MINUS && lex != LEX_TRUE && lex != LEX_FALSE
	) return false;
	return true;
}

void Parser::mono()
{
	if (!is_mono(lex.type))
		scanner.error("wrong format of operand");
	if (lex.type == LEX_ID) {
		mono1();
	} else {
		is_const = true;
		constant();
		t_lex type = var_type(dconst.type);
		type_stack.add(new t_lex(type));
		poliz->add(new Lex(POLIZ_CONST));
		poliz->add(new Lex(dconst.type, dconst.value));
	}
	return;
}

void Parser::mono1()
{
	id();
	if (lex.type == LEX_LPAREN) {
		is_func = true;
		num_call_func = find(&func_table, name);
		if (num_call_func == -1)
			scanner.error("function not found");
		call_func();
		return;
	} else {
		num_id = find(&local_table, name);
		if (num_id == -1) {
			num_id = find(&global_table, name);
			if (num_id != -1)
				table = &global_table;
			else
				scanner.error("variable not found");
		} else
			table = &local_table;
		var_ident();
	}
}

void Parser::var_ident()
{
	// генерация команд в полизе для выделения памяти
	Table<Ident> *table = this->table;
	int tmp = num_id;
	if (lex.type == LEX_LBRACKET) {
		if ((*table)[num_id]->id == ID_ARRAY) {
			scan_lex();
			index();
		} else
			scanner.error("operator [] defined for array");
	} else {
		if ((*table)[num_id]->id == ID_ARRAY)
			scanner.error("expected operator []");
	}
	num_id = tmp;
	t_lex type = (*table)[num_id]->type;
	type_stack.add(new t_lex(type));
	poliz->add(new Lex(POLIZ_VAR));
	if (table == &global_table) {
		poliz->add(new Lex(POLIZ_GLOBAL));
		poliz->add(new Lex(LEX_NUM, num_id));
	} else {
		num_id = local_table.get_num() - num_id - 1;
		poliz->add(new Lex(LEX_NUM, num_id));
	}

}

void Parser::call_func()
{
	scan_lex(LEX_LPAREN, "expected '(' after function name");
	Table<int> &arr = func_table[num_call_func]->arr;
	t_lex type = (func_table[num_call_func])->type;
	type_stack.add(new t_lex(type));
	int num = arr.get_num();
	int i = 0;
	for (i = 0 ; i < num ; i++) {
		t_lex arg_type = (t_lex) *arr[i];
		if (!is_mono(lex.type)) break;
		expression();
		t_lex expr_type = *(type_stack.pop());
		if (arg_type != expr_type)
			scanner.error("type missmatch of arguments in call of func");
		if (lex.type != LEX_COMMA) break;
		scan_lex();
	}
	scan_lex(LEX_RPAREN, "expected ')' after function arguments");
	// генерация команды:
	// вызвать функцию по номеру записи в таблице
	// запись содержит типы аргументов, возвращаемого значения, адрес
	poliz->add(new Lex(LEX_CALL));
	poliz->add(new Lex(LEX_NUM, num_call_func));
}

bool Parser::is_bool_operation(t_lex type)
{
	return (type == LEX_GTR || type == LEX_LSS ||
			type == LEX_GEQ || type == LEX_LEQ ||
			type == LEX_NEQ || type == LEX_EQ
	);
}

void Parser::bool_operation()
{
	scan_lex();
	return;
}

void Parser::id()
{
	if (lex.type != LEX_ID)
		scanner.error("miss identifier");
//	if (name) delete name;
	name = new char [strlen(lex.value.str_val) + 1];
	strcpy(name, lex.value.str_val);
	scan_lex();
	return;
}
void Parser::constant()
{
	int sign = 0;
	if (lex.type == LEX_PLUS) {
		sign = 1;
		scan_lex();
	} else if (lex.type == LEX_MINUS) {
		sign = -1;
		scan_lex();
	}
	dconst.type = lex.type;
	if (lex.type == LEX_NUM) {
		sign = sign >= 0 ? 1 : -1;
		dconst.value.int_val = sign * lex.value.int_val;
	} else if (lex.type == LEX_REALNUM) {
		sign = sign >= 0 ? 1 : -1;
		dconst.value.real_val = sign * lex.value.real_val;
	} else if (lex.type == LEX_TRUE) {
		if (sign) scanner.error("wrong bool constant");
		dconst.value.int_val = 1;
	} else if (lex.type == LEX_FALSE) {
		if (sign) scanner.error("wrong bool constant");
		dconst.value.int_val = 0;
	} else if (lex.type == LEX_SYMBOL) {
		if (sign) scanner.error("wrong char constant");
		dconst.value.int_val = lex.value.int_val;
	} else if (lex.type == LEX_STRING) {
		if (sign) scanner.error("wrong string constant");
		dconst.value.str_val = new char [strlen(lex.value.str_val) + 1];
		strcpy(dconst.value.str_val, lex.value.str_val);
	}
	scan_lex();
}

// методы проверок типов для разных операций (семантический анализ)
void Parser::check_types_minus_plus()
{
	t_lex *pt1 = type_stack.pop();
	t_lex t1 = *pt1; delete pt1;
	if (t1 != LEX_INT && t1 != LEX_REAL)
		scanner.error("wrong type for this unary operation");
	type_stack.add(new t_lex(t1));
}

void Parser::check_types_not()
{
	t_lex *pt1 = type_stack.pop();
	t_lex t1 = *pt1; delete pt1;
	if (t1 != LEX_BOOL)
		scanner.error("wrong type for operation 'not'");
	type_stack.add(new t_lex(LEX_BOOL));
}

void Parser::check_types_with_str()
{
	t_lex *pt1 = type_stack.pop();
	t_lex t1 = *pt1; delete pt1;
	t_lex *pt2 = type_stack.pop();
	t_lex t2 = *pt2; delete pt2;
	if (t1 == LEX_STR) {
		if (t2 == LEX_STR) {
			type_stack.add(new t_lex(LEX_STR)); return;
		} else
			scanner.error("type mismatch");
	}
	if ((t1!=LEX_INT && t1!=LEX_REAL) || 
		(t2!=LEX_INT && t2!=LEX_REAL)
	)
		scanner.error("wrong type for this operation");
	t_lex type = t1 == t2 ? LEX_INT : LEX_REAL;
	type_stack.add(new t_lex(type));
}

void Parser::check_types_math()
{
	t_lex *pt1 = type_stack.pop();
	t_lex t1 = *pt1; delete pt1;
	t_lex *pt2 = type_stack.pop();
	t_lex t2 = *pt2; delete pt2;
	if ((t1!=LEX_INT && t1!=LEX_REAL) || 
		(t2!=LEX_INT && t2!=LEX_REAL)
	)
		scanner.error("wrong type for this operation");
	t_lex type = t1 == t2 ? LEX_INT : LEX_REAL;
	type_stack.add(new t_lex(type));
}

void Parser::check_types_assign()
{
	t_lex *pt1 = type_stack.pop();
	t_lex t1 = *pt1; delete pt1;
	t_lex *pt2 = type_stack.pop();
	t_lex t2 = *pt2; delete pt2;
	if (t2 == LEX_INT) {
		if (t1 == LEX_INT || t1 == LEX_REAL)
			type_stack.add(new t_lex(LEX_INT));
		else
			scanner.error("type mismatch in assigment");
		return;
	}
	if (t2 == LEX_REAL) {
		if (t1 == LEX_INT || t1 == LEX_REAL)
			type_stack.add(new t_lex(LEX_REAL));
		else
			scanner.error("type mismatch in assigment");
		return;
	}
	if (t1 != t2)
		scanner.error("type mismatch in assigment");
	type_stack.add(new t_lex(t1));
}


void Parser::check_types_bool()
{
	t_lex *pt1 = type_stack.pop();
	t_lex t1 = *pt1; delete pt1;
	t_lex *pt2 = type_stack.pop();
	t_lex t2 = *pt2; delete pt2;
	if (t1 != t2 || t1 != LEX_BOOL)
		scanner.error("type mismatch in bool operation");
	type_stack.add(new t_lex(LEX_BOOL));
}

void Parser::check_type_fgo()
{
	t_lex *pt1 = type_stack.pop();
	t_lex t1 = *pt1; delete pt1;
	if (t1 != LEX_BOOL)
		scanner.error("expression must have bool type");
}

void Parser::check_types_cmp()
{
	t_lex *pt1 = type_stack.pop();
	t_lex t1 = *pt1; delete pt1;
	t_lex *pt2 = type_stack.pop();
	t_lex t2 = *pt2; delete pt2;
	if (t1 == t2) {
		type_stack.add(new t_lex(LEX_BOOL));
		return;
	}
	if ((t1!=LEX_INT && t1!=LEX_REAL) || 
		(t2!=LEX_INT && t2!=LEX_REAL))
		scanner.error("wrong type for this operation");
	type_stack.add(new t_lex(LEX_BOOL));
}

void Parser::check_types_mod()
{
	t_lex *pt1 = type_stack.pop();
	t_lex t1 = *pt1; delete pt1;
	t_lex *pt2 = type_stack.pop();
	t_lex t2 = *pt2; delete pt2;
	if (t1 == t2 && t1 == LEX_INT)
		type_stack.add(new t_lex(LEX_INT));
	else
		scanner.error("wrong types for '%'");
	return;
}
