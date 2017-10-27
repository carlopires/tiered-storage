#ifndef __COMMON_H__
#define __COMMON_H__

#include <string>
#include <boost/functional/hash.hpp>
#include <boost/format.hpp>
#include <boost/crc.hpp>
#include <functional>
#include <curl/curl.h>

using namespace std;

// Define global replication factor
#define REPLICATION 2

// Define port offset
#define CLIENT_CONNECTION_OFFSET -100
#define CLIENT_NOTIFY_OFFSET 500
#define SEED_CONNECTION_OFFSET 0
#define CHANGESET_OFFSET 0
#define LOCAL_GOSSIP_OFFSET 0
#define DISTRIBUTED_GOSSIP_OFFSET 0
#define LOCAL_REDISTRIBUTE_OFFSET 100
#define LOCAL_DEPART_OFFSET 200
#define LOCAL_DEPART_DONE_OFFSET 300
#define NODE_JOIN_OFFSET 100
#define NODE_DEPART_OFFSET 200
#define KEY_EXCHANGE_OFFSET 300

#define SERVER_PORT 6560

class node_t {
public:
    node_t() {}
    node_t(string ip, size_t port): ip_(ip), port_(port) {
        id_ = ip + ":" + to_string(port);
    }
    string id_;
    string ip_;
    size_t port_;
};

class master_node_t: public node_t {
public:
    master_node_t() : node_t() {}
    master_node_t(string ip) : node_t(ip, SERVER_PORT) {
        seed_connection_connect_addr_ = "tcp://" + ip + ":" + to_string(SERVER_PORT + SEED_CONNECTION_OFFSET);
        seed_connection_bind_addr_ = "tcp://*:" + to_string(SERVER_PORT + SEED_CONNECTION_OFFSET);
        client_notify_connect_addr_ = "tcp://" + ip + ":" + to_string(SERVER_PORT + CLIENT_NOTIFY_OFFSET);
        client_notify_bind_addr_ = "tcp://*:" + to_string(SERVER_PORT + CLIENT_NOTIFY_OFFSET);
        changeset_addr_ = "inproc://" + to_string(SERVER_PORT + CHANGESET_OFFSET);
        node_join_connect_addr_ = "tcp://" + ip + ":" + to_string(SERVER_PORT + NODE_JOIN_OFFSET);
        node_join_bind_addr_ = "tcp://*:" + to_string(SERVER_PORT + NODE_JOIN_OFFSET);
        node_depart_connect_addr_ = "tcp://" + ip + ":" + to_string(SERVER_PORT + NODE_DEPART_OFFSET);
        node_depart_bind_addr_ = "tcp://*:" + to_string(SERVER_PORT + NODE_DEPART_OFFSET);
        key_exchange_connect_addr_ = "tcp://" + ip + ":" + to_string(SERVER_PORT + KEY_EXCHANGE_OFFSET);
        key_exchange_bind_addr_ = "tcp://*:" + to_string(SERVER_PORT + KEY_EXCHANGE_OFFSET);
        local_depart_done_addr_ = "inproc://" + to_string(SERVER_PORT + LOCAL_DEPART_DONE_OFFSET);
    }
    string seed_connection_connect_addr_;
    string seed_connection_bind_addr_;
    string client_notify_connect_addr_;
    string client_notify_bind_addr_;
    string changeset_addr_;
    string node_join_connect_addr_;
    string node_join_bind_addr_;
    string node_depart_connect_addr_;
    string node_depart_bind_addr_;
    string key_exchange_connect_addr_;
    string key_exchange_bind_addr_;
    string local_depart_done_addr_;
};

class worker_node_t: public node_t {
public:
    worker_node_t() : node_t() {}
    worker_node_t(string ip, size_t port) : node_t(ip, port) {
        client_connection_connect_addr_ = "tcp://" + ip + ":" + to_string(port + CLIENT_CONNECTION_OFFSET);
        client_connection_bind_addr_ = "tcp://*:" + to_string(port + CLIENT_CONNECTION_OFFSET);
        local_gossip_addr_ = "inproc://" + to_string(port + LOCAL_GOSSIP_OFFSET);
        distributed_gossip_connect_addr_ = "tcp://" + ip + ":" + to_string(port + DISTRIBUTED_GOSSIP_OFFSET);
        distributed_gossip_bind_addr_ = "tcp://*:" + to_string(port + DISTRIBUTED_GOSSIP_OFFSET);
        local_redistribute_addr_ = "inproc://" + to_string(port + LOCAL_REDISTRIBUTE_OFFSET);
        local_depart_addr_ = "inproc://" + to_string(port + LOCAL_DEPART_OFFSET);
    }
    string client_connection_connect_addr_;
    string client_connection_bind_addr_;
    string local_gossip_addr_;
    string distributed_gossip_connect_addr_;
    string distributed_gossip_bind_addr_;
    string local_redistribute_addr_;
    string local_depart_addr_;
};

bool operator<(const node_t& l, const node_t& r) {
    if (l.id_.compare(r.id_) == 0) return false;
    else return true;
}

bool operator==(const node_t& l, const node_t& r) {
    if (l.id_.compare(r.id_) == 0) return true;
    else return false;
}

struct node_hash {
    std::size_t operator () (const node_t &n) const {
        return std::hash<string>{}(n.id_);
    }
};

struct crc32_hasher {
    uint32_t operator()(const node_t& node) {
        boost::crc_32_type ret;
        ret.process_bytes(node.id_.c_str(), node.id_.size());
        return ret.checksum();
    }
    uint32_t operator()(const string& key) {
        boost::crc_32_type ret;
        ret.process_bytes(key.c_str(), key.size());
        return ret.checksum();
    }
    typedef uint32_t result_type;
};

struct ebs_hasher {
    hash<string>::result_type operator()(const node_t& node) {
        return hash<string>{}(node.id_);
    }
    hash<string>::result_type operator()(const string& key) {
        return hash<string>{}(key);
    }
    typedef hash<string>::result_type result_type;
};

void split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string getIP() {
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "http://169.254.169.254/latest/meta-data/public-ipv4");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    }
    return readBuffer;
}

typedef consistent_hash_map<master_node_t,crc32_hasher> global_hash_t;



#endif
