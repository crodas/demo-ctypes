--TEST--
Some test
--FILE--
<?php
use CTypes\Library;
use CTypes\Resource;

$weird_type = new Resource("some name");
$weird_type->setDestructor(function($w) {
    echo "destroying\n";        
    var_dump($w);
});

$c = new Library("demo.php");
var_dump(array(
    $c, 
    $weird_type, 
    $weird_type->getResourceId(),
    $weird_type->getResourceDemo(),
    $weird_type->getResourceDemo(),
));

--EXPECT--
hi there
