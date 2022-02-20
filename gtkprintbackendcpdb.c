#include "gtkprintbackendcpdb.h"
#include "config.h"
#include "gtkprinterprivate.h"
#include <cpdb-libs-frontend.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include "gtkintl.h"
#include "gtkprintercups.h"
// #include <cups/language.h>


#ifdef HAVE_COLORD
#include <colord.h>
#endif


void
g_io_module_load (GIOModule *module)
{
  g_type_module_use (G_TYPE_MODULE (module));

  gtk_print_backend_cpdb_register_type (G_TYPE_MODULE (module));

  g_io_extension_point_implement (GTK_PRINT_BACKEND_EXTENSION_POINT_NAME,
                                  GTK_TYPE_PRINT_BACKEND_CPDB,
                                  "cpdb",
                                  10);
}

void
g_io_module_unload (GIOModule *module)
{
}

char **
g_io_module_query (void)
{
  char *eps[] = {
    (char *) GTK_PRINT_BACKEND_EXTENSION_POINT_NAME,
    NULL
  };

  return g_strdupv (eps);
}


static GType gtk_printer_cpdb_type = 0;

/*
GType
gtk_printer_cpdb_get_type (void)
{
  return gtk_printer_cpdb_type;
}
*/

enum
{
  PROP_0,
  PROP_PROFILE_TITLE
};

typedef struct
{
  GSource source;

  http_t *http;
  GtkCpdbRequest *request;
  GtkCpdbPollState poll_state;
  GPollFD *data_poll;
  GtkPrintBackendCpdb *backend;
  GtkPrintCpdbResponseCallbackFunc callback;
  gpointer callback_data;

} GtkPrintCpdbDispatchWatch;

GtkPrintBackend *
gtk_print_backend_cpdb_new (void)
{
  return g_object_new (GTK_TYPE_PRINT_BACKEND_CPDB, NULL);
}

static void
gtk_print_backend_cpdb_class_init (GtkPrintBackendCpdbClass *class)
{
  GtkPrintBackendClass *backend_class = GTK_PRINT_BACKEND_CLASS (class);

  backend_parent_class = g_type_class_peek_parent (class);

  backend_class->request_printer_list = cpdb_get_printer_list;
  backend_class->printer_create_cairo_surface = cpdb_printer_create_cairo_surface;
  backend_class->printer_get_options = cpdb_printer_get_options;
  backend_class->printer_get_settings_from_options = cpdb_printer_get_settings_from_options;
  backend_class->printer_prepare_for_print = cpdb_printer_prepare_for_print;
}

void
gtk_printer_cpdb_register_type (GTypeModule *module)
{
  const GTypeInfo object_info = {
    sizeof (GtkPrinterCupsClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) gtk_printer_cpdb_class_init,
    NULL, /* class_finalize */
    NULL, /* class_data */
    sizeof (GtkPrinterCpdb),
    0, /* n_preallocs */
    (GInstanceInitFunc) gtk_printer_cpdb_init,
  };

  gtk_printer_cpdb_type = g_type_module_register_type (module,
                                                       GTK_TYPE_PRINTER,
                                                       "GtkPrinterCpdb",
                                                       &object_info, 0);
}

static void
gtk_print_backend_cpdb_class_finalize (GtkPrintBackendCpdbClass *class)
{

}

static void
gtk_print_backend_cpdb_init (GtkPrintBackendCpdb *backend)
{
  gtkPrintBackend = GTK_PRINT_BACKEND (backend);
  frontendObj = get_new_FrontendObj(NULL, add_printer_callback, remove_printer_callback);
  connect_to_dbus(frontendObj);
}

static void
cpdb_get_printer_list(GtkPrintBackend *backend)
{
  gtkPrintBackend = GTK_PRINT_BACKEND (backend);
  refresh_printer_list(frontendObj);
}


static void
cpdb_printer_get_settings_from_options (GtkPrinter *printer,
                                        GtkPrinterOptionSet *options,
                                        GtkPrintSettings *settings)
{
  GtkPrinterOption *option;

  option = gtk_printer_option_set_lookup(options, "gtk-n-up");
  if (option)
    gtk_print_settings_set ( settings, GTK_PRINT_SETTINGS_NUMBER_UP, option->value );

  option = gtk_printer_option_set_lookup( options, "gtk-n-up-layout");
  if (option)
    gtk_print_settings_set( settings, GTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT, option->value);


}


