/*
 ============================================================================
 Name        : clevo-indicator.c
 Author      : AqD <iiiaqd@gmail.com>
 Version     :
 Description : Ubuntu fan control indicator for Clevo laptops

 Based on http://www.association-apml.fr/upload/fanctrl.c by Jonas Diemer
 (diemer@gmx.de)

 ============================================================================

 TEST:
 gcc clevo-indicator.c -o clevo-indicator `pkg-config --cflags --libs appindicator3-0.1` -lm
 sudo chown root clevo-indicator
 sudo chmod u+s clevo-indicator

 Run as effective uid = root, but uid = desktop user (in order to use indicator).

 ============================================================================
 Auto fan control algorithm:

 The algorithm is to replace the builtin auto fan-control algorithm in Clevo
 laptops which is apparently broken in recent models such as W350SSQ, where the
 fan doesn't get kicked until both of GPU and CPU are really hot (and GPU
 cannot be hot anymore thanks to nVIDIA's Maxwell chips). It's far more
 aggressive than the builtin algorithm in order to keep the temperatures below
 60°C all the time, for maximized performance with Intel turbo boost enabled.

 ============================================================================
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <gio/gio.h>
#include <libdbusmenu-glib/dbusmenu-glib.h>
#include <cairo/cairo.h>

#define NAME "clevo-indicator"
#define ITEM_PATH "/org/ayatana/NotificationItem/clevo_indicator"
#define MENU_PATH "/org/ayatana/NotificationItem/clevo_indicator/Menu"

#define EC_SC 0x66
#define EC_DATA 0x62

#define IBF 1
#define OBF 0
#define EC_SC_READ_CMD 0x80

/* EC registers can be read by EC_SC_READ_CMD or /sys/kernel/debug/ec/ec0/io:
 *
 * 1. modprobe ec_sys
 * 2. od -Ax -t x1 /sys/kernel/debug/ec/ec0/io
 */

#define EC_REG_SIZE 0x100
#define EC_REG_CPU_TEMP 0x07
#define EC_REG_GPU_TEMP 0xCD
#define EC_REG_FAN_DUTY 0xCE
#define EC_REG_FAN_RPMS_HI 0xD0
#define EC_REG_FAN_RPMS_LO 0xD1

#define MAX_FAN_RPM 5500.0

typedef enum {
    NA = 0, AUTO = 1, MANUAL = 2
} MenuItemType;

static void main_init_share(void);
static int main_ec_worker(void);
static void main_ui_worker(int argc, char** argv);
static void main_on_sigchld(int signum);
static void main_on_sigterm(int signum);
static int main_dump_fan(void);
static int main_test_fan(int duty_percentage);
static gboolean ui_update(gpointer user_data);
static void ui_render_icon(void);
static void ui_emit_signal(const char* member);
static gboolean ui_register_item(gpointer user_data);
static void ui_build_menu(void);
static void sni_on_item_activated(DbusmenuMenuitem* item, guint timestamp,
        gpointer user_data);
static void sni_on_method_call(GDBusConnection* connection,
        const gchar* sender, const gchar* object_path,
        const gchar* interface_name, const gchar* method_name,
        GVariant* parameters, GDBusMethodInvocation* invocation,
        gpointer user_data);
static GVariant* sni_get_property(GDBusConnection* connection,
        const gchar* sender, const gchar* object_path,
        const gchar* interface_name, const gchar* property_name,
        GError** error, gpointer user_data);
static void ui_command_set_fan(long fan_duty);
static void ui_command_quit(gchar* command);
static void ui_toggle_menuitems(int fan_duty);
static void ec_on_sigterm(int signum);
static int ec_init(void);
static int ec_auto_duty_adjust(void);
static int ec_query_cpu_temp(void);
static int ec_query_gpu_temp(void);
static int ec_query_fan_duty(void);
static int ec_query_fan_rpms(void);
static int ec_write_fan_duty(int duty_percentage);
static int ec_io_wait(const uint32_t port, const uint32_t flag,
        const char value);
static uint8_t ec_io_read(const uint32_t port);
static int ec_io_do(const uint32_t cmd, const uint32_t port,
        const uint8_t value);
static int calculate_fan_duty(int raw_duty);
static int calculate_fan_rpms(int raw_rpm_high, int raw_rpm_low);
static int check_proc_instances(const char* proc_name);
static void get_time_string(char* buffer, size_t max, const char* format);
static void signal_term(__sighandler_t handler);

