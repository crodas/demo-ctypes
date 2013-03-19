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
static temp_resource_id;

/*
**  Module number, perhaps there is another way to get this number
*/
int mnumber;


typedef struct {
    zend_fcall_info * callback;
    int resource_id;
} resource_entry;


static void ctypes_resource_destructor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    resource_entry * re;
	if (zend_hash_index_find(&resources, rsrc->type, (void **) &re)==SUCCESS) {
        if (re->callback) {
            zval * tmp, *args;
            MAKE_STD_ZVAL(tmp);
            MAKE_STD_ZVAL(args);

            /*
            **  Do something nasty, wrap the point which is supposed to 
            **  be free by us into a zval * again and do a PHP callback.
            **  They are responsible for freeing this resource.
            */  
            zend_register_resource(tmp, rsrc->ptr, temp_resource_id);
            add_next_index_zval(args, tmp);

	        zend_fcall_info_args(re->callback, args TSRMLS_CC);

	        if (zend_call_function(re->callback, NULL TSRMLS_CC) == FAILURE) {
            }

            efree(args);
            efree(tmp);
        }
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

int ctypes_resource_add_destructor(int id, zend_fcall_info * callback TSRMLS_DC)
{
    resource_entry * re;
	if (zend_hash_index_find(&resources, id, (void **) &re)==SUCCESS) {
        re->callback = callback;
        Z_ADDREF_P(callback);
	    zend_hash_index_update(&resources, id, (void *) re, sizeof(resource_entry), NULL);
    } else {
        return FAILURE;
    }
    return SUCCESS;
}

int ctypes_resource_create(TSRMLS_DC)
{
    int resource_id;
    resource_entry re;

    resource_id = zend_register_list_destructors_ex(NULL,  ctypes_resource_destructor, "ctypes", mnumber);

    re.callback = 0;
    re.resource_id = resource_id;


	if (zend_hash_next_index_insert(&resources, (void *) &re, sizeof(resource_entry), NULL)==FAILURE) {
		return FAILURE;
	}
    
    return resource_id;
}
