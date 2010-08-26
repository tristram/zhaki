#!/usr/bin/env ruby

require "lib/application_list"

list = ApplicationList.new
list.count.times do |i|
    number = sprintf( "%2d", i )
    puts "App #{number}: #{list.next}"
end
