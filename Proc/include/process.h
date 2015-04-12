#include <string>

const int PROCC_STDOUT_NONE = -1;
const int PROCC_STDOUT_PIPE = -2;
const int PROCC_STDERR_NONE = -1;
const int PROCC_STDERR_PIPE = -2;

class Procc
{
    public:
        Procc(int std_out_fd, int std_err_fd);
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
        int m_std_out_fd;
        int m_std_err_fd;
        const size_t PROC_MAX_STDOUT_BUF;
        const size_t PROC_MAX_STDERR_BUF;
};

