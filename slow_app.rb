#!/usr/bin/env ruby

require "gtk2"

window = Gtk::Window.new( "Slow App" )
window.signal_connect( "destroy" ) { Gtk.main_quit }
sleep 0.6
window.show_all
Gtk.main
