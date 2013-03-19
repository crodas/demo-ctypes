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

typedef struct {
    zend_object zo;
    int resid;
} resource_data;

static zend_class_entry * class_ce;

#define FETCH_DATA(name) \
    resource_data * name; \
    zval * this = getThis(); \
    name = zend_object_store_get_object(this TSRMLS_CC);


zend_object_value new_resource_class(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    resource_data * data;
    zval *tmp;

    data = emalloc(sizeof(resource_data));
    memset(data, 0, sizeof(resource_data));
    data->resid = ctypes_resource_create(TSRMLS_CC);


    zend_object_std_init(&data->zo, ce TSRMLS_CC);
    zend_hash_copy(data->zo.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(data, (zend_objects_store_dtor_t)zend_objects_destroy_object, NULL, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}



static PHP_METHOD(Resource, __construct)
{
	char *name;
	int name_len;
    FETCH_DATA(data)

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &name_len) == FAILURE) { \
		return;
	}
    //data->name = strndup(name, name_len);
}

static PHP_METHOD(Resource, getResourceId)
{
    FETCH_DATA(data)
    RETURN_LONG(data->resid);
}

/* {{{ methods arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, 0, 1)
    ZEND_ARG_INFO(0, library_path)
    ZEND_ARG_INFO(1, options)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo__getResourceId, 0, 0, 1)
ZEND_END_ARG_INFO()
/* }}} */

static zend_function_entry class_methods[] = {
    PHP_ME(Resource, __construct,  arginfo___construct, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Resource, getResourceId,  arginfo__getResourceId, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    { NULL, NULL, NULL }
};

void class_register_resource(TSRMLS_DC)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "CTypes\\Resource", class_methods);
    class_ce = zend_register_internal_class(&class TSRMLS_CC);
    class_ce->create_object = new_resource_class ;
}
