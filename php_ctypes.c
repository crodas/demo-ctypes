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

/* {{{ php_ctypes_throw_exception(char * message, int errno TSRMLS_DC)
    Throws an Exception  */
static void php_ctypes_throw_exception(char * message, int errorno TSRMLS_DC)
{
    zval * tc_ex;

    MAKE_STD_ZVAL(tc_ex);
    object_init_ex(tc_ex, ctypes_exception_ce);

    zend_update_property_string(ctypes_exception_ce, tc_ex, "message", sizeof("message") - 1,
            message TSRMLS_CC);
    zend_throw_exception_object(tc_ex TSRMLS_CC);
}
/* }}} */

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
 
/* {{{ methods arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, 0, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_add, 0)
    ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_save, 0)
    ZEND_ARG_INFO(0, language)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_parse, 0)
    ZEND_ARG_INFO(0, text)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo__save, 0)
    ZEND_ARG_INFO(0, language)
    ZEND_ARG_ARRAY_INFO(0, ngrams, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_getInfo, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo__load, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo__parser, 0)
    ZEND_ARG_INFO(0, text)
    ZEND_ARG_INFO(0, min)
    ZEND_ARG_INFO(0, max)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_setDir, 0)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ ctypes_class_methods */
static zend_function_entry base_ctypes_class_methods[] = {
    /*
    PHP_ME(Basectypes, __construct,  arginfo___construct, ZEND_ACC_PUBLIC)
    PHP_ME(Basectypes, add,     arginfo_add,         ZEND_ACC_PUBLIC)
    PHP_ME(Basectypes, getInfo, arginfo_getInfo,     ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Basectypes, save,    arginfo_save,     ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Basectypes, parse,  arginfo_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Basectypes, addSample,  arginfo_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Basectypes, getKnowledges,  arginfo_getInfo, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Basectypes, getCategory,  arginfo_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    ZEND_FENTRY(_save, NULL, arginfo__save, ZEND_ACC_PROTECTED | ZEND_ACC_ABSTRACT)
    ZEND_FENTRY(_load, NULL, arginfo__load, ZEND_ACC_PROTECTED | ZEND_ACC_ABSTRACT)
    ZEND_FENTRY(_list, NULL, arginfo__load, ZEND_ACC_PROTECTED | ZEND_ACC_ABSTRACT)
    ZEND_FENTRY(_parser, NULL, arginfo__parser, ZEND_ACC_PROTECTED | ZEND_ACC_ABSTRACT)
    /* abstract methods */
    { NULL, NULL, NULL }
};

static zend_function_entry ctypes_class_methods[] = {
    /*
    PHP_ME(ctypes, _save, arginfo__save, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(ctypes, _load, arginfo__load, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(ctypes, _list, arginfo__load, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(ctypes, _parser, arginfo__parser, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(ctypes, setDirectory, arginfo_setDir, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    */
    { NULL, NULL, NULL }
};
/* }}} * /

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(ctypes)
{
    ctypes_resource_init();
    mnumber = module_number;
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

#ifdef COMPILE_DL_ctypes
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
