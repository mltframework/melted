#!/bin/env perl
use ExtUtils::MakeMaker;

my $CXX = $ENV{'CXX'} || 'g++';

system( "ln -sf ../melted.i" );
system( "swig -c++ -I../../melted++ `pkg-config mlt++ --cflags` -perl5 melted.i" ); 
WriteMakefile(
	'NAME'    => 'melted',
	'CC'      => '${CXX} `pkg-config mlt++ --cflags` -I../../melted++',
	'OPTIMIZE' => '-O2 -g -pipe -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -m32 -march=i386',
	'LIBS'    => ['-L../../melted++ -lmelted++'],
	'OBJECT'  => 'melted_wrap.o',
	'DESTDIR' => $ENV{'DESTDIR'},
);

