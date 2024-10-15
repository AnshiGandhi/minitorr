#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <vector>
#include <unordered_map>
#include <thread>
#include <algorithm>
#include <openssl/sha.h>
#include <cmath>
#include <sstream>
#include <sys/stat.h>
#include "globals.h"

using namespace std;

vector<string> split_string(string &str, char delimiter) {
    vector<string> result;
    stringstream ss(str);
    string item;
    while (getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    return result;
}

void create_user(vector<string> command,int clientSocket,string ip = "" , int port = -1) {
    if (command.size() != 3) {
        // cout << "Invalid command" << endl;
        send(clientSocket,"Invalid Command",strlen("Invalid Command"),0);
        return;
    }
    string username = command[1];
    string password = command[2];
    if (usersMap.count(username) > 0) {
        // cout << "User already exists!" << endl;
        send(clientSocket,"User already exists!",strlen("User already exists!"),0);
    } else {
        users *new_user = new users();
        new_user->username = username;
        new_user->password = password;
        new_user->ip = ip;
        new_user->port = port;
        usersMap[username] = new_user;
        cout << "User created: " << username<< endl;
        send(clientSocket,"User created",strlen("User created"),0);
    }
}

void login(vector<string> command , string &user , int clientSocket,string ip,int port){
    if(command.size() != 3){
        // cout << "invalid command" << endl;
        send(clientSocket,"Invalid Command",strlen("Invalid Command"),0);
        return;
    }
    string username = command[1];
    string password = command[2];
    if(usersMap.count(username) > 0){
        if(usersMap[username]->password == password){
            if(user!=""){
                send(clientSocket,"User already logged in",strlen("User already logged in"),0);
                return;
            }
            if(find(usersLogin.begin(),usersLogin.end(),username) !=  usersLogin.end()){
                send(clientSocket,"User already logged in",strlen("User already logged in"),0);
                return;
            }
            if(user  == ""){
                cout << "Login successful " <<  username << endl;
                send(clientSocket,"Login successful ",strlen("Login successful "),0);
                usersLogin.push_back(username);
                usersMap[username]->ip = ip;
                usersMap[username]->port = port;
                user = username;
            }
        }
        else{
            // cout << "Incorrect password" << endl;
            send(clientSocket,"Incorrect password",strlen("Incorrect password"),0);
            return;
        }
    }
    else{
        // cout << "User does not exist" << endl;
        send(clientSocket,"User does not exist",strlen("User does not exist"),0);
        return;
    }
}

bool check_user(string &user , int clientSocket){
    if(strcmp(user.c_str(),"") == 0){
        // cout << "You are not logged in" << endl;
        send(clientSocket,"You are not logged in",strlen("You are not logged in"),0);
        return false;
    }
    return true;
}

void create_group(vector<string> command , string &user ,int clientSocket){
    if(command.size() != 2){
        // cout << "Invalid command" << endl;
        send(clientSocket,"Invalid command",strlen("Invalid command"),0);
        return;
    }
    string groupid = command[1];
    if(find(activegroupsid.begin(), activegroupsid.end(), groupid) != activegroupsid.end()){
        // cout << "Group already exists" << endl;
        send(clientSocket,"Group already exists",strlen("Group already exists"),0);
    }
    else{
        group *new_group = new  group();
        new_group->id = groupid;
        new_group->admin = user;
        new_group->members.push_back(user);
        activegroupsid.push_back(groupid);
        activegroups.push_back(new_group);
        cout << "Group created: " << new_group->id << " by " << user << endl;
        send(clientSocket,"Group created",strlen("Group created"),0);
    }
}

void join_group(vector<string> command , string &user , int clientSocket){
    if(command.size() != 2){
        // cout << "Invalid command" << endl;
        send(clientSocket,"Invalid command",strlen("Invalid command"),0);
        return;
    }
    string groupid = command[1];
    if(find(activegroupsid.begin(), activegroupsid.end(), groupid) == activegroupsid.end()){
        // cout << "No such group" << endl;
        send(clientSocket,"No such group",strlen("No such group"),0);
    }
    for(int i=0;i<activegroups.size();i++){
        if(activegroups[i]->id == groupid){
            if(find(activegroups[i]->members.begin(), activegroups[i]->members.end(), user) != activegroups[i]->members.end()){
                // cout << "You have already joined the group" << endl ;
                send(clientSocket,"You have already joined the group",strlen("You have already joined the group"),0);
                return;
            }
            if(find(activegroups[i]->pending_request.begin(), activegroups[i]->pending_request.end(), user) != activegroups[i]->pending_request.end()){
                // cout << "You have already sent a request to join this group" << endl;
                send(clientSocket,"You have already sent a request to join this group",strlen("You have already sent a request to join this group"),0);
                return;
            }
            activegroups[i]->pending_request.push_back(user);
            cout << "Requested to join the group : " << groupid << endl;
            send(clientSocket,"Requested to join the group",strlen("Requested to join the group"),0);
            return;
        }
    }
}

void leave_group(vector<string> command , string &user , int clientSocket){
    if(command.size() != 2){
        // cout << "Invalid command" << endl;
        send(clientSocket,"Invalid command",strlen("Invalid command"),0);
        return;
    }
    string groupid = command[1];
    if(find(activegroupsid.begin(), activegroupsid.end(), groupid) == activegroupsid.end()){
        // cout << "No such group" << endl;
        send(clientSocket,"No such group",strlen("No such group"),0);
        return;
    }
    for(int i = 0 ; i<activegroups.size();i++){
        if(activegroups[i]->id == groupid){
            if(find(activegroups[i]->members.begin(), activegroups[i]->members.end(), user) != activegroups[i]->members.end()){
                auto it = find(activegroups[i]->members.begin(), activegroups[i]->members.end(),user);
                activegroups[i]->members.erase(it);
                if(activegroups[i]->members.size() == 0){
                    activegroups.erase(activegroups.begin() + i);
                    activegroupsid.erase(activegroupsid.begin() + i);
                }
                if(strcmp(user.c_str(),activegroups[i]->admin.c_str()) == 0){
                    activegroups[i]->admin = activegroups[i]->members[0];
                }
                cout << "Left the group : " << groupid << endl;
                send(clientSocket,"Left the group",strlen("Left the group"),0);
                return;
            }
            if(find(activegroups[i]->pending_request.begin(), activegroups[i]->pending_request.end(), user) != activegroups[i]->pending_request.end()){
                auto it = find(activegroups[i]->pending_request.begin(), activegroups[i]->pending_request.end(),user);
                activegroups[i]->pending_request.erase(it);
                cout << "Request to join the group : " << groupid << " has been cancelled";
                send(clientSocket,"Request cancelled",strlen("Request cancelled"),0);
                return;
            }
            // cout << "You are not a member of this group" << endl;
            send(clientSocket,"You are not a member of this group",strlen("You are not a member of this group"),0);
        }
    }
}

// only for admin
void list_requests(vector<string> command , int clientSocket){
    if(command.size() != 2){
        // cout << "Invalid command" << endl;
        send(clientSocket,"Invalid command",strlen("Invalid command"),0);
        return;
    }
    string groupid = command[1];
    if(find(activegroupsid.begin(), activegroupsid.end(), groupid) == activegroupsid.end()){
        // cout << "No such group" << endl;
        send(clientSocket,"No such group",strlen("No such group"),0);
        return;
    }
    for(int i = 0 ; i<activegroups.size();i++){
        if(activegroups[i]->id == groupid){
            // cout << "Requests to join the group : " << groupid << endl;
            string grp = "";
            for(int j=0 ; j<activegroups[i]->pending_request.size();j++){
                // cout << activegroups[i]->pending_request[j] << endl;
                grp += activegroups[i]->pending_request[j];
                grp += " ";
            }
            send(clientSocket,grp.c_str(),grp.length(),0);
            return;
        }
    }
}

void accept_request(vector<string> command , string &user, int clientSocket){
    if(command.size() != 3){
        // cout << "Invalid command" << endl;
        send(clientSocket,"Invalid command",strlen("Invalid command"),0);
        return;
    }
    string groupid = command[1];
    string username = command[2];
    if(find(activegroupsid.begin(), activegroupsid.end(), groupid) == activegroupsid.end()){
        // cout << "No such group" << endl;
        send(clientSocket,"No such group",strlen("No such group"),0);
        return;
    }
    if(find(usersLogin.begin(), usersLogin.end(), username) == usersLogin.end()){
        // cout << "No such user" << endl;
        send(clientSocket,"No such user",strlen("No such user"),0);
        return;
    }
    for(int i = 0 ; i<activegroups.size();i++){
        if(activegroups[i]->id == groupid){
            if(activegroups[i]->admin == user){
                if(find(activegroups[i]->members.begin(), activegroups[i]->members.end(), username) != activegroups[i]->members.end()){
                    // cout << "User is already a member of this group" << endl;
                    send(clientSocket,"User is already a member of this group",strlen("User is already a member of this group"),0);
                    return;
                }
                if(find(activegroups[i]->pending_request.begin(), activegroups[i]->pending_request.end(), username) == activegroups[i]->pending_request.end()){
                    // cout << "User is not in the pending request list" << endl;
                    send(clientSocket,"User is not in the pending request list",strlen("User is not in the pending request list"),0);
                    return;
                }
                auto it = find(activegroups[i]->pending_request.begin(), activegroups[i]->pending_request.end(),username);
                activegroups[i]->pending_request.erase(it);
                activegroups[i]->members.push_back(username);
                cout <<  "User " << username << " has been added to the group " << groupid << endl;
                send(clientSocket,"User has been added to the group",strlen("User has been added to the group"),0);
            }
            else{
                // cout << "You are not the admin of this group" << endl;
                send(clientSocket,"You are not the admin of this group",strlen("You are not the admin of this group"),0);
            }
        }
    }
}

void list_groups(vector<string> command , int clientSocket){
    if(command.size() != 1){
        // cout << "Invalid command" << endl;
        send(clientSocket,"Invalid command",strlen("Invalid command"),0);
        return;
    }
    // cout << "Currently active groups : " << endl;
    string a = "";
    for(int i=0 ; i<activegroupsid.size();i++){
        // cout << activegroupsid[i] << endl;
        a += activegroupsid[i];
        a += " ";
    }
    send(clientSocket,a.c_str(),a.length(),0);
}

void logout(vector<string> command , string &user ,int clientSocket){
    if(command.size() != 1){
        // cout << "Invalid command" << endl;
        send(clientSocket,"Invalid command",strlen("Invalid command"),0);
        return;
    }
    if(user == ""){
        // cout << "You are not logged in" << endl;
        send(clientSocket,"User not logged in",strlen("User not logged in"),0);
    }
    auto it = find(usersLogin.begin(), usersLogin.end(), user);
    if(it != usersLogin.end()){
        usersLogin.erase(it);
    }
    usersMap[user]->ip="";
    usersMap[user]->port=-1;
    user = "";
    cout << "User logged out" << endl;
    send(clientSocket,"User logged out",strlen("User logged out"),0);
}

void upload_file(vector<string> command , string &user , int clientSocket){
    if(command.size() != 3){
        // cout << "Invalid command" << flush << endl;
        send(clientSocket,"Invalid command",strlen("Invalid command"),0);
        return;
    }
    string filepath = command[1];
    string groupid = command[2];
    size_t lastSlash = filepath.find_last_of("/");
    string filename = filepath.substr(lastSlash + 1);
    int filesize;
    if(access(filepath.c_str(), F_OK) != 0){
        // cout << "File does not exist" << flush << endl;
        send(clientSocket,"File does not exist",strlen("File does not exist"),0);
        return;
    }
    if(find(activegroupsid.begin(), activegroupsid.end(), groupid) == activegroupsid.end()){
        // cout << "No such group" << flush << endl;
        send(clientSocket,"No such group",strlen("No such group"),0);
        return;
    }
    for(int i = 0 ; i<activegroups.size();i++){
        if(activegroups[i]->id == groupid){
            if(find(activegroups[i]->members.begin(), activegroups[i]->members.end(), user) == activegroups[i]->members.end()){
                // cout << "User is not a member of this group" << flush << endl;
                send(clientSocket,"User is not a member of this group",strlen("User is not a member of this group"),0);
                return;
            }
        }
    }
    cout<<"Uploading..." << flush << endl;
    file *f = new file();
    f->name = filename;
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) {
        // cout<<"Error opening file" << flush << endl;
        send(clientSocket,"Error opening file.",strlen("Error opening file."),0);
        return;
    }
    filesize = lseek(fd, 0, SEEK_END);
    if (filesize == -1) {
        cerr << "Error using lseek: " << strerror(errno) << endl;
        close(fd);
        return;
    }
    f->numpieces = ceil((filesize*1.0)/521000);
    for(int i=0;i<f->numpieces;i++){
        usersMap[user]->available_chunks[filename].push_back(i);
    }
    lseek(fd, 0, SEEK_SET);
    unsigned char fhash[SHA_DIGEST_LENGTH];
    char* buffer = new char[filesize + 1]; 
    memset(buffer, 0, filesize + 1);
    ssize_t bytesRead = read(fd, buffer, filesize);
    if (bytesRead == -1) {
        // cout << "Error reading file" << flush << endl;
        send(clientSocket,"Error reading file.",strlen("Error reading file."),0);
        delete[] buffer;
        close(fd);
        return;
    }
    SHA1(reinterpret_cast<const unsigned char*>(buffer), filesize, fhash);
    string sha_str;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        char buff[3];
        sprintf(buff, "%02x", fhash[i]);
        sha_str += buff;
    }
    f->sha = sha_str;
    // cout<<f->sha<<flush;
    const size_t chunkSize = 521000;
    for (size_t i = 0; i < filesize; i += chunkSize) {
        size_t currentChunkSize = min(chunkSize, static_cast<size_t>(filesize - i));  // Handle last chunk
        unsigned char pieceHash[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(buffer + i), currentChunkSize, pieceHash);  // Use currentChunkSize
        for (int j = 0; j < SHA_DIGEST_LENGTH; j++) {
            char buff[3];
            sprintf(buff, "%02x", pieceHash[j]);
            f->shapieces += buff;
        }
    }
    delete[] buffer; 
    close(fd);
    for(int i = 0 ; i<activegroups.size();i++){
        if(activegroups[i]->id == groupid){
            activegroups[i]->shared_files[filename] = f;
            activegroups[i]->seeders[filename].push_back(user);
            break;
        }
    }
    usersMap[user]->locatefile[filename] = filepath;
    cout << "File uploaded" << flush << endl;
    send(clientSocket,"File Uploaded",strlen("File Uploaded"),0);
}

