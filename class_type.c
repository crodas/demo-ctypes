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

static zend_function_entry class_methods[] = {
    { NULL, NULL, NULL }
};

void class_register_type(TSRMLS_DC)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "CTypes\\Type", class_methods);

    class_ce = zend_register_internal_class(&class TSRMLS_CC);

    zend_declare_class_constant_long(class_ce, "tInteger", strlen("tInteger"), T_LONG TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tInt", strlen("tInt"), T_LONG TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tFloat", strlen("tFloat"), T_DOUBLE TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tString", strlen("tString"), T_STRING TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tChar", strlen("tChar"),  T_CHAR TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tBool", strlen("tBool"), T_BOOL TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tPtr", strlen("tPtr"), T_PTR TSRMLS_CC);
    zend_declare_class_constant_long(class_ce, "tPtrPtr", strlen("tPtrPtr"), T_PTRPTR TSRMLS_CC);
}
