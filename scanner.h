#include <cstdio>
#include "tables.h"
class Scanner {
public:
	Scanner(t_cstr name_file = 0);
	~Scanner();
	t_cstr get_lex_name(Lex& lex);
	const Lex get_lex();
	void error(t_cstr name);
private:
	static t_lex const lex_keyword_table[];
	static t_cstr const str_keyword_table[];
	static t_lex const lex_token_table[];
	static t_cstr const str_token_table[];

	FILE *file;
	int line, column;
	char c;
	void scan_char();
	void skip_blank();
	void skip_comment();
	char *scan_word();
	const Lex lex(t_lex type = LEX_NULL, t_value value = 0);
	const Lex get_word_lex();
	const Lex get_num_lex();
	const Lex get_string_lex();
	const Lex get_char_lex();
	const Lex get_onechar_lex();
	const Lex get_other_lex();
	t_lex find_keyword(char *buf);
	t_lex find_token(char *buf);
};
