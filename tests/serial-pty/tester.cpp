/*
    Project: FreeBASIC runtime serial integration tests
    --------------------------------------------------

    File: tester.cpp

    Purpose:

        Verify the Unix OPEN COM backend against a real kernel pseudo-terminal.

    Responsibilities:

        - allocate and seed a pseudo-terminal with hostile prior settings
        - run the FreeBASIC OPEN COM test program against the slave endpoint
        - inspect the termios state applied by the runtime
        - exchange binary data in both directions through the kernel TTY layer
        - verify that CLOSE restores the prior terminal configuration

    This file intentionally does NOT contain:

        - HART protocol behavior
        - physical serial-device assumptions
        - FreeBASIC runtime implementation details
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace {

constexpr int TEST_TIMEOUT_MS = 7000;

const unsigned char OUTBOUND_BYTES[] = {
    0xff, 0x80, 0x11, 0x13, 0x00, 0x7f, 0x02
};

const unsigned char INBOUND_BYTES[] = {
    0xfe, 0x81, 0x11, 0x13, 0x00, 0x55, 0xaa
};


bool wait_for_fd( int fd, short events, int timeout_ms )
{
    struct pollfd descriptor;
    int result;

    descriptor.fd = fd;
    descriptor.events = events;
    descriptor.revents = 0;

    do {
        result = poll( &descriptor, 1, timeout_ms );
    } while( (result < 0) && (errno == EINTR) );

    return (result > 0) && ((descriptor.revents & events) != 0);
}


bool read_exact( int fd, const unsigned char *expected, size_t length )
{
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds( TEST_TIMEOUT_MS );
    unsigned char received[sizeof( OUTBOUND_BYTES )];
    size_t offset = 0;

    if( length > sizeof( received ) )
        return false;

    while( offset < length ) {
        const auto now = std::chrono::steady_clock::now();
        ssize_t count;

        if( now >= deadline )
            return false;

        if( !wait_for_fd( fd, POLLIN,
            static_cast<int>( std::chrono::duration_cast<
                std::chrono::milliseconds >( deadline - now ).count() ) ) )
            continue;

        count = read( fd, received + offset, length - offset );
        if( count > 0 ) {
            offset += static_cast<size_t>( count );
            continue;
        }

        if( (count < 0) && ((errno == EINTR) || (errno == EAGAIN) ||
            (errno == EIO)) )
            continue;

        return false;
    }

    return memcmp( received, expected, length ) == 0;
}


bool write_all( int fd, const unsigned char *data, size_t length )
{
    size_t offset = 0;

    while( offset < length ) {
        ssize_t count;

        if( !wait_for_fd( fd, POLLOUT, TEST_TIMEOUT_MS ) )
            return false;

        count = write( fd, data + offset, length - offset );
        if( count > 0 ) {
            offset += static_cast<size_t>( count );
            continue;
        }

        if( (count < 0) && ((errno == EINTR) || (errno == EAGAIN)) )
            continue;

        return false;
    }

    return true;
}


bool same_restored_state( const struct termios &expected,
    const struct termios &actual )
{
    const tcflag_t input_mask = ISTRIP | IXON | IXOFF | IXANY;
    tcflag_t control_mask = CSIZE | PARENB | PARODD | CSTOPB | CLOCAL | HUPCL;

#ifdef CRTSCTS
    control_mask |= CRTSCTS;
#endif

    return (cfgetispeed( &expected ) == cfgetispeed( &actual )) &&
        (cfgetospeed( &expected ) == cfgetospeed( &actual )) &&
        ((expected.c_iflag & input_mask) == (actual.c_iflag & input_mask)) &&
        ((expected.c_cflag & control_mask) ==
            (actual.c_cflag & control_mask));
}


bool configured_state_is_correct( const struct termios &state )
{
    tcflag_t forbidden_input = ISTRIP | IXON | IXOFF | IXANY;

    if( (cfgetispeed( &state ) != B1200) ||
        (cfgetospeed( &state ) != B1200) )
        return false;

    if( (state.c_iflag & forbidden_input) != 0 )
        return false;

    if( (state.c_cflag & CSIZE) != CS8 )
        return false;

    if( (state.c_cflag & CSTOPB) != 0 )
        return false;

    if( (state.c_cflag & CLOCAL) == 0 )
        return false;

    if( (state.c_cflag & HUPCL) != 0 )
        return false;

#ifdef CRTSCTS
    if( (state.c_cflag & CRTSCTS) != 0 )
        return false;
#endif

    return true;
}


int finish_child( pid_t child, bool terminate )
{
    int status = 0;

    if( terminate )
        kill( child, SIGKILL );

    while( waitpid( child, &status, 0 ) < 0 ) {
        if( errno != EINTR )
            return -1;
    }

    if( !WIFEXITED( status ) )
        return -1;

    return WEXITSTATUS( status );
}

} // namespace


int main( int argc, char **argv )
{
    const char *testee = (argc > 1) ? argv[1] : "./testee-open-com";
    struct termios original_state;
    struct termios seeded_state;
    struct termios configured_state;
    struct termios restored_state;
    char *slave_name;
    pid_t child;
    int master_fd;
    int slave_fd;
    int child_result;

    master_fd = posix_openpt( O_RDWR | O_NOCTTY | O_NONBLOCK );
    if( master_fd < 0 ) {
        perror( "posix_openpt" );
        return 1;
    }

    if( (grantpt( master_fd ) != 0) || (unlockpt( master_fd ) != 0) ) {
        perror( "grantpt/unlockpt" );
        close( master_fd );
        return 1;
    }

    slave_name = ptsname( master_fd );
    if( slave_name == nullptr ) {
        perror( "ptsname" );
        close( master_fd );
        return 1;
    }

    slave_fd = open( slave_name, O_RDWR | O_NOCTTY );
    if( slave_fd < 0 ) {
        perror( "open slave" );
        close( master_fd );
        return 1;
    }

    if( tcgetattr( slave_fd, &original_state ) != 0 ) {
        perror( "tcgetattr original" );
        close( slave_fd );
        close( master_fd );
        return 1;
    }

    seeded_state = original_state;
    seeded_state.c_iflag |= ISTRIP | IXON | IXOFF | IXANY;
    seeded_state.c_cflag |= HUPCL;
#ifdef CRTSCTS
    seeded_state.c_cflag |= CRTSCTS;
#endif
    if( (cfsetispeed( &seeded_state, B9600 ) != 0) ||
        (cfsetospeed( &seeded_state, B9600 ) != 0) ||
        (tcsetattr( slave_fd, TCSANOW, &seeded_state ) != 0) ||
        (tcgetattr( slave_fd, &seeded_state ) != 0) ) {
        perror( "seed termios" );
        close( slave_fd );
        close( master_fd );
        return 1;
    }

    child = fork();
    if( child < 0 ) {
        perror( "fork" );
        close( slave_fd );
        close( master_fd );
        return 1;
    }

    if( child == 0 ) {
        close( slave_fd );
        close( master_fd );
        execl( testee, testee, slave_name, static_cast<char *>( nullptr ) );
        _exit( 127 );
    }

    if( !read_exact( master_fd, OUTBOUND_BYTES, sizeof( OUTBOUND_BYTES ) ) ) {
        fprintf( stderr, "OPEN COM did not preserve outbound binary bytes\n" );
        finish_child( child, true );
        tcsetattr( slave_fd, TCSANOW, &original_state );
        close( slave_fd );
        close( master_fd );
        return 1;
    }

    if( tcgetattr( slave_fd, &configured_state ) != 0 ) {
        perror( "tcgetattr configured" );
        finish_child( child, true );
        tcsetattr( slave_fd, TCSANOW, &original_state );
        close( slave_fd );
        close( master_fd );
        return 1;
    }

    if( !configured_state_is_correct( configured_state ) ) {
        fprintf( stderr, "OPEN COM applied incorrect termios settings\n" );
        finish_child( child, true );
        tcsetattr( slave_fd, TCSANOW, &original_state );
        close( slave_fd );
        close( master_fd );
        return 1;
    }

    if( !write_all( master_fd, INBOUND_BYTES, sizeof( INBOUND_BYTES ) ) ) {
        fprintf( stderr, "could not provide inbound binary bytes\n" );
        finish_child( child, true );
        tcsetattr( slave_fd, TCSANOW, &original_state );
        close( slave_fd );
        close( master_fd );
        return 1;
    }

    child_result = finish_child( child, false );
    if( child_result != 0 ) {
        fprintf( stderr, "OPEN COM testee failed with status %d\n", child_result );
        tcsetattr( slave_fd, TCSANOW, &original_state );
        close( slave_fd );
        close( master_fd );
        return 1;
    }

    if( tcgetattr( slave_fd, &restored_state ) != 0 ) {
        perror( "tcgetattr restored" );
        tcsetattr( slave_fd, TCSANOW, &original_state );
        close( slave_fd );
        close( master_fd );
        return 1;
    }

    if( !same_restored_state( seeded_state, restored_state ) ) {
        fprintf( stderr, "CLOSE did not restore the prior termios settings\n" );
        tcsetattr( slave_fd, TCSANOW, &original_state );
        close( slave_fd );
        close( master_fd );
        return 1;
    }

    tcsetattr( slave_fd, TCSANOW, &original_state );
    close( slave_fd );
    close( master_fd );
    puts( "PASS: POSIX OPEN COM pseudo-terminal integration" );
    return 0;
}

/* end of tester.cpp */
