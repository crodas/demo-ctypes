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

typedef struct {
    unsigned char * path; /* library path */
    short int loaded;
    zend_object zo;
} php_ctypes_t;


/****************************************
  Forward declarations
****************************************/

/****************************************
  ctypes callback support 
****************************************/


/****************************************
  Method implementations
****************************************/

/****************************************
  Internal support code
****************************************/

/* {{{ constructor/destructor */
static void php_ctypes_destroy(php_ctypes_t *obj TSRMLS_DC)
{

}

static void php_ctypes_free_storage(php_ctypes_t *obj TSRMLS_DC)
{
    zend_object_std_dtor(&obj->zo TSRMLS_CC);

    php_ctypes_destroy(obj TSRMLS_CC);
    efree(obj);
}

zend_object_value php_ctypes_new(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    php_ctypes_t *obj;
    zval *tmp;

    obj = (php_ctypes_t *) emalloc(sizeof(php_ctypes_t));
    memset(obj, 0, sizeof(php_ctypes_t));
    zend_object_std_init(&obj->zo, ce TSRMLS_CC);
    zend_hash_copy(obj->zo.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(obj, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)php_ctypes_free_storage, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(ctypes)
{
    mnumber = module_number;

    ctypes_resource_init(TSRMLS_C);
    class_register_resource(TSRMLS_C);
    class_register_library(TSRMLS_C);
    class_register_exception(TSRMLS_C);

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

/* {{{ ctypes_module_entry
 */
zend_module_entry ctypes_module_entry = {
    STANDARD_MODULE_HEADER,
    "ctypes",
    NULL,
    PHP_MINIT(ctypes),
    PHP_MSHUTDOWN(ctypes),
    NULL,
    NULL,
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
