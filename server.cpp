#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>
#include <glob.h>
#include <vector>
#include <sstream>

#define BUFF_SIZE 1024
#define FB_SIZE 40
#define MAXFD 2048
using namespace std;

int connecting[MAXFD];
int transfering[MAXFD]; // 1 : put, 2 : get 
FILE* using_fp[MAXFD];
int using_sz[MAXFD];
string using_filenm[MAXFD];

vector<string> globVector(const string& pattern) {
	glob_t glob_result;
	glob(pattern.c_str(), GLOB_TILDE, NULL, &glob_result);
	vector<string> files;
	for(unsigned i = 0; i < glob_result.gl_pathc; ++ i) {
		files.push_back(string(glob_result.gl_pathv[i]));
	}
	globfree(&glob_result);
	return files;
}
bool checkput(string s, string &name, int &sz) {
	stringstream ss;
	ss.clear(); ss.str("");
	ss << s;
	int i;
	for(i = 0; ss >> s; ++ i) {
		if(i == 0 && s != "put")	return 0;
		if(i == 1) name = s;
		if(i == 2) sz = stoi(s);
		if(i == 3) return 0;
	}
	//cerr << "i=" << i << '\n';
	return i == 3;
}
int checkget(string s, int id) {
	stringstream ss;
	ss.clear(); ss.str("");
	ss << s;
	int ii;
	for(ii = 0; ss >> s; ++ ii) {
		if(ii == 0 && s != "get")	return 0;
		if(ii == 1) using_filenm[id] = s;
		if(ii == 2) return 0;
	}
	if(ii != 2)	return 0;
	using_filenm[id] = "./server_folder/" + using_filenm[id];
	//if(access(s.c_str(), R_OK) != 0) return -1;
	using_fp[id] = fopen(using_filenm[id].c_str(), "r");
	if(using_fp[id] == NULL){
		using_sz[id] = -1;
		return 1;
	}
	fseek(using_fp[id], 0L, SEEK_END);
	using_sz[id] = ftell(using_fp[id]);
	fseek(using_fp[id], 0L, SEEK_SET);
	return 1;

}
int DO_transfer1(int id) {
	char putbuf[FB_SIZE + 1]; memset(putbuf, 0, sizeof putbuf);
	int rdsz = min(using_sz[id], FB_SIZE);
	recv(id, putbuf, rdsz, 0);
	int len = strlen(putbuf);
	if(len == 0)	return -1;
	if(len != rdsz){
		cerr << "This is an error2\n";
	}
	fwrite(putbuf, 1, rdsz, using_fp[id]);
	using_sz[id] -= rdsz;
	if(using_sz[id] == 0) return 2; // read done
	return 1; // usual
}
int DO_transfer2(int id) {
	char putbuf[FB_SIZE + 1]; memset(putbuf, 0, sizeof putbuf);
	int rdsz = min(using_sz[id], FB_SIZE);

	/*
	recv(id, putbuf, rdsz, 0);
	int len = strlen(putbuf);
	if(len == 0)	return -1;
	if(len != rdsz){
		cerr << "This is an error3\n";
	}
	fwrite(putbuf, 1, rdsz, using_fp[id]);
	using_sz[id] -= rdsz;
	if(using_sz[id] == 0) return 2; // read done
	return 1; // usual
	*/
}


