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
        // true 表示要进行深拷贝
        return new GPStore::Value(valueList, true);
    }
    return nullptr;
}

void load_node(
    string sf,
    string separator,
    string headersPath,
    string dynamicPath,
    string staticPath,
    std::unordered_map<string,std::vector<string>>& nodeType2File
) {
    string line1;
    int nodeId = 0;

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

        // 读入表头，获取属性列表
        // 以 Person 的表头为例，数据格式为：id:ID(Person)|firstName:STRING|lastName:STRING|gender:STRING|birthday:LONG|creationDate:LONG|locationIP:STRING|browserUsed:STRING|language:STRING[]|email:STRING[]
        while (std::getline(finHeader, line1)) {
            props = split(line1, '|');
        }

        // 逐行读入节点数据
        // 以 Person 的数据为例，数据格式为：933|Mahinda|Perera|male|628646400000|1266161530447|119.235.7.103|Firefox|si;en|Mahinda933@boarderzone.com;Mahinda933@hotmail.com;Mahinda933@yahoo.com;Mahinda933@zoho.com
        while (std::getline(finFile, line1)) {
            GPStore::Value id(-1);
            Node node(nodeId++);  // 创建一个新的节点，编号从 0 开始
            node.setLabel(nodeType);

            std::vector<std::string> stringContents = split(line1, '|');
            // 核对表头 和 数据 的列数是否一致
            if (props.size()!= stringContents.size()) {
                std::cerr << "Props and contents size not equal." << std::endl;
                return;
            }

            for (size_t i = 0; i < props.size(); ++i) {
                size_t pos = props[i].find(":");
                std::string content = stringContents[i];
                std::string propsType = props[i].substr(pos + 1);

                GPStore::Value* value = createValue(content, propsType);
                // 如果是 ID 属性，则将该属性的值作为节点的 ID
                if (propsType.find("ID")!= std::string::npos) {
                    id = *value;
                    // printf("id: %s\n", id.toString().c_str());
                }
                // 以 key-value 的形式存储属性
                // 注意，ID的存储方式是 key(string): "ID(Person)"  value(long long): 32985348834375
                node.setValues(props[i], value);
            }
            
            // static 中的节点，例如 Place 的形式为：id:ID(Place)|name:STRING|url:STRING|:LABEL，会有一个 :LABEL 属性
            // if(!isDynamic){
            //     string type = node.columns[":LABEL"].toString();
            //     // 把具体数据中的 label 首字母大写，如 country, city 转换为 Country, City
            //     type = string(1, toupper(type[0])) + type.substr(1);
            //     // 过滤掉类型不对应的数据
            //     if (type != nodeType) continue;
            // }

            // 添加节点到相应的Map中
            auto& innerMap = *type2Map[nodeType];
            auto& innerIDMap = *type2IDMap[nodeType];

            // 这里的 id 就是 nodeId 变量
            std::string nodeIdStr = std::to_string(node.node_id_);
            // { Node 类中的全局 id : 构造好的 Node 对象 }
            innerMap[nodeIdStr] = node;
            // { 数据中的 ID : Node 类中的全局 id }
            innerIDMap[id.toString()] = nodeIdStr;
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

void load_edge(
    string sf,
    string separator,
    string headersPath,
    string dynamicPath,
    string staticPath,
    std::unordered_multimap<string,std::vector<string>>& nodeType2RelationFile
) {
    string line1;
    int edgeId = 0;

    for (auto& nodeInfo : nodeType2RelationFile) {
        std::string nodeType = nodeInfo.first;
        bool isDynamic = (nodeInfo.second[0] == "dynamic");

        std::string headerPath = headersPath + separator + nodeInfo.second[0] + separator + nodeInfo.second[1];
        std::string filePath = (isDynamic? dynamicPath : staticPath) + separator + nodeInfo.second[2];

        std::ifstream finHeader(headerPath);
        std::ifstream finFile(filePath);

        checkOpen(finHeader, headerPath);
        checkOpen(finFile, filePath);

        std::vector<std::string> props;

        // 读入表头，获取属性列表
        // 以 Person_knows_Person 的表头为例，数据格式为：:START_ID(Person)|:END_ID(Person)|creationDate:LONG
        std::string fromType, toType, attribute;
        while (std::getline(finHeader, line1)) {
            props = split(line1, '|');
            attribute = "";

            // 获得关系 两侧的 节点类型
            fromType = parseTypeFromProp(props[0]);
            toType = parseTypeFromProp(props[1]);

            if (props.size() == 3) attribute = props[2];

            // std::cout << "fromType: " << fromType << " toType: " << toType << "\n";
        }

        // { Node 类中的全局 id : 构造好的 Node 对象 }
        auto& fromNodeMap = *(type2Map.find(fromType)->second);
        auto& toNodeMap = *(type2Map.find(toType)->second);
        // { 数据中的 ID : Node 类中的全局 id }
        auto& fromIDMap = *(type2IDMap.find(fromType)->second);
        auto& toIDMap = *(type2IDMap.find(toType)->second);

        // 逐行读入节点数据
        // 以 Person_knows_Person 的数据为例，数据格式为：933|2199023256077|1271939457947
        while (std::getline(finFile, line1)) {
            std::vector<std::string> stringContents = split(line1, '|');
            // 核对表头 和 数据 的列数是否一致
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

            auto toNodeIt = toNodeMap.find(index2);
            if (toNodeIt == toNodeMap.end()) {
                throw std::runtime_error("Index: " + index2 + " 不在 " + toType + " Map中");
            }

            std::string attributeValue;
            if (props.size() == 3) attributeValue = stringContents[2];
 
            // 把 nodeInfo.second[1] 去掉 .CSV 的后缀后，转换为大写，作为关系名
            std::string relationName = split(nodeInfo.second[1], '.')[0];
            std::transform(relationName.begin(), relationName.end(), relationName.begin(), ::toupper);

            if (index1 == "8336" && relationName == "ORGANISATION_ISLOCATEDIN_PLACE") {
                std::cout << "☀" << endl;
                std::cout << "index1: " << index1 << " index2: " << index2 << endl;
                std::cout << "☀" << endl;
            }

            // 用 & 操作符表示是对原本节点的引用，而不是拷贝
            Node& fromNodeRef = fromNodeIt->second;
            Node& toNodeRef = toNodeIt->second;
            // 直接在map中修改节点
            fromNodeRef.addRelation(relationName, index2, attribute, attributeValue);
            // 如果是 PERSON_KNOWS_PERSON 关系，则需要在 toNode 中也添加关系，形成双向关系
            if (relationName == "PERSON_KNOWS_PERSON") {
                toNodeRef.addRelation(relationName, index1, attribute, attributeValue);
            }

            // 在处理边的时候，对于 Post 和 Comment 的创建者关系，建立反向关系，便于从 Person 出边查询
            if (relationName == "POST_HASCREATOR_PERSON") {
                // toNode是Person，fromNode是Post
                toNodeRef.addRelation("PERSON_CREATED_POST", index1, attribute, attributeValue);
            }
            else if (relationName == "COMMENT_HASCREATOR_PERSON") {
                // toNode是Person，fromNode是Comment
                toNodeRef.addRelation("PERSON_CREATED_COMMENT", index1, attribute, attributeValue);
            }
        }
    }
}

// 加载数据集，根据给定的缩放因子（sf）选择相应的数据目录
int load_dataset(string sf)
{
    string separator = "/";
    // 当前目录下的两个数据集  "...-sf0.1"  "...-sf3"
    string dataBasePath = "social_network-csv_composite-longdateformatter-sf" + sf;

    string headersPath, dynamicPath, staticPath;
    string line1;
    int nodeId = 0;

    // 根据缩放因子选择相应的数据目录  
    headersPath = dataBasePath + separator + "headers";
    dynamicPath = dataBasePath + separator + "dynamic";
    staticPath = dataBasePath + separator + "static";

    // 定义节点类型到文件的映射
    // 以 节点类型 为键，值为一个vector，包含了 动态/静态、节点 的表头文件、具体的节点数据文件
    std::unordered_map<string,std::vector<string>> nodeType2File={
        // NOTE: LDBC SNB 数据库生成的数据集中，节点对应的文件名是：[节点类型]_0_0.csv
        {"Comment",{"dynamic", "Comment.csv", "comment_0_0.csv"}},
        {"Forum",{"dynamic", "Forum.csv", "forum_0_0.csv"}},
        {"Person",{"dynamic", "Person.csv", "person_0_0.csv"}},
        {"Post",{"dynamic", "Post.csv", "post_0_0.csv"}},

        {"Organisation",{"static", "Organisation.csv", "organisation_0_0.csv"}},
        {"Place",{"static", "Place.csv", "place_0_0.csv"}},
        {"Tag",{"static", "Tag.csv", "tag_0_0.csv"}},
        {"TagClass",{"static", "TagClass.csv", "tagclass_0_0.csv"}},
    };

    // 定义节点类型到 以节点为起点的关系 文件的映射
    // 以 节点类型 为键，值为一个vector，包含了 动态/静态、以节点为起点的关系 的表头文件、具体的关系数据文件
    std::unordered_multimap<string, std::vector<string>> nodeType2RelationFile = {
        // NOTE: LDBC SNB 数据库生成的数据集中，关系对应的文件名是：[关系]_0_0.csv
        {"Person", {"dynamic", "Person_knows_Person.csv", "person_knows_person_0_0.csv"}},
        {"Person", {"dynamic", "Person_isLocatedIn_City.csv", "person_isLocatedIn_place_0_0.csv"}},
        {"Person", {"dynamic", "Person_studyAt_University.csv", "person_studyAt_organisation_0_0.csv"}},
        {"Person", {"dynamic", "Person_workAt_Company.csv", "person_workAt_organisation_0_0.csv"}},
        {"Person", {"dynamic", "Person_hasInterest_Tag.csv", "person_hasInterest_tag_0_0.csv"}},
        {"Person", {"dynamic", "Person_likes_Comment.csv", "person_likes_comment_0_0.csv"}},
        {"Person", {"dynamic", "Person_likes_Post.csv", "person_likes_post_0_0.csv"}},
        {"Comment", {"dynamic", "Comment_hasCreator_Person.csv", "comment_hasCreator_person_0_0.csv"}},
        {"Comment", {"dynamic", "Comment_hasTag_Tag.csv", "comment_hasTag_tag_0_0.csv"}},
        {"Comment", {"dynamic", "Comment_isLocatedIn_Country.csv", "comment_isLocatedIn_place_0_0.csv"}},
        {"Comment", {"dynamic", "Comment_replyOf_Comment.csv", "comment_replyOf_comment_0_0.csv"}},
        {"Comment", {"dynamic", "Comment_replyOf_Post.csv", "comment_replyOf_post_0_0.csv"}},
        {"Post", {"dynamic", "Post_hasCreator_Person.csv", "post_hasCreator_person_0_0.csv"}},
        {"Post", {"dynamic", "Post_hasTag_Tag.csv", "post_hasTag_tag_0_0.csv"}},
        {"Post", {"dynamic", "Post_isLocatedIn_Country.csv", "post_isLocatedIn_place_0_0.csv"}},
        // {"Forum", {"dynamic", "Forum_containerOf_Post.csv", "forum_containerOf_post_0_0.csv"}},
        // {"Forum", {"dynamic", "Forum_hasMember_Person.csv", "forum_hasMember_person_0_0.csv"}},
        // {"Forum", {"dynamic", "Forum_hasModerator_Person.csv", "forum_hasModerator_person_0_0.csv"}},
        // {"Forum", {"dynamic", "Forum_hasTag_Tag.csv", "forum_hasTag_tag_0_0.csv"}},
        {"Organisation", {"static", "Organisation_isLocatedIn_Place.csv", "organisation_isLocatedIn_place_0_0.csv"}},
        // {"Place", {"static", "Place_isPartOf_Place.csv", "place_isPartOf_place_0_0.csv"}},
        // {"Tag", {"static", "Tag_hasType_TagClass.csv", "tag_hasType_tagclass_0_0.csv"}},
        // {"TagClass", {"static", "TagClass_isSubclassOf_TagClass.csv", "tagclass_isSubclassOf_tagclass_0_0.csv"}}
    };

    load_node(sf, separator, headersPath, dynamicPath, staticPath, nodeType2File);

    load_edge(sf, separator, headersPath, dynamicPath, staticPath, nodeType2RelationFile);

    cout<<"Data loaded successfully"<<endl;
    return 0; // 返回0表示成功加载数据集
}


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
    // IC1 测试代码：查找核心 Person 对象
    // GetNode("Person", "32985348834375").print();
    // IC1 测试代码：查找目标对象【看看是否将 studyAt\workAt 关系添加到了 Person 节点中】
    // GetNode("Person", "4398046512492").print();
    // IC1 测试代码：查找 Organisation 对象 Florida_State_University_College_of_Business (数据 ID:6805, 全局 ID 8336)
    // GetNode("Organisation", "6805").print();
    // IC1 测试代码：查找 Place 对象 Tallahassee (数据 ID:903, 全局 ID 974)
    // GetNode("Place", "903").print();
    // 调试代码，仅仅查看是否成功加载数据集
    // return 0;

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
                        // 中心点这个人是 Alfred|Hoffmann
                        printf("参数1-要检查的 personID: %lld\n参数2-认识的人要遵守的 firstName: %s\n", personId, firstName.c_str());
                        ic1(args, result);
                        // return 0;
                    }
                    else if (proc == "ic2") {
                        size_t pos = line.find(" ");
                        string personId_str = line.substr(0, pos);
                        string datetime_str = line.substr(pos + 1);
                        args.emplace_back(personId_str);
                        args.emplace_back(datetime_str);
                        ic2(args, result);
                    } 
                    else if (proc == "is1") {
                        long long personId = stoll(line);
                        args.emplace_back(personId);
                        is1(args, result);
                    }
                    // printResults(result);
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