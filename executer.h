// фрейм стэка исполнения
struct exec_node
{
	exec_node(t_lex type, t_value value, bool is_global = false, int index = -1);
	t_lex type;
	t_value value;
	bool is_global;
	int index;
};

// фрейм памяти
struct memory_node
{
	memory_node(t_lex type, t_value value, Table<int> *size = NULL);
	t_lex type;
	t_value value;
	Table <int> *size;
	Table <t_value> memory;
};

//объект для вызова функции во время исполнения
struct func_node
{
	func_node(Table<t_lex> *t_arg, int address);
	int address;
	Table<t_lex> *t_arg;
};

class Executer
{
public:
	Executer(Table<Lex> *poliz, char *argv[]);
	~Executer();
	int run();
	void print();
private:
	int get_index(Table<int> *size);
	void scan_lex();
	void new_variable();
	void new_func();
	void push_const();
	void push_var();
	void push_val(int address);
	void push_addr(int address);
	void go();
	void fgo();
	void case_fgo();
	void fin();
	void return_();
	void write();
	void read();
	void assign();
	void add();
	void sub();
	void mul();
	void div();
	void neg();
	void equal();
	void nequal();
	void gtr();
	void geq();
	void lss();
	void leq();
	void call();
	void booltgo();
	void boolfgo();
	void not_();
	void mod();
	void start();
	Table<exec_node> exec_stack;
	Table<memory_node> local_memory;
	Table<memory_node> global_memory;
	Table<memory_node> *memory;
	Table<func_node> func;
	Table<int> func_addr;
	Table<Lex> *poliz;
	Lex lex;
	int poliz_index; // счётчик полиза
	bool is_global; // является ли память глобальной
	int argc;
	char **argv;
};
