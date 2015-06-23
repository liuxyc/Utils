/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <boost/algorithm/string.hpp>    
#include <boost/filesystem.hpp>    
#include <vector>
#include <algorithm>
#include <fcntl.h>
#include <string>
#include <fstream>

#include "process.h"

Procc::Procc(int std_out_fd, int std_err_fd, size_t max_buf_len)
: m_pid(-1)
, m_isInit(false)
, m_stdout_buf(NULL)
, m_stderr_buf(NULL)
, m_std_out_fd(std_out_fd)
, m_std_err_fd(std_err_fd)
, PROC_MAX_STDOUT_BUF(max_buf_len)
, PROC_MAX_STDERR_BUF(max_buf_len)
{
    m_isInit = true;
    m_stdout_buf = new char[PROC_MAX_STDOUT_BUF];
    m_stderr_buf = new char[PROC_MAX_STDERR_BUF];

}

Procc::~Procc()
{
    delete [] m_stdout_buf;
    delete [] m_stderr_buf;
}

bool Procc::run(const std::string &cmd, bool use_shell, const std::string &cwd)
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
    if(::pipe2(m_stdout_pipe_fd, O_CLOEXEC) < 0)
    {
        printf("stdout pipe create error\n");
        return false;
    }
    fcntl(m_stdout_pipe_fd[0], F_SETFL, O_NOATIME); 
    fcntl(m_stdout_pipe_fd[1], F_SETFL, O_NOATIME); 
    if(::pipe2(m_stderr_pipe_fd, O_CLOEXEC) < 0)
    {
        printf("stderr pipe create error\n");
        return false;
    }
    fcntl(m_stderr_pipe_fd[0], F_SETFL, O_NOATIME); 
    fcntl(m_stderr_pipe_fd[1], F_SETFL, O_NOATIME); 

    m_pid = vfork();
    if (m_pid == 0)//子进程
    {
        ::close(m_stdout_pipe_fd[0]);
        ::close(m_stderr_pipe_fd[0]);
        ::close(STDIN_FILENO);
        if (m_std_out_fd == PROCC_STDOUT_PIPE) {
            if (::dup2(m_stdout_pipe_fd[1], STDOUT_FILENO) < 0) {
                printf("dup2 stdout error\n");
                ::_exit(-1);
            }
        }
        else {
            ::close(m_stdout_pipe_fd[1]);
        }
        if (m_std_err_fd == PROCC_STDERR_PIPE) {
            if (::dup2(m_stderr_pipe_fd[1], STDERR_FILENO) < 0) {
                printf("dup2 stderr error\n");
                ::_exit(-1);
            }
        }
        else {
            ::close(m_stderr_pipe_fd[1]);
        }

        if (m_std_out_fd >= 0) {
            if (::dup2(m_std_out_fd, STDOUT_FILENO) < 0) {
                printf("dup2 stdout error\n");
                ::_exit(-1);
            }
        }
        if (m_std_err_fd >= 0) {
            if (::dup2(m_std_err_fd, STDERR_FILENO) < 0) {
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
    else {
        ::close(m_stdout_pipe_fd[1]);
        ::close(m_stderr_pipe_fd[1]);
        delete [] exe_args;
        return true;
    }
}

int Procc::pid() 
{
    return m_pid;
}

bool Procc::is_alive(pid_t pid)
{
    boost::filesystem::path proc_path = std::string("/proc/") + std::to_string(pid);
    if (boost::filesystem::exists(proc_path)) {
        proc_path /= "stat";
        std::ifstream proc_st_file(proc_path.string());
        if (proc_st_file.is_open()) {
            std::string pid, st_str, st;
            proc_st_file >> pid >> st_str >> st;
            if (st != "Z") {
                return true;
            }
        }
    }
    return false;
}

int Procc::communicate(char **stdout_b, char **stderr_b, uint32_t timeout)
{

    size_t stdout_len = 0;
    size_t stderr_len = 0;
    fd_set readfds;
    int maxfds = 0;
    maxfds = std::max(m_stdout_pipe_fd[0], m_stderr_pipe_fd[0]) + 1;
    struct timeval timeout_s;
    memset(&timeout_s, 0, sizeof(struct timeval));
    int select_ret = 0;
    FD_ZERO(&readfds);
    bool stdout_end = false;
    bool stderr_end = false;
    time_t begin_t = time(NULL);
    while(1) {
        FD_SET(m_stdout_pipe_fd[0],&readfds);
        FD_SET(m_stderr_pipe_fd[0],&readfds);
        timeout_s.tv_sec = 1;
        timeout_s.tv_usec = 0;
        select_ret = select(maxfds,&readfds,NULL,NULL,&timeout_s);
        if(select_ret > 0){
            if(FD_ISSET(m_stdout_pipe_fd[0], &readfds)){
                int stdout_read_len = PROC_MAX_STDOUT_BUF - stdout_len;
                int count = ::read(m_stdout_pipe_fd[0], &m_stdout_buf[stdout_len], stdout_read_len); 
                if (count == 0) {
                    m_stdout_buf[stdout_len] = '\0';
                    if (stdout_b != NULL) 
                        *stdout_b = m_stdout_buf;
                    stdout_end = true;
                } else {
                    stdout_len += count;
                    if (stdout_len >= PROC_MAX_STDOUT_BUF) {
                        stdout_len = 0;
                    }
                }
            }
            if(FD_ISSET(m_stderr_pipe_fd[0], &readfds)){
                int stderr_read_len = PROC_MAX_STDERR_BUF - stderr_len;
                int count = ::read(m_stderr_pipe_fd[0], &m_stderr_buf[stderr_len], stderr_read_len);
                if (count == 0) {
                    m_stderr_buf[stderr_len] = '\0';
                    if (stderr_b != NULL) 
                        *stderr_b = m_stderr_buf;
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
        if(timeout > 0) {
            if(time(NULL) - begin_t >= timeout) {
                m_stdout_buf[stdout_len] = '\0';
                if (stdout_b != NULL) 
                    *stdout_b = m_stdout_buf;
                m_stderr_buf[stderr_len] = '\0';
                if (stderr_b != NULL) 
                    *stderr_b = m_stderr_buf;
                kill(m_pid, 9);
                break;
            }
        }
    }
    int status;
    pid_t ret_pid = ::waitpid(m_pid, &status, 0);
    printf("Procc %d return %d\n", ret_pid, status);
    m_pid = -1;
    return status;
}

int Procc::system(const std::string &cmd) {
    return ::system(cmd.c_str());
}

