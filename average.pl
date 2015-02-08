#!/usr/bin/perl 

$max = 0;
while (<>) {
 $sum = $sum + $_;
 $max = $_ if ($_ > $max);
 $count++;
}
$avg=$sum/$count;
print "$count numbers total=$sum max=$max mean=$avg\n";
