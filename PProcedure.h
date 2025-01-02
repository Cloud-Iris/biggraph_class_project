#include <set>
#include <tuple>
#include <algorithm>
#include "Node.h"
typedef unsigned TYPE_ENTITY_LITERAL_ID;
typedef int TYPE_PREDICATE_ID;
typedef unsigned TYPE_PROPERTY_ID;

void ic1(const std::vector<GPStore::Value> &args, std::vector<std::vector<GPStore::Value>> &result);
void ic2(const std::vector<GPStore::Value> &args, std::vector<std::vector<GPStore::Value>> &result);
void ic7(const std::vector<GPStore::Value> &args, std::vector<std::vector<GPStore::Value>> &result);
void is1(const std::vector<GPStore::Value> &args, std::vector<std::vector<GPStore::Value>> &result);

// 将自定义的函数抽到 PProcedure.h 中
// 根据节点类型和数据实体 id 获取 Node 对象
Node GetNode(std::string node_type, const std::string &data_id);
Node GetNodeByGlobalID(std::string node_type, const std::string &global_id);