/* Status Notifier Item D-Bus interface (implements the standard directly,
 * so the tray looks and behaves the same on LXQt, GNOME and KDE). */
#define SNI_XML \
"<node>" \
" <interface name='org.kde.StatusNotifierItem'>" \
"  <property name='Id' type='s' access='read'/>" \
"  <property name='Category' type='s' access='read'/>" \
"  <property name='Title' type='s' access='read'/>" \
"  <property name='Status' type='s' access='read'/>" \
"  <property name='WindowId' type='i' access='read'/>" \
"  <property name='IconName' type='s' access='read'/>" \
"  <property name='IconPixmap' type='a(iiay)' access='read'/>" \
"  <property name='OverlayIconName' type='s' access='read'/>" \
"  <property name='OverlayIconPixmap' type='a(iiay)' access='read'/>" \
"  <property name='AttentionIconName' type='s' access='read'/>" \
"  <property name='AttentionIconPixmap' type='a(iiay)' access='read'/>" \
"  <property name='AttentionMovieName' type='s' access='read'/>" \
"  <property name='ToolTip' type='(sa(iiay)ss)' access='read'/>" \
"  <property name='ItemIsMenu' type='b' access='read'/>" \
"  <property name='Menu' type='o' access='read'/>" \
"  <method name='ContextMenu'>" \
"   <arg name='x' type='i' direction='in'/>" \
"   <arg name='y' type='i' direction='in'/>" \
"  </method>" \
"  <method name='Activate'>" \
"   <arg name='x' type='i' direction='in'/>" \
"   <arg name='y' type='i' direction='in'/>" \
"  </method>" \
"  <method name='SecondaryActivate'>" \
"   <arg name='x' type='i' direction='in'/>" \
"   <arg name='y' type='i' direction='in'/>" \
"  </method>" \
"  <method name='Scroll'>" \
"   <arg name='delta' type='i' direction='in'/>" \
"   <arg name='orientation' type='s' direction='in'/>" \
"  </method>" \
"  <signal name='NewIcon'/>" \
"  <signal name='NewAttentionIcon'/>" \
"  <signal name='NewOverlayIcon'/>" \
"  <signal name='NewIconThemePath'>" \
"   <arg name='icon_theme_path' type='s'/>" \
"  </signal>" \
"  <signal name='NewToolTip'/>" \
"  <signal name='NewStatus'>" \
"   <arg name='status' type='s'/>" \
"  </signal>" \
"  <signal name='NewTitle'/>" \
" </interface>" \
"</node>"

static const GDBusInterfaceVTable sni_vtable = {
    sni_on_method_call, sni_get_property, NULL, { 0 }
};

static GDBusConnection* bus = NULL;
static GDBusNodeInfo* sni_node = NULL;
static guint sni_reg_id = 0;
static GMainLoop* ui_loop = NULL;
static DbusmenuServer* menu_server = NULL;
static guint8* pixmap_buf[2] = { NULL, NULL };
static int pixmap_buf_idx = 0;
static double blade_rotation = 0.0;
static char tool_tip_title[256] = "Clevo";
static char tool_tip_desc[256] = "";

struct {
    char label[256];
    GCallback callback;
    long option;
    MenuItemType type;
    DbusmenuMenuitem* widget;

}static menuitems[] = {
        { "Set FAN to AUTO", G_CALLBACK(ui_command_set_fan), 0, AUTO, NULL },
        { "", NULL, 0L, NA, NULL },
        { "Set FAN to  60%", G_CALLBACK(ui_command_set_fan), 60, MANUAL, NULL },
        { "Set FAN to  70%", G_CALLBACK(ui_command_set_fan), 70, MANUAL, NULL },
        { "Set FAN to  80%", G_CALLBACK(ui_command_set_fan), 80, MANUAL, NULL },
        { "Set FAN to  90%", G_CALLBACK(ui_command_set_fan), 90, MANUAL, NULL },
        { "Set FAN to 100%", G_CALLBACK(ui_command_set_fan), 100, MANUAL, NULL },
        { "", NULL, 0L, NA, NULL },
        { "Quit", G_CALLBACK(ui_command_quit), 0L, NA, NULL }
};

static int menuitem_count = (sizeof(menuitems) / sizeof(menuitems[0]));

