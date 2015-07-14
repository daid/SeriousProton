#ifndef RESOURCELOADER_H
#define RESOURCELOADER_H

#include "stringImproved.h"
#include "P.h"

class ResourceStream : public virtual PObject, public sf::InputStream
{
public:
    virtual ~ResourceStream() {}
    
    string readLine();
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
    
    virtual P<ResourceStream> getResourceStream(const string filename);
    virtual std::vector<string> findResources(const string searchPattern);
private:
    void findResources(std::vector<string>& paths, const string basepath, const string searchPattern);
};

P<ResourceStream> getResourceStream(const string filename);
std::vector<string> findResources(const string searchPattern);

#endif//RESOURCELOADER_H
