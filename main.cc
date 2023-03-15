#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <string.h>
#include <fcntl.h>

const int READ = 0;
const int WRITE = 1;

void parse_and_run_command(const std::string &command);
std::vector<std::vector<char *>> split_by_pipe_then_space(const std::string &command);
std::vector<char *> filter_redirection(const std::vector<char *> &cmd);
void handle_redirection(const std::vector<char *> &cmd);
void handle_error(int exit_code = EXIT_FAILURE);
void delete_pipev(int pipec, int **pipev);

int main(void) {
    char *host = new char[1024];
    gethostname(host,1024);
    auto hostname = std::string(host);
    delete [] host;

    std::string prefix = hostname + " % ";

    std::string command;
    std::cout << prefix;
    while (std::getline(std::cin, command)) {
        parse_and_run_command(command);
        std::cout << prefix;
    }
    return 0;
}

void parse_and_run_command(const std::string &command) {
    std::vector<std::vector<char *>> cmds(split_by_pipe_then_space(command));

    for (auto& cmd : cmds) {
        if (!strcmp(cmd[0], "exit")) {
            exit(0);
        }
    }

    int pipec = cmds.size() - 1;
    int **pipev = new int*[pipec];
    for (int i = 0; i < pipec; ++i) {
        pipev[i] = new int[2];
        if (pipe(pipev[i]) == -1) {
            std::cerr << "Pipe Failed \n";
            exit(EXIT_FAILURE);
        }
    }

    std::vector<std::string> logs;
    int in_ref = dup(STDIN_FILENO), out_ref = dup(STDOUT_FILENO), stop = 0;
    for (size_t i = 0; i < cmds.size(); ++i) {
        std::vector<char *> cmd{cmds[i]};
        if (i == 0 && pipec > 0) {
            dup2(pipev[0][WRITE], STDOUT_FILENO);
            close(pipev[0][WRITE]);
        } else if (i > 0 && i < cmds.size() - 1) {
            dup2(pipev[i - 1][READ], STDIN_FILENO);
            close(pipev[i - 1][READ]);

            dup2(pipev[i][WRITE], STDOUT_FILENO);
            close(pipev[i][WRITE]);
        } else if (i > 0 && i == cmds.size() - 1) {
            dup2(pipev[i - 1][READ], STDIN_FILENO);
            close(pipev[i - 1][READ]);
        }
        
        if (stop) {
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            handle_error();
        } 

        if (pid == 0) {
            std::vector<char *> argv{filter_redirection(cmd)};
            handle_redirection(cmd);

            execv(argv[0], argv.data());

            delete_pipev(pipec, pipev);
            handle_error(255);
        } 

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            dup2(out_ref, STDOUT_FILENO);
            auto exitcode = WEXITSTATUS(status);
            std::string cmd_name(cmd[0]);
            logs.push_back(cmd_name + " exit status: " + std::to_string(exitcode) + '\n');
            if (exitcode != 0) {
                stop = 1;
            }
        } else {
            handle_error();
        }
    }

    dup2(out_ref, STDOUT_FILENO);
    dup2(in_ref, STDIN_FILENO);
    close(in_ref);
    close(out_ref);

    delete_pipev(pipec, pipev);

    for (auto& log : logs) {
        std::cout << log;
    }
}

std::vector<std::vector<char *>> split_by_pipe_then_space(const std::string &command) {
    std::vector<std::vector<char *>> res;
    char *p = (char *)command.c_str(), *slice;
    while ((slice= strtok_r(p, "|", &p))) {
        std::vector<char *> temp;
        char *token;
        while ((token = strtok_r(slice, " \t\v\f\r\n", &slice))) {
            temp.push_back(token);
        }
        res.push_back(temp);
    }
    return res;
}

std::vector<char *> filter_redirection(const std::vector<char *> &cmd) {
    std::vector<char *> res;
    for (size_t i = 0; i < cmd.size(); ++i) {
        if (strcmp(cmd[i], "<") && strcmp(cmd[i], ">")) {
            res.push_back(cmd[i]);
        } else {
            ++i;
        }
    }
    res.push_back(NULL);
    return res;
}

void handle_redirection(const std::vector<char *> &cmd) {
    int fd_in, fd_out, in = 0, out = 0;
    size_t n = cmd.size();
    for (size_t i = 0; i < n; ++i) {
        if (!strcmp(cmd[i], "<")) {
            if (i == n - 1) {
                handle_error();
            }   
            fd_in = open(cmd[++i], O_RDONLY);
            dup2(fd_in, STDIN_FILENO);
            in = 1;
        }
        if (!strcmp(cmd[i], ">")) {
            if (i == n - 1) {
                handle_error();
            }   
            fd_out = open(cmd[++i], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            dup2(fd_out, STDOUT_FILENO);
            out = 1;
        }
    }
    if (in) {
        close(fd_in);
    }
    if (out) {
        close(fd_out);
    }
}

void handle_error(int exit_code) {
    perror(NULL);
    exit(exit_code);
}

void delete_pipev(int pipec, int **pipev) {
    for (int i = 0; i < pipec; ++i) {
        delete [] pipev[i];
    }
    delete [] pipev;
}
