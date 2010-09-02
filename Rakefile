task :default do
    sh "rake -s -T"
end

desc "Run tests."
task :test => [:build] do
    sh "ruby test/*_test.rb"
end

desc "Build C extension."
task :build => ["tmp", "lib", "lib/application_driver.so"]

file "tmp" do
    directory "tmp"
end

file "lib" => ["lib64", "lib32"] do
    hardware = %x{uname -m}.chomp
    if hardware.eql? "x86_64"
        sh "ln -s lib64 lib"
    else
        sh "ln -s lib32 lib"
    end
end

file "lib64" do
    directory "lib64"
end

file "lib32" do
    directory "lib32"
end

file "lib/application_driver.so" => ["ext/application_driver.c"] do |target|
    basename = File.basename( target.name, ".so" )
    rm( Dir["tmp/*"] )
    cp( target.prerequisites, "tmp" )
    File.open( "tmp/extconf.rb", "w" ) do |config_file|
        config_file.puts( "require 'mkmf'" )
        config_file.puts( "pkg_config('cspi-1.0')" )
        config_file.puts( "create_makefile('#{basename}')" )
    end
    sh "cd tmp; ruby extconf.rb"
    sh "make -C tmp"
    cp( "tmp/#{basename}.so", "lib" )
end
