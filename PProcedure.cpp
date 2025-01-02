#include <iostream>
#include "PProcedure.h"
using namespace std;
const unsigned LIMIT_NUM = 20;
const char EDGE_IN = 'i';
const char EDGE_OUT = 'o';

void ProcessMessageLikes(Node &message, long long message_id, long long message_creation_date,
const std::string &message_content, Node &other_person, std::map<TYPE_ENTITY_LITERAL_ID, long long> &person_id_map,
std::map<long long, std::pair<long long, long long> > &candidates_index, std::map<std::pair<long long, long long>, std::tuple<long long, long long, std::string, int> > &candidates) {
    std::shared_ptr<const TYPE_ENTITY_LITERAL_ID[]> person_friends = nullptr;
    unsigned friends_num = 0;
    std::shared_ptr<const long long[]> creation_date_list = nullptr;
    unsigned creation_data_width = 0;
    message.GetLinkedNodesWithEdgeProps("LIKES", person_friends, creation_date_list, creation_data_width, friends_num, EDGE_IN);
    for (unsigned j = 0; j < friends_num; ++j) {
        auto person_vid = person_friends[j];
        long long like_creation_date = creation_date_list[j];
        auto it = candidates_index.find(person_vid);
        if (it != candidates_index.end()) {
        auto &key = it->second;
        if (like_creation_date < 0 - key.first) {
            continue;
        }
        if (like_creation_date == 0 - key.first) {
            auto cit = candidates.find(key);
            long long old_message_id = std::get<1>(cit->second);
            if (message_id > old_message_id) {
            continue;
            }
        }
        candidates.erase(key);
        key.first = 0 - like_creation_date;
        candidates.emplace(key, std::make_tuple(person_vid, message_id, message_content,
                            (like_creation_date - message_creation_date) / 1000 / 60));
        } else {
        long long person_id;
        auto pit = person_id_map.find(person_vid);
        if (pit != person_id_map.end()) {
            person_id = pit->second;
        } else {
            other_person.Goto(person_vid);
            person_id = other_person["id"]->toLLong();
            person_id_map[person_vid] = person_id;
        }
        auto key = std::make_pair(0 - like_creation_date, person_id);
        if (candidates.size() >= LIMIT_NUM && candidates.lower_bound(key) == candidates.end()) {
            continue;
        }
        candidates.emplace(key, std::make_tuple(person_vid, message_id, message_content,
                            (like_creation_date - message_creation_date) / 1000 / 60));
        candidates_index.emplace(person_vid, key);
        if (candidates.size() > LIMIT_NUM) {
            auto cit = --candidates.end();
            candidates_index.erase(candidates_index.find(std::get<0>(cit->second)));
            candidates.erase(cit);
        }
        }
    }
}

Node GetNodeByEntityID(std::string node_type, const std::string &entity_id) {
    auto& IDMap = *(type2IDMap.find(node_type)->second);
    auto& myMap = *(type2Map.find(node_type)->second);

    // 根据 数据实体 id 获取 Node 类的全局 id
    auto it = IDMap.find(entity_id);
    if (it == IDMap.end()) {
        cout<<"PersonIDMap not found"<<endl;
        return Node(-1);
    }
    // 根据找到的 Node 类的全局 id 获取 Node 对象
    auto it2 = myMap.find(it->second);
    if (it2 == myMap.end()) {
        cout<<"PersonMap not found"<<endl;
        return Node(-1);
    }
    return it2->second;
}

// 根据节点类型和全局 id 获取 Node 对象
// 注意：这里相当于是拷贝，所以只能读取，修改不会作用于原对象
Node GetNodeByGlobalID(std::string node_type, const std::string &global_id) {
    // 根据节点类型找到对应的 Node Map
    auto& nodeMap = *(type2Map.find(node_type)->second);
    auto it = nodeMap.find(global_id);
    if (it == nodeMap.end()) {
        cout << node_type << " Map not found ❗"<<endl;
        return Node(-1);
    }
    return it->second;
}