void list_files(vector<string> command , string &user, int clientSocket) {
    if (command.size() != 2) {
        // Invalid command
        send(clientSocket, "Invalid command", strlen("Invalid command"), 0);
        return;
    }

    string groupid = command[1];
    if (find(activegroupsid.begin(), activegroupsid.end(), groupid) == activegroupsid.end()) {
        // No such group
        send(clientSocket, "No such group", strlen("No such group"), 0);
        return;
    }

    // Check if the user is a member of the group
    for (int i = 0; i < activegroups.size(); i++) {
        if (activegroups[i]->id == groupid) {
            if (find(activegroups[i]->members.begin(), activegroups[i]->members.end(), user) == activegroups[i]->members.end()) {
                // User is not a member of this group
                send(clientSocket, "User is not a member of this group", strlen("User is not a member of this group"), 0);
                return;
            }
        }
    }

    // Iterate through the shared files in the group
    for (int i = 0; i < activegroups.size(); i++) {
        if (activegroups[i]->id == groupid) {
            string response = "";

            for (auto &file_entry : activegroups[i]->shared_files) {
                string filename = file_entry.first;
                file* file_info = file_entry.second;

                // Check if the seeders list for this file is not empty
                if (activegroups[i]->seeders.find(filename) != activegroups[i]->seeders.end()) {
                    vector<string> seeders = activegroups[i]->seeders[filename];

                    // Check if at least one seeder is logged in
                    bool seeder_logged_in = false;
                    for (const string &seeder : seeders) {
                        if (find(usersLogin.begin(), usersLogin.end(), seeder) != usersLogin.end()) {
                            seeder_logged_in = true;
                            break; // No need to check further once we find a logged-in seeder
                        }
                    }

                    // If at least one seeder is logged in, add the file to the response
                    if (seeder_logged_in) {
                        response += filename + " ";
                    }
                }
            }

            // Send the list of available files back to the client
            if (response.empty()) {
                send(clientSocket, "No available files", strlen("No available files"), 0);
            } else {
                send(clientSocket, response.c_str(), response.length(), 0);
            }
            break;
        }
    }
}

