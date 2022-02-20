#ifndef __GTK_PRINT_BACKEND_CPDB_H__
#define __GTK_PRINT_BACKEND_CPDB_H__

#include <glib-object.h>
#include "gtkprintbackendprivate.h"
#include <cpdb-libs-frontend.h>
#include <cups/ppd.h>
#include <gtk/gtkunixprint.h>
#include <gtk/gtkprinterprivate.h>


G_BEGIN_DECLS

#define GTK_TYPE_PRINTER_CPDB (gtk_printer_cpdb_get_type ())
#define GTK_TYPE_PRINT_BACKEND_CPDB (gtk_print_backend_cpdb_get_type ())
#define GTK_PRINT_BACKEND_CPDB(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_PRINT_BACKEND_CPDB, GtkPrintBackendCpdb))
#define GTK_IS_PRINT_BACKEND_CPDB(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_PRINT_BACKEND_CPDB))
#define GTK_PRINTER_CPDB(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_PRINTER_CPDB, GtkPrinterCpdb))
#define GTK_PRINT_BACKEND_CPDB_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_PRINT_BACKEND_CPDB, GtkPrintBackendCpdbClass))
#define GTK_IS_PRINT_BACKEND_CPDB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_PRINT_BACKEND_CPDB))
#define GTK_PRINT_BACKEND_CPDB_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_PRINT_BACKEND_CPDB, GtkPrintBackendCpdbClass))

/*
GType gtk_printer_cpdb_get_type (void)
{
  return gtk_printer_cpdb_type;
}

GType gtk_printer_cpdb_get_type (void) G_GNUC_CONST;
*/

typedef struct _GtkPrintBackendCpdbClass
{
  GtkPrintBackendClass parent_class;
} GtkPrintBackendCpdbClass;

typedef struct _GtkPrintBackendCpdb
{
  GtkPrintBackend parent_instance;
} GtkPrintBackendCpdb;

typedef enum
{
  GTK_CPDB_ERROR_HTTP,
  GTK_CPDB_ERROR_IPP,
  GTK_CPDB_ERROR_IO,
  GTK_CPDB_ERROR_AUTH,
  GTK_CPDB_ERROR_GENERAL
} GtkCpdbErrorType;

typedef enum
{
  GTK_CPDB_POST,
  GTK_CPDB_GET
} GtkCpdbRequestType;

typedef struct _GtkPrinterCpdbClass
{
  GtkPrinterClass parent_class;

} GtkPrinterCpdbClass;

typedef enum
{
  GTK_CPDB_CONNECTION_AVAILABLE,
  GTK_CPDB_CONNECTION_NOT_AVAILABLE,
  GTK_CPDB_CONNECTION_IN_PROGRESS
} GtkCpdbConnectionState;

typedef struct _GtkCpdbConnectionTest
{
  GtkCpdbConnectionState at_init;
  http_addrlist_t *addrlist;
  http_addrlist_t *current_addr;
  http_addrlist_t *last_wrong_addr;
  int socket;
} GtkCpdbConnectionTest;

typedef enum
{
  GTK_CPDB_HTTP_IDLE,
  GTK_CPDB_HTTP_RE,
  GTK_CPDB_HTTP_WRITE
} GtkCpdbPollState;

typedef enum
{
  GTK_CPDB_PASSWORD_NONE,
  GTK_CPDB_PASSWORD_REQUESTED,
  GTK_CPDB_PASSWORD_HAS,
  GTK_CPDB_PASSWORD_APPLIED,
  GTK_CPDB_PASSWORD_NOT_VALID
} GtkCpdbPasswordState;

typedef struct _GtkPrinterCpdb
{
  GtkPrinter parent_instance;

  char *device_uri;
  char *original_device_uri;
  char *printer_uri;
  char *hostname;
  int port;
  char **auth_info_required;
  char *original_hostname;
  char *original_resource;
  int original_port;
  gboolean request_original_uri; /* Request PPD from original host */

  ipp_pstate_t state;
  gboolean reading_ppd;
  char *ppd_name;
  ppd_file_t *ppd_file;

  char *media_default;
  GList *media_supported;
  GList *media_size_supported;
  int media_bottom_margin_default;
  int media_top_margin_default;
  int media_left_margin_default;
  int media_right_margin_default;
  gboolean media_margin_default_set;
  char *sides_default;
  GList *sides_supported;
  char *output_bin_default;
  GList *output_bin_supported;

  char *default_cover_before;
  char *default_cover_after;

  int default_number_up;

  gboolean remote;
  guint get_remote_ppd_poll;
  int get_remote_ppd_attempts;
  GtkCpdbConnectionTest *remote_cpdb_connection_test;


#ifdef HAVE_COLORD
  CdClient *colord_client;
  CdDevice *colord_device;
  CdProfile *colord_profile;
  GCancellable *colord_cancellable;
  char *colord_title;
  char *colord_qualifier;
#endif

  gboolean avahi_browsed;
  char *avahi_name;
  char *avahi_type;
  char *avahi_domain;

  guchar ipp_version_major;
  guchar ipp_version_minor;
  gboolean supports_copies;
  gboolean supports_collate;
  gboolean supports_number_up;
  char **covers;
  int number_of_covers;

} GtkPrinterCpdb;

