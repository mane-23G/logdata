/*
Title: logdata.c
Author: Mane Galstyan
Created on: March 24, 2024
Description: print total log in time for user
Purpose: prints total log in times for users from the wtmp file
Usage: logdata [-a -s -f <file>] username ...
Build with: gcc -o logdata logdata.c
Modifications: March 25, 2024 clean up and comments
 */

#define _GNU_SOURCE
#include <langinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <locale.h>
#include <paths.h>
#include <pwd.h>
#include <utmp.h>
#include <utmpx.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_USERS 1000
#define MAXLEN 1024

//create a BOOL type
#ifdef FALSE
    #undef FALSE
#endif
#ifdef TRUE
    #undef TRUE
#endif

#ifdef BOOL
    #undef BOOL
#endif
typedef enum{FALSE, TRUE} BOOL;

/* error_message()
   This prints an error message associated with errnum  on standard error
   if errnum > 0. If errnum <= 0, it prints the msg passed to it.
   It does not terminate the calling program.
   This is used when there is a way  to recover from the error.               */
void error_mssge(int errornum, const char * msg) {
    errornum > 0 ? fprintf(stderr, "%s\n", msg) : fprintf(stderr, "%s\n", msg);
}

/* fatal_error()
   This prints an error message associated with errnum  on standard error
   before terminating   the calling program, if errnum > 0.
   If errnum <= 0, it prints the msg passed to it.
   fatal_error() should be called for a nonrecoverable error.                 */
void fatal_error(int errornum, const char * msg) {
    error_mssge(errornum, msg);
    exit(EXIT_FAILURE);
}

/* usage_error()
   This prints a usage error message on standard output, advising the
   user of the correct way to call the program, and terminates the program.   */
void usage_error(const char * msg) {
    fprintf(stderr, "usage: %s\n", msg);
    exit(EXIT_FAILURE);
}

//struct to keep track of all users read from utmp file
typedef struct {
    char username[sizeof(((struct utmpx *)0)->ut_user)];
    time_t total_login_time;
    time_t last_login_time;
    BOOL is_logged_in;
} UserLoginInfo;


UserLoginInfo users[MAX_USERS];
int user_count = 0;
time_t total_duration = 0;

//finds if user exists in array based on username
int find_user(const char *username) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return i;
        }
    }
    return -1; // User not found
}

//updates time based on the logout time
void update_login_time(int user_index, time_t logout_time) {
    if (users[user_index].is_logged_in) {
        users[user_index].total_login_time += logout_time - users[user_index].last_login_time;
        users[user_index].is_logged_in = FALSE;
    }
}

//gets the utmpx struct and process it to find the user and record the log in time for user
void add_or_update_user(const struct utmpx *ut) {
    int user_index = find_user(ut->ut_user);
    if (ut->ut_type == USER_PROCESS) {
        if (user_index == -1) {
            user_index = user_count++;
            strncpy(users[user_index].username, ut->ut_user, sizeof(users[user_index].username) - 1);
            users[user_index].username[sizeof(users[user_index].username) - 1] = '\0';
            users[user_index].total_login_time = 0;
        }
        users[user_index].last_login_time = ut->ut_tv.tv_sec;
        users[user_index].is_logged_in = TRUE;
    }
    else if (user_index != -1 && ut->ut_type == DEAD_PROCESS) {
        update_login_time(user_index, ut->ut_tv.tv_sec);
    }
}

//formart teh time given a time_t struct
char* format_time(time_t total_login_time) {
    char *total_login_time_str = malloc(MAXLEN * sizeof(char));
    if (total_login_time_str == NULL) {
        perror("Failed to allocate memory for formatted time string");
        exit(EXIT_FAILURE); // Or handle the error as appropriate
    }
    int minutes = (total_login_time / 60) % 60;
    int hours = (total_login_time / 3600) % 24;
    int days = total_login_time / 86400;
    int seconds = total_login_time % 60;
    
    char min_str[64] = "";
    char hour_str[64] = "";
    char day_str[64] = "";
    char sec_str[64] = "";
    
    if(minutes == 1) {
        sprintf(min_str, "1 min");
    } else if(minutes > 1) {
        sprintf(min_str, "%d mins", minutes);
    }

    if(hours == 1) {
        sprintf(hour_str, "1 hour");
    } else if(hours > 1) {
        sprintf(hour_str, "%d hours", hours);
    }

    if(days == 1) {
        sprintf(day_str, "1 day");
    } else if(days > 1) {
        sprintf(day_str, "%d days", days);
    }

    if(seconds == 1) {
        sprintf(sec_str, "1 sec");
    } else if(seconds > 1) {
        sprintf(sec_str, "%d secs", seconds);
    }

    char* times[] = {day_str, hour_str, min_str, sec_str};
    total_login_time_str[0] = '\0';
    for (int i = 0; i < 4; ++i) {
        if (times[i][0] != '\0') {
            if (total_login_time_str[0] != '\0') {
                strcat(total_login_time_str, " ");
            }
            strcat(total_login_time_str, times[i]);
        }
    }
    return total_login_time_str;
}


