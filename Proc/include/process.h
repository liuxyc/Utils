#include <string>

class Procc
{
    public:
        Procc();
        ~Procc();
        bool run(const std::string &cmd, bool use_shell, const std::string &cwd);
        int communicate(char **stdout_b, char **stderr_b);

    private:
        int stdout_pipe_fd[2];
        int stderr_pipe_fd[2];
        pid_t pid;
        bool isInit;
        char *stdout_buf;
        char *stderr_buf;
        const size_t BCLOUD_PROC_MAX_STDOUT_BUF;
        const size_t BCLOUD_PROC_MAX_STDERR_BUF;
};

