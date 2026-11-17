/*
 * Project: FreeBASIC runtime tests
 * File: native-io-counts.c
 * Purpose: Verify bounded POSIX I/O and native socket address lengths.
 * Responsibilities: Observe production adapters without allocating large buffers.
 * Real loopback transfers belong to the existing tests/file/tcp.bas program.
 */
#include "devices/dev_tcp.c"

static unsigned long checks;
static size_t expected_length;
static ssize_t returned_count;
static int returned_error, io_calls;
static socklen_t expected_address_length;
static struct sockaddr_storage address;
static struct addrinfo resolved;

ssize_t __wrap_recv( int socket, void *buffer, size_t length, int flags );
ssize_t __wrap_send( int socket, const void *buffer, size_t length, int flags );
int __wrap_getaddrinfo( const char *host, const char *service, const struct addrinfo *hints, struct addrinfo **result );
void __wrap_freeaddrinfo( struct addrinfo *result );
int __wrap_socket( int domain, int type, int protocol );
int __wrap_connect( int socket, const struct sockaddr *value, socklen_t length );
int __wrap_bind( int socket, const struct sockaddr *value, socklen_t length );
int __wrap_listen( int socket, int backlog );
int __wrap_setsockopt( int socket, int level, int option, const void *value, socklen_t length );
int __wrap_ioctl( int socket, unsigned long request, ... );

static void check( int condition )
{
	checks++;
	if( !condition ) {
		fprintf( stderr, "NATIVE_IO_COUNTS_FAIL check=%lu\n", checks );
		exit(1);
	}
}

ssize_t __wrap_recv( int socket, void *buffer, size_t length, int flags )
{
	check( socket == 19 && buffer != NULL && flags == 0 );
	check( length == expected_length && length <= (size_t)INT_MAX );
	io_calls++;
	errno = returned_error;
	return returned_count;
}

ssize_t __wrap_send( int socket, const void *buffer, size_t length, int flags )
{
	check( socket == 19 && buffer != NULL && flags == 0 );
	check( length == expected_length && length <= (size_t)INT_MAX );
	io_calls++;
	errno = returned_error;
	return returned_count;
}

int __wrap_getaddrinfo( const char *host, const char *service, const struct addrinfo *hints, struct addrinfo **result )
{
	check( strcmp(host, "127.0.0.1") == 0 && strcmp(service, "42") == 0 );
	check( hints->ai_socktype == SOCK_STREAM );
	memset( &resolved, 0, sizeof(resolved) );
	resolved.ai_family = AF_INET;
	resolved.ai_socktype = SOCK_STREAM;
	resolved.ai_protocol = IPPROTO_TCP;
	resolved.ai_addr = (struct sockaddr *)&address;
	resolved.ai_addrlen = expected_address_length;
	*result = &resolved;
	return 0;
}

void __wrap_freeaddrinfo( struct addrinfo *result )
{
	check( result == &resolved );
}

int __wrap_socket( int domain, int type, int protocol )
{
	check( domain == AF_INET && type == SOCK_STREAM && protocol == IPPROTO_TCP );
	return 19;
}

int __wrap_connect( int socket, const struct sockaddr *value, socklen_t length )
{
	check( socket == 19 && value == (struct sockaddr *)&address );
	check( length == expected_address_length );
	return 0;
}

int __wrap_bind( int socket, const struct sockaddr *value, socklen_t length )
{
	check( socket == 19 && value == (struct sockaddr *)&address );
	check( length == expected_address_length );
	return 0;
}

int __wrap_listen( int socket, int backlog )
{
	check( socket == 19 && backlog == 1 );
	return 0;
}

int __wrap_setsockopt( int socket, int level, int option, const void *value, socklen_t length )
{
	(void)level;
	(void)option;
	check( socket == 19 && value != NULL && length > 0 );
	return 0;
}

int __wrap_ioctl( int socket, unsigned long request, ... )
{
	check( socket == 19 && request == FIONBIO );
	return 0;
}

FBCALL int fb_ErrorSetNum( int error )
{
	return error;
}

int main( void )
{
	const size_t lengths[] = { 0, 1, 16, (size_t)INT_MAX, (size_t)INT_MAX + 1u, SIZE_MAX };
	const socklen_t address_lengths[] = { 0, 1, (socklen_t)sizeof(address), (socklen_t)INT_MAX, (socklen_t)-1 };
	DEV_TCP_INFO info;
	DEV_TCP_PROTOCOL protocol;
	char buffer[16], host[] = "127.0.0.1";
	memset( &info, 0, sizeof(info) );
	info.hSocket = 19;
	for( size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++ ) {
		expected_length = MIN(lengths[i], (size_t)INT_MAX);
		for( int outcome = 0; outcome < 4; outcome++ ) {
			int error = 0;
			returned_count = outcome == 0 ? (ssize_t)expected_length : (outcome == 1 ? 0 : -1);
			returned_error = outcome == 2 ? EAGAIN : (outcome == 3 ? EINTR : 0);
			io_calls = 0;
			check( fb_hDevTcpRecvUnlocked(&info, buffer, lengths[i], 0, &error) == returned_count );
			check( error == returned_error && io_calls == 1 );
			check( fb_hDevTcpSendUnlocked(&info, buffer, lengths[i], 0, &error) == returned_count );
			check( error == returned_error && io_calls == 2 );
		}
	}
	memset( &protocol, 0, sizeof(protocol) );
	protocol.host = host;
	protocol.port = 42;
	protocol.backlog = 1;
	for( size_t i = 0; i < sizeof(address_lengths) / sizeof(address_lengths[0]); i++ ) {
		FB_TCP_SOCKET socket;
		expected_address_length = address_lengths[i];
		check( fb_hDevTcpCreateConnectedSocket(&protocol, &socket) == FB_RTERROR_OK );
		check( socket == 19 );
		check( fb_hDevTcpCreateServerSocket(&protocol, &socket) == FB_RTERROR_OK );
		check( socket == 19 );
	}
	fprintf( stderr, "NATIVE_IO_COUNTS_PASS checks=%lu\n", checks );
	return 0;
}

/* end of native-io-counts.c */
