#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <thread>
#include <openssl/sha.h>
#include <arpa/inet.h>
#include <fstream>
#include <climits>
#include <iomanip>
#include <fcntl.h>
#include <tuple>
#include <mutex>
#include <sstream>
#include <cerrno>
#include <unordered_map>
#include <algorithm>
#include "../tracker/globals.h"


using namespace std;

// Function to split a string based on a delimiter
vector<string> split_string(const string &str, char delimiter) {
    vector<string> result;
    stringstream ss(str);
    string item;
    while (getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    return result;
}

// Function to implement the rarest piece first strategy
unordered_map<int, tuple<string, string, int, string>> piece_selection_algorithm(
    unordered_map<int, vector<string>> &seeder_pieces,                  // Maps piece number to list of seeders
    unordered_map<string, tuple<string, int, string>> seeder_port,      // Maps seeder to (IP, port, path)
    unordered_map<string, vector<int>> &seeder_available_chunks         // Maps seeder to list of available chunks
) {
    unordered_map<int, tuple<string, string, int, string>> selected_pieces; // Stores piece -> (seeder name, IP, port, path)

    // Get the availability of each piece, sorted by the number of seeders (rarest first)
    vector<pair<int, vector<string>>> piece_availability(seeder_pieces.begin(), seeder_pieces.end());
    sort(piece_availability.begin(), piece_availability.end(),
        [](const pair<int, vector<string>> &a, const pair<int, vector<string>> &b) {
            return a.second.size() < b.second.size(); // Sort by number of seeders (ascending)
        });

    // Loop through each piece, starting with the rarest
    for (const auto &piece_info : piece_availability) {
        int piece_number = piece_info.first;
        vector<string> seeders = piece_info.second;

        if (seeders.empty()) {
            cout << "No seeders available for piece " << piece_number << endl;
            continue;
        }

        // Sort seeders by the number of chunks they have (ascending)
        sort(seeders.begin(), seeders.end(),
            [&seeder_available_chunks](const string &seeder1, const string &seeder2) {
                return seeder_available_chunks[seeder1].size() < seeder_available_chunks[seeder2].size();
            });

        // Select the best seeder (the one with the fewest chunks)
        string selected_seeder = seeders[0];
        string seeder_ip = get<0>(seeder_port[selected_seeder]); // Get IP
        int seeder_port_number = get<1>(seeder_port[selected_seeder]); // Get port
        string seeder_path = get<2>(seeder_port[selected_seeder]); // Get path

        // Store the piece's seeder name, IP, port, and path
        selected_pieces[piece_number] = make_tuple(selected_seeder, seeder_ip, seeder_port_number, seeder_path);
    }

    return selected_pieces;
}

// Thread function to act as a client and connect to a seeder
void connectToSeeder(int piece_number, string ip, int port,string filename,string seeder,string path,vector<string> &ans) {
    int pieceSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (pieceSocket < 0) {
        cerr << "Error creating socket for piece " << piece_number << endl;
        return;
    }

    sockaddr_in seederAddress;
    seederAddress.sin_family = AF_INET;
    seederAddress.sin_port = htons(port);
    seederAddress.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(pieceSocket, (struct sockaddr*)&seederAddress, sizeof(seederAddress)) < 0) {
        cerr << "Failed to connect for piece " << piece_number << " to IP: " << ip << " Port: " << port << endl;
        close(pieceSocket);
        return;
    }

    cout << "Connected for piece " << piece_number << " to IP: " << ip << " Port: " << port << endl;
    
    // Send the request for the piece
    string message = filename;
    message += " ";
    message += to_string(piece_number);
    message += " ";
    message += seeder;
    message += " ";
    message += path;
    send(pieceSocket, message.c_str(), message.length(), 0);

    // // Receiving the piece data (you can modify this for real data)
    // char buffer[521000] = {0};
    // recv(pieceSocket, buffer, sizeof(buffer), 0);
    // // ofstream logfile("debug_log.txt", ios::app);
    // // logfile << "\nReceived data for piece " << piece_number << ": \n" << buffer << endl;
    // // logfile.close();

    // Receiving the piece data (you can modify this for real data)
    unsigned char buffer[521000+1] = {0};
    int totalBytesReceived = 0;
    int bytesReceived;

    while (totalBytesReceived < sizeof(buffer)) {
        bytesReceived = recv(pieceSocket, buffer + totalBytesReceived, sizeof(buffer) - totalBytesReceived, 0);
        if (bytesReceived <= 0) {
            break; // Connection closed or error occurred
        }
        totalBytesReceived += bytesReceived;
    }

    ans[piece_number] = string(reinterpret_cast<char*>(buffer), totalBytesReceived);

    close(pieceSocket);
}

