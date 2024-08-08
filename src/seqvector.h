#ifndef SEQVECTOR_H
#define SEQVECTOR_H

#include <vector>

namespace sp {

// A vector that tracks sequenced elements
template<typename T> class SeqVector : public std::vector<T>{
public:
    unsigned int last_seq = 0;
};

}//namespace sp

#endif//SEQVECTOR_H
