#include "blade.h"

#define CONSOLE_INPUT_MAX 512

// @todo switch to wait condition once something is being done with the main thread during runtime
ks_bool_t g_shutdown = KS_FALSE;

void loop(blade_handle_t *bh);
void process_console_input(blade_handle_t *bh, char *line);

typedef void(*command_callback)(blade_handle_t *bh, char *args);

struct command_def_s {
	const char *cmd;
	command_callback callback;
};

void command_quit(blade_handle_t *bh, char *args);

static const struct command_def_s command_defs[] = {
	{ "quit", command_quit },

	{ NULL, NULL }
};


int main(int argc, char **argv)
{
	blade_handle_t *bh = NULL;
	config_t config;
	config_setting_t *config_blade = NULL;
	const char *cfgpath = "switchblade.cfg";

	ks_global_set_default_logger(KS_LOG_LEVEL_DEBUG);

	blade_init();

	blade_handle_create(&bh);

	config_init(&config);
	if (!config_read_file(&config, cfgpath)) {
		ks_log(KS_LOG_ERROR, "%s:%d - %s\n", config_error_file(&config), config_error_line(&config), config_error_text(&config));
		config_destroy(&config);
		return EXIT_FAILURE;
	}
	config_blade = config_lookup(&config, "blade");
	if (!config_blade) {
		ks_log(KS_LOG_ERROR, "Missing 'blade' config group\n");
		config_destroy(&config);
		return EXIT_FAILURE;
	}
	if (config_setting_type(config_blade) != CONFIG_TYPE_GROUP) {
		ks_log(KS_LOG_ERROR, "The 'blade' config setting is not a group\n");
		return EXIT_FAILURE;
	}

	if (blade_handle_startup(bh, config_blade) != KS_STATUS_SUCCESS) {
		ks_log(KS_LOG_ERROR, "Blade startup failed\n");
		return EXIT_FAILURE;
	}

	loop(bh);

	blade_handle_destroy(&bh);

	config_destroy(&config);

	blade_shutdown();

	return 0;
}

void loop(blade_handle_t *bh)
{
	char buf[CONSOLE_INPUT_MAX];
	while (!g_shutdown) {
		if (!fgets(buf, CONSOLE_INPUT_MAX, stdin)) break;

		for (int index = 0; buf[index]; ++index) {
			if (buf[index] == '\r' || buf[index] == '\n') {
				buf[index] = '\0';
				break;
			}
		}
		process_console_input(bh, buf);

		ks_sleep_ms(100);
	}
}

void parse_argument(char **input, char **arg, char terminator)
{
	char *tmp;

	ks_assert(input);
	ks_assert(*input);
	ks_assert(arg);

	tmp = *input;
	*arg = tmp;

	while (*tmp && *tmp != terminator) ++tmp;
	if (*tmp == terminator) {
		*tmp = '\0';
		++tmp;
	}
	*input = tmp;
}

void process_console_input(blade_handle_t *bh, char *line)
{
	char *args = line;
	char *cmd = NULL;
	ks_bool_t found = KS_FALSE;

	parse_argument(&args, &cmd, ' ');

	ks_log(KS_LOG_DEBUG, "Command: %s, Args: %s\n", cmd, args);

	for (int32_t index = 0; command_defs[index].cmd; ++index) {
		if (!strcmp(command_defs[index].cmd, cmd)) {
			found = KS_TRUE;
			command_defs[index].callback(bh, args);
		}
	}
	if (!found) ks_log(KS_LOG_INFO, "Command '%s' unknown.\n", cmd);
}

void command_quit(blade_handle_t *bh, char *args)
{
	//ks_assert(bh);
	//ks_assert(args);

	g_shutdown = KS_TRUE;
}


/* For Emacs:
* Local Variables:
* mode:c
* indent-tabs-mode:t
* tab-width:4
* c-basic-offset:4
* End:
* For VIM:
* vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
*/