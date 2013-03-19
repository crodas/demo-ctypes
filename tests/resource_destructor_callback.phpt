--TEST--
Resoure id
--FILE--
<?php
use CTypes\Library;
use CTypes\Resource;

$type1 = new Resource;
$type1->setDestructor('something');

--EXPECTF--
Fatal error: Uncaught exception 'CTypes\Exception' with message 'Expecting a valid callback' in /home/crodas/projects/newest/php-ctypes/tests/resource_destructor_callback.php:6
Stack trace:
#0 %s/tests/resource_destructor_callback.php(6): CTypes\Resource->setDestructor('something')
#1 {main}
  thrown in %s/tests/resource_destructor_callback.php on line 6
