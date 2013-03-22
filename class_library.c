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

/* macros {{{ */
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
/* }}} */

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

    object_init_ex(return_value, class_ce_fnc);

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

// Library::getLibraryPath {{{
static PHP_METHOD(Library, __toString)
{
    FETCH_DATA(data);
    RETURN_STRING(data->path, 0);
}
// }}}

// Library::getLibraryPath {{{
static PHP_METHOD(Library, getLibraryPath)
{
    FETCH_DATA(data);
    RETURN_STRING(data->path, 0);
}
// }}}

// }}}

static int ctypes_zval_to_argument(zval * arg, void ** ptr,  int *should_free, long arg_type, int i, char * error TSRMLS_DC)
{
    #define IF_IS_EX(x, code) if (IS_TYPE(x, arg)) { \
            arg; \
            return SUCCESS; \
        }

    #define boolean unsigned char
    #define IF_IS_WRAP_AS(x, type, op) IF_IS_EX(x, 
            *ptr = emalloc(sizeof(type)); \
            *should_free = 1; \
            convert_to_##x(arg);
            **(type**) = op(arg);
        )
        


    if (IS_NATIVE(arg_type) && !IS_PTR(arg_type) && !IS_PTRPTR(arg_type)) {
        should_free = 0;

        IF_IS_WRAP_AS(BOOL,     boolean,    Z_BVAL_P);
        IF_IS_WRAP_AS(LONG,     long,       Z_LVAL_P);
        IF_IS_WRAP_AS(DOUBLE,   double,     Z_DVAL_P);
        IF_IS_WRAP_AS(STRING,   string      Z_STRVAL);

        return FAILURE;
    } else 
        // we expect a pointer :-)
    }
}

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

    if (Z_OBJCE_P(lib_obj) != class_ce) {
        ctypes_exception("First argument *must* be a CTypes\\Library instance", 4);
        return;
        
    }

    FETCH_DATA_EX_EX(lib_obj, library_data, data);
    FETCH_DATA_EX(function, data_func);

    data_func->libdata = data;
    
    // copy the instance of the library
    MAKE_STD_ZVAL(data_func->lib_instance);
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

    printf("%d arguments\n", data_func->argc);fflush(stdout);
    if (ffi_prep_cif(&(data_func->cif), FFI_DEFAULT_ABI, data_func->argc, data_func->return_type, data_func->args) != FFI_OK) {
        data_func->failed = 1;
        ctypes_exception("ffi error while creating the function bridge", 4);
        return;
    }
    printf("%s\n", fnc_name);fflush(stdout);
}

PHP_METHOD(Function, getLibrary)
{
    FETCH_DATA_EX(function, data);
    *return_value = *(data->lib_instance);
    zval_copy_ctor(return_value);
}

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
                    ctypes_excpetion(error, 4);
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

PHP_METHOD(Function, __toString)
{
    FETCH_DATA_EX(function, data);
    char * text;
    text = (char *)emalloc(strlen(data->name) + strlen(data->lib->path) + 2);

    int offset = 0;
    memcpy(text, data->lib->path, strlen(data->lib->path)); 
    offset += strlen(data->lib->path); 
    text[offset++] = ':';

    memcpy(text + offset, data->name, strlen(data->name)); 
    offset += strlen(data->name);

    text[offset] = '\0';

    RETURN_STRING(text, 0);
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
ZEND_BEGIN_ARG_INFO_EX(arginfo_z__construct, 0, 0, 1)
    ZEND_ARG_INFO(0, library)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, return_type)
    ZEND_ARG_INFO(0, function_signature)
ZEND_END_ARG_INFO()
/* }}} */

static zend_function_entry class_methods[] = {
    PHP_ME(Library, __construct,  arginfo___construct, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Library, getLibraryPath,  arginfo__getLibraryPath, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Library, getFunction,  arginfo__getFunction, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Library, __toString,  arginfo__getFunction, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    { NULL, NULL, NULL }
};

static zend_function_entry class_methods_fnc[] = {
    PHP_ME(Function, __construct,  arginfo_z__construct, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Function, __invoke,  NULL, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Function, getLibrary,  NULL, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(Function, __toString,  arginfo__getFunction, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    { NULL, NULL, NULL }
};

void class_register_library(TSRMLS_DC)
{
    zend_class_entry class, class_fnc;
    INIT_CLASS_ENTRY(class, "CTypes\\Library", class_methods);
    INIT_CLASS_ENTRY(class_fnc, "CTypes\\FunctionProxy", class_methods_fnc);

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
