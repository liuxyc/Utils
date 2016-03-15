/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef UTIL_PROC_PROCESS_H
#define UTIL_PROC_PROCESS_H

#include <string>
#include <tuple>
#include <vector>
#include <ctime>

const int PROCC_STDOUT_NONE = -1;
const int PROCC_STDOUT_PIPE = -2;
const int PROCC_STDERR_NONE = -1;
const int PROCC_STDERR_PIPE = -2;

class PerfResults;

class Procc
{
    public:

        /**
         * @brief construct Procc
         *
         * @param std_out_fd
         * @param std_err_fd
         * @param max_buf_len init buffer for stdout/stderr data will overlap if larger than buffer
         */
        Procc(int std_out_fd, int std_err_fd, size_t max_buf_len=8*1024);

        ~Procc();
        /**
         * @brief run command, will return if programme start success
         *
         * @param cmd
         * @param use_shell
         * @param cwd
         *
         * @return 
         */
        bool run(const std::string &cmd, bool use_shell, const std::string &cwd);

        /**
         * @brief wait programme stop or timeout, get stdout/stderr back
         *
         * @param stdout_b
         * @param stderr_b
         * @param timeout
         *
         * @return 
         */
        int communicate(char **stdout_b, char **stderr_b, uint32_t timeout=0);

        /**
         * @brief get surrent running process id
         *
         * @return 
         */
        pid_t pid();

        /**
         * @brief return is process alive
         *
         * @param pid specify pid
         *
         * @return 
         */
        static bool is_alive(pid_t pid);

        static int system(const std::string &cmd);

        /* enable/disable process performance data collection
         * data_num   set the collection data number, sample interval is 1 second
         *            will put PerfPoint into vector PerfResults::m_sample_data
         *            >0 enable, =0 disable
         */
        void setCollectPerf(size_t data_num);

    private:
        int m_stdout_pipe_fd[2];
        int m_stderr_pipe_fd[2];
        pid_t m_pid;
        bool m_isInit;
        char *m_stdout_buf;
        char *m_stderr_buf;
        int m_std_out_fd;
        int m_std_err_fd;
        const size_t PROC_MAX_STDOUT_BUF;
        const size_t PROC_MAX_STDERR_BUF;

        int _stdread(int pipe_fd, char *buf, size_t &buflen, size_t max_buf_len, char **std_b, bool &is_end);
        size_t m_collect_num;
        PerfResults *m_collector;
};

class PerfResults
{
  //sample interval is fixed to 1 second
  typedef std::tuple<float, uint64_t> PerfPoint;
  public:
    explicit PerfResults(pid_t pid, size_t max_sample_num);
    ~PerfResults();
    void collect();

    std::time_t m_start_time = 0;
    
    std::vector<PerfPoint> m_sample_data;

    PerfResults(const PerfResults &) = delete;
    PerfResults &operator=(const PerfResults &) = delete;
  private:
    pid_t m_pid;
    size_t m_max_sample_num;
    size_t m_cur_sample_pos;
    uint64_t m_total_sys_cpu;
    uint64_t m_total_proc_cpu;
    long m_cpu_count;
    uint64_t m_page_size_kb;
};

#endif
