#include<unistd.h>
#include<iostream>
#include<sys/wait.h>
#include<cstring>
#include<string>
#include<sstream>
#include<vector>
#define MAXLINE 256

using namespace std;

int main(){
  char buff[MAXLINE];
  pid_t pid;
  int status;
  cout << "%";
  while(fgets(buff, MAXLINE, stdin) != NULL){
    if(buff[strlen(buff) - 1] == '\n'){
      buff[strlen(buff) - 1] = '\0';
    }
    if((pid = fork()) < 0){
      cout << "fork error";
      exit(1);
    }
    if(pid == 0){
    stringstream iss(buff);  
    vector<string> words;
    string word;
    while(iss >> word){
      words.push_back(word);
    }
    char * args[words.size() + 1];
    for(int i = 0; i < words.size(); ++i){
      args[i] = &words[i][0];
    }
     args[words.size()] = NULL;
     execvp(args[0], args);   
     cout << "couldn't execute " << buff;
     exit(127);
    }
    if((pid = waitpid(pid, &status, 0)) < 0){
      cout << "waitpid error";
    }
    cout << "%";
  }  
  return 0;
}
