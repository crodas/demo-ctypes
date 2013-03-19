--TEST--
Resoure id
--FILE--
<?php
use CTypes\Library;
use CTypes\Resource;

$type1 = new Resource;
$type2 = new Resource;

var_dump($type1->getResourceId() !== $type2->getResourceId());

--EXPECT--
bool(true)
