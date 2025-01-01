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

Node GetPersonNode(const std::string &person_id) {
    auto it = PersonIDMap.find(person_id);
    if (it == PersonIDMap.end()) {
        // To Do: Error handling
        cout<<"PersonIDMap not found"<<endl;
        return Node(-1);
    }
    auto it2 = PersonMap.find(it->second);
    if (it2 == PersonMap.end()) {
        // To Do: Error handling
        cout<<"PersonMap not found"<<endl;
        return Node(-1);
    }
    return it2->second;
}

/**
 * ic1 给定一个人的 $personId，找这个人直接或者间接认识的人（关系限制为 knows，最多 3 steps）
 * 然后筛选这些人 firstName 是否是给定的 $firstName，返回这些人 Persons 的：
 * distance(1 2 3)、summaries of the Persons workplaces 和 places of study
 * input: ic1 32985348834375 Tom
 */
void ic1(const std::vector<GPStore::Value> &args, std::vector<std::vector<GPStore::Value>> &result) {
    string first_name = args[1].toString();
    const char* first_name_char = first_name.data();
    unsigned first_name_size = first_name.size();

    Node person_node = GetPersonNode(args[0].toString());

    if (person_node.node_id_ == -1)
        return;
    std::set<std::tuple<int, std::string, long long, unsigned> > candidates;
    TYPE_ENTITY_LITERAL_ID start_vid = person_node.node_id_;
    std::vector<TYPE_ENTITY_LITERAL_ID> curr_frontier({start_vid});
    std::set<TYPE_ENTITY_LITERAL_ID> visited({start_vid});

    for (int distance = 0; distance <= 3; distance++) {
        std::vector<TYPE_ENTITY_LITERAL_ID > next_frontier;
        for (const auto& vid : curr_frontier) {
        Node froniter_person(vid);
        bool flag = vid == start_vid;
        flag = flag || (froniter_person["firstName"]->toString() != first_name);
        if (flag) continue;
        std::string last_name = froniter_person["lastName"]->toString();
        long long person_id = froniter_person["id"]->toLLong();
        auto tup = std::make_tuple(distance, last_name, person_id, vid);
        if (candidates.size() >= LIMIT_NUM) {
            auto& candidate = *candidates.rbegin();
            if (tup > candidate) continue;
        }
        candidates.emplace(std::move(tup));
        if (candidates.size() > LIMIT_NUM) {
            candidates.erase(--candidates.end());
        }
        }
        if (candidates.size() >= LIMIT_NUM || distance == 3) break;
        for (auto vid : curr_frontier) {
        Node froniter_person(vid);
        std::shared_ptr<const TYPE_ENTITY_LITERAL_ID[]> friends_list = nullptr; unsigned list_len;
        froniter_person.GetLinkedNodes("KNOWS", friends_list, list_len, EDGE_OUT);
        for (unsigned friend_index = 0; friend_index < list_len; ++friend_index) {
            TYPE_ENTITY_LITERAL_ID friend_vid = friends_list[friend_index];
            if (visited.find(friend_vid) == visited.end()) {
            visited.emplace(friend_vid);
            next_frontier.emplace_back(friend_vid);
            }
        }
        friends_list = nullptr;
        froniter_person.GetLinkedNodes("KNOWS", friends_list, list_len, EDGE_IN);
        for (unsigned friend_index = 0; friend_index < list_len; ++friend_index) {
            TYPE_ENTITY_LITERAL_ID friend_vid = friends_list[friend_index];
            if (visited.find(friend_vid) == visited.end()) {
            visited.emplace(friend_vid);
            next_frontier.emplace_back(friend_vid);
            }
        }
        friends_list = nullptr;
        }
        std::sort(next_frontier.begin(), next_frontier.end());
        curr_frontier.swap(next_frontier);
    }

    for (const auto & tup : candidates) {
        unsigned vid = std::get<3>(tup);
        Node person(vid);

        result.emplace_back();
        result.back().reserve(13);
        result.back().emplace_back(std::get<2>(tup));
        result.back().emplace_back(std::get<1>(tup));
        result.back().emplace_back(std::get<0>(tup));
        result.back().emplace_back(*person["birthday"]);
        result.back().emplace_back(*person["creationDate"]);
        result.back().emplace_back(*person["gender"]);
        result.back().emplace_back(*person["browserUsed"]);
        result.back().emplace_back(*person["locationIP"]);
        result.back().emplace_back(*person["email"]);
        result.back().emplace_back(*person["language"]);
        result.back().emplace_back(*Node(person["PERSON_PLACE"]->toLLong())["name"]);   // TODO: PERSON_PLACE not in original data

        std::shared_ptr<const unsigned[]> list = nullptr; unsigned list_len = 0;
        std::shared_ptr<const long long[]> prop_list = nullptr; unsigned prop_len = 0;

        result.back().emplace_back(GPStore::Value::Type::LIST);
        person.GetLinkedNodesWithEdgeProps("STUDY_AT", list, prop_list, prop_len, list_len, EDGE_OUT);
        for (unsigned i = 0; i < list_len; ++i) {
        Node university(list[i]);
        Node location_city(university["ORGANISATION_PLACE"]->toLLong());    // TODO: "ORGANISATION_PLACE"
        vector<GPStore::Value *> university_prop_vec{university["name"], new GPStore::Value(prop_list[i]), location_city["name"]};
        result.back().back().data_.List->emplace_back(new GPStore::Value(university_prop_vec, true));
        }
        list = nullptr; prop_list = nullptr;

        result.back().emplace_back(GPStore::Value::Type::LIST);
        person.GetLinkedNodesWithEdgeProps("WORK_AT", list, prop_list, prop_len, list_len, EDGE_OUT);
        for (unsigned i = 0; i < list_len; ++i) {
        Node company(list[i]);
        Node location_country(company["ORGANISATION_PLACE"]->toLLong());
        vector<GPStore::Value *> university_prop_vec{company["name"], new GPStore::Value(prop_list[i]), location_country["name"]};
        result.back().back().data_.List->emplace_back(new GPStore::Value(university_prop_vec, true));
        }
    }
}

