#include "Updatable.h"
PVector<Updatable> updatableList;
Updatable::Updatable()
{
    updatableList.push_back(this);
}

Updatable::~Updatable()
{
    //dtor
}
