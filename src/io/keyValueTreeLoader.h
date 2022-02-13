#ifndef SP_IO_KEYVALUETREE_LOADER_H
#define SP_IO_KEYVALUETREE_LOADER_H

#include <keyValueTree.h>
#include <resources.h>


namespace sp {
namespace io {

class KeyValueTreeLoader
{
public:
    static KeyValueTreePtr load(const string& resource_name);

private:
    KeyValueTreePtr result;
    P<ResourceStream> stream;
    
    KeyValueTreeLoader(const string& resource_name);
    void parseNode(KeyValueTreeNode* node);
};

}//namespace io
}//namespace sp

#endif//SP2_IO_KEYVALUETREE_LOADER_H
