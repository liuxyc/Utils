#include <iostream>
#include <unistd.h>
#include <vector>
#include "process.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>
#include <utility>


int main(int argc, char *argv[])
{
    PerfResults pppa(getpid(), 1);
    for(auto i = 0; i < 3; ++i) {
      pppa.collect();
      for(auto &pd: pppa.m_sample_data) {
        printf("PerfResults test cpu:%f RES:%lu\n", std::get<0>(pd), std::get<1>(pd));
      }
      printf("------\n");
      sleep(1);
    }
    
    Procc pp(PROCC_STDOUT_PIPE, PROCC_STDERR_PIPE);
    std::vector<const char *> vec_cmd;
    vec_cmd.push_back("/sbin/ifconfig");
    vec_cmd.push_back("/sbin/ifconfig -a");
    vec_cmd.push_back("/bin/ls -a -l");
    vec_cmd.push_back("svn indo");
    vec_cmd.push_back("/bin/notexists -a -l");
    vec_cmd.push_back("/usr/bin/python -u sleep.py");
    vec_cmd.push_back("/usr/bin/python -u busy.py");
    size_t collect_size = 0;
    for (auto it = vec_cmd.begin(); it != vec_cmd.end(); ++it) {
        pp.run(*it, false, "./");
        char *stdoutbuf = NULL;
        char *stderrbuf = NULL; 
        int ret = pp.communicate(&stdoutbuf, &stderrbuf, 5, collect_size++);
        if ( ret == 9) {
            printf("call %s timeout\n", *it);
        }

        if ( ret < 0) {
            printf("call %s error\n", *it);
        }
        printf("stdout %s\n", stdoutbuf);
        printf("stderr %s\n", stderrbuf);
        const PerfResults *pr = pp.getCollector();
        for(auto &pd: pr->m_sample_data) {
          printf("PerfResults test cpu:%f RES:%lu\n", std::get<0>(pd), std::get<1>(pd));
        }
        printf("------\n");
    }

    std::vector<const char *> vec_cmd_shell;
    vec_cmd_shell.push_back("cd Util; /sbin/ifconfig");
    vec_cmd_shell.push_back("cd ~/ && /sbin/ifconfig -a");
    vec_cmd_shell.push_back("ls");
    vec_cmd_shell.push_back("svn indo");
    vec_cmd_shell.push_back("ls -a -l");
    vec_cmd_shell.push_back("unkonw -a -l");
    vec_cmd_shell.push_back("python ./largeout.py");
    collect_size = 10;
    for (auto it = vec_cmd_shell.begin(); it != vec_cmd_shell.end(); ++it) {
        pp.run(*it, true, "");
        char *stdoutbuf = NULL;
        char *stderrbuf = NULL;
        int ret = pp.communicate(&stdoutbuf, &stderrbuf, 2, collect_size--);
        if ( ret == 9) {
            printf("call %s timeout\n", *it);
        }

        if ( ret < 0) {
            printf("call %s error\n", *it);
        }
        printf("stdout %s\n", stdoutbuf);
        printf("stderr %s\n", stderrbuf);
        const PerfResults *pr = pp.getCollector();
        for(auto &pd: pr->m_sample_data) {
          printf("PerfResults test cpu:%f RES:%lu\n", std::get<0>(pd), std::get<1>(pd));
        }
        printf("------\n");
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

    pp.run("sleep 5", false, "");
    while(pp.is_alive(pp.pid())) {
        printf("%d alive\n", pp.pid());
        sleep(1);
    }
    pp.communicate(NULL, NULL);


    time_t begin_t = time(NULL);
    pp.run("sleep 1", false, "");
    pp.communicate(NULL, NULL, 5);
    assert(time(NULL) - begin_t == 1);

    assert(Procc::system("echo \"hello\"") == 0);
    assert(Procc::system("exit 15") == 3840);
    return 0;
    
}
