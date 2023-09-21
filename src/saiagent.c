#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "saiagent_switch.h"
#include "saiagent_log.h"
#include "saiagent_ipc.h"
#include "saiagent.h"

#define PARAM_NAME_LEN 128
#define PARAM_VALUE_LEN 256
#define PARAM_MAX_LINE_SIZE PARAM_NAME_LEN + PARAM_VALUE_LEN + 16

struct param {
    char name[PARAM_NAME_LEN];
    char value[PARAM_VALUE_LEN];
    struct param *next;
};

static struct param *g_profile_params;
static struct param *g_iter;

static saiagent_rc_t sai_profile_parse(const char *filename, struct param **params)
{
    FILE *f = NULL;
    char line[PARAM_MAX_LINE_SIZE] = { 0 };
    struct param *param_last = NULL;

    if (NULL == (f = fopen(filename, "r"))) {
        LOG_ERR("Open profile file \"%s\" failed", filename);
        return RC_STATUS_ERR;
    }

    while (fgets(line, PARAM_MAX_LINE_SIZE - 1, f) != NULL)
    {
        struct param *new_param = calloc(1, sizeof(struct param)); 
        if (!new_param || sscanf(line, "%[a-z_A-Z0-9]=%[^#\n\r]",
            new_param->name, new_param->value) < 2) {
            LOG_ERR("SAI: profile parse fail \"%s\"", line);
            free(new_param); fclose(f);
            return RC_STATUS_OK;
        }
        LOG_INFO("PROFILE: parse %s = %s", new_param->name, new_param->value);
        new_param->next = param_last;
        param_last = new_param;
    }

    *params = param_last; fclose(f);
    return RC_STATUS_OK;
}

static const char* sai_profile_get_value(sai_switch_profile_id_t profile_id,
    const char* variable)
{
    LOG_INFO("PROFILE: getting '%s' value", variable);
    for (struct param *it = g_profile_params; it; it = it->next)
    {
        if (variable && !strcmp(it->name, variable)) {
            return it->value;
        }
    }

    LOG_INFO("PROFILE: param not found");
    return NULL;
}

static int sai_profile_get_next_value(sai_switch_profile_id_t profile_id,
    const char** variable, const char** value)
{
    if (variable == NULL || !g_iter) {
        LOG_INFO("PROFILE: resetting...");
        g_iter = g_profile_params;
	return -1; /* -1 at the end of the list */
    }

    LOG_INFO("SAI: profile enum: name=%s, val=%s", g_iter->name, g_iter->value);

    *variable = g_iter->name;
    *value = g_iter->value;
    g_iter = g_iter->next;

    return 0; /* 0 if next value exists */
}
static const sai_service_method_table_t sai_services = {
    sai_profile_get_value, sai_profile_get_next_value
};

struct cmd_opts {
    FILE *port_map_file;
    struct param *profile_params;
};

static saiagent_rc_t cmd_line_process(int argc, char **argv, struct cmd_opts *opts)
{
    FILE *f = NULL;
    struct option long_options[] =
    {
        { "profile",          required_argument, 0, 'p' },
        { "portmap",          required_argument, 0, 'f' },
        { 0,                  0,                 0,  0  }
    };

    while (1)
    {
        int option_index = 0;
        int c = getopt_long(argc, argv, "p:f:", long_options, &option_index);
        if (c < 0)
            break;

        switch (c)
        {
            case 'p':
                LOG_INFO("profile map file: %s", optarg);
                if (sai_profile_parse(optarg, &opts->profile_params) != RC_STATUS_OK) {
                    LOG_ERR("Parse profile failed");
                    return RC_STATUS_ERR;
                }
                break;
            case 'f':
                if (NULL == (f = fopen(optarg, "r"))) {
                    LOG_ERR("Open port map \"%s\" failed", optarg);
                    return RC_STATUS_ERR;
                }
                LOG_INFO("port map file: %s", optarg);
                opts->port_map_file = f;
                fclose(f);
                break;
            default:
                printf("getopt_long() failure\n");
                return RC_STATUS_ERR;
        }
    }
    return RC_STATUS_OK;
}

int
main(int argc, char* argv[])
{
    saiagent_rc_t status;
    LOG_INFO("Starting SAI Agent ...");

    struct cmd_opts opts = {0};
    if (RC_STATUS_OK != cmd_line_process(argc, argv, &opts)) {
        LOG_ERR("Command line arguments parse error");
        return EXIT_FAILURE;
    }

    if (!opts.profile_params) {
        LOG_ERR("Please provide profile file via -p <FILE> option");
        return EXIT_FAILURE;
    }

    LOG_INFO("SAI initialize ...");
    g_profile_params = opts.profile_params;
    g_iter = opts.profile_params;
    sai_status_t sai_status = sai_api_initialize(0, &sai_services);
    if (SAI_STATUS_SUCCESS != sai_status) {
        LOG_ERR("SAI init failed with status %d", sai_status);
        return EXIT_FAILURE;
    }

    sai_status = sai_log_set(SAI_API_SWITCH, SAI_LOG_LEVEL_INFO);
    if (SAI_STATUS_SUCCESS != sai_status) {
        LOG_ERR("SAI: set switch log failed %d", sai_status);
        return EXIT_FAILURE;
    }

    status = saiagent_ipc_init();
    if (RC_STATUS_OK != status) {
        LOG_ERR("IPC: init failed");
        goto err_ipc_init;
    }

    status = saiagent_switch_ipc_init();
    if (RC_STATUS_OK != status) {
        LOG_ERR("saiagent_switch_ipc_init() fail");
        goto err_ipc_init;
    }

    status = saiagent_ipc_drv_init();
    if (RC_STATUS_OK != status) {
        LOG_ERR("IPC: drv init failed");
        goto err_ipc_init;
    }

    saiagent_ipc_enter_loop();
    sai_api_uninitialize();

err_ipc_init:
    saiagent_ipc_deinit();

    LOG_INFO("Stopping SAI Agent ...");
    return EXIT_SUCCESS;
}
