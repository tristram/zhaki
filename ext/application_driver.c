#include "ruby.h"

#include <cspi/spi.h>

// Declarations

typedef struct {
    char*                       application_title;
    int                         found_application;
    Accessible*                 frame;
    AccessibleEventListener*    window_listener;
} Driver;

static void initialize( Driver* dp, char* application_title );
static void initialize_SPI();
static void listen_for_application_activation( Driver* dp );
static void check_for_application(
       const AccessibleEvent* event, void* user_data );
static void check_for_matching_frame( Driver* dp, Accessible* accessible );
static void signal_main_loop_to_stop();
static void search_desktop_for_application( Driver* dp );
static SPIBoolean exception_handler( SPIException* err, SPIBoolean is_fatal );
static void search_for_application( Driver* dp, Accessible* desktop );
static void search_for_matching_frame( Driver* dp, Accessible* application );
static void remove_listener( Driver* dp );
static void closedown( Driver* dp );
static VALUE allocate_structure( VALUE class_name );
static Driver* retrieve_structure( VALUE self );
static void free_structure( Driver* dp );
static VALUE wrap_initialize( VALUE self, VALUE application_title );
static VALUE wrap_closedown( VALUE self );

// C functions

static void initialize( Driver* dp, char* application_title ) {
    dp->application_title = application_title;
    initialize_SPI();
    listen_for_application_activation( dp );
    search_desktop_for_application( dp );
    if ( dp->found_application ) {
        remove_listener( dp );
        closedown( dp );
        return;
    }

    SPI_event_main();

    remove_listener( dp );
    closedown( dp );
    if ( ! dp->found_application )
        rb_raise( rb_eRuntimeError, "No such application running." );
}

static void initialize_SPI() {
    int result = SPI_init();
    if ( result == 2 )
        rb_raise( rb_eSystemCallError, "Assistive Technologies not enabled." );
    if ( result != 0 )
        rb_raise( rb_eSystemCallError, "SPI_init returned error %d.", result );
}

static void listen_for_application_activation( Driver* dp ) {
    dp->window_listener = SPI_createAccessibleEventListener(
        check_for_application,
        dp
    );
    int success = SPI_registerGlobalEventListener(
        dp->window_listener,
        "window:activate"
    );
    if ( ! success )
        rb_raise( rb_eSystemCallError,
                "Failed to register a GlobalEventListener" );
}

static void check_for_application(
       const AccessibleEvent* event, void* user_data ) {
    Driver* dp = user_data;
    Accessible* source = event->source;
    check_for_matching_frame( dp, source );
    if ( dp->found_application)
        signal_main_loop_to_stop();
}

static void check_for_matching_frame( Driver* dp, Accessible* accessible ) {
    int role = Accessible_getRole( accessible );
    if ( role == SPI_ROLE_FRAME ) {
        char* frame_name = Accessible_getName( accessible );
        if ( strcmp(frame_name, dp->application_title) == 0 ) {
            dp->frame = accessible;
            dp->found_application = TRUE;
        }
        SPI_freeString( frame_name );
    }
}

static void signal_main_loop_to_stop() {
    SPI_event_quit();       // This call returns.
                            // The main loop will stop asynchronously.
}

static void search_desktop_for_application( Driver* dp ) {
    SPIExceptionHandler handler = exception_handler;
    if ( ! SPI_exceptionHandlerPush(&handler) )
        rb_raise( rb_eSystemCallError, "Failed to add an ExceptionHandler." );

    int count = SPI_getDesktopCount();
    if ( count != 1 )
        rb_raise( rb_eSystemCallError,
                "There are %d desktops.  Expected to find one.", count );

    Accessible* desktop = SPI_getDesktop( 0 );
    search_for_application( dp, desktop );
    Accessible_unref( desktop );

    SPI_exceptionHandlerPop();
}

static SPIBoolean exception_handler( SPIException* err, SPIBoolean is_fatal ) {
    // We wish to swallow a non-fatal error...
    if ( is_fatal )
        return FALSE;       // We haven't dealt with it.

    // ...which has a specific description.
    char* description = SPIException_getDescription( err );
    int result = strcmp( description, "IDL:omg.org/CORBA/COMM_FAILURE:1.0" );
    SPI_freeString( description );

    if ( result != 0 )
        return FALSE;       // We haven't dealt with it.
    else
        return TRUE;        // We have dealt with it -- ie ignore it.
}

static void search_for_application( Driver* dp, Accessible* desktop ) {
    int child_count = Accessible_getChildCount( desktop );
    if ( child_count == 0 )
        rb_raise( rb_eSystemCallError,
                "The Desktop has no children. Have you re-logged in?" );

    int i;
    for ( i = 0; i < child_count; ++i ) {
        Accessible* child = Accessible_getChildAtIndex( desktop, i );
        if ( child == NULL )
            continue;

        search_for_matching_frame( dp, child );
        Accessible_unref( child );
        if ( dp->found_application )
            break;
    }
}

static void search_for_matching_frame( Driver* dp, Accessible* application ) {
    int child_count = Accessible_getChildCount( application );
    int i;
    for ( i = 0; i < child_count; ++i ) {
        Accessible* child = Accessible_getChildAtIndex( application, i );
        check_for_matching_frame( dp, child );
        Accessible_unref( child );
        if ( dp->found_application )
            break;
    }
}

static void remove_listener( Driver* dp ) {
    int success = SPI_deregisterGlobalEventListenerAll( dp->window_listener );
    if ( ! success )
        rb_raise( rb_eSystemCallError, "Failed to deregister GlobalEventListener." );
    AccessibleEventListener_unref( dp->window_listener );
}

static void closedown( Driver* dp ) {
    Accessible_unref( dp->frame );
    int leaks = SPI_exit();
    if ( leaks != 0 )
        rb_raise( rb_eSystemCallError, "There were %d SPI memory leaks.", leaks );
}

// Memory handling

static VALUE allocate_structure( VALUE class_name ) {
    Driver* dp = calloc( 1, sizeof(Driver) );    // guaranteed zeroed memory
    return Data_Wrap_Struct(
        class_name,         // class owning this memory
        0,                  // no mark routine required
        free_structure,     // function called when garbage collected
        dp
    );
}

static Driver* retrieve_structure( VALUE self ) {
    Driver* dp;
    Data_Get_Struct( self, Driver, dp );
    return dp;
}

static void free_structure( Driver* dp ) {
    free( dp );
}

// Wrap C functions as Ruby methods

static VALUE wrap_initialize( VALUE self, VALUE application_title ) {
    Driver* dp = retrieve_structure( self );
    initialize( dp, RSTRING_PTR(application_title) );
    return Qnil;
}

static VALUE wrap_closedown( VALUE self ) {
    Driver* dp = retrieve_structure( self );
    closedown( dp );
    return Qnil;
}
    
// Export methods

void Init_application_driver() {
    VALUE driver = rb_define_class( "ApplicationDriver", rb_cObject );
    rb_define_alloc_func( driver, allocate_structure );
    rb_define_method( driver, "initialize", wrap_initialize, 1 );
    rb_define_method( driver, "closedown", wrap_closedown, 0 );
}
