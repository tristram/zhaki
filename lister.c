#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cspi/spi.h>

static void list_applications();
static void check_result( char* failure_message );
static void enumerate_desktop();
static void enumerate_children( Accessible* parent );
static SPIBoolean exception_handler( SPIException* err, SPIBoolean is_fatal );
static void describe_child( int slot, Accessible* child );

int main( int argc, char** argv ) {
    list_applications();
    return 0;
}

int result;

static void list_applications() {
    result = SPI_init();
    if ( result == 2 ) {
        printf( "SPI_init returned error 2.\n" );
        printf( "Are Assistive Technologies enabled?\n" );
        exit( 1 );
    }
    check_result( "SPI_init returned error %d!\n" );

    enumerate_desktop();

    result = SPI_exit();
    check_result( "There were %d SPI memory leaks.\n" );
}

static void check_result( char* error_message ) {
    if ( result != 0 ) {
        printf( error_message, result );
        exit( 1 );
    }
}

static void enumerate_desktop() {
    Accessible*             desktop;

    result = SPI_getDesktopCount();
    if ( result != 1 )
        check_result( "There are %d desktops.  Expected to find one.\n" );

    desktop = SPI_getDesktop( 0 );
    enumerate_children( desktop );
    Accessible_unref( desktop );
}

static void enumerate_children( Accessible* parent ) {
    int                     child_count;
    SPIExceptionHandler     handler;
    Accessible*             child;

    child_count = Accessible_getChildCount( parent );
    if ( child_count == 0 ) {
        printf( "The Desktop has no children!\n" );
        printf( "Have you re-logged in after enabling Assistive Technologies?\n" );
        exit( 1 );
    }

    handler = exception_handler;
    if ( ! SPI_exceptionHandlerPush(&handler) ) {
        printf( "Failed to add an ExceptionHandler.\n" );
        exit( 1 );
    }

    for ( int i = 0; i < child_count; ++i ) {
        child = Accessible_getChildAtIndex( parent, i );
        describe_child( i, child );
        Accessible_unref( child );
    }

    SPI_exceptionHandlerPop();
}

static SPIBoolean exception_handler( SPIException* err, SPIBoolean is_fatal ) {
    char*   description;

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

static void describe_child( int slot, Accessible* child ) {
    char*       name;

    if ( child == NULL ) {
        printf( "App %2d is missing (NULL).\n", slot );
        return;
    }

    name = Accessible_getName( child );
    printf( "App %2d: '%s'\n", slot, name );
    SPI_freeString( name );

    if ( ! Accessible_isApplication(child) )
        printf( "   NOT AN APPLICATION!\n\n" );
}
