#!/usr/bin/perl
#
#

my $last_line_cnt=1;
my $last_line;
while ( <> )
{ 
	chomp;
	if( $last_line )
	{
		if( $last_line eq $_ )
		{
			++ $last_line_cnt;
			next;
		}
		print "$last_line_cnt $last_line\n";
		$last_line_cnt=1;
	}
	$last_line = $_;
}
if( $last_line )
{
	print "$last_line_cnt $last_line\n";
}
