#ifndef RESOURCELOADER_H
#define RESOURCELOADER_H

#include "stringImproved.h"
#include "P.h"


class ResourceStream : public virtual PObject
{
public:
    virtual ~ResourceStream() {}
    
    virtual size_t read(void* ptr, size_t amount) = 0;
    virtual size_t seek(size_t offset) = 0;
    virtual size_t tell() = 0;
    virtual size_t getSize() = 0;

    string readLine();
    string readAll();
};

class ResourceProvider : public virtual PObject
{
protected:
    bool searchMatch(const string name, const string searchPattern);
public:
    ResourceProvider();
    
    virtual P<ResourceStream> getResourceStream(const string filename) = 0;
    virtual std::vector<string> findResources(const string searchPattern) = 0;
};

class DirectoryResourceProvider : public ResourceProvider
{
    string basepath;
public:
    DirectoryResourceProvider(const string basepath);
    
    virtual P<ResourceStream> getResourceStream(const string filename) override;
    virtual std::vector<string> findResources(const string searchPattern) override;
private:
    void findResources(std::vector<string>& paths, const string basepath, const string searchPattern);
};

P<ResourceStream> getResourceStream(const string filename);
std::vector<string> findResources(const string searchPattern);

#endif//RESOURCELOADER_H