/**
 * ic1  input: $personId $firstName
 * 给定一个人的 $personId，找这个人直接或者间接认识的人（关系限制为 knows，最多 3 steps）
 * 然后筛选这些人 firstName 是否是给定的 $firstName，返回这些人 Persons 的 13 种信息
 * 
 * 涉及到的节点类型：Person, Place(:LABEL=country, city), Organisation(:LABEL=university, company)
 *  1. Place 有 3 种 LABEL: country, city, continent，这里只涉及到 country 和 city
 *  2. Organisation 有 2 中 LABEL: university, company
 * 涉及到的关系类型：person_knows_person, person_isLocatedIn_place, organisation_isLocatedIn_place, place_isPartOf_place
 *  1. person_knows_person。对应表头文件：Person_knows_Person
 * --- 以下是检索到认识的人需要的信息 ---
 *  2. person_isLocatedIn_place。对应表头文件：Person_isLocatedIn_City
 *  3. person_workAt_organisation。对应表头文件：Person_workAt_Company
 *      3.1 如果这个人存在工作的公司，则还要找到这个公司所在的国家。
 *          关系：organisation_isLocatedIn_place。对应表头文件：Organisation_isLocatedIn_Place
 *  4. person_studyAt_organisation。对应表头文件：Person_studyAt_University
 *      4.1 如果这个人存在学习的学校，则还要找到这个学校所在的城市。
 *          关系：organisation_isLocatedIn_place。对应表头文件：Organisation_isLocatedIn_Place
 * 
 * 结果要求：
 * 排序：按照 distance 从小到大排序，按照 last name 字典序排序，按照 person_id 从小到大排序
 * 最多返回 20 个结果
 */
