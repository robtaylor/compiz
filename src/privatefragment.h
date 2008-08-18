#ifndef _PRIVATEFRAGMENT_H
#define _PRIVATEFRAGMENT_H

#include <compfragment.h>

namespace CompFragment {

    class Function;
    class Program;

    class Storage {
	public:
	    Storage ();
	    ~Storage ();

	public:
	    int lastFunctionId;
	    std::vector<Function *> functions;
	    std::vector<Program *> programs;

	    FunctionId saturateFunction[2][64];
    };
};

#endif
