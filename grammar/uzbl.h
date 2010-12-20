
struct Argument;
struct Command;

enum {ARG_STR, ARG_COMMAND};

struct Argument {
	union {
		char * str;
		struct Command * command;
	} argument;
	int type;
	struct Argument * next;
};

struct Command {
	char * command;
	struct Argument * args;
	struct Command * next;
};