static GtkPrinterOptionSet *cpdb_printer_get_options (GtkPrinter *printer,
                          GtkPrintSettings *settings,
                          GtkPageSetup *page_setup,
                          GtkPrintCapabilities capabilities)
{
  GtkPrinterOption *option;
  GtkPrinterOptionSet *set = gtk_printer_option_set_new();

  // CPDB Option CPD_OPTION_NUMBER_UP
  const char *n_up[] = {"1", "2", "4", "6", "9", "16" };
  option = gtk_printer_option_new ("gtk-n-up", C_("printer option", "Pages per Sheet"), GTK_PRINTER_OPTION_TYPE_PICKONE);
  gtk_printer_option_choices_from_array (option, G_N_ELEMENTS (n_up), n_up, n_up);
  gtk_printer_option_set (option, "1");
  gtk_printer_option_set_add (set, option);

  // CPDB Option CPD_OPTION_JOB_PRIORITY
  const char *priority[] = {"100", "80", "50", "30" };
  const char *priority_display[] = {N_("Urgent"), N_("High"), N_("Medium"), N_("Low") };
  option = gtk_printer_option_new ("gtk-job-prio", _("Job Priority"), GTK_PRINTER_OPTION_TYPE_PICKONE);
  gtk_printer_option_choices_from_array (option, G_N_ELEMENTS (priority), priority, priority_display);
  gtk_printer_option_set (option, "50");
  gtk_printer_option_set_add (set, option);
  g_object_unref (option);

  // CPDB Option CPD_OPTION_SIDES
  const char *sides[] = {"one-sided", "two-sided-short", "two-sided-long"};
  const char *sides_display[] = {N_("One Sided"), N_("Long Edged (Standard)"), N_("Short Edged (Flip)")};
  option = gtk_printer_option_new ("gtk-duplex", _("Duplex Printing"), GTK_PRINTER_OPTION_TYPE_PICKONE);
  gtk_printer_option_choices_from_array (option, G_N_ELEMENTS (sides), sides, sides_display);
  gtk_printer_option_set (option, "one-sided");
  gtk_printer_option_set_add (set, option);
  g_object_unref (option);

  return set;
}

static void cpdb_printer_prepare_for_print (GtkPrinter *printer,
                                GtkPrintJob *print_job,
                                GtkPrintSettings *settings,
                                GtkPageSetup *page_setup)
{
  double scale;
  GtkPrintPages pages;
  GtkPageRange *ranges;
  GtkPageSet page_set;
  GtkPaperSize *paper_size;
  const char *ppd_paper_name;
  int n_ranges;


  pages = gtk_print_settings_get_print_pages (settings);
  gtk_print_job_set_pages (print_job, pages);
  if ( pages == GTK_PRINT_PAGES_RANGES )
    ranges = gtk_print_settings_get_page_ranges ( settings, &n_ranges );
  else
    {
      ranges = NULL;
      n_ranges = 0;
    }
  gtk_print_job_set_page_ranges (print_job, ranges, n_ranges);

  gtk_print_job_set_collate (print_job, gtk_print_settings_get_collate (settings));
  gtk_print_job_set_reverse (print_job, gtk_print_settings_get_reverse (settings));
  gtk_print_job_set_num_copies (print_job, gtk_print_settings_get_n_copies (settings));
  gtk_print_job_set_n_up (print_job, gtk_print_settings_get_number_up (settings));
  gtk_print_job_set_n_up_layout (print_job, gtk_print_settings_get_number_up_layout (settings));
  gtk_print_job_set_rotate (print_job, TRUE);

  scale = gtk_print_settings_get_scale (settings);
  if (scale != 100.0)
    gtk_print_job_set_scale (print_job, scale / 100.0);

  page_set = gtk_print_settings_get_page_set (settings);
  if (page_set == GTK_PAGE_SET_EVEN)
    gtk_print_settings_set (settings, "cpdb-page-set", "even");
  else if (page_set == GTK_PAGE_SET_ODD)
    gtk_print_settings_set (settings, "cpdb-page-set", "odd");
  gtk_print_job_set_page_set (print_job, GTK_PAGE_SET_ALL);

  paper_size = gtk_page_setup_get_paper_size (page_setup);
  ppd_paper_name = gtk_paper_size_get_ppd_name (paper_size);

  if (ppd_paper_name != NULL)
    gtk_print_settings_set (settings, "cpdb-PageSize", ppd_paper_name);
  else if (gtk_paper_size_is_ipp (paper_size))
    gtk_print_settings_set (settings, "cpdb-media", gtk_paper_size_get_name (paper_size));
  else
    {
      char width[G_ASCII_DTOSTR_BUF_SIZE];
      char height[G_ASCII_DTOSTR_BUF_SIZE];
      char *custom_name;

      g_ascii_formatd (width, sizeof (width), "%.2f", gtk_paper_size_get_width (paper_size, GTK_UNIT_POINTS));
      g_ascii_formatd (height, sizeof (height), "%.2f", gtk_paper_size_get_height (paper_size, GTK_UNIT_POINTS));
      custom_name = g_strdup_printf (("Custom.%sx%s"), width, height);
      gtk_print_settings_set (settings, "cpdb-PageSize", custom_name);
      g_free (custom_name);
    }

  if (gtk_print_settings_get_number_up (settings) > 1)
    {
      GtkNumberUpLayout  layout = gtk_print_settings_get_number_up_layout (settings);
      GEnumClass        *enum_class;
      GEnumValue        *enum_value;

      switch (gtk_page_setup_get_orientation (page_setup))
        {
        case GTK_PAGE_ORIENTATION_LANDSCAPE:
          if (layout < 4)
            layout = layout + 2 + 4 * (1 - layout / 2);
          else
            layout = layout - 3 - 2 * (layout % 2);
          break;
        case GTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
          layout = (layout + 3 - 2 * (layout % 2)) % 4 + 4 * (layout / 4);
          break;
        case GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
          if (layout < 4)
            layout = layout + 5 - 2 * (layout % 2);
          else
            layout = layout - 6 + 4 * (1 - (layout - 4) / 2);
          break;

        case GTK_PAGE_ORIENTATION_PORTRAIT:
        default:
          break;
        }

      enum_class = g_type_class_ref (GTK_TYPE_NUMBER_UP_LAYOUT);
      enum_value = g_enum_get_value (enum_class, layout);
      gtk_print_settings_set (settings, "cpdb-number-up-layout", enum_value->value_nick);
      g_type_class_unref (enum_class);
    }

}

