#include <cstdio>
#include "error.h"
#include "parser.h"
#include "executer.h"
int main(int argc, char *argv[])
{
	try {
		Poliz poliz;
		// создаём полиз
		Parser parser(argv[1], &poliz);
		parser.parser();
		// исполняем его
		Executer executer(&poliz, argv);
	//	executer.print(); printf("\n");
		executer.run();
	}

	// ловим ошибки
	catch (const Error& err) {
		err.print();
	}
	catch (...) {
		fprintf(stderr, "unknown error\n");
	}
	return -1;
}
