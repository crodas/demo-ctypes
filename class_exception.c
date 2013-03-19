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

static zend_class_entry * class_ce;

void ctypes_exception(char * message, int errorno TSRMLS_DC)
{
    zval * tc_ex;

    MAKE_STD_ZVAL(tc_ex);
    object_init_ex(tc_ex, class_ce);

    zend_update_property_string(class_ce, tc_ex, "message", sizeof("message") - 1,
            message TSRMLS_CC);
    zend_throw_exception_object(tc_ex TSRMLS_CC);
}

void class_register_exception(TSRMLS_DC)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "CTypes\\Exception", NULL);
    class_ce = zend_register_internal_class_ex(&class,  zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
}
