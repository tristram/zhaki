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
        assert_equal( "nautilus", @list.next )
    end

    def test_list_count
        # bogus test -- it won't be 17 next time
        assert_equal( 17, @list.count )
    end
end
