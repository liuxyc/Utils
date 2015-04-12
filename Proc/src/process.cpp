#include "process.h"
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <boost/algorithm/string.hpp>    
#include <vector>
#include <algorithm>
#include <fcntl.h>

Procc::Procc()
: isInit(false)
, stdout_buf(NULL)
, stderr_buf(NULL)
, PROC_MAX_STDOUT_BUF(1024*1024*64)
, PROC_MAX_STDERR_BUF(1024*1024*64)
{
    isInit = true;
    stdout_buf = new char[PROC_MAX_STDOUT_BUF];
    stderr_buf = new char[PROC_MAX_STDERR_BUF];

}

Procc::~Procc()
{
    delete [] stdout_buf;
    delete [] stderr_buf;
}

bool Procc::system(const std::string &cmd, const std::string &cwd) 
{
    return run(cmd, false, true, cwd);
}

bool Procc::run(const std::string &cmd, bool need_out, bool use_shell, const std::string &cwd)
{
    char **exe_args = NULL;
    const char *exe_file = NULL;
    std::vector<std::string> vec_cmd_args;
    if (use_shell) {
        exe_file = "/bin/sh";
        exe_args = new char*[4];
        exe_args[0] = (char *)"/bin/sh";
        exe_args[1] = (char *)"-c";
        exe_args[2] = (char *)cmd.c_str();
        exe_args[3] = NULL;
    }
    else {
        boost::split(vec_cmd_args, cmd, boost::is_any_of(" "));
        exe_file = vec_cmd_args[0].c_str();
        if (vec_cmd_args.size() > 0) {
            exe_args = new char*[vec_cmd_args.size() + 1];
            for (size_t i = 0; i < vec_cmd_args.size(); ++i) {
                exe_args[i] = (char *)(vec_cmd_args[i].c_str());
            }
            exe_args[vec_cmd_args.size()] = NULL;


        }
        else {
            exe_args = new char*[2];
            exe_args[0] = (char *)(vec_cmd_args[0].c_str());
            exe_args[1] = NULL;
        }
    }
    if(::pipe2(stdout_pipe_fd, O_CLOEXEC) < 0)
    {
        printf("stdout pipe create error\n");
        return false;
    }
    if(::pipe2(stderr_pipe_fd, O_CLOEXEC) < 0)
    {
        printf("stderr pipe create error\n");
        return false;
    }

    pid = vfork();
    if (pid == 0)//子进程
    {
        printf("cmd sub process started\n");
        ::close(stdout_pipe_fd[0]);
        ::close(stderr_pipe_fd[0]);
        ::close(STDIN_FILENO);
        if (need_out) {
            if (::dup2(stdout_pipe_fd[1], STDOUT_FILENO) < 0) {
                printf("dup2 stdout error\n");
                ::_exit(-1);
            }
            if (::dup2(stderr_pipe_fd[1], STDERR_FILENO) < 0) {
                printf("dup2 stderr error\n");
                ::_exit(-1);
            }
        }
        if (cwd != "") {
            int ret = chdir(cwd.c_str());
            if (ret != 0) {
                printf("chdir to %s error \n", cwd.c_str());
                return false;
            }
        }

        ::execvp(exe_file, exe_args);

        //NOTICE:如果execvp失败，这里不调_exit()会惨死哦
        ::_exit(-1);
    }
    ::close(stdout_pipe_fd[1]);
    ::close(stderr_pipe_fd[1]);
    delete [] exe_args;
    return true;
}

int Procc::communicate(char **stdout_b, char **stderr_b)
{

    memset(stdout_buf, 0, PROC_MAX_STDOUT_BUF);
    memset(stderr_buf, 0, PROC_MAX_STDERR_BUF);

    size_t stdout_len = 0;
    size_t stderr_len = 0;
    fd_set readfds;
    int maxfds = 0;
    maxfds = std::max(stdout_pipe_fd[0], stderr_pipe_fd[0]) + 1;
    struct timeval timeout;
    memset(&timeout, 0, sizeof(struct timeval));
    int select_ret = 0;
    FD_ZERO(&readfds);
    bool stdout_end = false;
    bool stderr_end = false;
    while(1) {
        FD_SET(stdout_pipe_fd[0],&readfds);
        FD_SET(stderr_pipe_fd[0],&readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 1;
        select_ret = select(maxfds,&readfds,NULL,NULL,&timeout);
        if(select_ret > 0){
            if(FD_ISSET(stdout_pipe_fd[0], &readfds)){
                int count = ::read(stdout_pipe_fd[0], (void *)&(stdout_buf[stdout_len]), 4 * 1024); 
                if (count == 0) {
                    stdout_buf[stdout_len] = '\0';
                    *stdout_b = stdout_buf;
                    stdout_end = true;
                } else {
                    stdout_len += count;
                    if (stdout_len >= PROC_MAX_STDOUT_BUF) {
                        stdout_len = 0;
                    }
                }
            }
            if(FD_ISSET(stderr_pipe_fd[0], &readfds)){
                int count = ::read(stderr_pipe_fd[0], (void *)&(stderr_buf[stderr_len]), 4 * 1024);
                if (count == 0) {
                    stderr_buf[stderr_len] = '\0';
                    *stderr_b = stderr_buf;
                    stderr_end = true;
                } else {
                    stderr_len += count;
                    if (stderr_len >= PROC_MAX_STDERR_BUF) {
                        stderr_len = 0;
                    }
                }
            }
            if (stdout_end && stderr_end) {
                break;
            }
        } 
        else {
        }
    }
    int status;
    pid_t ret_pid = ::waitpid(pid, &status, 0);
    printf("Procc %d return\n", ret_pid);
    return status;
}
