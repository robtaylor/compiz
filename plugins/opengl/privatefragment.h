#ifndef _PRIVATEFRAGMENT_H
#define _PRIVATEFRAGMENT_H

#include <vector>

#include <opengl/fragment.h>

namespace GLFragment {

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
