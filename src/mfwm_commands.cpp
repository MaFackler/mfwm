

void command_run_process_sync(const char* cmd) {
    system(cmd);
}

void command_run_process_child(const char* cmd) {
    if (fork() == 0) {
        char * l[] = {(char *) cmd, NULL};
        setsid();
        execvp(cmd, l);
    }
}

