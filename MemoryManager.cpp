#include "MemoryManager.h"

Node::Node(int index, int length, bool isFree)
{
    this->index = index;
    this->length = length;
    this->isFree = isFree;
}
MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator)
{
    // wordSize is how many words
    // sizeInWords is how many bytes in a word
    this->algorithm = allocator;
    this->wordSize = wordSize;
}
MemoryManager::~MemoryManager()
{
    shutdown();
}
void MemoryManager::initialize(size_t sizeInWords)
{

    if (sizeInWords <= 65536)
    {
        // sizeInWords = 65536;
        byteLimit = wordSize * sizeInWords;
        memoryBlock = std::vector<uint64_t>(byteLimit);

        memStart = (uint64_t *)memoryBlock.data();

        indextoNode[0] = 0;
        nodes.push_back(Node(0, sizeInWords, true));
    }
}
void MemoryManager::shutdown()
{
    memoryBlock.clear();
    nodes.clear();
    indextoNode.clear();
}
void *MemoryManager::allocate(size_t sizeInBytes)
{
    // find ceiling
    size_t sizeInWords = ceil(sizeInBytes / wordSize * 1.0);

    // make sure to delete
    uint16_t *myList = static_cast<uint16_t *>(getList());
    uint16_t *listEntryPoint = myList;

    int openIndex = algorithm(sizeInWords, listEntryPoint);

    delete[] listEntryPoint;

    // failed to find hole
    if (openIndex == -1)
    {
        std::cout << "Failed";
        return nullptr;
    }

    if (indextoNode.count(openIndex) == 0)
    {
        return nullptr;
    }

    int nodeIndex = indextoNode[openIndex];

    // if there is extra space, allocate and shrink
    if (nodes[nodeIndex].length != sizeInWords)
    {
        // creates used node
        Node used = Node(nodes[nodeIndex].index, sizeInWords, false);
        nodes.insert(nodes.begin() + nodeIndex, used);
        // adjust pre-existing node
        nodes[nodeIndex + 1].index += sizeInWords;
        nodes[nodeIndex + 1].length -= sizeInWords;
        // map
        indextoNode[nodes[nodeIndex].index] = nodeIndex;
        indextoNode[nodes[nodeIndex + 1].index] = nodeIndex + 1;

        for (auto it = indextoNode.find(nodes[nodeIndex + 1].index); it != indextoNode.end(); it++)
        {
            if (it == indextoNode.find(nodes[nodeIndex + 1].index))
            {
                // skip the first one!
                continue;
            }
            else
            {
                it->second++;
            }
        }
    }
    else
    {
        nodes[nodeIndex].isFree = false;
    }

    return (void *)(memoryBlock.data() + openIndex);
}
void MemoryManager::free(void *address)
{
    // finds node and turns it free
    int byteIndex = (uint64_t *)address - memoryBlock.data();

    if (indextoNode.count(byteIndex) == 0)
    {
        return;
    }

    int nodeIndex = indextoNode[byteIndex];
    nodes[nodeIndex].isFree = true;

    // if prev consecutive free
    if (nodeIndex > 0 && nodes[nodeIndex - 1].isFree)
    {
        // updates map
        indextoNode.erase(nodes[nodeIndex].index);
        // combines with previous and removes current
        nodes[nodeIndex - 1].length += nodes[nodeIndex].length;
        nodes.erase(nodes.begin() + nodeIndex);

        // decrement in case next free too
        nodeIndex--;
    }

    // if next consecutive free
    if (nodes.size() > nodeIndex && nodes[nodeIndex + 1].isFree)
    {
        // updates map
        for (auto it = indextoNode.find(nodes[nodeIndex + 1].index); it != indextoNode.end(); it++)
        {
            it->second--;
        }

        indextoNode.erase(nodes[nodeIndex + 1].index);
        // combines with next and removes next
        nodes[nodeIndex].length += nodes[nodeIndex + 1].length;
        nodes.erase(nodes.begin() + nodeIndex + 1);
    }
}
void MemoryManager::setAllocator(std::function<int(int, void *)> allocator)
{
    this->algorithm = allocator;
}
int MemoryManager::dumpMemoryMap(char *filename)
{
    // third value is permissions
    int file = open(filename, O_CREAT | O_WRONLY, 0666);
    if (file < 0)
    {
        return -1;
    }

    std::string output = "";
    uint16_t *holeList = static_cast<uint16_t *>(getList());
    uint16_t *listEntryPoint = holeList;

    uint16_t holeListlength = *holeList++;

    for (uint16_t i = 0; i < (holeListlength)*2; i += 2)
    {
        output += "[" + std::to_string(holeList[i]) + ", " + std::to_string(holeList[i + 1]) + "] - ";
    }

    output = output.substr(0, output.size() - 3);

    const char *c = output.c_str();
    delete[] listEntryPoint;

    if (write(file, c, strlen(c)) == -1)
    {
        return -1;
    }

    if (close(file) == -1)
    {
        return -1;
    }

    return 0;
}
void *MemoryManager::getList()
{
    if (nodes.size() == 0)
    {
        return nullptr;
    }

    std::vector<int> holes;
    int numholes = 0;
    for (unsigned int i = 0; i < nodes.size(); i++)
    {
        if (nodes[i].isFree)
        {
            numholes++;
            holes.push_back(nodes[i].index);
            holes.push_back(nodes[i].length);
        }
    }
    holes.insert(holes.begin(), numholes);

    uint16_t *list = new uint16_t[1 + (numholes * 2)];
    for (unsigned int i = 0; i < holes.size(); i++)
    {
        list[i] = (uint16_t)holes[i];
    }

    return list;
}
void *MemoryManager::getBitmap()
{
    std::string binary = "";
    std::vector<std::string> binaries;

    for (unsigned int i = 0; i < nodes.size(); i++)
    {
        int length = nodes[i].length;
        while (length > 0)
        {
            if (nodes[i].isFree)
            {
                binary = "0" + binary;
            }
            else
            {
                binary = "1" + binary;
            }
            length--;
        }
    }

    // find ceiling
    int sizeMap = (binary.size() + 8 - 1) / 8;

    while (binary.size() > 0)
    {
        if (binary.size() > 8)
        {
            binaries.push_back(binary.substr(binary.size() - 8));
            binary = binary.substr(0, binary.size() - 8);
        }
        else
        {
            binaries.push_back(binary);
            binary.clear();
        }
    }

    // MAKE SURE UINT8
    std::string sizebinary = std::bitset<8>(sizeMap).to_string();
    // std::reverse(sizebinary.begin(), sizebinary.end());

    binaries.insert(binaries.begin(), sizebinary.substr(4));
    binaries.insert(binaries.begin() + 1, sizebinary.substr(0, 4));

    binaries.size();

    uint8_t *list = new uint8_t[binaries.size()];

    for (unsigned int i = 0; i < binaries.size(); i++)
    {
        list[i] = (uint8_t)std::bitset<8>(binaries[i]).to_ullong();
        // std::cout << list[i] << " ";
    }

    // list[0] = sizeMap & 0xff;
    // list[1] = (sizeMap >> 8) & 0xff;

    return list;
}
unsigned MemoryManager::getWordSize()
{
    return wordSize;
}
void *MemoryManager::getMemoryStart()
{
    return memStart;
}
unsigned MemoryManager::getMemoryLimit()
{
    return byteLimit;
}

int bestFit(int sizeInWords, void *list)
{
    int index = -1;
    int min = 65536;

    uint16_t *holeList = static_cast<uint16_t *>(list);
    uint16_t holeListlength = *holeList++;

    for (uint16_t i = 1; i < (holeListlength)*2; i += 2)
    {
        if (holeList[i] >= sizeInWords && holeList[i] < min)
        {
            min = holeList[i];
            index = holeList[i - 1];
        }
    }

    if (index == -1)
    {
        return -1;
    }
    return index;
}
int worstFit(int sizeInWords, void *list)
{
    int index = -1;
    int max = -1;

    uint16_t *holeList = static_cast<uint16_t *>(list);
    uint16_t holeListlength = *holeList++;

    for (uint16_t i = 1; i < (holeListlength)*2; i += 2)
    {
        if (holeList[i] >= sizeInWords && holeList[i] > max)
        {
            max = holeList[i];
            index = holeList[i - 1];
        }
    }

    if (index == -1)
    {
        return -1;
    }
    return index;
}
