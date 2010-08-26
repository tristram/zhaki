require "test/quieter_unit.rb"
require "lib/application_list"

class ApplicationListTest < MiniTest::Unit::TestCase

    def setup
        @list = ApplicationList.new
    end

    def teardown
        unless @list.nil?
            @list.closedown
        end
    end

    def test_can_create_application_list
        assert_instance_of( ApplicationList, @list )
    end

    def test_calling_next_on_list
        # this is highly flaky -- we don't know that nautilus is first
        assert_equal( "x-nautilus-desktop", @list.next )
    end
end
