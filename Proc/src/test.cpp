#include <iostream>
#include <unistd.h>
#include <vector>
#include "process.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char *argv[])
{
    
    Procc pp(PROCC_STDOUT_PIPE, PROCC_STDERR_PIPE);
    std::vector<const char *> vec_cmd;
    vec_cmd.push_back("/sbin/ifconfig");
    vec_cmd.push_back("/sbin/ifconfig -a");
    vec_cmd.push_back("/bin/ls -a -l");
    vec_cmd.push_back("svn indo");
    vec_cmd.push_back("/bin/notexists -a -l");
    vec_cmd.push_back("/usr/bin/python -u sleep.py");
    for (auto it = vec_cmd.begin(); it != vec_cmd.end(); ++it) {
        pp.run(*it, false, "./");
        char *stdoutbuf = NULL;
        char *stderrbuf = NULL; 
        int ret = pp.communicate(&stdoutbuf, &stderrbuf, 3);
        if ( ret == 9) {
            printf("call %s timeout\n", *it);
        }

        if ( ret < 0) {
            printf("call %s error\n", *it);
        }
        printf("stdout %s\n", stdoutbuf);
        printf("stderr %s\n", stderrbuf);
    }

    std::vector<const char *> vec_cmd_shell;
    vec_cmd_shell.push_back("cd Util; /sbin/ifconfig");
    vec_cmd_shell.push_back("cd ~/ && /sbin/ifconfig -a");
    vec_cmd_shell.push_back("ls");
    vec_cmd_shell.push_back("svn indo");
    vec_cmd_shell.push_back("ls -a -l");
    vec_cmd_shell.push_back("unkonw -a -l");
    vec_cmd_shell.push_back("python ./largeout.py");
    for (auto it = vec_cmd_shell.begin(); it != vec_cmd_shell.end(); ++it) {
        pp.run(*it, true, "");
        char *stdoutbuf = NULL;
        char *stderrbuf = NULL;
        int ret = pp.communicate(&stdoutbuf, &stderrbuf);
        if ( ret == 9) {
            printf("call %s timeout\n", *it);
        }

        if ( ret < 0) {
            printf("call %s error\n", *it);
        }
        printf("stdout %s\n", stdoutbuf);
        printf("stderr %s\n", stderrbuf);
    }

    Procc pp_none(PROCC_STDOUT_NONE, PROCC_STDERR_NONE);
    std::vector<const char *> vec_cmdn;
    vec_cmdn.push_back("/sbin/ifconfig");
    vec_cmdn.push_back("/sbin/ifconfig -a");
    vec_cmdn.push_back("/bin/ls -a -l");
    vec_cmdn.push_back("svn indo");
    vec_cmdn.push_back("/bin/notexists -a -l");
    for (auto it = vec_cmdn.begin(); it != vec_cmdn.end(); ++it) {
        pp_none.run(*it, false, "./include");
        int ret = 0;
        if ( (ret = pp_none.communicate(NULL, NULL)) != 0) {
            printf("call %s error \n", *it);
        }
    }
    for (auto it = vec_cmdn.begin(); it != vec_cmdn.end(); ++it) {
        pp_none.run(*it, true, "./include");
        int ret = 0;
        if ( (ret = pp_none.communicate(NULL, NULL)) != 0) {
            printf("call %s error \n", *it);
        }
    }

    int out_fd = open("./outout", O_WRONLY|O_CREAT, 0644);

    Procc pp_fd(out_fd, out_fd);
    for (auto it = vec_cmdn.begin(); it != vec_cmdn.end(); ++it) {
        pp_fd.run(*it, false, "./include");
        int ret = 0;
        if ( (ret = pp_fd.communicate(NULL, NULL)) != 0) {
            printf("call %s error \n", *it);
        }
    }

    int err_fd = open("./erer", O_WRONLY|O_CREAT, 0644);
    Procc pp_errfd(PROCC_STDOUT_NONE, err_fd);
    for (auto it = vec_cmdn.begin(); it != vec_cmdn.end(); ++it) {
        pp_errfd.run(*it, false, "./include");
        int ret = 0;
        if ( (ret = pp_errfd.communicate(NULL, NULL)) != 0) {
            printf("call %s error \n", *it);
        }
    }

    printf("ret %d\n", Procc::system("echo \"hello\""));
    printf("ret %d\n", Procc::system("exit 15"));
    return 0;
    
}