/*
static GSourceFuncs _cpdb_dispatch_watch_funcs = {
  cpdb_dispatch_watch_prepare,
  cpdb_dispatch_watch_check,
  cpdb_dispatch_watch_dispatch,
  cpdb_dispatch_watch_finalize
};


static void
cpdb_request_execute (GtkPrintBackendCpdb *print_backend,
                      GtkCpdbRequest *request,
                      GtkPrintCpdbResponseCallbackFunc callback,
                      gpointer user_data,
                      GDestroyNotify notify)
{
  GtkPrintCpdbDispatchWatch *dispatch;

  dispatch = (GtkPrintCpdbDispatchWatch *) g_source_new (&_cpdb_dispatch_watch_funcs,
                                                         sizeof (GtkPrintCpdbDispatchWatch));
  g_source_set_name (&dispatch->source, "GTK CPDB backend");

  GTK_NOTE (PRINTING,
            g_print (" CPDB: %s <source %p> - Executing cpdb request on server '%s' and resource '%s'\n", G_STRFUNC, dispatch, request->server, request->resource));

  dispatch->request = request;
  dispatch->backend = g_object_ref (print_backend);
  dispatch->poll_state = GTK_CPDB_HTTP_IDLE;
  dispatch->data_poll = NULL;
  dispatch->callback = NULL;
  dispatch->callback_data = NULL;

  print_backend->requests = g_list_prepend (print_backend->requests, dispatch);

  g_source_set_callback ((GSource *) dispatch, (GSourceFunc) callback, user_data, notify);

  if (request->need_auth_info)
    {
      dispatch->callback = callback;
      dispatch->callback_data = user_data;
      lookup_auth_info (dispatch);
    }
  else
    {
      g_source_attach ((GSource *) dispatch, NULL);
      g_source_unref ((GSource *) dispatch);
    }
}
*/

static cairo_surface_t *
cpdb_printer_create_cairo_surface (GtkPrinter *printer,
                                   GtkPrintSettings *settings,
                                   double width,
                                   double height,
                                   GIOChannel *cache_io)
{
  cairo_surface_t *surface;
  return surface;
}

#define UNSIGNED_FLOAT_REGEX "([0-9]+([.,][0-9]*)?|[.,][0-9]+)([e][+-]?[0-9]+)?"
#define SIGNED_FLOAT_REGEX "[+-]?"UNSIGNED_FLOAT_REGEX
#define SIGNED_INTEGER_REGEX "[+-]?([0-9]+)"

static void
add_cpdb_options (const char *key,
                  const char *value,
                  gpointer user_data)
{
  CpdbOptionsData *data = (CpdbOptionsData *) user_data;
  GtkCpdbRequest *request = data->request;
  GtkPrinterCpdb *printer = data->printer;
  gboolean custom_value = FALSE;
  char *new_value = NULL;
  int i;

  if (!key || !value)
    return;

  if (!g_str_has_prefix (key, "cpdb-"))
    return;

  if (strcmp (value, "gtk-ignore-value") == 0)
    return;

  key = key + strlen ("cpdb-");

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  if (printer && printer->ppd_file && !g_str_has_prefix (value, "Custom."))
    {
      ppd_coption_t *coption;
      gboolean found = FALSE;
      gboolean custom_values_enabled = FALSE;

      coption = ppdFindCustomOption (printer->ppd_file, key);
      if (coption && coption->option)
        {
          for (i = 0; i < coption->option->num_choices; i++)
            {
              /* Are custom values enabled ? */
              if (g_str_equal (coption->option->choices[i].choice, "Custom"))
                custom_values_enabled = TRUE;

              /* Is the value among available choices ? */
              if (g_str_equal (coption->option->choices[i].choice, value))
                found = TRUE;
            }

          if (custom_values_enabled && !found)
            {
              /* Check syntax of the invalid choice to see whether
                 it could be a custom value */
              if (g_str_equal (key, "PageSize") ||
                  g_str_equal (key, "PageRegion"))
                {
                  /* Handle custom page sizes... */
                  if (g_regex_match_simple ("^" UNSIGNED_FLOAT_REGEX "x" UNSIGNED_FLOAT_REGEX "(cm|mm|m|in|ft|pt)?$", value, G_REGEX_CASELESS, 0))
                    custom_value = TRUE;
                  else
                    {
                      if (data->page_setup != NULL)
                        {
                          custom_value = TRUE;
                          new_value =
                              g_strdup_printf ("Custom.%.2fx%.2fmm",
                                               gtk_paper_size_get_width (gtk_page_setup_get_paper_size (data->page_setup), GTK_UNIT_MM),
                                               gtk_paper_size_get_height (gtk_page_setup_get_paper_size (data->page_setup), GTK_UNIT_MM));
                        }
                    }
                }
              else
                {
                  /* Handle other custom options... */
                  ppd_cparam_t *cparam;

                  cparam = (ppd_cparam_t *) cupsArrayFirst (coption->params);
                  if (cparam != NULL)
                    {
                      switch (cparam->type)
                        {
                        case PPD_CUSTOM_CURVE:
                        case PPD_CUSTOM_INVCURVE:
                        case PPD_CUSTOM_REAL:
                          if (g_regex_match_simple ("^" SIGNED_FLOAT_REGEX "$", value, G_REGEX_CASELESS, 0))
                            custom_value = TRUE;
                          break;

                        case PPD_CUSTOM_POINTS:
                          if (g_regex_match_simple ("^" SIGNED_FLOAT_REGEX "(cm|mm|m|in|ft|pt)?$", value, G_REGEX_CASELESS, 0))
                            custom_value = TRUE;
                          break;

                        case PPD_CUSTOM_INT:
                          if (g_regex_match_simple ("^" SIGNED_INTEGER_REGEX "$", value, G_REGEX_CASELESS, 0))
                            custom_value = TRUE;
                          break;

                        case PPD_CUSTOM_PASSCODE:
                        case PPD_CUSTOM_PASSWORD:
                        case PPD_CUSTOM_STRING:
                          custom_value = TRUE;
                          break;

                           #if (CUPS_VERSION_MAJOR >= 3) ||                            \
                               (CUPS_VERSION_MAJOR == 2 && CUPS_VERSION_MINOR >= 3) || \
                               (CUPS_VERSION_MAJOR == 2 && CUPS_VERSION_MINOR == 2 && CUPS_VERSION_PATCH >= 12)
                                                   case PPD_CUSTOM_UNKNOWN:
                           #endif

                        default:
                          custom_value = FALSE;
                        }
                    }
                }
            }
        }
    }

  G_GNUC_END_IGNORE_DEPRECATIONS

  /* Add "Custom." prefix to custom values if not already added. */
  if (custom_value)
    {
      if (new_value == NULL)
        new_value = g_strdup_printf ("Custom.%s", value);
      gtk_cpdb_request_encode_option (request, key, new_value);
      g_free (new_value);
    }
  else
    gtk_cpdb_request_encode_option (request, key, value);
}