// Function to send the entire response in a loop
void sendAll(int clientSocket, const string& response) {
    size_t totalSent = 0;
    size_t toSend = response.length();  // Total bytes to send
    const char* data = response.c_str();

    while (totalSent < toSend) {
        ssize_t bytesSent = send(clientSocket, data + totalSent, toSend - totalSent, 0);
        if (bytesSent == -1) {
            cerr << "Error while sending data!" << endl;
            break;
        }
        totalSent += bytesSent;  // Update the total number of bytes sent
    }
}

void download_file(vector<string> command, string &user, int clientSocket,int port) {
    // Check for valid command format
    if (command.size() != 4) {
        send(clientSocket, "Invalid command", strlen("Invalid command"), 0);
        return;
    }

    string groupid = command[1];
    string filename = command[2];
    string destinationPath = command[3];

    downloads[make_pair(groupid,filename)] = 'D';

    // Check if the group exists
    if (find(activegroupsid.begin(), activegroupsid.end(), groupid) == activegroupsid.end()) {
        send(clientSocket, "No such group", strlen("No such group"), 0);
        return;
    }

    // Check if the user is a member of the group
    for (int i = 0; i < activegroups.size(); i++) {
        if (activegroups[i]->id == groupid) {
            if (find(activegroups[i]->members.begin(), activegroups[i]->members.end(), user) == activegroups[i]->members.end()) {
                send(clientSocket, "You are not a member of this group", strlen("You are not a member of this group"), 0);
                return;
            }

            // Check if the file exists in the group
            if (activegroups[i]->shared_files.count(filename) == 0) {
                send(clientSocket, "File does not exist in the group", strlen("File does not exist in the group"), 0);
                return;
            }

            // Get file info
            file *f = activegroups[i]->shared_files[filename];

            // Construct the response string
            string response;
            response += "SHA:" + f->sha + "|";
            response += "NUMPIECES:" + to_string(f->numpieces) + "|";
            response += "SHAPIECES:" + f->shapieces + "|";
            response += "SEEDERS:";

            // Append seeders and their available chunks
            if (activegroups[i]->seeders.count(filename) > 0) {
                for (const string& seeder : activegroups[i]->seeders[filename]) {
                    if (find(usersLogin.begin(), usersLogin.end(), seeder) != usersLogin.end()) {
                        response += seeder + "#";  // Seeder's username
                        response += "PIECES:";

                        // Add available chunks for each seeder
                        for (int chunk : usersMap[seeder]->available_chunks[filename]) {
                            response += to_string(chunk) + ",";
                        }
                        response.pop_back();  // Remove the last comma
                        response += "#";
                        response += usersMap[seeder]->ip;
                        response += "#";
                        response += to_string(usersMap[seeder]->port);
                        response += "#";
                        response += usersMap[seeder]->locatefile[filename];
                        response += "@";  // Delimiter for the next seeder
                    }
                }
                response.pop_back();  // Remove the last '@'
            }

            // Send the response string to the client
            sendAll(clientSocket, response);


            char  b[256];
            recv(clientSocket, b, 256, 0);
            string x(b);
            vector<string> a = split_string(x,' ');
            int n = stoi(a[0]);
            string destinationPath = a[1];
            activegroups[i]->seeders[filename].push_back(user);
            usersMap[user]->available_chunks[filename].push_back(n);
            usersMap[user]->locatefile[filename] = destinationPath;
            return;
        }
    }

    // If group wasn't found (shouldn't happen due to earlier checks, but for safety)
    send(clientSocket, "Error finding group or file", strlen("Error finding group or file"), 0);
}

