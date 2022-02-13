#include <io/keyValueTreeLoader.h>
#include <logging.h>

namespace sp {
namespace io {

KeyValueTreePtr KeyValueTreeLoader::load(const string& resource_name)
{
    KeyValueTreeLoader loader(resource_name);
    return loader.result;
}

KeyValueTreeLoader::KeyValueTreeLoader(const string& resource_name)
{
    stream = getResourceStream(resource_name);
    if (!stream)
    {
        LOG(Error, "Failed to open " + resource_name + " for tree loading");
        return;
    }
    result = std::make_shared<KeyValueTree>();

    LOG(Info, "Loading tree", resource_name);

    while(stream->tell() < stream->getSize())
    {
        string line = stream->readLine().strip();
        if (line.startswith("//"))
            continue;
        int comment_start = line.find(" //");
        if (comment_start >= 0)
            line = line.substr(0, comment_start);
        if (line == "{")
        {
            //New anomounous node.
            result->root_nodes.emplace_back();
            parseNode(&result->root_nodes.back());
        }
        else if (line.startswith("[") && line.find("]") > -1 && line.endswith("{"))
        {
            //New named node.
            result->root_nodes.emplace_back(line.substr(1, line.find("]")));
            parseNode(&result->root_nodes.back());
        }
        else if (line == "}")
        {
            LOG(Error, "Failed to parse key value tree: Node close while no node open.", resource_name);
            result = nullptr;
            return;
        }
        else if (line.startswith("#"))
        {
            //Comment line.
        }
        else if (line.length() > 0)
        {
            LOG(Error, "Failed to parse line:", line);
            result->root_nodes.clear();
            return;
        }
    }
}

void KeyValueTreeLoader::parseNode(KeyValueTreeNode* node)
{
    while(stream->tell() < stream->getSize())
    {
        string line = stream->readLine().strip();
        if (line.startswith("//"))
            continue;
        int comment_start = line.find(" //");
        if (comment_start >= 0)
            line = line.substr(0, comment_start);
        if (line == "{")
        {
            //New anonymous node.
            node->child_nodes.emplace_back();
            parseNode(&node->child_nodes.back());
        }
        else if (line.startswith("[") && line.find("]") > -1 && line.endswith("{"))
        {
            //New named node.
            node->child_nodes.emplace_back();
            node->child_nodes.back().id = line.substr(1, line.find("]"));
            parseNode(&node->child_nodes.back());
        }
        else if (line == "}")
        {
            return;
        }
        else if (line.startswith("#"))
        {
            //Comment line.
        }
        else if (line.find(":") > 0)
        {
            while(line.endswith("\\"))
                line = line.substr(0, -1) + "\n" + stream->readLine().strip();
            string key = line.substr(0, line.find(":")).strip();
            string value = line.substr(line.find(":") + 1).strip();
            node->items[key] = value;
        }
        else if (line.length() > 0)
        {
            LOG(Error, "Failed to parse line in key value tree: " + line);
        }
    }
}

}//namespace io
}//namespace sp
