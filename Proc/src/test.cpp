#include <iostream>
#include <unistd.h>
#include <vector>
#include "process.h"

int main(int argc, char *argv[])
{
    Procc pp;
    std::vector<const char *> vec_cmd;
    vec_cmd.push_back("/sbin/ifconfig");
    vec_cmd.push_back("/sbin/ifconfig -a");
    vec_cmd.push_back("/bin/ls -a -l");
    vec_cmd.push_back("/bin/notexists -a -l");
    for (auto it = vec_cmd.begin(); it != vec_cmd.end(); ++it) {
        pp.run(*it, false, "./include");
        char *stdoutbuf = NULL;
        char *stderrbuf = NULL;
        int ret = -1;
        if ( (ret = pp.communicate(&stdoutbuf, &stderrbuf)) != 0) {
            printf("call %s error \n", *it);
        }
        printf("stdout %s\n", stdoutbuf);
        printf("stderr %s\n", stderrbuf);
    }

    std::vector<const char *> vec_cmd_shell;
    vec_cmd_shell.push_back("cd Util; /sbin/ifconfig");
    vec_cmd_shell.push_back("cd ~/ && /sbin/ifconfig -a");
    vec_cmd_shell.push_back("ls");
    vec_cmd_shell.push_back("ls -a -l");
    vec_cmd_shell.push_back("unkonw -a -l");
    for (auto it = vec_cmd_shell.begin(); it != vec_cmd_shell.end(); ++it) {
        pp.run(*it, true, "");
        char *stdoutbuf = NULL;
        char *stderrbuf = NULL;
        int ret = -1;
        if ( (ret = pp.communicate(&stdoutbuf, &stderrbuf)) != 0) {
            printf("call %s error \n", *it);
        }
        printf("stdout %s\n", stdoutbuf);
        printf("stderr %s\n", stderrbuf);
    }
    
    return 0;
    
}