struct {
    volatile int exit;
    volatile int cpu_temp;
    volatile int gpu_temp;
    volatile int fan_duty;
    volatile int fan_rpms;
    volatile int auto_duty;
    volatile int auto_duty_val;
    volatile int manual_next_fan_duty;
    volatile int manual_prev_fan_duty;
}static *share_info = NULL;

static pid_t parent_pid = 0;
static pid_t ec_worker_pid = 0;

int main(int argc, char* argv[]) {
    printf("Simple fan control utility for Clevo laptops\n");
    if (check_proc_instances(NAME) > 1) {
        printf("Multiple running instances!\n");
        return EXIT_FAILURE;
    }
    if (ec_init() != EXIT_SUCCESS) {
        printf("unable to control EC: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    if (argc <= 1) {
        char* display = getenv("DISPLAY");
        if (display == NULL || strlen(display) == 0) {
            return main_dump_fan();
        } else {
            parent_pid = getpid();
            main_init_share();
            signal(SIGCHLD, &main_on_sigchld);
            signal_term(&main_on_sigterm);
            pid_t worker_pid = fork();
            if (worker_pid == 0) {
                signal(SIGCHLD, SIG_DFL);
                signal_term(&ec_on_sigterm);
                return main_ec_worker();
            } else if (worker_pid > 0) {
                ec_worker_pid = worker_pid;
                main_ui_worker(argc, argv);
                share_info->exit = 1;
                waitpid(worker_pid, NULL, 0);
            } else {
                printf("unable to create worker: %s\n", strerror(errno));
                return EXIT_FAILURE;
            }
        }
    } else {
        if (argv[1][0] == '-') {
            printf(
                    "\n\
Usage: clevo-indicator [fan-duty-percentage]\n\
\n\
Dump/Control fan duty on Clevo laptops. Display indicator by default.\n\
\n\
Arguments:\n\
  [fan-duty-percentage]\t\tTarget fan duty in percentage, from 40 to 100\n\
  -?\t\t\t\tDisplay this help and exit\n\
\n\
Without arguments this program should attempt to display an indicator in\n\
the Ubuntu tray area for fan information display and control. The indicator\n\
requires this program to have setuid=root flag but run from the desktop user\n\
, because a root user is not allowed to display a desktop indicator while a\n\
non-root user is not allowed to control Clevo EC (Embedded Controller that's\n\
responsible of the fan). Fix permissions of this executable if it fails to\n\
run:\n\
    sudo chown root clevo-indicator\n\
    sudo chmod u+s  clevo-indicator\n\
\n\
Note any fan duty change should take 1-2 seconds to come into effect - you\n\
can verify by the fan speed displayed on indicator icon and also louder fan\n\
noise.\n\
\n\
In the indicator mode, this program would always attempt to load kernel\n\
module 'ec_sys', in order to query EC information from\n\
'/sys/kernel/debug/ec/ec0/io' instead of polling EC ports for readings,\n\
which may be more risky if interrupted or concurrently operated during the\n\
process.\n\
\n\
DO NOT MANIPULATE OR QUERY EC I/O PORTS WHILE THIS PROGRAM IS RUNNING.\n\
\n");
            return main_dump_fan();
        } else {
            int val = atoi(argv[1]);
            if (val < 40 || val > 100)
                    {
                printf("invalid fan duty %d!\n", val);
                return EXIT_FAILURE;
            }
            return main_test_fan(val);
        }
    }
    return EXIT_SUCCESS;
}

static void main_init_share(void) {
    void* shm = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED,
            -1, 0);
    share_info = shm;
    share_info->exit = 0;
    share_info->cpu_temp = 0;
    share_info->gpu_temp = 0;
    share_info->fan_duty = 0;
    share_info->fan_rpms = 0;
    share_info->auto_duty = 1;
    share_info->auto_duty_val = 0;
    share_info->manual_next_fan_duty = 0;
    share_info->manual_prev_fan_duty = 0;
}

