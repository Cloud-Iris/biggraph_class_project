#include <string>
#include <memory>
#include "Value.h"
#include <unordered_map>

class Node {
  public:
    unsigned node_id_; // Node ID
    std::string label_string; // Label of the node
    std::string prop_string; // Properties of the node
    std::unordered_map<std::string,GPStore::Value> columns;  //node的属性

    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> relations;
    std::unordered_map<std::string, std::string> relationsProp; //The first string is the relation type, the second string is the ID

    Node()=default; // Default constructor
    Node(const std::string& label_string, const std::string& prop_string, const GPStore::Value* value); // Constructor with parameters
    Node(unsigned node_id); // Constructor with node ID
    void Goto(unsigned new_node_id) { // Method to update node ID
      node_id_ = new_node_id;
    }
    GPStore::Value* operator[](const std::string& property_string); // Overloaded operator to access properties
    void GetLinkedNodes(const std::string&, std::shared_ptr<const unsigned[]>& nodes_list, unsigned& list_len, char edge_dir); // Method to get linked nodes
    void GetLinkedNodesWithEdgeProps(const std::string& pre_str, std::shared_ptr<const unsigned[]>& nodes_list, std::shared_ptr<const long long[]>& prop_list,
                                    unsigned& prop_len, unsigned& list_len, char edge_dir); // Method to get linked nodes with edge properties
    void setLabel(const std::string& label_string);
    void setValues(const std::string& prop_string, const GPStore::Value* value);

    void addRelation(std::string relationName, std::string index, std::string prop, std::string propValue);

    void print(); // Method to print node details

};

// Different node lists, formatted as id2Node
extern std::unordered_map<std::string, Node> PersonMap;
extern std::unordered_map<std::string, Node> OrganisationMap;
extern std::unordered_map<std::string, Node> CityMap;
extern std::unordered_map<std::string, Node> CompanyMap;
extern std::unordered_map<std::string, Node> PlaceMap;
extern std::unordered_map<std::string, Node> UniversityMap;
extern std::unordered_map<std::string, Node> CountryMap;
extern std::unordered_map<std::string, Node> MessageMap;
extern std::unordered_map<std::string, Node> PostMap;
extern std::unordered_map<std::string, Node> CommentMap;

// Entity ID to global ID mapping for nodes
extern std::unordered_map<std::string, std::string> PersonIDMap;
extern std::unordered_map<std::string, std::string> OrganisationIDMap;
extern std::unordered_map<std::string, std::string> CityIDMap;
extern std::unordered_map<std::string, std::string> CompanyIDMap;
extern std::unordered_map<std::string, std::string> PlaceIDMap;
extern std::unordered_map<std::string, std::string> UniversityIDMap;
extern std::unordered_map<std::string, std::string> CountryIDMap;
extern std::unordered_map<std::string, std::string> MessageIDMap;
extern std::unordered_map<std::string, std::string> PostIDMap;
extern std::unordered_map<std::string, std::string> CommentIDMap;