void ic1(const std::vector<GPStore::Value> &args, std::vector<std::vector<GPStore::Value>> &result) {
    string first_name = args[1].toString();
    const char* first_name_char = first_name.data();
    unsigned first_name_size = first_name.size();
    // 参数是 数据实体 id，找到 参数对应的 Person 节点
    Node person_node = GetNodeByEntityID("Person", args[0].toString());
    // person_node.print();

    if (person_node.node_id_ == -1)
        return;

    std::set<std::tuple<int, std::string, long long, unsigned> > candidates;
    // 注意这里是全局 id
    TYPE_ENTITY_LITERAL_ID start_vid = person_node.node_id_;
    std::vector<TYPE_ENTITY_LITERAL_ID> curr_frontier({start_vid});
    std::set<TYPE_ENTITY_LITERAL_ID> visited({start_vid});

    for (int distance = 0; distance <= 3; distance++) {
        std::vector<TYPE_ENTITY_LITERAL_ID > next_frontier;
        for (const auto& vid : curr_frontier) {
            // Node froniter_person(vid);
            Node froniter_person = GetNodeByGlobalID("Person", std::to_string(vid));
            if (froniter_person.node_id_ == -1) return;

            bool flag = vid == start_vid;
            // 查看这些人的 firstName 是否等于 给定的参数 $firstName
            // flag = flag || (froniter_person["firstName"]->toString() != first_name);
            flag = flag || (froniter_person.columns["firstName:STRING"].toString() != first_name);
            if (flag) continue;
            // froniter_person.print();

            // std::string last_name = froniter_person["lastName"]->toString();
            // long long person_id = froniter_person["id"]->toLLong();
            std::string last_name = froniter_person.columns["lastName:STRING"].toString();
            long long person_id = froniter_person.columns["id:ID(Person)"].toLLong();
            // 符合条件的 认识的人
            auto tup = std::make_tuple(distance, last_name, person_id, vid);
            // 如果此时候已经有 20 个结果了，且当前结果比最后一个结果还要大，则跳过
            if (candidates.size() >= LIMIT_NUM) {
                auto& candidate = *candidates.rbegin();
                // c++ 元组会自动按元素 逐个进行比较
                if (tup > candidate) continue;
            }
            // std::move 将 tup 的所有权从当前作用域转移到 candidates 集合，不进行数据的拷贝
            candidates.emplace(std::move(tup));
            // 若插入新元素导致 candidates.size() 大于 LIMIT_NUM，则删除最后一个元素
            if (candidates.size() > LIMIT_NUM) {
                candidates.erase(--candidates.end());
            }
        }
        if (candidates.size() >= LIMIT_NUM || distance == 3) break;

        for (auto vid : curr_frontier) {
            // Node froniter_person(vid);
            Node froniter_person = GetNodeByGlobalID("Person", std::to_string(vid));
            // std::shared_ptr<const string[]> friends_list = nullptr; unsigned list_len;
            // froniter_person.GetLinkedNodes("KNOWS", friends_list, list_len, EDGE_OUT);
            std::vector<std::string> friends_list; unsigned list_len;
            froniter_person.GetLinkedNodes("PERSON_KNOWS_PERSON", friends_list, list_len, EDGE_OUT);

            for (unsigned friend_index = 0; friend_index < list_len; ++friend_index) {
                // 将字符串形式的全局 id 转为 unsigned 类型
                TYPE_ENTITY_LITERAL_ID friend_vid = std::stoul(friends_list[friend_index]);
                // 如果当前节点未被访问过，则将其加入 visited 集合。并在下一轮循环中继续访问
                if (visited.find(friend_vid) == visited.end()) {
                    visited.emplace(friend_vid);
                    next_frontier.emplace_back(friend_vid);
                }
            }
            // 因为我们组在增加 "PERSON_KNOWS_PERSON" 关系的时候，加入的是双向边，所以这里不需要遍历出边
            // friends_list = nullptr;
            // froniter_person.GetLinkedNodes("KNOWS", friends_list, list_len, EDGE_IN);
        }
        std::sort(next_frontier.begin(), next_frontier.end());
        // 交换内容 next_frontier 会变为空容器，而 curr_frontier 会包含排序后的元素。
        curr_frontier.swap(next_frontier);
    }

    for (const auto & tup : candidates) {
        // 将元组中的第 4 个元素赋值给 vid，即候选者的全局 id
        unsigned vid = std::get<3>(tup);
        // Node person(vid);
        Node person = GetNodeByGlobalID("Person", std::to_string(vid));

        result.emplace_back();
        // 预先分配 13 个空间记录候选者的 13 个信息
        result.back().reserve(13);
        // 认识的人数据实体 id、last name、distance
        result.back().emplace_back(std::get<2>(tup));
        result.back().emplace_back(std::get<1>(tup));
        result.back().emplace_back(std::get<0>(tup));
        // birthday、creationDate、gender、browserUsed、locationIP、email、language、place
        // result.back().emplace_back(*person["birthday"]);
        result.back().emplace_back(person.columns["birthday:LONG"]);
        result.back().emplace_back(person.columns["creationDate:LONG"]);
        result.back().emplace_back(person.columns["gender:STRING"]);
        result.back().emplace_back(person.columns["browserUsed:STRING"]);
        result.back().emplace_back(person.columns["locationIP:STRING"]);
        result.back().emplace_back(person.columns["email:STRING[]"]);
        result.back().emplace_back(person.columns["language:STRING[]"]);

        
        // 要补充所在城市、大学、公司
        // result.back().emplace_back(*Node(person["PERSON_PLACE"]->toLLong())["name"]);   // TODO: PERSON_PLACE not in original data
        auto place_pair = person.relations["PERSON_ISLOCATEDIN_CITY"][0];
        // place_pair 的形式是 <全局 id(place), "">
        string person_place_id = place_pair.first;
        Node person_place = GetNodeByGlobalID("Place", person_place_id);
        result.back().emplace_back(person_place.columns["name:STRING"]);

        // 推入一个 vector，存储大学信息
        result.back().emplace_back(GPStore::Value::Type::LIST);
        // 返回关系为 STUDY_AT 的节点
        // person.GetLinkedNodesWithEdgeProps("STUDY_AT", list, prop_list, prop_len, list_len, EDGE_OUT);
        for (auto study_pair : person.relations["PERSON_STUDYAT_UNIVERSITY"]) {
            // study_pair 的形式是 <全局 id(organisation), "classYear">
            string organ_id = study_pair.first;
            Node university = GetNodeByGlobalID("Organisation", organ_id);
            auto place_pair = university.relations["ORGANISATION_ISLOCATEDIN_PLACE"][0];
            // place_pair 的形式是 <全局 id(place), "">
            string organ_place_id = place_pair.first;
            Node location_city = GetNodeByGlobalID("Place", organ_place_id);
            vector<GPStore::Value *> university_prop_vec{
                &university.columns["name:STRING"],
                new GPStore::Value(std::stoll(study_pair.second)), // studyAt.classYear
                &location_city.columns["name:STRING"]
            };
            result.back().back().data_.List->emplace_back(new GPStore::Value(university_prop_vec, true));
        }
        // for (unsigned i = 0; i < list_len; ++i) {
        //     Node university(list[i]);
        //     Node location_city(university["ORGANISATION_PLACE"]->toLLong());    // TODO: "ORGANISATION_PLACE"
        //     vector<GPStore::Value *> university_prop_vec{
        //         university["name"],
        //         new GPStore::Value(prop_list[i]), // studyAt.classYear
        //         location_city["name"]
        //     };
        //     result.back().back().data_.List->emplace_back(new GPStore::Value(university_prop_vec, true));
        // }
        // list = nullptr; prop_list = nullptr;

        // 推入一个 vector，存储公司信息
        result.back().emplace_back(GPStore::Value::Type::LIST);
        // 返回关系为 WORK_AT 的节点
        for (auto work_pair : person.relations["PERSON_WORKAT_COMPANY"]) {
            // work_pair 的形式是 <全局 id(organisation), "workFrom">
            string organ_id = work_pair.first;
            Node company = GetNodeByGlobalID("Organisation", organ_id);
            auto place_pair = company.relations["ORGANISATION_ISLOCATEDIN_PLACE"][0];
            // place_pair 的形式是 <全局 id(place), "">
            string organ_place_id = place_pair.first;
            Node location_country = GetNodeByGlobalID("Place", organ_place_id);
            vector<GPStore::Value *> university_prop_vec{
                &company.columns["name:STRING"],
                new GPStore::Value(std::stoll(work_pair.second)), // workAt.workFrom
                &location_country.columns["name:STRING"]
            };
            result.back().back().data_.List->emplace_back(new GPStore::Value(university_prop_vec, true));
        }

        // person.GetLinkedNodesWithEdgeProps("WORK_AT", list, prop_list, prop_len, list_len, EDGE_OUT);
        // for (unsigned i = 0; i < list_len; ++i) {
        //     Node company(list[i]);
        //     Node location_country(company["ORGANISATION_PLACE"]->toLLong());
        //     vector<GPStore::Value *> university_prop_vec{company["name"], new GPStore::Value(prop_list[i]), location_country["name"]};
        //     result.back().back().data_.List->emplace_back(new GPStore::Value(university_prop_vec, true));
        // }
    }
}

