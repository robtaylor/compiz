#ifndef _WRAPABLE_H_
#define _WRAPABLE_H_

#include <vector>

#define WRAPABLE_DEF(rtype, func, ...) public: virtual rtype func (__VA_ARGS__); bool func ## _enabled; public:

#define WRAPABLE_DEF_FUNC(func, ...) \
{ \
    if (mHandler) \
    { \
	func ## _enabled = false; \
	mHandler-> func (__VA_ARGS__); \
    } \
}

#define WRAPABLE_DEF_FUNC_RETURN(func, ...) \
{ \
    if (mHandler) \
    { \
	func ## _enabled = false; \
	return mHandler-> func (__VA_ARGS__); \
    } \
    return 0; \
}

#define WRAPABLE_HND(rtype, func, ...) public: rtype func (__VA_ARGS__); private: unsigned int mCurr_ ## func ; public:

#define WRAPABLE_HND_FUNC(func, ...) \
{ \
    unsigned int curr = mCurr_ ## func; \
    while (mCurr_ ## func < ifce.size() && \
           !ifce[mCurr_ ## func]->func ## _enabled) \
        mCurr_ ## func ++; \
    if (mCurr_ ## func < ifce.size()) \
    { \
	ifce[mCurr_ ## func++]-> func (__VA_ARGS__); \
	mCurr_ ## func = curr; \
	return; \
    } \
    mCurr_ ## func = curr; \
}

#define WRAPABLE_HND_FUNC_RETURN(rtype, func, ...) \
{ \
    int   curr = mCurr_ ## func; \
    rtype rv; \
    while (mCurr_ ## func < ifce.size() && \
           !ifce[mCurr_ ## func]->func ## _enabled) \
        mCurr_ ## func ++; \
    if (mCurr_ ## func < ifce.size()) \
    { \
	rv = ifce[mCurr_ ## func++]-> func (__VA_ARGS__); \
	mCurr_ ## func = curr; \
	return rv; \
    } \
    mCurr_ ## func = curr; \
}

#define WRAPABLE_INIT_FUNC(func) \
    func ## _enabled = true

#define WRAPABLE_INIT_HND(func) \
    mCurr_ ## func = 0

template <typename T>
class WrapableInterface {
    protected:
	WrapableInterface () : mHandler(0) {};
	virtual ~WrapableInterface () {};

	void setHandler (T *handler)
	{
	    mHandler = handler;
	}
        T *mHandler;
};

template <typename T>
class WrapableHandler : public T {
    public:
	void add (T *);
	void remove (T *);
	
    protected:
	
        std::vector<T *> ifce;
};

template <typename T>
void WrapableHandler<T>::add (T *obj)
{
    ifce.insert (ifce.begin(), obj);
};

template <typename T>
void WrapableHandler<T>::remove (T *obj)
{
    typename std::vector<T *>::iterator it;
    for (it = ifce.begin(); it != ifce.end(); it++)
	if ((*it) == obj)
	{
	    ifce.erase (it);
	    break;
	}
}

#endif
