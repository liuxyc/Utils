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
#include <vector>
#include <algorithm>
#include <fcntl.h>
#include <string>
#include <fstream>

#include "process.h"

bool canAccess(const char *file_path)
{
  if(access(file_path, R_OK) == 0) {
    return true;
  }
  return false;
}

Procc::Procc(int std_out_fd, int std_err_fd, size_t max_buf_len)
: m_pid(-1)
, m_isInit(false)
, m_stdout_buf(NULL)
, m_stderr_buf(NULL)
, m_std_out_fd(std_out_fd)
, m_std_err_fd(std_err_fd)
, PROC_MAX_STDOUT_BUF(max_buf_len)
, PROC_MAX_STDERR_BUF(max_buf_len)
, m_collector(nullptr)
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
        if(exe_args) {
          delete [] exe_args;
        }
        return false;
    }
    fcntl(m_stdout_pipe_fd[0], F_SETFL, O_NOATIME); 
    fcntl(m_stdout_pipe_fd[1], F_SETFL, O_NOATIME); 
    if(::pipe2(m_stderr_pipe_fd, O_CLOEXEC) < 0)
    {
        printf("stderr pipe create error\n");
        if(exe_args) {
          delete [] exe_args;
        }
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
                ::_exit(-1);
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
  std::string proc_path("/proc/");
  proc_path += std::to_string(pid);
  if (canAccess(proc_path.c_str())) {
    proc_path += "/stat";
    std::ifstream proc_st_file(proc_path);
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

int Procc::_stdread(int pipe_fd, char *buf, size_t &buflen, size_t max_buf_len, char **std_b, bool &is_end) {
    int std_read_len = max_buf_len - buflen;
    int count = ::read(pipe_fd, &buf[buflen], std_read_len); 
    if (count == 0) {
        buf[buflen] = '\0';
        if (std_b != NULL) 
            *std_b = buf;
        is_end = true;
    } else {
        buflen += count;
        if (buflen >= max_buf_len) {
            buflen = 0;
        }
    }
    return 0;
}

int Procc::communicate(char **stdout_b, char **stderr_b, uint32_t timeout, size_t collectNum)
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
    time_t begin_time = time(NULL);
    if(m_collector == nullptr) {
      m_collector.reset(new PerfCollector(m_pid, collectNum));
    }
    else {
      m_collector->setSample(m_pid, collectNum);
    }
    while(1) {
        FD_SET(m_stdout_pipe_fd[0],&readfds);
        FD_SET(m_stderr_pipe_fd[0],&readfds);
        timeout_s.tv_sec = 1;
        timeout_s.tv_usec = 0;
        select_ret = select(maxfds,&readfds,NULL,NULL,&timeout_s);
        if(select_ret > 0){
            if(FD_ISSET(m_stdout_pipe_fd[0], &readfds)){
                _stdread(m_stdout_pipe_fd[0], m_stdout_buf, stdout_len, PROC_MAX_STDOUT_BUF, stdout_b, stdout_end);
            }
            if(FD_ISSET(m_stderr_pipe_fd[0], &readfds)){
                _stdread(m_stderr_pipe_fd[0], m_stderr_buf, stderr_len, PROC_MAX_STDERR_BUF, stderr_b, stderr_end);
            }
            if (stdout_end && stderr_end) {
                break;
            }
        } 
        if(timeout > 0) {
            if(time(NULL) - begin_time >= timeout) {
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
        if(m_collector && collectNum > 0) {
          m_collector->collect();
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

PerfCollector::PerfCollector(pid_t pid, size_t max_sample_num)
: m_pid(pid)
, m_max_sample_num(max_sample_num)
, m_cur_sample_pos(0)
, m_total_sys_cpu(0)
, m_total_proc_cpu(0)
{
  m_cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  m_page_size_kb = sysconf(_SC_PAGESIZE) / 1024;
}

PerfCollector::~PerfCollector() {
}


void PerfCollector::setSample(pid_t pid, size_t num) {
  m_pid = pid;
  m_cur_sample_pos = 0;
  m_max_sample_num = num;
  while (m_sample_data.size() > m_max_sample_num && m_max_sample_num != 0) {
    m_sample_data.pop_front();
  }
  m_sample_data.shrink_to_fit();
}

void PerfCollector::collect()
{
  if (m_start_time == 0) {
    m_start_time = std::time(nullptr);
  }
  float cpu = -1.0;
  uint64_t mem = 0;

  std::string useless;
  std::ifstream sys_stat_file("/proc/stat");

  std::string proc_path("/proc/");
  proc_path += std::to_string(m_pid);

  std::ifstream proc_st_file(proc_path + "/stat");
  std::ifstream proc_stm_file(proc_path + "/statm");

  if (sys_stat_file.is_open() && canAccess(proc_path.c_str()) 
      && proc_st_file.is_open() && proc_stm_file.is_open()) {
    uint64_t sys_total = 0;
    std::string xtime;
    sys_stat_file >> useless;
    for(auto c = 0; c < 10; ++c) {
      sys_stat_file >> xtime;
      sys_total += std::stoll(xtime);
    }
    std::string str_utime, str_stime;
    for (auto c = 0; c < 13; ++c) {
      proc_st_file >> useless;
    }
    proc_st_file >> str_utime;
    proc_st_file >> str_stime;
    uint64_t p_utime = std::stoll(str_utime);
    uint64_t p_stime = std::stoll(str_stime);
    if (sys_total != m_total_sys_cpu) {
      cpu = 100 * m_cpu_count * (p_utime + p_stime - m_total_proc_cpu) / (sys_total - m_total_sys_cpu);
    }
    else {
      cpu = 0;
    }

    std::string str_resident;
    proc_stm_file >> useless;
    proc_stm_file >> str_resident;
    mem = std::stoll(str_resident) * m_page_size_kb;
    m_sample_data.emplace_back(cpu,mem);
    if (m_sample_data.size() > m_max_sample_num) {
      m_sample_data.pop_front();
    }

    m_total_sys_cpu = sys_total;
    m_total_proc_cpu = p_utime + p_stime;
  }
  else {
    printf("get stat file error\n");
  }
  ++m_cur_sample_pos;
}
