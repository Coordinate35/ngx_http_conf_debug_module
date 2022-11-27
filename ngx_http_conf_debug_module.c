#include <ngx_http.h>
#include <ngx_config.h>

/*
 * version: 0.1.0
 */


#define NGX_HTTP_NOT_ACCEPTABLE 406


typedef struct {
    ngx_flag_t enabled;
    ngx_flag_t interrupt;
    ngx_uint_t status_code;
} ngx_http_conf_debug_loc_conf_t;


static void* ngx_http_conf_debug_create_loc_conf(ngx_conf_t *cf);
static char* ngx_http_conf_debug_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_conf_debug_preconfiguration(ngx_conf_t *cf);
static ngx_int_t ngx_http_conf_debug_postconfiguration(ngx_conf_t *cf);
static ngx_int_t ngx_http_conf_debug_location_mode(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_conf_debug_location_name(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_conf_debug_handler(ngx_http_request_t *r);


static ngx_str_t ngx_http_conf_debug_location_mode_exact = ngx_string("=");
static ngx_str_t ngx_http_conf_debug_location_mode_prefix_prior = ngx_string("^~");
static ngx_str_t ngx_http_conf_debug_location_mode_regular_case_insensitive = ngx_string("~*");
static ngx_str_t ngx_http_conf_debug_location_mode_regular_case_sensitive = ngx_string("~");
static ngx_str_t ngx_http_conf_debug_location_mode_prefix_normal = ngx_string("blank");


static ngx_http_variable_t ngx_http_conf_debug_variables[] = {
    { ngx_string("conf_debug_location_mode"), NULL,
      ngx_http_conf_debug_location_mode,
      0, 0, 0 },
    
    { ngx_string("conf_debug_location_name"), NULL,
      ngx_http_conf_debug_location_name,
      0, 0, 0 },

    { ngx_null_string, NULL, NULL, 0, 0, 0}
};

static ngx_command_t ngx_http_conf_debug_commands[] = {
    { ngx_string("conf_debug_enable"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_conf_debug_loc_conf_t, enabled),
      NULL },

    { ngx_string("conf_debug_interrupt"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_conf_debug_loc_conf_t, interrupt),
      NULL },

    { ngx_string("conf_debug_interrupt_status_code"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_conf_debug_loc_conf_t, status_code),
      NULL },

    ngx_null_command
};

static ngx_http_module_t ngx_http_conf_debug_module_ctx = {
    ngx_http_conf_debug_preconfiguration,  /* preconfiguration */
    ngx_http_conf_debug_postconfiguration, /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_conf_debug_create_loc_conf,   /* create location configuration */
    ngx_http_conf_debug_merge_loc_conf     /* merge location configuration */
};

ngx_module_t ngx_http_conf_debug_module = {
    NGX_MODULE_V1,
    &ngx_http_conf_debug_module_ctx, /* module context */
    ngx_http_conf_debug_commands,    /* module directives */
    NGX_HTTP_MODULE,                 /* module type */
    NULL,                            /* init master */
    NULL,                            /* init module */
    NULL,                            /* init process */
    NULL,                            /* init thread */
    NULL,                            /* exit thread */
    NULL,                            /* exit process */
    NULL,                            /* exit master */
    NGX_MODULE_V1_PADDING
};


static void*
ngx_http_conf_debug_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_conf_debug_loc_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_conf_debug_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->enabled = NGX_CONF_UNSET;
    conf->interrupt = NGX_CONF_UNSET;
    conf->status_code = NGX_CONF_UNSET_UINT;

    return conf;
}


static char*
ngx_http_conf_debug_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_conf_debug_loc_conf_t  *prev = parent;
    ngx_http_conf_debug_loc_conf_t  *conf = child;

    ngx_conf_merge_value(conf->enabled, prev->enabled, 0);
    ngx_conf_merge_value(conf->interrupt, prev->interrupt, 0);
    ngx_conf_merge_uint_value(conf->status_code, prev->status_code, NGX_HTTP_NOT_ACCEPTABLE);

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_conf_debug_preconfiguration(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_conf_debug_variables; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }
        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_conf_debug_postconfiguration(ngx_conf_t *cf)
{
    ngx_http_handler_pt             *h;
    ngx_http_core_main_conf_t       *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_conf_debug_handler;

    return NGX_OK;
}


static ngx_int_t
ngx_http_conf_debug_location_mode(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    int                              n, options;
    ngx_http_core_loc_conf_t        *clcf;
    ngx_http_conf_debug_loc_conf_t  *cdlcf;

    options = 0;

    cdlcf = ngx_http_get_module_loc_conf(r, ngx_http_conf_debug_module);
    if (!cdlcf->enabled) {
        v->not_found = 1;
        return NGX_OK;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
    v->valid = 0;

    if (clcf->exact_match) {
        v->valid = 1;
        v->len = ngx_http_conf_debug_location_mode_exact.len;
        v->data = ngx_http_conf_debug_location_mode_exact.data;
    }

    if (clcf->noregex) {
        v->valid = 1;
        v->len = ngx_http_conf_debug_location_mode_prefix_prior.len;
        v->data = ngx_http_conf_debug_location_mode_prefix_prior.data;
    }

    if (
        !clcf->exact_match
        && !clcf->noregex
#if (NGX_PCRE)
        && clcf->regex == NULL
#endif
    ) {
        v->valid = 1;
        v->len = ngx_http_conf_debug_location_mode_prefix_normal.len;
        v->data = ngx_http_conf_debug_location_mode_prefix_normal.data;
    }

#if (NGX_PCRE)
    if (
        clcf->regex == NULL
        || clcf->regex->regex == NULL
        || clcf->regex->regex->code == NULL
    ) {
        return NGX_ERROR;
    }
    n = pcre_fullinfo(clcf->regex->regex->code, NULL, PCRE_INFO_OPTIONS, &options);
    if (n < 0) {
        return NGX_ERROR;
    }
    if (options & PCRE_CASELESS) {
        v->valid = 1;
        v->len = ngx_http_conf_debug_location_mode_regular_case_insensitive.len;
        v->data = ngx_http_conf_debug_location_mode_regular_case_insensitive.data;
    } else {
        v->valid = 1;
        v->len = ngx_http_conf_debug_location_mode_regular_case_sensitive.len;
        v->data = ngx_http_conf_debug_location_mode_regular_case_sensitive.data;
    }
#endif

    if (!v->valid) {
        return NGX_ERROR;
    }

    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_conf_debug_location_name(ngx_http_request_t *r, ngx_http_variable_value_t *v, 
    uintptr_t data)
{
    ngx_http_core_loc_conf_t        *clcf;
    ngx_http_conf_debug_loc_conf_t  *cdlcf;

    cdlcf = ngx_http_get_module_loc_conf(r, ngx_http_conf_debug_module);

    if (!cdlcf->enabled) {
        v->not_found = 1;
        return NGX_OK;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    v->len = clcf->name.len;
    v->data = clcf->name.data;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_conf_debug_handler(ngx_http_request_t *r)
{
    ngx_http_conf_debug_loc_conf_t *cdlcf;

    cdlcf = ngx_http_get_module_loc_conf(r, ngx_http_conf_debug_module);

    if (cdlcf->enabled && cdlcf->interrupt) {
        return cdlcf->status_code;
    }

    return NGX_DECLINED;
}