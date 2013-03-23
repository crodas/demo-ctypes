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

$memo = $weird_type->getResourceDemo();

new FunctionProxy($weird_type, 'some name', $weird_type);

--EXPECTF--
Fatal error: Uncaught exception 'CTypes\Exception' with message 'First argument *must* be a CTypes\Library instance' in %s/tests/memory_isses.php:%d
Stack trace:
#0 %s/tests/memory_isses.php(%d): CTypes\FunctionProxy->__construct(Object(CTypes\Resource), 'some name', Object(CTypes\Resource))
#1 {main}
  thrown in %s/tests/memory_isses.php on line %d
destroying
resource(%d) of type (ctypes)
