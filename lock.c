#include<stdio.h>

#include<stdlib.h>

#include<string.h>

#include<sys/stat.h>

#include<fcntl.h>

#include<unistd.h>

#include<signal.h>

#include<errno.h>

#include<getopt.h>

#define RESULT_FILENAME "result.txt"
#define LOCK_EXTENSION ".lck"

int lockCount = 0;
int unlockCount = 0;
int isWritingResult = 0;

void help() {
    printf("help:\n");
    printf("./lock <filename>\n");
}

int isFileExists(char * filename) {
    struct stat statBuf;
    int statResult = stat(filename, & statBuf);

    if (statResult == -1) {
        if (errno == ENOENT) {
            return 0;
        }
        perror("Error: stat call end with error");
        exit(1);
    }

    if (S_ISREG(statBuf.st_mode)) {
        return 1;
    }

    fprintf(stderr, "Error: not regular file\n");
    exit(1);
}

void sendSignal() {
    kill(getpid(), SIGINT);
}

void handleError(char * error) {
    fprintf(stderr, error);

    if (!isWritingResult) {
        sendSignal();
    }

    exit(1);
}

void closeFile(int fd) {
    if (close(fd) == -1) {
        perror("Error: can't close file\n");
        exit(1);
    }
}

void removeFile(char * filename) {
    if (remove(filename) == -1) {
        perror("Error: can't remove lock file\n");
        exit(1);
    }
}

void createLockFile(char * filename) {
    char lockFileName[1024];
    sprintf(lockFileName, "%s%s", filename, LOCK_EXTENSION);

    int fd;

    do {
        while (isFileExists(lockFileName)) {}
        fd = open(lockFileName, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    } while (fd == -1 && errno == EEXIST);

    if (fd == -1) {
        perror("Error: can't create lock file\n");
        exit(1);
    }

    closeFile(fd);
}

void writePIDToLockFile(char * filename) {
    char lockFileName[1024];
    snprintf(lockFileName, sizeof(lockFileName), "%s%s", filename, LOCK_EXTENSION);

    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d\n", getpid());

    int fd = open(lockFileName, O_WRONLY | O_APPEND);

    if (write(fd, buffer, strlen(buffer)) != strlen(buffer)) {
        handleError("Error: error occurred while writing PID to lock file\n");
    }

    closeFile(fd);
}

void checkPidFromLockFile(int fd) {
    char strPID[1024];
    ssize_t len = read(fd, strPID, 1023);

    if (len == -1) {
        handleError("Error: can't read PID from lock file\n");
    }

    strPID[len] = '\0';

    pid_t lockPID;

    if (sscanf(strPID, "%d", & lockPID) != 1) {
        handleError("Error: bad format of lock-file\n");
    }

    if (getpid() != lockPID) {
        handleError("Error: lock file contains bad PID\n");
    }
}

void unlock(char * filename) {
    char lockFileName[1024];
    sprintf(lockFileName, "%s%s", filename, LOCK_EXTENSION);

    if (!isFileExists(lockFileName)) {
        handleError("Error: lock file does not exist while unlock\n");
    }

    int fd_lock = open(lockFileName, O_RDONLY);

    if (fd_lock == -1) {
        handleError("Error: can't open lock file while unlock\n");
    }

    checkPidFromLockFile(fd_lock);

    closeFile(fd_lock);

    removeFile(lockFileName);
}

void lock(char * filename) {
    createLockFile(filename);
    writePIDToLockFile(filename);
}

void writeResult() {

    isWritingResult = 1;

    lock(RESULT_FILENAME);

    char resultLine[1024];

    sprintf(resultLine, "PID:%d, locks:%d, unlocks:%d\n", getpid(), lockCount, unlockCount);

    int fd = open(RESULT_FILENAME, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (write(fd, resultLine, strlen(resultLine)) != strlen(resultLine)) {
        fprintf(stderr, "Error: error occurred while writing result\n");
        exit(1);
    }

    if (close(fd) == -1) {
        perror("Error: can't close result file\n");
        exit(1);
    }

    unlock(RESULT_FILENAME);
    exit(0);
}

void signalHandler(int sig) {
    writeResult();
}

void task(char * filename) {
    while (1) {
        lock(filename);
        lockCount++;
        sleep(1);
        unlock(filename);
        unlockCount++;
        sleep(1);
    }
}

int main(int argc, char * argv[]) {
    char * filename = NULL;

    if (argc < 2) {
        fprintf(stderr, "Error: filename is not provided\n");
        help();
        exit(EXIT_FAILURE);
    }

    filename = argv[1];

    if (signal(SIGINT, & signalHandler) == SIG_ERR) {
        perror("Error: can't set signal handler for SIGINT\n");
        exit(1);
    }

    task(filename);

    return 0;
}