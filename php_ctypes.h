/*
  +----------------------------------------------------------------------+
  | Copyright (c) 2013 The PHP Group                                     |
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

#ifndef PHP_CTYPES_H
#define PHP_CTYPES_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include <ext/standard/info.h>
#include <Zend/zend_exceptions.h>
#include <ffi.h>

extern zend_module_entry ctypes_module_entry;
int ctypes_resource_init(TSRMLS_DC);
void ctypes_resource_destroy();
int ctypes_resource_create(TSRMLS_DC);

void class_register_library(TSRMLS_DC);
void class_register_resource(TSRMLS_DC);
void class_register_exception(TSRMLS_DC);
void class_register_function(TSRMLS_DC);
void class_register_type(TSRMLS_DC);
void ctypes_exception(char * message, int errorno TSRMLS_DC);
int ctypes_resource_exists(int id TSRMLS_DC);
int is_register_object(zval * x) ;

extern int mnumber;
extern zend_class_entry * class_ce_function;
extern zend_class_entry * class_ce_library;

ZEND_BEGIN_MODULE_GLOBALS(ctypes)
    HashTable * resources;
ZEND_END_MODULE_GLOBALS(ctypes)

extern zend_ctypes_globals ctypes_globals;

#if ZTS
    #define G(x) (TSRMG(ctypes_globals_id, zend_ctypes_globals*, x))
#else 
    #define G(x) (ctypes_globals.x)
#endif


typedef struct {
    zend_object zo;
    DL_HANDLE lib;
    char * path;
} library_data;

typedef struct {
    zend_object zo;
    zval * lib_instance;
    library_data * libdata;
    char * name;
    void * ptr;

    int failed;

    int argc;
    ffi_cif cif;
    ffi_type ** args;
    long     * args_types;
    ffi_type * return_type;
} function;

#define phpext_ctypes_ptr &ctypes_module_entry

#ifdef PHP_WIN32
    #define PHP_CTYPES_API __declspec(dllexport)
#else
    #define PHP_CTYPES_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define PHP_CTYPES_VERSION "0.1"


#define FETCH_DATA_EX_EX(zthis, type, name) \
    type * name; \
    name = (type *)zend_object_store_get_object(zthis TSRMLS_CC);

#define FETCH_DATA_EX(type, name) \
    zval * this = getThis(); \
    FETCH_DATA_EX_EX(this, type, name)

#define CALL_METHOD(Class, Method, retval, thisptr)  PHP_FN(Class##_##Method)(0, retval, NULL, thisptr, 0 TSRMLS_CC);

#define EXITLOOP \
    zval_dtor(&value); \
    zend_hash_internal_pointer_reset(Z_ARRVAL_P(zval_array)); 

#define ENDFOREACH \
    zval_dtor(&value); } \
    zend_hash_internal_pointer_reset(Z_ARRVAL_P(zval_array));  \
    } while (0);

#define DEBUG(x)    printf x;fflush(stdout);
#define FOREACH(array) FOREACH_EX(array, 1 == 1)

#define FOREACH_EX(array, exp) do {\
    zval * zval_array; \
    HashPosition pos; \
    zval **current, value; \
    char *key;\
    uint keylen;\
    ulong idx;\
    int type;\
    zval_array = array; \
    for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(array), &pos); \
            zend_hash_get_current_data_ex(Z_ARRVAL_P(array), (void**)&current, &pos) == SUCCESS && exp; \
            zend_hash_move_forward_ex(Z_ARRVAL_P(array), &pos) \
    ) { \
        value = **current;\
        zval_copy_ctor(&value);\
        INIT_PZVAL(&value);


#endif /* PHP_ctypes_H */

#define T_NATIVE    (1 << 24)
#define TYPE(x)     (T_NATIVE | (1 << (x+12)))
#define T_LONG      TYPE(1)
#define T_CHAR      TYPE(2)
#define T_STRING    TYPE(5)
#define T_BOOL      TYPE(6)
#define T_DOUBLE    TYPE(7)
#define T_PTR       (1 << 25)
#define T_PTRPTR    (1 << 26) 

#define IS_TYPE(x, y)       ((T_##x & y) == T_##x)
#define IS_NATIVE(x)        IS_TYPE(NATIVE, x)
#define IS_PTR(x)           IS_TYPE(PTR, x)
#define IS_PTRPTR(x)        IS_TYPE(PTRPTR, x)
//#define IS_NATIVE(x)    ((T_NATIVE & x) == 1)


PHP_MINIT_FUNCTION(ctypes);
PHP_MINFO_FUNCTION(ctypes);

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