// Server thread function to listen for incoming connections and serve pieces
void servePieces(int serverPort) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cerr << "Error creating server socket" << endl;
        return;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Bind failed on server socket" << endl;
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 10) < 0) {
        cerr << "Listen failed on server socket" << endl;
        close(serverSocket);
        return;
    }

    // cout << "Server listening on port " << serverPort << endl;

    while (true) {
        sockaddr_in clientAddress;
        socklen_t clientAddrLen = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddrLen);

        if (clientSocket < 0) {
            cerr << "Accept failed" << endl;
            continue;
        }

        thread([clientSocket]() {
            char buffer[521000] = {0};
            recv(clientSocket, buffer, sizeof(buffer), 0);
            cout << "Received request: Requesting piece " << buffer << endl;

            vector<string> message = split_string(buffer,' ');
            string filename =  message[0];
            int piece_number = stoi(message[1]);
            string myuser = message[2];
            string fpath = message[3];
            // ofstream logfile("debug_log.txt", ios::app);

            // Debugging the value of myuser and filename
            // logfile << "myuser: " << myuser << ", filename: " << filename << ", filepath: " << fpath << endl;

            // // Simulate sending the requested piece data
            // logfile << "Trying to open file at path: " << fpath << endl;
            // logfile.close();


            int fd = open(fpath.c_str(), O_RDONLY);
            if (fd == -1) {
                cerr << "Error opening file: " << strerror(errno) << endl;
                return;
            }

            off_t offset = (piece_number * 521000); // Position in the file to seek to (50 bytes from the start)

            // Use lseek to move the file pointer to the desired position
            off_t current_offset = lseek(fd, offset, SEEK_SET);
            if (current_offset == -1) {
                cerr << "Error using lseek: " << strerror(errno) << endl;
                close(fd);
                return;
            }

            // Buffer to store the read data
            unsigned char buff[521000 + 1];
            ssize_t bytes_read = read(fd, buff, sizeof(buff) - 1); // Read up to 1499 bytes
            if (bytes_read == -1) {
                cerr << "Error reading file: " << strerror(errno) << endl;
                close(fd);
                return;
            }

            // Null-terminate the buffer to treat it as a string
            buff[bytes_read] = '\0';

            // Close the file descriptor after reading
            close(fd);

            // Loop to ensure all data is sent
            int totalBytesSent = 0;
            int bytesSent = 0;
            int dataSize = bytes_read; // The actual data size read

            while (totalBytesSent < dataSize) {
                bytesSent = send(clientSocket, buff + totalBytesSent, dataSize - totalBytesSent, 0);
                
                if (bytesSent == -1) {
                    cerr << "Error sending data: " << strerror(errno) << endl;
                    close(clientSocket);
                    return;
                }
                
                totalBytesSent += bytesSent;
            }

            // Close the client socket after sending the data
            close(clientSocket);

        }).detach(); // Detach the thread so it can run independently
    }

    close(serverSocket);
}

