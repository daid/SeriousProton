#ifndef _MULTIPLAYER_HPP_
#define _MULTIPLAYER_HPP_

template <> bool multiplayerReplicationFunctions<string>::isChanged(void* data, void* prev_data_ptr)
{
    string* ptr = (string*)data;
    int64_t* hash_ptr = (int64_t*)prev_data_ptr;

    int64_t hash = 5381;
    hash = ((hash << 5) + hash) + ptr->length();
    for(unsigned int n=0; n<ptr->length(); n++)
        hash = (hash * 33) + (*ptr)[n];
    if (*hash_ptr != hash)
    {
        *hash_ptr = hash;
        return true;
    }
    return false;
}
#endif /* _MULTIPLAYER_HPP_ */
