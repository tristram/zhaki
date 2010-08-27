#!/usr/bin/env ruby

require "gtk2"

window = Gtk::Window.new( "Simple App" )
window.signal_connect( "destroy" ) { Gtk.main_quit }
window.show_all
Gtk.main
