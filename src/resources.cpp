#include "resources.h"

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#endif
#include <cstdio>
#include <filesystem>
#include <SDL.h>

#ifdef ANDROID
#include <SDL.h>
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
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

string ResourceStream::readAll()
{
    string result;
    result.resize(getSize());
    read(result.data(), result.size());
    return result;
}

class FileResourceStream : public ResourceStream
{
    SDL_RWops *io;
    size_t size = 0;
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
            io = nullptr;
        }
        else
            io = SDL_RWFromFile(filename.c_str(), "rb");
#else
       //Android reads from the assets bundle, so we cannot check if the file exists and is a regular file
       io = SDL_RWFromFile(filename.c_str(), "rb");
#endif
    }

    virtual ~FileResourceStream()
    {
        if (io)
            io->close(io);
    }
    
    bool isOpen()
    {
        return io != nullptr;
    }
    
    virtual size_t read(void* data, size_t size) override
    {
        return io->read(io, data, 1, size);
    }
    virtual size_t seek(size_t position) override
    {
        auto offset = io->seek(io, position, RW_SEEK_SET);
        SDL_assert(offset != -1);
        return static_cast<size_t>(offset);
    }
    virtual size_t tell() override
    {
        auto offset = io->seek(io, 0, RW_SEEK_CUR);
        SDL_assert(offset != -1);
        return static_cast<size_t>(offset);
    }
    virtual size_t getSize() override
    {
        if (size == 0) {
            size_t cur = tell();
            auto end_offset = io->seek(io, 0, RW_SEEK_END);
            SDL_assert(end_offset != -1);
            size = static_cast<size_t>(end_offset);
            seek(cur);
        }
        return size;
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
    static jobject asset_manager_jobject;
    static AAssetManager* asset_manager = nullptr;
    if (!asset_manager)
    {
        JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
        jobject activity = (jobject)SDL_AndroidGetActivity();
        jclass clazz(env->GetObjectClass(activity));
        jmethodID method_id = env->GetMethodID(clazz, "getAssets", "()Landroid/content/res/AssetManager;");
        asset_manager_jobject = env->CallObjectMethod(activity, method_id);
        asset_manager = AAssetManager_fromJava(env, asset_manager_jobject);

        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(clazz);
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
