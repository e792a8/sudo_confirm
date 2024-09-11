#include <string.h>
#include <limits.h>
#include <grp.h>
#include <pwd.h>

#include <sudo_plugin.h>

#define LOG_MSG_TYPE (SUDO_CONV_INFO_MSG | SUDO_CONV_PREFER_TTY)

sudo_conv_t sudo_conv;
sudo_printf_t sudo_log;
char * const * sudo_settings;
char * const * sudo_user_info;
int sudo_optind;
char * const * sudo_argv;
char * const * sudo_envp;
char * const * sudo_plugin_options;

uid_t runas_uid;
gid_t runas_gid;
const char * runas_user;
const char * runas_group;

static int
approval_open(unsigned int version, sudo_conv_t conversation,
              sudo_printf_t sudo_plugin_printf, char * const settings[],
              char * const user_info[], int submit_optind,
              char * const submit_argv[], char * const submit_envp[],
              char * const plugin_options[], const char **errstr)
{
    char * const * ui;
    struct passwd * pw;
    struct group * gr;

	sudo_conv = conversation;
	sudo_log = sudo_plugin_printf;
    sudo_settings = settings;
    sudo_user_info = user_info;
    sudo_optind = submit_optind;
    sudo_argv = submit_argv;
    sudo_envp = submit_envp;
    sudo_plugin_options = plugin_options;

    for (ui = settings; *ui != NULL; ui++) {
        if (strncmp(*ui, "runas_user=", sizeof("runas_user=") - 1) == 0) {
            runas_user = *ui + sizeof("runas_user=") - 1;
        }
        if (strncmp(*ui, "runas_group=", sizeof("runas_group=") - 1) == 0) {
            runas_group = *ui + sizeof("runas_group=") - 1;
        }
    }
    if (runas_user != NULL) {
        if ((pw = getpwnam(runas_user)) == NULL) {
            sudo_log(SUDO_CONV_ERROR_MSG, "unknown user %s\n", runas_user);
            return 0;
        }
	    runas_uid = pw->pw_uid;
    }
    if (runas_group != NULL) {
        if ((gr = getgrnam(runas_group)) == NULL) {
            sudo_log(SUDO_CONV_ERROR_MSG, "unknown group %s\n", runas_group);
            return 0;
        }
	    runas_gid = gr->gr_gid;
    }

    return 1;
}

static int 
approval_check(char * const command_info[], char * const run_argv[],
               char * const run_envp[], const char **errstr)
{
    struct sudo_conv_message msg;
    struct sudo_conv_reply repl;

    sudo_log(LOG_MSG_TYPE,
             "Executing as uid %d(%s) and gid %d(%s) with sudo:\n\n   ",
             runas_uid, runas_user, runas_gid, runas_group);
    for(char * const *ui = run_argv; *ui != NULL; ui++) {
        sudo_log(LOG_MSG_TYPE, " %s", *ui);
    }
    sudo_log(LOG_MSG_TYPE, "\n\n");

    memset(&msg, 0, sizeof(msg));
    msg.msg_type = SUDO_CONV_PROMPT_ECHO_ON
                   | SUDO_CONV_PROMPT_ECHO_OK
                   | SUDO_CONV_PREFER_TTY;
    msg.msg = "Confirm? [y/N] ";
    memset(&repl, 0, sizeof(repl));
    sudo_conv(1, &msg, &repl, NULL);

    if (repl.reply != NULL
        && (strcmp(repl.reply, "y") == 0 || strcmp(repl.reply, "Y") == 0)) {
	    return 1;
    }
	sudo_log(LOG_MSG_TYPE, "Abort.\n");
    return 0;
}

static int
approval_version(int verbose)
{
    sudo_log(SUDO_CONV_INFO_MSG, "sudo_confirm plugin version %s\n",
	         PACKAGE_VERSION);
    return 1;
}

struct approval_plugin sudo_confirm = {
    .type = SUDO_APPROVAL_PLUGIN,
    .version = SUDO_API_VERSION,
    .open = approval_open,
    .close = NULL,
    .check = approval_check,
    .show_version = approval_version,
};
