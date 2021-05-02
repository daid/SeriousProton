#include "resources.h"

#include <SFML/System.hpp>
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#endif
#include <cstdio>
#include <filesystem>

#if defined(ANDROID)
#include <SFML/System/NativeActivity.hpp>
#include <android/native_activity.h>
#include <android/asset_manager.h>
#endif

PVector<ResourceProvider> resourceProviders;

ResourceProvider::ResourceProvider()
{
    resourceProviders.push_back(this);
}

bool ResourceProvider::searchMatch(const string name, const string searchPattern)
{
    std::vector<string> parts = searchPattern.split("*");
    int pos = 0;
    if (parts[0].length() > 0)
    {
        if (name.find(parts[0]) != 0)
            return false;
    }
    for(unsigned int n=1; n<parts.size(); n++)
    {
        int offset = name.find(parts[n], pos);
        if (offset < 0)
            return false;
        pos = offset + static_cast<int>(parts[n].length());
    }
    return pos == static_cast<int>(name.length());
}

string ResourceStream::readLine()
{
    string ret;
    char c;
    while(true)
    {
        if (read(&c, 1) < 1)
            return ret;
        if (c == '\n')
            return ret;
        ret += string(c);
    }
}

class FileResourceStream : public ResourceStream
{
    sf::FileInputStream stream;
    bool open_success;
public:
    FileResourceStream(string filename)
    {
#ifndef ANDROID
        std::error_code ec;
        if(!std::filesystem::is_regular_file(filename.c_str(), ec))
        {
            //Error code "no such file or directory" thrown really often, so no trace here
            //not to spam the log
            open_success = false;
        }
        else
            open_success = stream.open(filename);
#else
       //Android reads from the assets bundle, so we cannot check if the file exists and is a regular file
       open_success = stream.open(filename);
#endif
    }
    virtual ~FileResourceStream()
    {
    }
    
    bool isOpen()
    {
        return open_success;
    }
    
    virtual sf::Int64 read(void* data, sf::Int64 size)
    {
        return stream.read(data, size);
    }
    virtual sf::Int64 seek(sf::Int64 position)
    {
        return stream.seek(position);
    }
    virtual sf::Int64 tell()
    {
        return stream.tell();
    }
    virtual sf::Int64 getSize()
    {
        return stream.getSize();
    }
};


DirectoryResourceProvider::DirectoryResourceProvider(string basepath)
{
    this->basepath = basepath.rstrip("\\/");
}

P<ResourceStream> DirectoryResourceProvider::getResourceStream(string filename)
{
    P<FileResourceStream> stream = new FileResourceStream(basepath + "/" + filename);
    if (stream->isOpen())
        return stream;
    return nullptr;
}

std::vector<string> DirectoryResourceProvider::findResources(string searchPattern)
{
    std::vector<string> found_files;
#if defined(ANDROID)
    //Limitation : 
    //As far as I know, Android NDK won't provide a way to list subdirectories
    //So we will only list files in the first level directory 
    ANativeActivity *nactivity {sf::getNativeActivity()};
   
    AAssetManager *asset_manager{nullptr};
    if(nactivity)
    {
        asset_manager = nactivity->assetManager;
    }
    if(asset_manager)
    {
        int idx = searchPattern.rfind("/");
        string forced_path = basepath;
        string prefix = "";
        if (idx > -1)
        {
            prefix = searchPattern.substr(0, idx);
            forced_path += "/" + prefix;
            prefix += "/";
        }
        AAssetDir* dir = AAssetManager_openDir(asset_manager, (forced_path).c_str());
        if (dir)
        {
            const char* filename;
            while ((filename = AAssetDir_getNextFileName(dir)) != nullptr)
            {
                if (searchMatch(prefix + filename, searchPattern))
                    found_files.push_back(prefix + filename);
            }
            AAssetDir_close(dir);
        }
    }
#else
    findResources(found_files, "", searchPattern);
#endif
    return found_files;
}

void DirectoryResourceProvider::findResources(std::vector<string>& found_files, const string path, const string searchPattern)
{
#ifdef _MSC_VER
    WIN32_FIND_DATAA data;
    string search_root(basepath + "/" + path);
    if (!search_root.endswith("/"))
    {
        search_root += "/";
    }
    HANDLE handle = FindFirstFileA((search_root + "*").c_str(), &data);
    if (handle == INVALID_HANDLE_VALUE)
        return;
    do
    {
        if (data.cFileName[0] == '.')
            continue;
        string name = path + string(data.cFileName);
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            findResources(found_files, name + "/", searchPattern);
        }
        else
        {
            if (searchMatch(name, searchPattern))
                found_files.push_back(name);
        }
    } while (FindNextFileA(handle, &data));

    FindClose(handle);
#else
    DIR* dir = opendir((basepath + "/" + path).c_str());
    if (!dir)
        return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (entry->d_name[0] == '.')
            continue;
        string name = path + string(entry->d_name);
        if (searchMatch(name, searchPattern))
            found_files.push_back(name);
        findResources(found_files, path + string(entry->d_name) + "/", searchPattern);
    }
    closedir(dir);
#endif
}

P<ResourceStream> getResourceStream(string filename)
{
    foreach(ResourceProvider, rp, resourceProviders)
    {
        P<ResourceStream> stream = rp->getResourceStream(filename);
        if (stream)
            return stream;
    }
    return NULL;
}

std::vector<string> findResources(string searchPattern)
{
    std::vector<string> foundFiles;
    foreach(ResourceProvider, rp, resourceProviders)
    {
        std::vector<string> res = rp->findResources(searchPattern);
        foundFiles.insert(foundFiles.end(), res.begin(), res.end());
    }
    return foundFiles;
}
