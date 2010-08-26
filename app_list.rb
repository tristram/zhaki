#!/usr/bin/env ruby

require "lib/application_list"

list = ApplicationList.new
i = 0
while ( name = list.next )
    i += 1
    number = sprintf( "%2d", i )
    puts "App #{number}: #{name}"
end
