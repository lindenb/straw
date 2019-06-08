#ifndef DEBUG_HH
#define DEBUG_HH
#include <stdexcept>
#include <sstream>
#include <iostream>

#ifndef NDEBUG

#define DEBUG(a) do{ std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] " << a << "." << std::endl;} while(0)

#else

#define DEBUG(a) do{} while(0)

#endif


#define THROW(TYPE,a) do {std::ostringstream _os;\
	_os << "[" << __FILE__ << ":" << __LINE__ << "] " << a << ".";\
	std::string _str=_os.str();\
	throw TYPE(_str);\
	} while(0);

#define THROW_ERROR(a) THROW(runtime_error,a)

#endif

