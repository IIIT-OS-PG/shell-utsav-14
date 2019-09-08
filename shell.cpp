#include<unistd.h>
#include<iostream>
#include<sys/wait.h>
#include<cstring>
#include<string>
#include<sstream>
#include<vector>
#include<sys/types.h>
#include<sys/stat.h>
#include<fstream>
#include<stdlib.h>
#include<fcntl.h>
#include<unordered_map>
#define MAXLINE 256
#define MAXARGS 10
#define MAXCOMMANDS 10
using namespace std;

unordered_map<string, vector<string>> alias_map;

void execute_pipes(vector<vector<string>>);
void clear_screen();
vector<string> setup_env();
void print_prompt(vector<string>);
void start_shell();
char* take_input();
vector<vector<string>> set_up_args(char [], string);
void execute_single_shell_command(vector<string>);
void execute_redirect(bool, vector<string>, string);


int main(){

    pid_t shell = fork();
    if(shell < 0){
        perror("Error while forking");
        exit(1);
    }
    if(shell == 0){
        //Terminal starting as main's child
        clear_screen();
        start_shell();
    }
    else{
        //Waiting for shell to finish
        wait(NULL);
    }    
    return 0;
}

void start_shell(){
    //Start a REPL
    //take input->parse->execute->print output->loop
    vector<string> vars = setup_env();
    print_prompt(vars);
    char* buff = take_input();
    while(buff){
        vector<vector<string>> args = set_up_args(buff, "|");
        int commands_count = args.size();
        if(commands_count > 1){
            execute_pipes(args);
        }else{
             if(args[0][0] == "exit"){
                exit(0);
             } 
             if(args[0][0] == "cd"){
                if(chdir(&args[0][1][0])==0){
                    print_prompt(vars);
                    buff = take_input();
                    continue;
                }else{
                    perror("Couldn't change directory");
                }
            }else if(args[0][0] == "alias"){
              string key = args[0][1];
              vector<string> val(args[0].begin() + 3, args[0].end());
              if(alias_map.find(key) == alias_map.end()){
                alias_map.insert(make_pair(key, val));
              }
            }else if(alias_map.find(args[0][0]) != alias_map.end()){
              //using alias
              auto it = alias_map.find(args[0][0]);
              vector<string> expansion = it->second;
              expansion.insert(expansion.end(), args[0].begin() + 1, args[0].end());
              args[0] = expansion;
              execute_single_shell_command(args[0]);
            }else{
                //single shell command, may contain redirects
                bool isRedirect = false, isAppend = false;
                vector<vector<string>> args2 = set_up_args(buff, ">>");
                if(args2.size() > 1){
                  isRedirect = true;
                  isAppend = true;
                }else {
                  args2 = set_up_args(buff, ">");
                  if(args2.size() > 1){
                    isRedirect = true;
                  }
                }
                if(isRedirect){
                  execute_redirect(isAppend, args2[0], args2[1][0]);
                }else{
                  execute_single_shell_command(args[0]);
                }
            } 
        }
        print_prompt(vars);
        buff = take_input();
    } 
}

void execute_redirect(bool isAppend, vector<string> args, string dest){
    // cout << isAppend << " " << dest << endl;
    // for(string s : args){
    //   cout << s << " ";
    // }
    // cout << endl;
    int fd_dest;
    if(isAppend){
      fd_dest = open(&dest[0], O_WRONLY | O_APPEND | O_CREAT);
    }else{
      fd_dest = open(&dest[0], O_WRONLY | O_TRUNC | O_CREAT);
    }
    int fd[2];
    pid_t child_id;
    pipe(fd);
    if ((child_id = fork()) == -1) {
      perror("fork");
      exit(1);
    }
    else if (child_id == 0) {
      close(fd[0]);
      dup2(fd_dest, 1);
      close(fd[1]);
      char* comm_i[MAXARGS];
      for(int i = 0; i < args.size(); ++i){
          comm_i[i] = &args[i][0];
      }
      comm_i[args.size()] = NULL;
      execvp(&args[0][0], comm_i);
      perror("Error while executing the command");
      exit(1);
    }
    else {
      wait(NULL);
      close(fd[1]);
    }
}

void execute_single_shell_command(vector<string> command){
  pid_t pid = fork();
  if(pid < 0){
    perror("Error while forking");
    exit(1);
  }
  if(pid == 0){
    char* comm_i[MAXARGS];
    for(int i = 0; i < command.size(); ++i){
        comm_i[i] = &command[i][0];
    }
    comm_i[command.size()] = NULL;
    execvp(&command[0][0], comm_i);
    perror("Error while executing the command");
    exit(1);
  }else{
    wait(NULL);
  }
}

void execute_pipes(vector<vector<string>> arg){
  int fd[2];
  pid_t child_id;
  int fd_temp = 0, size = arg.size();
  for (int i = 0; i < size; ++i) {
    pipe(fd);
    if ((child_id = fork()) == -1) {
      perror("fork");
      exit(1);
    }
    else if (child_id == 0) {
      dup2(fd_temp, 0);
      close(fd[0]);
      if (i < size - 1) {
        dup2(fd[1], 1);
      }
      close(fd[1]);
      char* comm_i[MAXARGS];
      for(int j = 0; j < arg[i].size(); ++j){
          comm_i[j] = &arg[i][j][0];
      }
      comm_i[arg[i].size()] = NULL;
      execvp(&arg[i][0][0], comm_i);
      perror("Error while executing the command");
      exit(1);
    }
    else {
      wait(NULL);
      close(fd[1]);
      fd_temp = fd[0];
    }
  }
}

void clear_screen(){
  pid_t pid;
  if((pid = fork()) < 0){
    perror("error creating child process");
  }else if(!pid){
    execlp("clear", "clear", NULL);
    perror("Error while clearing the screen");
    exit(1);
  }else{
      wait(NULL);
  }
}
char* take_input(){
    char *buff = (char*)calloc(MAXLINE, sizeof(char));
    fgets(buff, MAXLINE, stdin);
    if(buff){
        if(buff[strlen(buff) - 1] == '\n'){
            buff[strlen(buff) - 1] = '\0';
        }
    }
    return buff;
}

vector<vector<string>> set_up_args(char buff[], string delim){
  stringstream iss(buff);
  string word;
  vector<vector<string>> args;
  int pipe_count = 0;
  vector<string> arg;
  while(iss >> word){
    if(word == delim){
      args.push_back(arg);
      arg.clear();
    }else{
      arg.push_back(word);
    }
  }
  args.push_back(arg);
  return args;
}

vector<string> setup_env(){
    vector<string> vars(3); //0 - user_name, 1 - host_name, 2 - prompt_sym
    string user_name, prompt_sym;
    fstream file_io;
    file_io.open(".myrc", ios :: out);
    int uid = getuid();
    char host[100];
    gethostname(host, 100);
    string host_name(host);
    user_name = getenv("USER"); 
    if(!uid){ // Root
      file_io << user_name << endl;
      file_io << host_name << endl;
      prompt_sym = "%";
      file_io << prompt_sym << endl;
    }else{
      file_io << user_name << endl;
      file_io << host_name << endl;
      prompt_sym = ">";
      file_io << prompt_sym << endl;
    }
    gethostname(host, 100);
    vars[0] = user_name;
    vars[1] = host_name;
    vars[2] = prompt_sym;
    return vars;
}

void print_prompt(vector<string> vars){
    int size = 200;
    char curr_dir[size];
    getcwd(curr_dir, size);
    cout << vars[0] << "@" << vars[1] << ":~" << curr_dir << vars[2] << " ";
}