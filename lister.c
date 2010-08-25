#include <stdio.h>
#include <stdlib.h>

#include <cspi/spi.h>

static void list_applications();
static void check_result( char* failure_message );
static void enumerate_desktop();
static void enumerate_children( Accessible* parent );
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
    Accessible*             child;

    child_count = Accessible_getChildCount( parent );
    if ( child_count == 0 ) {
        printf( "The Desktop has no children!\n" );
        printf( "Have you rebooted after enabling Assistive Technologies?\n" );
        exit( 1 );
    }
    
    for ( int i = 0; i < child_count; ++i ) {
        child = Accessible_getChildAtIndex( parent, i );
        describe_child( i, child );
        Accessible_unref( child );
    }
}

static void describe_child( int slot, Accessible* child ) {
    char* name = Accessible_getName( child );
    printf( "App %2d: '%s'\n", slot, name );
    SPI_freeString( name );

    if ( ! Accessible_isApplication(child) )
        printf( "   NOT AN APPLICATION!\n\n" );
}