typedef struct
{
  const char	*name;
  ipp_tag_t	value_tag;
} ipp_option_t;

static const ipp_option_t ipp_options[] = {
  { "blackplot",		IPP_TAG_BOOLEAN },
  { "brightness",		IPP_TAG_INTEGER },
  { "columns",			IPP_TAG_INTEGER },
  { "copies",			IPP_TAG_INTEGER },
  { "finishings",		IPP_TAG_ENUM },
  { "fitplot",			IPP_TAG_BOOLEAN },
  { "gamma",			IPP_TAG_INTEGER },
  { "hue",			IPP_TAG_INTEGER },
  { "job-k-limit",		IPP_TAG_INTEGER },
  { "job-page-limit",		IPP_TAG_INTEGER },
  { "job-priority",		IPP_TAG_INTEGER },
  { "job-quota-period",		IPP_TAG_INTEGER },
  { "landscape",		IPP_TAG_BOOLEAN },
  { "media",			IPP_TAG_KEYWORD },
  { "mirror",			IPP_TAG_BOOLEAN },
  { "natural-scaling",		IPP_TAG_INTEGER },
  { "number-up",		IPP_TAG_INTEGER },
  { "orientation-requested",	IPP_TAG_ENUM },
  { "page-bottom",		IPP_TAG_INTEGER },
  { "page-left",		IPP_TAG_INTEGER },
  { "page-ranges",		IPP_TAG_RANGE },
  { "page-right",		IPP_TAG_INTEGER },
  { "page-top",			IPP_TAG_INTEGER },
  { "penwidth",			IPP_TAG_INTEGER },
  { "ppi",			IPP_TAG_INTEGER },
  { "prettyprint",		IPP_TAG_BOOLEAN },
  { "printer-resolution",	IPP_TAG_RESOLUTION },
  { "print-quality",		IPP_TAG_ENUM },
  { "saturation",		IPP_TAG_INTEGER },
  { "scaling",			IPP_TAG_INTEGER },
  { "sides",			IPP_TAG_KEYWORD },
  { "wrap",			IPP_TAG_BOOLEAN },
  { "number-up-layout",		IPP_TAG_INTEGER }
};

static ipp_tag_t
_find_option_tag (const char *option)
{
  int lower_bound, upper_bound, num_options;
  int current_option;
  ipp_tag_t result;

  result = IPP_TAG_ZERO;

  lower_bound = 0;
  upper_bound = num_options = (int) G_N_ELEMENTS (ipp_options) - 1;

  while (1)
    {
      int match;
      current_option = (int) (((upper_bound - lower_bound) / 2) + lower_bound);

      match = strcasecmp (option, ipp_options[current_option].name);
      if (match == 0)
        {
	  result = ipp_options[current_option].value_tag;
	  return result;
	}
      else if (match < 0)
        {
          upper_bound = current_option - 1;
	}
      else
        {
          lower_bound = current_option + 1;
	}

      if (upper_bound == lower_bound && upper_bound == current_option)
        return result;

      if (upper_bound < 0)
        return result;

      if (lower_bound > num_options)
        return result;

      if (upper_bound < lower_bound)
        return result;
    }
}

