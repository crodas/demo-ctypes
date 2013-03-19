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

extern zend_module_entry ctypes_module_entry;
int ctypes_resource_init(TSRMLS_DC);
void ctypes_resource_destroy();
int ctypes_resource_create(TSRMLS_DC);
int ctypes_resource_add_destructor(int, zend_fcall_info * TSRMLS_DC);

extern int mnumber;

#define phpext_ctypes_ptr &ctypes_module_entry

#ifdef PHP_WIN32
    #define PHP_ctypes_API __declspec(dllexport)
#else
    #define PHP_ctypes_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define PHP_CTYPES_VERSION "0.1"

PHP_MINIT_FUNCTION(ctypes);
PHP_MINFO_FUNCTION(ctypes);

#define CALL_METHOD(Class, Method, retval, thisptr)  PHP_FN(Class##_##Method)(0, retval, NULL, thisptr, 0 TSRMLS_CC);


#endif /* PHP_ctypes_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
