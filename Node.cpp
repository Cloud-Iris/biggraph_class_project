
#include <iostream>
#include "Node.h"

Node::Node(const std::string& label_string, const std::string& prop_string, const GPStore::Value* value) {
    this->label_string = label_string;

}

Node::Node(unsigned node_id) {
    this->node_id_ = node_id;
}

GPStore::Value* Node::operator[](const std::string& property_string) {

}

void Node::GetLinkedNodes(const std::string& pre_str, std::shared_ptr<const unsigned[]>& nodes_list, unsigned& list_len, char edge_dir) {

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
    std::cout << "Node_id: " << this->node_id_ << "\n";
    std::cout << "label_string: " << this->label_string << "\n";
    std::cout << "columns_size: " << this->columns.size() << "\n";
    for(auto& item:this->columns)
        std::cout << item.first << ": " << item.second.toString() << "\n";
    std::cout << "relations_size: " << this->relations.size() << "\n";
    for(auto& item:this->relations)
        std::cout << item.first << "\n";
}

void Node::addRelation(std::string relationName, std::string index, std::string prop, std::string propValue) {
    this->relations[relationName].emplace_back(index,propValue);
    this->relationsProp[relationName] = prop;
}


// Different node lists, formatted as id2Node
std::unordered_map<std::string, Node> PersonMap;
std::unordered_map<std::string, Node> OrganisationMap;
std::unordered_map<std::string, Node> PlaceMap;
std::unordered_map<std::string, Node> PostMap;
std::unordered_map<std::string, Node> CommentMap;
std::unordered_map<std::string, Node> TagMap;
std::unordered_map<std::string, Node> TagClassMap;
std::unordered_map<std::string, Node> ForumMap;

// Entity ID to global ID mapping for nodes
std::unordered_map<std::string, std::string> PersonIDMap;
std::unordered_map<std::string, std::string> OrganisationIDMap;
std::unordered_map<std::string, std::string> PlaceIDMap;
std::unordered_map<std::string, std::string> PostIDMap;
std::unordered_map<std::string, std::string> CommentIDMap;
std::unordered_map<std::string, std::string> TagIDMap;
std::unordered_map<std::string, std::string> TagClassIDMap;
std::unordered_map<std::string, std::string> ForumIDMap;