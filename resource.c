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

/*
** Global hash with the list of list_resources
** created through the extension.
*/  
static HashTable list_resources;

/*
** Register a temporary resource ID, so we can wrap the pointer
** as a PHP zval and do the destructor callbacks. 
** Because this resource doesn't have a `callback` function, when it is 
** released it won't end up doing a double free.
*/
static int temp_resource_id;

/*
**  Module number, perhaps there is another way to get this number
*/
int mnumber;

ZEND_BEGIN_MODULE_GLOBALS(resource)
    HashTable * resources;
ZEND_END_MODULE_GLOBALS(resource)

ZEND_DECLARE_MODULE_GLOBALS(resource);

#if ZTS
    #define G(x) (TSRMG(resource_globals_id, zend_resource_globals*, x))
#else 
    #define G(x) (resource_globals.x)
#endif

// Global variables (per request) {{{
static void resource_globals_ctor(zend_resource_globals * ptr TSRMLS_DC)
{
	zend_hash_init(&ptr->resources, 50, NULL, NULL, 1);
}
    zend_hash_destroy(&ptr->resources);
}

int ctypes_resource_request_destroy(TSRMLS_DC)
{
    resource_globals_dtor(&resource_globals TSRMLS_CC);
}

int ctypes_resource_request_init(TSRMLS_DC)
{
    #if ZTS
    ts_allocate_id(&resource_globals_id,
            sizeof(zend_resource_globals),
            (ts_allocate_ctor)resource_globals_ctor,
            (ts_allocate_dtor)resource_globals_dtor);
    #else
    resource_globals_ctor(&resource_globals TSRMLS_CC);
    #endif
}
// }}}

typedef struct {
    zval * resource_object;
    zval * callback;
    int resource_id;
} resource_entry;


static void ctypes_resource_destructor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    resource_entry * re;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;

    if (rsrc->type == temp_resource_id) {
        return;
    }

    if (!rsrc->ptr) {
        ctypes_exception("Internal error when freeing memory", 4);
        return;
    }

	if (zend_hash_index_find(&list_resources, rsrc->type, (void **) &re)==SUCCESS) {
        if (re->callback) {
            if (zend_fcall_info_init(re->callback, 0, &fci, &fcc, NULL, NULL TSRMLS_CC) == FAILURE) {
                ctypes_exception("failed setup for destructor callback", 10);
                return;
            }

            zval * tmp, *args, *retval;
            MAKE_STD_ZVAL(tmp);
            MAKE_STD_ZVAL(args);

            /*
            **  Do something nasty, wrap the resource which is supposed to 
            **  be free by "us" into a (zval *) again and then do a PHP callback.
            **  They are responsible for freeing this resource.
            */  
            array_init_size(args, (uint)1);

            zend_register_resource(tmp, rsrc->ptr, temp_resource_id);
            add_next_index_zval(args, tmp);

            if (zend_fcall_info_call(&fci, &fcc, &retval, args TSRMLS_CC) != SUCCESS) {
                ctypes_exception("failed destructor callback", 10);
            }

            zval_ptr_dtor(&args);
            Z_DELREF_P(re->callback);
            Z_DELREF_P(re->resource_object);
        } else {
            ctypes_exception("Trying to destroy resource but couldn't find any callback", 2);
        }
    } else {
        ctypes_exception("Unknown resource", 2);
    }
}

int ctypes_resource_init(TSRMLS_DC)
{
	int retval;

	retval = zend_hash_init(&list_resources, 50, NULL, NULL, 1);

    temp_resource_id = ctypes_resource_create(TSRMLS_CC);

	return retval;
}

void ctypes_resource_destroy()
{
    zend_hash_destroy(&list_resources);
}

int ctypes_resource_exists(int id TSRMLS_DC)
{
    resource_entry * re;
	return zend_hash_index_find(&list_resources, id, (void **)&re);
}

int ctypes_new_resource(int type, zval * output, void * pointer)
{
    resource_entry * re;
	if (zend_hash_index_find(&list_resources, type, (void **)&re) == SUCCESS) {
        if (!re->callback) {
            ctypes_exception("Resource must have a destructor", 343);
            return FAILURE;
        }
        Z_ADDREF_P(re->callback);
        Z_ADDREF_P(re->resource_object);
        return zend_register_resource(output, pointer, type);
    }
    return FAILURE;
}

int ctypes_resource_add_destructor(int id, zval * this, zval * callback TSRMLS_DC)
{
    resource_entry * re;

	if (zend_hash_index_find(&list_resources, id, (void **) &re)==SUCCESS) {
        re->callback = callback;
        re->resource_object = this;

        Z_ADDREF_P(this);
        Z_ADDREF_P(callback);

        return SUCCESS;
    }
    return FAILURE;
}

int ctypes_resource_create(TSRMLS_DC)
{
    int resource_id;
    resource_entry re;

    resource_id = zend_register_list_destructors_ex(ctypes_resource_destructor, NULL, "ctypes", mnumber);

    re.callback = NULL;
    re.resource_id = resource_id;

    /*
    ** Match the Ids of our private resource array
    ** with php's list_resources array.
    */
	list_resources.nNextFreeElement = resource_id;
	if (zend_hash_next_index_insert(&list_resources, (void *) &re, sizeof(resource_entry), NULL)==FAILURE) {
		return FAILURE;
	}
    
    return resource_id;
}
