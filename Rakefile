task :default do
    sh "rake -s -T"
end

desc "Build lister"
task :build => "lister"

file "lister" => "lister.c" do
    sh "gcc -Wall -std=c99 lister.c -o lister $(pkg-config --cflags --libs libspi-1.0 cspi-1.0)"
end
