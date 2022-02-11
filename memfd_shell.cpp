#include <iostream>
#include <readline/readline.h>
#include <readline/history.h>
#include <string>
#include <vector>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/memfd.h>
#include <sys/syscall.h>
#include <errno.h>

using namespace std;

vector<string> split2(const string &str, const string &pattern)
{
    char *strc = new char[strlen(str.c_str()) + 1];
    strcpy(strc, str.c_str());
    vector<string> res;
    char *temp = strtok(strc, pattern.c_str());
    while (temp != NULL)
    {
        res.push_back(string(temp));
        temp = strtok(NULL, pattern.c_str());
    }
    delete[] strc;
    return res;
}

int anonyexec(const char *path, char *argv[])
{
    int   fd, fdm, filesize;
    void *elfbuf;
    char  cmdline[256];

    fd = open(path, O_RDONLY);
    filesize = lseek(fd, SEEK_SET, SEEK_END);
    lseek(fd, SEEK_SET, SEEK_SET);
    elfbuf = malloc(filesize);
    read(fd, elfbuf, filesize);
    close(fd);
    fdm = syscall(__NR_memfd_create, "nginx", MFD_CLOEXEC);
    ftruncate(fdm, filesize);
    write(fdm, elfbuf, filesize);
    free(elfbuf);
    sprintf(cmdline, "/proc/self/fd/%d", fdm);
    argv[0] = cmdline;
    execve(argv[0], argv, NULL);
    free(elfbuf);
    return -1;
}

int main()
{
    cout << "use ctrl+c or quit to exit." << endl;
    int pid;
    while (1)
    {
        string command;
        command = readline("[memfd command]# ");

        vector<string> cmd_vec = split2(command, " ");
        if(!command.compare("quit"))
            exit(0);

        int arg_count = cmd_vec.size();
        int pid = fork();
        if (pid < 0)
        {
            /* error occurred */
            cout << "forked failed" << endl;
            exit(-1);
        }
        else if (pid == 0)
        {
            /*  child process   */
            char **argv_bb = (char **)malloc(sizeof(char *) * arg_count + 1);
            memset(argv_bb, 0, sizeof(char *) * arg_count + 1);

            for (int i = 0; i < arg_count; ++i)
            {
                argv_bb[i] = strdup(cmd_vec[i].c_str());
            }

            argv_bb[arg_count] = NULL;
            return anonyexec(argv_bb[0], argv_bb);
            free(argv_bb);

        }
        else
        {
            /*    parent process  */
            /* parent will wait for the child to complete*/
            wait(NULL);
        }
    }


    return 0;
}