void
gtk_cpdb_request_encode_option (GtkCpdbRequest *request,
                                const char *option,
                                const char *value)
{
  ipp_tag_t option_tag;

  g_return_if_fail (option != NULL);
  g_return_if_fail (value != NULL);

  option_tag = _find_option_tag (option);

  if (option_tag == IPP_TAG_ZERO)
    {
      option_tag = IPP_TAG_NAME;
      if (strcasecmp (value, "true") == 0 ||
          strcasecmp (value, "false") == 0)
        {
          option_tag = IPP_TAG_BOOLEAN;
        }
    }

  switch ((guint) option_tag)
    {
    case IPP_TAG_INTEGER:
    case IPP_TAG_ENUM:
      ippAddInteger (request->ipp_request,
                     IPP_TAG_JOB,
                     option_tag,
                     option,
                     strtol (value, NULL, 0));
      break;

    case IPP_TAG_BOOLEAN:
      {
        char b;

        if (strcasecmp (value, "true") == 0 ||
            strcasecmp (value, "on") == 0 ||
            strcasecmp (value, "yes") == 0)
          b = 1;
        else
          b = 0;

        ippAddBoolean (request->ipp_request,
                       IPP_TAG_JOB,
                       option,
                       b);

        break;
      }

    case IPP_TAG_RANGE:
      {
        char *s;
        int lower;
        int upper;

        if (*value == '-')
          {
            lower = 1;
            s = (char *) value;
          }
        else
          lower = strtol (value, &s, 0);

        if (*s == '-')
          {
            if (s[1])
              upper = strtol (s + 1, NULL, 0);
            else
              upper = 2147483647;
          }
        else
          upper = lower;

        ippAddRange (request->ipp_request,
                     IPP_TAG_JOB,
                     option,
                     lower,
                     upper);

        break;
      }

    case IPP_TAG_RESOLUTION:
      {
        char *s;
        int xres;
        int yres;
        ipp_res_t units;

        xres = strtol (value, &s, 0);

        if (*s == 'x')
          yres = strtol (s + 1, &s, 0);
        else
          yres = xres;

        if (strcasecmp (s, "dpc") == 0)
          units = IPP_RES_PER_CM;
        else
          units = IPP_RES_PER_INCH;

        ippAddResolution (request->ipp_request,
                          IPP_TAG_JOB,
                          option,
                          units,
                          xres,
                          yres);

        break;
      }

    default:
      {
        char *values;
        char *s;
        int in_quotes;
        char *next;
        GPtrArray *strings;

        values = g_strdup (value);
        strings = NULL;
        in_quotes = 0;

        for (s = values, next = s; *s != '\0'; s++)
          {
            if (in_quotes != 2 && *s == '\'')
              {
                /* skip quoted value */
                if (in_quotes == 0)
                  in_quotes = 1;
                else
                  in_quotes = 0;
              }
            else if (in_quotes != 1 && *s == '\"')
              {
                /* skip quoted value */
                if (in_quotes == 0)
                  in_quotes = 2;
                else
                  in_quotes = 0;
              }
            else if (in_quotes == 0 && *s == ',')
              {
                /* found delimiter, add to value array */
                *s = '\0';
                if (strings == NULL)
                  strings = g_ptr_array_new ();
                g_ptr_array_add (strings, next);
                next = s + 1;
              }
            else if (in_quotes == 0 && *s == '\\' && s[1] != '\0')
              {
                /* skip escaped character */
                s++;
              }
          }

        if (strings == NULL)
          {
            /* single value */
            ippAddString (request->ipp_request,
                          IPP_TAG_JOB,
                          option_tag,
                          option,
                          NULL,
                          value);
          }
        else
          {
            /* multiple values */

            /* add last value */
            g_ptr_array_add (strings, next);

            ippAddStrings (request->ipp_request,
                           IPP_TAG_JOB,
                           option_tag,
                           option,
                           strings->len,
                           NULL,
                           (const char **) strings->pdata);
            g_ptr_array_free (strings, TRUE);
          }

        g_free (values);
      }

      break;
    }
}