int main(int argc, char* argv[]) {
    
    //parse command line for options
    char usage_msg[512];             /* Usage message                      */
    char ch;                         /* For option handling                */
    char options[] = "asf:h";         /* getopt string                      */
    BOOL a_option = FALSE;           /* Flag to indicate -a found          */
    BOOL s_option = FALSE;           /* Flag to indicate -s found          */
    BOOL f_option = FALSE;           /* Flag to indicate -f <file> found   */
    BOOL user_option = FALSE;           /* Flag to indicate -f <file> found   */
    BOOL curr_user_option = FALSE;           /* Flag to indicate -f <file> found   */
    char *f_arg;                     /* Dynamic string for -f argument     */
    int f_arg_length;                /* Length of -f argument string       */
    
    while(TRUE) {
        ch = getopt(argc, argv, options);
        if(-1 == ch)
            break;
        switch (ch) {
            case 'f':
                f_option = TRUE;
                f_arg_length = strlen(optarg);
                f_arg = malloc((f_arg_length+1) * sizeof(char));
                if(NULL == f_arg)
                    fatal_error(EXIT_FAILURE, "calloc could not allocate memory\n");
                strcpy(f_arg, optarg);
                break;
            case 'a':
                a_option = TRUE;
                break;
            case 's':
                s_option = TRUE;
                break;
            case 'h':
                sprintf(usage_msg, "%s [-a -s -f <file>] username ...", argv[0]);
                usage_error(usage_msg);
                break;
            case '?' :
                fprintf(stderr,"Found invalid option\n");
                sprintf(usage_msg, "%s [-a -s -f <file>] username ...>", argv[0]);
                usage_error(usage_msg);
                break;
            case ':' :
                fprintf(stderr,"Found option with missing argument\n");
                sprintf(usage_msg, "%s [-a -s -f <file>] username ...", argv[0]);
                usage_error(usage_msg);
                break;
        }
    }
    
    int wtmp_fd; //file descriptor
    struct utmpx ut; //sturct for wtmp file
    
    //if user gave a file use that file else use _PATH_WTMP
    if(f_option) {
        wtmp_fd = open(f_arg, O_RDONLY);
        if (wtmp_fd == -1) {
            sprintf(usage_msg, "error while opening %s", f_arg);
            free(f_arg);
            fatal_error(errno, usage_msg);
        }
    }
    else {
        wtmp_fd = open(_PATH_WTMP, O_RDONLY);
        if (wtmp_fd == -1) {
            sprintf(usage_msg, "error while opening %s", _PATH_WTMP);
            fatal_error(errno, usage_msg);
        }
    }

    //read wtmp record from file and process it
    while (read(wtmp_fd,&ut, sizeof(ut)) == sizeof(ut)) {
        add_or_update_user(&ut);
    }

    // Close the wtmp file
    close(wtmp_fd);

    // Update total login time for users still logged in
    time_t current_time = time(NULL);
    for (int i = 0; i < user_count; i++) {
        if (users[i].is_logged_in) {
            update_login_time(i, current_time); // Use current time as logout time
        }
    }
    
    if(optind < argc) {
        user_option = TRUE;
    }
    
    if(!user_option && !a_option) {
        curr_user_option = TRUE;
    }
    
    //print users listed on command line
    if(!a_option && user_option) {
        int index;
        for (int i = optind; i < argc; i++) {
            index = find_user(argv[i]);
            if(index == -1) {
                printf("%-18.18s    0 seconds\n", argv[i]);
            }
            else {
                char *formatted_time = format_time(users[index].total_login_time);
                printf("%-18.18s    %s\n", users[index].username, formatted_time);
                if(s_option) {
                    total_duration += users[index].total_login_time;
                }
                free(formatted_time);
            }
        }
    }
    
    //print all users in wtmp file
    if(a_option) {
        // Print total login time for each user
        for (int i = 0; i < user_count; i++) {
            char *formatted_time = format_time(users[i].total_login_time);
            printf("%-18.18s    %s\n", users[i].username, formatted_time);
            if(s_option) {
                total_duration += users[i].total_login_time;
            }
            free(formatted_time);
        }
    }
    
    //print calling user log in times
    if(!user_option && !a_option) {
        struct passwd *pw = getpwuid(getuid());
        if (!pw) {
            fatal_error(errno, "Failed to get the current user's username");
        }
        int index = find_user(pw->pw_name);
        if(index == -1) {
            printf("%-18.18s    0 seconds\n", pw->pw_name);
        }
        else {
            char *formatted_time = format_time(users[index].total_login_time);
            printf("%-18.18s    %s\n", users[index].username, formatted_time);
            if(s_option) {
                total_duration += users[index].total_login_time;
            }
            free(formatted_time);
        }
        
    }
    
    //print total duration if s option is present
    if(s_option) {
        char *formatted_time = format_time(total_duration);
        printf("\nTotal duration for all users listed: %s\n", formatted_time);
        free(formatted_time);
    }

    return 0;
}
