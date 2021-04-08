#ifndef PEE_POINTER_H
#define PEE_POINTER_H

#include <SFML/System.hpp>
#include <vector>
#include <assert.h>

#include "logging.h"

/**
    P<T> is a reference counting pointer class. This class keeps track to the amount of P<T> pointers pointing to a Pobject.
    If no more P<T> pointers point to a Pobject, the object is deleted. The object also has a destroyed flag, if the destroyed
    flag is set, the pointers pointing towards it act as NULL pointers from that point on.
    NOTE: This does not free circular references.

    The Pobject is not copyable and should not be created on the stack, only on the heap (so with "new")

    The Pvector class is a specialized version of the std::vector template. This vector holds an array of P<> pointers
    The "foreach" macro can be used to walk trough all the Pobjects in a list without needing to dive into details
    and automaticly removes any pointer from the list that points to an Pobject which has been destroyed.
 */
#ifdef DEBUG
class PObject;
extern int DEBUG_PobjCount;
extern PObject* DEBUG_PobjListStart;
#endif

class PObject: public sf::NonCopyable
{
private:
    int refCount;
    bool _destroyed_flag;

    //Make the P template a friend so it can access the private refCount and destroyed.
    template<typename> friend class P;
public:
#ifdef DEBUG
    PObject* DEBUG_PobjListNext;
#endif
    PObject();

    virtual ~PObject();

    virtual void destroy()
    {
        _destroyed_flag = true;
    }
    
    int getRefCount() const
    { 
        return refCount;
    }
    bool isDestroyed() const
    {
        return _destroyed_flag;
    }
};

template<class T>
class P
{
private:
    T* ptr;

public:
    P()
    {
        ptr = NULL;
    }
    P(const P& p)
    {
        ptr = NULL;
        set(p.ptr);
    }
    P(T* p)
    {
        ptr = NULL;
        set(p);
    }

    ~P()
    {
        release();
    }

    P& operator = (T* p)
    {
        set(p);
        return *this;
    }

    P& operator = (const P& p)
    {
        if (&p != this) set(p.ptr);
        return *this;
    }

    T* operator->() const
    {
#ifdef DEBUG
        if(!ptr || ptr->_destroyed_flag)
        {
            LOG(ERROR) << "Oh noes! Better find me and put a breakpoint here!";
        }
        assert(ptr);
        assert(!ptr->_destroyed_flag);
#endif
        return ptr;
    }
    T* operator*()
    {
        check_release();
        return ptr;
    }
    
    T* operator*() const
    {
        return ptr;
    }

    explicit operator bool()
    {
        check_release();
        return ptr != NULL;
    }

    explicit operator bool() const
    {
        return ptr != nullptr && !ptr->isDestroyed();
    }

    template<class T2> operator P<T2>()
    {
        return dynamic_cast<T2*>(**this);
    }

    template<class T2> operator P<T2>() const
    {
        return dynamic_cast<T2*>(**this);
    }

protected:
    void check_release()
    {
        if (ptr != NULL && ptr->_destroyed_flag)
            release();
    }

    void release()
    {
        if (ptr)
        {
            ptr->refCount--;
            if (ptr->refCount == 0)
                delete ptr;
            ptr = NULL;
        }
    }
    void set(T* p)
    {
        release();
        ptr = p;
        if (ptr != NULL)
            ptr->refCount ++;
    }
};

template<class T>
class PVector: public std::vector<P<T> > {
public:
    bool has(const P<T>& obj)
    {
        for(unsigned int n=0; n<std::vector<P<T> >::size(); n++)
            if ((*this)[n] == obj)
                return true;
        return false;
    }

    void remove(const P<T>& obj)
    {
        for(unsigned int n=0; n<std::vector<P<T> >::size(); n++)
        {
            if ((*this)[n] == obj)
            {
                std::vector<P<T> >::erase(std::vector<P<T> >::begin() + n);
                n--;
            }
        }
    }
    
    void update()
    {
        for(unsigned int n=0; n<std::vector<P<T> >::size(); n++)
        {
            if (!(*this)[n])
            {
                std::vector<P<T> >::erase(std::vector<P<T> >::begin() + n);
                n--;
            }
        }
    }
};

template<class T>
class Piterator : public P<T>
{
private:
    PVector<T>& list;
    unsigned int index;
public:
    Piterator(PVector<T>& list)
    : P<T>(NULL), list(list), index(0)
    {
       next();
    }

    void next()
    {
        while(true)
        {
            if (index >= list.size())
            {
                P<T>::set(NULL);
                return;
            }
            P<T>::set(*list[index]);
            if (*this)
            {
                index++;
                return;
            }
            list.erase(list.begin() + index);
        }
    }
};
#define foreach(type, var, list) for(Piterator<type> var(list); var; var.next())

template<class T> bool operator == (P<T>& p, const PObject* ptr)
{
    return *p == ptr;
}
template<class T> bool operator != (P<T>& p, const PObject* ptr)
{
    return *p != ptr;
}
template<class T1, class T2> bool operator == (P<T1>& p1, P<T2>& p2)
{
    return *p1 == *p2;
}
template<class T1, class T2> bool operator != (P<T1>& p1, P<T2>& p2)
{
    return *p1 != *p2;
}
template<class T1, class T2> bool operator == (const P<T1>& p1, const P<T2>& p2)
{
    return *p1 == *p2;
}
template<class T1, class T2> bool operator != (const P<T1>& p1, const P<T2>& p2)
{
    return *p1 != *p2;
}

namespace std
{
    //Make a specialization of std::hash 
    template <class T> struct hash<P<T>>
    {
        size_t operator()(const P<T>& k) const noexcept
        {
            return hash<void*>{}(*k);
        }
    };
}

#endif//PEE_POINTER_H
