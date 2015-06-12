#ifndef UPDATABLE_H
#define UPDATABLE_H

#include "P.h"

class Updatable;
extern PVector<Updatable> updatableList;
//Abstract class for entity that can be updated.
class Updatable: public virtual PObject
{
    public:
        Updatable();
        virtual ~Updatable();
        virtual void update(float delta) = 0;
    protected:
    private:
};

#endif // UPDATABLE_H