/*
static void
cpdb_request_execute (GtkPrintBackendCpdb *print_backend,
                      GtkCpdbRequest *request,
                      GtkPrintCpdbResponseCallbackFunc callback,
                      gpointer user_data,
                      GDestroyNotify notify)
{
  GtkPrintCpdbDispatchWatch *dispatch;

  dispatch = (GtkPrintCpdbDispatchWatch *) g_source_new (&_cpdb_dispatch_watch_funcs,
                                                         sizeof (GtkPrintCpdbDispatchWatch));
  g_source_set_name (&dispatch->source, "GTK CPDB backend");

  GTK_NOTE (PRINTING,
            g_print ("CPDB: %s <source %p> - Executing request on server '%s' and resource '%s'\n", G_STRFUNC, dispatch, request->server, request->resource));

  dispatch->request = request;
  dispatch->backend = g_object_ref (print_backend);
  dispatch->poll_state = GTK_CPDB_HTTP_IDLE;
  dispatch->data_poll = NULL;
  dispatch->callback = NULL;
  dispatch->callback_data = NULL;

  print_backend->requests = g_list_prepend (print_backend->requests, dispatch);

  g_source_set_callback ((GSource *) dispatch, (GSourceFunc) callback, user_data, notify);

  if (request->need_auth_info)
    {
      dispatch->callback = callback;
      dispatch->callback_data = user_data;
      lookup_auth_info (dispatch);
    }
  else
    {
      g_source_attach ((GSource *) dispatch, NULL);
      g_source_unref ((GSource *) dispatch);
    }
}

static void
gtk_print_backend_cpdb_print_stream (GtkPrintBackend *print_backend,
                                     GtkPrintJob *job,
                                     GIOChannel *data_io,
                                     GtkPrintJobCompleteFunc callback,
                                     gpointer user_data,
                                     GDestroyNotify dnotify)
{
  GtkPrinterCpdb *cpdb_printer;
  CpdbPrintStreamData *ps;
  CpdbOptionsData *options_data;
  GtkPageSetup *page_setup;
  GtkCpdbRequest *request = NULL;
  GtkPrintSettings *settings;
  const char *title;
  char printer_absolute_uri[HTTP_MAX_URI];
  http_t *http = NULL;
  GTK_NOTE (PRINTING,
            g_print ("CPDB: %s\n", G_STRFUNC));
  cpdb_printer = GTK_PRINTER_CPDB (gtk_print_job_get_printer (job));
  settings = gtk_print_job_get_settings (job);

  if (cpdb_printer->avahi_browsed)
    { 
      http = httpConnect2 (cpdb_printer->hostname, cpdb_printer->port,
                           NULL, AF_UNSPEC,
                           HTTP_ENCRYPTION_IF_REQUESTED,
                           1, 30000,
                           NULL); 

      if (http)
        {
          request = gtk_cpdb_request_new_with_username (http,
                                                        GTK_CPDB_POST,
                                                        IPP_PRINT_JOB,
                                                        data_io,
                                                        cpdb_printer->hostname,
                                                        cpdb_printer->device_uri,
                                                        GTK_PRINT_BACKEND_CPDB (print_backend)->username);
          g_snprintf (printer_absolute_uri, HTTP_MAX_URI, "%s", cpdb_printer->printer_uri);
        }
      else
        {
          GError *error = NULL;

          GTK_NOTE (PRINTING,
                    g_warning ("CPDB: Error connecting to %s:%d",
                               cpdb_printer->hostname,
                               cpdb_printer->port));

          error = g_error_new (gtk_print_error_quark (),
                               GTK_CPDB_ERROR_GENERAL,
                               "Error connecting to %s",
                               cpdb_printer->hostname);

          gtk_print_job_set_status (job, GTK_PRINT_STATUS_FINISHED_ABORTED);

          if (callback)
            {
              callback (job, user_data, error);
            }

          g_clear_error (&error);

          return;
        }
      else
      {
        request = gtk_cpdb_request_new_with_username (NULL,
                                                      GTK_CPDB_POST,
                                                      IPP_PRINT_JOB,
                                                      data_io,
                                                      NULL,
                                                      cpdb_printer->device_uri,
                                                      GTK_PRINT_BACKEND_CPDB (print_backend)->username);

        httpAssembleURIf (HTTP_URI_CODING_ALL,
                          printer_absolute_uri,
                          sizeof (printer_absolute_uri),
                          "ipp",
                          NULL,
                          "localhost",
                          ippPort (),
                          "/printers/%s",
                          gtk_printer_get_name (gtk_print_job_get_printer (job)));
      }

      gtk_cpdb_request_set_ipp_version (request,
                                        cpdb_printer->ipp_version_major,
                                        cpdb_printer->ipp_version_minor);

      gtk_cpdb_request_ipp_add_string (request, IPP_TAG_OPERATION,
                                       IPP_TAG_URI, "printer-uri",
                                       NULL, printer_absolute_uri);

      title = gtk_print_job_get_title (job);
      if (title)
        {
          char *title_truncated = NULL;
          size_t title_bytes = strlen (title);

          if (title_bytes >= IPP_MAX_NAME)
            {
              char *end;

              end = g_utf8_find_prev_char (title, title + IPP_MAX_NAME - 1);
              title_truncated = g_utf8_substring (title,
                                                  0,
                                                  g_utf8_pointer_to_offset (title, end));
            }

          gtk_cpdb_request_ipp_add_string (request, IPP_TAG_OPERATION,
                                           IPP_TAG_NAME, "job-name",
                                           NULL,
                                           title_truncated ? title_truncated : title);
          g_free (title_truncated);
        }

      g_object_get (job,
                    "page-setup", &page_setup,
                    NULL);
      
      options_data = g_new0 (CpdbOptionsData, 1);
      options_data->request = request;
      options_data->printer = cpdb_printer;
      options_data->page_setup = page_setup;
      gtk_print_settings_foreach (settings, add_cpdb_options, options_data);
      g_clear_object (&page_setup);
      g_free (options_data);

      ps = g_new0 (CpdbPrintStreamData, 1);
      ps->callback = callback;
      ps->user_data = user_data;
      ps->dnotify = dnotify;
      ps->job = g_object_ref (job);
      ps->http = http;

      request->need_auth_info = FALSE;
      request->auth_info_required = NULL;

 */
      /* Check if auth_info_required is set and if it should be handled.
       The libraries handle the ticket exchange for "negotiate". */
/*

      if (cpdb_printer->auth_info_required != NULL &&
          g_strv_length (cpdb_printer->auth_info_required) == 1 &&
          g_strcmp0 (cpdb_printer->auth_info_required[0], "negotiate") == 0)
        {
          GTK_NOTE (PRINTING,
                    g_print ("CPDB: Ignoring auth-info-required \"%s\"\n",
                             cpdb_printer->auth_info_required[0]));
        }
      else if (cpdb_printer->auth_info_required != NULL)
        {
          request->need_auth_info = TRUE;
          request->auth_info_required = g_strdupv (cpdb_printer->auth_info_required);
        }

      cpdb_request_execute (GTK_PRINT_BACKEND_CPDB (print_backend),
                            request,
                            (GtkPrintCpdbResponseCallbackFunc) cpdb_print_cb,
                            ps,
                            (GDestroyNotify) cpdb_free_print_stream_data);
    }
}
*/

