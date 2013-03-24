/*
  +----------------------------------------------------------------------+
  | Copyright (c) 2009 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Cesar Rodas <crodas@php.net>                                |
  +----------------------------------------------------------------------+
*/

/* $ Id: $ */

#include "php_ctypes.h"
#include <string.h>

/****************************************
  Helper macros
****************************************/

/****************************************
  Structures and definitions
****************************************/

static zend_class_entry * ctypes_object_ce = NULL;
static zend_class_entry * ctypes_exception_ce = NULL;

ZEND_DECLARE_MODULE_GLOBALS(ctypes)

// global variables (per request) {{{
static void check_pointers(void * tmp)
{
    zval * z;
    z = (zval *)tmp;
    printf("called here %d %d\n",Z_TYPE_P(z) == IS_RESOURCE,  Z_REFCOUNT_P(z));fflush(stdout);
}
static void ctypes_globals_ctor(zend_ctypes_globals * ptr TSRMLS_DC)
{
	zend_hash_init(&(ptr->resources), 50, NULL, check_pointers, 1);
}

static void ctypes_globals_dtor(zend_ctypes_globals * ptr TSRMLS_DC)
{
    zend_hash_destroy(&(ptr->resources));
}
// }}}

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(ctypes)
{
    mnumber = module_number;

    ctypes_resource_init(TSRMLS_C);
    class_register_resource(TSRMLS_C);
    class_register_function(TSRMLS_C);
    class_register_library(TSRMLS_C);
    class_register_exception(TSRMLS_C);
    class_register_type(TSRMLS_DC);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MFUNCTION_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(ctypes)
{
    ctypes_resource_destroy();
    return SUCCESS;
}
/* }}} */

PHP_RINIT_FUNCTION(ctypes)
{
    #if ZTS
    ts_allocate_id(&ctypes_globals_id,
        sizeof(zend_ctypes_globals),
        (ts_allocate_ctor)ctypes_globals_ctor,
        (ts_allocate_dtor)ctypes_globals_dtor
    );
    #else
    ctypes_globals_ctor(&ctypes_globals TSRMLS_CC);
    #endif
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(ctypes)
{
    #ifndef ZTS
    ctypes_globals_dtor(&ctypes_globals TSRMLS_CC);
    #endif
    return SUCCESS;
}

/* {{{ ctypes_module_entry
 */
zend_module_entry ctypes_module_entry = {
    STANDARD_MODULE_HEADER,
    "ctypes",
    NULL,

    PHP_MINIT(ctypes),
    PHP_MSHUTDOWN(ctypes),

    PHP_RINIT(ctypes),
    PHP_RSHUTDOWN(ctypes),

    PHP_MINFO(ctypes),
    PHP_CTYPES_VERSION,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(ctypes)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "ctypes support", "enabled");
    php_info_print_table_row(2, "Version", PHP_CTYPES_VERSION);
    php_info_print_table_end();
}
/* }}} */

#ifdef COMPILE_DL_CTYPES
ZEND_GET_MODULE(ctypes)
#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