// 给定 ID 为 $personId 的起始 Person，查找来自该 Person 的所有好友（好友节点）的最新消息。
// 仅考虑在给定$maxDate之前（不包括当天）创建的消息。
// input: ic2 15393162789932 1345740969124
void ic2(const std::vector<GPStore::Value> &args, std::vector<std::vector<GPStore::Value>> &result) {
    long long person_id = args[0].toLLong();
    long long max_date = args[1].toLLong();

    Node person_node = GetPersonNode(args[0].toString());
    if (person_node.node_id_ == -1) {
        return;
    }

    std::set<std::tuple<long long, std::string, std::string>> messages; // 存储消息，按日期排序
    std::shared_ptr<const TYPE_ENTITY_LITERAL_ID[]> friends_list = nullptr; 
    unsigned list_len = 0;

    // 获取好友列表
    person_node.GetLinkedNodes("KNOWS", friends_list, list_len, EDGE_OUT);
    for (unsigned i = 0; i < list_len; ++i) {
        Node friend_node(friends_list[i]);
        std::shared_ptr<const TYPE_ENTITY_LITERAL_ID[]> messages_list = nullptr; 
        unsigned messages_len = 0;

        // 获取好友的消息列表
        friend_node.GetLinkedNodes("HAS_POST", messages_list, messages_len, EDGE_OUT);
        for (unsigned j = 0; j < messages_len; ++j) {
            Node message_node(messages_list[j]);
            long long creation_date = message_node["creationDate"]->toLLong();
            if (creation_date < max_date) {
                std::string content = message_node["content"]->toString();
                std::string message_id = message_node["id"]->toString();
                messages.emplace(creation_date, message_id, content);
            }
        }
    }

    // 将消息添加到结果中
    for (const auto& msg : messages) {
        result.emplace_back();
        result.back().emplace_back(std::get<1>(msg)); // message_id
        result.back().emplace_back(std::get<2>(msg)); // content
        result.back().emplace_back(std::get<0>(msg)); // creation_date
    }
}

// 给定 ID 为 $personId 的开始人员，检索其名字、姓氏、生日、IP 地址、浏览器和居住城市。
// input: is1 32985348833679
void is1(const std::vector<GPStore::Value> &args, std::vector<std::vector<GPStore::Value>> &result) {

    Node person_node = GetPersonNode(args[0].toString());
    person_node.print();
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
    std::cout << "columns_size: " << person_node.columns.size() << "\n";
    for (auto& attr : attributeNames1) {
        for (auto& item : person_node.columns) {
            if (item.first.find(attr) != std::string::npos) { // 检查 attr 是否是 item.first 的子串
                result.back().emplace_back(item.second);
                break;
            }
        }
    }

    // To Do: Add location city
    std::cout << "relations.size: " << person_node.relations.size() << "\n";
    for (auto& item : person_node.relations) {
        if(item.first.find("PERSON_PLACE")==-1)
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