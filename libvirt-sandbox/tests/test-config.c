
#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include <libvirt-sandbox/libvirt-sandbox.h>


static gchar *readall(const gchar *path, GError **error)
{
    GFile *f = g_file_new_for_path(path);
    GFileInputStream *fis = NULL;
    GFileInfo *info;
    int len;
    gchar *ret = NULL;

    if (!(info = g_file_query_info(f, "standard::*",
                                   G_FILE_QUERY_INFO_NONE,
                                   NULL, error)))
        goto cleanup;

    len = g_file_info_get_size(info);
    ret = g_new0(gchar, len);
    if (!(fis = g_file_read(f, NULL, error)))
        goto cleanup;

    if (!g_input_stream_read_all(G_INPUT_STREAM(fis), ret, len, NULL, NULL, error))
        goto cleanup;
    *error = NULL;

cleanup:
    if (*error) {
        g_free(ret);
        ret = NULL;
    }
    g_object_unref(f);
    if (fis)
        g_object_unref(fis);
    return ret;
}


int main(int argc, char **argv)
{
    GVirSandboxConfig *cfg1 = NULL;
    GVirSandboxConfig *cfg2 = NULL;
    GError *err = NULL;
    gchar *f1 = NULL;
    gchar *f2 = NULL;
    int ret = EXIT_FAILURE;
    const gchar *mounts[] = {
        "/var/run/hell=/tmp/home",
        "/etc=/tmp/home",
        "/tmp=",
        NULL
    };
    const gchar *includes[] = {
        "/etc/nswitch.conf",
        "/etc/resolve.conf",
        "/tmp/bar=/var/tmp/foo/bar",
        NULL,
    };
    const gchar *command[] = {
        "/bin/ls", "-l", "--color", NULL,
    };

    unlink("test1.cfg");
    unlink("test2.cfg");

    if (!gvir_init_object_check(&argc, &argv, &err))
        goto cleanup;

    cfg1 = gvir_sandbox_config_new("demo");
    gvir_sandbox_config_set_root(cfg1, "/tmp");
    gvir_sandbox_config_set_arch(cfg1, "ia64");
    gvir_sandbox_config_set_tty(cfg1, TRUE);

    gvir_sandbox_config_set_command(cfg1, (gchar**)command);

    gvir_sandbox_config_set_userid(cfg1, 666);
    gvir_sandbox_config_set_groupid(cfg1, 666);
    gvir_sandbox_config_set_username(cfg1, "superdevil");
    gvir_sandbox_config_set_homedir(cfg1, "/var/run/hell");

    gvir_sandbox_config_add_mount_strv(cfg1, (gchar**)mounts);
    if (!gvir_sandbox_config_add_include_strv(cfg1, (gchar**)includes, &err))
        goto cleanup;

    gvir_sandbox_config_set_security_dynamic(cfg1, FALSE);
    gvir_sandbox_config_set_security_label(cfg1, "devil_u:devil_r:devil_t:s666:c0.c1023");

    cfg2 = gvir_sandbox_config_new("sandbox");

    unlink("test.cfg");

    if (!gvir_sandbox_config_save_path(cfg1, "test1.cfg", &err))
        goto cleanup;

    if (!gvir_sandbox_config_load_path(cfg2, "test1.cfg", &err))
        goto cleanup;

    if (!gvir_sandbox_config_save_path(cfg2, "test2.cfg", &err))
        goto cleanup;

    if (!(f1 = readall("test1.cfg", &err)))
        goto cleanup;

    if (!(f2 = readall("test2.cfg", &err)))
        goto cleanup;

    if (!g_str_equal(f1, f2)) {
        g_set_error(&err, 0, 0,
                    "Different test file content >>>%s<<< >>>%s<<<\n",
                    f1, f2);
        goto cleanup;
    }

    ret = EXIT_SUCCESS;
cleanup:
    if (ret != EXIT_SUCCESS)
        fprintf(stderr, "Error in test: %s", err && err->message ? err->message : "none");

    g_free(f1);
    g_free(f2);
    if (cfg1)
        g_object_unref(cfg1);
    if (cfg2)
        g_object_unref(cfg2);

    unlink("test1.cfg");
    unlink("test2.cfg");
    exit(ret);
}
