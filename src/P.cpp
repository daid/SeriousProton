#include "P.h"

    PObject::PObject()
    {
#ifdef DEBUG
        int stackTest = 0;
        std::ptrdiff_t diff = &stackTest - (int*)this;
        //Check if this object is created on the stack, PObjects should not be created on the stack, as they manage
        // their own destruction.
        SDL_assert(abs(diff) > 10000);//"Object on stack! Not allowed!"
        DEBUG_PobjCount ++;
        
        DEBUG_PobjListNext = DEBUG_PobjListStart;
        DEBUG_PobjListStart = this;
#endif
        refCount = 0;
        _destroyed_flag = false;
    }

    PObject::~PObject()
    {
#ifdef DEBUG
        DEBUG_PobjCount --;
        if (DEBUG_PobjListStart == this)
        {
            DEBUG_PobjListStart = DEBUG_PobjListStart->DEBUG_PobjListNext;
        }else{
            for(PObject* obj = DEBUG_PobjListStart; obj; obj = obj->DEBUG_PobjListNext)
            {
                if (obj->DEBUG_PobjListNext == this)
                {
                    obj->DEBUG_PobjListNext = obj->DEBUG_PobjListNext->DEBUG_PobjListNext;
                    break;
                }
            }
        }
#endif
    }
