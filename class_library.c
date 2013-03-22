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

zend_class_entry * class_ce_library;
static zend_class_entry * class_ce;

/* macros {{{ */
#define FETCH_DATA(name) FETCH_DATA_EX(library_data, name)

/* }}} */

// class Library {{{

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

// Library::getFunction {{{
static PHP_METHOD(Library, getFunction)
{
    zval * a, *b, *c;
    zval * args, *callable , *retval;

    FETCH_DATA(data);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zzz", &a, &b, &c) == FAILURE) {
		return;
	}

    object_init_ex(return_value, class_ce_function);

    MAKE_STD_ZVAL(callable);
    array_init_size(callable, 2);
    add_next_index_zval(callable, return_value);
    add_next_index_string(callable, "__construct", 1);

    MAKE_STD_ZVAL(args);
    array_init_size(args, 4);
    add_next_index_zval(args, this);
    add_next_index_zval(args, a);
    add_next_index_zval(args, b);
    add_next_index_zval(args, c);

    zend_fcall_info fci;
    zend_fcall_info_cache fcc;

    if (zend_fcall_info_init(callable, 0, &fci, &fcc, NULL, NULL TSRMLS_CC) == FAILURE) {
        ctypes_exception("failed to __construct FunctionProxy", 12);
        return;
    }

    if (zend_fcall_info_call(&fci, &fcc, &retval, args TSRMLS_CC) != SUCCESS) {
        ctypes_exception("failed to __construct FunctionProxy", 13);
    }

    // why? {{{
    Z_ADDREF_P(return_value);
    Z_ADDREF_P(this);
    Z_ADDREF_P(a);
    Z_ADDREF_P(b);
    Z_ADDREF_P(c);
    // }}}

    zval_ptr_dtor(&callable);
    zval_ptr_dtor(&args);
}
// }}}

// Library::__toString {{{
static PHP_METHOD(Library, __toString)
{
    FETCH_DATA(data);
    RETURN_STRING(data->path, 1);
}
// }}}

// Library::getLibraryPath {{{
static PHP_METHOD(Library, getLibraryPath)
{
    FETCH_DATA(data);
    RETURN_STRING(data->path, 1);
}
// }}}

// }}}

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
    PHP_ME(Library, __toString,  NULL, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    { NULL, NULL, NULL }
};

void class_register_library(TSRMLS_DC)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "CTypes\\Library", class_methods);

    class_ce = zend_register_internal_class(&class TSRMLS_CC);
    class_ce->create_object = new_library_object ;

    class_ce_library = class_ce;
}
