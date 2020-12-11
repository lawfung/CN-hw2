#include <iostream>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <fcntl.h>

#define BUFF_SIZE 1024

using namespace std;
bool checkput(string s) {
	stringstream ss;
	ss.clear(); ss.str("");
	ss << s;
	int i;
	for(i = 0; ss >> s; ++ i) {
		if(i == 0 && s != "put") return 0;
		if(i == 2) return 0;
	}
	return i == 2;
}
char RMG[BUFF_SIZE] = {};
int DO_read(int locsoc) {
		int recved;
		bzero(RMG, BUFF_SIZE);
		if ((recved = recv(locsoc, RMG, BUFF_SIZE,0)) < 0){
			cout << "recv failed, with received bytes = " << recved << endl;
			return -1;
		}
		else if (recved == 0){
			cout << "<end>\n";
			return -1;
		}
		return 0;
}
char filebuffer[BUFF_SIZE];
int main(int argc , char *argv[])
{

    int localSocket;
    localSocket = socket(AF_INET , SOCK_STREAM , 0);

    if (localSocket == -1){
        printf("Fail to create a socket.\n");
        return 0;
    }

    struct sockaddr_in info;
    bzero(&info,sizeof(info));

    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr("127.0.0.1");
    int port = atoi(argv[1]);
    info.sin_port = htons(port);


    int err = connect(localSocket,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error\n");
        return 0;
    }
    //char receiveMessage[BUFF_SIZE] = {};
    int result = mkdir("./client_folder", 0777);
    while(1){
    	string command;
		getline(cin, command);
		while(command.back() == ' ') command.pop_back();
		if(command == "ls") {
			write(localSocket, "ls", 2 );
			if(DO_read(localSocket) == -1) break;
			printf("%s", RMG);
		}
		else if(checkput(command)) {
			string filenm = command.substr(4, command.size() - 4);
			string tmps = "./client_folder/" + filenm;
			FILE* tmp_fp = fopen(tmps.c_str(), "r");
			if(tmp_fp == NULL) {
				cout << "The " << filenm << " doesn't exist.\n";
				continue; // I'm not pretty sure
			}
			fseek(tmp_fp, 0L, SEEK_END);
			int _sz = ftell(tmp_fp);
			fseek(tmp_fp, 0L, SEEK_SET);
			command += " " + to_string(_sz);
			write(localSocket, command.c_str(), command.size() );
			if(DO_read(localSocket) == -1) break;
			if(strcmp(RMG, "go ahead") == 0)	cerr << "suceed\n";
			


				

		}
		else {
			cout << "Command not found\n";
		}


    }
    printf("close Socket\n");
    close(localSocket);
    return 0;
}

