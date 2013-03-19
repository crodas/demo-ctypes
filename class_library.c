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

static PHP_METHOD(Library, __construct)
{
	char *filename; 
	int filename_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) { \
		return;
	}

    zval tmp;
	php_stat(filename, (php_stat_len) filename_len, FS_IS_R, &tmp TSRMLS_CC);
    if (!Z_BVAL(tmp)) {
        ctypes_exception("Cannot read the file", 1 TSRMLS_C);
        return;
    }
}

/* {{{ methods arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, 0, 1)
    ZEND_ARG_INFO(0, library_path)
    ZEND_ARG_INFO(1, options)
ZEND_END_ARG_INFO()
/* }}} */

static zend_function_entry class_methods[] = {
    PHP_ME(Library, __construct,  arginfo___construct, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    { NULL, NULL, NULL }
};

void class_register_library(TSRMLS_DC)
{
    zend_class_entry class;
    INIT_CLASS_ENTRY(class, "CTypes\\Library", class_methods);
    class_ce = zend_register_internal_class(&class TSRMLS_CC);
}
