bin_PROGRAMS = saiagent
saiagent_SOURCES = saiagent_switch.c saiagent_port.c saiagent_vlan.c \
    saiagent_fdb.c saiagent_router.c saiagent_msg.c saiagent_ipc.c \
    saiagent_thread.c saiagent.c
saiagent_CFLAGS = $(LIBNL_CFLAGS) $(LIBSAI_CFLAGS) -I$(top_srcdir)/$(PRESTERA_INC_DIR)
saiagent_LDFLAGS = $(LIBSAI_LDFLAGS)
saiagent_LDADD = $(LIBNL_LIBS) -lsai -lpthread
