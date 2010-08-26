#include "ruby.h"

#include <cspi/spi.h>

// Declarations

typedef struct {
    int                 result;
    Accessible*         desktop;
    int                 child_count;
    SPIExceptionHandler handler;
    int                 next;
} List;

static void initialize( List* lp );
static void check_result( List* lp, char* error_message );
static void closedown( List* lp );
static SPIBoolean exception_handler( SPIException* err, SPIBoolean is_fatal );
static int count( List* lp );
static VALUE next( List* lp );
static void free_structure( List* lp );
static VALUE allocate_structure( VALUE class_name );
static List* retrieve_structure( VALUE self );
static VALUE wrap_initialize( VALUE self );
static VALUE wrap_closedown( VALUE self );
static VALUE wrap_count( VALUE self );
static VALUE wrap_next( VALUE self );

// C functions

static void initialize( List* lp ) {
    lp->result = SPI_init();
    if ( lp->result == 2 )
        check_result( lp, "Assistive Technologies not enabled." );
    check_result( lp, "SPI_init returned error %d!" );

    lp->result = SPI_getDesktopCount();
    if ( lp->result != 1 )
        rb_raise( rb_eRuntimeError,
                "There are %d desktops.  Expected to find one.\n", lp->result );

    lp->desktop = SPI_getDesktop( 0 );

    lp->child_count = Accessible_getChildCount( lp->desktop );
    if ( lp->child_count == 0 )
        rb_raise( rb_eRuntimeError,
                "The Desktop has no children! Have you re-logged in?" );

    lp->handler = exception_handler;
    if ( ! SPI_exceptionHandlerPush(&lp->handler) )
        rb_raise( rb_eRuntimeError, "Failed to add an ExceptionHandler." );
}

static void check_result( List* lp, char* error_message ) {
    if ( lp->result != 0 ) {
        SPI_exit();     // try to tidy up -- ignore errors
        rb_raise( rb_eRuntimeError, error_message, lp->result );
    }
}

static void closedown( List* lp ) {
    if ( lp->handler )
        SPI_exceptionHandlerPop();
    if ( lp->desktop )
        Accessible_unref( lp->desktop );
    lp->result = SPI_exit();
    check_result( lp, "There were %d SPI memory leaks.\n" );
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

static int count( List* lp ) {
    return lp->child_count;
}

// C functions with knowledge of Ruby

static VALUE next( List* lp ) {
    Accessible*             child;
    VALUE                   name;

    if ( lp->next >= lp->child_count )
        name = rb_str_new2( "no more apps" );
    else {
        child = Accessible_getChildAtIndex( lp->desktop, lp->next );
        if ( child == NULL )
            name = rb_str_new2( "missing (NULL)" );
        else {
            char* child_name = Accessible_getName( child );
            name = rb_str_new2( child_name );
            SPI_freeString( child_name );
        }
        Accessible_unref( child );
        ++lp->next;
    }
    return name;
}

// Memory handling

static void free_structure( List* lp ) {
    free( lp );
}

static VALUE allocate_structure( VALUE class_name ) {
    List* lp = calloc( 1, sizeof(List) );    // guaranteed zeroed memory
    return Data_Wrap_Struct(
        class_name,         // class owning this memory
        0,                  // no mark routine required
        free_structure,     // function called when garbage collected
        lp
    );
}

static List* retrieve_structure( VALUE self ) {
    List* lp;
    Data_Get_Struct( self, List, lp );
    return lp;
}

// Method wrapping

static VALUE wrap_initialize( VALUE self ) {
    List* lp = retrieve_structure( self );
    initialize( lp );
    return Qnil;
}

static VALUE wrap_closedown( VALUE self ) {
    List* lp = retrieve_structure( self );
    closedown( lp );
    return Qnil;
}

static VALUE wrap_count( VALUE self ) {
    List* lp = retrieve_structure( self );
    return INT2FIX( count(lp) );
}

static VALUE wrap_next( VALUE self ) {
    List* lp = retrieve_structure( self );
    return next( lp );
}

// Export methods

void Init_application_list() {
    VALUE list = rb_define_class( "ApplicationList", rb_cObject );
    rb_define_alloc_func( list, allocate_structure );
    rb_define_method( list, "initialize", wrap_initialize, 0 );
    rb_define_method( list, "closedown", wrap_closedown, 0 );
    rb_define_method( list, "count", wrap_count, 0 );
    rb_define_method( list, "next", wrap_next, 0 );
}
