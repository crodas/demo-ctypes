--TEST--
Some test
--FILE--
<?php
use CTypes\Library;
use CTypes\Resource;
use CTypes\Type;

$weird_type = new Resource("some name");
$weird_type->setDestructor(function($w) {
    echo "destroying\n";        
    var_dump($w);
});

class Foobar {}

$c = new Library("/lib64/libc.so.6");

var_dump(array(
    $c, 
    $weird_type, 
    $weird_type->getResourceId(),
    $weird_type->getResourceDemo(),
    $weird_type->getResourceDemo(),
    Type::tInteger,
    Type::tChar,
    Type::tString,
    Type::tFloat,
    Type::tPtr | Type::tInteger,
    $c->getLibraryPath(),
    $var = $c->getFunction('isalpha', $weird_type, array(Type::tChar, $weird_type->getResourceId()| Type::tPtr  )),
    is_callable( $c->getFunction('isalpha', Type::tBool, array(Type::tChar)) ),
    $var . '',
));
$memory = new Resource;
$malloc = $c->getFunction('malloc', $memory, array(Type::tInteger));
$memory->setDestructor($c->getFunction('free', NULL, array($memory)));

$bytes = $malloc(250);
unset($bytes);


var_dump($var->getLibrary());
var_dump($var->getLibrary());
$var(3, $c);
--EXPECT--
hi there
