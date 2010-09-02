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
        sleep 0.5     # ensure app has finished starting
        @driver = ApplicationDriver.new( "Simple App" )
        assert_instance_of( ApplicationDriver, @driver )
    end

    def test_can_find_app_which_is_busy_starting
        @driver = ApplicationDriver.new( "Simple App" )
        assert_instance_of( ApplicationDriver, @driver )
    end

    def test_failing_to_find_nonexistant_app
        # Ruby bug #2413 is that the message is not checked
        assert_raises( RuntimeError, "No such application running." ) do
            @driver = ApplicationDriver.new( "Bogus App" )
        end
    end

    def test_name_of_app_is_required
        assert_raises( ArgumentError ) do
            @driver = ApplicationDriver.new
        end
    end
end

class TimeoutTest < MiniTest::Unit::TestCase

    def setup
        @pid = spawn( "./slow_app.rb" )
    end

    def teardown
        Process.kill( :TERM, @pid )
        unless @driver.nil?
            @driver.closedown
        end
        Process.wait
    end

    def test_slow_app_missed_with_default_timeout
        assert_raises( RuntimeError ) do
            ApplicationDriver.new( "Slow App" )
        end
    end

    def test_slow_app_found_with_increased_timeout
        @driver = ApplicationDriver.new( "Slow App", 1500 )
    end
end
