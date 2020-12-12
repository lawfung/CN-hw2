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
#define FB_SIZE 40

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
bool checkget(string s) {
	stringstream ss;
	ss.clear(); ss.str("");
	ss << s;
	int i;
	for(i = 0; ss >> s; ++ i) {
		if(i == 0 && s != "get") return 0;
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
			stringstream ss; ss.str(""); ss.clear();
			string filenm;
			ss << command; ss >> filenm; ss >> filenm;
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
			char putbuf[FB_SIZE + 1];
			cerr << _sz << '\n';
			while(_sz > 0) {
				int rdsz = min(_sz, FB_SIZE);
				fread(putbuf, 1, rdsz, tmp_fp);
				//cerr << putbuf << ' ' << rdsz << '\n';
				send(localSocket, putbuf, rdsz, 0);
				_sz -= rdsz;
			}
			if(DO_read(localSocket) == -1) break;
			if(strcmp(RMG, "done") == 0)	cerr << "suceed2\n";
		}
		else if(checkget(command)) {
			write(localSocket, command.c_str(), command.size());
			if(DO_read(localSocket) == -1) break;
			int sz = atoi(RMG);
			cerr << "sz=" << sz << '\n';
			stringstream ss; ss.str(""); ss.clear();
			string filenm;
			ss << command; ss >> filenm; ss >> filenm;
			if(sz == -1) {
				cout << "The " << filenm << "doesn't exist.\n";
				continue;
			}
			string sendb; sendb = "go ahead";
			write(localSocket, sendb.c_str(), sendb.size());
			
			filenm = "./client_folder/" + filenm;
			FILE* tmp_fp = fopen(filenm.c_str(), "w");

			char putbuf[FB_SIZE + 1];
			int cnt = 0;
			while(sz > 0) {
				++ cnt;
				int rdsz = min(sz, FB_SIZE);
				int len = recv(localSocket, putbuf, rdsz, 0);
				//cerr << len << ' ' << cnt << '\n';
				/*
				if(len != rdsz){
					cerr << "This is error 2\n";
					return 0;
				}
				*/
				fwrite(putbuf, 1, len, tmp_fp);
				sz -= len;
			}
			fclose(tmp_fp);
			cerr << "get done\n";
		}
		else {
			cout << "Command not found\n";
		}


    }
    printf("close Socket\n");
    close(localSocket);
    return 0;
}

