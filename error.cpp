#include <cstring>
#include <cstdio>
#include "error.h"
Error::Error(const char *error_name)
{
	name = new char [strlen(error_name) + 1];
	strcpy(name, error_name);
}

SyntaxError::SyntaxError(const char *error_name, int n) 
	: Error(error_name)
{
	line = n;
}

void SyntaxError::print() const
{
	fprintf(stderr, "syntax error: %d %s\n", line, name);
}

void RuntimeError::print() const
{
	fprintf(stderr, "runtime error: %s\n", name);
}

void OtherError::print() const
{
	fprintf(stderr, "fatal error: %s\n", name);
}
