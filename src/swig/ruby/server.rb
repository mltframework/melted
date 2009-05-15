#!/usr/bin/env ruby

require 'mlt'
require 'melted'

def command 
	puts "command"
end

def push 
	puts "push"
end

melted = Melted::Melted.new( "melted-ruby", 5260 )
melted.start
melted.execute( "uadd sdl" )
listener = Melted::Listener.new( melted, "command-received", method( :command ) )
listener = Melted::Listener.new( melted, "push-received", method( :push ) )
melted.wait_for_shutdown