static int main_ec_worker(void) {
    setuid(0);
    system("modprobe ec_sys");
    while (share_info->exit == 0) {
        // check parent
        if (parent_pid != 0 && kill(parent_pid, 0) == -1) {
            printf("worker on parent death\n");
            break;
        }
        // write EC
        int new_fan_duty = share_info->manual_next_fan_duty;
        if (new_fan_duty != 0
                && new_fan_duty != share_info->manual_prev_fan_duty) {
            ec_write_fan_duty(new_fan_duty);
            share_info->manual_prev_fan_duty = new_fan_duty;
        }
        // read EC
        int io_fd = open("/sys/kernel/debug/ec/ec0/io", O_RDONLY, 0);
        if (io_fd < 0) {
            printf("unable to read EC from sysfs: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        unsigned char buf[EC_REG_SIZE];
        ssize_t len = read(io_fd, buf, EC_REG_SIZE);
        switch (len) {
        case -1:
            printf("unable to read EC from sysfs: %s\n", strerror(errno));
            break;
        case 0x100:
            share_info->cpu_temp = buf[EC_REG_CPU_TEMP];
            share_info->gpu_temp = buf[EC_REG_GPU_TEMP];
            share_info->fan_duty = calculate_fan_duty(buf[EC_REG_FAN_DUTY]);
            share_info->fan_rpms = calculate_fan_rpms(buf[EC_REG_FAN_RPMS_HI],
                    buf[EC_REG_FAN_RPMS_LO]);
            /*
             printf("temp=%d, duty=%d, rpms=%d\n", share_info->cpu_temp,
             share_info->fan_duty, share_info->fan_rpms);
             */
            break;
        default:
            printf("wrong EC size from sysfs: %ld\n", len);
        }
        close(io_fd);
        // auto EC
        if (share_info->auto_duty == 1) {
            int next_duty = ec_auto_duty_adjust();
            if (next_duty != 0 && next_duty != share_info->auto_duty_val) {
                char s_time[256];
                get_time_string(s_time, 256, "%m/%d %H:%M:%S");
                printf("%s CPU=%d°C, GPU=%d°C, auto fan duty to %d%%\n", s_time,
                        share_info->cpu_temp, share_info->gpu_temp, next_duty);
                ec_write_fan_duty(next_duty);
                share_info->auto_duty_val = next_duty;
            }
        }
        //
        usleep(200 * 1000);
    }
    printf("worker quit\n");
    return EXIT_SUCCESS;
}

static void main_ui_worker(int argc, char** argv) {
    (void) argc;
    (void) argv;
    printf("Indicator...\n");
    int desktop_uid = getuid();
    setuid(desktop_uid);
    //
    bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (bus == NULL) {
        printf("unable to connect to the session bus\n");
        return;
    }
    sni_node = g_dbus_node_info_new_for_xml(SNI_XML, NULL);
    sni_reg_id = g_dbus_connection_register_object(bus, ITEM_PATH,
            sni_node->interfaces[0], &sni_vtable, NULL, NULL, NULL);
    if (sni_reg_id == 0) {
        printf("unable to export the StatusNotifierItem object\n");
        return;
    }
    // an icon and a menu must exist before hosts start reading the item
    ui_render_icon();
    ui_build_menu();
    //
    ui_loop = g_main_loop_new(NULL, FALSE);
    g_timeout_add(500, &ui_update, NULL);
    // registers with the StatusNotifierWatcher, retrying until it succeeds
    g_timeout_add(2000, &ui_register_item, NULL);
    ui_register_item(NULL);
    g_main_loop_run(ui_loop);
    printf("main on UI quit\n");
}

static void main_on_sigchld(int signum) {
    // Reap children but only quit when the EC worker itself is gone.
    // Other children (e.g. helpers spawned by GTK or the input-method
    // module) must not terminate the indicator.
    int status = 0;
    pid_t p;
    while ((p = waitpid(-1, &status, WNOHANG)) > 0) {
        if (ec_worker_pid != 0 && p == ec_worker_pid) {
            printf("main: EC worker exited (status=%d)\n", status);
            fflush(stdout);
            exit(EXIT_SUCCESS);
        }
    }
}

static void main_on_sigterm(int signum) {
    printf("main on signal: %s\n", strsignal(signum));
    if (share_info != NULL)
        share_info->exit = 1;
    exit(EXIT_SUCCESS);
}

static int main_dump_fan(void) {
    printf("Dump fan information\n");
    printf("  FAN Duty: %d%%\n", ec_query_fan_duty());
    printf("  FAN RPMs: %d RPM\n", ec_query_fan_rpms());
    printf("  CPU Temp: %d°C\n", ec_query_cpu_temp());
    printf("  GPU Temp: %d°C\n", ec_query_gpu_temp());
    return EXIT_SUCCESS;
}

static int main_test_fan(int duty_percentage) {
    printf("Change fan duty to %d%%\n", duty_percentage);
    ec_write_fan_duty(duty_percentage);
    printf("\n");
    main_dump_fan();
    return EXIT_SUCCESS;
}

static gboolean ui_update(gpointer user_data) {
    (void) user_data;
    // gentle spin of the blades while the fan runs
    if (share_info->fan_rpms > 0)
        blade_rotation += 15.0;
    ui_render_icon();
    // tooltip: title shown by most hosts, description below it (GTK markup)
    snprintf(tool_tip_title, sizeof(tool_tip_title),
            "Clevo - CPU %d°C · GPU %d°C · %d RPM (%d%%)",
            share_info->cpu_temp, share_info->gpu_temp,
            share_info->fan_rpms, share_info->fan_duty);
    snprintf(tool_tip_desc, sizeof(tool_tip_desc),
            "Clevo Fan Control — ventilador a %d RPM (duty %d%%)",
            share_info->fan_rpms, share_info->fan_duty);
    static char prev_tt[256] = "";
    if (strcmp(prev_tt, tool_tip_title) != 0) {
        snprintf(prev_tt, sizeof(prev_tt), "%s", tool_tip_title);
        ui_emit_signal("NewToolTip");
    }
    ui_emit_signal("NewIcon");
    return G_SOURCE_CONTINUE;
}

static void ui_render_icon(void) {
    const int S = 24;
    const double C = S / 2.0;
    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
            S, S);
    cairo_t* cr = cairo_create(surf);
    // shroud ring (fully opaque colors: the pixmap travels as plain ARGB32,
    // so semi-transparent colors would look darker on some hosts)
    cairo_set_source_rgba(cr, 0.15, 0.15, 0.15, 1.0);
    cairo_set_line_width(cr, 1.4);
    cairo_arc(cr, C, C, 10.4, 0.0, 2.0 * M_PI);
    cairo_stroke(cr);
    // rotating blades
    cairo_translate(cr, C, C);
    cairo_rotate(cr, blade_rotation * M_PI / 180.0);
    for (int i = 0; i < 3; i++) {
        cairo_save(cr);
        cairo_rotate(cr, i * 2.0 * M_PI / 3.0);
        cairo_move_to(cr, 0.0, 0.0);
        cairo_curve_to(cr, 4.8, -2.6, 4.8, -6.6, 0.0, -8.6);
        cairo_curve_to(cr, -2.6, -7.2, -3.6, -3.6, 0.0, 0.0);
        cairo_close_path(cr);
        cairo_set_source_rgba(cr, 0.93, 0.93, 0.93, 1.0);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 0.15, 0.15, 0.15, 1.0);
        cairo_set_line_width(cr, 1.4);
        cairo_stroke(cr);
        cairo_restore(cr);
    }
    // hub
    cairo_arc(cr, 0.0, 0.0, 2.2, 0.0, 2.0 * M_PI);
    cairo_set_source_rgba(cr, 0.93, 0.93, 0.93, 1.0);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.15, 0.15, 0.15, 1.0);
    cairo_stroke(cr);
    cairo_destroy(cr);
    // copy the cairo buffer into a big-endian ARGB32 pixmap buffer (the SNI
    // IconPixmap format is network byte order); ping-pong buffers keep the
    // previous frame valid while hosts read the current one
    int new_idx = pixmap_buf_idx ^ 1;
    guint8* dst = pixmap_buf[new_idx];
    if (dst == NULL)
        dst = pixmap_buf[new_idx] = g_malloc((gsize) S * S * 4);
    const guint8* src = cairo_image_surface_get_data(surf);
    int stride = cairo_image_surface_get_stride(surf);
    for (int y = 0; y < S; y++) {
        const guint32* row = (const guint32*) (src + (gsize) y * stride);
        guint32* out = (guint32*) (dst + (gsize) y * S * 4);
        for (int x = 0; x < S; x++)
            out[x] = GUINT32_TO_BE(row[x]);
    }
    cairo_surface_destroy(surf);
    pixmap_buf_idx = new_idx;
}

