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
#define MAXFD 2048

int connecting[MAXFD];

using namespace std;
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
	// cerr << "here\n";
	if(FD_ISSET(localSocket, &svr_fd_set)) {
        	remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);  
		if (remoteSocket < 0) {
		    printf("accept failed!");
		    return 0;
		}
		std::cout << "Connection accepted" << std::endl;
		connecting[remoteSocket] = 1;
	}
	// cerr << "here " << maxfd << '\n';
	

	for(int i = 0; i < maxfd; ++ i) {
		if(connecting[i] == 0)	continue;
		// cerr << "here2 " << remoteSocket << '\n';
		struct timeval timeout;
		fd_set tmp_fd_set;
		timeout.tv_sec = timeout.tv_usec = 0;
		FD_ZERO(&tmp_fd_set);
		FD_SET(i, &tmp_fd_set);
		select(maxfd, &tmp_fd_set, NULL, NULL, &timeout);
		if(FD_ISSET(i, &tmp_fd_set) == false) continue;
		char tmpstr[100];
		memset(tmpstr, 0, sizeof tmpstr);
		read(i, tmpstr, 99);
		if(strlen(tmpstr) == 0) {
			connecting[i] = 0;
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
			char lsbuf[1024];	memset(lsbuf, 0, sizeof lsbuf);
			for(int j = 0; j < files.size(); ++ j ) {
				strcpy(lsbuf + strlen(lsbuf), files[j].c_str());
			}
			cout << lsbuf;
			send(i, lsbuf, strlen(lsbuf), 0);
		}
		else if(checkput(ope, _bb, _cc)) {
			string filenm = _bb;
			int sz = _cc;
			strcpy(Message,"go ahead");
			send(i ,Message,strlen(Message), 0);
			cerr << filenm << ' ' << sz << '\n';
			
		}
		else {
			cerr << "This is an error\n";
			//int sent;
			//strcpy(Message,"Command not found\n");
			//sent = send(i ,Message,strlen(Message), 0);
			//strcpy(Message,"Computer Networking is interesting!!\n");
			//sent = send(i ,Message,strlen(Message), 0);
		}

	}
        // close(remoteSocket);
    }
    return 0;
}
