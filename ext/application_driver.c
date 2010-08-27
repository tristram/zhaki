#include "ruby.h"

#include <cspi/spi.h>

// Declarations

typedef struct {
    Accessible*         desktop;
    SPIExceptionHandler handler;
    Accessible*         application;
} Driver;

static void initialize( Driver* dp, char* application_title );
static void search_for_application( Driver* dp, char* application_title );
static int app_contains_matching_frame( Driver* dp, Accessible* application,
        char* application_title );
static void closedown( Driver* dp );
static SPIBoolean exception_handler( SPIException* err, SPIBoolean is_fatal );
static void free_structure( Driver* dp );
static VALUE allocate_structure( VALUE class_name );
static Driver* retrieve_structure( VALUE self );
static VALUE wrap_initialize( VALUE self, VALUE application_title );
static VALUE wrap_closedown( VALUE self );

// C functions

static void initialize( Driver* dp, char* application_title ) {
    int                 result;
    
    result = SPI_init();
    if ( result == 2 )
        rb_raise( rb_eSystemCallError, "Assistive Technologies not enabled." );
    if ( result != 0 )
        rb_raise( rb_eSystemCallError, "SPI_init returned error %d.", result );

    result = SPI_getDesktopCount();
    if ( result != 1 )
        rb_raise( rb_eSystemCallError,
                "There are %d desktops.  Expected to find one.\n", result );

    dp->desktop = SPI_getDesktop( 0 );

    search_for_application( dp, application_title );
}

static void search_for_application( Driver* dp, char* application_title ) {
    int                 child_count;
    int                 i;
    Accessible*         child;

    child_count = Accessible_getChildCount( dp->desktop );
    if ( child_count == 0 )
        rb_raise( rb_eSystemCallError,
                "The Desktop has no children. Have you re-logged in?" );

    dp->handler = exception_handler;
    if ( ! SPI_exceptionHandlerPush(&dp->handler) )
        rb_raise( rb_eSystemCallError, "Failed to add an ExceptionHandler." );

    for ( i = 0; i < child_count; ++i ) {
        child = Accessible_getChildAtIndex( dp->desktop, i );
        if ( child == NULL )
            continue;

        if ( app_contains_matching_frame(dp, child, application_title) )
            return;
        Accessible_unref( child );
    }

    closedown( dp );
    rb_raise( rb_eRuntimeError, "No such application running." );
}

static int app_contains_matching_frame( Driver* dp, Accessible* application,
        char* application_title ) {
    int                 grandchild_count;
    int                 i;
    Accessible*         grandchild;
    int                 role;

    grandchild_count = Accessible_getChildCount( application );
    for ( i = 0; i < grandchild_count; ++i ) {
        grandchild = Accessible_getChildAtIndex( application, i );
        role = Accessible_getRole( grandchild );

        if ( role == SPI_ROLE_FRAME ) {
            char* frame_name = Accessible_getName( grandchild );
            if ( 0 == strcmp(frame_name, application_title) ) {
                SPI_freeString( frame_name );
                Accessible_unref( grandchild );
                dp->application = application;
                return TRUE;
            }
            SPI_freeString( frame_name );
        }

        Accessible_unref( grandchild );
    }
    return FALSE;
}

static void closedown( Driver* dp ) {
    int                     result;

    SPI_exceptionHandlerPop();
    Accessible_unref( dp->desktop );
    Accessible_unref( dp->application );
    result = SPI_exit();
    if ( result != 0 )
        rb_raise( rb_eSystemCallError, "There were %d SPI memory leaks.", result );
}

static SPIBoolean exception_handler( SPIException* err, SPIBoolean is_fatal ) {
    char*   description;
    int     result;

    // We wish to swallow a non-fatal error...
    if ( is_fatal )
        return FALSE;       // We haven't dealt with it.

    // ...which has a specific description.
    description = SPIException_getDescription( err );
    result = strcmp( description, "IDL:omg.org/CORBA/COMM_FAILURE:1.0" );
    SPI_freeString( description );

    if ( result != 0 )
        return FALSE;       // We haven't dealt with it.
    else
        return TRUE;        // We have dealt with it -- ie ignore it.
}

// Memory handling

static void free_structure( Driver* dp ) {
    free( dp );
}

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

// Method wrapping

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
