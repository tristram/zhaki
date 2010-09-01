require "test/quieter_unit.rb"
require "lib/application_driver"

class ApplicationDriverTest < MiniTest::Unit::TestCase

    def setup
        @pid = spawn( "./simple_app.rb" )
    end

    def teardown
        Process.kill( :TERM, @pid )
        unless @driver.nil?
            @driver.closedown
        end
        Process.wait
    end

    def test_can_find_already_started_app
        sleep 1     # ensure app has already started
        @driver = ApplicationDriver.new( "Simple App" )
        assert_instance_of( ApplicationDriver, @driver )
    end

    def test_can_find_app_which_is_busy_starting
        @driver = ApplicationDriver.new( "Simple App" )
        assert_instance_of( ApplicationDriver, @driver )
    end

=begin
Currently broken because no timeout on waiting for app.
    def test_failing_to_find_nonexistant_app
        # Ruby bug #2413 is that the message is not checked
        assert_raises( RuntimeError, "No such application running." ) do
            @driver = ApplicationDriver.new( "Bogus App" )
        end
    end
=end
end
