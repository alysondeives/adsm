#include <fstream>
#include <cstring>
#include <cstdarg>
#include <cassert>

#include "Logger.h"
#include "Private.h"

#include <typeinfo>
#if defined(__GNUC__)
#include <cxxabi.h>
#define demangle(name) abi::__cxa_demangle(name, NULL, 0, NULL)
#elif defined(_MSC_VER)

static char *demangle(const char *name)
{
	char *ret = NULL;
	ret = (char *)malloc(strlen(name) + 1);
	memcpy(ret, name, strlen(name) + 1);
	return ret;
}
#endif

#if defined(_MSC_VER)
static char *strcasestr(const char *haystack, const char *needle)
{
	const char *p, *startn = 0, *np = 0;
	for(p = haystack; *p; p++) {
		if(np) {
			if(toupper(*p) == toupper(*np)) {
				if(!*++np) return (char *)startn;
			}
			else {
				np = 0;
			}
		} else if (toupper(*p) == toupper(*needle)) {
			np = needle + 1;
			startn = p;
		}
	}
	return 0;
}
#endif

namespace __impl { namespace util {

#ifdef DEBUG
Atomic Logger::Ready_ = 0;
Parameter<const char *> *Logger::Level_ = NULL;
const char *Logger::DebugString_ = NULL;
std::list<std::string> *Logger::Tags_ = NULL;
PRIVATE char *Logger::Buffer_;
#endif

DESTRUCTOR(fini);
static void fini()
{
    Logger::Fini();
}

void Logger::Init()
{
#ifdef DEBUG
	Buffer_ = new char[BufferSize_];
    Tags_ = new std::list<std::string>();
    Level_ = new Parameter<const char *>(&Logger::DebugString_, "Logger::DebugString_", "none", "GMAC_DEBUG");
    char *tmp = new char[strlen(DebugString_) + 1];
    memcpy(tmp, DebugString_, strlen(DebugString_) + 1);
	char *next = NULL;
	char *tag = STRTOK(tmp, ", ", &next);
    while(tag != NULL) {
        Tags_->push_back(std::string(tag));
        tag = STRTOK(NULL, ", ", &next);
    }
    delete[] tmp;

	Ready_ = true;
#endif
}

void Logger::Fini()
{
#ifdef DEBUG
    if(Ready_ == 0) return;
    delete Level_;
    delete Tags_;;
    //if (Buffer_) delete [] Buffer_;
#endif
}

#ifdef DEBUG
bool Logger::Check(const char *name)
{
    if(AtomicTestAndSet(Ready_, 0, 1) == 0) Init();
	if(name == NULL) return true;
    std::list<std::string>::const_iterator i;
    for(i = Tags_->begin(); i != Tags_->end(); i++) {
        if(strstr(name, i->c_str()) != NULL) return true;
    }
    return false;
}

void Logger::Log(const char *name, const char *tag, const char *fmt, va_list list)
{
    if(Check(name) == false) return;
    Print(tag, name, fmt, list);
}

void Logger::Print(const char *tag, const char *name, const char *fmt, va_list list)
{
	if (Buffer_ == NULL) {
		Buffer_ = new char[BufferSize_];
	}
	
	VSNPRINTF(Buffer_, BufferSize_, fmt, list);
	if(name != NULL) fprintf(stderr,"%s [%s]: %s\n", tag, name, Buffer_);
	else fprintf(stderr,"%s: %s\n", tag, Buffer_);
}

#endif

}}
