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
#include "php_ctypes.h"
#include <php.h>
#include <fopen_wrappers.h>
#include <ext/standard/basic_functions.h>

static zend_class_entry * class_ce;
static zend_class_entry * class_ce_fnc;

typedef struct {
    zend_object zo;
    DL_HANDLE lib;
    char * path;
} library_data;

typedef struct {
    library_data * lib;
    void * fnc;
} function;

#if defined(PHP_WIN32)
# define GET_FUNCTION(handle, name)	GetProcAddress(handle, name)
#elif HAVE_LIBDL
# define GET_FUNCTION(handle, name)	dlsym(handle, name)
#elif defined(HAVE_MACH_O_DYLD_H)
# define GET_FUNCTION(handle, name)	zend_mh_bundle_symbol(handle, name)
#else
# error You lose
#endif

#define FETCH_DATA(name) FETCH_DATA_EX(library_data, name)

#define IS_NATIVE   (1 << 24)
#define TYPE(x)     (IS_NATIVE | (1 << (x+12)))
#define T_LONG      TYPE(1)
#define T_CHAR      TYPE(2)
#define T_STRING    TYPE(5)
#define T_BOOL      TYPE(6)
#define T_DOUBLE    TYPE(7)
#define T_PTR       1 << 25
#define T_PTRPTR    1 << 26 

// Library constructor / destructor {{{
static void free_library_object(library_data *obj TSRMLS_DC)
{
    zend_object_std_dtor(&obj->zo);
    if (obj->path) {
        efree(obj->path);
    }
    if (obj->lib) {
        DL_UNLOAD(obj->lib);
    }
    efree(obj);
}

zend_object_value new_library_object(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    library_data * data;
    zval *tmp;

    data = emalloc(sizeof(library_data));
    memset(data, 0, sizeof(library_data));

    zend_object_std_init(&data->zo, ce TSRMLS_CC);
    zend_hash_copy(data->zo.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(data, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)free_library_object, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}

static PHP_METHOD(Library, __construct)
{
    FETCH_DATA(data)
	char *filename; 
	int filename_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
		return;
	}

    data->lib = DL_LOAD(filename);
    if (!data->lib) {
        ctypes_exception("the file doesn't look like a valid library", 1 TSRMLS_C);
        return;
    }
    data->path = estrndup(filename, filename_len);
}
// }}}

// Function constructor / destructor {{{
zend_object_value new_function_object(zend_class_entry *ce TSRMLS_DC)
{
}
// }}}

static int is_valid(zval * data)
{
    switch (Z_TYPE_P(data)) {
    case IS_NULL:
        break;
    case IS_LONG:
        if ((IS_NATIVE & Z_LVAL_P(data)) == 0) {
            /* It's not a native data type, then it must
               be a resource, let's find out */
            long id = Z_LVAL_P(data);
            if ((id & T_PTR) > 0) {
                id -= T_PTR;
            }
            if ((id & T_PTRPTR) > 0) {
                id -= T_PTRPTR;
            }

            return ctypes_resource_exists((int)id TSRMLS_CC);
        }
        break;
    case IS_OBJECT:
        if (is_register_object(data) == FAILURE) {
            return FAILURE;
        }
        break;
    default: 
        return FAILURE;
    }

    return SUCCESS;
}

static PHP_METHOD(Library, getFunction)
{
    char * fnc_name;
    int  * fnc_len;
    zval * return_type;
    zval * function_signature;
    void * ptr;

    FETCH_DATA(data);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|za", &fnc_name, &fnc_len, &return_type, &function_signature) == FAILURE) {
		return;
	}

    ptr = GET_FUNCTION(data->lib, fnc_name);

    if (!ptr) {
        ctypes_exception("Cannot find function", 4);
        return;
    }

    if (is_valid(return_type) == FAILURE) {
        ctypes_exception("Invalid $return_type", 4);
        return;
    }


    FOREACH(function_signature)
        if (is_valid(&value) == FAILURE) {
            ctypes_exception("Invalid function signature", 4);
            return;
        }
    ENDFOREACH(function_signature)
}

static PHP_METHOD(Library, getLibraryPath)
{
    FETCH_DATA(data);
    RETURN_STRING(data->path, strlen(data->path));
}

/* {{{ methods arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, 0, 1)
    ZEND_ARG_INFO(0, library_path)
    ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo__getLibraryPath, 0, 0, 1)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo__getFunction, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, return_type)
    ZEND_ARG_INFO(0, function_signature)
ZEND_END_ARG_INFO()
/* }}} */

static zend_function_entry class_methods[] = {
    PHP_ME(Library, __construct,  arginfo___construct, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Library, getLibraryPath,  arginfo__getLibraryPath, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Library, getFunction,  arginfo__getFunction, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    { NULL, NULL, NULL }
};

void class_register_library(TSRMLS_DC)
{
    zend_class_entry class, class_fnc;
    INIT_CLASS_ENTRY(class, "CTypes\\Library", class_methods);
    INIT_CLASS_ENTRY(class_fnc, "CTypes\\FunctionProxy", class_methods);

    class_ce = zend_register_internal_class(&class TSRMLS_CC);
    class_ce->create_object = new_library_object ;

    class_ce_fnc = zend_register_internal_class(&class_fnc TSRMLS_CC);
    class_ce_fnc->create_object = new_function_object;


    zend_declare_class_constant_long(class_ce, "tInteger", strlen("tInteger"), T_LONG TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tInt", strlen("tInt"), T_LONG TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tFloat", strlen("tFloat"), T_DOUBLE TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tString", strlen("tString"), T_STRING TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tChar", strlen("tChar"),  T_CHAR TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tBool", strlen("tBool"), T_BOOL TSRMLS_CC);

    zend_declare_class_constant_long(class_ce, "tPtr", strlen("tPtr"), T_PTR TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tPtrPtr", strlen("tPtrPtr"), T_PTRPTR TSRMLS_CC);
}