// Function to calculate SHA1 hash
string calculate_sha1(const string &input) {
    unsigned char hash[SHA_DIGEST_LENGTH]; // SHA1 hash is 20 bytes
    SHA1(reinterpret_cast<const unsigned char *>(input.c_str()), input.length(), hash);

    // Convert the hash to a hex string
    stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

int main(int argc, char* argv[]){
    if (argc != 3) {
        cout<<"Please enter the tracker IP and port number/tracker info fie"<<endl;
        return 0;
    }
    string  ipport = argv[1];
    vector <string> c = split_string(ipport,':');
    string c_ip = c[0];
    int  c_port = stoi(c[1]);
    string tracker_info_file = argv[2];

    // Start the thread to listen to connection request
    thread serverThread(servePieces, c_port);

    // creating socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // specifying address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // sending connection request
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Connection failed!" << endl;
        return -1;
    }

    send(clientSocket, argv[1], strlen(argv[1]), 0);

    cout<<"Connected to server\n";

    while (true) {
        // sending data
        string message;
        cout << "Enter message : ";
        getline(cin, message);

        if(message == "quit"){
            exit(0);
        }

        // Send the message to the server
        if (send(clientSocket, message.c_str(), message.length(), 0) < 0) {
            cerr << "Failed to send message!" << endl;
            break;
        }

        

        char buffer[521000] = {0};
        recv(clientSocket, buffer, sizeof(buffer), 0);
        if(!strstr(buffer,"SHA")){
            cout<<buffer<<flush<<endl;
        }

        

        if(strstr(buffer,"SHA")){
            cout << "Processing download_file response...\n";

            // Receive the download file data from the tracker
            string tracker_response(buffer);

            // Split the tracker response based on the '|' delimiter
            vector<string> response_parts = split_string(tracker_response, '|');
            vector<string> message_parts = split_string(message, ' ');
            string groupid = message_parts[1];
            string filename = message_parts[2];
            string destPath = message_parts[3];

            // Parse the response parts
            string sha = response_parts[0].substr(4);  // SHA:abcdef1234567890
            int numpieces = stoi(response_parts[1].substr(10));  // NUMPIECES:10
            string shapieces = response_parts[2].substr(10);  // SHAPIECES:12345...

            // Parse seeders and their pieces
            string seeders_info = response_parts[3].substr(8);  // SEEDERS:user1#PIECES:0,1,2@user2#PIECES:...
            vector<string> seeders = split_string(seeders_info, '@');  // Split each seeder by '@'

            unordered_map<int,vector<string>>  seeder_pieces;
            unordered_map<string, tuple<string, int, string>> seeder_port;

            if(seeders.size() == 0){
                cout << "No seeders available for this file." << endl;
                continue;
            }
            for (const string &seeder_info : seeders) {
                // Split seeder's info (username#PIECES:0,1,2) by '#'
                vector<string> seeder_parts = split_string(seeder_info, '#');
                string seeder_name = seeder_parts[0];
                string pieces_info = seeder_parts[1].substr(7);  // PIECES:0,1,2
                string ip = seeder_parts[2];
                int port = stoi(seeder_parts[3]);
                string path =  seeder_parts[4];
                vector<string> pieces = split_string(pieces_info, ',');
                for (const string &piece : pieces) {
                    seeder_pieces[stoi(piece)].push_back(seeder_name);
                }
                seeder_port[seeder_name] = {ip, port, path};;
            }

            unordered_map<string, vector<int>> seeder_available_chunks;
            for(int i=0;i<usersLogin.size();i++){
                if(usersMap[usersLogin[i]]->available_chunks[filename].size()!=0)
                    seeder_available_chunks[usersLogin[i]]=usersMap[usersLogin[i]]->available_chunks[filename];
            }

            // piece wise selection
            unordered_map<int, tuple<string, string, int,string>> selected_pieces = piece_selection_algorithm(seeder_pieces, seeder_port, seeder_available_chunks);

            // Print the selected pieces and their assigned IP/port
            for (const auto &piece : selected_pieces) {
                cout << "Piece " << piece.first << " is available at IP: " << get<1>(piece.second) << ", Port: " << get<2>(piece.second) << endl;
            }

            // Create threads for each piece and connect to respective seeders
            vector<thread> threads;

            vector<string> ans(numpieces);

            for (const auto &piece_info : selected_pieces) {
                int piece_number = piece_info.first;
                string ip = get<1>(piece_info.second);
                int port = get<2>(piece_info.second);
                string seeder_uname = get<0>(piece_info.second);
                string path =  get<3>(piece_info.second);

                // Create a thread to handle the connection for this piece
                threads.push_back(thread(connectToSeeder, piece_number, ip, port,filename,seeder_uname,path,ref(ans)));
            }

            // Join all threads (wait for them to finish)
            for (auto &t : threads) {
                if (t.joinable()) {
                    t.join();
                }
            }

            string combined_file_data;

            // Combine all the pieces into one string
            for (const string &piece : ans) {
                combined_file_data += piece;
            }

            // Calculate the SHA1 hash of the entire combined file data
            string file_sha = calculate_sha1(combined_file_data);

            // Compare the calculated SHA with the expected SHA
            if (file_sha != sha) {
                cerr << "SHA1 hash of the downloaded file does not match the expected SHA!" << endl;
                continue; // Exit with error
            }
            else {
                cout<<"SHA verified"<<endl;
            }

            // Calculate piece-wise SHA1 and compare with shapieces
            // string concatenated_piece_sha;
            // for (const string &piece : ans) {
            //     string piece_sha = calculate_sha1(piece);  // Calculate SHA1 for the piece data
            //     concatenated_piece_sha += piece_sha.substr(0, 20);  // Take first 20 characters (SHA1 first 20 bytes in hex)
            // }

            // if (concatenated_piece_sha != shapieces) {
            //     cerr << "Piece-wise SHA1 hash does not match the expected SHAPIECES!" << endl;
            //     continue; // Exit with error
            // }

            int fd = open(destPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (fd < 0) {
                perror("Error opening file");
                continue; // Exit with error code
            }

            for(int i=0;i<ans.size();i++){

                ssize_t bytesWritten = write(fd, ans[i].c_str(), ans[i].length());

                if (bytesWritten < 0) {
                    perror("Error writing to file");
                    close(fd); // Always close the file descriptor
                    continue; // Exit with error code
                }

                string m = to_string(i);
                m+=" ";
                m+=destPath;

                send(clientSocket, m.c_str(), m.length(), 0);
            }

            if (close(fd) < 0) {
                perror("Error closing file");
                continue; // Exit with error code
            }

            downloads[make_pair(groupid,filename)] = 'C';

        }

    }

    // closing socket
    close(clientSocket);

    // Join the server thread (though it runs indefinitely, you can handle graceful shutdowns if needed)
    serverThread.join();
    return 0;
}
