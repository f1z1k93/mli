#include "tables.h"
static t_lex lex_table[] = 
{
	LEX_MAIN, LEX_INT, LEX_REAL, LEX_BOOL, LEX_STR, LEX_CHAR,
	LEX_ARRAY, LEX_FUNC, LEX_FIN, LEX_COMMA,
	LEX_END, LEX_ASSIGN,
	LEX_PLUS, LEX_MINUS, LEX_DIV, LEX_MUL, LEX_CALL,
	POLIZ_FGO, POLIZ_GO, LEX_RETURN,
	POLIZ_CONST, LEX_NEG,
	POLIZ_BOOLFGO, POLIZ_BOOLTGO,
	POLIZ_VAR, POLIZ_ADDR, POLIZ_CONST,
	LEX_WRITE, LEX_READ,
	LEX_NULL
};

static t_cstr str_table[] =
{
	"main ", "int ", "real " ,"bool ", "str ", "char ",
	"array ", "func ", "\n", ", ", 
	"func_end\n", "= ",
	"+ ", "- ", "/ ", "* ", "call ",
	"fgo ", "go ", "return ",
	"const ", "$ ",
	"bool_fgo ", "bool_tgo ",
	"var ", "& ", "const ",
	"write ", "read "
};

// вывод полиза на экран(не доработан)
void Lex::print()
{
	switch (type) {
	case LEX_STRING:
		printf("\"%s\" ", value.str_val);return;
	case LEX_NUM: case LEX_BOOL:
		printf("%d ", value.int_val);return;
	case LEX_REALNUM:
		printf("%g ", value.real_val);return;
	default:; 
	}

	for (int i = 0; lex_table[i] != LEX_NULL; i++) {
		if (lex_table[i] == type) {
			printf("%s", str_table[i]);
			return;
		}
	}
	printf("unknown %d ", type);
}

t_cstr str_type(t_lex a)
{
	switch (a) {
	case LEX_INT: return "int";
	case LEX_REAL: return "real";
	case LEX_BOOL: return "bool";
	case LEX_STR: return "str";
	case LEX_CHAR: return "char";
	default: return "unknown type";
	}
	return "unknown type";
}

Ident::Ident(t_lex type, char *name, t_id id, Table<int> *arr)
		: type(type), id(id)
{
	this->name = name;
	if (id == ID_VAR) return;
	
	int num = arr->get_num();
	for (int i = 0; i < num; i++) {
		int tmp = *((*arr)[i]);
		(this->arr).add(new int(tmp));
	}
	return;
}