static void ui_emit_signal(const char* member) {
    if (bus == NULL)
        return;
    GError* err = NULL;
    g_dbus_connection_emit_signal(bus, NULL, ITEM_PATH,
            "org.kde.StatusNotifierItem", member, NULL, &err);
    if (err != NULL)
        g_error_free(err);
}

static gboolean ui_register_item(gpointer user_data) {
    static int registered = 0;
    (void) user_data;
    if (bus == NULL || registered)
        return G_SOURCE_REMOVE;
    GError* err = NULL;
    GVariant* res = g_dbus_connection_call_sync(bus,
            "org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher",
            "org.kde.StatusNotifierWatcher", "RegisterStatusNotifierItem",
            g_variant_new("(s)", ITEM_PATH), NULL, G_DBUS_CALL_FLAGS_NONE,
            1000, NULL, &err);
    if (res != NULL) {
        g_variant_unref(res);
        registered = 1;
        printf("StatusNotifierItem registered\n");
        fflush(stdout);
        return G_SOURCE_REMOVE;
    }
    printf("unable to register with StatusNotifierWatcher (%s), retrying\n",
            err != NULL ? err->message : "?");
    fflush(stdout);
    if (err != NULL)
        g_error_free(err);
    return G_SOURCE_CONTINUE;
}

