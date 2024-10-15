#include "globals.h"

unordered_map<string,users*> usersMap;
vector<string> usersLogin;
vector<string> activegroupsid;
vector<group*> activegroups;
unordered_map<pair<string,string>,char,pair_hash> downloads;