static GtkPrinter *
get_gtk_printer_from_printer_obj (PrinterObj *p)
{
  GtkPrinter *printer;

  printer = g_object_new (GTK_TYPE_PRINTER,
                          "name", p->name,
                          "backend", GTK_PRINT_BACKEND_CPDB (gtkPrintBackend));

  gtk_printer_set_icon_name (printer, "printer");
  gtk_printer_set_state_message(printer, p->state);
  gtk_printer_set_location(printer, p->location);
  gtk_printer_set_description(printer, p->info);
  gtk_printer_set_is_accepting_jobs(printer, p->is_accepting_jobs);
  gtk_printer_set_job_count(printer, get_active_jobs_count(p));

  gtk_printer_set_has_details (printer, TRUE);
  gtk_printer_set_is_active (printer, TRUE);
  // // Given GCP is going to be deprecated, the CUPS default printer wil be overall default.
  // if (strcmp(p->backend_name, "CUPS") == 0 &&
  //     strcmp(get_default_printer(frontendObj, p->backend_name), p->name) == 0)
  //   {
  //     gtk_printer_set_is_default (printer, TRUE);
  //   }
  return printer;
}

static int add_printer_callback(PrinterObj *p)
{
  GtkPrinter *printer = get_gtk_printer_from_printer_obj(p);
  gtk_print_backend_add_printer (gtkPrintBackend, printer);
  g_object_unref (printer);
  gtk_print_backend_set_list_done (gtkPrintBackend);

  return 0;
}

static int remove_printer_callback(PrinterObj *p)
{
  g_message("Removed Printer %s : %s!\n", p->name, p->backend_name);
  return 0;
}

/*
 * Add the below functions to cpdbutils.c
 */

/*
void
gtk_cpdb_request_ipp_add_string (GtkCpdbRequest *request,
                                 ipp_tag_t group,
                                 ipp_tag_t tag,
                                 const char *name,
                                 const char *charset,
                                 const char *value)
{
  ippAddString (request->ipp_request,
                group,
                tag,
                name,
                charset,
                value);
}

void
gtk_cpdb_request_set_ipp_version (GtkCpdbRequest *request,
                                  int major,
                                  int minor)
{
  ippSetVersion (request->ipp_request, major, minor);
}

GtkCpdbRequest *
gtk_cpdb_request_new_with_username (http_t *connection,
                                    GtkCpdbRequestType req_type,
                                    int operation_id,
                                    GIOChannel *data_io,
                                    const char *server,
                                    const char *resource,
                                    const char *username)
{
  GtkCpdbRequest *request;
  cups_lang_t *language;

  request = g_new0 (GtkCpdbRequest, 1);
  request->result = g_new0 (GtkCpdbResult, 1);

  request->result->error_msg = NULL;
  request->result->ipp_response = NULL;

  request->result->is_error = FALSE;
  request->result->is_ipp_response = FALSE;

  request->type = req_type;
  request->state = GTK_CPDB_REQUEST_START;

  request->password_state = GTK_CPDB_PASSWORD_NONE;

  if (server)
    request->server = g_strdup (server);
  // else
  //   request->server = g_strdup (cupsServer ());

  if (resource)
    request->resource = g_strdup (resource);
  else
    request->resource = g_strdup ("/");

  if (connection != NULL)
    {
      request->http = connection;
      request->own_http = FALSE;
    }
  else
    {
      request->http = httpConnect2 (request->server, ippPort (),
                                    NULL, AF_UNSPEC,
                                    HTTP_ENCRYPTION_IF_REQUESTED,
                                    1, 30000, NULL);

      if (request->http)
        httpBlocking (request->http, 0);

      request->own_http = TRUE;
    }

  request->last_status = HTTP_CONTINUE;

  request->attempts = 0;
  request->data_io = data_io;
  request->ipp_request = ippNew ();
  ippSetOperation (request->ipp_request, operation_id);
  ippSetRequestId (request->ipp_request, 1);

  language = cupsLangDefault ();

  gtk_cpdb_request_ipp_add_string (request, IPP_TAG_OPERATION, IPP_TAG_CHARSET,
                                   "attributes-charset",
                                   NULL, "utf-8");

  gtk_cpdb_request_ipp_add_string (request, IPP_TAG_OPERATION, IPP_TAG_LANGUAGE,
                                   "attributes-natural-language",
                                   NULL, language->language);

  // if (username != NULL)
  gtk_cpdb_request_ipp_add_string (request, IPP_TAG_OPERATION, IPP_TAG_NAME,
                                   "requesting-user-name",
                                   NULL, username);
  // else
  //   gtk_cpdb_request_ipp_add_string (request, IPP_TAG_OPERATION, IPP_TAG_NAME,
  //                                    "requesting-user-name",
  //                                    NULL, cupsUser ());
  request->auth_info_required = NULL;
  request->auth_info = NULL;
  request->need_auth_info = FALSE;
  // cupsLangFree (language);
  return request;
}
*/

/*
 * gtkprintercpdb.c
 */

