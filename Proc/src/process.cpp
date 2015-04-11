#include "process.h"
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string>
#include <boost/algorithm/string.hpp>    
#include <vector>

Procc::Procc()
: isInit(false)
, stdout_buf(NULL)
, stderr_buf(NULL)
, BCLOUD_PROC_MAX_STDOUT_BUF(1024*1024*64)
, BCLOUD_PROC_MAX_STDERR_BUF(1024*1024*64)
{
    if(pipe(stdout_pipe_fd)<0)
    {
        printf("cmd pipe create error\n");
        return;
    }
    if(pipe(stderr_pipe_fd)<0)
    {
        printf("ret pipe create error\n");
        return;
    }
    isInit = true;
    stdout_buf = new char[BCLOUD_PROC_MAX_STDOUT_BUF];
    stderr_buf = new char[BCLOUD_PROC_MAX_STDERR_BUF];

}

Procc::~Procc()
{
    delete [] stdout_buf;
    delete [] stderr_buf;
}

bool Procc::run(const std::string &cmd, bool use_shell, const std::string &cwd)
{
    char **exe_args = NULL;
    char exe_file[256] = {NULL};
    std::vector<std::string> vec_cmd_args;
    if (use_shell) {
        strcpy(exe_file, "/bin/sh");
        exe_args = new char*[4];
        exe_args[0] = static_cast<char *>("/bin/sh");
        exe_args[1] = static_cast<char *>("-c");
        exe_args[2] = (char *)cmd.c_str();
        exe_args[3] = NULL;
    }
    else {
        boost::split(vec_cmd_args, cmd, boost::is_any_of(" "));
        strcpy(exe_file, vec_cmd_args[0].c_str());
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

    pid = vfork();
    if (pid == 0)//子进程
    {
        printf("cmd sub process started\n");
        ::close(stdout_pipe_fd[0]);
        ::close(stderr_pipe_fd[0]);
        ::dup2(stdout_pipe_fd[1], 1);
        ::dup2(stderr_pipe_fd[1], 2);
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
    delete [] exe_args;
    return true;
}

int Procc::communicate(char **stdout_b, char **stderr_b)
{
    int status;
    
    pid_t ret_pid = ::waitpid(pid, &status, 0);
    fd_set readfds;
    int maxfds;
    struct timeval timeout;
    int select_ret;
    FD_ZERO(&readfds);
    FD_SET(stdout_pipe_fd[0],&readfds);
    FD_SET(stderr_pipe_fd[0],&readfds);
    maxfds = stderr_pipe_fd[0] + 1;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;
    select_ret = select(maxfds,&readfds,NULL,NULL,&timeout);
    if(select_ret > 0){
        if(FD_ISSET(stdout_pipe_fd[0], &readfds)){
            int count = ::read(stdout_pipe_fd[0], stdout_buf, BCLOUD_PROC_MAX_STDOUT_BUF - 1); 
            if (count > 0) {
                stdout_buf[count] = '\0';
                *stdout_b = stdout_buf;
            } else {
                printf("stdout IO Error\n");
            }
        }
        if(FD_ISSET(stderr_pipe_fd[0], &readfds)){
            int count = ::read(stderr_pipe_fd[0], stderr_buf, BCLOUD_PROC_MAX_STDERR_BUF - 1);
            if (count > 0) {
                stderr_buf[count] = '\0';
                *stderr_b = stderr_buf;
            } else {
                printf("stderr IO Error\n");
            }
        }
    } 
    return status;
}