static void ui_build_menu(void) {
    menu_server = dbusmenu_server_new(MENU_PATH);
    DbusmenuMenuitem* root = dbusmenu_menuitem_new();
    for (int i = 0; i < menuitem_count; i++) {
        DbusmenuMenuitem* item = dbusmenu_menuitem_new();
        if (strlen(menuitems[i].label) == 0) {
            dbusmenu_menuitem_property_set(item, "type", "separator");
        } else {
            dbusmenu_menuitem_property_set(item, "label", menuitems[i].label);
            dbusmenu_menuitem_property_set_bool(item, "enabled", TRUE);
            g_signal_connect(G_OBJECT(item), "item-activated",
                    G_CALLBACK(sni_on_item_activated), GINT_TO_POINTER(i));
        }
        dbusmenu_menuitem_child_append(root, item);
        menuitems[i].widget = item;
    }
    dbusmenu_server_set_root(menu_server, root);
}

static void sni_on_item_activated(DbusmenuMenuitem* item, guint timestamp,
        gpointer user_data) {
    (void) item;
    (void) timestamp;
    int i = GPOINTER_TO_INT(user_data);
    if (i >= 0 && i < menuitem_count && menuitems[i].callback != NULL)
        ((void (*)(long)) menuitems[i].callback)(menuitems[i].option);
}

static void sni_on_method_call(GDBusConnection* connection,
        const gchar* sender, const gchar* object_path,
        const gchar* interface_name, const gchar* method_name,
        GVariant* parameters, GDBusMethodInvocation* invocation,
        gpointer user_data) {
    (void) connection;
    (void) sender;
    (void) object_path;
    (void) interface_name;
    (void) parameters;
    (void) user_data;
    // Activate (left click) is a no-op: hosts that prefer the menu show it
    // themselves because ItemIsMenu is set. ContextMenu is also a no-op:
    // the Menu property points to our dbusmenu object.
    if (strcmp(method_name, "Activate") == 0
            || strcmp(method_name, "SecondaryActivate") == 0
            || strcmp(method_name, "ContextMenu") == 0
            || strcmp(method_name, "Scroll") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
}

/* Builds the IconPixmap array "(iiay)" from the current frame. cairo ARGB32
 * data is premultiplied little-endian; the SNI format wants big-endian ARGB32
 * (identical here because all colors are fully opaque). */
static GVariant* ui_pixmap_variant(void) {
    guint8* data = pixmap_buf[pixmap_buf_idx];
    if (data == NULL)
        return g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0);
    GVariant* ay = g_variant_new_from_data(G_VARIANT_TYPE("ay"), data,
            (gsize) 24 * 24 * 4, TRUE, NULL, NULL);
    GVariant* entry = g_variant_new("(ii@ay)", 24, 24, ay);
    return g_variant_new_array(G_VARIANT_TYPE("(iiay)"), &entry, 1);
}