static GtkPrinterClass *gtk_printer_cpdb_parent_class;

static void gtk_printer_cpdb_set_property (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);
static void gtk_printer_cpdb_get_property (GObject      *object,
                                           guint         prop_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);

static void
gtk_printer_cpdb_class_init (GtkPrinterCpdbClass *class)
{
  GObjectClass *object_class = (GObjectClass *) class;

  object_class->finalize = gtk_printer_cpdb_finalize;
  object_class->set_property = gtk_printer_cpdb_set_property;
  object_class->get_property = gtk_printer_cpdb_get_property;

  gtk_printer_cpdb_parent_class = g_type_class_peek_parent (class);

  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_PROFILE_TITLE,
                                   g_param_spec_string ("profile-title",
                                                        P_("Color Profile Title"),
                                                        P_("The title of the color profile to use"),
                                                        "",
                                                        G_PARAM_READABLE));
}

static void
gtk_printer_cpdb_init (GtkPrinterCpdb *printer)
{
  printer->device_uri = NULL;
  printer->original_device_uri = NULL;
  printer->printer_uri = NULL;
  printer->state = 0;
  printer->hostname = NULL;
  printer->port = 0;
  printer->original_hostname = NULL;
  printer->original_resource = NULL;
  printer->original_port = 0;
  printer->request_original_uri = FALSE;
  printer->ppd_name = NULL;
  printer->ppd_file = NULL;
  printer->default_cover_before = NULL;
  printer->default_cover_after = NULL;
  printer->remote = FALSE;
  printer->get_remote_ppd_poll = 0;
  printer->get_remote_ppd_attempts = 0;
  printer->remote_cpdb_connection_test = NULL;
  printer->auth_info_required = NULL;
  printer->default_number_up = 1;
  printer->avahi_browsed = FALSE;
  printer->avahi_name = NULL;
  printer->avahi_type = NULL;
  printer->avahi_domain = NULL;
  printer->ipp_version_major = 1;
  printer->ipp_version_minor = 1;
  printer->supports_copies = FALSE;
  printer->supports_collate = FALSE;
  printer->supports_number_up = FALSE;
  printer->media_default = NULL;
  printer->media_supported = NULL;
  printer->media_size_supported = NULL;
  printer->media_bottom_margin_default = 0;
  printer->media_top_margin_default = 0;
  printer->media_left_margin_default = 0;
  printer->media_right_margin_default = 0;
  printer->media_margin_default_set = FALSE;
  printer->sides_default = NULL;
  printer->sides_supported = NULL;
  printer->number_of_covers = 0;
  printer->covers = NULL;
  printer->output_bin_default = NULL;
  printer->output_bin_supported = NULL;
}

static void
gtk_printer_cpdb_finalize (GObject *object)
{
  GtkPrinterCpdb *printer;

  g_return_if_fail (object != NULL);

  printer = GTK_PRINTER_CPDB (object);

  g_free (printer->device_uri);
  g_free (printer->original_device_uri);
  g_free (printer->printer_uri);
  g_free (printer->hostname);
  g_free (printer->original_hostname);
  g_free (printer->original_resource);
  g_free (printer->ppd_name);
  g_free (printer->default_cover_before);
  g_free (printer->default_cover_after);
  g_strfreev (printer->auth_info_required);

#ifdef HAVE_COLORD
  if (printer->colord_cancellable)
    {
      g_cancellable_cancel (printer->colord_cancellable);
      g_object_unref (printer->colord_cancellable);
    }
  g_free (printer->colord_title);
  g_free (printer->colord_qualifier);
  if (printer->colord_client)
    g_object_unref (printer->colord_client);
  if (printer->colord_device)
    g_object_unref (printer->colord_device);
  if (printer->colord_profile)
    g_object_unref (printer->colord_profile);
#endif

  g_free (printer->avahi_name);
  g_free (printer->avahi_type);
  g_free (printer->avahi_domain);

  g_strfreev (printer->covers);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  if (printer->ppd_file)
    ppdClose (printer->ppd_file);

  G_GNUC_END_IGNORE_DEPRECATIONS

  g_free (printer->media_default);
  g_list_free_full (printer->media_supported, g_free);
  g_list_free_full (printer->media_size_supported, g_free);

  g_free (printer->sides_default);
  g_list_free_full (printer->sides_supported, g_free);

  g_free (printer->output_bin_default);
  g_list_free_full (printer->output_bin_supported, g_free);

  if (printer->get_remote_ppd_poll > 0)
    g_source_remove (printer->get_remote_ppd_poll);
  printer->get_remote_ppd_attempts = 0;

  gtk_cpdb_connection_test_free (printer->remote_cpdb_connection_test);

  G_OBJECT_CLASS (gtk_printer_cpdb_parent_class)->finalize (object);
}

static void
gtk_printer_cpdb_set_property (GObject         *object,
                               guint            prop_id,
                               const GValue    *value,
                               GParamSpec      *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_printer_cpdb_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
#ifdef HAVE_COLORD
  GtkPrinterCpdb *printer = GTK_PRINTER_CUPS (object);
#endif

  switch (prop_id)
    {
    case PROP_PROFILE_TITLE:
#ifdef HAVE_COLORD
      if (printer->colord_title)
        g_value_set_string (value, printer->colord_title);
      else
        g_value_set_static_string (value, "");
#else
      g_value_set_static_string (value, NULL);
#endif
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}
