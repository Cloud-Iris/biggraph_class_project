#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <stack>
#include "PProcedure.h"
#include <regex>
using namespace std;

void printResults(vector<vector<GPStore::Value>> &result) {
    if (result.empty()) {
        cout << "Empty result" << endl;
        return;
    }
    for (const auto &row : result) {
        size_t rowLen = row.size();
        for (size_t i = 0; i < rowLen; i++) {
            const auto &val = row[i];
            cout << val.toString();
            if (i != rowLen - 1)
                cout << "|";
        }
        cout << endl;
    }
}

std::string trim(const std::string& str, const std::string& whitespace = " \t\n\r") {
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return ""; // No content
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::vector<std::string> splitList(const std::string& str) {
	std::vector<std::string> tokens;
	std::string cur = "";
	stack<char> st;
	for (int i = 0; i < str.size(); ++i) {
		if (str[i] == '[') {
			st.push('[');
			cur += str[i];
		} else if (str[i] == ']') {
			st.pop();
			cur += str[i];
			if (st.empty()) {
				tokens.push_back(cur);
				cur = "";
			}
		} else if (str[i] == ',' || str[i] == ' ') {
			if (st.empty()) {
				cur = "";
			} else {
				cur += str[i];
			}
		} else {
			cur += str[i];
		}
	}
	return tokens;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

bool compareResults(std::vector<std::vector<GPStore::Value>> &result, std::vector<std::vector<std::string>> &trueResults) {
    if (result.size() != trueResults.size()) {
        cout << "Mismatch in number of rows" << endl;
        return false;
    }
    for (int i = 0; i < result.size(); ++i) {
        if (result[i].size() != trueResults[i].size()) {
            cout << "Mismatch in number of columns" << endl;
            return false;
        }
        for (int j = 0; j < result[i].size(); ++j) {
            if (result[i][j].getType() != GPStore::Value::LIST) {
                if (result[i][j].toString() != trueResults[i][j]) {
                    cout << "Mismatch in value" << endl;
                    return false;
                }
            } else {
                // Parse the trueResults into a vector<string>
                string curTrue = trueResults[i][j].substr(1, trueResults[i][j].size() - 2);	// get rid of outermost brackets
				vector<string> trueList;
				if (curTrue[0] == '[')
					trueList = splitList(curTrue);
				else
					trueList = split(curTrue, ',');
                size_t genListLen = 0;
                std::vector<GPStore::Value *> *genListVal = result[i][j].data_.List;
                for (int k = 0; k < genListVal->size(); ++k) {
                    string curVal = (*genListVal)[k]->toString();
                    if (find(trueList.begin(), trueList.end(), curVal) == trueList.end()) {
                        cout << "Mismatch in list value" << endl;
                        return false;
                    }
                    genListLen++;
                }
                if (genListLen != trueList.size()) {
                    cout << "Mismatch in list length" << endl;
                    return false;
                }
            }
        }
    }
    return true;
}

// 检查文件是否成功打开，如果失败则抛出异常
void checkOpen(const std::ifstream& fin, const std::string& path) {
    if (!fin.is_open()) {
        throw std::runtime_error("Failed to open " + path);
    }
}

// 根据属性类型创建GPStore::Value对象
GPStore::Value* createValue(const std::string& content, const std::string& propsType) {
    if (propsType.find("ID")!= std::string::npos) {
        return new GPStore::Value(std::stoll(content));
    } else if (propsType == "LABEL") {
        return new GPStore::Value(content);
    } else if (propsType == "STRING") {
        return new GPStore::Value(content);
    } else if (propsType == "LONG") {
        return new GPStore::Value(std::stoll(content));
    } else if (propsType == ("STRING[]")) {
        std::vector<std::string> stringList = split(content, ';');
        std::vector<GPStore::Value*> valueList;
        for (auto& item : stringList) {
            valueList.push_back(new GPStore::Value(item));
        }
        return new GPStore::Value(valueList, true);
    }
    return nullptr;
}

void load_node(string sf, std::unordered_map<string,std::vector<string>>& nodeType2File, std::unordered_map<string, std::unordered_map<string, Node>*>& type2Map, std::unordered_map<string, std::unordered_map<string, string>*>& type2IDMap)
{
    string separator = "/";
    string smallGraph = "social_network-csv_composite-longdateformatter-sf0.1" + separator + "social_network-csv_composite-longdateformatter-sf0.1";
    string bigGraph = "social_network-csv_composite-longdateformatter-sf3" + separator + "social_network-csv_composite-longdateformatter-sf3";

    string headersPath, dynamicPath, staticPath;
    string line1;
    int nodeId = 0;

    // 根据缩放因子选择相应的数据目录
    if(sf=="0.1"){
        headersPath = smallGraph + separator + "headers";
        dynamicPath = smallGraph + separator + "dynamic";
        staticPath = smallGraph + separator + "static";
    }
    else{
        headersPath = bigGraph + separator + "headers";
        dynamicPath = bigGraph + separator + "dynamic";
        staticPath = bigGraph + separator + "static";
    }

   for (auto& nodeInfo : nodeType2File) {
        std::string nodeType = nodeInfo.first;
        bool isDynamic = (nodeInfo.second[0] == "dynamic");

        std::string headerPath = headersPath + separator + nodeInfo.second[0] + separator + nodeInfo.second[1];
        std::string filePath = (isDynamic? dynamicPath : staticPath) + separator + nodeInfo.second[2];

        std::ifstream finHeader(headerPath);
        std::ifstream finFile(filePath);

        checkOpen(finHeader, headerPath);
        checkOpen(finFile, filePath);

        std::vector<std::string> props;

        while (std::getline(finHeader, line1)) {
            props = split(line1, '|');
            // for (auto& item : props) std::cout << item << " ";
            // std::cout << "\n";
        }

        while (std::getline(finFile, line1)) {
            GPStore::Value id(-1);
            Node node(nodeId++);
            node.setLabel(nodeType);

            std::vector<std::string> stringContents = split(line1, '|');
            if (props.size()!= stringContents.size()) {
                std::cerr << "Props and contents size not equal." << std::endl;
                return;
            }

            for (size_t i = 0; i < props.size(); ++i) {
                size_t pos = props[i].find(":");
                std::string content = stringContents[i];
                std::string propsType = props[i].substr(pos + 1);

                GPStore::Value* value = createValue(content, propsType);
                if (propsType.find("ID")!= std::string::npos) {
                    id = *value;
                }
                node.setValues(props[i], value);
            }
            
            // 添加节点到相应的Map中
            auto& innerMap = *type2Map[nodeType];
            auto& innerIDMap = *type2IDMap[nodeType];

            std::string nodeIdStr = std::to_string(node.node_id_);
            innerMap[nodeIdStr] = node;
            innerIDMap[id.toString()] = nodeIdStr;

            // if(id.toString() == "349"){
            //     std::cout << nodeType <<": " << id.toString() << "\n";
            // }
            // else if (nodeIdStr == "349"){
            //     std::cout << " odeIdStr: " << nodeIdStr << "\n";
            // }
            
        }

        // std::cout << nodeType << " Mapsize: " << type2Map[nodeType]->size() << "\n";
    }
}

// 从属性字符串中解析出类型（fromType或toType），使用正则表达式匹配括号中的内容
std::string parseTypeFromProp(const std::string& prop) {
    std::regex pattern(R"(\(([^)]+)\))");
    std::smatch matches;
    if (!std::regex_search(prop, matches, pattern)) {
        throw std::runtime_error("无法从 " + prop + " 中解析出类型");
    }
    return matches[1];
}

void load_edge(string sf, std::unordered_multimap<string,std::vector<string>>& nodeType2RelationFile, std::unordered_map<string, std::unordered_map<string, Node>*>& type2Map, std::unordered_map<string, std::unordered_map<string, string>*>& type2IDMap)
{
    string separator = "/";
    string smallGraph = "social_network-csv_composite-longdateformatter-sf0.1" + separator + "social_network-csv_composite-longdateformatter-sf0.1";
    string bigGraph = "social_network-csv_composite-longdateformatter-sf3" + separator + "social_network-csv_composite-longdateformatter-sf3";

    string headersPath, dynamicPath, staticPath;
    string line1;
    int edgeId = 0;

    // 根据缩放因子选择相应的数据目录
    if(sf=="0.1"){
        headersPath = smallGraph + separator + "headers";
        dynamicPath = smallGraph + separator + "dynamic";
        staticPath = smallGraph + separator + "static";
    }
    else{
        headersPath = bigGraph + separator + "headers";
        dynamicPath = bigGraph + separator + "dynamic";
        staticPath = bigGraph + separator + "static";
    }

    for (auto& nodeInfo : nodeType2RelationFile) {
        std::string nodeType = nodeInfo.first;
        bool isDynamic = (nodeInfo.second[0] == "dynamic");

        std::string headerPath = headersPath + separator + nodeInfo.second[0] + separator + nodeInfo.second[1];
        std::string filePath = (isDynamic? dynamicPath : staticPath) + separator + nodeInfo.second[2];

        std::ifstream finHeader(headerPath);
        std::ifstream finFile(filePath);

        checkOpen(finHeader, headerPath);
        checkOpen(finFile, filePath);

        if(filePath.find("person_isLocatedIn_place_0_0")!=-1)
        {
            std::cout << "filePath: " << filePath << "\n";
        }

        std::vector<std::string> props;
        std::string fromType, toType, attribute;
        while (std::getline(finHeader, line1)) {
            props = split(line1, '|');
            attribute = "";

            fromType = parseTypeFromProp(props[0]);
            toType = parseTypeFromProp(props[1]);

            if (props.size() == 3) attribute = props[2];

            // std::cout << "fromType: " << fromType << " toType: " << toType << "\n";
        }

        auto fromIDMapIt = type2IDMap.find(fromType);
        auto fromNodeMapIt = type2Map.find(fromType);
        auto toIDMapIt = type2IDMap.find(toType);
        auto toNodeMapIt = type2Map.find(toType);


        auto& fromIDMap = *fromIDMapIt->second;
        auto& fromNodeMap = *fromNodeMapIt->second;
        auto& toIDMap = *toIDMapIt->second;
        auto& toNodeMap = *toNodeMapIt->second;

        while (std::getline(finFile, line1)) {
            std::vector<std::string> stringContents = split(line1, '|');
            if (props.size()!= stringContents.size()) {
                throw std::runtime_error("第 " + line1 + " 行: 属性数量与内容数量不匹配");
            }
            std::string id1 = stringContents[0];
            std::string id2 = stringContents[1];

            if (fromIDMap.find(id1) == fromIDMap.end()) {
                throw std::runtime_error("ID: " + id1 + " 不在 " + fromType + " ID Map中");
            }
            std::string index1 = fromIDMap[id1];

            if (toIDMap.find(id2) == toIDMap.end()) {
                throw std::runtime_error("ID: " + id2 + " 不在 " + toType + " ID Map中");
            }
            std::string index2 = toIDMap[id2];

            auto fromNodeIt = fromNodeMap.find(index1);
            if (fromNodeIt == fromNodeMap.end()) {
                throw std::runtime_error("Index: " + index1 + " 不在 " + fromType + " Map中");
            }
            Node fromNode = fromNodeMap[index1];

            auto toNodeIt = toNodeMap.find(index2);
            if (toNodeIt == toNodeMap.end()) {
                throw std::runtime_error("Index: " + index2 + " 不在 " + toType + " Map中");
            }
            Node toNode = toNodeMap[index2];

            std::string attributeValue;
            if (props.size() == 3) attributeValue = stringContents[2];

            // 将字符串中的所有字符转换为大写
            std::string upperFromType = fromType;
            std::string upperToType = toType;
            std::transform(upperFromType.begin(), upperFromType.end(), upperFromType.begin(), ::toupper);
            std::transform(upperToType.begin(), upperToType.end(), upperToType.begin(), ::toupper);
            std::string relationName = upperFromType + "_" + upperToType;

            // TODO: fail to add relation
            Node& citeNode = fromNodeIt->second;

            // 直接在map中修改节点
            citeNode.addRelation(relationName, index2, attribute, attributeValue);
            // if(id1 == "32985348833679"){
            //     citeNode.print();
            //     cout << "relationName: " << relationName << " index2: " << index2 << " attribute: " << attribute << " attributeValue: " << attributeValue << "\n";
            // }
        }
    }
}

// 加载数据集，根据给定的缩放因子（sf）选择相应的数据目录
int load_dataset(string sf)
{
    string separator = "/";
    string smallGraph = "social_network-csv_composite-longdateformatter-sf0.1" + separator + "social_network-csv_composite-longdateformatter-sf0.1";
    string bigGraph = "social_network-csv_composite-longdateformatter-sf3" + separator + "social_network-csv_composite-longdateformatter-sf3";

    string headersPath, dynamicPath, staticPath;
    string line1;
    int nodeId = 0;

    // 根据缩放因子选择相应的数据目录
    if(sf=="0.1"){
        headersPath = smallGraph + separator + "headers";
        dynamicPath = smallGraph + separator + "dynamic";
        staticPath = smallGraph + separator + "static";
    }
    else{
        headersPath = bigGraph + separator + "headers";
        dynamicPath = bigGraph + separator + "dynamic";
        staticPath = bigGraph + separator + "static";
    }

    // 定义节点类型到文件的映射
    std::unordered_map<string,std::vector<string>> nodeType2File={
        {"Comment",{"dynamic", "Comment.csv", "comment_0_0.csv"}},
        {"Forum",{"dynamic", "Forum.csv", "forum_0_0.csv"}},
        {"Person",{"dynamic", "Person.csv", "person_0_0.csv"}},
        {"Post",{"dynamic", "Post.csv", "post_0_0.csv"}},

        {"Organisation",{"static", "Organisation.csv", "organisation_0_0.csv"}},
        {"Place",{"static", "Place.csv", "place_0_0.csv"}},
        {"Tag",{"static", "Tag.csv", "tag_0_0.csv"}},
        {"TagClass",{"static", "TagClass.csv", "tagclass_0_0.csv"}},
    };

    // 定义节点类型到关系文件的映射
    std::unordered_multimap<string, std::vector<string>> nodeType2RelationFile = {
        {"Person", {"dynamic", "Person_knows_Person.csv", "person_knows_person_0_0.csv"}},
        {"Person", {"dynamic", "Person_isLocatedIn_City.csv", "person_isLocatedIn_place_0_0.csv"}},
        {"Person", {"dynamic", "Person_studyAt_University.csv", "person_studyAt_organisation_0_0.csv"}},
        {"Person", {"dynamic", "Person_workAt_Company.csv", "person_workAt_organisation_0_0.csv"}},
        {"Person", {"dynamic", "Person_hasInterest_Tag.csv", "person_hasInterest_tag_0_0.csv"}},
        {"Person", {"dynamic", "Person_likes_Comment.csv", "person_likes_comment_0_0.csv"}},
        {"Person", {"dynamic", "Person_likes_Post.csv", "person_likes_post_0_0.csv"}},
        // {"Comment", {"dynamic", "Comment_hasCreator_Person.csv", "comment_hasCreator_person_0_0.csv"}},
        // {"Comment", {"dynamic", "Comment_hasTag_Tag.csv", "comment_hasTag_tag_0_0.csv"}},
        // {"Comment", {"dynamic", "Comment_isLocatedIn_Country.csv", "comment_isLocatedIn_place_0_0.csv"}},
        // {"Comment", {"dynamic", "Comment_replyOf_Comment.csv", "comment_replyOf_comment_0_0.csv"}},
        // {"Comment", {"dynamic", "Comment_replyOf_Post.csv", "comment_replyOf_post_0_0.csv"}},
        // {"Post", {"dynamic", "Post_hasCreator_Person.csv", "post_hasCreator_person_0_0.csv"}},
        // {"Post", {"dynamic", "Post_hasTag_Tag.csv", "post_hasTag_tag_0_0.csv"}},
        // {"Post", {"dynamic", "Post_isLocatedIn_Country.csv", "post_isLocatedIn_place_0_0.csv"}},
        // {"Forum", {"dynamic", "Forum_containerOf_Post.csv", "forum_containerOf_post_0_0.csv"}},
        // {"Forum", {"dynamic", "Forum_hasMember_Person.csv", "forum_hasMember_person_0_0.csv"}},
        // {"Forum", {"dynamic", "Forum_hasModerator_Person.csv", "forum_hasModerator_person_0_0.csv"}},
        // {"Forum", {"dynamic", "Forum_hasTag_Tag.csv", "forum_hasTag_tag_0_0.csv"}},
        // {"Organisation", {"static", "Organisation_isLocatedIn_Place.csv", "organisation_isLocatedIn_place_0_0.csv"}},
        // {"Place", {"static", "Place_isPartOf_Place.csv", "place_isPartOf_place_0_0.csv"}},
        // {"Tag", {"static", "Tag_hasType_TagClass.csv", "tag_hasType_tagclass_0_0.csv"}},
        // {"TagClass", {"static", "TagClass_isSubclassOf_TagClass.csv", "tagclass_isSubclassOf_tagclass_0_0.csv"}}
    };

    // 定义节点类型到节点映射的映射
    std::unordered_map<string, std::unordered_map<string, Node>*> type2Map={
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
    std::unordered_map<string, std::unordered_map<string, string>*> type2IDMap={
        {"Person", &PersonIDMap},
        {"Comment", &CommentIDMap},
        {"Post", &PostIDMap},
        {"Forum", &ForumIDMap},

        {"Organisation", &OrganisationIDMap},
        {"Place", &PlaceIDMap},
        {"Tag", &TagIDMap},
        {"TagClass", &TagClassIDMap},
    };

    load_node(sf, nodeType2File, type2Map, type2IDMap);

    // test if load successfully
    // auto it = PersonIDMap.find("32985348833679");
    // if (it != PersonIDMap.end()) {
    //     std::cout << "PersonIDMap found" << "\n";
    // }
    // else{
    //     std::cout << "PersonIDMap not found" << "\n";
    // }

    load_edge(sf, nodeType2RelationFile, type2Map, type2IDMap);

    cout<<"Data loaded successfully"<<endl;

    return 0; // 返回0表示成功加载数据集
}

const string DATA_DIR_0_1 = "social_network-csv_composite-longdateformatter-sf0.1/";
const string DATA_DIR_3 = "social_network-csv_composite-longdateformatter-sf3/";
/**
 * ic1 给定一个人的 $personId，找这个人直接或者间接认识的人（关系限制为 knows，最多 3 steps）
 * 然后筛选这些人 firstName 是否是给定的 $firstName，返回这些人 Persons 的：
 * distance(1 2 3)、summaries of the Persons workplaces 和 places of study
 */



int main(int argc, char *argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <sf in {0.1, 3}>" << endl;
        return 1;
    }
    string sf = argv[1];
    if (sf != "0.1" && sf != "3") {
        cout << "Usage: " << argv[0] << " <sf in {0.1, 3}>" << endl;
        return 1;
    }

    // [FILL HERE] Load the dataset according to the scale factor
    load_dataset(sf);

    // Repeatedly read test cases from stdin
    string line;
    while (getline(cin, line)) {
        if (line == "exit")
            break;
        if (line == "builtin_test") {
            // Read test cases from file and compare the results with the ground truth
            string groundTruthDir = "ground_truth/";
            vector<string> procs = {"ic1", "ic2", "is1"};
            for (const string &proc : procs) {
                // NOTE 找到对应的 ground truth 文件 ic1-sf0.1.txt, ic2-sf0.1.txt, is1-sf0.1.txt
                string groundTruthFile = groundTruthDir + proc + "-sf" + sf + ".txt";
                ifstream fin(groundTruthFile);
                if (!fin.is_open()) {
                    cout << "Failed to open " << groundTruthFile << endl;
                    return 1;
                }
                string line;
                while (getline(fin, line)) {
                    cout << proc << " " << line << endl;
                    vector<GPStore::Value> args;
                    vector<vector<GPStore::Value>> result;
                    if (proc == "ic1") {
                        // 数据例子：32985348834375 Tom
                        // 拆解为 personId = 32985348834375, firstName = Tom
                        size_t pos = line.find(" ");
                        string personId_str = line.substr(0, pos);
                        string firstName = line.substr(pos + 1);
                        long long personId = stoll(personId_str);
                        args.emplace_back(personId);
                        args.emplace_back(firstName);
                        // ic1(args, result);
                    } else if (proc == "ic2") {
                        size_t pos = line.find(" ");
                        string personId_str = line.substr(0, pos);
                        string datetime_str = line.substr(pos + 1);
                        long long personId = stoll(personId_str);
                        long long datetime = stoll(datetime_str);
                        args.emplace_back(personId);
                        args.emplace_back(datetime);
                        ic2(args, result);
                    } else if (proc == "is1") {
                        long long personId = stoll(line);
                        args.emplace_back(personId);
                        is1(args, result);
                    }
                    printResults(result);
                    getline(fin, line);
                    // 3 种文件都会有一个数字，表示接下来数据的行数
                    int numRows = stoi(line);
                    vector<vector<string>> trueResults;
                    for (int i = 0; i < numRows; ++i) {
                        getline(fin, line);
                        trueResults.emplace_back(split(line, '|'));
                    }
                    // Compare results
                    if (compareResults(result, trueResults))
                        cout << "Test passed" << endl;
                }
            }
        } else {
            size_t pos = line.find(" ");
            string proc = line.substr(0, pos);
            vector<GPStore::Value> args;
            vector<vector<GPStore::Value>> result;
            if (proc == "ic1") {
                size_t next_pos = line.find(" ", pos + 1);
                if (next_pos == string::npos) {
                    cout << "Invalid input" << endl;
                    continue;
                }
                string personId_str = line.substr(pos + 1, next_pos - pos - 1);
                string firstName = line.substr(next_pos + 1);
                long long personId = -1;
                try {
                    personId = stoll(personId_str);
                } catch (invalid_argument &e) {
                    cout << "Invalid input" << endl;
                    continue;
                }
                args.emplace_back(personId);
                args.emplace_back(firstName);
                ic1(args, result);
            } else if (proc == "ic2") {
                size_t next_pos = line.find(" ", pos + 1);
                if (next_pos == string::npos) {
                    cout << "Invalid input" << endl;
                    continue;
                }
                string personId_str = line.substr(pos + 1, next_pos - pos - 1);
                string datetime_str = line.substr(next_pos + 1);
                long long personId = -1, datetime = -1;
                try {
                    personId = stoll(personId_str);
                } catch (invalid_argument &e) {
                    cout << "Invalid input" << endl;
                    continue;
                }
                try {
                    datetime = stoll(datetime_str);
                } catch (invalid_argument &e) {
                    cout << "Invalid input" << endl;
                    continue;
                }
                args.emplace_back(personId);
                args.emplace_back(datetime);
                ic2(args, result);
            } else if (proc == "is1") {
                string personId_str = line.substr(pos + 1);
                long long personId = -1;
                try {
                    personId = stoll(personId_str);
                } catch (invalid_argument &e) {
                    cout << "Invalid input" << endl;
                    continue;
                }
                args.emplace_back(personId);
                is1(args, result);
            }
            printResults(result);
        }
    }
    return 0;
}