int main(int argc, char** argv){

    int localSocket, remoteSocket;                               
    int port = atoi(argv[1]);

    struct  sockaddr_in localAddr,remoteAddr;
          
    int addrLen = sizeof(struct sockaddr_in);  

    localSocket = socket(AF_INET , SOCK_STREAM , 0);
    
    if (localSocket == -1){
        printf("socket() call failed!!");
        return 0;
    }

    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(port);

    char Message[BUFF_SIZE] = {};


        
	if( bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
		printf("Can't bind() socket");
		return 0;
	}
	
	listen(localSocket , 3);
	int maxfd = getdtablesize();
	int result = mkdir("./server_folder", 0777);
	cerr << result << '\n';
	while(1){    
		//std::cout <<  "Waiting for connections...\n"
		//        <<  "Server Port:" << port << std::endl;
			struct timeval svrtimeout;
			fd_set svr_fd_set;
			svrtimeout.tv_sec = svrtimeout.tv_usec = 0;
			FD_ZERO(&svr_fd_set);
			FD_SET(localSocket, &svr_fd_set);
		select(maxfd, &svr_fd_set, NULL, NULL, &svrtimeout);
		if(FD_ISSET(localSocket, &svr_fd_set)) {
			remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);  
			if (remoteSocket < 0) {
				printf("accept failed!");
				return 0;
			}
			std::cout << "Connection accepted" << std::endl;
			connecting[remoteSocket] = 1;
		}
		

		for(int i = 0; i < maxfd; ++ i) {
			if(connecting[i] == 0)	continue;
			if(transfering[i] == 2) {
				int sta = DO_transfer2(i);
				if(sta == -1) {
					connecting[i] = 0;
					transfering[i] = 0;
					fclose(using_fp[i]);
				}
				if(sta == 2) {
					transfering[i] = 0;
					fclose(using_fp[i]);
					//strcpy(Message,"done");
					//send(i ,Message,strlen(Message), 0);
				}
				continue;
			}
				struct timeval timeout;
				fd_set tmp_fd_set;
				timeout.tv_sec = timeout.tv_usec = 0;
				FD_ZERO(&tmp_fd_set);
				FD_SET(i, &tmp_fd_set);
			select(maxfd, &tmp_fd_set, NULL, NULL, &timeout);
			if(FD_ISSET(i, &tmp_fd_set) == false) continue;
			
			if(transfering[i] == 1) {
				int sta = DO_transfer1(i);
				if(sta == -1) {
					connecting[i] = 0;
					transfering[i] = 0;
					fclose(using_fp[i]);
				}
				if(sta == 2) {
					transfering[i] = 0;
					fclose(using_fp[i]);
					strcpy(Message,"done");
					send(i ,Message,strlen(Message), 0);
				}
				continue;
			}

			char tmpstr[FB_SIZE + 1];	memset(tmpstr, 0, sizeof tmpstr);
			read(i, tmpstr, FB_SIZE);
			if(strlen(tmpstr) == 0) {
				connecting[i] = 0;
				transfering[i] = 0;
				continue;
			}
			string ope(tmpstr);
			string _bb; int _cc;
			cout << "This operation is " << ope << '\n';
			if(ope == "ls") {
				string folder_path = "server_folder/*";
				int fol_sz = folder_path.size() - 1;
				//string cmd = "ls -la server_folder";
				//system(cmd.c_str());
				vector<string> files = globVector(folder_path);
				for(int j = 0; j < files.size(); ++ j) {
					files[j] = files[j].substr(fol_sz, files[j].size() - fol_sz);
					files[j].push_back('\n');

				}
				char lsbuf[BUFF_SIZE];	memset(lsbuf, 0, sizeof lsbuf);
				strcpy(lsbuf, "There are :\n" );
				for(int j = 0; j < files.size(); ++ j ) {
					strcpy(lsbuf + strlen(lsbuf), files[j].c_str());
				}
				cout << lsbuf;
				send(i, lsbuf, strlen(lsbuf), 0);
			}
			else if(checkput(ope, using_filenm[i], using_sz[i])) {
				strcpy(Message,"go ahead");
				send(i ,Message,strlen(Message), 0);
				
				using_filenm[i] = "./server_folder/" + using_filenm[i];
				using_fp[i] = fopen(using_filenm[i].c_str(), "w");
				transfering[i] = 1;
			}
			else if(checkget(ope, i) ) {
				strcpy(Message, to_string(using_sz[i]).c_str() );
				send(i ,Message,strlen(Message), 0);

				if(using_sz[i] == -1)	continue;
				recv(i, Message, BUFF_SIZE, 0);
				if(strcmp(Message, "go ahead") == 0) cerr << "suceed\n";
				transfering[i] = 2;
			}
			else {
				cerr << "This is an error\n";
				//int sent;
				//strcpy(Message,"Command not found\n");
				//sent = send(i ,Message,strlen(Message), 0);
				//strcpy(Message,"Computer Networking is interesting!!\n");
				//sent = send(i ,Message,strlen(Message), 0);
			}
		} // endfor
		// close(remoteSocket);
	} // end while
	return 0;
}
