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

/** Macros {{{ */
#define FETCH_DATA(name) FETCH_DATA_EX(function, name)

#if defined(PHP_WIN32)
# define GET_FUNCTION(handle, name)	GetProcAddress(handle, name)
#elif HAVE_LIBDL
# define GET_FUNCTION(handle, name)	dlsym(handle, name)
#elif defined(HAVE_MACH_O_DYLD_H)
# define GET_FUNCTION(handle, name)	zend_mh_bundle_symbol(handle, name)
#else
# error You lose
#endif

/* }}} */

zend_class_entry * class_ce_function;
static zend_class_entry * class_ce;

static int ctypes_zval_to_argument(zval * arg, void ** ptr,  int *should_free, long arg_type, int i, char * error TSRMLS_DC)
{
    #define IF_IS_EX(x, code) if (IS_TYPE(x, arg_type)) { \
            arg; \
            return SUCCESS; \
        }

    #define boolean unsigned char
    #define IF_IS_WRAP_AS(x, type, op) IF_IS_EX(x, \
            *ptr = emalloc(sizeof(type)); \
            *should_free = 1; \
            convert_to_##x(arg); \
            **(type**) = op(arg); \
        )
        


    if (IS_NATIVE(arg_type) && !IS_PTR(arg_type) && !IS_PTRPTR(arg_type)) {
        should_free = 0;

        IF_IS_WRAP_AS(BOOL,     boolean,    Z_BVAL_P);
        IF_IS_WRAP_AS(BOOL,     boolean,    Z_BVAL_P);
        IF_IS_WRAP_AS(LONG,     long,       Z_LVAL_P);
        IF_IS_WRAP_AS(DOUBLE,   double,     Z_DVAL_P);
        IF_IS_WRAP_AS(STRING,   string,     Z_STRVAL);

        return FAILURE;
    } else {
        // we expect a pointer :-)
    }
}

// parse_type {{{
static int parse_type(zval * data, ffi_type ** type)
{
    #define SET_TYPE(t) if (type) { \
            *type = &ffi_type_##t; \
        };

    #define IF_IS(x, y) \
        if (IS_TYPE(x, Z_LVAL_P(data))) { \
            SET_TYPE(y) \
        } 

    switch (Z_TYPE_P(data)) {
    case IS_NULL:
        SET_TYPE(void)
        break;
    case IS_LONG:
        if (!IS_NATIVE(Z_LVAL_P(data))) {
            /* It's not a native data type, then it must
               be a resource, let's find out */
            long id = Z_LVAL_P(data);
            if ((id & T_PTR) > 0) {
                id -= T_PTR;
            }
            if ((id & T_PTRPTR) > 0) {
                id -= T_PTRPTR;
            }

            SET_TYPE(pointer)
            return ctypes_resource_exists((int)id TSRMLS_CC);
        }

        IF_IS(BOOL,     uchar);
        IF_IS(LONG,     slong);
        IF_IS(DOUBLE,   double);
        IF_IS(CHAR,     uchar);
        IF_IS(STRING,   pointer);
        IF_IS(PTR,      pointer);
        IF_IS(PTRPTR,   pointer);
        break;
    case IS_OBJECT:
        if (is_register_object(data) == FAILURE) {
            return FAILURE;
        }
        SET_TYPE(pointer)
        break;
    default: 
        return FAILURE;
    }

    #undef IF_IS
    #undef SET_TYPE
    return SUCCESS;
}
// }}}

// Function constructor / destructor {{{
static void free_function_object(function *obj TSRMLS_DC)
{
    zend_object_std_dtor(&obj->zo);
    zval_ptr_dtor(&obj->lib_instance);
    if (obj->name) {
        efree(obj->name);
    }
    if (obj->argc > 0) {
        efree(obj->args);
        efree(obj->args_types);
    }
    efree(obj);
}

