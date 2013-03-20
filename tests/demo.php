<?php
use CTypes\Library;
use CTypes\Resource;

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
    Library::tInteger,
    Library::tChar,
    Library::tString,
    Library::tFloat,
    Library::tPtr | Library::tInteger,
    $c->getLibraryPath(),
    $c->getFunction('isalpha', $weird_type, array(Library::tChar, $weird_type->getResourceId()| Library::tPtr  )),
    $c->getFunction('isalpha', Library::tBool, array(Library::tChar)),
));