static GVariant* sni_get_property(GDBusConnection* connection,
        const gchar* sender, const gchar* object_path,
        const gchar* interface_name, const gchar* property_name,
        GError** error, gpointer user_data) {
    (void) connection;
    (void) sender;
    (void) object_path;
    (void) interface_name;
    (void) user_data;
    if (strcmp(property_name, "Id") == 0)
        return g_variant_new_string(NAME);
    if (strcmp(property_name, "Category") == 0)
        return g_variant_new_string("Hardware");
    if (strcmp(property_name, "Title") == 0)
        return g_variant_new_string("Clevo");
    if (strcmp(property_name, "Status") == 0)
        return g_variant_new_string("Passive");
    if (strcmp(property_name, "WindowId") == 0)
        return g_variant_new_int32(0);
    if (strcmp(property_name, "ItemIsMenu") == 0)
        return g_variant_new_boolean(TRUE);
    if (strcmp(property_name, "Menu") == 0)
        return g_variant_new_object_path(MENU_PATH);
    if (strcmp(property_name, "IconName") == 0
            || strcmp(property_name, "OverlayIconName") == 0
            || strcmp(property_name, "AttentionIconName") == 0
            || strcmp(property_name, "AttentionMovieName") == 0
            || strcmp(property_name, "IconThemePath") == 0)
        return g_variant_new_string("");
    if (strcmp(property_name, "IconPixmap") == 0)
        return ui_pixmap_variant();
    if (strcmp(property_name, "OverlayIconPixmap") == 0
            || strcmp(property_name, "AttentionIconPixmap") == 0)
        return g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0);
    if (strcmp(property_name, "ToolTip") == 0)
        return g_variant_new("(s@a(iiay)ss)", "", ui_pixmap_variant(),
                tool_tip_title, tool_tip_desc);
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
            "No such property: %s", property_name);
    return NULL;
}

static void ui_command_set_fan(long fan_duty) {
    int fan_duty_val = (int) fan_duty;
    if (fan_duty_val == 0) {
        printf("clicked on fan duty auto\n");
        share_info->auto_duty = 1;
        share_info->auto_duty_val = 0;
        share_info->manual_next_fan_duty = 0;
    } else {
        printf("clicked on fan duty: %d\n", fan_duty_val);
        share_info->auto_duty = 0;
        share_info->auto_duty_val = 0;
        share_info->manual_next_fan_duty = fan_duty_val;
    }
    ui_toggle_menuitems(fan_duty_val);
}

static void ui_command_quit(gchar* command) {
    (void) command;
    printf("clicked on quit\n");
    if (ui_loop != NULL)
        g_main_loop_quit(ui_loop);
}

static void ui_toggle_menuitems(int fan_duty) {
    for (int i = 0; i < menuitem_count; i++) {
        if (menuitems[i].widget == NULL)
            continue;
        int enabled;
        if (fan_duty == 0)
            enabled = menuitems[i].type != AUTO;
        else
            enabled = menuitems[i].type != MANUAL
                    || (int) menuitems[i].option != fan_duty;
        dbusmenu_menuitem_property_set_bool(menuitems[i].widget, "enabled",
                enabled ? TRUE : FALSE);
    }
}

