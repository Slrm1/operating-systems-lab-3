/* Build the pipeline: cat scores | grep <pattern> | sort. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static void close_pipes(int first[2], int second[2]) {
    close(first[0]); close(first[1]);
    close(second[0]); close(second[1]);
}

static void wait_for_process(pid_t pid, const char *name, int *failed) {
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        *failed = 1;
    } else if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
        fprintf(stderr, "%s failed.
", name);
        *failed = 1;
    }
}

int main(int argc, char *argv[]) {
    int cat_to_grep[2], grep_to_sort[2];
    pid_t cat_pid, grep_pid, sort_pid;
    int failed = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s pattern
", argv[0]);
        return EXIT_FAILURE;
    }
    if (pipe(cat_to_grep) == -1 || pipe(grep_to_sort) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    cat_pid = fork();
    if (cat_pid < 0) {
        perror("fork cat");
        close_pipes(cat_to_grep, grep_to_sort);
        return EXIT_FAILURE;
    }
    if (cat_pid == 0) {
        if (dup2(cat_to_grep[1], STDOUT_FILENO) == -1) {
            perror("dup2 cat");
            _exit(EXIT_FAILURE);
        }
        close_pipes(cat_to_grep, grep_to_sort);
        execlp("cat", "cat", "scores", (char *)NULL);
        perror("execlp cat");
        _exit(127);
    }

    grep_pid = fork();
    if (grep_pid < 0) {
        perror("fork grep");
        close_pipes(cat_to_grep, grep_to_sort);
        wait_for_process(cat_pid, "cat", &failed);
        return EXIT_FAILURE;
    }
    if (grep_pid == 0) {
        if (dup2(cat_to_grep[0], STDIN_FILENO) == -1 ||
            dup2(grep_to_sort[1], STDOUT_FILENO) == -1) {
            perror("dup2 grep");
            _exit(EXIT_FAILURE);
        }
        close_pipes(cat_to_grep, grep_to_sort);
        execlp("grep", "grep", argv[1], (char *)NULL);
        perror("execlp grep");
        _exit(127);
    }

    sort_pid = fork();
    if (sort_pid < 0) {
        perror("fork sort");
        close_pipes(cat_to_grep, grep_to_sort);
        wait_for_process(cat_pid, "cat", &failed);
        wait_for_process(grep_pid, "grep", &failed);
        return EXIT_FAILURE;
    }
    if (sort_pid == 0) {
        if (dup2(grep_to_sort[0], STDIN_FILENO) == -1) {
            perror("dup2 sort");
            _exit(EXIT_FAILURE);
        }
        close_pipes(cat_to_grep, grep_to_sort);
        execlp("sort", "sort", (char *)NULL);
        perror("execlp sort");
        _exit(127);
    }

    close_pipes(cat_to_grep, grep_to_sort);
    wait_for_process(cat_pid, "cat", &failed);
    wait_for_process(grep_pid, "grep", &failed);
    wait_for_process(sort_pid, "sort", &failed);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
