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
** Global hash with the list of resources
** created through the extension.
*/  
static HashTable resources;

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


typedef struct {
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

	if (zend_hash_index_find(&resources, rsrc->type, (void **) &re)==SUCCESS) {
        printf("here\n");fflush(stdout);
        if (re->callback) {
            printf("%x\n\n", re->callback);fflush(stdout);
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

	retval = zend_hash_init(&resources, 50, NULL, NULL, 1);
	resources.nNextFreeElement=  1;	/* we don't want resource type 0 */

    temp_resource_id = ctypes_resource_create(TSRMLS_CC);

	return retval;
}

void ctypes_resource_destroy()
{
	zend_hash_destroy(&resources);
}

int ctypes_resource_exists(int id TSRMLS_DC)
{
    resource_entry * re;
	return zend_hash_index_find(&resources, id, (void **)&re);
}

int ctypes_resource_add_destructor(int id, zval * callback TSRMLS_DC)
{
    resource_entry * re;

	if (zend_hash_index_find(&resources, id, (void **) &re)==SUCCESS) {
        if (re->callback) {
            zval_ptr_dtor(&re->callback);
        }
        MAKE_STD_ZVAL(re->callback);
        *(re->callback) = *callback;
        zval_copy_ctor(re->callback);

	    //zend_hash_index_update(&resources, id, (void *) re, sizeof(resource_entry), NULL);
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
    ** with php's resources array.
    */
	resources.nNextFreeElement = resource_id;
	if (zend_hash_next_index_insert(&resources, (void *) &re, sizeof(resource_entry), NULL)==FAILURE) {
		return FAILURE;
	}
    
    return resource_id;
}
