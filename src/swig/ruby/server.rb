#!/usr/bin/env ruby

require 'mlt'

def command 
	puts "command"
end

def push 
	puts "push"
end

melted = Mlt::Melted.new( "melted-ruby", 5260 )
melted.start
melted.execute( "uadd sdl" )
listener = Mlt::Listener.new( melted, "command-received", method( :command ) )
listener = Mlt::Listener.new( melted, "push-received", method( :push ) )
melted.wait_for_shutdown
