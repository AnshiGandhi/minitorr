#ifndef GLOBALS_H
#define GLOBALS_H

#include <unordered_map>
#include <string>
#include <vector>
#include <tuple>
#include <functional> 
#include <utility>

using namespace std;

struct pair_hash {
    template <class T1, class T2>
    size_t operator() (const pair<T1, T2>& p) const {
        auto hash1 = hash<T1>{}(p.first);
        auto hash2 = hash<T2>{}(p.second);
        return hash1 ^ hash2; // XOR combination of the two hash values
    }
};

class file{
public:
    string name;
    string sha;
    int numpieces;
    string shapieces;
};

class group {
public:
    string id;
    string admin;
    vector<string> members;
    vector<string> pending_request;
    unordered_map<string,file*> shared_files;
    unordered_map<string,vector<string>> seeders;
};

class users {
public:
    string username;
    string password;
    string ip;
    int port;
    unordered_map<string,string> locatefile;
    unordered_map<string,vector<int>> available_chunks;
};

extern unordered_map<string,users*> usersMap;
extern vector<string> usersLogin;
extern vector<string> activegroupsid;
extern vector<group*> activegroups;
extern unordered_map<pair<string,string>,char,pair_hash> downloads;

#endif // GLOBALS_H
