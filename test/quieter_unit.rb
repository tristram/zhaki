require 'minitest/unit'

# Turn down the noise from the official runner.
# This implementation is dangerous, because the duplication will not track
#   changes in the official code.  (This was taken from the 1.9.1 release.)

class QuieterUnit < MiniTest::Unit

    def run args = []
        @verbose = args.delete('-v')

        filter = if args.first =~ /^(-n|--name)$/ then
            args.shift
            arg = args.shift
            arg =~ /\/(.*)\// ? Regexp.new($1) : arg
        else
            /./ # anything - ^test_ already filtered by #tests
        end

        run_test_suites filter

        @report.each_with_index do |msg, i|
            @@out.puts "\n%3d) %s" % [i + 1, msg]
        end

        if ( failures + errors == 0 )
            @@out.puts "\n#{@test_count} okay"
        end

        return 0    # Errors already reported.  We don't want noise from rake.
    end

    def self.autorun
        at_exit {
            next if $! # don't run if there was an exception
            exit_code = QuieterUnit.new.run(ARGV)
            exit false if exit_code && exit_code != 0
        } unless @@installed_at_exit
        @@installed_at_exit = true
    end
end

QuieterUnit.autorun