// 给定 ID 为 $personId 的起始 Person，查找来自该 Person 的所有好友（好友节点）的最新消息。
// 仅考虑在给定$maxDate之前（不包括当天）创建的消息。
// input: ic2 15393162789932 1345740969124
void ic2(const std::vector<GPStore::Value> &args, std::vector<std::vector<GPStore::Value>> &result) {
    // 获取输入参数
    long long person_id = args[0].toLLong();
    long long max_date = args[1].toLLong();
    
    // 获取起始Person节点
    Node person_node = GetNodeByEntityID("Person", std::to_string(person_id));
    if (person_node.node_id_ == -1) return;

    // 用于存储结果的map，key为创建时间（用于排序）
    std::map<std::pair<long long, long long>, std::tuple<long long, std::string, std::string, long long, std::string, long long>> messages;
    
    // 获取所有好友
    std::vector<std::string> friends_list;
    unsigned list_len;
    person_node.GetLinkedNodes("PERSON_KNOWS_PERSON", friends_list, list_len, EDGE_OUT);

    // 遍历每个好友
    for (const auto& friend_id : friends_list) {
        Node friend_node = GetNodeByGlobalID("Person", friend_id);
        if (friend_node.node_id_ == -1) continue;

        // 获取好友的基本信息
        long long friend_person_id = friend_node.columns["id:ID(Person)"].toLLong();
        std::string firstName = friend_node.columns["firstName:STRING"].toString();
        std::string lastName = friend_node.columns["lastName:STRING"].toString();

        // 处理该好友创建的所有Post
        for (const auto& post_pair : friend_node.relations["PERSON_CREATED_POST"]) {
            Node post = GetNodeByGlobalID("Post", post_pair.first);
            if (post.node_id_ == -1) continue;

            long long creation_date = post.columns["creationDate:LONG"].toLLong();
            if (creation_date > max_date) continue;

            long long message_id = post.columns["id:ID(Post)"].toLLong();
            std::string content;
            
            // 先尝试获取imageFile
            auto it = post.columns.find("imageFile:STRING");
            if (it != post.columns.end() && !it->second.toString().empty()) {
                content = it->second.toString();
            } else {
                // 如果没有imageFile或为空，则获取content
                content = post.columns["content:STRING"].toString();
            }

            messages[{-creation_date, message_id}] = std::make_tuple(
                friend_person_id, firstName, lastName, message_id, content, creation_date);
        }

        // 处理该好友创建的所有Comment
        for (const auto& comment_pair : friend_node.relations["PERSON_CREATED_COMMENT"]) {
            Node comment = GetNodeByGlobalID("Comment", comment_pair.first);
            if (comment.node_id_ == -1) continue;

            long long creation_date = comment.columns["creationDate:LONG"].toLLong();
            if (creation_date >= max_date) continue;

            long long message_id = comment.columns["id:ID(Comment)"].toLLong();
            std::string content = comment.columns["content:STRING"].toString();

            messages[{-creation_date, message_id}] = std::make_tuple(
                friend_person_id, firstName, lastName, message_id, content, creation_date);
        }
    }

    // 将结果添加到result中
    for (const auto& message : messages) {
        result.emplace_back();
        // 取消 C++ 14 引入的结构化绑定，改用 std::get
        // const auto& [friend_id, first_name, last_name, message_id, content, creation_date] = message.second;
        const auto& tuple = message.second;
        long long friend_id = std::get<0>(tuple);
        const std::string& first_name = std::get<1>(tuple);
        const std::string& last_name = std::get<2>(tuple);
        long long message_id = std::get<3>(tuple);
        const std::string& content = std::get<4>(tuple);
        long long creation_date = std::get<5>(tuple);

        result.back().emplace_back(friend_id);    // personId
        result.back().emplace_back(first_name);   // firstName
        result.back().emplace_back(last_name);    // lastName
        result.back().emplace_back(message_id);   // messageId
        result.back().emplace_back(content);      // content
        result.back().emplace_back(creation_date); // creationDate
        
        if (result.size() >= LIMIT_NUM) break;
    }
}