static int ec_init(void) {
    if (ioperm(EC_DATA, 1, 1) != 0)
        return EXIT_FAILURE;
    if (ioperm(EC_SC, 1, 1) != 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

static void ec_on_sigterm(int signum) {
    printf("ec on signal: %s\n", strsignal(signum));
    if (share_info != NULL)
        share_info->exit = 1;
}

static int ec_auto_duty_adjust(void) {
    int temp = MAX(share_info->cpu_temp, share_info->gpu_temp);
    int duty = share_info->fan_duty;
    //
    if (temp >= 80 && duty < 100)
        return 100;
    if (temp >= 70 && duty < 90)
        return 90;
    if (temp >= 60 && duty < 80)
        return 80;
    if (temp >= 50 && duty < 70)
        return 70;
    if (temp >= 40 && duty < 60)
        return 60;
    if (temp >= 30 && duty < 50)
        return 50;
    if (temp >= 20 && duty < 40)
        return 40;
    if (temp >= 10 && duty < 30)
        return 30;
    //
    if (temp <= 15 && duty > 30)
        return 30;
    if (temp <= 25 && duty > 40)
        return 40;
    if (temp <= 35 && duty > 50)
        return 50;
    if (temp <= 45 && duty > 60)
        return 60;
    if (temp <= 55 && duty > 70)
        return 70;
    if (temp <= 65 && duty > 80)
        return 80;
    if (temp <= 75 && duty > 90)
        return 90;
    //
    return 0;
}

static int ec_query_cpu_temp(void) {
    return ec_io_read(EC_REG_CPU_TEMP);
}

static int ec_query_gpu_temp(void) {
    return ec_io_read(EC_REG_GPU_TEMP);
}

static int ec_query_fan_duty(void) {
    int raw_duty = ec_io_read(EC_REG_FAN_DUTY);
    return calculate_fan_duty(raw_duty);
}

static int ec_query_fan_rpms(void) {
    int raw_rpm_hi = ec_io_read(EC_REG_FAN_RPMS_HI);
    int raw_rpm_lo = ec_io_read(EC_REG_FAN_RPMS_LO);
    return calculate_fan_rpms(raw_rpm_hi, raw_rpm_lo);
}

static int ec_write_fan_duty(int duty_percentage) {
    if (duty_percentage < 60 || duty_percentage > 100) {
        printf("Wrong fan duty to write: %d\n", duty_percentage);
        return EXIT_FAILURE;
    }
    double v_d = ((double) duty_percentage) / 100.0 * 255.0;
    int v_i = (int) v_d;
    return ec_io_do(0x99, 0x01, v_i);
}

static int ec_io_wait(const uint32_t port, const uint32_t flag,
        const char value) {
    uint8_t data = inb(port);
    int i = 0;
    while ((((data >> flag) & 0x1) != value) && (i++ < 100)) {
        usleep(1000);
        data = inb(port);
    }
    if (i >= 100) {
        printf("wait_ec error on port 0x%x, data=0x%x, flag=0x%x, value=0x%x\n",
                port, data, flag, value);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static uint8_t ec_io_read(const uint32_t port) {
    ec_io_wait(EC_SC, IBF, 0);
    outb(EC_SC_READ_CMD, EC_SC);

    ec_io_wait(EC_SC, IBF, 0);
    outb(port, EC_DATA);

    //wait_ec(EC_SC, EC_SC_IBF_FREE);
    ec_io_wait(EC_SC, OBF, 1);
    uint8_t value = inb(EC_DATA);

    return value;
}

static int ec_io_do(const uint32_t cmd, const uint32_t port,
        const uint8_t value) {
    ec_io_wait(EC_SC, IBF, 0);
    outb(cmd, EC_SC);

    ec_io_wait(EC_SC, IBF, 0);
    outb(port, EC_DATA);

    ec_io_wait(EC_SC, IBF, 0);
    outb(value, EC_DATA);

    return ec_io_wait(EC_SC, IBF, 0);
}

static int calculate_fan_duty(int raw_duty) {
    return (int) ((double) raw_duty / 255.0 * 100.0);
}

static int calculate_fan_rpms(int raw_rpm_high, int raw_rpm_low) {
    int raw_rpm = (raw_rpm_high << 8) + raw_rpm_low;
    return raw_rpm > 0 ? (2156220 / raw_rpm) : 0;
}

static int check_proc_instances(const char* proc_name) {
    int proc_name_len = strlen(proc_name);
    pid_t this_pid = getpid();
    DIR* dir;
    if (!(dir = opendir("/proc"))) {
        perror("can't open /proc");
        return -1;
    }
    int instance_count = 0;
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        char* endptr;
        long lpid = strtol(ent->d_name, &endptr, 10);
        if (*endptr != '\0')
            continue;
        if (lpid == this_pid)
            continue;
        char buf[512];
        snprintf(buf, sizeof(buf), "/proc/%ld/comm", lpid);
        FILE* fp = fopen(buf, "r");
        if (fp) {
            if (fgets(buf, sizeof(buf), fp) != NULL) {
                if ((buf[proc_name_len] == '\n' || buf[proc_name_len] == '\0')
                        && strncmp(buf, proc_name, proc_name_len) == 0) {
                    fprintf(stderr, "Process: %ld\n", lpid);
                    instance_count += 1;
                }
            }
            fclose(fp);
        }
    }
    closedir(dir);
    return instance_count;
}

static void get_time_string(char* buffer, size_t max, const char* format) {
    time_t timer;
    struct tm tm_info;
    time(&timer);
    localtime_r(&timer, &tm_info);
    strftime(buffer, max, format, &tm_info);
}

static void signal_term(__sighandler_t handler) {
    signal(SIGHUP, handler);
    signal(SIGINT, handler);
    signal(SIGQUIT, handler);
    signal(SIGPIPE, handler);
    signal(SIGALRM, handler);
    signal(SIGTERM, handler);
    signal(SIGUSR1, handler);
    signal(SIGUSR2, handler);
}
