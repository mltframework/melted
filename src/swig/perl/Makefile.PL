#!/bin/env perl
use ExtUtils::MakeMaker;

my $CXX = $ENV{'CXX'} || 'g++';

system( "ln -sf ../mlt.i" );
system( "swig -c++ -I../../melted++ `pkg-config mlt++ --cflags` -perl5 mlt.i" ); 
WriteMakefile(
	'NAME'    => 'mlt',
	'CC'      => '${CXX} `pkg-config mlt++ --cflags` -I../../melted++',
	'OPTIMIZE' => '-O2 -g -pipe -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -m32 -march=i386',
	'LIBS'    => ['-L../../melted++ -lmelted++'],
	'OBJECT'  => 'mlt_wrap.o',
	'DESTDIR' => $ENV{'DESTDIR'},
);

