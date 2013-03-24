--TEST--
Fixed memory issues
--FILE--
<?php
use CTypes\Library;
use CTypes\Resource;
use CTypes\Type;
use CTypes\FunctionProxy;

$weird_type = new Resource("some name");
$weird_type->setDestructor(function($w) {
    echo "destroying\n";        
    var_dump($w);
});

$c = new Library("/lib64/libc.so.6");
function foobar() {};

$d[] = $weird_type->getResourceDemo();
$d[] = $weird_type->getResourceDemo();

foobar(array(
    $d,
    new FunctionProxy($c, 'some_non_existen_function', $weird_type, array($weird_type->getResourceId() | Type::tPtr)),
    $c->getFunction('some_non_existen_function', $weird_type, array($weird_type->getResourceId() | Type::tPtr)),
));

--EXPECTF--
Fatal error: Uncaught exception 'CTypes\Exception' with message 'Cannot find function' in %s/tests/memory_isses.php:%d
Stack trace:
#0 [internal function]: CTypes\FunctionProxy->__construct(%s)
#1 %s/tests/memory_isses.php(%d): CTypes\Library->getFunction(%s)
#2 {main}
  thrown in %s/tests/memory_isses.php on line %d
destroying
resource(%d) of type (ctypes)
destroying
resource(%d) of type (ctypes)