void show_downloads(vector<string> command, int clientSocket){
    if (command.size() != 1) {
        // Invalid command
        send(clientSocket, "Invalid command", strlen("Invalid command"), 0);
        return;
    }
    string d = "";
    for (const auto& entry : downloads) {
        const pair<string, string>& key = entry.first; // the pair (key)
        char value = entry.second;                    // the value

        d += value;
        d+= " ";
        d += key.second;
        d+= " ";
        d += key.first;
        d+= "\n";

    }
    send(clientSocket, d.c_str(), d.length(), 0);
}

void handleClient(int clientSocket) {
    cout << "Client connected!" << endl;
    string curr_user;

    char portdetails[256] = {0};
    ssize_t bytesReceived = recv(clientSocket, portdetails, sizeof(portdetails) - 1, 0);  // Leave space for null terminator
    if (bytesReceived < 0) {
        cerr << "Error receiving port details!" << endl;
        return;
    }

    portdetails[bytesReceived] = '\0';  // Ensure null termination of the received string

    string ipport(portdetails);
    vector<string> a= split_string(ipport,':');
    string ip=a[0];
    int port = stoi(a[1]);
    
    while (true) {
        // Receiving data
        char buffer[521000] = {0};
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived < 0) {
            cout << "Error receiving data!" << endl;
            break;
        } else if (bytesReceived == 0) {
            cout << "Client disconnected!" << endl;
            break;
        }

        // Exit if client sends "quit"
        if (strcmp(buffer, "quit") == 0) {
            break;
        } 
        else {
            // Process the received data
            vector<string> command;
            char* word = strtok(buffer, " ");
            while (word != nullptr) {
                command.push_back(string(word));
                word = strtok(nullptr, " ");
            }

            // Check the command
            if (command[0] == "create_user") {
                create_user(command,clientSocket);
            }
            else if(command[0] == "login"){
                login(command , curr_user,clientSocket, ip, port);
            }
            else if(command[0] == "create_group"){
                if(check_user(curr_user,clientSocket)){
                    create_group(command , curr_user,clientSocket);
                }
            }
            else if(command[0] == "join_group"){
                if(check_user(curr_user,clientSocket)){
                    join_group(command , curr_user,clientSocket);
                }
            }
            else if(command[0] == "leave_group"){
                if(check_user(curr_user,clientSocket)){
                    leave_group(command , curr_user,clientSocket);
                }
            }
            else if(command[0] == "list_requests"){
                list_requests(command,clientSocket);
            }
            else if(command[0] == "accept_request"){
                accept_request(command, curr_user,clientSocket);
            }
            else if(command[0] == "list_groups"){
                list_groups(command,clientSocket);
            }
            else if(command[0] == "logout"){
                if(check_user(curr_user,clientSocket)){
                    logout(command , curr_user,clientSocket);
                }
            }
            else if(command[0] == "upload_file"){
                if(check_user(curr_user,clientSocket)){
                    upload_file(command , curr_user,clientSocket);
                }
            }
            else if(command[0] == "list_files"){
                if(check_user(curr_user,clientSocket)){
                    list_files(command , curr_user,clientSocket);
                }
            }
            else if(command[0] == "download_file"){
                if(check_user(curr_user,clientSocket)){
                    download_file(command, curr_user, clientSocket,port);
                }
            }
            else if(command[0] == "show_downloads"){
                if(check_user(curr_user,clientSocket)){
                    show_downloads(command, clientSocket);
                }
            }
            else{
                send(clientSocket,"Invalid command. Please try again.",strlen("Invalid command. Please try again."),0);
                continue;
            }
        }
    }

    // Closing the client socket
    close(clientSocket);
    cout << "Client exited" << endl;
}

void quit(){
    while(true){
        string q;
        cin>>q;
        if(q=="quit")
            exit(0);
    }
}

int main(int argc, char* argv[]) {
    string trackerinfo = argv[1]; // File name containing tracker info
    string trackerno = argv[2];   // User input for tracker number
    int trackerport = -1;         // Initialize tracker port

    // Creating socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Specifying the address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Binding socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Binding failed!" << endl;
        return -1;
    }

    // Listening to the assigned socket
    if (listen(serverSocket, 5) < 0) {
        cerr << "Error while listening!" << endl;
        return -1;
    }

    cout << "Server is listening on port 8080..." << endl;

    thread quitThread(quit);

    while (true) {
        // Accepting connection request
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            cerr << "Failed to accept connection!" << endl;
            continue;
        }

        // Create a new thread to handle the client
        thread clientThread(handleClient, clientSocket);
        clientThread.detach();  // Detach the thread to allow independent execution
    }

    // Closing the server socket (though this line won't be reached in this case)
    close(serverSocket);
    quitThread.join();
    return 0;
}