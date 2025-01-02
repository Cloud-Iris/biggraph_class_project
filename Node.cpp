
#include <iostream>
#include "Node.h"

Node::Node(const std::string& label_string, const std::string& prop_string, const GPStore::Value* value) {
    this->label_string = label_string;

}

Node::Node(unsigned long long node_id) {
    this->node_id_ = node_id;
}

GPStore::Value* Node::operator[](const std::string& property_string) {

}

// 根据关系名在 节点的 relations 中进行查找，返回从节点出发，用 pre_str 关系 指向的节点的全局 id 列表
// 边的方向 edge_dir 暂时不用
void Node::GetLinkedNodes(const std::string& pre_str, std::shared_ptr<const std::string[]>& nodes_list, unsigned& list_len, char edge_dir) {
    // 初始化节点列表长度为0
    list_len = 0;

    // 检查关系是否存在
    auto it = this->relations.find(pre_str);
    if (it == this->relations.end()) {
        return; // 如果关系不存在，直接返回
    }

    // 获取关系列表
    const auto& relation_list = it->second;

    // 计算节点列表长度
    list_len = relation_list.size();

    // 分配内存并填充节点列表
    std::shared_ptr<std::string[]> temp_list(new std::string[list_len]);
    for (unsigned i = 0; i < list_len; ++i) {
        temp_list[i] = relation_list[i].first; // 节点的全局 id
    }

    // 将临时列表赋值给输出参数
    nodes_list = temp_list;
}

void Node::GetLinkedNodesWithEdgeProps(const std::string& pre_str, std::shared_ptr<const unsigned[]>& nodes_list, std::shared_ptr<const long long[]>& prop_list,
                                       unsigned& prop_len, unsigned& list_len, char edge_dir) {

}

void Node::setLabel(const std::string &label_string) {
    this->label_string = label_string;
}

void Node::setValues(const std::string &prop_string, const GPStore::Value *value) {
    if (value != nullptr) {
        this->columns[prop_string] = *value;
        delete value; // 删除传入的指针
    }
}

void Node::print() {

    std::cout << "\n\nNode_id: " << this->node_id_ << "\n";
    std::cout << "label_string: " << this->label_string << "\n";
    std::cout << "columns_size: " << this->columns.size() << "\n";
    for(auto& item:this->columns)
        std::cout << item.first << ": " << item.second.toString() << "\n";
    std::cout << "relations_size: " << this->relations.size() << "\n";
    for(auto& item:this->relations)
    {
        std::cout << "----------------\n";
        std::cout << item.first << "\n";
        std::vector<std::pair<std::string, std::string>> relation_vec = item.second;
        if (relation_vec.size() <= 20) {
            for(auto& relation: relation_vec)
                std::cout << relation.first << " " << relation.second << "\n";
        } else {
            for(size_t i = 0; i < 20; ++i)
                std::cout << relation_vec[i].first << " " << relation_vec[i].second << "\n";
            
            std::cout << "... 共有 " << relation_vec.size() << " 个" << item.first << " 关系\n";
        }
    }
    std::cout << "relationsProp_size: " << this->relationsProp.size() << "\n";
    for(auto& item:this->relationsProp)
        std::cout << item.first << " " << item.second << "\n";
    std::cout<<"\n\n";
}

void Node::addRelation(std::string relationName, std::string index, std::string prop, std::string propValue) {
    this->relations[relationName].push_back(std::pair<std::string, std::string>(index,propValue));
    this->relationsProp[relationName] = prop;
}


// Different node lists, formatted as id2Node
// { Node 类中的全局 id : 构造好的 Node 对象 }
std::unordered_map<std::string, Node> PersonMap;
std::unordered_map<std::string, Node> OrganisationMap;
std::unordered_map<std::string, Node> PlaceMap;
std::unordered_map<std::string, Node> PostMap;
std::unordered_map<std::string, Node> CommentMap;
std::unordered_map<std::string, Node> TagMap;
std::unordered_map<std::string, Node> TagClassMap;
std::unordered_map<std::string, Node> ForumMap;

// Entity ID to global ID mapping for nodes
// { 数据中的 ID : Node 类中的全局 id }
std::unordered_map<std::string, std::string> PersonIDMap;
std::unordered_map<std::string, std::string> OrganisationIDMap;
std::unordered_map<std::string, std::string> PlaceIDMap;
std::unordered_map<std::string, std::string> PostIDMap;
std::unordered_map<std::string, std::string> CommentIDMap;
std::unordered_map<std::string, std::string> TagIDMap;
std::unordered_map<std::string, std::string> TagClassIDMap;
std::unordered_map<std::string, std::string> ForumIDMap;

// 定义节点类型到节点映射的映射
std::unordered_map<std::string, std::unordered_map<std::string, Node>*> type2Map={
    {"Person", &PersonMap},
    {"Comment", &CommentMap},
    {"Post", &PostMap},
    {"Forum", &ForumMap},

    {"Organisation", &OrganisationMap},
    {"Place", &PlaceMap},
    {"Tag", &TagMap},
    {"TagClass", &TagClassMap},
};

// 定义节点类型到ID映射的映射
std::unordered_map<std::string, std::unordered_map<std::string, std::string>*> type2IDMap={
    {"Person", &PersonIDMap},
    {"Comment", &CommentIDMap},
    {"Post", &PostIDMap},
    {"Forum", &ForumIDMap},

    {"Organisation", &OrganisationIDMap},
    {"Place", &PlaceIDMap},
    {"Tag", &TagIDMap},
    {"TagClass", &TagClassIDMap},
};