zend_object_value new_function_object(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    function * data;
    zval *tmp;

    data = emalloc(sizeof(function));
    memset(data, 0, sizeof(function));

    zend_object_std_init(&data->zo, ce TSRMLS_CC);
    zend_hash_copy(data->zo.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(data, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)free_function_object, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}
// }}}

// Function::__construct() {{{
PHP_METHOD(Function, __construct)
{
    char * fnc_name;
    int  * fnc_len;
    zval * return_type;
    zval * function_signature;
    zval * lib_obj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "os|za", &lib_obj, &fnc_name, &fnc_len, &return_type, &function_signature) == FAILURE) {
		return;
	}

    FETCH_DATA_EX_EX(lib_obj, library_data, data);
    FETCH_DATA_EX(function, data_func);
    MAKE_STD_ZVAL(data_func->lib_instance);
    ZVAL_NULL(data_func->lib_instance);

    data_func->libdata = data;

    if (Z_OBJCE_P(lib_obj) != class_ce_library) {
        data_func->failed = 1;
        ctypes_exception("First argument *must* be a CTypes\\Library instance", 4);
        return;
    }
    
    // copy the instance of the library
    *(data_func->lib_instance) = *lib_obj;
    zval_copy_ctor(data_func->lib_instance);

    data_func->ptr  = GET_FUNCTION(data_func->libdata->lib, fnc_name);
    data_func->name = estrndup(fnc_name, fnc_len);

    if (!data_func->ptr) {
        ctypes_exception("Cannot find function", 4);
        data_func->failed = 1;
        return;
    }

    if (parse_type(return_type, &data_func->return_type) == FAILURE) {
        data_func->failed = 1;
        ctypes_exception("Invalid $return_type", 4);
        return;
    }

    data_func->argc = zend_hash_num_elements( Z_ARRVAL_P(function_signature) );
    if (data_func->argc > 0) {
        data_func->args = (ffi_type **)emalloc(sizeof(ffi_type*) * (data_func->argc+1));
        data_func->args_types = (long *)emalloc(sizeof(long) * (data_func->argc+1));
    } else {
        data_func->args = NULL;
        data_func->args_types = NULL;
    }

    int cnt = 0;
    FOREACH(function_signature)
        if (parse_type(&value, &(data_func->args[cnt])) == FAILURE) {
            ctypes_exception("Invalid function signature", 4);
            data_func->failed = 1;
            EXITLOOP
            return;
        }
        data_func->args_types[cnt++] = Z_LVAL(value);
    ENDFOREACH

    if (ffi_prep_cif(&(data_func->cif), FFI_DEFAULT_ABI, data_func->argc, data_func->return_type, data_func->args) != FFI_OK) {
        data_func->failed = 1;
        ctypes_exception("ffi error while creating the function bridge", 4);
        return;
    }
}
// }}}

// Function::getLibrary {{{
PHP_METHOD(Function, getLibrary)
{
    FETCH_DATA_EX(function, data);
    *return_value = *(data->lib_instance);
    zval_copy_ctor(return_value);
}
// }}}

// Function::_invoke() {{{
PHP_METHOD(Function, __invoke)
{
    FETCH_DATA_EX(function, data);
    zval ** args = NULL;
    void ** values = NULL;
    int  * need_free = NULL;
    zval * return_value_buf;
    int i;

    if (data->failed == 1) {
        ctypes_exception("cannot call this function briget, it was not initialized properly", 33);
        return;
    }

    if (ZEND_NUM_ARGS() != data->argc) {
        ctypes_exception("incorrect number of parameters", 33);
        return;
    }

    if (data->argc > 0) {
		args = (zval **)safe_emalloc(sizeof(zval *), data->argc, 0);
		zend_get_parameters_array(ht, data->argc, args);

		values = emalloc(sizeof(void*) * data->argc);
		need_free = emalloc(sizeof(int) * data->argc);

        memset(values, 0, sizeof(void*) * data->argc);
        memset(need_free, 0, sizeof(int) * data->argc);
    }

    if (data->argc > 0) {
        char error[400];
	    for (i = 0; i < data->argc; i++) {
            need_free[i] = 0;
            if (ctypes_zval_to_argument(args[i], &values[i], &need_free[i], &data->args_types[i], i, error TSRMLS_CC) == FAILURE) {
                ctypes_exception("cant parse argument", 4);
                goto release_memory;
            }
        }   
    }

    printf("Calling %x (%s) from %s\n", data->ptr, data->name, data->libdata->path);
    fflush(stdout);

    release_memory:
    if (data->argc > 0) {
	    for (i = 0; i < data->argc; i++) {
		    if (need_free[i]) {
			    efree(values[i]);
		    }
	    }

		efree(values);
		efree(args);
	}
}
// }}}

// Function::toString() {{{
PHP_METHOD(Function, __toString)
{
    FETCH_DATA_EX(function, data);
    char * text;
    text = (char *)emalloc(strlen(data->name) + strlen(data->libdata->path) + 2);

    int offset = 0;
    memcpy(text, data->libdata->path, strlen(data->libdata->path)); 
    offset += strlen(data->libdata->path); 
    text[offset++] = ':';

    memcpy(text + offset, data->name, strlen(data->name)); 
    offset += strlen(data->name);

    text[offset] = '\0';

    RETURN_STRING(text, 0);
}
// }}}

ZEND_BEGIN_ARG_INFO_EX(arginfo_z__construct, 0, 0, 1)
    ZEND_ARG_INFO(0, library)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, return_type)
    ZEND_ARG_INFO(0, function_signature)
ZEND_END_ARG_INFO()

static zend_function_entry class_methods[] = {
    PHP_ME(Function, __construct,  arginfo_z__construct, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Function, __invoke,  NULL, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Function, getLibrary,  NULL, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Function, __toString,  NULL, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    { NULL, NULL, NULL }
};

void class_register_function(TSRMLS_DC)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "CTypes\\FunctionProxy", class_methods);

    class_ce = zend_register_internal_class(&class TSRMLS_CC);
    class_ce->create_object = new_function_object;

    class_ce_function = class_ce;
}