typedef struct _GtkCpdbResult
{
  char *error_msg;
  ipp_t *ipp_response;
  GtkCpdbErrorType error_type;

  /* some error types like HTTP_ERROR have a status and a code */
  int error_status;
  int error_code;

  guint is_error : 1;
  guint is_ipp_response : 1;
} GtkCpdbResult;

typedef struct _GtkCpdbRequest
{
  GtkCpdbRequestType type;

  http_t *http;
  http_status_t last_status;
  ipp_t *ipp_request;

  char *server;
  char *resource;
  GIOChannel *data_io;
  int attempts;

  GtkCpdbResult *result;

  int state;
  GtkCpdbPollState poll_state;
  guint64 bytes_received;

  char *password;
  char *username;

  int own_http : 1;
  int need_password : 1;
  int need_auth_info : 1;
  char **auth_info_required;
  char **auth_info;
  GtkCpdbPasswordState password_state;
} GtkCpdbRequest;

typedef struct _CpdbPrintStreamData
{
  GtkPrintJobCompleteFunc callback;
  GtkPrintJob *job;
  gpointer user_data;
  GDestroyNotify dnotify;
  http_t *http;
} CpdbPrintStreamData;

typedef struct _CpdbOptionData
{
  GtkCpdbRequest *request;
  GtkPageSetup *page_setup;
  GtkPrinterCpdb *printer;
} CpdbOptionsData;

typedef void (*GtkPrintCpdbResponseCallbackFunc) (GtkPrintBackend *print_backend,
                                                  GtkCpdbResult *result,
                                                  gpointer user_data);

static GObjectClass *backend_parent_class;
static FrontendObj *frontendObj;
static GtkPrintBackend *gtkPrintBackend;

GtkPrintBackend *gtk_print_backend_cpdb_new (void);
GType gtk_print_backend_cpdb_get_type (void) G_GNUC_CONST;
G_DEFINE_DYNAMIC_TYPE (GtkPrintBackendCpdb, gtk_print_backend_cpdb, GTK_TYPE_PRINT_BACKEND)

static void add_cpdb_options (const char *key,
                              const char *value,
                              gpointer user_data);

static void cpdb_get_printer_list (GtkPrintBackend *backend);

static void cpdb_printer_get_settings_from_options (GtkPrinter *printer,
                                                    GtkPrinterOptionSet *options,
                                                    GtkPrintSettings *settings);
static GtkPrinterOptionSet *cpdb_printer_get_options (GtkPrinter *printer,
                                                      GtkPrintSettings *settings,
                                                      GtkPageSetup *page_setup,
                                                      GtkPrintCapabilities capabilities);
static void cpdb_printer_prepare_for_print (GtkPrinter *printer,
                                            GtkPrintJob *print_job,
                                            GtkPrintSettings *settings,
                                            GtkPageSetup *page_setup);
static cairo_surface_t *cpdb_printer_create_cairo_surface (GtkPrinter *printer,
                                                           GtkPrintSettings *settings,
                                                           double width,
                                                           double height,
                                                           GIOChannel *cache_io);
static void gtk_print_backend_cpdb_print_stream (GtkPrintBackend *print_backend,
                                                 GtkPrintJob *job,
                                                 GIOChannel *data_io,
                                                 GtkPrintJobCompleteFunc callback,
                                                 gpointer user_data,
                                                 GDestroyNotify dnotify);

static int add_printer_callback (PrinterObj *p);
static int remove_printer_callback (PrinterObj *p);

/*
 * gtkprintercpdb.h
 */
void                     gtk_printer_cpdb_register_type (GTypeModule     *module);

static void gtk_printer_cpdb_init       (GtkPrinterCpdb      *printer);
static void gtk_printer_cpdb_class_init (GtkPrinterCpdbClass *class);
static void gtk_printer_cpdb_finalize   (GObject             *object);

/*
 * gtkcpdbutils.h
 */

#define GTK_CPDB_REQUEST_START 0
#define GTK_CPDB_REQUEST_DONE 500

void                    gtk_cpdb_request_ipp_add_string    (GtkCpdbRequest     *request,
							    ipp_tag_t           group,
							    ipp_tag_t           tag,
							    const char         *name,
							    const char         *charset,
							    const char         *value);

void                    gtk_cpdb_request_set_ipp_version   (GtkCpdbRequest     *request,
							    int                 major,
							    int                 minor);

GtkCpdbRequest        * gtk_cpdb_request_new_with_username (http_t             *connection,
							    GtkCpdbRequestType  req_type,
							    int                 operation_id,
							    GIOChannel         *data_io,
							    const char         *server,
							    const char         *resource,
							    const char         *username);

void                    gtk_cpdb_request_encode_option     (GtkCpdbRequest     *request,
						            const char         *option,
							    const char         *value);


G_END_DECLS

#endif
