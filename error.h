typedef const char * t_cstr;
class Error {
public:
	virtual ~Error() {delete name; }
	Error (const char *error_name);
	virtual void print() const = 0;
protected:
	char *name;
};

class SyntaxError: public Error {
public:
	SyntaxError(const char *error_name, int n);
	~SyntaxError() {}
	void print() const;
private:
	int line;
};

class RuntimeError: public Error {
public:
	RuntimeError(const char *error_name): Error(error_name) {};
	~RuntimeError() {}
	void print() const;
};

class OtherError: public Error {
public:
	OtherError(const char *error_name): Error(error_name) {};
	~OtherError() {}
	void print() const;
};

