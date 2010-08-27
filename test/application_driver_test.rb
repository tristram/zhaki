require "test/quieter_unit.rb"
require "lib/application_driver"

class ApplicationDriverTest < MiniTest::Unit::TestCase

    def setup
        @pid = spawn( "./simple_app.rb" )
        sleep 0.5         # This is entirely bogus, the wait needs to be in AppDriver.
    end

    def teardown
        Process.kill( :TERM, @pid )
        unless @driver.nil?
            @driver.closedown
        end
        Process.wait
    end

    def test_can_find_sample_app
        @driver = ApplicationDriver.new( "Simple App" )

        assert_instance_of( ApplicationDriver, @driver )
    end

    def test_failing_to_find_app
        # Ruby bug #2413 is that the message is not checked
        assert_raises( RuntimeError, "No such application running." ) do
            @driver = ApplicationDriver.new( "Bogus App" )
        end
    end
end
