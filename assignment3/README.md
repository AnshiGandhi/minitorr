# Author : Anshi Gandhi 2024201038

# Date : 03/10/24

# P2P assignment 3

Peer to Peer Distributed File Sharing System

# File Structure

2024201038/
│
├── tracker
│ ├── tracker.app
│ └── tracker
│ └── tracker_info.txt
│ └── globals.h
│ └── globals.cpp
├── client
│ ├── client.cpp
│ ├── client
└── README.md # This file

# Description

# Commands

1. Tracker:
   Run Tracker: ./tracker tracker_info.txt tracker_no
   tracker_info.txt - Contains ip , port details of all the
2. Client:
   Run Client: ./client <IP>:<PORT> tracker_info.txt
   tracker_info.txt - Contains ip, port details of all the trackers
   Create User Account: create_user <user_id> <passwd>
   Login: login <user_id> <passwd>
   Create Group: create_group <group_id>
   Join Group: join_group <group_id>
   Leave Group: leave_group <group_id>
   List pending join: list_requests <group_id>
   Accept Group Joining Request: accept_request <group_id> <user_id>
   List All Group In Network: list_groups

# Assumption

For now, there is only one tracker. Therefore the file tracker_info.txtx is not used yet.
For now, client is not runned using ./client <IP>:<PORT> tracker_info.txt. Instead, it is runned using ./client
In leave gourp , if there are no meembers in group after leaving, the group is deleted.

# Running the program

To run tracker(port 8080), ./tracker tracker_info.txt 1
To run client, ./client
Multiple clients can be runned together.
For each client and tracker, open a different terminal.

g++ tracker.cpp globals.cpp -o tracker -lssl -lcrypto
./tracker tracker_info.txt 1
g++ client.cpp ../tracker/globals.cpp -lssl -lcrypto -o client
./client 127.1.0.2:8082 tracker_info.txt

absolute path

no space in filename

admin not changing after logout

quit in tracker