// 给定 ID 为 $personId 的开始人员，检索其名字、姓氏、生日、IP 地址、浏览器和居住城市。
// input: is1 32985348833679
void is1(const std::vector<GPStore::Value> &args, std::vector<std::vector<GPStore::Value>> &result) {

    Node person_node = GetNodeByEntityID("Person", args[0].toString());
    // person_node.print();
    result.emplace_back();
    result.back().reserve(8);

    std::vector<std::string> attributeNames1 = {
        "firstName",
        "lastName",
        "birthday",
        "locationIP",
        "browserUsed",
    };

    std::vector<std::string> attributeNames2 = {
        "gender",
        "creationDate"
    };

    // 打印节点的详细信息
    // person_node.print();
    for (auto& attr : attributeNames1) {
        for (auto& item : person_node.columns) {
            if (item.first.find(attr) != std::string::npos) { // 检查 attr 是否是 item.first 的子串
                result.back().emplace_back(item.second);
                break;
            }
        }
    }

    for (auto& item : person_node.relations) {
        if(item.first.find("PERSON_ISLOCATEDIN_CITY")==-1)
        {
            continue;
        }
        for(auto& item2 : item.second)
        {
            for(auto& item3 : PlaceIDMap)
            {
                if(item3.second == item2.first)
                {
                    result.back().emplace_back(item3.first);
                    break;
                }
            }
        }
    }

    for (auto& attr : attributeNames2) {
        for (auto& item : person_node.columns) {
            if (item.first.find(attr) != std::string::npos) { // 检查 attr 是否是 item.first 的子串
                result.back().emplace_back(item.second);
                break;
            }
        }
    }
}